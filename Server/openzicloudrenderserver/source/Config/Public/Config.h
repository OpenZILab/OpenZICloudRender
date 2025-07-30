/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3, (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.gnu.org/licenses/agpl-3.0.en.html#license-text
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/

#pragma once

#include "CoreMinimal.h"
#include <string>

namespace OpenZI::CloudRender
{
    class FAppConfig
    {
    public:
        static FAppConfig& Get();

        uint32 Width;
        uint32 Height;
        // uint32 RenderWidth;
        // uint32 RenderHeight;
        uint32 ResolutionX;
        uint32 ResolutionY;

        // config parameters
        uint32 FPS;
        uint32 BitRate;

        int32 EncoderTargetBitrate;
        int32 EncoderMaxBitrate;
        bool DebugDumpFrame;
        int32 EncoderMinQP;
        int32 EncoderMaxQP;
        std::string EncoderRateControl;
        bool EnableFillerData;
        std::string EncoderMultipass;
        std::string H264Profile;
        std::string H265Profile;
        int32 EncoderKeyframeInterval;
        std::string EncoderCodec;
        // End Encoder CVars

        // Begin WebRTC CVars
        std::string DegradationPreference;
        int32 WebRTCFps;
        int32 WebRTCStartBitrate;
        int32 WebRTCMinBitrate;
        int32 WebRTCMaxBitrate;
        int WebRTCLowQpThreshold;
        int WebRTCHighQpThreshold;
        bool WebRTCDisableReceiveAudio;
        bool WebRTCDisableTransmitAudio;
        bool WebRTCDisableAudioSync;
        bool WebRTCUseLegacyAudioDevice;
        bool WebRTCDisableStats;
        // End WebRTC CVars

        // Begin Pixel Streaming Plugin CVars
        bool AllowConsoleCommands;
        bool OnScreenStats;
        bool LogStats;

        int32 FreezeFrameQuality;
        bool SendPlayerIdAsInteger;
        bool DisableLatencyTester;

        std::string SignallingServerUrl;
        std::string StreamerId;
        std::string RHIName;

        bool bEnableDynamicFps;
        bool bEncoderUseLowPreset;
        std::string EncoderTuningInfo;
        int32 EncoderGopLength;

        std::string ShareMemorySuffix;
        uint32 Guid;
        bool bUseGraphicsAdapter;
        int GpuIndex;

        uint32 AdapterLuidLowPart;
        uint32 AdapterLuidHighPart;
        uint32 AdapterDeviceId;
        std::string PipelineUUID;
    private:
        FAppConfig();
        FAppConfig(const FAppConfig&) = delete;
        FAppConfig(FAppConfig&&) = delete;
        FAppConfig& operator=(const FAppConfig&) = delete;
        FAppConfig& operator=(FAppConfig&&) = delete;
        void InitializeDefaultConfigValue();
    };
} // namespace OpenZI::CloudRender