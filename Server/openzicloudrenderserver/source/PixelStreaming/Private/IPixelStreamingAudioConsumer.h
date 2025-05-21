// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 10:55
// 
#pragma once
#include "CoreMinimal.h"

namespace OpenZI::CloudRender
{
    // Interface for consuming audio coming in from the browser.
    class IPixelStreamingAudioConsumer
    {
    public:
        IPixelStreamingAudioConsumer(){};
        virtual ~IPixelStreamingAudioConsumer(){};
        virtual void ConsumeRawPCM(const int16 *AudioData, int InSampleRate, size_t NChannels, size_t NFrames) = 0;
        virtual void OnConsumerAdded() = 0;
        virtual void OnConsumerRemoved() = 0;
    };
} // namespace OpenZI::CloudRender