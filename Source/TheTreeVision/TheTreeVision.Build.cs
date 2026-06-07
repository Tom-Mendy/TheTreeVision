using UnrealBuildTool;

public class TheTreeVision : ModuleRules
{
    public TheTreeVision(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "EnhancedInput",
            "InputCore"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "BlueprintGraph",
                "Kismet",
                "UnrealEd"
            });
        }
    }
}
