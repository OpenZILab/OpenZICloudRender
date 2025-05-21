// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 11:27
// 
#include "EncoderFactory.h"
#include "VideoEncoderRTC.h"
#include "Config.h"
#include "absl/strings/match.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "VideoEncoderH264Wrapper.h"
#include "SimulcastEncoderAdapter.h"
#include "Thread/CriticalSection.h"
#include "VideoEncoderNvEnc.h"
#include "Logger/CommandLogger.h"

namespace
{
    using namespace OpenZI;
    uint64 HashResolution(uint32 Width, uint32 Height)
    {
        return static_cast<uint64>(Width) << 32 | static_cast<uint64>(Height);
    }
} // namespace

namespace OpenZI::CloudRender
{
    /*
     * ------------- FVideoEncoderFactory ---------------
     */

    FVideoEncoderFactory::FVideoEncoderFactory()
    {
    }

    FVideoEncoderFactory::~FVideoEncoderFactory()
    {
    }

    webrtc::SdpVideoFormat FVideoEncoderFactory::CreateH264Format(webrtc::H264Profile profile, webrtc::H264Level level)
    {
        const absl::optional<std::string> ProfileString =
            webrtc::H264ProfileLevelIdToString(webrtc::H264ProfileLevelId(profile, level));
        // check(ProfileString);
        return webrtc::SdpVideoFormat(
            cricket::kH264CodecName,
            {{cricket::kH264FmtpProfileLevelId, *ProfileString},
             {cricket::kH264FmtpLevelAsymmetryAllowed, "1"},
             {cricket::kH264FmtpPacketizationMode, "1"}});
    }

    std::vector<webrtc::SdpVideoFormat> FVideoEncoderFactory::GetSupportedFormats() const
    {
        std::vector<webrtc::SdpVideoFormat> video_formats;
        auto EncoderCodec = FAppConfig::Get().EncoderCodec;
        if (EncoderCodec == "VP8")
        {
            video_formats.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
        }
        else if (EncoderCodec == "VP9")
        {
            video_formats.push_back(webrtc::SdpVideoFormat(cricket::kVp9CodecName));
        }
        else if (EncoderCodec == "H265" || EncoderCodec == "h265")
        {
            video_formats.push_back(webrtc::SdpVideoFormat(cricket::kH265CodecName));
        }
        else if (EncoderCodec == "H264" || EncoderCodec == "h264")
        {
            video_formats.push_back(FVideoEncoderFactory::CreateH264Format(webrtc::H264Profile::kProfileConstrainedBaseline, webrtc::H264Level::kLevel3_1));
        }
        return video_formats;
    }

