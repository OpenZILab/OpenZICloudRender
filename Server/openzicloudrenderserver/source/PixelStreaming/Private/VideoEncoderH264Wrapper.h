// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/01 10:40
// 
#pragma once
#include "WebRTCIncludes.h"
#include "VideoEncoder.h"
#include "CoreMinimal.h"
#include <memory>


namespace OpenZI::CloudRender
{
    class FVideoEncoderFactory;

    class FVideoEncoderH264Wrapper
    {
    public:
        FVideoEncoderH264Wrapper(uint64 EncoderId, std::unique_ptr<FVideoEncoder> Encoder);
        ~FVideoEncoderH264Wrapper();

        uint64 GetId() const { return Id; }

        void SetForceNextKeyframe() { bForceNextKeyframe = true; }

        void Encode(const webrtc::VideoFrame &WebRTCFrame, bool bKeyframe);

        static void OnEncodedPacket(uint64 SourceEncoderId,
                                    FVideoEncoderFactory *Factory,
                                    uint32 InLayerIndex, std::vector<uint8_t>& Buffer,
                                    const FBufferInfo BufferInfo
                                    );

    private:
        uint64 Id;
        std::unique_ptr<FVideoEncoder> Encoder;
        bool bForceNextKeyframe = false;
        FBufferInfo BufferInfo;
    };
} // namespace OpenZI::CloudRender