// Copyright Jan Klimaschewski. All Rights Reserved.

using UnrealBuildTool;

public class ConcordLearning : ModuleRules
{
    public ConcordLearning(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        CppStandard = CppStandardVersion.Cpp17;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "ConcordCore",
            "Slate",
            "DesktopPlatform",
            "UnrealEd"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "JsonUtilities",
            "Json"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
