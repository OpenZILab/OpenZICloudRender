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
#include "OpenZICloudRenderModule.h"
#include "InputReceiveThread.h"
#include "FdSocket.h"
#include "LaunchCustomServerThread.h"
#include "LaunchNativeServerThread.h"
#include "Misc/Guid.h"
#include "SendThread.h"
#include "SubmixCapturer.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
// #include "Misc/MessageDialog.h"

using namespace OpenZI::CloudRender;

#define LOCTEXT_NAMESPACE "FOpenZICloudRenderModule"

IOpenZICloudRenderModule* FOpenZICloudRenderModule::OpenZICloudRenderModule = nullptr;

void FOpenZICloudRenderModule::StartupModule() {
    // Pixel Streaming does not make sense without an RHI so we don't run in commandlets without one.
    if (IsRunningCommandlet() && !IsAllowCommandletRendering()) {
        return;
    }

    // only D3D11/D3D12 is supported
    if (GDynamicRHI == nullptr || !(GDynamicRHI->GetName() == FString(TEXT("D3D11")) || GDynamicRHI->GetName() == FString(TEXT("D3D12")) || GDynamicRHI->GetName() == FString(TEXT("Vulkan")))) {
        UE_LOG(LogTemp, Warning, TEXT("Only D3D11/D3D12/Vulkan Dynamic RHI is supported. Detected %s"), GDynamicRHI != nullptr ? GDynamicRHI->GetName() : TEXT("[null]"));
        return;
    } else if (GDynamicRHI->GetName() == FString(TEXT("D3D11")) || GDynamicRHI->GetName() == FString(TEXT("D3D12")) || GDynamicRHI->GetName() == FString(TEXT("Vulkan"))) {
        // By calling InitStreamer post engine init we can use pixel streaming in standalone editor mode
        FCoreDelegates::OnFEngineLoopInitComplete.AddRaw(this, &FOpenZICloudRenderModule::InitCloudRender);
    }
    if (!GIsEditor) {
        // InitLaunchFile(TEXT("内嵌.bat"));
        // InitLaunchFile(TEXT("外嵌Demo网页使用官版像素流插件.bat"));
        // InitLaunchFile(TEXT("外嵌Demo网页使用自研云渲染产品.bat"));
        // InitLaunchFile(TEXT("ModifyJson.ps1"));
    }
}

void FOpenZICloudRenderModule::ShutdownModule() {
    if (!GIsEditor)
        IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
}

void FOpenZICloudRenderModule::InitCloudRender() {
    if (!ensure(GEngine != nullptr)) {
        return;
    }
    if (GIsEditor) {
        FText TitleText = FText::FromString(TEXT("OpenZICloudRender Plugin"));
        FString ErrorString = TEXT("OpenZICloudRender Plugin is not supported in editor");
        // FText ErrorText = FText::FromString(ErrorString);
        // FMessageDialog::Open(EAppMsgType::Ok, ErrorText, &TitleText);
        UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorString);
        return;
    }
    IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
}

void FOpenZICloudRenderModule::InitLaunchFile(FString LaunchFileName) {
    FString BatStartPath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Programs"), *LaunchFileName));
    FString BatEndPath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), *LaunchFileName));
    if (!IFileManager::Get().FileExists(*BatEndPath) &&
        IFileManager::Get().FileExists(*BatStartPath)) {
        IFileManager::Get().Copy(*BatEndPath, *BatStartPath);
    }
}

IOpenZICloudRenderModule* FOpenZICloudRenderModule::GetModule() {
    if (OpenZICloudRenderModule) {
        return OpenZICloudRenderModule;
    }
    IOpenZICloudRenderModule* Module =
        FModuleManager::Get().LoadModulePtr<IOpenZICloudRenderModule>("OpenZICloudRender");
    if (Module) {
        OpenZICloudRenderModule = Module;
    }
    return OpenZICloudRenderModule;
}

