//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/02 18:29
//
#include "Streamer.h"
// #include "AudioCapturer.h"
#include "PlayerSession.h"
// #include "EncoderFactory.h"
#include "AudioSink.h"
#include "AudioDeviceModule.h"
#include "Config.h"
#include "EncoderFactory.h"
#include "Logger/CommandLogger.h"
#include "MessageCenter.h"
#include "SessionDescriptionObservers.h"
#include "VideoSource.h"
// #include "VideoDecoderFactory.h"

namespace OpenZI::CloudRender {
    bool FStreamer::IsPlatformCompatible() {
        // return
        // AVEncoder::FVideoEncoderFactory::Get().HasEncoderForCodec(AVEncoder::ECodecType::H264);
        return true;
    }

    FStreamer::FStreamer()
        : FStreamer(FAppConfig::Get().SignallingServerUrl, FAppConfig::Get().StreamerId) {}
    FStreamer::FStreamer(const std::string& InSignallingServerUrl, const std::string& InStreamerId)
        : SignallingServerUrl(InSignallingServerUrl), StreamerId(InStreamerId),
        //   WebRtcSignallingThread(std::make_unique<rtc::Thread>(rtc::SocketServer::CreateDefault())),
          WebRtcSignallingThread(rtc::Thread::CreateWithSocketServer()),
          SignallingServerConnection(
              std::make_unique<FSignallingServerConnection>(*this, InStreamerId)),
          PlayerSessions(WebRtcSignallingThread.get()) {

        // TODO: 开启WebRtcSignallingThread前确保AVEncoder模块和WebSockets模块已加载
        // FModuleManager::LoadModuleChecked<IModuleInterface>(std::string("AVEncoder"));

        // // required for communication with Signalling Server and must be called in the game
        // thread, while it's used in signalling thread
        // FModuleManager::LoadModuleChecked<FWebSocketsModule>("WebSockets");
        // ML_LOG(Streamer, Log, "Construct FStreamer, SignallingServerUrl=%s",
        //        SignallingServerUrl.c_str());

        StartWebRtcSignallingThread();
        ConnectToSignallingServer();
        // if (UPixelStreamingDelegates *Delegates =
        // UPixelStreamingDelegates::GetPixelStreamingDelegates())
        // {
        //     Delegates->OnClosedConnectionNative.AddRaw(this, &FStreamer::PostPlayerDeleted);
        //     Delegates->OnQualityControllerChangedNative.AddRaw(this,
        //     &FStreamer::OnQualityControllerChanged);
        // }
        SUBSCRIBE_MESSAGE_TwoParams(OnClosedConnectionNative, this, &FStreamer::PostPlayerDeleted,
                                    FPixelStreamingPlayerId, PlayerId, bool, WasQualityController);
        SUBSCRIBE_MESSAGE_OneParam(OnQualityControllerChangedNative, this,
                                   &FStreamer::OnQualityControllerChanged, FPixelStreamingPlayerId,
                                   PlayerId);
        SUBSCRIBE_MESSAGE_OneParam(OnSendVideo, this, &FStreamer::SendVideo, const std::string&, VideoBuffer);
    }

    void FStreamer::SendVideo(const std::string& VideoContent) {
        SendPlayerMessage(EToPlayerMsg::VideoContent, VideoContent);
    }

    FStreamer::~FStreamer() {
        UNSUBSCRIBE_MESSAGE(OnClosedConnectionNative);
        UNSUBSCRIBE_MESSAGE(OnQualityControllerChangedNative);
        UNSUBSCRIBE_MESSAGE(OnDataChannelOpenNative);
        DeleteAllPlayerSessions();
        P2PPeerConnectionFactory = nullptr;
        SFUPeerConnectionFactory = nullptr;
        WebRtcSignallingThread->Stop();
        rtc::CleanupSSL();
    }

    rtc::scoped_refptr<webrtc::AudioDeviceModule> FStreamer::CreateAudioDeviceModule() const
    {
        bool bUseLegacyAudioDeviceModule = FAppConfig::Get().WebRTCUseLegacyAudioDevice;
        rtc::scoped_refptr<webrtc::AudioDeviceModule> AudioDeviceModule;
        if (bUseLegacyAudioDeviceModule)
        {
            // AudioDeviceModule = new rtc::RefCountedObject<FAudioCapturer>();
        }
        else
        {
            AudioDeviceModule = new rtc::RefCountedObject<FAudioDeviceModule>();
        }
        return AudioDeviceModule;
    }

