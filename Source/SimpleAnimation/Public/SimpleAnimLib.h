// Copyright (c) Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleAnimLib.generated.h"

class UCapsuleComponent;
/**
 * Runtime animation tooling
 */
UCLASS()
class SIMPLEANIMATION_API USimpleAnimLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Magenta FLinearColor(1.f, 0.f, 1.f)
	// Cyan FLinearColor(0.f, 1.f, 1.f)
	// Orange FLinearColor(1.f, 0.5f, 0.f)
	
	/**
	 * Draw debug shapes for the physics bodies of a pawn's skeletal mesh component
	 * @param Pawn The pawn to draw physics bodies for
	 * @param Mesh The skeletal mesh component to draw physics bodies for
	 * @param bDrawAuthority Whether to draw physics bodies for the authority role
	 * @param bDrawLocal Whether to draw physics bodies for the local role
	 * @param bDrawSimulated Whether to draw physics bodies for the simulated role
	 * @param AuthColor The color of the debug lines for the authority role
	 * @param LocalColor The color of the debug lines for the local role
	 * @param SimulatedColor The color of the debug lines for the simulated role
	 * @param bPersistentLines Whether the debug lines should be persistent
	 * @param Duration The duration of the debug lines
	 * @param Thickness The thickness of the debug lines
	 * @note This function only works in non-shipping builds
	 */
	UFUNCTION(BlueprintCallable, Category=SimpleAnimation, meta=(DefaultToSelf="Pawn", DevelopmentOnly))
	static void DrawPawnDebugPhysicsBodies(
		APawn* Pawn, USkeletalMeshComponent* Mesh,
		const bool bDrawAuthority = false, const bool bDrawLocal = true, const bool bDrawSimulated = false,
		FLinearColor AuthColor = FLinearColor(1.f, 0.5f, 0.f),		// Orange
		FLinearColor LocalColor = FLinearColor(0.f, 1.f, 1.f),		// Cyan
		FLinearColor SimulatedColor = FLinearColor(1.f, 0.f, 1.f),   // Magenta
		const bool bPersistentLines = false, const float Duration = -1.f, const float Thickness = 0.f);

	/**
	 * Draw debug shapes for the physics bodies of a skeletal mesh component
	 * @param Mesh The skeletal mesh component to draw physics bodies for
	 * @param Color The color of the debug lines
	 * @param bPersistentLines Whether the debug lines should be persistent
	 * @param Duration The duration of the debug lines
	 * @param Thickness The thickness of the debug lines
	 */
	UFUNCTION(BlueprintCallable, Category=SimpleAnimation, meta=(DevelopmentOnly))
	static void DrawDebugPhysicsBodies(USkeletalMeshComponent* Mesh, FLinearColor Color = FLinearColor(1.f, 0.5f, 0.f),  // Orange
		const bool bPersistentLines = false, const float Duration = -1.f, const float Thickness = 0.f);

	/**
	 * Draw debug shapes for the physics bodies of a pawn's skeletal mesh component
	 * @param Pawn The pawn to draw physics bodies for
	 * @param Mesh The skeletal mesh component to draw physics bodies for
	 * @param bDrawAuthority Whether to draw physics bodies for the authority role
	 * @param bDrawLocal Whether to draw physics bodies for the local role
	 * @param bDrawSimulated Whether to draw physics bodies for the simulated role
	 * @param AuthColor The color of the debug lines for the authority role
	 * @param LocalColor The color of the debug lines for the local role
	 * @param SimulatedColor The color of the debug lines for the simulated role
	 * @param bPersistentLines Whether the debug lines should be persistent
	 * @param Duration The duration of the debug lines
	 * @param Thickness The thickness of the debug lines
	 * @note This function only works in non-shipping builds
	 */
	UFUNCTION(BlueprintCallable, Category=SimpleAnimation, meta=(DefaultToSelf="Pawn", DevelopmentOnly))
	static void DrawPawnDebugPhysicsCapsule(
		APawn* Pawn, const UCapsuleComponent* Mesh,
		const bool bDrawAuthority = false, const bool bDrawLocal = true, const bool bDrawSimulated = false,
		FLinearColor AuthColor = FLinearColor(1.f, 0.5f, 0.f),		// Orange
		FLinearColor LocalColor = FLinearColor(0.f, 1.f, 1.f),		// Cyan
		FLinearColor SimulatedColor = FLinearColor(1.f, 0.f, 1.f),	// Magenta
		const bool bPersistentLines = false, const float Duration = -1.f, const float Thickness = 0.f);
	
	/**
	 * Draw debug shapes for the physics bodies of a skeletal mesh component
	 * @param Capsule The capsule component to draw
	 * @param Color The color of the debug lines
	 * @param bPersistentLines Whether the debug lines should be persistent
	 * @param Duration The duration of the debug lines
	 * @param Thickness The thickness of the debug lines
	 */
	UFUNCTION(BlueprintCallable, Category=SimpleAnimation, meta=(DevelopmentOnly))
	static void DrawDebugPhysicsCapsule(const UCapsuleComponent* Capsule, FLinearColor Color = FLinearColor(1.f, 0.5f, 0.f),  // Orange
		const bool bPersistentLines = false, const float Duration = -1.f, const float Thickness = 0.f);
};
