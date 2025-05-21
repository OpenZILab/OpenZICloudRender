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
#include "LaunchServerThreadBase.h"
#include "Containers/UnrealString.h"
#include "HAL/FileManager.h"
#include "LaunchServerUtil.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 4 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0)
#include "D3D11RHIBasePrivate.h"
#include "D3D11RHIPrivate.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5
#include "D3D12RHIPrivate.h"
#endif

#elif PLATFORM_LINUX
#include "VulkanRHIPrivate.h"
#endif

namespace OpenZI::CloudRender {

    FLaunchServerThreadBase::FLaunchServerThreadBase(TSharedPtr<FJsonObject> InConfigRoot,
                                                     FGuid Guid)
        : bWatchCloudRenderServer(true), bWatchSignallingWebServer(true), bWatchTurnWebServer(true),
          ConfigRoot(InConfigRoot) {
        ShmId = Guid;
        ShareMemorySuffix = Guid.ToString();
        ProgramsPath = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(FPaths::ProjectDir(), TEXT("Programs")));
#if PLATFORM_WINDOWS
        CloudRenderServerPath =
            FPaths::Combine(ProgramsPath, TEXT("CloudRenderServer/OpenZICloudRenderServer.exe"));
#elif PLATFORM_LINUX
        CloudRenderServerPath =
            FPaths::Combine(ProgramsPath, TEXT("CloudRenderServer/OpenZICloudRenderServer"));
#endif
    }

    FLaunchServerThreadBase::~FLaunchServerThreadBase() {
        if (Thread != nullptr) {
            Thread->Kill(true);
            delete Thread;
            Thread = nullptr;
        }
    }

    void FLaunchServerThreadBase::LaunchCloudRenderServer() {
        if (!FPaths::FileExists(CloudRenderServerPath)) {
            return;
        }
        FString ConfigPath = FPaths::Combine(ProgramsPath, TEXT("../Config"));
        bool bCloudRenderServerHidden = false;
        int32 CVarExplicitAdapterValue = -1;
        FParse::Value(FCommandLine::Get(), TEXT("graphicsadapter="), CVarExplicitAdapterValue);
        FString Params = TEXT("");
        FString RHIName = GDynamicRHI->GetName();
        if (RHIName == TEXT("D3D11")) {
#if PLATFORM_WINDOWS
#if ENGINE_MAJOR_VERSION == 4 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0)
            FD3D11Device* D3D11Device = GD3D11RHI->GetDevice();
            if (D3D11Device) {
                TRefCountPtr<IDXGIDevice> DXGIDevice(nullptr);
                D3D11Device->QueryInterface(__uuidof(IDXGIDevice),
                                            (void**)DXGIDevice.GetInitReference());
                if (DXGIDevice.IsValid()) {
                    TRefCountPtr<IDXGIAdapter> DXGIAdapter(nullptr);
                    DXGIDevice->GetAdapter(DXGIAdapter.GetInitReference());
                    DXGI_ADAPTER_DESC AdapterDesc;
                    DXGIAdapter->GetDesc(&AdapterDesc);
                    auto Luid = AdapterDesc.AdapterLuid;
                    auto DeviceId = AdapterDesc.DeviceId;
                    Params = FString::Printf(TEXT("%s %d %d %d %d"), *ShareMemorySuffix,
                                             CVarExplicitAdapterValue, (uint32)Luid.LowPart,
                                             (uint32)Luid.HighPart, (uint32)DeviceId);
                }
            }
#endif
#endif
        } else if (RHIName == TEXT("D3D12")) {
#if PLATFORM_WINDOWS
#if ENGINE_MAJOR_VERSION >= 5
            auto D3D12DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
            if (D3D12DynamicRHI) {
                auto AdapterDesc = D3D12DynamicRHI->GetAdapter().GetD3DAdapterDesc();
                auto Luid = AdapterDesc.AdapterLuid;
                auto DeviceId = AdapterDesc.DeviceId;
                Params = FString::Printf(TEXT("%s %d %d %d %d"), *ShareMemorySuffix,
                                         CVarExplicitAdapterValue, (uint32)Luid.LowPart,
                                         (uint32)Luid.HighPart, (uint32)DeviceId);
            }
#endif
#endif
        } else if (RHIName == TEXT("Vulkan")) {
#if PLATFORM_LINUX
            FVulkanDevice* Device = GVulkanRHI->GetDevice();
            if (Device) {
                auto Props = Device->GetDeviceProperties();
                uint32 DeviceID = Props.deviceID;
                FString UUIDStr = TEXT("");
                for (int i = 0; i < VK_UUID_SIZE; i++) {
                    UUIDStr += FString::FromInt(Props.pipelineCacheUUID[i]);
                    if (i != 15) {
                        UUIDStr += TEXT(",");
                    }
                }
                Params = FString::Printf(TEXT("%d %d %d %s"), ShmId.A, CVarExplicitAdapterValue,
                                         DeviceID, *UUIDStr);
            }
            FTCHARToUTF8 Utf8Path(*ConfigPath, ConfigPath.Len());
            chdir(Utf8Path.Get());
#endif
        } else {
            Params = FString::Printf(TEXT("%s %d"), *ShareMemorySuffix, CVarExplicitAdapterValue);
        }
        CloudRenderServer = FPlatformProcess::CreateProc(
            *CloudRenderServerPath, *Params, false, bCloudRenderServerHidden,
            bCloudRenderServerHidden, nullptr, 0, *ConfigPath, nullptr, nullptr);
        if (CloudRenderServer.IsValid()) {
            UE_LOG(LogTemp, Log, TEXT("Launched cloud render server"));
        }
    }

    FProcHandle ExecTurn(const TCHAR* URL, const TCHAR* Params) {
        // try use CreateProc function to launch turn server
        // this implemention that launch turn server is validable
        return FPlatformProcess::CreateProc(URL, Params, false, false, false, nullptr, 0, nullptr,
                                            nullptr, nullptr);
    }

    void FLaunchServerThreadBase::WriteConfigToWebRoot() {
        TSharedPtr<FJsonObject> WebConfigRoot;
        FString ConfigPath = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(ProgramsPath, SignallingDirName, TEXT("config.json")));
        FString JsonString;
        if (FFileHelper::LoadFileToString(JsonString, *ConfigPath)) {
            TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<>::Create(JsonString);
            if (FJsonSerializer::Deserialize(Reader, WebConfigRoot)) {
                const TSharedPtr<FJsonObject>* WebConfigObjectPtr = nullptr;
                if (!ConfigRoot->TryGetObjectField(TEXT("WebConfig"), WebConfigObjectPtr)) {
                    UE_LOG(LogTemp, Warning,
                           TEXT("Failed to get 'WebConfig' key in OpenZICloudRender.json"));
                    return;
                }
                TSharedPtr<FJsonObject> WebConfigObject = *WebConfigObjectPtr;
                TMap<FString, TSharedPtr<FJsonValue>> WebConfigs = WebConfigObject->Values;
                for (const auto& WebConfig : WebConfigs) {
                    WebConfigRoot->SetField(WebConfig.Key, WebConfig.Value);
                }
                FString WriteJsonString;
                TSharedRef<TJsonWriter<TCHAR>> Writer =
                    TJsonWriterFactory<>::Create(&WriteJsonString);
                if (!FJsonSerializer::Serialize(WebConfigRoot.ToSharedRef(), Writer)) {
                    return;
                }
                UE_LOG(LogTemp, Log, TEXT("Current signalling config.json content is: \n%s"),
                       *WriteJsonString);
                FFileHelper::SaveStringToFile(FStringView(WriteJsonString),
                                              ConfigPath.GetCharArray().GetData(),
                                              FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            } else {
                UE_LOG(
                    LogTemp, Warning,
                    TEXT("Failed to deserialize config.json, please check Programs/%s/config.json"),
                    *SignallingDirName);
            }
        } else {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load config.json"));
        }
    }
    void FLaunchServerThreadBase::LaunchSignallingWebServer() {
        WriteConfigToWebRoot();
        FString StunServer = "";
        FString TurnServer = "";
        FString PublicIP = "";
        ConfigRoot->TryGetStringField(TEXT("CloudStunServer"), StunServer);
        ConfigRoot->TryGetStringField(TEXT("CloudTurnServer"), TurnServer);
        ConfigRoot->TryGetStringField(TEXT("CloudPublibIP"), PublicIP);
        TArray<FString> SplitArr;
        if (StunServer.ParseIntoArray(SplitArr, TEXT(":"), true) > 0) {
            FString IP = SplitArr[0];
            FString Port = TEXT("18890");
            if (SplitArr.Num() > 1) {
                Port = SplitArr[1];
            }
            if (!IsValidIPAddress(IP)) {
                IP = TEXT("127.0.0.1");
                StunServer = IP + TEXT(":") + Port;
            }
        }
        SplitArr.Empty();
        if (TurnServer.ParseIntoArray(SplitArr, TEXT(":"), true) > 0) {
            FString IP = SplitArr[0];
            FString Port = TEXT("18891");
            if (SplitArr.Num() > 1) {
                Port = SplitArr[1];
            }
            if (!IsValidIPAddress(IP)) {
                IP = TEXT("127.0.0.1");
                TurnServer = IP + TEXT(":") + Port;
            }
        }
        if (!IsValidIPAddress(PublicIP)) {
            PublicIP = TEXT("127.0.0.1");
        }

        bool bSignallingWebServerHidden = false;
        FString SignallingScriptPath =
            FPaths::Combine(ProgramsPath, SignallingDirName, TEXT("cirrus"));
        FString Params = "cirrus";
#if PLATFORM_WINDOWS
        Params = FString::Printf(
            TEXT("%s --peerConnectionOptions=\"{ \\\"iceServers\\\": [{\\\"urls\\\": "
                 "[\\\"stun:%s\\\",\\\"turn:%s\\\"], \\\"username\\\": \\\"PixelStreamingUser\\\", "
                 "\\\"credential\\\": \\\"AnotherTURNintheroad\\\"}] }\" --PublicIP=%s"),
            *SignallingScriptPath, *StunServer, *TurnServer, *PublicIP);
#elif PLATFORM_LINUX
        Params = FString::Printf(
            TEXT("%s --peerConnectionOptions=\"{ \"iceServers\": [{\"urls\": "
                 "[\"stun:%s\",\"turn:%s\"], \"username\": \"PixelStreamingUser\", "
                 "\"credential\": \"AnotherTURNintheroad\"}] }\" --PublicIP=%s"),
            *SignallingScriptPath, *StunServer, *TurnServer, *PublicIP);
#endif
        SignallingWebServer = FPlatformProcess::CreateProc(
            *SignallingWebServerPath, *Params, false, bSignallingWebServerHidden,
            bSignallingWebServerHidden, nullptr, 0, nullptr, nullptr, nullptr);
        if (SignallingWebServer.IsValid()) {
            UE_LOG(LogTemp, Log, TEXT("Launched signalling web server"));
        }
    }

    void FLaunchServerThreadBase::LaunchTurnWebServer() {
        bool bPublicAccess = false;
        ConfigRoot->TryGetBoolField(TEXT("bCloudPublicAccess"), bPublicAccess);
        bool bLaunchTurnServer = false;
        ConfigRoot->TryGetBoolField(TEXT("bLaunchTurnServer"), bLaunchTurnServer);
        if (bPublicAccess && bLaunchTurnServer) {
            FString TurnServer = "";
            FString PublicIP = "";
            FString LocalIP = "";
            ConfigRoot->TryGetStringField(TEXT("CloudTurnServer"), TurnServer);
            ConfigRoot->TryGetStringField(TEXT("CloudPublibIP"), PublicIP);
            ConfigRoot->TryGetStringField(TEXT("CloudLocalIP"), LocalIP);
            TArray<FString> SplitArr;
            if (TurnServer.ParseIntoArray(SplitArr, TEXT(":"), true) > 0) {
                FString IP = SplitArr[0];
                FString Port = TEXT("18891");
                if (SplitArr.Num() > 1) {
                    Port = SplitArr[1];
                }
                if (!IsValidIPAddress(IP)) {
                    IP = TEXT("127.0.0.1");
                    TurnServer = IP + TEXT(":") + Port;
                }
            }
            if (!IsValidIPAddress(PublicIP)) {
                PublicIP = TEXT("127.0.0.1");
            }
            if (!IsValidIPAddress(LocalIP)) {
                LocalIP = TEXT("127.0.0.1");
            }
            FString TurnPort = "19311";
            TurnServer.Split(TEXT(":"), nullptr, &TurnPort);
            if (TurnPort.IsEmpty()) {
                TurnPort = "19311";
            }
            FString TurnParams =
                FString::Printf(TEXT("-p %s -r PixelStreaming -X %s -E %s -L %s --no-cli --no-tls "
                                     "--no-dtls --pidfile \"C:\\coturn.pid\" -f -a -v -n -u "
                                     "PixelStreamingUser:AnotherTURNintheroad"),
                                *TurnPort, *PublicIP, *LocalIP, *LocalIP);
            TurnServerHandle = FProcHandle(ExecTurn(*TurnWebServerPath, *TurnParams));
        } else {
            bWatchTurnWebServer = false;
        }
        if (TurnServerHandle.IsValid()) {
            UE_LOG(LogTemp, Log, TEXT("Launched turn web server"));
        }
    }

    bool FLaunchServerThreadBase::IsCloudRenderServerRunning() {
        if (CloudRenderServer.IsValid()) {
            return FPlatformProcess::IsProcRunning(CloudRenderServer);
        }
        return false;
    }

    bool FLaunchServerThreadBase::IsSignallingWebServerRunning() {
        if (SignallingWebServer.IsValid()) {
            return FPlatformProcess::IsProcRunning(SignallingWebServer);
        }
        return false;
    }

    bool FLaunchServerThreadBase::IsTurnWebServerRunning() {
        if (TurnServerHandle.IsValid()) {
            return FPlatformProcess::IsProcRunning(TurnServerHandle);
        }
        return false;
    }

    void FLaunchServerThreadBase::TermaniteCloudRenderServer() {
        if (CloudRenderServer.IsValid()) {
            FPlatformProcess::TerminateProc(CloudRenderServer, true);
            CloudRenderServer.Reset();
        }
    }

    void FLaunchServerThreadBase::TermaniteSignallingWebServer() {
        if (SignallingWebServer.IsValid()) {
            FPlatformProcess::TerminateProc(SignallingWebServer, true);
            SignallingWebServer.Reset();
        }
    }

    void FLaunchServerThreadBase::TermaniteTurnWebServer() {
        if (TurnServerHandle.IsValid()) {
            FPlatformProcess::TerminateProc(TurnServerHandle, true);
            TurnServerHandle.Reset();
        }
    }

    void FLaunchServerThreadBase::TryRebootCloudRenderServer() {
        TermaniteCloudRenderServer();
        LaunchCloudRenderServer();
    }

    void FLaunchServerThreadBase::TryRebootSignallingWebServer() {
        TermaniteSignallingWebServer();
        LaunchSignallingWebServer();
    }

    void FLaunchServerThreadBase::TryRebootTurnWebServer() {
        TermaniteTurnWebServer();
        LaunchTurnWebServer();
    }

    bool FLaunchServerThreadBase::Init() {
        bool bLaunchSignallingWebServer = false;
        ConfigRoot->TryGetBoolField(TEXT("bLaunchSignallingWebServer"), bLaunchSignallingWebServer);
        if (!bLaunchSignallingWebServer) {
            bWatchSignallingWebServer = false;
        }

        if (!bWatchCloudRenderServer && !bWatchSignallingWebServer && !bWatchTurnWebServer) {
            ExitRequest.Set(true);
            return true;
        }
        ExitRequest.Set(false);
        return true;
    }

    uint32 FLaunchServerThreadBase::Run() {
        bool bInitalized = false;
        int32 DelaySeconds = 3;
        int32 DelayCounter = 0;
        if (bWatchTurnWebServer) {
            LaunchTurnWebServer();
        }
        if (bWatchSignallingWebServer)
            LaunchSignallingWebServer();
        while (!ExitRequest.GetValue()) {
            if (bInitalized == false && DelayCounter++ == DelaySeconds) {
                if (bWatchCloudRenderServer)
                    LaunchCloudRenderServer();
                bInitalized = true;
                DelayCounter = 0;
            }
            if (bInitalized) {
                if (bWatchCloudRenderServer && !IsCloudRenderServerRunning()) {
                    UE_LOG(LogTemp, Log, TEXT("Checked cloud render server is not running"));
                    FPlatformProcess::Sleep(1.f);
                    TryRebootCloudRenderServer();
                }
                if (bWatchSignallingWebServer && !IsSignallingWebServerRunning()) {
                    TermaniteCloudRenderServer();
                    bInitalized = false;
                    UE_LOG(LogTemp, Log, TEXT("Checked signalling web server is not running"));
                    FPlatformProcess::Sleep(1.f);
                    TryRebootSignallingWebServer();
                }
                if (bWatchTurnWebServer && !IsTurnWebServerRunning()) {
                    TermaniteSignallingWebServer();
                    UE_LOG(LogTemp, Log, TEXT("Checked turn web server is not running"));
                    FPlatformProcess::Sleep(1.f);
                    TryRebootTurnWebServer();
                }
            }
            FPlatformProcess::Sleep(1.f);
        }
        TermaniteCloudRenderServer();
        TermaniteSignallingWebServer();
        TermaniteTurnWebServer();
        return 0;
    }

    void FLaunchServerThreadBase::Stop() { ExitRequest.Set(true); }

    void FLaunchServerThreadBase::Exit() {
        TermaniteCloudRenderServer();
        TermaniteSignallingWebServer();
        TermaniteTurnWebServer();
    }
} // namespace OpenZI::CloudRender