    void FStreamer::StartWebRtcSignallingThread() {
        // Initialisation of WebRTC. Note: That interfacing with WebRTC should generally happen in
        // WebRTC signalling thread.

        // Create our own WebRTC thread for signalling
        WebRtcSignallingThread->SetName("WebRtcSignallingThread", this);
        WebRtcSignallingThread->Start();
        rtc::InitializeSSL();

        PeerConnectionConfig = {};

        /* ---------- P2P Peer Connection Factory ---------- */
        // ML_LOG(Streamer, Log,
        //        "StartWebRtcSignallingThread, Create P2PFactory, VideoEncoderFactory");
        std::unique_ptr<FVideoEncoderFactory> P2PPeerConnectionFactoryPtr =
            std::make_unique<FVideoEncoderFactory>();
        P2PVideoEncoderFactory = P2PPeerConnectionFactoryPtr.get();
        P2PPeerConnectionFactory = webrtc::CreatePeerConnectionFactory(
            nullptr,                      // network_thread
            nullptr,                      // worker_thread
            WebRtcSignallingThread.get(), // signal_thread
            CreateAudioDeviceModule(),    // audio device module
            webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderOpus>(),
            webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderOpus>(),
            std::move(P2PPeerConnectionFactoryPtr),
            // std::make_unique<webrtc::InternalDecoderFactory>(),
            nullptr,
            // std::make_unique<FVideoDecoderFactory>(),
            // nullptr,
            nullptr,                     // audio_mixer
            SetupAudioProcessingModule() // audio_processing
        );

        assert(P2PPeerConnectionFactory.get() != nullptr);
        /* ---------- SFU Peer Connection Factory ---------- */

        // std::unique_ptr<FSimulcastEncoderFactory> SFUPeerConnectionFactoryPtr =
        //     std::make_unique<FSimulcastEncoderFactory>();
        // SFUVideoEncoderFactory = SFUPeerConnectionFactoryPtr.get();

        // SFUPeerConnectionFactory = webrtc::CreatePeerConnectionFactory(
        //     nullptr,                      // network_thread
        //     nullptr,                      // worker_thread
        //     WebRtcSignallingThread.get(), // signal_thread
        //     // CreateAudioDeviceModule(),    // audio device module
        //     // webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderOpus>(),
        //     // webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderOpus>(),
        //     nullptr, // 音频先不扩展，使用WebRTC默认配置
        //     webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderOpus>(),
        //     webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderOpus>(),
        //     std::move(SFUPeerConnectionFactoryPtr),
        //     // std::make_unique<webrtc::InternalEncoderFactory>(),
        //     std::make_unique<webrtc::InternalDecoderFactory>(),
        //     nullptr, // audio_mixer
        //     // SetupAudioProcessingModule()// audio_processing
        //     nullptr);
        SFUPeerConnectionFactory = nullptr;

        // check(SFUPeerConnectionFactory.get() != nullptr);
    }

    rtc::scoped_refptr<webrtc::AudioProcessing> FStreamer::SetupAudioProcessingModule() {
        rtc::scoped_refptr<webrtc::AudioProcessing> AudioProcessingModule = webrtc::AudioProcessingBuilder().Create();
        webrtc::AudioProcessing::Config Config;

        // Enabled multi channel audio capture/render
        Config.pipeline.multi_channel_capture = true;
        Config.pipeline.multi_channel_render = true;
        Config.pipeline.maximum_internal_processing_rate = 48000;

        // Turn off all other audio processing effects in UE's WebRTC. We want to stream audio from
        // UE as pure as possible.
        Config.pre_amplifier.enabled = false;
        Config.high_pass_filter.enabled = false;
        Config.echo_canceller.enabled = false;
        Config.noise_suppression.enabled = false;
        Config.transient_suppression.enabled = false;
        // Config.voice_detection.enabled = false;
        Config.gain_controller1.enabled = false;
        Config.gain_controller2.enabled = false;
        // Config.residual_echo_detector.enabled = false;
        // Config.level_estimation.enabled = false;

        // Apply the config.
        AudioProcessingModule->ApplyConfig(Config);

        return AudioProcessingModule;
    }

    void FStreamer::ConnectToSignallingServer() {
        SignallingServerConnection->Connect(SignallingServerUrl);
    }

    void FStreamer::OnQualityControllerChanged(FPixelStreamingPlayerId PlayerId) {}

    void FStreamer::PostPlayerDeleted(FPixelStreamingPlayerId PlayerId, bool WasQualityController) {
    }

    void FStreamer::OnConfig(const webrtc::PeerConnectionInterface::RTCConfiguration& Config) {
        PeerConnectionConfig = Config;
    }

    void FStreamer::OnDataChannelOpen(FPixelStreamingPlayerId PlayerId,
                                      webrtc::DataChannelInterface* DataChannel) {
        if (DataChannel) {
            // When data channel is open, try to send cached freeze frame (if we have one)
            SendCachedFreezeFrameTo(PlayerId);
        }
    }

