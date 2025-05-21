// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 11:43
// 
#include "VideoEncoderRTC.h"
#include "EncoderFactory.h"
#include "FrameBuffer.h"
#include "PlayerSession.h"
#include "PixelStreamingPlayerId.h"
#include "Logger/CommandLogger.h"
#include "VideoEncoderH264Wrapper.h"
#include "MessageCenter/MessageCenter.h"
#include "Config.h"


namespace
{
    using namespace OpenZI::CloudRender;
    // Begin utility functions etc.
    std::map<std::string, FVideoEncoder::ERateControlMode> const RateControlCVarMap{
        {"ConstQP", FVideoEncoder::ERateControlMode::CONSTQP},
        {"VBR", FVideoEncoder::ERateControlMode::VBR},
        {"CBR", FVideoEncoder::ERateControlMode::CBR},
    };

    std::map<std::string, FVideoEncoder::EMultipassMode> const MultipassCVarMap{
        {"DISABLED", FVideoEncoder::EMultipassMode::DISABLED},
        {"QUARTER", FVideoEncoder::EMultipassMode::QUARTER},
        {"FULL", FVideoEncoder::EMultipassMode::FULL},
    };

    std::map<std::string, FVideoEncoder::EH264Profile> const H264ProfileMap{
        {"AUTO", FVideoEncoder::EH264Profile::AUTO},
        {"BASELINE", FVideoEncoder::EH264Profile::BASELINE},
        {"MAIN", FVideoEncoder::EH264Profile::MAIN},
        {"HIGH", FVideoEncoder::EH264Profile::HIGH},
        {"HIGH444", FVideoEncoder::EH264Profile::HIGH444},
        {"STEREO", FVideoEncoder::EH264Profile::STEREO},
        {"SVC_TEMPORAL_SCALABILITY", FVideoEncoder::EH264Profile::SVC_TEMPORAL_SCALABILITY},
        {"PROGRESSIVE_HIGH", FVideoEncoder::EH264Profile::PROGRESSIVE_HIGH},
        {"CONSTRAINED_HIGH", FVideoEncoder::EH264Profile::CONSTRAINED_HIGH},
    };
    std::map<std::string, FVideoEncoder::EH265Profile> const H265ProfileMap{
        {"AUTO", FVideoEncoder::EH265Profile::AUTO},
        {"MAIN", FVideoEncoder::EH265Profile::MAIN},
        {"MAIN10", FVideoEncoder::EH265Profile::MAIN10},
    };

    FVideoEncoder::ERateControlMode GetRateControlCVar()
    {
        const std::string EncoderRateControl = FAppConfig::Get().EncoderRateControl;
        auto const Iter = RateControlCVarMap.find(EncoderRateControl);
        if (Iter == std::end(RateControlCVarMap))
            return FVideoEncoder::ERateControlMode::CBR;
        return Iter->second;
    }

    FVideoEncoder::EMultipassMode GetMultipassCVar()
    {
        const std::string EncoderMultipass = FAppConfig::Get().EncoderMultipass;
        auto const Iter = MultipassCVarMap.find(EncoderMultipass);
        if (Iter == std::end(MultipassCVarMap))
            return FVideoEncoder::EMultipassMode::FULL;
        return Iter->second;
    }

    FVideoEncoder::EH264Profile GetH264Profile()
    {
        const std::string H264Profile = FAppConfig::Get().H264Profile;
        auto const Iter = H264ProfileMap.find(H264Profile);
        if (Iter == std::end(H264ProfileMap))
            return FVideoEncoder::EH264Profile::BASELINE;
        return Iter->second;
    }

    FVideoEncoder::EH265Profile GetH265Profile()
    {
        const std::string H265Profile = FAppConfig::Get().H265Profile;
        auto const Iter = H265ProfileMap.find(H265Profile);
        if (Iter == std::end(H265ProfileMap))
            return FVideoEncoder::EH265Profile::AUTO;
        return Iter->second;
    }
}

