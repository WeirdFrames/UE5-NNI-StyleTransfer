// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class NNIStyleTransferTest : ModuleRules
{
    public NNIStyleTransferTest(ReadOnlyTargetRules Target) : base(Target)
    {
        //added to supress warnings from opencv plugin
        bEnableUndefinedIdentifierWarnings = false;
        DefaultBuildSettings = BuildSettingsVersion.V2;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(new string[] { Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private") });

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject", 
            "Engine", 
            "OpenCV", 
            "OpenCVHelper", 
            "InputCore", 
            "HeadMountedDisplay"
        });

        PrivateDependencyModuleNames.AddRange(new string[]{
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "Renderer",
                    "RenderCore",
                    "RHI",
                    "RHICore",
                    "D3D12RHI",
                    "OpenCV",
                    "OpenCVHelper"
        });

    }
}
