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
#pragma once
#include "Dom/JsonObject.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/Guid.h"
#include "Templates/SharedPointer.h"

namespace OpenZI::CloudRender {
    class FLaunchServerThreadBase : public FRunnable {
    public:
        FLaunchServerThreadBase(TSharedPtr<FJsonObject> InConfigRoot, FGuid Guid);
        ~FLaunchServerThreadBase();

    protected:
        virtual bool Init() override;
        virtual uint32 Run() override;
        virtual void Stop() override;
        virtual void Exit() override;

    protected:
        virtual bool IsCloudRenderServerRunning();
        virtual bool IsSignallingWebServerRunning();
        virtual bool IsTurnWebServerRunning();

        virtual void LaunchCloudRenderServer();
        virtual void LaunchSignallingWebServer();
        virtual void LaunchTurnWebServer();

        virtual void TermaniteCloudRenderServer();
        virtual void TermaniteSignallingWebServer();
        virtual void TermaniteTurnWebServer();

        virtual void TryRebootCloudRenderServer();
        virtual void TryRebootSignallingWebServer();
        virtual void TryRebootTurnWebServer();

        virtual void WriteConfigToWebRoot();

        FProcHandle CloudRenderServer;
        FProcHandle SignallingWebServer;
        FProcHandle TurnServerHandle;

        FString ProgramsPath;
        FString SignallingDirName;
        FString CloudRenderServerPath;
        FString SignallingWebServerPath;
        FString TurnWebServerPath;
        FString ShareMemorySuffix;

        bool bWatchCloudRenderServer;
        bool bWatchSignallingWebServer;
        bool bWatchTurnWebServer;

        FThreadSafeCounter ExitRequest;
        FRunnableThread* Thread;
        FGuid ShmId;
        TSharedPtr<FJsonObject> ConfigRoot;
    };
} // namespace OpenZI::CloudRender