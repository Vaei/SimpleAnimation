// Copyright (c) Jared Taylor.


#include "RootMotionAxisModifier.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RootMotionAxisModifier)

#define LOCTEXT_NAMESPACE "RootMotionAxisModifier"

namespace RootMotionAxisModifier
{
	static double FilterAxis(ERootMotionAxisFilter Filter, double Original, double First, double Last, double Alpha)
	{
		switch (Filter)
		{
		case ERootMotionAxisFilter::Remove:		return First;
		case ERootMotionAxisFilter::Straighten:	return FMath::Lerp(First, Last, Alpha);
		default:								return Original;
		}
	}
}

void URootMotionAxisModifier::OnApply_Implementation(UAnimSequence* Animation)
{
	if (!Animation)
	{
		return;
	}

	const USkeleton* Skeleton = Animation->GetSkeleton();
	if (!Skeleton)
	{
		UE_LOG(LogAnimation, Error, TEXT("RootMotionAxisModifier failed. Reason: Target animation has no skeleton. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	IAnimationDataController& Controller = Animation->GetController();
	const IAnimationDataModel* Model = Animation->GetDataModel();
	if (!Model)
	{
		UE_LOG(LogAnimation, Error, TEXT("RootMotionAxisModifier failed. Reason: Invalid Data Model. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	if (RefSkeleton.GetNum() == 0)
	{
		UE_LOG(LogAnimation, Error, TEXT("RootMotionAxisModifier failed. Reason: Invalid Ref Skeleton. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	const FName RootBoneName = RefSkeleton.GetBoneName(0);
	if (!Model->IsValidBoneTrackName(RootBoneName))
	{
		UE_LOG(LogAnimation, Error, TEXT("RootMotionAxisModifier failed. Reason: Animation has no root bone track. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	const bool bFiltersAnything =
		TranslationX != ERootMotionAxisFilter::Keep || TranslationY != ERootMotionAxisFilter::Keep || TranslationZ != ERootMotionAxisFilter::Keep ||
		Roll != ERootMotionAxisFilter::Keep || Pitch != ERootMotionAxisFilter::Keep || Yaw != ERootMotionAxisFilter::Keep;
	if (!bFiltersAnything)
	{
		UE_LOG(LogAnimation, Warning, TEXT("RootMotionAxisModifier: Every axis is set to Keep, nothing to do. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	const int32 NumKeys = Model->GetNumberOfKeys();
	if (NumKeys < 2)
	{
		UE_LOG(LogAnimation, Warning, TEXT("RootMotionAxisModifier: Animation has fewer than two keys, nothing to do. Animation: %s"), *GetNameSafe(Animation));
		return;
	}

	// The bone that takes back whatever the root gives up, so the visible pose survives the edit
	FName CompensationBoneName = NAME_None;
	if (bPreservePose)
	{
		CompensationBoneName = CompensationBone.BoneName;
		if (CompensationBoneName.IsNone())
		{
			for (int32 BoneIdx = 1; BoneIdx < RefSkeleton.GetNum(); BoneIdx++)
			{
				if (RefSkeleton.GetParentIndex(BoneIdx) == 0)
				{
					CompensationBoneName = RefSkeleton.GetBoneName(BoneIdx);
					break;
				}
			}
		}

		if (CompensationBoneName.IsNone() || !Model->IsValidBoneTrackName(CompensationBoneName))
		{
			UE_LOG(LogAnimation, Error, TEXT("RootMotionAxisModifier failed. Reason: Compensation bone %s has no track in the animation. Animation: %s"), *CompensationBoneName.ToString(), *GetNameSafe(Animation));
			return;
		}

		if (RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(CompensationBoneName)) != 0)
		{
			UE_LOG(LogAnimation, Error, TEXT("RootMotionAxisModifier failed. Reason: Compensation bone %s is not a direct child of the root. Animation: %s"), *CompensationBoneName.ToString(), *GetNameSafe(Animation));
			return;
		}
	}

	// Snapshot before touching the model, because writing keys changes what a later evaluate returns
	TArray<FTransform> RootTransforms;
	TArray<FTransform> ChildTransforms;
	RootTransforms.Reserve(NumKeys);
	ChildTransforms.Reserve(bPreservePose ? NumKeys : 0);
	for (int32 Key = 0; Key < NumKeys; Key++)
	{
		RootTransforms.Add(Model->EvaluateBoneTrackTransform(RootBoneName, Key, EAnimInterpolationType::Step));
		if (bPreservePose)
		{
			ChildTransforms.Add(Model->EvaluateBoneTrackTransform(CompensationBoneName, Key, EAnimInterpolationType::Step));
		}
	}

	const FVector FirstLocation = RootTransforms[0].GetLocation();
	const FVector LastLocation = RootTransforms.Last().GetLocation();
	const FRotator FirstRotation = RootTransforms[0].GetRotation().Rotator();
	FRotator LastRotation = RootTransforms.Last().GetRotation().Rotator();

	// Straightening interpolates in euler space, so the end has to be expressed as a delta from the
	// start rather than wrapped into [-180, 180], or a turn past half a circle lerps the short way back
	LastRotation.Roll = FirstRotation.Roll + FMath::UnwindDegrees(LastRotation.Roll - FirstRotation.Roll);
	LastRotation.Pitch = FirstRotation.Pitch + FMath::UnwindDegrees(LastRotation.Pitch - FirstRotation.Pitch);
	LastRotation.Yaw = FirstRotation.Yaw + FMath::UnwindDegrees(LastRotation.Yaw - FirstRotation.Yaw);

	constexpr bool bShouldTransact = false;
	Controller.OpenBracket(LOCTEXT("RootMotionAxisModifier_Bracket", "Filtering root motion axes"), bShouldTransact);

	FQuat PreviousRootRotation = RootTransforms[0].GetRotation();
	FQuat PreviousChildRotation = bPreservePose ? ChildTransforms[0].GetRotation() : FQuat::Identity;

	for (int32 Key = 0; Key < NumKeys; Key++)
	{
		const double Alpha = static_cast<double>(Key) / static_cast<double>(NumKeys - 1);

		const FTransform& OldRoot = RootTransforms[Key];
		const FVector OldLocation = OldRoot.GetLocation();
		const FRotator OldRotation = OldRoot.GetRotation().Rotator();

		const FVector NewLocation(
			RootMotionAxisModifier::FilterAxis(TranslationX, OldLocation.X, FirstLocation.X, LastLocation.X, Alpha),
			RootMotionAxisModifier::FilterAxis(TranslationY, OldLocation.Y, FirstLocation.Y, LastLocation.Y, Alpha),
			RootMotionAxisModifier::FilterAxis(TranslationZ, OldLocation.Z, FirstLocation.Z, LastLocation.Z, Alpha));

		const FRotator NewRotation(
			RootMotionAxisModifier::FilterAxis(Pitch, OldRotation.Pitch, FirstRotation.Pitch, LastRotation.Pitch, Alpha),
			RootMotionAxisModifier::FilterAxis(Yaw, OldRotation.Yaw, FirstRotation.Yaw, LastRotation.Yaw, Alpha),
			RootMotionAxisModifier::FilterAxis(Roll, OldRotation.Roll, FirstRotation.Roll, LastRotation.Roll, Alpha));

		FTransform NewRoot(NewRotation.Quaternion(), NewLocation, OldRoot.GetScale3D());

		// Neighbouring keys interpolate as quaternions, so one that flips hemisphere against its
		// predecessor spins the long way around
		FQuat NewRootRotation = NewRoot.GetRotation();
		if ((NewRootRotation | PreviousRootRotation) < 0.f)
		{
			NewRootRotation = -NewRootRotation;
			NewRoot.SetRotation(NewRootRotation);
		}
		PreviousRootRotation = NewRootRotation;

		const FInt32Range KeyRange(Key, Key + 1);
		Controller.UpdateBoneTrackKeys(RootBoneName, KeyRange,
			{ NewRoot.GetLocation() }, { NewRoot.GetRotation() }, { NewRoot.GetScale3D() }, bShouldTransact);

		if (bPreservePose)
		{
			// Child * OldRoot is where the bone actually was, so re-parenting that under the filtered
			// root leaves it there
			FTransform NewChild = ChildTransforms[Key] * OldRoot * NewRoot.Inverse();

			FQuat NewChildRotation = NewChild.GetRotation();
			if ((NewChildRotation | PreviousChildRotation) < 0.f)
			{
				NewChildRotation = -NewChildRotation;
				NewChild.SetRotation(NewChildRotation);
			}
			PreviousChildRotation = NewChildRotation;

			Controller.UpdateBoneTrackKeys(CompensationBoneName, KeyRange,
				{ NewChild.GetLocation() }, { NewChild.GetRotation() }, { NewChild.GetScale3D() }, bShouldTransact);
		}
	}

	Controller.CloseBracket(bShouldTransact);
}

#undef LOCTEXT_NAMESPACE
