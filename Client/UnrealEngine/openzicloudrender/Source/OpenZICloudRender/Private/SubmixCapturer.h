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

#include "AudioMixerDevice.h"
#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/CriticalSection.h"
#include "HAL/IConsoleManager.h"
#include "IPCTypes.h"
#include "Logging/LogMacros.h"
#include "Misc/Guid.h"
#include "Templates/Function.h"
#include "Templates/SharedPointer.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
#include "ISubmixBufferListener.h"
#endif

namespace OpenZI::CloudRender {
    // Captures audio from UE and passes it along to WebRTC.
    class FSubmixCapturer : public ISubmixBufferListener {

    public:
        // This magic number is the max volume used in webrtc fake audio device module.
        static const uint32_t MaxVolumeLevel = 14392;

        FSubmixCapturer(FGuid Guid);

        virtual ~FSubmixCapturer() = default;

        bool Init();
        bool IsInitialised() const;
        bool IsCapturing() const;
        void Uninitialise();
        bool StartCapturing();
        bool EndCapturing();
        uint32_t GetVolume() const;
        void SetVolume(uint32_t NewVolume);

        // ISubmixBufferListener interface
        void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples,
                               int32 NumChannels, const int32 SampleRate,
                               double AudioClock) override;
        void WriteToSharedMemory();

    private:
        int32 GetSamplesPerDurationSecs(float InSeconds) const;

    private:
        FThreadSafeBool bInitialised;
        FThreadSafeBool bCapturing;
        uint32 TargetSampleRate;
        uint8 TargetNumChannels;
        bool bReportedSampleRateMismatch;
        uint32_t VolumeLevel;
        TArray<int16_t> RecordingBuffer;
        FCriticalSection CriticalSection; // One thread captures audio from UE, the other controls
                                          // the state of the capturer.

        FString ShareMemoryName;
        FString ShareHandlePrefix;
        FGuid ShmId;
        TSharedPtr<FShareMemoryWriter> Writer;
    };
} // namespace OpenZI::CloudRender