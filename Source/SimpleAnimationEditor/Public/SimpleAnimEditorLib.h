// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "SimpleAnimEditorLib.generated.h"

/**
 * Editor-only animation tooling
 */
UCLASS()
class SIMPLEANIMATIONEDITOR_API USimpleAnimEditorLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** If keys have ArriveTangentWeight and LeaveTangentWeight, this apply set them */
	UFUNCTION(BlueprintPure, Category=SimpleAnimation)
	static void AutoSetTangents(TArray<FRichCurveKey>& CurveKeys, float Tension = 0.f);

	/** The same as pressing '1' with animation curve keys selected in the editor */
	UFUNCTION(BlueprintPure, Category=SimpleAnimation)
	static void SetAutoCubicKeyInterpolation(const TMap<float, float>& TimeValueMap, TArray<FRichCurveKey>& OutKeys);

	/** The same as pressing '4' with animation curve keys selected in the editor */
	UFUNCTION(BlueprintPure, Category=SimpleAnimation)
	static void SetAutoLinearKeyInterpolation(const TMap<float, float>& TimeValueMap, TArray<FRichCurveKey>& OutKeys);
	
	static void SetAutoKeyInterpolation(ERichCurveInterpMode InterpMode, const TMap<float, float>& TimeValueMap, TArray<FRichCurveKey>& OutKeys);
	
	/**
	 * @return True if the poses ( TArray<FTransform> ) at the start and end of the animation are within the Detection
	 * Threshold. Don't use small numbers because a proper looping animation isn't identical, but 1 frame apart
	 *
	 * Ignoring root motion is usually desired, because even on looping animations if it's a run cycle with root motion
	 * the difference would be extreme.
	 *
	 * Ignoring pelvis is sometimes useful because sometimes animations erroneously have root motion on the pelvis.
	 */
	static bool IsLoopingAnimation(const UAnimSequence* Animation, float DetectionThreshold = 5.f,
		bool bIgnoreRootMotion = true, bool bIgnorePelvis = false);

	static bool CompareBoneTransforms(const TArray<FTransform>& TransformsA, const TArray<FTransform>& TransformsB, float Tolerance = KINDA_SMALL_NUMBER);

	static void GetPoseForTime(const UAnimSequenceBase* Animation, TArray<FTransform>& Transforms, float Time);
	static void GetBonePoseForTime(const UAnimSequenceBase* Animation, FName BoneName, float Time, FTransform& Pose);
	static void GetBonePosesForTimeInternal(const UAnimSequenceBase* Animation, TArray<FName> BoneNames, float Time, TArray<FTransform>& Poses);
};
