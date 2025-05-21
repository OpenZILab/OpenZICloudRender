// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/26 13:34
// 

#pragma once

#include "PixelStreamingPlayerId.h"
#include "WebRTCIncludes.h"
#include "Thread/CriticalSection.h"
#include "CoreMinimal.h"
#include "TextureSource.h"
#include "VideoEncoderNvEnc.h"
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace OpenZI::CloudRender
{
    class IPumpedVideoSource : public rtc::RefCountInterface
    {
    public:
        virtual void OnPump(int32_t FrameId) = 0;
        virtual bool IsReadyForPump() const = 0;

        /* rtc::RefCountInterface */
        virtual void AddRef() const = 0;
        virtual rtc::RefCountReleaseStatus Release() const = 0;
        virtual bool HasOneRef() const = 0;
    };

    /*
     * Runs a seperate thread that "pumps" at a fixed FPS interval. WebRTC video sources may add themselves to be "pumped"
     * at a fixed interval indepedent of render FPS or game thread FPS. This is useful so that poorly
     * performing applications can still submit a constant amount of frames to WebRTC and are not penalized with a
     * bitrate reduction.
     */
    class FFixedFPSPump
    {
    public:
        FFixedFPSPump();
        ~FFixedFPSPump();
        void Shutdown();
        void RegisterVideoSource(FPixelStreamingPlayerId PlayerId, IPumpedVideoSource *Source);
        void UnregisterVideoSource(FPixelStreamingPlayerId PlayerId);
        static FFixedFPSPump *Get();
        void EncodeAndSendH265Video();

    private:
        void PumpLoop();

    private:
        FCriticalSection SourcesGuard;
        std::map<FPixelStreamingPlayerId, IPumpedVideoSource*> VideoSources;
        std::unique_ptr<std::thread> PumpThread;
        std::condition_variable ConditionVariable;
        std::mutex ConditionVariableMutex;
        bool bThreadRunning = true;
        int32_t NextFrameId = 0;
        int32_t FPS = 30;
        static FFixedFPSPump *Instance;
        std::shared_ptr<ITextureSource> TextureSource;
        std::unique_ptr<FVideoEncoder> Encoder;
#if PLATFORM_WINDOWS
        LARGE_INTEGER Start;
        LARGE_INTEGER End;
        LARGE_INTEGER Frequency;
#elif PLATFORM_LINUX

#endif

    };
} // namespace OpenZI::CloudRender