//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2023/04/13 19:09
//

#include "AudioDeviceModule.h"

#include "Config.h"
#include "Logger/CommandLogger.h"

namespace OpenZI::CloudRender {
    // These are copied from webrtc internals
#define CHECKinitialized_()                                                                        \
    {                                                                                              \
        if (!bInitialized) {                                                                       \
            return -1;                                                                             \
        };                                                                                         \
    }
#define CHECKinitialized__BOOL()                                                                   \
    {                                                                                              \
        if (!bInitialized) {                                                                       \
            return false;                                                                          \
        };                                                                                         \
    }

    int32 FAudioDeviceModule::ActiveAudioLayer(AudioLayer* audioLayer) const {
        *audioLayer = AudioDeviceModule::kDummyAudio;
        return 0;
    }

    int32 FAudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport* audioCallback) {
        // ML_LOG(Main, Log, "AudioDevice RegisterAudioCallback");
        Requester.RegisterAudioCallback(audioCallback);
        Capturer.RegisterAudioCallback(audioCallback);
        return 0;
    }

    int32 FAudioDeviceModule::Init() {
        // ML_LOG(Main, Log, "AudioDevice Init");

        // ML_LOG(LogPixelStreamingAudioDeviceModule, Verbose, "Init PixelStreamingAudioDeviceModule");
        bInitialized = true;
        InitRecording();
        return 0;
    }

    int32 FAudioDeviceModule::Terminate() {
        // ML_LOG(Main, Log, "AudioDevice Terminate");
        if (!bInitialized) {
            return 0;
        }

        Requester.Uninitialise();
        Capturer.Uninitialise();

        // ML_LOG(LogPixelStreamingAudioDeviceModule, Verbose, "Terminate");

        return 0;
    }

    bool FAudioDeviceModule::Initialized() const { return bInitialized; }

    int16 FAudioDeviceModule::PlayoutDevices() {
        CHECKinitialized_();
        return -1;
    }

    int16 FAudioDeviceModule::RecordingDevices() {
        CHECKinitialized_();
        return -1;
    }

    int32 FAudioDeviceModule::PlayoutDeviceName(uint16 index,
                                                char name[webrtc::kAdmMaxDeviceNameSize],
                                                char guid[webrtc::kAdmMaxGuidSize]) {
        CHECKinitialized_();
        return -1;
    }

    int32 FAudioDeviceModule::RecordingDeviceName(uint16 index,
                                                  char name[webrtc::kAdmMaxDeviceNameSize],
                                                  char guid[webrtc::kAdmMaxGuidSize]) {
        CHECKinitialized_();
        return -1;
    }

    int32 FAudioDeviceModule::SetPlayoutDevice(uint16 index) {
        CHECKinitialized_();
        return 0;
    }

    int32 FAudioDeviceModule::SetPlayoutDevice(WindowsDeviceType device) {
        CHECKinitialized_();
        return 0;
    }

    int32 FAudioDeviceModule::SetRecordingDevice(uint16 index) {
        CHECKinitialized_();
        return 0;
    }

    int32 FAudioDeviceModule::SetRecordingDevice(WindowsDeviceType device) {
        CHECKinitialized_();
        return 0;
    }

    int32 FAudioDeviceModule::PlayoutIsAvailable(bool* available) {
        CHECKinitialized_();
        *available = !FAppConfig::Get().WebRTCDisableReceiveAudio;
        return 0;
    }

    int32 FAudioDeviceModule::InitPlayout() {
        CHECKinitialized_();
        bool bIsPlayoutAvailable = false;
        PlayoutIsAvailable(&bIsPlayoutAvailable);
        if (!bIsPlayoutAvailable) {
            return -1;
        }

        Requester.InitPlayout();
        return 0;
    }

    bool FAudioDeviceModule::PlayoutIsInitialized() const {
        CHECKinitialized__BOOL();
        return Requester.PlayoutIsInitialized();
    }

    int32 FAudioDeviceModule::StartPlayout() {
        CHECKinitialized_();

        if (!Requester.PlayoutIsInitialized()) {
            return -1;
        }

        Requester.StartPlayout();
        return 0;
    }

    int32 FAudioDeviceModule::StopPlayout() {
        CHECKinitialized_();
        if (!Requester.PlayoutIsInitialized()) {
            return -1;
        }

        Requester.StopPlayout();
        return 0;
    }

    bool FAudioDeviceModule::Playing() const {
        CHECKinitialized__BOOL();
        return Requester.Playing();
    }

    int32 FAudioDeviceModule::RecordingIsAvailable(bool* available) {
        CHECKinitialized_();
        *available = !FAppConfig::Get().WebRTCDisableTransmitAudio;
        return 0;
    }

    int32 FAudioDeviceModule::InitRecording() {
        // ML_LOG(Main, Log, "AudioDevice InitRecording");
        CHECKinitialized_();

        // ML_LOG(Main, Log, "AudioDevice InitRecording after");
        bool bIsRecordingAvailable = false;
        RecordingIsAvailable(&bIsRecordingAvailable);
        if (!bIsRecordingAvailable) {
            return -1;
        }

        // ML_LOG(Main, Log, "AudioDevice InitRecording after 2");
        if (!Capturer.IsInitialised()) {
            Capturer.Init();
            // StartRecording();
        }

        return 0;
    }

    bool FAudioDeviceModule::RecordingIsInitialized() const {
        CHECKinitialized__BOOL();
        return Capturer.IsInitialised();
    }

    int32 FAudioDeviceModule::StartRecording() {
        // ML_LOG(Main, Log, "AudioDevice StartRecording");
        CHECKinitialized_();
        if (!Capturer.IsCapturing()) {
            Capturer.StartCapturing();
        }
        return 0;
    }

    int32 FAudioDeviceModule::StopRecording() {
        // ML_LOG(Main, Log, "AudioDevice StopRecording");
        CHECKinitialized_();
        if (Capturer.IsCapturing()) {
            Capturer.EndCapturing();
        }
        return 0;
    }

    bool FAudioDeviceModule::Recording() const {
        CHECKinitialized__BOOL();
        return Capturer.IsCapturing();
    }

    int32 FAudioDeviceModule::InitSpeaker() {
        CHECKinitialized_();
        return -1;
    }

    bool FAudioDeviceModule::SpeakerIsInitialized() const {
        CHECKinitialized__BOOL();
        return false;
    }

    int32 FAudioDeviceModule::InitMicrophone() {
        CHECKinitialized_();
        return 0;
    }

    bool FAudioDeviceModule::MicrophoneIsInitialized() const {
        CHECKinitialized__BOOL();
        return true;
    }

    int32 FAudioDeviceModule::StereoPlayoutIsAvailable(bool* available) const {
        CHECKinitialized_();
        *available = true;
        return 0;
    }

    int32 FAudioDeviceModule::SetStereoPlayout(bool enable) {
        CHECKinitialized_();
        // FString AudioChannelStr = enable ? TEXT("stereo") : TEXT("mono");
        // UE_LOG(LogPixelStreamingAudioDeviceModule, Verbose,
        //        TEXT("WebRTC has requested browser audio playout in UE be: %s"),
        //        *AudioChannelStr);
        return 0;
    }

    int32 FAudioDeviceModule::StereoPlayout(bool* enabled) const {
        CHECKinitialized_();
        *enabled = true;
        return 0;
    }

    int32 FAudioDeviceModule::StereoRecordingIsAvailable(bool* available) const {
        CHECKinitialized_();
        *available = true;
        return 0;
    }

    int32 FAudioDeviceModule::SetStereoRecording(bool enable) {
        CHECKinitialized_();
        return 0;
    }

    int32 FAudioDeviceModule::StereoRecording(bool* enabled) const {
        CHECKinitialized_();
        *enabled = true;
        return 0;
    }

    // --FSubmixCapturer

    uint32_t FSubmixCapturer::GetVolume() const { return VolumeLevel; }

    void FSubmixCapturer::SetVolume(uint32_t NewVolume) { VolumeLevel = NewVolume; }

    bool FSubmixCapturer::Init() {
        // ML_LOG(Main, Log, "AudioDevice Capturer Init");
        FScopeLock Lock(&CriticalSection);

        // already initialised
        if (bInitialised) {
            // ML_LOG(Main, Log, "AudioDevice Capturer Init 2");
            return true;
        }

        SUBSCRIBE_MESSAGE_TwoParams(OnAudioReceived, this, &FSubmixCapturer::OnNewSubmixBuffer,
                                    uint8*, AudioData, uint32, DataSize);

        bInitialised = true;
        // ML_LOG(Main, Log, "AudioDevice Capturer Init 3");
        return true;
    }

    void FSubmixCapturer::OnNewSubmixBuffer(uint8* AudioData, uint32 DataSize) {
        FScopeLock Lock(&CriticalSection);

        if (!bInitialised || !bCapturing) {
            // ML_LOG(Main, Log, "AudioDevice OnNewSubmixBuffer, bInit=%d, bCap=%d", bInitialised.load(), bCapturing.load());
            return;
        }

        // No point doing anything with UE audio if the callback from WebRTC has not been set yet.
        if (AudioCallback == nullptr) {
            return;
        }
        // ML_LOG(Main, Log, "OnNewSubmixBuffer called, AudioDataSize = %d", DataSize);
        // // Check if the sample rate from UE matches our target sample rate
        // if (TargetSampleRate != SampleRate) {
        //     // Only report the problem once
        //     if (!bReportedSampleRateMismatch) {
        //         bReportedSampleRateMismatch = true;
        //         UE_LOG(LogPixelStreamingAudioDeviceModule, Error,
        //                TEXT("Audio sample rate mismatch. Expected: %d | Actual: %d"),
        //                TargetSampleRate, SampleRate);
        //     }
        //     return;
        // }

        // UE_LOG(LogPixelStreamingAudioDeviceModule, VeryVerbose,
        //        TEXT("captured %d samples, %dc, %dHz"), NumSamples, NumChannels, SampleRate);

        // Note: TSampleBuffer takes in AudioData as float* and internally converts to int16
        // Audio::TSampleBuffer<int16> Buffer(AudioData, NumSamples, NumChannels, SampleRate);

        // Mix to our target number of channels if the source does not already match.
        // if (Buffer.GetNumChannels() != TargetNumChannels) {
        //     Buffer.MixBufferToChannels(TargetNumChannels);
        // }
        // std::vector<int16> Buffer;
        // Buffer.assign((int16*)AudioData, DataSize / 2);
        uint32 BufferSize = DataSize / 2;
        RecordingBuffer.insert(RecordingBuffer.end(), (int16*)AudioData,
                               (int16*)AudioData + BufferSize);
        // RecordingBuffer.emplace_back()
        // RecordingBuffer.Append(Buffer.GetData(), Buffer.GetNumSamples());

        const float ChunkDurationSecs = 0.01f; // 10ms
        const int32 SamplesPer10Ms = GetSamplesPerDurationSecs(ChunkDurationSecs);

        // Feed in 10ms chunks
        while (RecordingBuffer.size() > SamplesPer10Ms) {

            // Extract a 10ms chunk of samples from recording buffer
            // std::vector<int16_t> SubmitBuffer(RecordingBuffer.GetData(), SamplesPer10Ms);
            std::vector<int16_t> SubmitBuffer;
            SubmitBuffer.insert(SubmitBuffer.end(), RecordingBuffer.begin(),
                                RecordingBuffer.begin() + SamplesPer10Ms);
            const size_t Frames = SubmitBuffer.size() / TargetNumChannels;
            const size_t BytesPerFrame = TargetNumChannels * sizeof(int16_t);

            uint32_t OutMicLevel = VolumeLevel;

            int32_t WebRTCRes = AudioCallback->RecordedDataIsAvailable(
                SubmitBuffer.data(), Frames, BytesPerFrame, TargetNumChannels, TargetSampleRate, 0,
                0, VolumeLevel, false, OutMicLevel);

            SetVolume(OutMicLevel);

            // Remove 10ms of samples from the recording buffer now it is submitted
            // RecordingBuffer.RemoveAt(0, SamplesPer10Ms, false);
            RecordingBuffer.erase(RecordingBuffer.begin(),
                                  RecordingBuffer.begin() + SamplesPer10Ms);
        }
    }

    int32 FSubmixCapturer::GetSamplesPerDurationSecs(float InSeconds) const {
        int32 SamplesPerSecond = TargetNumChannels * TargetSampleRate;
        int32 NumSamplesPerDuration = (int32)(SamplesPerSecond * InSeconds);
        return NumSamplesPerDuration;
    }

    void FSubmixCapturer::RegisterAudioCallback(webrtc::AudioTransport* AudioTransportCallback) {
        // ML_LOG(Main, Log, "AudioDevice RegisterAudioCallback");
        FScopeLock Lock(&CriticalSection);
        AudioCallback = AudioTransportCallback;
    }

    bool FSubmixCapturer::IsInitialised() const { return bInitialised; }

    void FSubmixCapturer::Uninitialise() {
        FScopeLock Lock(&CriticalSection);
        UNSUBSCRIBE_MESSAGE(OnAudioReceived);
        RecordingBuffer.clear();
        bInitialised = false;
        bCapturing = false;
    }

    bool FSubmixCapturer::StartCapturing() {
        // ML_LOG(Main, Log, "AudioDevice StartCapturing");
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

    //--FAudioPlayoutRequester--

    void FAudioPlayoutRequester::Uninitialise() {
        StopPlayout();
        AudioCallback = nullptr;
        bIsPlayoutInitialised = false;
        PlayoutBuffer.clear();
    }

    void FAudioPlayoutRequester::InitPlayout() { bIsPlayoutInitialised = true; }

    void FAudioPlayoutRequester::StartPlayout() {
        if (PlayoutIsInitialized() && !Playing()) {
            // ML_LOG(Main, Log, "AudioDevice Requester StartPlayout");
            std::function<void()> RequesterFunc = [this]() {
                FScopeLock Lock(&PlayoutCriticalSection);

                // Only request audio if audio callback is valid
                if (!AudioCallback) {
                    return;
                }

                // Our intention is to request samples at some fixed interval (i.e 10ms)
                int32 NSamplesPerChannel =
                    (SampleRate) / (1000 / FAudioPlayoutRequester::RequestIntervalMs);
                const size_t BytesPerFrame = NumChannels * sizeof(int16_t);

                // Ensure buffer has the total number of samples we need for all audio frames
                PlayoutBuffer.reserve(NSamplesPerChannel * NumChannels);

                size_t OutNSamples = 0;
                int64_t ElapsedTimeMs = -1;
                int64_t NtpTimeMs = -1;

                // Note this is mixed result of all audio sources, which in turn triggers the sinks
                // for each audio source to be called. For example, if you had 3 audio sources in
                // the browser sending 16kHz mono they would all be mixed down into the number of
                // channels and sample rate specified below. However, for listening to each audio
                // source prior to mixing refer to FAudioSink.
                uint32_t Result = AudioCallback->NeedMorePlayData(
                    NSamplesPerChannel, BytesPerFrame, NumChannels, SampleRate,
                    PlayoutBuffer.data(), OutNSamples, &ElapsedTimeMs, &NtpTimeMs);

                // if (Result != 0) {
                //     UE_LOG(LogPixelStreamingAudioDeviceModule, Error,
                //            TEXT("NeedMorePlayData return non-zero result indicating an error"));
                // }
            };

            // TODO: 开线程，执行RequesterFunc函数
            // RequesterThread.Reset(nullptr);
            // RequesterRunnable =
            //     MakeUnique<FAudioPlayoutRequester::Runnable>(RequesterFunc);
            // RequesterThread.Reset(FRunnableThread::Create(
            //     RequesterRunnable.Get(), TEXT("Pixel Streaming WebRTC Audio Requester")));
            bIsPlaying = true;
        }
    }

    void FAudioPlayoutRequester::StopPlayout() {
        // ML_LOG(Main, Log, "AudioDevice Requester StopPlayout");
        if (PlayoutIsInitialized() && Playing()) {
            // RequesterRunnable->Stop();
            bIsPlaying = false;
        }
    }

    bool FAudioPlayoutRequester::Playing() const { return bIsPlaying; }

    bool FAudioPlayoutRequester::PlayoutIsInitialized() const { return bIsPlayoutInitialised; }

    void FAudioPlayoutRequester::RegisterAudioCallback(webrtc::AudioTransport* AudioCb) {
        // ML_LOG(Main, Log, "AudioDevice Requester RegisterAudioCallback");
        FScopeLock Lock(&PlayoutCriticalSection);
        AudioCallback = AudioCb;
    }

    //--FAudioRequester::Runnable--

    // bool FAudioPlayoutRequester::Runnable::Init() {
    //     return RequestPlayoutFunc != nullptr;
    // }

    // uint32 FAudioPlayoutRequester::Runnable::Run() {
    //     LastAudioRequestTimeMs = rtc::TimeMillis();
    //     bIsRunning = true;

    //     // Request audio in a loop until this boolean is toggled off.
    //     while (bIsRunning) {
    //         int64_t Now = rtc::TimeMillis();
    //         int64_t DeltaMs = Now - LastAudioRequestTimeMs;

    //         // Check if the 10ms delta has elapsed, if it has not, then sleep the remaining
    //         if (DeltaMs < FAudioPlayoutRequester::RequestIntervalMs) {
    //             int64_t SleepTimeMs =
    //                 FAudioPlayoutRequester::RequestIntervalMs - DeltaMs;
    //             float SleepTimeSecs = (float)SleepTimeMs / 1000.0f;
    //             std::this_thread::sleep_for(std::chrono::seconds(SleepTimeSecs));
    //             // FPlatformProcess::Sleep(SleepTimeSecs);
    //             continue;
    //         }

    //         // Update request time to now seeing as the 10ms delta has elapsed
    //         LastAudioRequestTimeMs = Now;

    //         // Actually request playout
    //         RequestPlayoutFunc();
    //     }

    //     return 0;
    // }

    // void FAudioPlayoutRequester::Runnable::Stop() { bIsRunning = false; }

    // void FAudioPlayoutRequester::Runnable::Exit() {
    //     bIsRunning = false;
    //     RequestPlayoutFunc = nullptr;
    // }
} // namespace OpenZI::CloudRender