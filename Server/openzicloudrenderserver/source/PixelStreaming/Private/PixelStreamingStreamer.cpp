// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/05 16:03
// 
#include "PixelStreamingStreamer.h"
#include "Streamer.h"

namespace OpenZI::CloudRender
{
    FPixelStreamingStreamer::FPixelStreamingStreamer()
    {
        Streamer = std::make_shared<FStreamer>();
    }
    FPixelStreamingStreamer::~FPixelStreamingStreamer()
    {
        Streamer.reset();
    }
} // namespace OpenZI::CloudRender