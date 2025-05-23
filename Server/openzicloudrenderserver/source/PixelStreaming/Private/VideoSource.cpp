// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/31 14:38
// 
#include "VideoSource.h"
#include "TextureSource.h"
#include "IPixelStreamingSessions.h"
#include "Containers/IntPoint.h"
#include "FrameBuffer.h"
// #include "Stats.h"
#include "PixelStreamingStatNames.h"
#include "Logger/CommandLogger.h"

namespace OpenZI::CloudRender
{
    FVideoSourceBase::FVideoSourceBase(FPixelStreamingPlayerId InPlayerId)
        : PlayerId(InPlayerId)
    {
    }

    void FVideoSourceBase::AddRef() const
    {
        RefCount.IncRef();
    }

    rtc::RefCountReleaseStatus FVideoSourceBase::Release() const
    {
        const rtc::RefCountReleaseStatus Status = RefCount.DecRef();
        if (Status == rtc::RefCountReleaseStatus::kDroppedLastRef)
        {
            FFixedFPSPump::Get()->UnregisterVideoSource(PlayerId);
            delete this;
        }
        return Status;
    }

    void FVideoSourceBase::Initialize()
    {
        CurrentState = webrtc::MediaSourceInterface::SourceState::kInitializing;
        // ML_LOG(VideoSource, Log, "RegisterVideoSource");
        FFixedFPSPump::Get()->RegisterVideoSource(PlayerId, this);
    }

    FVideoSourceBase::~FVideoSourceBase()
    {
        CurrentState = webrtc::MediaSourceInterface::SourceState::kEnded;
    }

    void FVideoSourceBase::OnPump(int32 FrameId)
    {

        // `OnPump` prepares a frame to be passed to WebRTC's `OnFrame`.
        // we dont call `OnFrame` outside this method so we can call AdaptCaptureFrame and possibly
        // drop a frame if requested.

        CurrentState = webrtc::MediaSourceInterface::SourceState::kLive;

        webrtc::VideoFrame Frame = CreateFrame(FrameId);

        // might want to allow source frames to be scaled here?
        const FIntPoint Resolution(Frame.width(), Frame.height());
        if (!AdaptCaptureFrame(Frame.timestamp_us(), Resolution))
        {
            return;
        }

        OnFrame(Frame);
    }

    bool FVideoSourceBase::AdaptCaptureFrame(const int64 TimestampUs, FIntPoint Resolution)
    {
        int outWidth, outHeight, cropWidth, cropHeight, cropX, cropY;
        // FIXME(OpenZILab): 这里返回的是false，会导致后续的OnFrame未执行
        if (!AdaptFrame(Resolution.X, Resolution.Y, TimestampUs, &outWidth, &outHeight, &cropWidth, &cropHeight, &cropX, &cropY))
        {
            return false;
        }

        return true;
    }

    /*
     * ------------------ FPlayerVideSource ------------------
     */

    FVideoSourceP2P::FVideoSourceP2P(FPixelStreamingPlayerId InPlayerId, IPixelStreamingSessions *InSessions)
        : FVideoSourceBase(InPlayerId), Sessions(InSessions)
    {
        // if (Settings::IsCodecVPX())
        // {
        //     TextureSource = MakeShared<FTextureSourceBackBufferToCPU>(1.0);
        // }
        // else
        // {
        // ML_LOG(VideoSource, Warning, "Construct VideoSourceP2P");
        TextureSource = std::make_shared<FTextureSourceBackBuffer>(1.0f);
        // }

        // We store the codec during construction as querying it everytime seem overly wasteful
        // especially considering we don't actually support switching codec mid-stream.
        // Codec = Settings::GetSelectedCodec();
    }

    bool FVideoSourceP2P::IsReadyForPump() const
    {
        return TextureSource && TextureSource->IsAvailable();
    }

    webrtc::VideoFrame FVideoSourceP2P::CreateFrameH264(int32 FrameId)
    {
        bool bQualityController = Sessions->IsQualityController(PlayerId);
        TextureSource->SetEnabled(bQualityController);
        const int64 TimestampUs = rtc::TimeMicros();

        // Based on quality control we either send a frame buffer with a texture or an empty frame
        // buffer to indicate that this frame should be skipped by the encoder as this peer is having its
        // frames transmitted by some other peer (the quality controlling peer).
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> FrameBuffer;

        if (bQualityController)
        {
            FrameBuffer = new rtc::RefCountedObject<FLayerFrameBuffer>(TextureSource);
        }
        else
        {
            FrameBuffer = new rtc::RefCountedObject<FInitializeFrameBuffer>(TextureSource);
        }

        webrtc::VideoFrame Frame = webrtc::VideoFrame::Builder()
                                       .set_video_frame_buffer(FrameBuffer)
                                       .set_timestamp_us(TimestampUs)
                                       .set_rotation(webrtc::VideoRotation::kVideoRotation_0)
                                       .set_id(FrameId)
                                       .build();

        return Frame;
    }

    webrtc::VideoFrame FVideoSourceP2P::CreateFrame(int32 FrameId)
    {
        // switch (Codec)
        // {
        // case Settings::ECodec::VP8:
        // case Settings::ECodec::VP9:
        //     return CreateFrameVPX(FrameId);
        // case Settings::ECodec::H264:
        // default:
            return CreateFrameH264(FrameId);
        // }
    }

    /*
     * ------------------ FVideoSourceSFU ------------------
     */

    FVideoSourceSFU::FVideoSourceSFU()
        : FVideoSourceBase(SFU_PLAYER_ID)
    {
        // TODO: !!!这里需要翻译，先注释为了编译
        // Make a copy of simulcast settings and sort them based on scaling.
        // using FLayer = Settings::FSimulcastParameters::FLayer;

        // TArray<FLayer *> SortedLayers;
        // for (FLayer &Layer : Settings::SimulcastParameters.Layers)
        // {
        //     SortedLayers.Add(&Layer);
        // }
        // SortedLayers.Sort([](const FLayer &LayerA, const FLayer &LayerB)
        //                   { return LayerA.Scaling > LayerB.Scaling; });

        // for (FLayer *SimulcastLayer : SortedLayers)
        // {
        //     const float Scale = 1.0f / SimulcastLayer->Scaling;
        //     TSharedPtr<FTextureSourceBackBuffer> TextureSource = MakeShared<FTextureSourceBackBuffer>(Scale);
        //     TextureSource->SetEnabled(true);
        //     LayerTextures.Add(TextureSource);
        // }
    }

    bool FVideoSourceSFU::IsReadyForPump() const
    {
        // Check all texture sources are ready.
        int NumReady = 0;

        for (int i = 0; i < LayerTextures.size(); i++)
        {
            bool bIsReady = LayerTextures[i] && LayerTextures[i]->IsAvailable();
            NumReady += bIsReady ? 1 : 0;
        }

        return NumReady == LayerTextures.size();
    }

    webrtc::VideoFrame FVideoSourceSFU::CreateFrame(int32 FrameId)
    {
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> FrameBuffer = rtc::scoped_refptr<webrtc::VideoFrameBuffer>(new FSimulcastFrameBuffer(LayerTextures));
        const int64 TimestampUs = rtc::TimeMicros();

        webrtc::VideoFrame Frame = webrtc::VideoFrame::Builder()
                                       .set_video_frame_buffer(FrameBuffer)
                                       .set_timestamp_us(TimestampUs)
                                       .set_rotation(webrtc::VideoRotation::kVideoRotation_0)
                                       .set_id(FrameId)
                                       .build();

        return Frame;
    }
} // namespace OpenZI::CloudRender