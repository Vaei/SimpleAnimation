// Copyright (c) Jared Taylor. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimpleAnimAssetEditorLib.generated.h"

class UAnimationModifier;
/**
 * Functions for editor action utilities for animation assets
 */
UCLASS()
class SIMPLEANIMATIONEDITOR_API USimpleAnimAssetEditorLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** For use with AssetActionUtility. Editor only. Engine will crash if you give it the wrong type */
	UFUNCTION(BlueprintCallable, Category="Editor|Animation", CallInEditor, meta=(DeterminesOutputType="CastToClass", DynamicOutputParam="Result"))
	static void EditorCastArrayChecked(TArray<UObject*> ArrayToCast, TSubclassOf<UObject> CastToClass, TArray<UObject*>& Result);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void SetAnimRootLock(bool bLock, const TArray<UAnimSequence*>& Animations);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void SetAnimEnableRootMotion(bool bEnableRootMotion, const TArray<UAnimSequence*>& Animations);
		
	/** @return Any animations whose compression type changed */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static TArray<UAnimSequence*> SetCompressionTypeForAnimations(const TArray<UAnimSequence*>& Animations, UAnimCurveCompressionSettings* CurveCompressionSettings);
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void CompressAnimations(const TArray<UAnimSequence*>& Animations);
	
	/** Modifying an animation while its editor is open isn't always safe */
	UFUNCTION(BlueprintCallable, Category="Editor|Animation", CallInEditor)
	static void CloseAllAnimationEditors(UAnimSequence* Animation);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void RemoveAllAnimCurves(const TArray<UAnimSequence*>& Animations);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void RemoveAllAnimNotifies(const TArray<UAnimSequence*>& Animations);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void RemoveAllAnimModifiers(const TArray<UAnimSequence*>& Animations);

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void AddAnimModifiers(const TArray<UAnimSequence*>& Animations, const TArray<TSubclassOf<UAnimationModifier>>& Modifiers);
	
	/** Print all assets to the message log */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Editor|Animation")
	static void PrintAllAssetsToMessageLog(const TArray<UObject*>& Assets, FName LogName="AssetCheck", bool bOpenMessageLog=true);

protected:
	/** @return Num anim modifiers removed */
	static int32 RemoveAllAnimModifiers_Internal(UAnimSequence* Animation);

	/** Creates a new Modifier instance to store with the current asset */
	static UAnimationModifier* CreateModifierInstance(UObject* Outer, const UClass* InClass, UObject* Template = nullptr);
};
