// Copyright (c) Jared Taylor. All Rights Reserved


#include "SimpleAnimAssetEditorLib.h"

#include "AnimationBlueprintLibrary.h"
#include "AnimationModifier.h"
#include "AnimationModifiersAssetUserData.h"
#include "PackageTools.h"
#include "SimpleAnimationDeveloperSettings.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
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

void USimpleAnimAssetEditorLib::ApplyPreviewMesh(const TArray<UAnimSequence*>& Animations)
{
	const USimpleAnimationDeveloperSettings* Settings = USimpleAnimationDeveloperSettings::Get();
	USkeletalMesh* PreviewMesh = Settings->DefaultSkeletalMesh.LoadSynchronous();
	if (!IsValid(PreviewMesh))
	{
		FMessageLog("AssetCheck")
			.Error()
			->AddToken(FUObjectToken::Create(Settings, LOCTEXT("NoDefaultSkeletalMesh", "No default skeletal mesh set in Simple Animation Settings.")))
			->AddToken(FTextToken::Create(LOCTEXT("ApplyPreviewMeshError", "Please set a default skeletal mesh in the Simple Animation Settings.")));
		FMessageLog("AssetCheck").Open();
		return;
	}
	
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			if (Animation->GetPreviewMesh() != PreviewMesh)
			{
				Animation->SetPreviewMesh(PreviewMesh, true);

				// ReSharper disable once CppExpressionWithoutSideEffects
				Animation->MarkPackageDirty();
			}
		}
	}
}

void USimpleAnimAssetEditorLib::SetAnimRootLock(bool bLock, const TArray<UAnimSequence*>& Animations)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			if (Animation->bForceRootLock != bLock)
			{
				Animation->bForceRootLock = bLock;

				// ReSharper disable once CppExpressionWithoutSideEffects
				Animation->MarkPackageDirty();
			}
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
			if (Animation->bEnableRootMotion != bEnableRootMotion)
			{
				Animation->bEnableRootMotion = bEnableRootMotion;

				// ReSharper disable once CppExpressionWithoutSideEffects
				Animation->MarkPackageDirty();
			}
		}
	}
}

void USimpleAnimAssetEditorLib::AddAnimFloatCurve(const TArray<UAnimSequence*>& Animations, FName CurveName,
	float CurveValue, bool bMetaDataCurve)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, CurveName, ERawCurveTrackTypes::RCT_Float))
			{
				// If the curve already exists, remove it first
				UAnimationBlueprintLibrary::RemoveCurve(Animation, CurveName);
			}
			
			UAnimationBlueprintLibrary::AddCurve(Animation, CurveName, ERawCurveTrackTypes::RCT_Float, bMetaDataCurve);
			UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, CurveName, 0.f, CurveValue);

			// ReSharper disable once CppExpressionWithoutSideEffects
			Animation->MarkPackageDirty();
		}
	}
}

void USimpleAnimAssetEditorLib::RemoveAnimFloatCurve(const TArray<UAnimSequence*>& Animations, FName CurveName)
{
	for (UAnimSequence* Animation : Animations)
	{
		if (IsValid(Animation))
		{
			if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, CurveName, ERawCurveTrackTypes::RCT_Float))
			{
				// If the curve already exists, remove it first
				UAnimationBlueprintLibrary::RemoveCurve(Animation, CurveName);

				// ReSharper disable once CppExpressionWithoutSideEffects
				Animation->MarkPackageDirty();
			}
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
			
			// ReSharper disable once CppExpressionWithoutSideEffects
			Animation->MarkPackageDirty();
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

			// ReSharper disable once CppExpressionWithoutSideEffects
			Animation->MarkPackageDirty();
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

	struct FAssetDataPair
	{
		UAnimationModifiersAssetUserData* UserData;
		UAnimSequence* Animation;
	};

	TArray<FAssetDataPair> AssetUserData;
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
		
		AssetUserData.Add({ UserData, Animation });
	}

	// For each added modifier create add a new instance to each of the user data entries, using the one(s) set up in the window as template(s)
	UE::Anim::FApplyModifiersScope Scope;
	for (const TSubclassOf<UAnimationModifier>& Modifier : Modifiers)
	{
		for (const FAssetDataPair& UserDataPair : AssetUserData)
		{
			UAnimationModifiersAssetUserData* UserData = UserDataPair.UserData;
			const bool bAlreadyContainsModifier = UserData->GetAnimationModifierInstances().ContainsByPredicate(
				[Modifier](const UAnimationModifier* TestModifier)
				{
					return Modifier == TestModifier->GetClass();
				});

			UAnimationModifier* Processor = CreateModifierInstance(UserData, Modifier, Modifier.GetDefaultObject());

			if (!bAlreadyContainsModifier)
			{
				const TArray<UAnimationModifier*>& Instances = UserData->GetAnimationModifierInstances();
				TArray<UAnimationModifier*>& MutableInstances = const_cast<TArray<UAnimationModifier*>&>(Instances);
				MutableInstances.Add(Processor);
			}
			
			Processor->ApplyToAnimationSequence(UserDataPair.Animation);
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

TArray<FName> USimpleAnimAssetEditorLib::GetAssetDependencies_Name(const UObject* Asset)
{
	const FString PackageName = UPackageTools::FilenameToPackageName(Asset->GetPackage()->GetName());

	const IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
	constexpr FAssetRegistryDependencyOptions Options { true, true, false, false, false };
	TArray<FName> Dependencies;
	AssetRegistry->K2_GetDependencies(FName(PackageName), Options, Dependencies);
	return Dependencies;
}

TArray<FString> USimpleAnimAssetEditorLib::GetAssetDependencies(const UObject* Asset)
{
	const TArray<FName> DependencyNames = GetAssetDependencies_Name(Asset);

	TArray<FString> StringNames;
	Algo::Transform(DependencyNames, StringNames, [](FName Name) { return Name.ToString(); });
	
	return StringNames;
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
