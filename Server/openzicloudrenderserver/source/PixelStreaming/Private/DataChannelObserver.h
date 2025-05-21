// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 13:52
// 
#pragma once

#include "CoreMinimal.h"
#include "Thread/CriticalSection.h"
#include "WebRTCIncludes.h"
#include "PixelStreamingPlayerId.h"

namespace OpenZI::CloudRender
{
    class IPixelStreamingSessions;
    class FShareMemory;
    class ZInputSocketSender;
#if PLATFORM_WINDOWS
    class FShareMemoryWriter;
#elif PLATFORM_LINUX
    using FShareMemoryWriter = FShareMemory;
#endif

    // Observer on a particular player/peer's data channel.
    class FDataChannelObserver : public webrtc::DataChannelObserver
    {

    public:
        FDataChannelObserver(IPixelStreamingSessions *InSessions, FPixelStreamingPlayerId InPlayerId);

        // Begin webrtc::DataChannelObserver

        // The data channel state have changed.
        virtual void OnStateChange() override;

        //  A data buffer was successfully received.
        virtual void OnMessage(const webrtc::DataBuffer &buffer) override;

        // The data channel's buffered_amount has changed.
        virtual void OnBufferedAmountChange(uint64_t sent_data_size) override;

        // End webrtc::DataChannelObserver

        void SendInitialSettings() const;

        void Register(rtc::scoped_refptr<webrtc::DataChannelInterface> InDataChannel);
        void Unregister();

    private:
        void SendLatencyReport() const;

    private:
        rtc::scoped_refptr<webrtc::DataChannelInterface> DataChannel;
        IPixelStreamingSessions *PlayerSessions;
        FPixelStreamingPlayerId PlayerId;
#if PLATFORM_WINDOWS
        std::shared_ptr<FShareMemoryWriter> InputWriter;
#elif PLATFORM_LINUX
        std::shared_ptr<FShareMemoryWriter> InputWriter;
#endif
        uint64 InputId = 0;
    };
#if PLATFORM_LINUX
    // extern std::shared_ptr<ZInputSocketSender> InputSender;
    // extern FCriticalSection Mutex;
#endif
} // namespace OpenZI::CloudRender