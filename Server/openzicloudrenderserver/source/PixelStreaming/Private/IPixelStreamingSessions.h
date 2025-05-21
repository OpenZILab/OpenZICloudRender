// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 10:58
// 
#pragma once

#include "IPixelStreamingAudioSink.h"
#include "PixelStreamingPlayerId.h"
#include "ProtocolDefs.h"
#include <string>
#include <vector>

namespace OpenZI::CloudRender
{
    // `FPlayerSession` is only directly accessible to `FThreadSafePlayerSessions` as it completely owns the lifecycle of those objects.
    // This is nice from a safety point of view; however, some other objects in Pixel Streaming need to interface with `FPlayerSession`.
    // By using `FPixelStreamingPlayerId` users can use this `IPixelStreamingSessions` interface to perform a limited set of allowable
    // actions on `FPlayerSession` objects.
    class IPixelStreamingSessions
    {
    public:
        virtual int GetNumPlayers() const = 0;
        virtual IPixelStreamingAudioSink *GetAudioSink(FPixelStreamingPlayerId PlayerId) const = 0;
        virtual IPixelStreamingAudioSink *GetUnlistenedAudioSink() const = 0;
        virtual bool IsQualityController(FPixelStreamingPlayerId PlayerId) const = 0;
        virtual void SetQualityController(FPixelStreamingPlayerId PlayerId) = 0;
        virtual bool Send(FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type, const std::string &Descriptor) const = 0;
        virtual void SendLatestQP(FPixelStreamingPlayerId PlayerId, int LatestQP) const = 0;
        virtual void SendFreezeFrameTo(FPixelStreamingPlayerId PlayerId, const std::vector<uint8> &JpegBytes) const = 0;
        virtual void PollWebRTCStats() const = 0;
    };
} // namespace OpenZI::CloudRender