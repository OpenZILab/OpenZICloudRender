// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/26 10:34
// 

#pragma once
#include <string>

namespace OpenZI::CloudRender
{
    using FPixelStreamingPlayerId = std::string;

    inline FPixelStreamingPlayerId ToPlayerId(std::string PlayerIdString)
    {
        return FPixelStreamingPlayerId(PlayerIdString);
    }

    inline FPixelStreamingPlayerId ToPlayerId(int32_t PlayerIdInteger)
    {
        return std::to_string(PlayerIdInteger);
    }

    inline int32_t PlayerIdToInt(FPixelStreamingPlayerId PlayerId)
    {
        return std::stoi(PlayerId);
    }

    static const FPixelStreamingPlayerId INVALID_PLAYER_ID = ToPlayerId("Invalid Player Id");
    static const FPixelStreamingPlayerId SFU_PLAYER_ID = FPixelStreamingPlayerId("1");
} // namespace OpenZI::CloudRender