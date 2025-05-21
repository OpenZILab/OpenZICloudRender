/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/
using UnrealBuildTool;
using System.IO;
using System.Collections.Generic;

public class OpenZICloudRender : ModuleRules
{
	public OpenZICloudRender(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;
		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
		// bUsePrecompiled = true;

		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDir, "Source/Runtime/AudioMixer/Private"),
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"ApplicationCore",
				"Json",
				"InputCore",
				"InputDevice",
				"RenderCore",
				"Slate",
				"SlateCore",
				"RHI",
				"AudioMixer",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);

		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");

		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Windows)) {
			PrivateDependencyModuleNames.Add("D3D11RHI");
			PrivateIncludePaths.AddRange(new string[]
			{
				Path.Combine(EngineDir, "Source/Runtime/Windows/D3D11RHI/Private"),
				Path.Combine(EngineDir, "Source/Runtime/Windows/D3D11RHI/Private/Windows"),
			});
			// required by D3D11RHI
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
			PublicSystemLibraries.AddRange(new string[] {
				"DXGI.lib",
				"d3d11.lib",
			});

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
		}
#if UE_5_0_OR_LATER
		PrivateDependencyModuleNames.Add("RHICore");
		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Windows))
		{
			PrivateDependencyModuleNames.Add("D3D12RHI");
			PrivateIncludePaths.AddRange(
				new string[]{
					Path.Combine(EngineDir, "Source/Runtime/D3D12RHI/Private"),
					Path.Combine(EngineDir, "Source/Runtime/D3D12RHI/Private/Windows")
				});

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
		}
#endif
#if UE_5_3_OR_LATER
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
#endif
		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Linux))
		{
			PrivateIncludePathModuleNames.Add("VulkanRHI");
			PrivateIncludePaths.Add(Path.Combine(EngineDir, "Source/Runtime/VulkanRHI/Private"));
			AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
			PrivateIncludePaths.Add(Path.Combine(EngineDir, "Source/Runtime/VulkanRHI/Private/Linux"));
		}
	}
}
