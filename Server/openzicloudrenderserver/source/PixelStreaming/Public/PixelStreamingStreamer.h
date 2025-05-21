// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/05 16:03
// 
#pragma once
#include <memory>

namespace OpenZI::CloudRender
{
    class FStreamer;

    class FPixelStreamingStreamer
    {
    public:
        FPixelStreamingStreamer();
        ~FPixelStreamingStreamer();


    private:
        std::shared_ptr<FStreamer> Streamer;
    };
} // namespace OpenZI::CloudRender