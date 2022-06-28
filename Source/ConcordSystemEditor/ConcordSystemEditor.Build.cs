// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ConcordSystemEditor : ModuleRules
{
    public ConcordSystemEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "PropertyEditor", "ConcordSystem", "Concord", "ConcordCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MetasoundGraphCore", "JsonUtilities", "Json", "Slate", "SlateCore", "AudioExtensions", "MainFrame", "PropertyEditor", "ApplicationCore", "DesktopPlatform" });

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/midifile/include"));
        bEnableUndefinedIdentifierWarnings = false;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // VS2015 updated some of the CRT definitions but not all of the Windows SDK has been updated to match.
            // Microsoft provides this shim library to enable building with VS2015 until they fix everything up.
            //@todo: remove when no longer neeeded (no other code changes should be necessary).
            if (Target.WindowsPlatform.bNeedsLegacyStdioDefinitionsLib)
            {
                PublicSystemLibraries.Add("legacy_stdio_definitions.lib");
            }
        }
        PublicDefinitions.Add("BUILDING_STATIC");
        AddEngineThirdPartyPrivateStaticDependencies(Target, "coremod");
    }
}
