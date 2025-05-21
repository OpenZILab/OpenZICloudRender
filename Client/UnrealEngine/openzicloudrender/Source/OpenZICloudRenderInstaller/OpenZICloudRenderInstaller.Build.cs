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
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;

public class OpenZICloudRenderInstaller : ModuleRules
{
	private void Director(string TargetDir)
	{
		DirectoryInfo ThirdPartyInfo = new DirectoryInfo(TargetDir);
		FileSystemInfo[] ThirdPartyFileSystemInfos = ThirdPartyInfo.GetFileSystemInfos();
		foreach (FileSystemInfo ThirdPartyFileSystemInfo in ThirdPartyFileSystemInfos)
		{
			if (ThirdPartyFileSystemInfo is DirectoryInfo)     //判断是否为文件夹
			{
				if (ThirdPartyFileSystemInfo.Name.Contains("logs") == false)
					Director(ThirdPartyFileSystemInfo.FullName);//递归调用
			}
			else 
			{
				if (ThirdPartyFileSystemInfo.Name.Contains(".log") == false)
					RuntimeDependencies.Add(ThirdPartyFileSystemInfo.FullName);
			}
		}
	}

	private void CopyFolderToTargetedFolder(string folder, string targetFolder)
    {
        Action<string, string> action = null;

        action = (f1, f2) =>
        {
            string[] files = Directory.GetFiles(f1);
            string[] directories = Directory.GetDirectories(f1);

            if (Directory.Exists(f2))
            {
                for (int i = 0; i < files.Length; i++)
                {
                    try
                    {
                        System.IO.File.Copy(files[i], f2 + "\\" + Path.GetFileName(files[i]), true);
                    }
                    catch (Exception ex)
                    {
                        throw ex;
                    }
                }
            }
            else
            {
                Directory.CreateDirectory(f2);

                for (int i = 0; i < files.Length; i++)
                {
                    try
                    {
                        System.IO.File.Copy(files[i], f2 + "\\" + Path.GetFileName(files[i]), true);
                    }
                    catch (Exception ex)
                    {
                        throw ex;
                    }
                }
            }

            for (int i = 0; i < directories.Length; i++)
            {
                action(directories[i], f2 + "\\" + directories[i].Split('/', '\\').Last());
            }
        };

        if (Directory.Exists(folder))
        {
            action(folder, targetFolder);
        }
    }

	private void CopyFileToTargetedFolder(string sourceName, string folderPath)
	{
		//例子：
		//源文件路径
		//string sourceName = @"D:\Source\Test.txt";
		//目标路径:项目下的NewTest文件夹,(如果没有就创建该文件夹)
		//string folderPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "NewTest");
	
		if (!Directory.Exists(folderPath))
		{
			Directory.CreateDirectory(folderPath);
		}
	
		//当前文件如果不用新的文件名，那么就用原文件文件名
		string fileName = Path.GetFileName(sourceName);
		//这里可以给文件换个新名字，如下：
		//string fileName = string.Format("{0}.{1}", "newFileText", "txt");
	
		//目标整体路径
		string targetPath = Path.Combine(folderPath, fileName);
	
		//Copy到新文件下
		FileInfo file = new FileInfo(sourceName);
		if (file.Exists)
		{
			//true 为覆盖已存在的同名文件，false 为不覆盖
			file.CopyTo(targetPath, true);
			RuntimeDependencies.Add(targetPath);
		}
	}

	public OpenZICloudRenderInstaller(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.IsInPlatformGroup(UnrealPlatformGroup.Linux))
		{
			CopyFolderToTargetedFolder(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../OpenZICloudRender/Programs/CloudRenderServer")), Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../Programs/CloudRenderServer")));
			CopyFolderToTargetedFolder(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../OpenZICloudRender/Programs/SignallingWebServer")), Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../Programs/SignallingWebServer")));
			CopyFolderToTargetedFolder(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../OpenZICloudRender/Programs/SignallingWebServerCustom")), Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../Programs/SignallingWebServerCustom")));
			CopyFileToTargetedFolder(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../OpenZICloudRender/Programs/CloudRenderServer/OpenZICloudRenderServer.json")), Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../Config")));
			CopyFileToTargetedFolder(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../OpenZICloudRender/Config/OpenZICloudRender.json")), Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../Config")));

			// 打包时自动拷贝云渲染和信令服务器文件夹内容
			string TargetDir = Path.Combine(ModuleDirectory, "../../../../Programs");
			Director(TargetDir);
		}
	}
}