    webrtc::PeerConnectionInterface* FStreamer::CreateSession(FPixelStreamingPlayerId PlayerId,
                                                              int Flags) {
        // PeerConnectionConfig.enable_simulcast_stats = true;

        bool bIsSFU = (Flags & EPlayerFlags::PSPFlag_IsSFU) != EPlayerFlags::PSPFlag_None;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PCFactory =
            bIsSFU ? SFUPeerConnectionFactory : P2PPeerConnectionFactory;

        webrtc::PeerConnectionInterface* PeerConnection = PlayerSessions.CreatePlayerSession(
            PlayerId, PCFactory, PeerConnectionConfig, SignallingServerConnection.get(), Flags);

        if (PeerConnection != nullptr) {
            if ((Flags & EPlayerFlags::PSPFlag_SupportsDataChannel) != EPlayerFlags::PSPFlag_None) {
                webrtc::DataChannelInit DataChannelConfig;
                DataChannelConfig.reliable = true;
                DataChannelConfig.ordered = true;

                rtc::scoped_refptr<webrtc::DataChannelInterface> DataChannel =
                    PeerConnection->CreateDataChannel("dataChannel", &DataChannelConfig);
                PlayerSessions.SetPlayerSessionDataChannel(PlayerId, DataChannel);

                // Bind to the on data channel open delegate (we use this as the earliest time peer
                // is ready for freeze frame)
                FDataChannelObserver* DataChannelObserver =
                    PlayerSessions.GetDataChannelObserver(PlayerId);
                // check(DataChannelObserver);
                //  if (UPixelStreamingDelegates *Delegates =
                //  UPixelStreamingDelegates::GetPixelStreamingDelegates())
                //  {
                //      Delegates->OnDataChannelOpenNative.AddRaw(this,
                //      &FStreamer::OnDataChannelOpen);
                //  }
                SUBSCRIBE_MESSAGE_TwoParams(OnDataChannelOpenNative, this,
                                            &FStreamer::OnDataChannelOpen, FPixelStreamingPlayerId,
                                            PlayerId, webrtc::DataChannelInterface*, DataChannel);
                DataChannelObserver->Register(DataChannel);
            }
        }

        return PeerConnection;
    }

    void FStreamer::OnSessionDescription(FPixelStreamingPlayerId PlayerId, webrtc::SdpType Type,
                                         const std::string& Sdp) {
        ML_LOG(Temp, Verbose, "FStreamer::OnSessionDescription, %d", Type);
        switch (Type) {
        case webrtc::SdpType::kOffer:
            OnOffer(PlayerId, Sdp);
            break;
        case webrtc::SdpType::kAnswer:
        case webrtc::SdpType::kPrAnswer:
            PlayerSessions.OnAnswer(PlayerId, Sdp);
            SetStreamingStarted(true);
            break;
        case webrtc::SdpType::kRollback:
            ML_LOG(LogPixelStreaming, Error, "Rollback SDP is currently unsupported. SDP is: %s",
                   Sdp.c_str());
            break;
        }
    }

