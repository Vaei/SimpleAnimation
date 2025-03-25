// Copyright (c) Jared Taylor. All Rights Reserved


#include "SimpleAnimAssetEditorLib.h"

#include "AnimationBlueprintLibrary.h"
#include "AnimationModifier.h"
#include "AnimationModifiersAssetUserData.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SimpleAnimAssetEditorLib)

#define LOCTEXT_NAMESPACE "SimpleAnimAssetEditorLib"

void USimpleAnimAssetEditorLib::EditorCastArrayChecked(TArray<UObject*> ArrayToCast, TSubclassOf<UObject> CastToClass,
	TArray<UObject*>& Result)
{
	Result.Reset();
	
	if (!CastToClass)
	{
		return;
	}

	for (UObject* ObjectToCast : ArrayToCast)
	{
		if (IsValid(ObjectToCast))
		{
			Result.Add(ObjectToCast);
		}
	}
}

void USimpleAnimAssetEditorLib::SetAnimRootLock(bool bLock, const TArray<UAnimSequence*>& Animations)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			Animation->bForceRootLock = bLock;
		}
	}
}

void USimpleAnimAssetEditorLib::SetAnimEnableRootMotion(bool bEnableRootMotion,
	const TArray<UAnimSequence*>& Animations)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			Animation->bEnableRootMotion = bEnableRootMotion;
		}
	}
}

TArray<UAnimSequence*> USimpleAnimAssetEditorLib::SetCompressionTypeForAnimations(const TArray<UAnimSequence*>& Animations,
	UAnimCurveCompressionSettings* CurveCompressionSettings)
{
	TArray<UAnimSequence*> ChangedAnimations;
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			if (Animation->CurveCompressionSettings != CurveCompressionSettings)
			{
				Animation->CurveCompressionSettings = CurveCompressionSettings;

				// ReSharper disable once CppExpressionWithoutSideEffects
				Animation->MarkPackageDirty();

				ChangedAnimations.AddUnique(Animation);
			}
		}
	}

	return ChangedAnimations;
}

void USimpleAnimAssetEditorLib::CompressAnimations(const TArray<UAnimSequence*>& Animations)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation) && Animation->GetOutermost() != GetTransientPackage())
		{
			if (const ITargetPlatform* RunningPlatform = GetTargetPlatformManagerRef().GetRunningTargetPlatform())
			{
				Animation->CacheDerivedData(RunningPlatform);
		
				// ReSharper disable once CppExpressionWithoutSideEffects
				Animation->MarkPackageDirty();
			}
		}
	}
}

void USimpleAnimAssetEditorLib::CloseAllAnimationEditors(UAnimSequence* Animation)
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->CloseAllEditorsForAsset(Animation);
}

void USimpleAnimAssetEditorLib::RemoveAllAnimCurves(const TArray<UAnimSequence*>& Animations)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			UAnimationBlueprintLibrary::RemoveAllCurveData(Animation);
		}
	}
}

void USimpleAnimAssetEditorLib::RemoveAllAnimNotifies(const TArray<UAnimSequence*>& Animations)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			UAnimationBlueprintLibrary::RemoveAllAnimationNotifyTracks(Animation);
		}
	}
}

void USimpleAnimAssetEditorLib::RemoveAllAnimModifiers(const TArray<UAnimSequence*>& Animations)
{
	int32 Removed = 0;
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			CloseAllAnimationEditors(Animation);

			Removed += RemoveAllAnimModifiers_Internal(Animation);
			if (Removed > 0)
			{
				// ReSharper disable once CppExpressionWithoutSideEffects
				Animation->MarkPackageDirty();
			}
		}
	}

	FMessageDialog Dialog;
	const FText DialogMsg = FText::FromString(FString::Printf(
		TEXT("Removed %d modifiers"), Removed));
	Dialog.Open(EAppMsgType::Ok, DialogMsg);
}

