// Copyright (c) Jared Taylor.

#pragma once

#include "CoreMinimal.h"
#include "BlendPoseModifier.h"
#include "Editor/AnimationModifiers/Public/AnimationModifier.h"
#include "BlendTransformModifier.generated.h"

/**
 * Bakes a user-supplied transform as an additive delta into every frame of a
 * target animation at edit time, with per-bone weight masking.
 *
 * Useful for distributing a rotation across a bone chain (e.g. adjusting the
 * angle an arm holds a weapon) by pairing with a blend profile or per-bone weights.
 */
UCLASS(DisplayName = "Blend Transform Modifier")
class SIMPLEANIMATIONMODIFIERS_API UBlendTransformModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	/** Transform delta to apply additively (relative to identity) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	FTransform Transform;

	/** Global blend weight, multiplied with per-bone weights */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings", meta=(ClampMin="0.0", ClampMax="1.0", Delta="0.05", ForceUnits="%"))
	float BlendAlpha = 1.0f;

	/** How per-bone weights are determined */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	EBoneWeightSource BoneWeightSource = EBoneWeightSource::BlendProfile;

	/** BlendProfile name on the skeleton for per-bone weight masking */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings", meta=(EditCondition="BoneWeightSource == EBoneWeightSource::BlendProfile", EditConditionHides))
	FName BlendProfileName;

	/** Per-bone weight multipliers (bones not listed are skipped) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings", meta=(EditCondition="BoneWeightSource == EBoneWeightSource::PerBone", EditConditionHides))
	TMap<FName, float> BoneWeights;

	/** Blend translation component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	bool bBlendTranslation = true;

	/** Blend rotation component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	bool bBlendRotation = true;

	/** Blend scale component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	bool bBlendScale = true;

public:
	virtual void OnApply_Implementation(UAnimSequence* Animation) override;
	virtual void OnRevert_Implementation(UAnimSequence* Animation) override {}
};
