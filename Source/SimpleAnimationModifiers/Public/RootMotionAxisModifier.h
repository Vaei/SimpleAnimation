// Copyright (c) Jared Taylor.

#pragma once

#include "CoreMinimal.h"
#include "Animation/BoneReference.h"
#include "Editor/AnimationModifiers/Public/AnimationModifier.h"
#include "RootMotionAxisModifier.generated.h"

UENUM(BlueprintType)
enum class ERootMotionAxisFilter : uint8
{
	/** Leave this axis as authored */
	Keep			UMETA(DisplayName="Keep"),
	/** Hold this axis at its first frame value, so the root never moves along it */
	Remove			UMETA(DisplayName="Remove"),
	/** Replace this axis with a straight line from its first to its last frame value, keeping the net displacement but discarding the curve between them */
	Straighten		UMETA(DisplayName="Straighten"),
};

/**
 * Strips or straightens root motion on individual axes of the root bone track.
 *
 * Intended for animation exported from tools that derive the root from the pelvis (Cascadeur, mocap
 * cleanup), where a straight walk arrives with a sideways arc baked into the root.
 *
 * The root track is authored in the animation's own space, so the axes here are the world space axes
 * of the animation, not of the character's facing.
 *
 * With Preserve Pose set, the motion taken off the root is folded back into a child bone (the pelvis
 * by default), so the body still sways exactly as authored while the capsule travels the filtered
 * path. Clear it to flatten the visible motion along with the root.
 */
UCLASS(DisplayName = "Root Motion Axis Modifier")
class SIMPLEANIMATIONMODIFIERS_API URootMotionAxisModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	/** Sideways translation for a character authored facing +X */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Translation")
	ERootMotionAxisFilter TranslationX = ERootMotionAxisFilter::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Translation")
	ERootMotionAxisFilter TranslationY = ERootMotionAxisFilter::Keep;

	/** Vertical translation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Translation")
	ERootMotionAxisFilter TranslationZ = ERootMotionAxisFilter::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rotation")
	ERootMotionAxisFilter Roll = ERootMotionAxisFilter::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rotation")
	ERootMotionAxisFilter Pitch = ERootMotionAxisFilter::Keep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rotation")
	ERootMotionAxisFilter Yaw = ERootMotionAxisFilter::Keep;

	/** Fold the removed motion into a child bone so the visible pose is unchanged and only the root motion differs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	bool bPreservePose = true;

	/** Bone that receives the removed motion. None uses the first child of the root, i.e. the pelvis */
	UPROPERTY(EditAnywhere, Category="Settings", meta=(EditCondition="bPreservePose", EditConditionHides))
	FBoneReference CompensationBone;

public:
	virtual void OnApply_Implementation(UAnimSequence* Animation) override;
	virtual void OnRevert_Implementation(UAnimSequence* Animation) override {}
};