    std::unique_ptr<webrtc::VideoEncoder> FVideoEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format)
    {
        ML_LOG(EncoderFactory, Log, "CreateVideoEncoder, format.name=%s", format.name.c_str());
        if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
        {
            return webrtc::VP8Encoder::Create();
        }
        else if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
        {
            return webrtc::VP9Encoder::Create();
        }
        else
        {
            // Lock during encoder creation
            FScopeLock Lock(&ActiveEncodersGuard);
            // ML_LOG(EncoderFactory, Log, "CreateVideoEncoder, Create VideoEncoderRTC");
            auto VideoEncoder = std::make_unique<FVideoEncoderRTC>(*this);
            ActiveEncoders.push_back(VideoEncoder.get());
            return VideoEncoder;
        }
    }

    void FVideoEncoderFactory::OnEncodedImage(FVideoEncoderFactory::FHardwareEncoderId SourceEncoderId, const webrtc::EncodedImage &encoded_image, const webrtc::CodecSpecificInfo *codec_specific_info)
    {
        // 工厂OnEncodedImage调用VideoEncoderRTC::SendEncodedImage
        // Lock as we send encoded image to each encoder.
        FScopeLock Lock(&ActiveEncodersGuard);

        // Go through each encoder and send our encoded image to its callback
        for (const auto& Encoder : ActiveEncoders)
        {
            Encoder->SendEncodedImage(SourceEncoderId, encoded_image, codec_specific_info);
        }
    }

    void FVideoEncoderFactory::ReleaseVideoEncoder(FVideoEncoderRTC* Encoder)
    {
        // Lock during deleting an encoder
        FScopeLock Lock(&ActiveEncodersGuard);
        auto It = std::find(ActiveEncoders.begin(), ActiveEncoders.end(), Encoder);
        if (It != ActiveEncoders.end())
            ActiveEncoders.erase(It);
    }

    void FVideoEncoderFactory::ForceKeyFrame(bool bLock)
    {
        if (bLock) {
            FScopeLock Lock(&ActiveEncodersGuard);
            // Go through each encoder and send our encoded image to its callback
            for (auto &KeyAndEncoder : HardwareEncoders)
            {
                KeyAndEncoder.second->SetForceNextKeyframe();
            }
        } else {
            for (auto &KeyAndEncoder : HardwareEncoders)
            {
                KeyAndEncoder.second->SetForceNextKeyframe();
            }
        }
    }

    std::optional<FVideoEncoderFactory::FHardwareEncoderId> FVideoEncoderFactory::GetOrCreateHardwareEncoder(int Width, int Height, int MaxBitrate, int TargetBitrate, int MaxFramerate)
    {
        FVideoEncoderFactory::FHardwareEncoderId EncoderId = HashResolution(Width, Height);
        std::shared_ptr<FVideoEncoderH264Wrapper> Existing = GetHardwareEncoder(EncoderId);

        std::optional<FVideoEncoderFactory::FHardwareEncoderId> Result = EncoderId;

        if (!Existing)
        {
            // Make the encoder config
            // FVideoEncoder::FLayerConfig EncoderConfig;
            // EncoderConfig.Width = Width;
            // EncoderConfig.Height = Height;
            // EncoderConfig.MaxFramerate = MaxFramerate;
            // EncoderConfig.TargetBitrate = TargetBitrate;
            // EncoderConfig.MaxBitrate = MaxBitrate;

            // Make the actual AVEncoder encoder.
            // TODO: 在这里绑定了编码器编码完成的回调，出发H264Wrapper的OnEncodedPacket
            ML_LOG(EncoderFactory, Log, "GetHardwareEncoder %ld is not existing", EncoderId);
            std::unique_ptr<FVideoEncoder> Encoder = std::make_unique<FVideoEncoderNvEnc>();
            if (Encoder)
            {
                Encoder->SetOnEncodedPacket([EncoderId, this](uint32 InLayerIndex, std::vector<uint8>& InBuffer, FBufferInfo InBufferInfo)
                                            {
				// Note: this is a static method call.
                FVideoEncoderH264Wrapper::OnEncodedPacket(EncoderId, this, InLayerIndex, InBuffer, InBufferInfo); });

                // Make the hardware encoder wrapper
                auto EncoderWrapper = std::make_shared<FVideoEncoderH264Wrapper>(EncoderId, std::move(Encoder));
                {
                    FScopeLock Lock(&HardwareEncodersGuard);
                    HardwareEncoders[EncoderId] = EncoderWrapper;
                }

                ML_LOG(EncoderFactory, Verbose, "After SetOnEncodedPacket");
                return Result;
            }
            else
            {
                ML_LOG(LogPixelStreaming, Error, "Could not create encoder. Check encoder config or perhaps you used up all your HW encoders.");
                // We could not make the encoder, so indicate the id was not set successfully.
                Result.reset();
                return Result;
            }
        }
        else
        {
            return Result;
        }
    }

    std::shared_ptr<FVideoEncoderH264Wrapper> FVideoEncoderFactory::GetHardwareEncoder(FVideoEncoderFactory::FHardwareEncoderId Id)
    {
        FScopeLock Lock(&HardwareEncodersGuard);
        // ML_LOG(EncoderFactory, Log, "GetHardwareEncoder");
        // for (const auto& HardwareEncoder : HardwareEncoders) {
            // ML_LOG(EncoderFactory, Log, "Existed HardwareEncoder Id = %ld", HardwareEncoder.first);
        // }
        auto &&Existing = HardwareEncoders.find(Id);
        if (Existing != HardwareEncoders.end())
        {
            return Existing->second;
        }
        return nullptr;
    }

    FSimulcastEncoderFactory::FSimulcastEncoderFactory()
        : RealFactory(new FVideoEncoderFactory())
    {
    }

    FSimulcastEncoderFactory::~FSimulcastEncoderFactory()
    {
    }

    std::vector<webrtc::SdpVideoFormat> FSimulcastEncoderFactory::GetSupportedFormats() const
    {
        return RealFactory->GetSupportedFormats();
    }

    std::unique_ptr<webrtc::VideoEncoder> FSimulcastEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format)
    {
        return std::make_unique<FSimulcastEncoderAdapter>(RealFactory.get(), format);
    }

    FVideoEncoderFactory *FSimulcastEncoderFactory::GetRealFactory() const
    {
        return RealFactory.get();
    }

} // namespace OpenZI::CloudRender