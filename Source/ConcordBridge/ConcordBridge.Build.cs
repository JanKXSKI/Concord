// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

using UnrealBuildTool;

public class ConcordBridge : ModuleRules
{
    public ConcordBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "ConcordCore", "Networking", "Sockets" });
        
        PrivateDependencyModuleNames.AddRange(new string[] { "JsonUtilities", "Json" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
