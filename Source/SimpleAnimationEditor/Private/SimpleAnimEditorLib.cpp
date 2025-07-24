// Copyright (c) Jared Taylor. All Rights Reserved


#include "SimpleAnimEditorLib.h"

#include "AnimPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SimpleAnimEditorLib)

void USimpleAnimEditorLib::AutoSetTangents(TArray<FRichCurveKey>& OutKeys, float Tension)
{
	// Iterate over all points in this InterpCurve
	for (int32 KeyIndex = 0; KeyIndex<OutKeys.Num(); KeyIndex++)
	{
		FRichCurveKey& Key = OutKeys[KeyIndex];
		float ArriveTangent = Key.ArriveTangent;
		float LeaveTangent  = Key.LeaveTangent;

		// Variables for computing tangent.
		bool bNeedsComputeTangent = false;
		const FRichCurveKey* PrevKey = &Key;
		const FRichCurveKey* CurrentKey = &Key;
		const FRichCurveKey* NextKey = &Key;

		if (KeyIndex == 0) // Start point
		{
			// If first section is not a curve, or is a curve and first point has manual tangent setting.
			if (KeyIndex < (OutKeys.Num() - 1) && Key.TangentMode == RCTM_Auto)
			{
				LeaveTangent = 0.0f;
			}
		}
		else
		{
			if (KeyIndex < OutKeys.Num() - 1) // Inner point
			{
				PrevKey = &OutKeys[KeyIndex-1];

				if (Key.InterpMode == RCIM_Cubic && (Key.TangentMode == RCTM_Auto))
				{
					NextKey = &OutKeys[KeyIndex+1];
					bNeedsComputeTangent = true;
				}
				else if ((PrevKey->InterpMode == RCIM_Constant) || (Key.InterpMode == RCIM_Constant))
				{
					LeaveTangent  = 0.0f;

					if (PrevKey->InterpMode != RCIM_Cubic)
					{
						ArriveTangent = 0.0f;
					}
				}
				
			}
			else // End point
			{
				// If last section is not a curve, or is a curve and final point has manual tangent setting.
				if (Key.InterpMode == RCIM_Cubic && Key.TangentMode == RCTM_Auto)
				{
					ArriveTangent = 0.0f;
				}
			}
		}

		if (bNeedsComputeTangent)
		{
			ComputeCurveTangent(
				PrevKey->Time,		// Previous time
				PrevKey->Value,		// Previous point
				CurrentKey->Time,	// Current time
				CurrentKey->Value,	// Current point
				NextKey->Time,		// Next time
				NextKey->Value,		// Next point
				Tension,			// Tension
				false,				// Want clamping?
				ArriveTangent);		// Out

			// In 'auto' mode, arrive and leave tangents are always the same
			LeaveTangent = ArriveTangent;
		}

		Key.ArriveTangent = ArriveTangent;
		Key.LeaveTangent = LeaveTangent;
	}
}

void USimpleAnimEditorLib::SetAutoCubicKeyInterpolation(const TMap<float, float>& TimeValueMap,
	TArray<FRichCurveKey>& OutKeys)
{
	SetAutoKeyInterpolation(RCIM_Cubic, TimeValueMap, OutKeys);
}

void USimpleAnimEditorLib::SetAutoLinearKeyInterpolation(const TMap<float, float>& TimeValueMap,
	TArray<FRichCurveKey>& OutKeys)
{
	SetAutoKeyInterpolation(RCIM_Linear, TimeValueMap, OutKeys);
}

void USimpleAnimEditorLib::SetAutoKeyInterpolation(ERichCurveInterpMode InterpMode,
	const TMap<float, float>& TimeValueMap, TArray<FRichCurveKey>& OutKeys)
{
	OutKeys.Reset();
	
	if (TimeValueMap.Num() <= 1)
	{
		return;
	}
	
	for (auto& Itr : TimeValueMap)
	{
		FRichCurveKey& Key = OutKeys.AddDefaulted_GetRef();
		Key.Time = Itr.Key;
		Key.Value = Itr.Value;
		Key.InterpMode = InterpMode;
		Key.TangentMode = RCTM_Auto;
		Key.TangentWeightMode = RCTWM_WeightedNone;
		Key.ArriveTangent = 0.f;
		Key.LeaveTangent = 0.f;
	}

	const FRichCurveKey FirstKey = OutKeys[0];
	const FRichCurveKey LastKey  = OutKeys.Last();
	FRichCurveKey PrevKey = FirstKey;
	FRichCurveKey NextKey = OutKeys[1];

	int32 Key = 0;
	constexpr float OneThird = 1.0f / 3.0f;

	for (FRichCurveKey& ThisKey : OutKeys)
	{
		if (OutKeys.IsValidIndex(Key + 1))
		{
			NextKey = OutKeys[Key + 1];
		}
		
		// Calculate a tangent weight based upon tangent and time difference
		// Calculate arrive tangent weight
		if (ThisKey != FirstKey)
		{
			const float X = ThisKey.Time - PrevKey.Time;
			const float Y = ThisKey.ArriveTangent * X;
			ThisKey.ArriveTangentWeight = FMath::Sqrt(X*X + Y*Y) * OneThird;
		}
		
		// Calculate leave weight
		if(ThisKey != LastKey)
		{
			const float X = NextKey.Time - ThisKey.Time;
			const float Y = ThisKey.LeaveTangent *X;
			ThisKey.LeaveTangentWeight = FMath::Sqrt(X*X + Y*Y) * OneThird;
		}
		
		PrevKey = ThisKey;
		Key++;
	}

	AutoSetTangents(OutKeys);
}

