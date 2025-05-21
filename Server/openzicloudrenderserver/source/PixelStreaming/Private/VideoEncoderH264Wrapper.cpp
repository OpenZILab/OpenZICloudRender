//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/01 10:49
//
#include "VideoEncoderH264Wrapper.h"
#include "EncoderFactory.h"
#include "FrameBuffer.h"
#include "SystemTime.h"
#include "Config.h"
#include "Logger/CommandLogger.h"

namespace OpenZI::CloudRender
{
    FVideoEncoderH264Wrapper::FVideoEncoderH264Wrapper(uint64 EncoderId, std::unique_ptr<FVideoEncoder> InEncoder)
        : Id(EncoderId), Encoder(std::move(InEncoder))
    {
        // FVideoEncoderH264Wrapper::bH264 =
        //     FAppConfig::Get().EncoderCodec == "H264" || FAppConfig::Get().EncoderCodec == "h264";
    }

    FVideoEncoderH264Wrapper::~FVideoEncoderH264Wrapper()
    {
    }

    void FVideoEncoderH264Wrapper::Encode(const webrtc::VideoFrame &WebRTCFrame, bool bKeyframe)
    {
        FTexture2DRHIRef SourceTexture;

        FFrameBuffer *FrameBuffer = static_cast<FFrameBuffer *>(WebRTCFrame.video_frame_buffer().get());
        FLayerFrameBuffer *LayerFrameBuffer = static_cast<FLayerFrameBuffer *>(FrameBuffer);
        SourceTexture = LayerFrameBuffer->GetFrame();

        if (SourceTexture.IsValid())
        {
            BufferInfo.TimestampRTP = WebRTCFrame.timestamp();
            BufferInfo.TimestampUs = WebRTCFrame.timestamp_us();
            BufferInfo.FrameIndex = WebRTCFrame.id();
            BufferInfo.IsKeyFrame = bKeyframe || bForceNextKeyframe;
            bForceNextKeyframe = false;
            Encoder->Transmit(SourceTexture, BufferInfo);
        }
    }

    /* ------------------ Static functions below --------------------- */

    void FVideoEncoderH264Wrapper::OnEncodedPacket(uint64 SourceEncoderId,
                                                   FVideoEncoderFactory *Factory,
                                                   uint32 InLayerIndex, std::vector<uint8_t> &Buffer, const FBufferInfo BufferInfo)
    {
        static bool bH264 = 
            FAppConfig::Get().EncoderCodec == "H264" || FAppConfig::Get().EncoderCodec == "h264";
        webrtc::EncodedImage Image;

        // webrtc::RTPFragmentationHeader FragHeader;
        // std::vector<webrtc::H264::NaluIndex> NALUIndices = webrtc::H264::FindNaluIndices(Buffer.data(), Buffer.size());
        // FragHeader.VerifyAndAllocateFragmentationHeader(NALUIndices.size());
        // FragHeader.fragmentationVectorSize = static_cast<uint16_t>(NALUIndices.size());
        // for (int i = 0; i != NALUIndices.size(); ++i)
        // {
        //     webrtc::H264::NaluIndex const &NALUIndex = NALUIndices[i];
        //     FragHeader.fragmentationOffset[i] = NALUIndex.payload_start_offset;
        //     FragHeader.fragmentationLength[i] = NALUIndex.payload_size;
        // }
        
#if PLATFORM_WINDOWS
        Image.timing_.packetization_finish_ms = static_cast<int64>(FSystemTime::GetInMillSeconds(GetTimestampUs()));
        Image.timing_.encode_start_ms = static_cast<int64>(FSystemTime::GetInMillSeconds(BufferInfo.StartTimestampUs));
        Image.timing_.encode_finish_ms = static_cast<int64>(FSystemTime::GetInMillSeconds(BufferInfo.FinishTimestampUs));
#elif PLATFORM_LINUX
        Image.timing_.packetization_finish_ms = GetTotalMilliseconds(GetNowTicks());
        Image.timing_.encode_start_ms = GetTotalMilliseconds(BufferInfo.StartTimestampUs) ;
        Image.timing_.encode_finish_ms = GetTotalMilliseconds(BufferInfo.FinishTimestampUs);
#endif
        Image.timing_.flags = webrtc::VideoSendTiming::kTriggeredByTimer;

        Image.SetEncodedData(webrtc::EncodedImageBuffer::Create(const_cast<uint8_t *>(Buffer.data()), Buffer.size()));
        Image._encodedWidth = FAppConfig::Get().Width;
        Image._encodedHeight = FAppConfig::Get().Height;
        Image._frameType = BufferInfo.IsKeyFrame ? webrtc::VideoFrameType::kVideoFrameKey : webrtc::VideoFrameType::kVideoFrameDelta;
        Image.content_type_ = webrtc::VideoContentType::UNSPECIFIED;
        Image.qp_ = BufferInfo.VideoQP;
        Image.SetSpatialIndex(InLayerIndex);
        // Image._completeFrame = true;
        Image.rotation_ = webrtc::VideoRotation::kVideoRotation_0;
        Image.SetTimestamp(BufferInfo.TimestampRTP);
        Image.capture_time_ms_ = static_cast<int64>(BufferInfo.TimestampUs / 1000.0);

        webrtc::CodecSpecificInfo CodecInfo;
        if (bH264) {
            CodecInfo.codecType = webrtc::VideoCodecType::kVideoCodecH264;
            CodecInfo.codecSpecific.H264.packetization_mode = webrtc::H264PacketizationMode::NonInterleaved;
            CodecInfo.codecSpecific.H264.temporal_idx = webrtc::kNoTemporalIdx;
            CodecInfo.codecSpecific.H264.idr_frame = BufferInfo.IsKeyFrame;
            CodecInfo.codecSpecific.H264.base_layer_sync = false;
        } else {
            CodecInfo.codecType = webrtc::VideoCodecType::kVideoCodecH265;
            CodecInfo.codecSpecific.H265.packetization_mode = webrtc::H265PacketizationMode::NonInterleaved;
            CodecInfo.codecSpecific.H265.idr_frame = BufferInfo.IsKeyFrame;
        }
        Factory->OnEncodedImage(SourceEncoderId, Image, &CodecInfo);
        ML_LOG(EncoderFactory, Verbose, "Called OnEncodedPacket");
    }
} // namespace OpenZI::CloudRender