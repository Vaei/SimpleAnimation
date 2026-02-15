// Copyright (c) Jared Taylor.


#include "BlendTransformModifier.h"

#include "AnimPose.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlendTransformModifier)

#define LOCTEXT_NAMESPACE "BlendTransformModifier"

void UBlendTransformModifier::OnApply_Implementation(UAnimSequence* Animation)
{
	if (!Animation)
	{
		return;
	}

	USkeleton* Skeleton = Animation->GetSkeleton();
	if (!Skeleton)
	{
		UE_LOG(LogAnimation, Error, TEXT("BlendTransformModifier failed. Reason: Target animation has no skeleton. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	IAnimationDataController& Controller = Animation->GetController();
	const IAnimationDataModel* Model = Animation->GetDataModel();

	if (Model == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("BlendTransformModifier failed. Reason: Invalid Data Model. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	// Gather bone track names in target animation
	TArray<FName> TrackNames;
	Model->GetBoneTrackNames(TrackNames);

	// Resolve blend profile if needed
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	UBlendProfile* BlendProfile = nullptr;
	if (BoneWeightSource == EBoneWeightSource::BlendProfile && !BlendProfileName.IsNone())
	{
		BlendProfile = Skeleton->GetBlendProfile(BlendProfileName);
		if (!BlendProfile)
		{
			UE_LOG(LogAnimation, Error, TEXT("BlendTransformModifier failed. Reason: BlendProfile '%s' not found on skeleton. Animation: %s"), *BlendProfileName.ToString(), *GetNameSafe(Animation));
			return;
		}
	}

	// Pre-compute values from the input transform
	const FVector TranslationValue = Transform.GetLocation();
	const FQuat RotationValue = Transform.GetRotation();
	const FVector ScaleValue = Transform.GetScale3D();
	const FVector ScaleDelta = ScaleValue - FVector::OneVector;

	// Build per-bone weight cache
	struct FBoneWeightEntry
	{
		FName BoneName;
		float Weight;
	};

	TArray<FBoneWeightEntry> BoneCache;
	BoneCache.Reserve(TrackNames.Num());

	for (const FName& BoneName : TrackNames)
	{
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

		BoneCache.Add({ BoneName, Weight });
	}

	if (BoneCache.IsEmpty())
	{
		UE_LOG(LogAnimation, Warning, TEXT("BlendTransformModifier: No bones with non-zero weight to blend. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	// Temporally set ForceRootLock to true so we get the correct transforms regardless of root motion configuration
	TGuardValue<bool> ForceRootLockGuard(Animation->bForceRootLock, true);

	// Start editing animation data
	constexpr bool bShouldTransact = false;
	Controller.OpenBracket(LOCTEXT("BlendTransformModifier_Bracket", "Blending transform into animation"), bShouldTransact);

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
			const FBoneWeightEntry& Entry = BoneCache[BoneIdx];
			const FTransform& OrigLocal = OriginalTransforms[FrameIdx * NumBones + BoneIdx];

			const FQuat OrigRotation = OrigLocal.GetRotation();
			FVector NewTranslation = OrigLocal.GetLocation();
			FQuat NewRotation = OrigRotation;
			FVector NewScale = OrigLocal.GetScale3D();

			if (bBlendTranslation)
			{
				NewTranslation += TranslationValue * Entry.Weight;
			}
			if (bBlendRotation)
			{
				// Pre-multiply: WeightedDelta * Original
				NewRotation = FQuat::Slerp(FQuat::Identity, RotationValue, Entry.Weight) * NewRotation;
			}
			if (bBlendScale)
			{
				NewScale *= (FVector::OneVector + ScaleDelta * Entry.Weight);
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
			Controller.UpdateBoneTrackKeys(Entry.BoneName, KeyRange,
				{ NewTranslation },
				{ NewRotation },
				{ NewScale });
		}
	}

	// Done editing animation data
	Controller.CloseBracket(bShouldTransact);
}

#undef LOCTEXT_NAMESPACE
