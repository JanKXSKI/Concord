// Copyright Jan Klimaschewski. All Rights Reserved.

using UnrealBuildTool;

public class ConcordEditor : ModuleRules
{
    public ConcordEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {});

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UnrealEd",
            "Concord",
            "ConcordCore",
            "Slate",
            "SlateCore",
            "EditorStyle",
            "GraphEditor",
            "KismetWidgets",
            "Projects",
            "ToolMenus",
            "ApplicationCore",
            "DesktopPlatform",
            "ConcordSystemEditor"
        });

        if (Target.bWithLiveCoding)
        {
            PrivateIncludePathModuleNames.Add("LiveCoding");
        }

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
