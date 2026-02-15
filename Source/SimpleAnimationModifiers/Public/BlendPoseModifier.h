// Copyright (c) Jared Taylor.

#pragma once

#include "CoreMinimal.h"
#include "Editor/AnimationModifiers/Public/AnimationModifier.h"
#include "BlendPoseModifier.generated.h"

UENUM(BlueprintType)
enum class EBlendPoseMode : uint8
{
	/** Lerp toward source pose */
	Override,
	/** Apply delta from reference pose in local bone space (matches ApplyAdditive node) */
	Additive
};

UENUM(BlueprintType)
enum class EBoneWeightSource : uint8
{
	/** All bones receive the same BlendAlpha weight */
	Uniform,
	/** Use a BlendProfile from the skeleton for per-bone weight masking */
	BlendProfile,
	/** Specify per-bone weights explicitly via a map */
	PerBone
};

/**
 * Bakes a source pose into every frame of a target animation at edit time,
 * using either override (lerp) or additive (delta) blending, with per-bone
 * weight masking via UBlendProfile.
 */
UCLASS(DisplayName = "Blend Pose Modifier")
class SIMPLEANIMATIONMODIFIERS_API UBlendPoseModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	/** Source animation to sample the blend pose from */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	TObjectPtr<UAnimSequence> SourceAnimation;

	/** Time in the source animation to sample (clamped to sequence length) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings", meta=(ClampMin="0.0", UIMin="0.0", Delta="0.05", ForceUnits="s"))
	double SourceTime = 0.0;

	/** Blending mode: Override lerps toward source, Additive applies deltas from ref pose */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	EBlendPoseMode BlendMode = EBlendPoseMode::Override;

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
