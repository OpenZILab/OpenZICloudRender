// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/26 15:42
// 

#include "FixedFPSPump.h"
#include "Config.h"
#include "Logger/CommandLogger.h"
#include "SystemTime.h"
#include "WebRTCIncludes.h"
#include <chrono>

#include "VideoEncoderNvEnc.h"
#include "SystemTime.h"


namespace OpenZI::CloudRender
{
    FFixedFPSPump *FFixedFPSPump::Instance = nullptr;

    void FFixedFPSPump::EncodeAndSendH265Video() {

    }

    FFixedFPSPump *FFixedFPSPump::Get()
    {
        return Instance;
    }

    FFixedFPSPump::FFixedFPSPump()
    {
        FPS = FAppConfig::Get().WebRTCFps;
#if PLATFORM_WINDOWS
        QueryPerformanceFrequency(&Frequency);
#elif PLATFORM_LINUX

#endif
        // FIXME(20230926): 初始化过早，分辨率没用到配置文件中的分辨率
        Encoder = std::make_unique<FVideoEncoderNvEnc>();
        TextureSource = std::make_shared<FTextureSourceBackBuffer>(1.0f);
        PumpThread = std::make_unique<std::thread>([this]() { PumpLoop(); });
        Instance = this;
    }

    FFixedFPSPump::~FFixedFPSPump()
    {
        Shutdown();
        PumpThread->join();
    }

    void FFixedFPSPump::Shutdown()
    {
        bThreadRunning = false;
        ConditionVariable.notify_one();
    }

    void FFixedFPSPump::UnregisterVideoSource(FPixelStreamingPlayerId PlayerId)
    {
        FScopeLock Lock(&SourcesGuard);
        VideoSources.erase(PlayerId);
        ConditionVariable.notify_one();
    }

    void FFixedFPSPump::RegisterVideoSource(FPixelStreamingPlayerId PlayerId, IPumpedVideoSource* Source)
    {
        FScopeLock Lock(&SourcesGuard);
        VideoSources[PlayerId] = Source;
        // ML_LOG(FixedFPSPump, Log, "RegisterVideoSource");
        ConditionVariable.notify_one();
    }

    void FFixedFPSPump::PumpLoop()
    {
#if PLATFORM_WINDOWS
        QueryPerformanceCounter(&Start);
        uint64 LastCycles = Start.QuadPart;
#elif PLATFORM_LINUX
        auto LastCycles = webrtc::Clock::GetRealTimeClock()->TimeInMicroseconds();
        // double LastCycles = GetTotalMilliseconds(GetNowTicks());
        // uint64 LastCycles = 0;
#endif
        while (bThreadRunning)
        {
            if (VideoSources.size() == 0)
            {
                std::unique_lock ConditionVariableLock(ConditionVariableMutex);
                ConditionVariable.wait(ConditionVariableLock);
                // ML_LOG(FixedFPSPump, Log, "PumpLoop waited the conditionvariable");
            }

            {
                FScopeLock Lock(&SourcesGuard);
                const int32_t FrameId = NextFrameId++;
                FBufferInfo BufferInfo;
                BufferInfo.FrameIndex = FrameId;
                BufferInfo.TimestampRTP = GetTimestampUs();
                static bool bFirst = true;
                if (bFirst) {
                    bFirst = false;
                    BufferInfo.IsKeyFrame = true;
                }
                static int KeyFrameInterval = 0;
                if (++KeyFrameInterval >= 600) {
                    KeyFrameInterval = 0;
                    BufferInfo.IsKeyFrame = true;
                }
                if (TextureSource->IsAvailable()) {
                    Encoder->Transmit(TextureSource->GetTexture(), BufferInfo);
                }
                // for (const auto& Item : VideoSources)
                // {
                //     IPumpedVideoSource* VideoSource = Item.second;
                //     if (VideoSource->IsReadyForPump())
                //     {
                //         // ML_LOG(FixedFPSPump, Verbose, "PumpLoop is ready, count=%d", VideoSources.size());
                //         VideoSource->OnPump(FrameId);
                //     } else {
                //         // ML_LOG(FixedFPSPump, Verbose, "PumpLoop is not ready, count=%d", VideoSources.size());
                //     }
                // }
            }

#if PLATFORM_WINDOWS
            QueryPerformanceCounter(&End);
            const uint64 EndCycles = End.QuadPart;
            const double DeltaMs = (EndCycles - LastCycles) * 1000.0 / Frequency.QuadPart;
#elif PLATFORM_LINUX
            // const double EndCycles = GetTotalMilliseconds(GetNowTicks());
            const auto EndCycles = webrtc::Clock::GetRealTimeClock()->TimeInMicroseconds();
            const double DeltaMs = (EndCycles - LastCycles) / 1000.0;
#endif
            const double FrameDeltaMs = 1000.0 / FAppConfig::Get().WebRTCFps;
            const double SleepMs = FrameDeltaMs - DeltaMs;
            // ML_LOG(FixedFPSPump, Verbose, "LastCycles=%llu,EndCycles=%llu,DeltaMs=%f,SleepMs=%f", LastCycles, EndCycles, DeltaMs, SleepMs);
            // ML_LOG(FixedFPSPump, Verbose, "LastCycles=%f,EndCycles=%f,DeltaMs=%f,SleepMs=%f", LastCycles, EndCycles, DeltaMs, SleepMs);
            LastCycles = EndCycles;
            if (SleepMs > 0)
            {
                std::unique_lock ConditionVariableLock(ConditionVariableMutex);
                ConditionVariable.wait_for(ConditionVariableLock, std::chrono::duration<double, std::milli>(SleepMs));
                // Sleep(SleepMs);
            }
        }
    }
} // namespace OpenZI::CloudRender