bool USimpleAnimEditorLib::IsLoopingAnimation(const UAnimSequence* Animation, float DetectionThreshold,
	bool bIgnoreRootMotion, bool bIgnorePelvis)
{
	// Only do this if the animation is looping
	TArray<FTransform> FirstPose;
	TArray<FTransform> LastPose;
	GetPoseForTime(Animation, FirstPose, 0.f);
	GetPoseForTime(Animation, LastPose, Animation->GetPlayLength());
	if (bIgnoreRootMotion)
	{
		FirstPose.RemoveAt(0);  // Don't compare root
		LastPose.RemoveAt(0);
	}
	if (bIgnorePelvis)
	{
		const int32 PelvisIndex = bIgnoreRootMotion ? 0 : 1;
		FirstPose.RemoveAt(PelvisIndex);  // Don't compare pelvis, for when animator accidentally put root motion there instead...
		LastPose.RemoveAt(PelvisIndex);
	}
	return CompareBoneTransforms(FirstPose, LastPose, DetectionThreshold);
}

bool USimpleAnimEditorLib::CompareBoneTransforms(const TArray<FTransform>& TransformsA,
	const TArray<FTransform>& TransformsB, float Tolerance)
{
	if (TransformsA.Num() != TransformsB.Num())
	{
		return false;
	}
	for (int32 BoneIndex = 0; BoneIndex < TransformsA.Num(); ++BoneIndex)
	{
		if (!TransformsA[BoneIndex].Equals(TransformsB[BoneIndex], Tolerance))
		{
			return false;
		}
	}
	return true;
}

void USimpleAnimEditorLib::GetPoseForTime(const UAnimSequenceBase* Animation, TArray<FTransform>& Transforms,
	float Time)
{
	// Initialize the array of transforms
	const FReferenceSkeleton& RefSkeleton = Animation->GetSkeleton()->GetReferenceSkeleton();
	const int32 NumBones = RefSkeleton.GetRawBoneNum();
	Transforms.SetNum(NumBones);

	// Iterate through each bone track and get the transform at the specified time
	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		FTransform BoneTransform;
		GetBonePoseForTime(Animation, BoneName, Time, BoneTransform);
		Transforms[BoneIndex] = BoneTransform;
	}
}

void USimpleAnimEditorLib::GetBonePoseForTime(const UAnimSequenceBase* Animation, FName BoneName, float Time, FTransform& Pose)
{
	Pose.SetIdentity();
	TArray<FName> BoneNameArray;
	TArray<FTransform> PoseArray;
	BoneNameArray.Add(BoneName);
	GetBonePosesForTimeInternal(Animation, BoneNameArray, Time, PoseArray);
	Pose = PoseArray[0];
}

void USimpleAnimEditorLib::GetBonePosesForTimeInternal(const UAnimSequenceBase* Animation, TArray<FName> BoneNames,
	float Time, TArray<FTransform>& Poses)
{
	Poses.Empty(BoneNames.Num());
	if (Animation && Animation->GetSkeleton())
	{
		Poses.AddDefaulted(BoneNames.Num());

		if (BoneNames.Num())
		{
			TArray<FName> TrackNames;
			Animation->GetDataModel()->GetBoneTrackNames(TrackNames);
			
			for (int32 BoneNameIndex = 0; BoneNameIndex < BoneNames.Num(); ++BoneNameIndex)
			{
				const FName& BoneName = BoneNames[BoneNameIndex];
				if (TrackNames.Contains(BoneName))
				{
					const FAnimPoseEvaluationOptions EvaluationOptions = FAnimPoseEvaluationOptions();
					FAnimPose AnimPose;
    
					UAnimPoseExtensions::GetAnimPoseAtTime(Animation, Time, EvaluationOptions, AnimPose);
					Poses[BoneNameIndex] = UAnimPoseExtensions::GetBonePose(AnimPose, BoneName, EAnimPoseSpaces::Local);	
				}
				else
				{
					// otherwise, get ref pose if exists
					const FReferenceSkeleton& RefSkeleton = Animation->GetSkeleton()->GetReferenceSkeleton();
					const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
					if (BoneIndex != INDEX_NONE)
					{
						Poses[BoneNameIndex] = RefSkeleton.GetRefBonePose()[BoneIndex];
					}
					else
					{
						Poses[BoneNameIndex] = FTransform::Identity;
					}
				}			
			}
		}
	}
}
