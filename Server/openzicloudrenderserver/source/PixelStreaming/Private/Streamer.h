// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/02 18:28
// 
#pragma once

#include "SignallingServerConnection.h"
#include "ProtocolDefs.h"
#include "IPixelStreamingSessions.h"
#include "ThreadSafePlayerSessions.h"
#include "FixedFPSPump.h"
#include "MessageCenter/MessageCenter.h"
#include <atomic>

namespace OpenZI::CloudRender
{
    class FPlayerSession;
    class FVideoEncoderFactory;
    class FSimulcastEncoderFactory;
    class FSetSessionDescriptionObserver;

    class FStreamer : public FSignallingServerConnectionObserver, public IPixelStreamingSessions
    {
    public:
        void SendVideo(const std::string& VideoContent);
        static bool IsPlatformCompatible();

        FStreamer();
        explicit FStreamer(const std::string &SignallingServerUrl, const std::string &StreamerId);
        virtual ~FStreamer() override;

        void SendPlayerMessage(EToPlayerMsg Type, const std::string &Descriptor);
        void SendFreezeFrame(const std::vector<uint8> &JpegBytes);
        void SendUnfreezeFrame();
        void SendFileData(std::vector<uint8> &ByteData, std::string &MimeType, std::string &FileExtension);
        void KickPlayer(FPixelStreamingPlayerId PlayerId);
        void ForceKeyFrame();

        bool IsStreaming() const { return bStreamingStarted; }
        void SetStreamingStarted(bool bStarted) { bStreamingStarted = bStarted; }
        FSignallingServerConnection *GetSignallingServerConnection() const { return SignallingServerConnection.get(); }
        // FVideoEncoderFactory *GetP2PVideoEncoderFactory() const { return P2PVideoEncoderFactory; }
        FFixedFPSPump &GetPumpThread() { return PumpThread; }
        // FGPUFencePoller &GetFencePollerThread() { return FencePollerThread; }


        // Begin IPixelStreamingSessions interface.
        virtual int GetNumPlayers() const override;
        virtual IPixelStreamingAudioSink *GetAudioSink(FPixelStreamingPlayerId PlayerId) const override;
        virtual IPixelStreamingAudioSink *GetUnlistenedAudioSink() const override;
        virtual bool IsQualityController(FPixelStreamingPlayerId PlayerId) const override;
        virtual void SetQualityController(FPixelStreamingPlayerId PlayerId) override;
        virtual bool Send(FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type, const std::string &Descriptor) const override;
        virtual void SendLatestQP(FPixelStreamingPlayerId PlayerId, int LatestQP) const override;
        virtual void SendFreezeFrameTo(FPixelStreamingPlayerId PlayerId, const std::vector<uint8> &JpegBytes) const override;
        virtual void PollWebRTCStats() const override;
        // End IPixelStreamingSessions interface.

        void SendCachedFreezeFrameTo(FPixelStreamingPlayerId PlayerId) const;

    private:
        webrtc::PeerConnectionInterface *CreateSession(FPixelStreamingPlayerId PlayerId, int Flags);
        rtc::scoped_refptr<webrtc::AudioProcessing> SetupAudioProcessingModule();

        // Procedure for WebRTC inter-thread communication
        void StartWebRtcSignallingThread();
        void ConnectToSignallingServer();

        // ISignallingServerConnectionObserver impl
        virtual void OnConfig(const webrtc::PeerConnectionInterface::RTCConfiguration &Config) override;
        virtual void OnSessionDescription(FPixelStreamingPlayerId PlayerId, webrtc::SdpType Type, const std::string &Sdp) override;
        virtual void OnRemoteIceCandidate(FPixelStreamingPlayerId PlayerId, const std::string &SdpMid, int SdpMLineIndex, const std::string &Sdp) override;
        virtual void OnPlayerConnected(FPixelStreamingPlayerId PlayerId, int Flags) override;
        virtual void OnPlayerDisconnected(FPixelStreamingPlayerId PlayerId) override;
        virtual void OnSignallingServerDisconnected() override;

