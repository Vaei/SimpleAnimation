using UnrealBuildTool;

public class SimpleAnimationEditor : ModuleRules
{
    public SimpleAnimationEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "DeveloperSettings",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "AnimationBlueprintLibrary", 
                "AnimationModifiers",
                "AssetRegistry",
            }
        );
    }
}