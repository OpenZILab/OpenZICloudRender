// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/02 16:06
// 
#pragma once

#include "CoreMinimal.h"
#include "IPixelStreamingSessions.h"
#include "PlayerSession.h"
#include "SignallingServerConnection.h"
#include "Thread/CriticalSection.h"
#include <vector>
#include <string>
#include <map>



namespace OpenZI::CloudRender
{
    class FDataChannelObserver;
    // This class provides a mechanism to read/write FPlayerSession on any thread.
    // This is mostly achieved by submitting all such work to the WebRTC signalling thread
    // using the WebRTC task queue (which is a single thread).
    // Note: Any public methods in this class with a return type will making the calling thread block
    // until the WebRTC thread is done and can return the result.
    class FThreadSafePlayerSessions : public IPixelStreamingSessions
    {
    public:
        FThreadSafePlayerSessions(rtc::Thread *WebRtcSignallingThread);
        virtual ~FThreadSafePlayerSessions() = default;
        bool IsInSignallingThread() const;
        void SendFreezeFrame(const std::vector<uint8> &JpegBytes);
        void SendUnfreezeFrame();
        void SendFileData(const std::vector<uint8> &ByteData, const std::string &MimeType, const std::string &FileExtension);
        void SendMessageAll(EToPlayerMsg Type, const std::string &Descriptor) const;
        void DisconnectPlayer(FPixelStreamingPlayerId PlayerId, const std::string &Reason);
        void SendLatestQPAllPlayers(int LatestQP) const;
        void OnRemoteIceCandidate(FPixelStreamingPlayerId PlayerId, const std::string &SdpMid, int SdpMLineIndex, const std::string &Sdp);
        void OnAnswer(FPixelStreamingPlayerId PlayerId, std::string Sdp);

        webrtc::PeerConnectionInterface *CreatePlayerSession(FPixelStreamingPlayerId PlayerId,
                                                             rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory,
                                                             webrtc::PeerConnectionInterface::RTCConfiguration PeerConnectionConfig,
                                                             FSignallingServerConnection *SignallingServerConnection,
                                                             int Flags);

        void DeleteAllPlayerSessions();
        int DeletePlayerSession(FPixelStreamingPlayerId PlayerId);
        FDataChannelObserver *GetDataChannelObserver(FPixelStreamingPlayerId PlayerId);
        void SetPlayerSessionDataChannel(FPixelStreamingPlayerId PlayerId, const rtc::scoped_refptr<webrtc::DataChannelInterface> &DataChannel);
        void SetVideoSource(FPixelStreamingPlayerId PlayerId, const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> &VideoSource);

        // Begin IPixelStreamingSessions
        virtual int GetNumPlayers() const override;
        virtual IPixelStreamingAudioSink *GetAudioSink(FPixelStreamingPlayerId PlayerId) const override;
        virtual IPixelStreamingAudioSink *GetUnlistenedAudioSink() const override;
        virtual bool IsQualityController(FPixelStreamingPlayerId PlayerId) const override;
        virtual void SetQualityController(FPixelStreamingPlayerId PlayerId) override;
        virtual bool Send(FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type, const std::string &Descriptor) const override;
        virtual void SendLatestQP(FPixelStreamingPlayerId PlayerId, int LatestQP) const override;
        virtual void SendFreezeFrameTo(FPixelStreamingPlayerId PlayerId, const std::vector<uint8> &JpegBytes) const override;
        void PollWebRTCStats() const override;
        // End IPixelStreamingSessions

    private:
        // Note: This is very intentionally internal and there is no public version because as soon as we hand it out it isn't thread safe anymore.
        FPlayerSession *GetPlayerSession_SignallingThread(FPixelStreamingPlayerId PlayerId) const;

        void SetVideoSource_SignallingThread(FPixelStreamingPlayerId PlayerId, const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> &VideoSource);
        void SetPlayerSessionDataChannel_SignallingThread(FPixelStreamingPlayerId PlayerId, const rtc::scoped_refptr<webrtc::DataChannelInterface> &DataChannel);
        void PollWebRTCStats_SignallingThread() const;
        void OnRemoteIceCandidate_SignallingThread(FPixelStreamingPlayerId PlayerId, const std::string &SdpMid, int SdpMLineIndex, const std::string &Sdp);
        void OnAnswer_SignallingThread(FPixelStreamingPlayerId PlayerId, std::string Sdp);
        IPixelStreamingAudioSink *GetUnlistenedAudioSink_SignallingThread() const;
        IPixelStreamingAudioSink *GetAudioSink_SignallingThread(FPixelStreamingPlayerId PlayerId) const;
        void SendLatestQP_SignallingThread(FPixelStreamingPlayerId PlayerId, int LatestQP) const;
        int GetNumPlayers_SignallingThread() const;
        void SendFreezeFrame_SignallingThread(const std::vector<uint8> &JpegBytes);
        void SendUnfreezeFrame_SignallingThread();
        void SendFreezeFrameTo_SignallingThread(FPixelStreamingPlayerId PlayerId, const std::vector<uint8> &JpegBytes) const;
        void SendFileData_SignallingThread(const std::vector<uint8> &ByteData, const std::string &MimeType, const std::string &FileExtension);
        void SetQualityController_SignallingThread(FPixelStreamingPlayerId PlayerId);
        void DisconnectPlayer_SignallingThread(FPixelStreamingPlayerId PlayerId, const std::string &Reason);
        void SendMessageAll_SignallingThread(EToPlayerMsg Type, const std::string &Descriptor) const;
        bool SendMessage_SignallingThread(FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type, const std::string &Descriptor) const;
        void SendLatestQPAllPlayers_SignallingThread(int LatestQP) const;

        webrtc::PeerConnectionInterface *CreatePlayerSession_SignallingThread(FPixelStreamingPlayerId PlayerId,
                                                                              rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory,
                                                                              webrtc::PeerConnectionInterface::RTCConfiguration PeerConnectionConfig,
                                                                              FSignallingServerConnection *SignallingServerConnection,
                                                                              int Flags);

        void DeleteAllPlayerSessions_SignallingThread();
        int DeletePlayerSession_SignallingThread(FPixelStreamingPlayerId PlayerId);
        FDataChannelObserver *GetDataChannelObserver_SignallingThread(FPixelStreamingPlayerId PlayerId);

    private:
        rtc::Thread *WebRtcSignallingThread;
        std::map<FPixelStreamingPlayerId, FPlayerSession *> Players;

        mutable FCriticalSection QualityControllerCS;
        FPixelStreamingPlayerId QualityControllingPlayer = ToPlayerId(std::string("No quality controlling peer."));
    };
} // namespace OpenZI::CloudRender