        // own methods
        void OnOffer(FPixelStreamingPlayerId PlayerId, const std::string &Sdp);
        void ModifyTransceivers(std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> Transceivers, int Flags);
        void MungeRemoteSDP(cricket::SessionDescription *SessionDescription);
        void DeletePlayerSession(FPixelStreamingPlayerId PlayerId);
        void DeleteAllPlayerSessions();
        void AddStreams(FPixelStreamingPlayerId PlayerId, webrtc::PeerConnectionInterface *PeerConnection, int Flags);
        void SetupVideoTrack(FPixelStreamingPlayerId PlayerId, webrtc::PeerConnectionInterface *PeerConnection, std::string const VideoStreamId, std::string const VideoTrackLabel, int Flags);
        void SetupVideoSource(FPixelStreamingPlayerId PlayerId, webrtc::PeerConnectionInterface *PeerConnection);
        void SetupAudioTrack(FPixelStreamingPlayerId PlayerId, webrtc::PeerConnectionInterface *PeerConnection, const std::string AudioStreamId, const std::string AudioTrackLabel, int Flags);
        std::vector<webrtc::RtpEncodingParameters> CreateRTPEncodingParams(int Flags);
        void SendAnswer(FPixelStreamingPlayerId PlayerId, webrtc::PeerConnectionInterface *PeerConnection, std::unique_ptr<webrtc::SessionDescriptionInterface> Sdp);
        void OnDataChannelOpen(FPixelStreamingPlayerId PlayerId, webrtc::DataChannelInterface *DataChannel);
        void OnQualityControllerChanged(FPixelStreamingPlayerId PlayerId);
        void PostPlayerDeleted(FPixelStreamingPlayerId PlayerId, bool WasQualityController);
        void SetLocalDescription(webrtc::PeerConnectionInterface *PeerConnection, FSetSessionDescriptionObserver *Observer, webrtc::SessionDescriptionInterface *SDP);
        std::string GetAudioStreamID() const;
        std::string GetVideoStreamID() const;
        webrtc::DegradationPreference GetDegradationPreference(std::string DegradationPreference);
        rtc::scoped_refptr<webrtc::AudioDeviceModule> CreateAudioDeviceModule() const;

    private:
        std::string SignallingServerUrl;
        std::string StreamerId;
        const std::string AudioTrackLabel = std::string("pixelstreaming_audio_track_label");
        const std::string VideoTrackLabel = std::string("pixelstreaming_video_track_label");

        std::unique_ptr<rtc::Thread> WebRtcSignallingThread;
        std::unique_ptr<rtc::Thread> WorkerThread;

        std::unique_ptr<FSignallingServerConnection> SignallingServerConnection;
        double LastSignallingServerConnectionAttemptTimestamp = 0;

        webrtc::PeerConnectionInterface::RTCConfiguration PeerConnectionConfig;

        /* P2P Peer Connections use these exclusively */
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> P2PPeerConnectionFactory;
        FVideoEncoderFactory *P2PVideoEncoderFactory;
        /* P2P Peer Connections use these exclusively */

        /* SFU Peer Connections use these exclusively */
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> SFUPeerConnectionFactory;
        FSimulcastEncoderFactory *SFUVideoEncoderFactory;
        /* SFU Peer Connections use these exclusively */

        rtc::scoped_refptr<webrtc::AudioSourceInterface> AudioSource;
        cricket::AudioOptions AudioSourceOptions;

        // When we send a freeze frame we retain the data so we send freeze frame to new peers if they join during a freeze frame.
        std::vector<uint8> CachedJpegBytes;

        std::atomic<bool> bStreamingStarted = false;

        FThreadSafePlayerSessions PlayerSessions;
        // FStats Stats;

        FFixedFPSPump PumpThread;
        // FGPUFencePoller FencePollerThread;

        std::unique_ptr<webrtc::SessionDescriptionInterface> SFULocalDescription;
        std::unique_ptr<webrtc::SessionDescriptionInterface> SFURemoteDescription;
        DECLARE_MESSAGE(OnClosedConnectionNative);
        DECLARE_MESSAGE(OnQualityControllerChangedNative);
        DECLARE_MESSAGE(OnDataChannelOpenNative);
        DECLARE_MESSAGE(OnSendVideo);
    };
} // namespace OpenZI::CloudRender