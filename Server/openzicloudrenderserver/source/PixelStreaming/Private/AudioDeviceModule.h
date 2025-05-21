//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2023/04/13 19:04
//

#pragma once

#include "CoreMinimal.h"
#include "Thread/CriticalSection.h"
#include "MessageCenter/MessageCenter.h"
#include "WebRTCIncludes.h"

#include <atomic>

namespace OpenZI::CloudRender {
    class FSubmixCapturer {

    public:
        // This magic number is the max volume used in webrtc fake audio device module.
        static const uint32_t MaxVolumeLevel = 14392;

        FSubmixCapturer()
            : bInitialised(false), bCapturing(false), TargetSampleRate(48000), TargetNumChannels(2),
              bReportedSampleRateMismatch(false), AudioCallback(nullptr),
              VolumeLevel(FSubmixCapturer::MaxVolumeLevel), RecordingBuffer(), CriticalSection() {}

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
        void OnNewSubmixBuffer(uint8* AudioData, uint32 DataSize);

        void RegisterAudioCallback(webrtc::AudioTransport* AudioCb);

    private:
        int32 GetSamplesPerDurationSecs(float InSeconds) const;

    private:
        std::atomic<bool> bInitialised;
        std::atomic<bool> bCapturing;
        uint32 TargetSampleRate;
        uint8 TargetNumChannels;
        bool bReportedSampleRateMismatch;
        webrtc::AudioTransport* AudioCallback;
        uint32_t VolumeLevel;
        std::vector<int16_t> RecordingBuffer;
        FCriticalSection CriticalSection; // One thread captures audio from UE, the other controls
                                          // the state of the capturer.
        DECLARE_MESSAGE(OnAudioReceived);
    };

    // Requests audio from WebRTC at a regular interval (10ms)
    // This is required so that WebRTC audio sinks actually have
    // some audio data for their sinks. Without this WebRTC assumes
    // there is no demand for audio and does not populate the sinks.
    class FAudioPlayoutRequester {
    public:
        // class Runnable : public FRunnable {
        // public:
        //     Runnable(std::function<void()> RequestPlayoutFunc)
        //         : bIsRunning(false), LastAudioRequestTimeMs(0),
        //           RequestPlayoutFunc(RequestPlayoutFunc) {}
        //     // Begin FRunnable interface.
        //     virtual bool Init() override;
        //     virtual uint32 Run() override;
        //     virtual void Stop() override;
        //     virtual void Exit() override;
        //     // End FRunnable interface
        // private:
        //     bool bIsRunning;
        //     int64_t LastAudioRequestTimeMs;
        //     std::function<void()> RequestPlayoutFunc;
        // };

        FAudioPlayoutRequester()
            : bIsPlayoutInitialised(false), bIsPlaying(false),
              SampleRate(
                  48000) // webrtc will mix all sources into one big buffer with this sample rate
              ,
              NumChannels(
                  2) // webrtc will mix all sources into one big buffer with this many channels
              ,
              /*RequesterRunnable(), RequesterThread(), */ AudioCallback(nullptr),
              PlayoutCriticalSection(), PlayoutBuffer(){};

        void InitPlayout();
        void StartPlayout();
        void StopPlayout();
        bool Playing() const;
        bool PlayoutIsInitialized() const;
        void Uninitialise();
        void RegisterAudioCallback(webrtc::AudioTransport* AudioCallback);

    public:
        static int16_t const RequestIntervalMs = 10;

    private:
        std::atomic<bool> bIsPlayoutInitialised;
        std::atomic<bool> bIsPlaying;
        uint32 SampleRate;
        uint8 NumChannels;
        // TUniquePtr<FAudioPlayoutRequester::Runnable> RequesterRunnable;
        // TUniquePtr<FRunnableThread> RequesterThread;
        webrtc::AudioTransport* AudioCallback;
        FCriticalSection PlayoutCriticalSection;
        std::vector<int16_t> PlayoutBuffer;
    };

    // A custom audio device module for WebRTC.
    // It lets us receive WebRTC audio in UE
    // and also transmit UE audio to WebRTC.
    class FAudioDeviceModule : public webrtc::AudioDeviceModule {

    public:
        FAudioDeviceModule() : bInitialized(false), Capturer(), Requester(){};

        virtual ~FAudioDeviceModule() = default;

    private:
        std::atomic<bool> bInitialized; // True when we setup capturer/playout requester.
        FSubmixCapturer Capturer;
        FAudioPlayoutRequester Requester;

    private:
        //
        // webrtc::AudioDeviceModule interface
        //
        int32 ActiveAudioLayer(AudioLayer* audioLayer) const override;
        int32 RegisterAudioCallback(webrtc::AudioTransport* audioCallback) override;

