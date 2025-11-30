// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SimpleAnimationDeveloperSettings.generated.h"

class USkeletalMesh;

/**
 * 
 */
UCLASS(Config=Editor, meta=(DisplayName="Simple Animation Settings"))
class SIMPLEANIMATIONEDITOR_API USimpleAnimationDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const USimpleAnimationDeveloperSettings* Get();
	
	/** Skeletal mesh to assign when assigning the preview mesh in the editor */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category=Animation)
	TSoftObjectPtr<USkeletalMesh> DefaultSkeletalMesh;
};
