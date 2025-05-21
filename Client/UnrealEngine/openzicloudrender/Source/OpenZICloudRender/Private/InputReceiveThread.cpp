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
#include "InputReceiveThread.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"
#include "IPCTypes.h"
#include "OpenZICloudRenderModule.h"

// PRAGMA_DISABLE_OPTIMIZATION

namespace OpenZI::CloudRender {
    FInputReceiveThread::FInputReceiveThread()
        : Thread(nullptr), ThreadName(TEXT("CloudRenderInputReceiveThread")) {
#if PLATFORM_WINDOWS
        char* Name = "OpenZICloudRenderInput";
        Reader = MakeShareable(new FShareMemoryWriter(Name, 1024 * 1024 * 256));
#elif PLATFORM_LINUX

#endif
        if (!Reader->Check()) {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create shared memory that be used receive input commands from web."));
        }
        StartThread();
    }

    FInputReceiveThread::FInputReceiveThread(FGuid Guid) {
        ShmId = Guid;
        FString IdStr = ShmId.ToString();
        Thread = nullptr;
        ThreadName = FString::Printf(TEXT("%s_%s"), TEXT("CloudRenderInputReceiveThread"), *IdStr);
#if PLATFORM_WINDOWS
        ShareMemoryName = FString::Printf(TEXT("%s_%s"), TEXT("OpenZICloudRenderInput"), *IdStr);
        char* Name = TCHAR_TO_UTF8(*ShareMemoryName);
        Reader = MakeShareable(new FShareMemoryWriter(Name, 1024 * 1024 * 256));
#elif PLATFORM_LINUX

        key_t ShmKey = ShmId.A - 1;
        Reader = MakeShareable(new FShareMemoryWriter(ShmKey, 4096));
#endif
        if (!Reader->Check()) {
            UE_LOG(LogTemp, Warning,
                   TEXT("Failed to create shared memory that be used receive input commands from web."));
        }
        StartThread();
    }

    FInputReceiveThread::~FInputReceiveThread() {
        StopThread();
    }

    void FInputReceiveThread::StartThread() {
        Thread = FRunnableThread::Create(this, *ThreadName, 0, EThreadPriority::TPri_Highest);
    }

    void FInputReceiveThread::StopThread() {
        if (Thread != nullptr) {
            Thread->Kill(true);
            delete Thread;
            Thread = nullptr;
        }
    }

    bool FInputReceiveThread::Init() {
        ExitRequest.Set(false);
        return true;
    }

    uint32 FInputReceiveThread::Run() {
        FInputDevice& InputDevice = static_cast<FInputDevice&>(IOpenZICloudRenderModule::Get().GetInputDevice());
        FShareMemoryCallback Callback;
        FShareMemoryData* ReadData = new FShareMemoryData[1024];
#if PLATFORM_WINDOWS
        HANDLE SharedHandle = nullptr;
        while (!ExitRequest.GetValue()) {
            if (Reader->Read(ReadData, &Callback) >= 1 && SharedHandle != Reader->GetSharedHandle()) {
                SharedHandle = Reader->GetSharedHandle();
                uint32 Size = Reader->GetContentSize();
                FShareMemoryData* Content = Reader->GetContent();
                if (GEngine && GEngine->GameViewport != nullptr) {
                    InputDevice.OnMessage(Content, Size);
                }
            }
            FPlatformProcess::Sleep(0.000001f); // sleep 1 microsecond
        }
#elif PLATFORM_LINUX
            int SharedHandle = 0;
            while (!ExitRequest.GetValue()) {
                if (Reader->Read(ReadData, &Callback) >= 1 && SharedHandle != Reader->GetSharedHandle()) {
                    SharedHandle = Reader->GetSharedHandle();
                    UE_LOG(LogOpenZICloudRenderInputDevice, Verbose, TEXT("Received inputid=%d"), SharedHandle);
                    uint32 Size = Reader->GetContentSize();
                    FShareMemoryData* Content = Reader->GetContent();
                    if (GEngine && GEngine->GameViewport != nullptr) {
                        InputDevice.OnMessage(Content, Size);
                    }
                }
            FPlatformProcess::Sleep(0.000001f); // sleep 1 microsecond
            }
#endif
        delete[] ReadData;
        return 0;
    }
    void FInputReceiveThread::Stop() {
        ExitRequest.Set(true);
    }

    void FInputReceiveThread::Exit() {
        // empty
    }
} // namespace OpenZI::CloudRender
// PRAGMA_ENABLE_OPTIMIZATION