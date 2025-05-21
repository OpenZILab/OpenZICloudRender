// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/16 10:42
// 
#pragma once

#include "CoreMinimal.h"
#include "RHIResources.h"
#include <functional>

namespace OpenZI::CloudRender
{
    struct FBufferInfo
    {
    public:
        uint64 FrameIndex;
        uint32 TimestampRTP;
        int64 TimestampUs;
        int VideoQP;
        bool IsKeyFrame;
        int64 StartTimestampUs;
        int64 FinishTimestampUs;
    };

    class FVideoEncoder
    {
    public:
        enum class ERateControlMode
        {
            UNKNOWN,
            CONSTQP,
            VBR,
            CBR
        };
        enum class EMultipassMode
        {
            UNKNOWN,
            DISABLED,
            QUARTER,
            FULL
        };
        enum class EH264Profile
        {
            AUTO,
            CONSTRAINED_BASELINE,
            BASELINE,
            MAIN,
            CONSTRAINED_HIGH,
            HIGH,
            HIGH444,
            STEREO,
            SVC_TEMPORAL_SCALABILITY,
            PROGRESSIVE_HIGH
        };
        enum class EH265Profile {
            AUTO,
            MAIN,
            MAIN10
        };

        struct FLayerConfig
        {
            uint32 Width = 0;
            uint32 Height = 0;
            uint32 MaxFramerate = 0;
            int32 MaxBitrate = 0;
            int32 TargetBitrate = 0;
            int32 QPMax = -1;
            int32 QPMin = -1;
            ERateControlMode RateControlMode = ERateControlMode::CBR;
            EMultipassMode MultipassMode = EMultipassMode::FULL;
            bool FillData = false;
            EH264Profile H264Profile = EH264Profile::BASELINE;

            bool operator==(FLayerConfig const &other) const
            {
                return Width == other.Width && Height == other.Height && MaxFramerate == other.MaxFramerate && MaxBitrate == other.MaxBitrate && TargetBitrate == other.TargetBitrate && QPMax == other.QPMax && QPMin == other.QPMin && RateControlMode == other.RateControlMode && MultipassMode == other.MultipassMode && FillData == other.FillData && H264Profile == other.H264Profile;
            }

            bool operator!=(FLayerConfig const &other) const
            {
                return !(*this == other);
            }
        };
        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual void Transmit(FTexture2DRHIRef Texture2DRHIRef, FBufferInfo BufferInfo) = 0;
        virtual void SetOnEncodedPacket(std::function<void(uint32, std::vector<uint8> &, const FBufferInfo)> InOnEncodedPacket) = 0;
    };
} // namespace OpenZI::CloudRender