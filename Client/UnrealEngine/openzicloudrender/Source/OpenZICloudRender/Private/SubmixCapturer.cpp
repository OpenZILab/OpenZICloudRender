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

#include "SubmixCapturer.h"
#include "AudioMixerDevice.h"
#include "Engine/GameEngine.h"
#include "IPCTypes.h"
#include "SampleBuffer.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOpenZICloudAudioDeviceModule, Log, All);
DEFINE_LOG_CATEGORY(LogOpenZICloudAudioDeviceModule);

namespace OpenZI::CloudRender {
    FSubmixCapturer::FSubmixCapturer(FGuid Guid)
        : bInitialised(false), bCapturing(false), TargetSampleRate(48000), TargetNumChannels(2),
          bReportedSampleRateMismatch(false), VolumeLevel(FSubmixCapturer::MaxVolumeLevel),
          RecordingBuffer(), CriticalSection() {

        ShmId = Guid;
        ShareHandlePrefix = Guid.ToString();
#if PLATFORM_WINDOWS
        ShareMemoryName =
            FString::Printf(TEXT("%s_%s"), TEXT("OpenZICloudAudio"), *ShareHandlePrefix);
        char* Name = TCHAR_TO_UTF8(*ShareMemoryName);
        Writer = MakeShareable(new FShareMemoryWriter(Name, 64 * 1024));
#elif PLATFORM_LINUX
        key_t ShmKey = ShmId.A - 3;
        Writer = MakeShareable(new FShareMemoryWriter(ShmKey, 64 * 1024));
#endif
        if (!Writer->Check()) {
            UE_LOG(LogOpenZICloudAudioDeviceModule, Error, TEXT("OpenZICloudAudio init failed."));
        } else {
            if (!IsInitialised()) {
                Init();
                StartCapturing();
            }
        }
    }
    uint32_t FSubmixCapturer::GetVolume() const { return VolumeLevel; }

    void FSubmixCapturer::SetVolume(uint32_t NewVolume) { VolumeLevel = NewVolume; }

    bool FSubmixCapturer::Init() {
        FScopeLock Lock(&CriticalSection);

        // subscribe to audio data
        if (!GEngine) {
            bInitialised = false;
            return false;
        }

        // already initialised
        if (bInitialised) {
            return true;
        }

        FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice();
        if (!AudioDevice) {
            UE_LOG(LogOpenZICloudAudioDeviceModule, Warning, TEXT("No audio device"));
            bInitialised = false;
            return false;
        }

        AudioDevice->RegisterSubmixBufferListener(this);
        bInitialised = true;

        UE_LOG(LogOpenZICloudAudioDeviceModule, Log, TEXT("Init openzi cloud audio capturer"));
        return true;
    }

    void FSubmixCapturer::OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData,
                                            int32 NumSamples, int32 NumChannels,
                                            const int32 SampleRate, double AudioClock) {
        FScopeLock Lock(&CriticalSection);

        if (!bInitialised || !bCapturing) {
            return;
        }

        // No point doing anything with UE audio if the callback from WebRTC has not been set yet.
        // if (AudioCallback == nullptr) {
        //     return;
        // }

        // Check if the sample rate from UE matches our target sample rate
        if (TargetSampleRate != SampleRate) {
            // Only report the problem once
            if (!bReportedSampleRateMismatch) {
                bReportedSampleRateMismatch = true;
                UE_LOG(LogOpenZICloudAudioDeviceModule, Error,
                       TEXT("Audio sample rate mismatch. Expected: %d | Actual: %d"),
                       TargetSampleRate, SampleRate);
            }
            return;
        }

        UE_LOG(LogOpenZICloudAudioDeviceModule, VeryVerbose, TEXT("captured %d samples, %dc, %dHz"),
               NumSamples, NumChannels, SampleRate);

        // Note: TSampleBuffer takes in AudioData as float* and internally converts to int16
        Audio::TSampleBuffer<int16> Buffer(AudioData, NumSamples, NumChannels, SampleRate);

        // Mix to our target number of channels if the source does not already match.
        if (Buffer.GetNumChannels() != TargetNumChannels) {
            Buffer.MixBufferToChannels(TargetNumChannels);
        }

        RecordingBuffer.Append(Buffer.GetData(), Buffer.GetNumSamples());

        UE_LOG(LogOpenZICloudAudioDeviceModule, VeryVerbose, TEXT("captured %d Bytes audio data"),
               RecordingBuffer.Num() / 4);
        // Write recording buffer to shared memory
        WriteToSharedMemory();

        // const float ChunkDurationSecs = 0.01f; // 10ms
        // const int32 SamplesPer10Ms = GetSamplesPerDurationSecs(ChunkDurationSecs);

        // // Feed in 10ms chunks
        // while (RecordingBuffer.Num() > SamplesPer10Ms) {

        //     // Extract a 10ms chunk of samples from recording buffer
        //     TArray<int16_t> SubmitBuffer(RecordingBuffer.GetData(), SamplesPer10Ms);
        //     const size_t Frames = SubmitBuffer.Num() / TargetNumChannels;
        //     const size_t BytesPerFrame = TargetNumChannels * sizeof(int16_t);

        //     uint32_t OutMicLevel = VolumeLevel;

        //     int32_t WebRTCRes = AudioCallback->RecordedDataIsAvailable(
        //         SubmitBuffer.GetData(), Frames, BytesPerFrame, TargetNumChannels,
        //         TargetSampleRate, 0, 0, VolumeLevel, false, OutMicLevel);

        //     SetVolume(OutMicLevel);

        //     // Remove 10ms of samples from the recording buffer now it is submitted
        //     RecordingBuffer.RemoveAt(0, SamplesPer10Ms, false);
        // }
    }

    void FSubmixCapturer::WriteToSharedMemory() {
        static uint64 Suffix = 0;
#if PLATFORM_WINDOWS
        Writer->Write((FShareMemoryData*)RecordingBuffer.GetData(),
                      RecordingBuffer.Num() * sizeof(int16_t), 0, 0, (HANDLE)++Suffix);
#elif PLATFORM_LINUX
        Writer->Write((FShareMemoryData*)RecordingBuffer.GetData(),
                      RecordingBuffer.Num() * sizeof(int16_t), 0, 0, ++Suffix);

#endif
        RecordingBuffer.RemoveAt(0, RecordingBuffer.Num(), false);
    }

    int32 FSubmixCapturer::GetSamplesPerDurationSecs(float InSeconds) const {
        int32 SamplesPerSecond = TargetNumChannels * TargetSampleRate;
        int32 NumSamplesPerDuration = (int32)(SamplesPerSecond * InSeconds);
        return NumSamplesPerDuration;
    }

    bool FSubmixCapturer::IsInitialised() const { return bInitialised; }

    void FSubmixCapturer::Uninitialise() {
        FScopeLock Lock(&CriticalSection);

        if (GEngine) {
            FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice();
            if (AudioDevice) {
                AudioDevice->UnregisterSubmixBufferListener(this);
            }
        }

        RecordingBuffer.Empty();
        bInitialised = false;
        bCapturing = false;
    }

    bool FSubmixCapturer::StartCapturing() {
        if (!bInitialised) {
            return false;
        }
        bCapturing = true;
        return true;
    }

    bool FSubmixCapturer::EndCapturing() {
        if (!bInitialised) {
            return false;
        }
        bCapturing = false;
        return true;
    }

    bool FSubmixCapturer::IsCapturing() const { return bCapturing; }
} // namespace OpenZI::CloudRender