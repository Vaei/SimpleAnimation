// Copyright (c) Jared Taylor.

#pragma once

#include "CoreMinimal.h"
#include "AnimPose.h"
#include "Editor/AnimationModifiers/Public/AnimationModifier.h"
#include "CopyIKBonesModifier.generated.h"

USTRUCT(BlueprintType)
struct FCopyBonePairs
{
	GENERATED_BODY()

	FCopyBonePairs(const FName& InSourceBone = NAME_None, const FName& InTargetBone = NAME_None)
		: SourceBone(InSourceBone), TargetBone(InTargetBone)
	{}

	/** Bone to get transform from */
	UPROPERTY(EditAnywhere, Category="Settings")
	FBoneReference SourceBone;

	/** Bone to apply transform to */
	UPROPERTY(EditAnywhere, Category="Settings")
	FBoneReference TargetBone;
};

/**
 * 
 */
UCLASS(DisplayName = "Copy IK Bones Modifier")
class SIMPLEANIMATIONMODIFIERS_API UCopyIKBonesModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	TArray<FCopyBonePairs> BonesToCopy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings")
	EAnimPoseSpaces BonePoseSpace = EAnimPoseSpaces::World;
	
public:
	UCopyIKBonesModifier()
		: BonesToCopy( {
			{ "hand_r", "ik_hand_gun" },
			{ "hand_r", "ik_hand_r" },
			{ "hand_l", "ik_hand_l" }, 
			{ "foot_r", "ik_foot_r" },
			{ "foot_l", "ik_foot_l" }  } )
	{}
	
	virtual void OnApply_Implementation(UAnimSequence* Animation) override;
	virtual void OnRevert_Implementation(UAnimSequence* Animation) override {}
};
