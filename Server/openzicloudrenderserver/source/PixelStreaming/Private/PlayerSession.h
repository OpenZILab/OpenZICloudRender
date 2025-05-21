// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/01 11:02
// 
#pragma once

#include "WebRTCIncludes.h"
#include "ProtocolDefs.h"
#include "AudioSink.h"
#include "PixelStreamingPlayerId.h"
#include "DataChannelObserver.h"
// #include "IPixelStreamingStatsConsumer.h"
#include "SignallingServerConnection.h"
#include <string>
#include <atomic>

namespace OpenZI::CloudRender
{
    class FSignallingServerConnection;
    class IPixelStreamingSessions;

    class FPlayerSession : public webrtc::PeerConnectionObserver
    {
    public:
        FPlayerSession(IPixelStreamingSessions *InSessions, FSignallingServerConnection *InSignallingServerConnection, FPixelStreamingPlayerId PlayerId);
        virtual ~FPlayerSession() override;

        webrtc::PeerConnectionInterface &GetPeerConnection();
        void SetPeerConnection(const rtc::scoped_refptr<webrtc::PeerConnectionInterface> &InPeerConnection);
        void SetDataChannel(const rtc::scoped_refptr<webrtc::DataChannelInterface> &InDataChannel);
        void SetVideoSource(const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> &InVideoSource);

        void OnAnswer(std::string Sdp);
        void OnRemoteIceCandidate(const std::string &SdpMid, int SdpMLineIndex, const std::string &Sdp);
        void DisconnectPlayer(const std::string &Reason);

        FPixelStreamingPlayerId GetPlayerId() const;
        FAudioSink &GetAudioSink();
        FDataChannelObserver &GetDataChannelObserver();

        bool Send(EToPlayerMsg Type, const std::string &Descriptor) const;
        void SendQualityControlStatus(bool bIsQualityController) const;
        void SendFreezeFrame(const std::vector<uint8> &JpegBytes) const;
        // void SendFileData(const std::vector<uint8> &ByteData, const std::string &MimeType, const std::string &FileExtension) const;
        void SendUnfreezeFrame() const;
        void SendArbitraryData(const std::vector<uint8> &DataBytes, const uint8 MessageType) const;
        // void SendVideoEncoderQP(double QP) const;
        void PollWebRTCStats() const;

    private:
        void ModifyAudioTransceiverDirection();
        void AddSinkToAudioTrack();

        //
        // webrtc::PeerConnectionObserver implementation.
        //
        virtual void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState NewState) override;
        virtual void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> Stream) override;
        virtual void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> Stream) override;
        virtual void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> Channel) override;
        virtual void OnRenegotiationNeeded() override;
        virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState NewState) override;
        virtual void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState NewState) override;
        virtual void OnIceCandidate(const webrtc::IceCandidateInterface *Candidate) override;
        virtual void OnIceCandidatesRemoved(const std::vector<cricket::Candidate> &candidates) override;
        virtual void OnIceConnectionReceivingChange(bool Receiving) override;
        virtual void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
        virtual void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;

    private:
        size_t SerializeToBuffer(rtc::CopyOnWriteBuffer &Buffer, size_t Pos, const void *Data, size_t DataSize) const;

        FSignallingServerConnection *SignallingServerConnection;
        FPixelStreamingPlayerId PlayerId;
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> PeerConnection;
        rtc::scoped_refptr<webrtc::DataChannelInterface> DataChannel;
        FDataChannelObserver DataChannelObserver;
        std::atomic<bool> bDisconnecting = false;
        FAudioSink AudioSink;
        rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback> WebRTCStatsCallback;
        // std::shared_ptr<IPixelStreamingStatsConsumer> QPReporter;
        rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> VideoSource;
    };
} // namespace OpenZI::CloudRender