// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 10:54
// 
#pragma once

namespace OpenZI::CloudRender
{
    class IPixelStreamingAudioConsumer;

    // Interface for a sink that collects audio coming in from the browser and passes into into UE's audio system.
    class IPixelStreamingAudioSink
    {
    public:
        IPixelStreamingAudioSink() {}
        virtual ~IPixelStreamingAudioSink() {}
        virtual void AddAudioConsumer(IPixelStreamingAudioConsumer *AudioConsumer) = 0;
        virtual void RemoveAudioConsumer(IPixelStreamingAudioConsumer *AudioConsumer) = 0;
        virtual bool HasAudioConsumers() = 0;
    };
} // namespace OpenZI::CloudRender