void USimpleAnimAssetEditorLib::AddAnimModifiers(const TArray<UAnimSequence*>& Animations,
	const TArray<TSubclassOf<UAnimationModifier>>& Modifiers)
{
	if (Animations.Num() == 0 || Modifiers.Num() == 0)
	{
		return;
	}

	TArray<UAnimationModifiersAssetUserData*> AssetUserData;
	for (UAnimSequence* Animation : Animations)
	{
		CloseAllAnimationEditors(Animation);
		
		UAnimationModifiersAssetUserData* UserData = Animation->GetAssetUserData<UAnimationModifiersAssetUserData>();
		if (!UserData)
		{
			UserData = NewObject<UAnimationModifiersAssetUserData>(Animation, UAnimationModifiersAssetUserData::StaticClass());
			checkf(UserData, TEXT("Unable to instantiate AssetUserData class"));
			UserData->SetFlags(RF_Transactional);
			Animation->AddAssetUserData(UserData);

			// ReSharper disable once CppExpressionWithoutSideEffects
			Animation->MarkPackageDirty();
		}
		
		AssetUserData.Add(UserData);
	}

	// For each added modifier create add a new instance to each of the user data entries, using the one(s) set up in the window as template(s)
	UE::Anim::FApplyModifiersScope Scope;
	for (const TSubclassOf<UAnimationModifier>& Modifier : Modifiers)
	{
		for (UAnimationModifiersAssetUserData* UserData : AssetUserData)
		{
			const bool bAlreadyContainsModifier = UserData->GetAnimationModifierInstances().ContainsByPredicate([Modifier](const UAnimationModifier* TestModifier) { return Modifier == TestModifier->GetClass(); });

			if (!bAlreadyContainsModifier)
			{
				UObject* Outer = UserData;
				UAnimationModifier* Processor = CreateModifierInstance(Outer, Modifier, Modifier.GetDefaultObject());
				const TArray<UAnimationModifier*>& Instances = UserData->GetAnimationModifierInstances();
				TArray<UAnimationModifier*>& MutableInstances = const_cast<TArray<UAnimationModifier*>&>(Instances);
				MutableInstances.Add(Processor);
			}
		}
	}
}

void USimpleAnimAssetEditorLib::PrintAllAssetsToMessageLog(const TArray<UObject*>& Assets, FName LogName,
	bool bOpenMessageLog)
{
	if (Assets.Num() == 0)
	{
		return;
	}

	FMessageLog MsgLog { LogName };
	
	for (const UObject* Asset : Assets)
	{
		const FText WarnMsg = FText::Format(
			LOCTEXT("PrintAssets_Print", "{0}"),
			FText::AsCultureInvariant(GetNameSafe(Asset)));

		MsgLog.Warning()
			->AddToken(FUObjectToken::Create(Asset))
			->AddToken(FTextToken::Create(WarnMsg));
	}

	if (bOpenMessageLog)
	{
		MsgLog.Open();
	}
}

int32 USimpleAnimAssetEditorLib::RemoveAllAnimModifiers_Internal(UAnimSequence* Animation)
{
	if (!IsValid(Animation))
	{
		return 0;
	}
	
	int32 Removed = 0;

	const TArray<UAssetUserData*>* DataPtr = Animation->GetAssetUserDataArray();
	if (DataPtr)
	{
		const TArray<UAssetUserData*> UserData = *DataPtr;
		for (UAssetUserData* Data : UserData)
		{
			if (const UAnimationModifiersAssetUserData* ModData = Cast<UAnimationModifiersAssetUserData>(Data))
			{
				Animation->RemoveUserDataOfClass(ModData->GetClass());
				Removed++;
			}
		}
	}

	return Removed;
}

UAnimationModifier* USimpleAnimAssetEditorLib::CreateModifierInstance(UObject* Outer, const UClass* InClass,
	UObject* Template)
{
	checkf(Outer, TEXT("Invalid outer value for modifier instantiation"));
	UAnimationModifier* ProcessorInstance = NewObject<UAnimationModifier>(Outer, InClass, NAME_None, RF_NoFlags, Template);
	checkf(ProcessorInstance, TEXT("Unable to instantiate modifier class"));
	ProcessorInstance->SetFlags(RF_Transactional);
	return ProcessorInstance;
}

#undef LOCTEXT_NAMESPACE