    void FStreamer::OnOffer(FPixelStreamingPlayerId PlayerId, const std::string& Sdp) {
        webrtc::PeerConnectionInterface* PeerConnection =
            CreateSession(PlayerId, EPlayerFlags::PSPFlag_SupportsDataChannel);
        if (PeerConnection) {
            webrtc::SdpParseError Error;
            std::unique_ptr<webrtc::SessionDescriptionInterface> SessionDesc =
                webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, Sdp, &Error);
            if (!SessionDesc) {
                ML_LOG(LogPixelStreaming, Error, "Failed to parse offer's SDP: %s\n%s",
                       Error.description.c_str(), Sdp.c_str());
                return;
            }
            ML_LOG(Temp, Verbose, "FStreamer::OnOffer SendAnswer");
            SendAnswer(PlayerId, PeerConnection,
                       std::unique_ptr<webrtc::SessionDescriptionInterface>(SessionDesc.release()));
        } else {
            ML_LOG(LogPixelStreaming, Error,
                   "Failed to create player session, peer connection was nullptr.");
        }
    }

    void FStreamer::SetLocalDescription(webrtc::PeerConnectionInterface* PeerConnection,
                                        FSetSessionDescriptionObserver* Observer,
                                        webrtc::SessionDescriptionInterface* SDP) {
        // Note from Luke about WebRTC: the sink of video capturer will be added as a direct result
        // of `PeerConnection->SetLocalDescription()` call but video encoder will be created later
        // on when the first frame is pushed into the WebRTC pipeline (by the capturer calling
        // `OnFrame`).

        ML_LOG(Temp, Verbose, "FStreamer::SetLocalDescription");
        PeerConnection->SetLocalDescription(Observer, SDP);

        // Once local description has been set we can start setting some encoding information for
        // the video stream rtp sender
        for (rtc::scoped_refptr<webrtc::RtpSenderInterface> Sender : PeerConnection->GetSenders()) {
            cricket::MediaType MediaType = Sender->media_type();
            if (MediaType == cricket::MediaType::MEDIA_TYPE_VIDEO) {
                webrtc::RtpParameters ExistingParams = Sender->GetParameters();

                // Set the degradation preference based on our CVar for it.
                ExistingParams.degradation_preference =
                    GetDegradationPreference(FAppConfig::Get().DegradationPreference);

                webrtc::RTCError Err = Sender->SetParameters(ExistingParams);
                if (!Err.ok()) {
                    const char* ErrMsg = Err.message();
                    std::string ErrorStr(ErrMsg);
                    ML_LOG(LogPixelStreaming, Error, "Failed to set RTP Sender params: %s",
                           ErrorStr.c_str());
                }
            }
        }
    }

    void FStreamer::SendAnswer(FPixelStreamingPlayerId PlayerId,
                               webrtc::PeerConnectionInterface* PeerConnection,
                               std::unique_ptr<webrtc::SessionDescriptionInterface> Sdp) {
        // below is async execution (with error handling) of:
        //		PeerConnection.SetRemoteDescription(SDP);
        //		Answer = PeerConnection.CreateAnswer();
        //		PeerConnection.SetLocalDescription(Answer);
        //		SignallingServerConnection.SendAnswer(Answer);

        FSetSessionDescriptionObserver* SetLocalDescriptionObserver =
            FSetSessionDescriptionObserver::Create(
                [this, PlayerId, PeerConnection]() // on success
                {
                    GetSignallingServerConnection()->SendAnswer(
                        PlayerId, *PeerConnection->local_description());
                    SetStreamingStarted(true);
                },
                [this, PlayerId](const std::string& Error) // on failure
                {
                    ML_LOG(LogPixelStreaming, Error, "Failed to set local description: %s",
                           Error.c_str());
                    PlayerSessions.DisconnectPlayer(PlayerId, Error);
                });

        auto OnCreateAnswerSuccess = [this, PlayerId, PeerConnection, SetLocalDescriptionObserver](
                                         webrtc::SessionDescriptionInterface* SDP) {
            SetLocalDescription(PeerConnection, SetLocalDescriptionObserver, SDP);
        };

        FCreateSessionDescriptionObserver* CreateAnswerObserver =
            FCreateSessionDescriptionObserver::Create(
                std::move(OnCreateAnswerSuccess),
                [this, PlayerId](const std::string& Error) // on failure
                {
                    ML_LOG(LogPixelStreaming, Error, "Failed to create answer: %s", Error.c_str());
                    PlayerSessions.DisconnectPlayer(PlayerId, Error);
                });

        auto OnSetRemoteDescriptionSuccess = [this, PlayerId, PeerConnection,
                                              CreateAnswerObserver]() {
            // Note: these offer to receive are superseded now we are use transceivers to setup our
            // peer connection media
            int offer_to_receive_video =
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kUndefined;
            int offer_to_receive_audio =
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::kUndefined;
            bool voice_activity_detection = false;
            bool ice_restart = true;
            bool use_rtp_mux = true;

            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions AnswerOption{
                offer_to_receive_video, offer_to_receive_audio, voice_activity_detection,
                ice_restart, use_rtp_mux};
            ML_LOG(LogAddStream, Warning, "SendAnswer and add streams");
            AddStreams(PlayerId, PeerConnection, EPlayerFlags::PSPFlag_SupportsDataChannel);
            ModifyTransceivers(PeerConnection->GetTransceivers(),
                               EPlayerFlags::PSPFlag_SupportsDataChannel);

            PeerConnection->CreateAnswer(CreateAnswerObserver, AnswerOption);
        };

        FSetSessionDescriptionObserver* SetRemoteDescriptionObserver =
            FSetSessionDescriptionObserver::Create(
                std::move(OnSetRemoteDescriptionSuccess),
                [this, PlayerId](const std::string& Error) // on failure
                {
                    ML_LOG(LogPixelStreaming, Error, "Failed to set remote description: %s",
                           Error.c_str());
                    PlayerSessions.DisconnectPlayer(PlayerId, Error);
                });

        cricket::SessionDescription* RemoteDescription = Sdp->description();
        MungeRemoteSDP(RemoteDescription);

        PeerConnection->SetRemoteDescription(SetRemoteDescriptionObserver, Sdp.release());
    }

    void FStreamer::MungeRemoteSDP(cricket::SessionDescription* RemoteDescription) {
        // Munge SDP of remote description to inject min, max, start bitrates
        std::vector<cricket::ContentInfo>& ContentInfos = RemoteDescription->contents();
        for (cricket::ContentInfo& Content : ContentInfos) {
            cricket::MediaContentDescription* MediaDescription = Content.media_description();
            if (MediaDescription->type() == cricket::MediaType::MEDIA_TYPE_VIDEO) {
                cricket::VideoContentDescription* VideoDescription = MediaDescription->as_video();
                std::vector<cricket::VideoCodec> CodecsCopy = VideoDescription->codecs();
                for (cricket::VideoCodec& Codec : CodecsCopy) {
                    // Note: These params are passed as kilobits, so divide by 1000.
                    Codec.SetParam(cricket::kCodecParamMinBitrate,
                                   FAppConfig::Get().WebRTCMinBitrate / 1000);
                    Codec.SetParam(cricket::kCodecParamStartBitrate,
                                   FAppConfig::Get().WebRTCStartBitrate / 1000);
                    Codec.SetParam(cricket::kCodecParamMaxBitrate,
                                   FAppConfig::Get().WebRTCMaxBitrate / 1000);
                }
                VideoDescription->set_codecs(CodecsCopy);
            }
        }
    }

    void FStreamer::OnRemoteIceCandidate(FPixelStreamingPlayerId PlayerId,
                                         const std::string& SdpMid, int SdpMLineIndex,
                                         const std::string& Sdp) {
        PlayerSessions.OnRemoteIceCandidate(PlayerId, SdpMid, SdpMLineIndex, Sdp);
    }

    void FStreamer::OnPlayerConnected(FPixelStreamingPlayerId PlayerId, int Flags) {
        // create peer connection
        if (webrtc::PeerConnectionInterface* PeerConnection = CreateSession(PlayerId, Flags)) {
            ML_LOG(LogAddStream, Warning, "OnPlayConnected and add streams");
            AddStreams(PlayerId, PeerConnection, Flags);
            ModifyTransceivers(PeerConnection->GetTransceivers(), Flags);

            // observer for creating offer
            FCreateSessionDescriptionObserver* CreateOfferObserver =
                FCreateSessionDescriptionObserver::Create(
                    [this, PlayerId, PeerConnection](
                        webrtc::SessionDescriptionInterface* SDP) // on SDP create success
                    {
                        FSetSessionDescriptionObserver* SetLocalDescriptionObserver =
                            FSetSessionDescriptionObserver::Create(
                                [this, PlayerId, SDP]() // on SDP set success
                                { GetSignallingServerConnection()->SendOffer(PlayerId, *SDP); },
                                [](const std::string& Error) // on SDP set failure
                                {
                                    ML_LOG(LogPixelStreaming, Error,
                                           "Failed to set local description: %s", Error.c_str());
                                });

                        SetLocalDescription(PeerConnection, SetLocalDescriptionObserver, SDP);
                    },
                    [](const std::string& Error) // on SDP create failure
                    {
                        ML_LOG(LogPixelStreaming, Error, "Failed to create offer: %s",
                               Error.c_str());
                    });

            PeerConnection->CreateOffer(CreateOfferObserver, {});
        }
    }

    void FStreamer::OnPlayerDisconnected(FPixelStreamingPlayerId PlayerId) {
        ML_LOG(LogPixelStreaming, Log, "player %s disconnected", PlayerId.c_str());
        DeletePlayerSession(PlayerId);
    }

    void FStreamer::OnSignallingServerDisconnected() {
        DeleteAllPlayerSessions();

        // Call disconnect here makes sure any other websocket callbacks etc are cleared
        SignallingServerConnection->Disconnect();
        ConnectToSignallingServer();
    }

    int FStreamer::GetNumPlayers() const { return PlayerSessions.GetNumPlayers(); }

    void FStreamer::ForceKeyFrame() {
        if (P2PVideoEncoderFactory) {
            P2PVideoEncoderFactory->ForceKeyFrame();
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Cannot force a key frame - video encoder factory is nullptr.");
        }
    }

    void FStreamer::SetQualityController(FPixelStreamingPlayerId PlayerId) {

        PlayerSessions.SetQualityController(PlayerId);
    }

    bool FStreamer::IsQualityController(FPixelStreamingPlayerId PlayerId) const {
        return PlayerSessions.IsQualityController(PlayerId);
    }

    IPixelStreamingAudioSink* FStreamer::GetAudioSink(FPixelStreamingPlayerId PlayerId) const {
        return PlayerSessions.GetAudioSink(PlayerId);
    }

    IPixelStreamingAudioSink* FStreamer::GetUnlistenedAudioSink() const {
        return PlayerSessions.GetUnlistenedAudioSink();
    }

    bool FStreamer::Send(FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type,
                         const std::string& Descriptor) const {
        return PlayerSessions.Send(PlayerId, Type, Descriptor);
    }

    void FStreamer::SendLatestQP(FPixelStreamingPlayerId PlayerId, int LatestQP) const {
        PlayerSessions.SendLatestQP(PlayerId, LatestQP);
    }

    void FStreamer::PollWebRTCStats() const { PlayerSessions.PollWebRTCStats(); }

    void FStreamer::DeleteAllPlayerSessions() {
        PlayerSessions.DeleteAllPlayerSessions();
        bStreamingStarted = false;
    }

    void FStreamer::DeletePlayerSession(FPixelStreamingPlayerId PlayerId) {
        int NumRemainingPlayers = PlayerSessions.DeletePlayerSession(PlayerId);

        if (NumRemainingPlayers == 0) {
            bStreamingStarted = false;
        }
    }

    std::string FStreamer::GetAudioStreamID() const {
        bool bSyncVideoAndAudio = !FAppConfig::Get().WebRTCDisableAudioSync;
        return bSyncVideoAndAudio ? std::string("pixelstreaming_av_stream_id")
                                  : std::string("pixelstreaming_audio_stream_id");
    }

    std::string FStreamer::GetVideoStreamID() const {
        bool bSyncVideoAndAudio = !FAppConfig::Get().WebRTCDisableAudioSync;
        return bSyncVideoAndAudio ? std::string("pixelstreaming_av_stream_id")
                                  : std::string("pixelstreaming_video_stream_id");
    }

    void FStreamer::AddStreams(FPixelStreamingPlayerId PlayerId,
                               webrtc::PeerConnectionInterface* PeerConnection, int Flags) {
        // check(PeerConnection);

        // Use PeerConnection's transceiver API to add create audio/video tracks.
        // ML_LOG(Streamer, Warning, "AddStreams, ready to setup video track");
        // SetupVideoTrack(PlayerId, PeerConnection, GetVideoStreamID(), VideoTrackLabel, Flags);
        SetupVideoSource(PlayerId, PeerConnection);
        SetupAudioTrack(PlayerId, PeerConnection, GetAudioStreamID(), AudioTrackLabel, Flags);
    }

    std::vector<webrtc::RtpEncodingParameters> FStreamer::CreateRTPEncodingParams(int Flags) {
        bool bIsSFU = (Flags & EPlayerFlags::PSPFlag_IsSFU) != EPlayerFlags::PSPFlag_None;

        std::vector<webrtc::RtpEncodingParameters> EncodingParams;

        if (bIsSFU /* && Settings::SimulcastParameters.Layers.size() > 0 */) {
            // using FLayer = Settings::FSimulcastParameters::FLayer;

            // // encodings should be lowest res to highest
            // std::vector<FLayer *> SortedLayers;
            // for (FLayer &Layer : Settings::SimulcastParameters.Layers)
            // {
            //     SortedLayers.Add(&Layer);
            // }

            // SortedLayers.Sort([](const FLayer &LayerA, const FLayer &LayerB)
            //                   { return LayerA.Scaling > LayerB.Scaling; });

            // const int LayerCount = SortedLayers.size();
            // for (int i = 0; i < LayerCount; ++i)
            // {
            //     const FLayer *SimulcastLayer = SortedLayers[i];
            //     webrtc::RtpEncodingParameters LayerEncoding{};
            //     LayerEncoding.rid = TCHAR_TO_UTF8(*(std::string("simulcast") +
            //     std::string::FromInt(LayerCount - i))); LayerEncoding.min_bitrate_bps =
            //     SimulcastLayer->MinBitrate; LayerEncoding.max_bitrate_bps =
            //     SimulcastLayer->MaxBitrate; LayerEncoding.scale_resolution_down_by =
            //     SimulcastLayer->Scaling; LayerEncoding.max_framerate = FMath::Max(60,
            //     FAppConfig::Get().WebRTCFps);

            //     // In M84 this will crash with "Attempted to set an unimplemented parameter of
            //     RtpParameters".
            //     // Try re-enabling this when we upgrade WebRTC versions.
            //     // LayerEncoding.network_priority = webrtc::Priority::kHigh;

            //     EncodingParams.push_back(LayerEncoding);
            // }
        } else {
            webrtc::RtpEncodingParameters encoding{};
            encoding.rid = "base";
            encoding.max_bitrate_bps = FAppConfig::Get().WebRTCMaxBitrate;
            encoding.min_bitrate_bps = FAppConfig::Get().WebRTCMinBitrate;
            encoding.max_framerate = std::max(60, FAppConfig::Get().WebRTCFps);
            encoding.scale_resolution_down_by.reset();
            encoding.network_priority = webrtc::Priority::kHigh;
            EncodingParams.push_back(encoding);
        }

        return EncodingParams;
    }

    webrtc::DegradationPreference
    FStreamer::GetDegradationPreference(std::string DegradationPreference) {
        if (DegradationPreference == "MAINTAIN_FRAMERATE") {
            return webrtc::DegradationPreference::MAINTAIN_FRAMERATE;
        } else if (DegradationPreference == "MAINTAIN_RESOLUTION") {
            return webrtc::DegradationPreference::MAINTAIN_RESOLUTION;
        }
        // Everything else, return balanced.
        return webrtc::DegradationPreference::BALANCED;
    }

    void FStreamer::SetupVideoSource(FPixelStreamingPlayerId PlayerId, webrtc::PeerConnectionInterface *PeerConnection) {

        // Create a video source for this player
        rtc::scoped_refptr<FVideoSourceBase> VideoSource;
        ML_LOG(Streamer, Log, "SetupVideoSource");
        VideoSource = new FVideoSourceP2P(PlayerId, this);

        PlayerSessions.SetVideoSource(PlayerId, VideoSource);
        VideoSource->Initialize();
    }

    void FStreamer::SetupVideoTrack(FPixelStreamingPlayerId PlayerId,
                                    webrtc::PeerConnectionInterface* PeerConnection,
                                    const std::string InVideoStreamId,
                                    const std::string InVideoTrackLabel, int Flags) {

        bool bIsSFU = (Flags & EPlayerFlags::PSPFlag_IsSFU) != EPlayerFlags::PSPFlag_None;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PCFactory =
            bIsSFU ? SFUPeerConnectionFactory : P2PPeerConnectionFactory;

        // Create a video source for this player
        rtc::scoped_refptr<FVideoSourceBase> VideoSource;
        if (bIsSFU) {
            VideoSource = new FVideoSourceSFU();
        } else {
            ML_LOG(Streamer, Log, "SetupVideoTrack, Not SFU, create VideoSourceP2P");
            VideoSource = new FVideoSourceP2P(PlayerId, this);
        }

        PlayerSessions.SetVideoSource(PlayerId, VideoSource);
        VideoSource->Initialize();

        // Create video track
        rtc::scoped_refptr<webrtc::VideoTrackInterface> VideoTrack =
            PCFactory->CreateVideoTrack(InVideoTrackLabel, VideoSource.get());
        VideoTrack->set_enabled(true);

        // Set some content hints based on degradation prefs, WebRTC uses these internally.
        webrtc::DegradationPreference DegradationPref =
            GetDegradationPreference(FAppConfig::Get().DegradationPreference);
        switch (DegradationPref) {
        case webrtc::DegradationPreference::MAINTAIN_FRAMERATE:
            VideoTrack->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kFluid);
            break;
        case webrtc::DegradationPreference::MAINTAIN_RESOLUTION:
            VideoTrack->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kDetailed);
            break;
        default:
            break;
        }

        bool bHasVideoTransceiver = false;

        for (auto& Transceiver : PeerConnection->GetTransceivers()) {
            rtc::scoped_refptr<webrtc::RtpSenderInterface> Sender = Transceiver->sender();
            if (Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO) {
                bHasVideoTransceiver = true;
                Sender->SetTrack(VideoTrack.get());
            }
        }

        // If there is no existing video transceiver, add one.
        if (!bHasVideoTransceiver) {
            webrtc::RtpTransceiverInit TransceiverOptions;
            TransceiverOptions.stream_ids = {InVideoStreamId};
            TransceiverOptions.direction = webrtc::RtpTransceiverDirection::kSendOnly;
            TransceiverOptions.send_encodings = CreateRTPEncodingParams(Flags);

            webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> Result =
                PeerConnection->AddTransceiver(VideoTrack, TransceiverOptions);
            // checkf(Result.ok(), std::string("Failed to add Video transceiver to PeerConnection.
            // Msg=%s"), *std::string(Result.error().message()));
        }
    }

    void FStreamer::SetupAudioTrack(FPixelStreamingPlayerId PlayerId,
                                    webrtc::PeerConnectionInterface* PeerConnection,
                                    std::string const InAudioStreamId,
                                    std::string const InAudioTrackLabel, int Flags) {

        bool bIsSFU = (Flags & EPlayerFlags::PSPFlag_IsSFU) != EPlayerFlags::PSPFlag_None;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PCFactory = bIsSFU ?
        SFUPeerConnectionFactory : P2PPeerConnectionFactory;

        bool bTransmitUEAudio = !FAppConfig::Get().WebRTCDisableTransmitAudio;

        // // Create one and only one audio source for Pixel Streaming.
        if (!AudioSource && bTransmitUEAudio)
        {
            // Setup audio source options, we turn off many of the "nice" audio settings that
            // would traditionally be used in a conference call because the audio source we are
            // transmitting is UE application audio (not some unknown microphone).
            AudioSourceOptions.echo_cancellation = false;
            AudioSourceOptions.auto_gain_control = false;
            AudioSourceOptions.noise_suppression = false;
            AudioSourceOptions.highpass_filter = false;
            AudioSourceOptions.stereo_swapping = false;
            AudioSourceOptions.audio_jitter_buffer_max_packets = 1000;
            AudioSourceOptions.audio_jitter_buffer_fast_accelerate = false;
            AudioSourceOptions.audio_jitter_buffer_min_delay_ms = 0;
            // AudioSourceOptions.audio_jitter_buffer_enable_rtx_handling = false;
            // AudioSourceOptions.typing_detection = false;
            // AudioSourceOptions.experimental_agc = false;
            // AudioSourceOptions.experimental_ns = false;
            // AudioSourceOptions.residual_echo_detector = false;
            // Create audio source
            AudioSource = PCFactory->CreateAudioSource(AudioSourceOptions);
        }

        // Add the audio track to the audio transceiver's sender if we are transmitting audio
        if (!bTransmitUEAudio)
        {
            return;
        }

        rtc::scoped_refptr<webrtc::AudioTrackInterface> AudioTrack =
        PCFactory->CreateAudioTrack(InAudioTrackLabel, AudioSource.get());

        bool bHasAudioTransceiver = false;
        for (auto &Transceiver : PeerConnection->GetTransceivers())
        {
            rtc::scoped_refptr<webrtc::RtpSenderInterface> Sender = Transceiver->sender();
            if (Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_AUDIO)
            {
                bHasAudioTransceiver = true;
                Sender->SetTrack(AudioTrack.get());
            }
        }

        if (!bHasAudioTransceiver)
        {
            // Add the track
            webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> Result =
            PeerConnection->AddTrack(AudioTrack, {InAudioStreamId});

            if (!Result.ok())
            {
                ML_LOG(LogPixelStreaming, Error, "Failed to add audio track to PeerConnection.Msg=%s", Result.error().message());
            }
        }
    }

    void FStreamer::ModifyTransceivers(
        std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> Transceivers, int Flags) {
        bool bTransmitUEAudio = !FAppConfig::Get().WebRTCDisableTransmitAudio;
        bool bIsSFU = (Flags & EPlayerFlags::PSPFlag_IsSFU) != EPlayerFlags::PSPFlag_None;
        bool bReceiveBrowserAudio = !bIsSFU && !FAppConfig::Get().WebRTCDisableReceiveAudio;

        for (auto& Transceiver : Transceivers) {
            rtc::scoped_refptr<webrtc::RtpSenderInterface> Sender = Transceiver->sender();

            if (Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_AUDIO) {

                // Determine the direction of the transceiver
                webrtc::RtpTransceiverDirection AudioTransceiverDirection;
                if (bTransmitUEAudio && bReceiveBrowserAudio) {
                    AudioTransceiverDirection = webrtc::RtpTransceiverDirection::kSendRecv;
                } else if (bTransmitUEAudio) {
                    AudioTransceiverDirection = webrtc::RtpTransceiverDirection::kSendOnly;
                } else if (bReceiveBrowserAudio) {
                    AudioTransceiverDirection = webrtc::RtpTransceiverDirection::kRecvOnly;
                } else {
                    AudioTransceiverDirection = webrtc::RtpTransceiverDirection::kInactive;
                }

                Transceiver->SetDirection(AudioTransceiverDirection);
                Sender->SetStreams({GetAudioStreamID()});
            } else if (Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO) {
                Transceiver->SetDirection(webrtc::RtpTransceiverDirection::kSendOnly);
                Sender->SetStreams({GetVideoStreamID()});

                webrtc::RtpParameters RtpParams = Sender->GetParameters();
                RtpParams.encodings = CreateRTPEncodingParams(Flags);
                Sender->SetParameters(RtpParams);
            }
        }
    }

    void FStreamer::SendPlayerMessage(EToPlayerMsg Type, const std::string& Descriptor) {
        PlayerSessions.SendMessageAll(Type, Descriptor);
    }

    void FStreamer::SendFreezeFrame(const std::vector<uint8>& JpegBytes) {
        if (!bStreamingStarted) {
            return;
        }

        PlayerSessions.SendFreezeFrame(JpegBytes);

        CachedJpegBytes = JpegBytes;
    }

    void FStreamer::SendCachedFreezeFrameTo(FPixelStreamingPlayerId PlayerId) const {
        if (CachedJpegBytes.size() > 0) {
            PlayerSessions.SendFreezeFrameTo(PlayerId, CachedJpegBytes);
        }
    }

    void FStreamer::SendFreezeFrameTo(FPixelStreamingPlayerId PlayerId,
                                      const std::vector<uint8>& JpegBytes) const {
        PlayerSessions.SendFreezeFrameTo(PlayerId, JpegBytes);
    }

    void FStreamer::SendUnfreezeFrame() {
        // Force a keyframe so when stream unfreezes if player has never received a h.264 frame
        // before they can still connect.
        ForceKeyFrame();

        PlayerSessions.SendUnfreezeFrame();

        CachedJpegBytes.clear();
    }

    void FStreamer::SendFileData(std::vector<uint8>& ByteData, std::string& MimeType,
                                 std::string& FileExtension) {
        PlayerSessions.SendFileData(ByteData, MimeType, FileExtension);
    }

    void FStreamer::KickPlayer(FPixelStreamingPlayerId PlayerId) {
        SignallingServerConnection->SendDisconnectPlayer(PlayerId,
                                                         std::string("Player was kicked"));
    }

} // namespace OpenZI::CloudRender