        // Main initialization and termination
        int32 Init() override;
        int32 Terminate() override;
        bool Initialized() const override;

        // Device enumeration
        int16 PlayoutDevices() override;
        int16 RecordingDevices() override;
        int32 PlayoutDeviceName(uint16 index, char name[webrtc::kAdmMaxDeviceNameSize],
                                char guid[webrtc::kAdmMaxGuidSize]) override;
        int32 RecordingDeviceName(uint16 index, char name[webrtc::kAdmMaxDeviceNameSize],
                                  char guid[webrtc::kAdmMaxGuidSize]) override;

        // Device selection
        int32 SetPlayoutDevice(uint16 index) override;
        int32 SetPlayoutDevice(WindowsDeviceType device) override;
        int32 SetRecordingDevice(uint16 index) override;
        int32 SetRecordingDevice(WindowsDeviceType device) override;

        // Audio transport initialization
        int32 PlayoutIsAvailable(bool* available) override;
        int32 InitPlayout() override;
        bool PlayoutIsInitialized() const override;
        int32 RecordingIsAvailable(bool* available) override;
        int32 InitRecording() override;
        bool RecordingIsInitialized() const override;

        // Audio transport control
        virtual int32 StartPlayout() override;
        virtual int32 StopPlayout() override;

        // True when audio is being pulled by the instance.
        virtual bool Playing() const override;

        virtual int32 StartRecording() override;
        virtual int32 StopRecording() override;
        virtual bool Recording() const override;

        // Audio mixer initialization
        virtual int32 InitSpeaker() override;
        virtual bool SpeakerIsInitialized() const override;
        virtual int32 InitMicrophone() override;
        virtual bool MicrophoneIsInitialized() const override;

        // Speaker volume controls
        virtual int32 SpeakerVolumeIsAvailable(bool* available) override { return -1; }
        virtual int32 SetSpeakerVolume(uint32 volume) override { return -1; }
        virtual int32 SpeakerVolume(uint32* volume) const override { return -1; }
        virtual int32 MaxSpeakerVolume(uint32* maxVolume) const override { return -1; }
        virtual int32 MinSpeakerVolume(uint32* minVolume) const override { return -1; }

        // Microphone volume controls
        virtual int32 MicrophoneVolumeIsAvailable(bool* available) override { return 0; }
        virtual int32 SetMicrophoneVolume(uint32 volume) override {
            // Capturer.SetVolume(volume);
            return 0;
        }
        virtual int32 MicrophoneVolume(uint32* volume) const override {
            //*volume = Capturer.GetVolume();
            return 0;
        }
        virtual int32 MaxMicrophoneVolume(uint32* maxVolume) const override {
            *maxVolume = FSubmixCapturer::MaxVolumeLevel;
            return 0;
        }
        virtual int32 MinMicrophoneVolume(uint32* minVolume) const override { return 0; }

        // Speaker mute control
        virtual int32 SpeakerMuteIsAvailable(bool* available) override { return -1; }
        virtual int32 SetSpeakerMute(bool enable) override { return -1; }
        virtual int32 SpeakerMute(bool* enabled) const override { return -1; }

        // Microphone mute control
        virtual int32 MicrophoneMuteIsAvailable(bool* available) override {
            *available = false;
            return -1;
        }
        virtual int32 SetMicrophoneMute(bool enable) override { return -1; }
        virtual int32 MicrophoneMute(bool* enabled) const override { return -1; }

        // Stereo support
        virtual int32 StereoPlayoutIsAvailable(bool* available) const override;
        virtual int32 SetStereoPlayout(bool enable) override;
        virtual int32 StereoPlayout(bool* enabled) const override;
        virtual int32 StereoRecordingIsAvailable(bool* available) const override;
        virtual int32 SetStereoRecording(bool enable) override;
        virtual int32 StereoRecording(bool* enabled) const override;

        // Playout delay
        virtual int32 PlayoutDelay(uint16* delayMS) const override {
            *delayMS = 0;
            return 0;
        }

        virtual bool BuiltInAECIsAvailable() const override { return false; }
        virtual bool BuiltInAGCIsAvailable() const override { return false; }
        virtual bool BuiltInNSIsAvailable() const override { return false; }

        // Enables the built-in audio effects. Only supported on Android.
        virtual int32 EnableBuiltInAEC(bool enable) override { return -1; }
        virtual int32 EnableBuiltInAGC(bool enable) override { return -1; }
        virtual int32 EnableBuiltInNS(bool enable) override { return -1; }
    };
} // namespace OpenZI::CloudRender