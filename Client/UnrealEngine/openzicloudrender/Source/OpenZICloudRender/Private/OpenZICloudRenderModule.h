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

#include "IOpenZICloudRenderModule.h"
#include "InputDevice.h"
#include "RHI.h"
#include "Tickable.h"

namespace OpenZI::CloudRender {
    class FInputReceiveThread;
    class FInputSocketReceiver;
    class FSendThread;
    class FLaunchServerThreadBase;
    class FSubmixCapturer;
    /**
     * This plugin allows the back buffer to be sent as a compressed video across
     * a network.
     */
    class FOpenZICloudRenderModule : public IOpenZICloudRenderModule, public FTickableGameObject {
    public:
        static IOpenZICloudRenderModule* GetModule();

    private:
        /** IModuleInterface implementation */
        void StartupModule() override;
        void ShutdownModule() override;
        void InitCloudRender();
        void InitLaunchFile(FString LaunchFileName);
        TSharedPtr<class IInputDevice> CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;

        /** IOpenZICloudRenderModule implementation */
        IOpenZICloudRenderModule::FReadyEvent& OnReady() override;
        bool IsReady() override;
        IInputDevice& GetInputDevice() override;

        /**
         * Returns a shared pointer to the device which handles pixel streaming
         * input.
         * @return The shared pointer to the input device.
         */
        TSharedPtr<FInputDevice> GetInputDevicePtr();

        // FTickableGameObject
        bool IsTickableWhenPaused() const override;
        bool IsTickableInEditor() const override;
        void Tick(float DeltaTime) override;
        TStatId GetStatId() const override;

    private:
        IOpenZICloudRenderModule::FReadyEvent ReadyEvent;
        TSharedPtr<FInputDevice> InputDevice;
        bool bFrozen = false;
        bool bCaptureNextBackBufferAndStream = false;
        double LastVideoEncoderQPReportTime = 0;
        static IOpenZICloudRenderModule* OpenZICloudRenderModule;
#if PLATFORM_WINDOWS
        TSharedPtr<FInputReceiveThread> InputReceiveThread;
#elif PLATFORM_LINUX
        TSharedPtr<FInputReceiveThread> InputReceiveThread;
        TSharedPtr<ZInputSocketReceiver> InputReceiver;
#endif

        TSharedPtr<FSendThread> SendThread;
        TSharedPtr<FLaunchServerThreadBase> LaunchServerThread;
        TSharedPtr<FSubmixCapturer> SubmixCapturer;
    };
} // namespace OpenZI::CloudRender