TSharedPtr<class IInputDevice> FOpenZICloudRenderModule::CreateInputDevice(
    const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) {
    InputDevice = MakeShareable(new FInputDevice(InMessageHandler));
    TSharedPtr<FJsonObject> ConfigRoot;
    FString ConfigPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("OpenZICloudRender.json")));
    FString JsonString;
    if (FFileHelper::LoadFileToString(JsonString, *ConfigPath /* , EHashOptions::None, 0 */)) {
        TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<>::Create(JsonString);
        if (FJsonSerializer::Deserialize(Reader, ConfigRoot)) {
            bool bEnableCloudRender;
            ConfigRoot->TryGetBoolField(TEXT("bEnableCloudRender"), bEnableCloudRender);
            if (!bEnableCloudRender) {
                UE_LOG(LogTemp, Log,
                       TEXT("Cloud render will be disabled bacause bEnableCloudRender = false"));
                return InputDevice;
            }
            bool bUseOpenZICloudRenderProduct = true;
            ConfigRoot->TryGetBoolField(TEXT("bUseOpenZICloudRenderProduct"), bUseOpenZICloudRenderProduct);
            if (!bUseOpenZICloudRenderProduct) {
                return InputDevice;
            }
            FString CloudMode;
            if (ConfigRoot->TryGetStringField(TEXT("CloudMode"), CloudMode)) {
                FGuid NewGuid = FGuid::NewGuid();
                UE_LOG(LogTemp, Log, TEXT("-----------------ShmKey=%d--------------"), NewGuid.A);
                if (CloudMode.Equals("UE")) {
                    LaunchServerThread = MakeShareable(new FLaunchNativeServerThread(
                        ConfigRoot, NewGuid));
                } else if (CloudMode.Equals("OpenZI")) {
#if PLATFORM_WINDOWS
                    InputReceiveThread = MakeShareable(new FInputReceiveThread(NewGuid));
#elif PLATFORM_LINUX
                    // std::string SocketName = "/tmp/openzicloudinput";
                    // InputReceiver = MakeShareable(new ZInputSocketReceiver(SocketName.data()));
                    InputReceiveThread = MakeShareable(new FInputReceiveThread(NewGuid));
#endif
                    uint32 FrameBufferSizeX = 2560;
                    uint32 FrameBufferSizeY = 1440;
                    if (!ConfigRoot->TryGetNumberField(TEXT("FrameBufferSizeX"),
                                                       FrameBufferSizeX)) {

                    }
                    if (!ConfigRoot->TryGetNumberField(TEXT("FrameBufferSizeY"),
                                                       FrameBufferSizeY)) {

                    }
                    
                    uint32 MaxCache = 60;
                    ConfigRoot->TryGetNumberField(TEXT("MaxCache"), MaxCache);
                    SendThread = MakeShareable(
                        new FSendThread(NewGuid, FrameBufferSizeX, FrameBufferSizeY, MaxCache));
                    LaunchServerThread = MakeShareable(new FLaunchCustomServerThread(
                        ConfigRoot, NewGuid));
                    SubmixCapturer = MakeShareable(new FSubmixCapturer(NewGuid));
                } else if (CloudMode.Equals("UEAndUsePixelStreaming")) {
                    LaunchServerThread = MakeShareable(new FLaunchNativeServerThread(
                        ConfigRoot, NewGuid));
                } else if (CloudMode.Equals("OpenZIAndUsePixelStreaming")) {
                    LaunchServerThread = MakeShareable(new FLaunchCustomServerThread(
                        ConfigRoot, NewGuid));
                }
            } else {
                UE_LOG(LogTemp, Log, TEXT("The cloud render mode is neither UE nor OpenZI and uses native pixel streaming"));
            }
        } else {
            UE_LOG(LogTemp, Warning, TEXT("Failed to deserialize OpenZICloudRender.json, please check Config/OpenZICloudRender.json"));
        }
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load OpenZICloudRender.json"));
    }

    // const auto& CommandLine = FCommandLine::Get();
    // FString CloudMode = "";
    // FParse::Value(CommandLine, TEXT("-CloudMode="), CloudMode);
    
    // if (CloudMode.Equals("UE")) {
    //     LaunchServerThread = MakeShareable(new FLaunchNativeServerThread(CommandLine));
    // } else if (CloudMode.Equals("OpenZI")) {
    //     LaunchServerThread = MakeShareable(new FLaunchCustomServerThread(CommandLine));
    // }
    // Thread has been created, so module is now "ready" for external use.
    ReadyEvent.Broadcast(*this);
    return InputDevice;
}

IOpenZICloudRenderModule::FReadyEvent& FOpenZICloudRenderModule::OnReady() {
    return ReadyEvent;
}

bool FOpenZICloudRenderModule::IsReady() {
    // return Streamer.IsValid();
    return true;
}

IInputDevice& FOpenZICloudRenderModule::GetInputDevice() {
    return *InputDevice;
}

TSharedPtr<FInputDevice> FOpenZICloudRenderModule::GetInputDevicePtr() {
    return InputDevice;
}

bool FOpenZICloudRenderModule::IsTickableWhenPaused() const {
    return true;
}

bool FOpenZICloudRenderModule::IsTickableInEditor() const {
    return true;
}

void FOpenZICloudRenderModule::Tick(float DeltaTime) {
}

TStatId FOpenZICloudRenderModule::GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(FOpenZICloudRenderModule, STATGROUP_Tickables);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOpenZICloudRenderModule, OpenZICloudRender)