namespace OpenZI::CloudRender
{
    FVideoEncoderRTC::FVideoEncoderRTC(FVideoEncoderFactory &InFactory)
        : Factory(InFactory)
    {
    }

    FVideoEncoderRTC::~FVideoEncoderRTC()
    {
        Factory.ReleaseVideoEncoder(this);
    }

    int FVideoEncoderRTC::InitEncode(webrtc::VideoCodec const *codec_settings, VideoEncoder::Settings const &settings)
    {
        // checkf(FVideoEncoderFactory::Get().IsSetup(), TEXT("FVideoEncoderFactory not setup"));

        // Try to create an encoder if it doesnt already.
        std::optional<FVideoEncoderFactory::FHardwareEncoderId> EncoderIdOptional = Factory.GetOrCreateHardwareEncoder(codec_settings->width, codec_settings->height, codec_settings->maxBitrate, codec_settings->startBitrate, codec_settings->maxFramerate);

        if (EncoderIdOptional.has_value())
        {
            HardwareEncoderId = EncoderIdOptional.value();
            // UpdateConfig();
            WebRtcProposedTargetBitrate = codec_settings->startBitrate;
            return WEBRTC_VIDEO_CODEC_OK;
        }
        else
        {
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
    }

    int32 FVideoEncoderRTC::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback *callback)
    {
        OnEncodedImageCallback = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32 FVideoEncoderRTC::Release()
    {
        OnEncodedImageCallback = nullptr;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32 FVideoEncoderRTC::Encode(webrtc::VideoFrame const &frame, std::vector<webrtc::VideoFrameType> const *frame_types)
    {
        // WebRTC中，当video_stream_encoder.cc Call EncodeVideoFrame，且该函数中的Encode函数被调用时，该函数才会触发
        FFrameBuffer *FrameBuffer = static_cast<FFrameBuffer *>(frame.video_frame_buffer().get());

        // If initialize frame is passed, this is a dummy frame so WebRTC is happy frames are passing through
        // This is used so that an encoder can be active as far as WebRTC is concerned by simply have frames transmitted by some other encoder on its behalf.
        if (FrameBuffer->GetFrameBufferType() == FFrameBufferType::Initialize)
        {
            return WEBRTC_VIDEO_CODEC_OK;
        }

        bool bKeyframe = false;
        if ((frame_types && (*frame_types)[0] == webrtc::VideoFrameType::kVideoFrameKey))
        {
            bKeyframe = true;
        }

        std::shared_ptr<FVideoEncoderH264Wrapper> Encoder = Factory.GetHardwareEncoder(HardwareEncoderId);
        if (!Encoder)
        {
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        // UpdateConfig();

        // ML_LOG(Temp, VeryVerbose, "FVideoEncoderRTC::Encode");
        Encoder->Encode(frame, bKeyframe);

        return WEBRTC_VIDEO_CODEC_OK;
    }

    // Pass rate control parameters from WebRTC to our encoder
    // This is how WebRTC can control the bitrate/framerate of the encoder.
    void FVideoEncoderRTC::SetRates(RateControlParameters const &parameters)
    {
        PendingRateChange.emplace(parameters);
    }

    webrtc::VideoEncoder::EncoderInfo FVideoEncoderRTC::GetEncoderInfo() const
    {
        VideoEncoder::EncoderInfo info;
        info.supports_native_handle = true;
        info.is_hardware_accelerated = true;
        // info.has_internal_source = false;
        info.supports_simulcast = false;
        info.implementation_name = "PIXEL_STREAMING_HW_ENCODER_D3D11";

        const int LowQP = FAppConfig::Get().WebRTCLowQpThreshold;
        const int HighQP = FAppConfig::Get().WebRTCHighQpThreshold;
        info.scaling_settings = VideoEncoder::ScalingSettings(LowQP, HighQP);

        // if true means HW encoder must be perfect and drop frames itself etc
        info.has_trusted_rate_controller = false;

        return info;
    }

    void FVideoEncoderRTC::UpdateConfig()
    {
        // if (FVideoEncoderH264Wrapper* Encoder = Factory.GetHardwareEncoder(HardwareEncoderId)) {
        //     // FVideoEncoder::FLayerConfig NewConfig = Encoder->GetCurrentConfig();

        //     if (PendingRateChange.has_value()) {
        //         const RateControlParameters& RateChangeParams = PendingRateChange.value();

        //         // NewConfig.MaxFramerate = RateChangeParams.framerate_fps;

        //         // We store what WebRTC wants as the bitrate, even if we are overriding it, so we
        //         // can restore back to it when user stops using CVar.
        //         WebRtcProposedTargetBitrate = RateChangeParams.bitrate.get_sum_kbps() * 1000;

        //         // Clear the rate change request
        //         PendingRateChange.reset();
        //     }

        //     // TODO how to handle res changes?

        //     // NewConfig = CreateEncoderConfigFromCVars(NewConfig);

        //     // Encoder->SetConfig(NewConfig);
        // }
    }

    FVideoEncoder::FLayerConfig FVideoEncoderRTC::CreateEncoderConfigFromCVars(FVideoEncoder::FLayerConfig InEncoderConfig) const
    {
        // Change encoder settings through CVars
        const int32 MaxBitrateCVar = FAppConfig::Get().EncoderMaxBitrate;
        const int32 TargetBitrateCVar = FAppConfig::Get().EncoderTargetBitrate;
        const int32 MinQPCVar = FAppConfig::Get().EncoderMinQP;
        const int32 MaxQPCVar = FAppConfig::Get().EncoderMaxQP;
        const FVideoEncoder::ERateControlMode RateControlCVar = GetRateControlCVar();
        const FVideoEncoder::EMultipassMode MultiPassCVar = GetMultipassCVar();
        const bool bFillerDataCVar = FAppConfig::Get().EnableFillerData;
        const FVideoEncoder::EH264Profile H264Profile = GetH264Profile();

        InEncoderConfig.MaxBitrate = MaxBitrateCVar > -1 ? MaxBitrateCVar : InEncoderConfig.MaxBitrate;
        InEncoderConfig.TargetBitrate = TargetBitrateCVar > -1 ? TargetBitrateCVar : WebRtcProposedTargetBitrate;
        InEncoderConfig.QPMin = MinQPCVar;
        InEncoderConfig.QPMax = MaxQPCVar;
        InEncoderConfig.RateControlMode = RateControlCVar;
        InEncoderConfig.MultipassMode = MultiPassCVar;
        InEncoderConfig.FillData = bFillerDataCVar;
        InEncoderConfig.H264Profile = H264Profile;

        return InEncoderConfig;
    }

    void FVideoEncoderRTC::SendEncodedImage(uint64 SourceEncoderId, webrtc::EncodedImage const &encoded_image, webrtc::CodecSpecificInfo const *codec_specific_info)
    {
        // 该函数很关键，只有调用该函数，编码后的图像才能被传输给Web端
        if (SourceEncoderId != HardwareEncoderId)
        {
            return;
        }

        if (FirstKeyframeCountdown > 0)
        {
            // ideally we want to make the first frame of new peers a keyframe but we
            // dont know when webrtc will decide to start sending out frames. this is
            // the next best option. its a count beause when it was the very next frame
            // it still didnt seem to work. delaying it a few frames seems to have worked.
            --FirstKeyframeCountdown;
            if (FirstKeyframeCountdown == 0)
            {
#if PLATFORM_WINDOWS
                Factory.ForceKeyFrame(false);
#elif PLATFORM_LINUX
                // Linux下，因为该函数中也加了锁，所以死锁了
                Factory.ForceKeyFrame(false);
#endif
            }
        }

        if (OnEncodedImageCallback)
        {
            // ML_LOG(Temp, VeryVerbose, "RTC::SendEncodedImage OnEncodedImage");
            OnEncodedImageCallback->OnEncodedImage(encoded_image, codec_specific_info);
        }
    }
} // namespace OpenZI::CloudRender