// Copyright (c) Jared Taylor.


#include "BlendPoseModifier.h"

#include "AnimPose.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendProfile.h"
#include "Logging/MessageLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlendPoseModifier)

#define LOCTEXT_NAMESPACE "BlendPoseModifier"

void UBlendPoseModifier::OnApply_Implementation(UAnimSequence* Animation)
{
	if (!Animation)
	{
		return;
	}

	if (!SourceAnimation)
	{
		UE_LOG(LogAnimation, Error, TEXT("BlendPoseModifier failed. Reason: No Source Animation set. Target: %s"), *GetNameSafe(Animation));
		return;
	}

	USkeleton* Skeleton = Animation->GetSkeleton();
	if (!Skeleton)
	{
		UE_LOG(LogAnimation, Error, TEXT("BlendPoseModifier failed. Reason: Target animation has no skeleton. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	if (SourceAnimation->GetSkeleton() != Skeleton)
	{
		UE_LOG(LogAnimation, Error, TEXT("BlendPoseModifier failed. Reason: Source and target animations must share the same skeleton. Source: %s, Target: %s"),
			*GetNameSafe(SourceAnimation), *GetNameSafe(Animation));
		return;
	}

	IAnimationDataController& Controller = Animation->GetController();
	const IAnimationDataModel* Model = Animation->GetDataModel();

	if (Model == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("BlendPoseModifier failed. Reason: Invalid Data Model. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	// Sample source pose once
	const double ClampedSourceTime = FMath::Clamp(SourceTime, 0.0, static_cast<double>(SourceAnimation->GetPlayLength()));
	FAnimPose SourcePose;
	UAnimPoseExtensions::GetAnimPoseAtTime(SourceAnimation, ClampedSourceTime, FAnimPoseEvaluationOptions(), SourcePose);
	if (!SourcePose.IsValid())
	{
		UE_LOG(LogAnimation, Error, TEXT("BlendPoseModifier failed. Reason: Could not evaluate source pose. Source: %s"), *GetNameSafe(SourceAnimation));
		return;
	}

	// For additive mode, validate source animation's additive settings and retrieve reference pose
	FAnimPose RefPose;
	bool bUseSkeletonRefPose = false;

	if (BlendMode == EBlendPoseMode::Additive)
	{
		FMessageLog MessageLog("AnimationModifiers");

		if (SourceAnimation->AdditiveAnimType == AAT_None)
		{
			MessageLog.Error(FText::Format(
				LOCTEXT("AdditiveTypeMismatch",
					"BlendPoseModifier: BlendMode is Additive but source animation '{0}' is not set to an additive type. Configure it in the animation's Additive Settings."),
				FText::FromString(GetNameSafe(SourceAnimation))));
			return;
		}

		// Evaluate reference pose based on the animation's configured RefPoseType
		switch (SourceAnimation->RefPoseType)
		{
		case ABPT_RefPose:
			bUseSkeletonRefPose = true;
			break;

		case ABPT_AnimFrame:
		{
			if (!SourceAnimation->RefPoseSeq)
			{
				MessageLog.Error(FText::Format(
					LOCTEXT("MissingRefPoseSeq",
						"BlendPoseModifier: Source animation '{0}' uses 'Selected animation frame' base pose type but has no Base Pose Animation set."),
					FText::FromString(GetNameSafe(SourceAnimation))));
				return;
			}
			const int32 ClampedRefFrame = FMath::Clamp(SourceAnimation->RefFrameIndex, 0,
				FMath::Max(0, SourceAnimation->RefPoseSeq->GetDataModel()->GetNumberOfKeys() - 1));
			UAnimPoseExtensions::GetAnimPoseAtFrame(SourceAnimation->RefPoseSeq, ClampedRefFrame, FAnimPoseEvaluationOptions(), RefPose);
			break;
		}

		case ABPT_LocalAnimFrame:
		{
			const int32 ClampedRefFrame = FMath::Clamp(SourceAnimation->RefFrameIndex, 0,
				FMath::Max(0, SourceAnimation->GetDataModel()->GetNumberOfKeys() - 1));
			UAnimPoseExtensions::GetAnimPoseAtFrame(SourceAnimation, ClampedRefFrame, FAnimPoseEvaluationOptions(), RefPose);
			break;
		}

		case ABPT_AnimScaled:
		{
			UAnimSequence* RefSeq = SourceAnimation->RefPoseSeq ? SourceAnimation->RefPoseSeq : SourceAnimation;
			const double SourceLength = SourceAnimation->GetPlayLength();
			const double RefTime = (SourceLength > SMALL_NUMBER)
				? RefSeq->GetPlayLength() * (ClampedSourceTime / SourceLength)
				: 0.0;
			UAnimPoseExtensions::GetAnimPoseAtTime(RefSeq, RefTime, FAnimPoseEvaluationOptions(), RefPose);
			break;
		}

		default:
			MessageLog.Error(FText::Format(
				LOCTEXT("UnsupportedRefPoseType",
					"BlendPoseModifier: Source animation '{0}' has an unsupported Base Pose Type."),
				FText::FromString(GetNameSafe(SourceAnimation))));
			return;
		}

		if (!bUseSkeletonRefPose && !RefPose.IsValid())
		{
			MessageLog.Error(FText::Format(
				LOCTEXT("InvalidRefPose",
					"BlendPoseModifier: Could not evaluate reference pose for source animation '{0}'."),
				FText::FromString(GetNameSafe(SourceAnimation))));
			return;
		}
	}

	// Helper to get reference bone local transform from the correct source
	auto GetRefBoneLocal = [&](const FName& BoneName) -> FTransform
	{
		if (bUseSkeletonRefPose)
		{
			return UAnimPoseExtensions::GetRefBonePose(SourcePose, BoneName, EAnimPoseSpaces::Local);
		}
		return UAnimPoseExtensions::GetBonePose(RefPose, BoneName, EAnimPoseSpaces::Local);
	};

	// Gather bone names present in source pose
	TArray<FName> SourceBoneNames;
	UAnimPoseExtensions::GetBoneNames(SourcePose, SourceBoneNames);
	const TSet<FName> SourceBoneNameSet(SourceBoneNames);

	// Gather bone track names in target animation
	TArray<FName> TargetTrackNames;
	Model->GetBoneTrackNames(TargetTrackNames);

	// Resolve blend profile if needed
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	UBlendProfile* BlendProfile = nullptr;
	if (BoneWeightSource == EBoneWeightSource::BlendProfile && !BlendProfileName.IsNone())
	{
		BlendProfile = Skeleton->GetBlendProfile(BlendProfileName);
		if (!BlendProfile)
		{
			UE_LOG(LogAnimation, Error, TEXT("BlendPoseModifier failed. Reason: BlendProfile '%s' not found on skeleton. Animation: %s"), *BlendProfileName.ToString(), *GetNameSafe(Animation));
			return;
		}
	}

	// Build per-bone data: source transforms, weights, and additive deltas
	struct FBlendBoneCache
	{
		FName BoneName;
		float Weight;
		// Source pose in local space (used by Override mode)
		FVector Translation;
		FQuat Rotation;
		FVector Scale;
		// Additive deltas (used by Additive mode)
		FVector TranslationDelta;		// Source - Ref
		FQuat RotationDelta;			// Source * Ref^-1
		FVector ScaleDelta;				// (Source / Ref) - 1
	};

	TArray<FBlendBoneCache> BoneCache;
	BoneCache.Reserve(TargetTrackNames.Num());

	for (const FName& BoneName : TargetTrackNames)
	{
		// Skip bones not present in source pose
		if (!SourceBoneNameSet.Contains(BoneName))
		{
			continue;
		}

		// Compute per-bone weight
		float Weight = BlendAlpha;
		switch (BoneWeightSource)
		{
		case EBoneWeightSource::BlendProfile:
			if (BlendProfile)
			{
				const int32 BoneIdx = RefSkeleton.FindBoneIndex(BoneName);
				if (BoneIdx != INDEX_NONE)
				{
					Weight *= BlendProfile->GetBoneBlendScale(BoneIdx);
				}
			}
			break;
		case EBoneWeightSource::PerBone:
			if (const float* BoneWeight = BoneWeights.Find(BoneName))
			{
				Weight *= *BoneWeight;
			}
			else
			{
				Weight = 0.0f;
			}
			break;
		default: // Uniform
			break;
		}

		if (Weight <= 0.0f)
		{
			continue;
		}

		FBlendBoneCache Cache;
		Cache.BoneName = BoneName;
		Cache.Weight = Weight;

		const FTransform& SourceLocal = UAnimPoseExtensions::GetBonePose(SourcePose, BoneName, EAnimPoseSpaces::Local);
		Cache.Translation = SourceLocal.GetLocation();
		Cache.Rotation = SourceLocal.GetRotation();
		Cache.Scale = SourceLocal.GetScale3D();

		if (BlendMode == EBlendPoseMode::Additive)
		{
			const FTransform RefLocal = GetRefBoneLocal(BoneName);

			// Translation delta: Source - Ref
			Cache.TranslationDelta = Cache.Translation - RefLocal.GetLocation();

			// Rotation delta in local space: Source * Ref^-1
			// Matches FAnimationRuntime::ConvertTransformToAdditive
			Cache.RotationDelta = (Cache.Rotation * RefLocal.GetRotation().Inverse()).GetNormalized();

			// Scale delta: (Source / Ref) - 1, applied as Scale *= (1 + Delta * Weight)
			// Matches FAnimationRuntime::ConvertTransformToAdditive
			const FVector RefScale = RefLocal.GetScale3D();
			Cache.ScaleDelta = FVector(
				FMath::IsNearlyZero(RefScale.X) ? 0.0 : (Cache.Scale.X / RefScale.X) - 1.0,
				FMath::IsNearlyZero(RefScale.Y) ? 0.0 : (Cache.Scale.Y / RefScale.Y) - 1.0,
				FMath::IsNearlyZero(RefScale.Z) ? 0.0 : (Cache.Scale.Z / RefScale.Z) - 1.0
			);
		}

		BoneCache.Add(Cache);
	}

	if (BoneCache.IsEmpty())
	{
		UE_LOG(LogAnimation, Warning, TEXT("BlendPoseModifier: No bones to blend between source and target. Source: %s, Target: %s"),
			*GetNameSafe(SourceAnimation), *GetNameSafe(Animation));
		return;
	}

	// Temporally set ForceRootLock to true so we get the correct transforms regardless of root motion configuration
	TGuardValue<bool> ForceRootLockGuard(Animation->bForceRootLock, true);

	// Start editing animation data
	constexpr bool bShouldTransact = false;
	Controller.OpenBracket(LOCTEXT("BlendPoseModifier_Bracket", "Blending pose into animation"), bShouldTransact);

	const int32 NumKeys = Model->GetNumberOfKeys();
	const int32 NumBones = BoneCache.Num();

	// Snapshot all original bone transforms before modifying the data model,
	// because UpdateBoneTrackKeys mutates the model and subsequent
	// GetAnimPoseAtFrame calls would read contaminated data
	TArray<FTransform> OriginalTransforms;
	OriginalTransforms.SetNumUninitialized(NumKeys * NumBones);

	for (int32 FrameIdx = 0; FrameIdx < NumKeys; FrameIdx++)
	{
		FAnimPose TargetPose;
		UAnimPoseExtensions::GetAnimPoseAtFrame(Animation, FrameIdx, FAnimPoseEvaluationOptions(), TargetPose);

		for (int32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
		{
			OriginalTransforms[FrameIdx * NumBones + BoneIdx] =
				UAnimPoseExtensions::GetBonePose(TargetPose, BoneCache[BoneIdx].BoneName, EAnimPoseSpaces::Local);
		}
	}

	// Apply blended transforms using the snapshot
	for (int32 FrameIdx = 0; FrameIdx < NumKeys; FrameIdx++)
	{
		for (int32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
		{
			const FBlendBoneCache& Cache = BoneCache[BoneIdx];
			const FTransform& OrigLocal = OriginalTransforms[FrameIdx * NumBones + BoneIdx];

			const FQuat OrigRotation = OrigLocal.GetRotation();
			FVector NewTranslation = OrigLocal.GetLocation();
			FQuat NewRotation = OrigRotation;
			FVector NewScale = OrigLocal.GetScale3D();

			if (BlendMode == EBlendPoseMode::Override)
			{
				if (bBlendTranslation)
				{
					NewTranslation = FMath::Lerp(NewTranslation, Cache.Translation, Cache.Weight);
				}
				if (bBlendRotation)
				{
					NewRotation = FQuat::Slerp(NewRotation, Cache.Rotation, Cache.Weight);
				}
				if (bBlendScale)
				{
					NewScale = FMath::Lerp(NewScale, Cache.Scale, Cache.Weight);
				}
			}
			else // Additive
			{
				// Matches FAnimationRuntime::AccumulateLocalSpaceAdditivePose
				if (bBlendTranslation)
				{
					NewTranslation += Cache.TranslationDelta * Cache.Weight;
				}
				if (bBlendRotation)
				{
					// Pre-multiply: WeightedDelta * Original
					NewRotation = FQuat::Slerp(FQuat::Identity, Cache.RotationDelta, Cache.Weight) * NewRotation;
				}
				if (bBlendScale)
				{
					NewScale *= (FVector::OneVector + Cache.ScaleDelta * Cache.Weight);
				}
			}

			// Preserve the original key's quaternion hemisphere so that the
			// animation curve's tangent/interpolation convention is maintained.
			// q and -q are the same rotation, but flipping sign relative to
			// the original key breaks the curve between neighboring keys.
			if ((NewRotation | OrigRotation) < 0.0f)
			{
				NewRotation = -NewRotation;
			}

			const FInt32Range KeyRange(FrameIdx, FrameIdx + 1);
			Controller.UpdateBoneTrackKeys(Cache.BoneName, KeyRange,
				{ NewTranslation },
				{ NewRotation },
				{ NewScale });
		}
	}

	// Done editing animation data
	Controller.CloseBracket(bShouldTransact);
}

#undef LOCTEXT_NAMESPACE
