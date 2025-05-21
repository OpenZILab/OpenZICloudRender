//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/01 11:06
//
#include "PlayerSession.h"
#include "SessionDescriptionObservers.h"
#include "Logger/CommandLogger.h"
#include "DataConverter.h"
#include <thread>

namespace OpenZI::CloudRender
{

    inline const char *EnumToString(webrtc::PeerConnectionInterface::SignalingState Val)
    {
        char const *SignallingStatesStr[] = {
            "Stable",
            "HaveLocalOffer",
            "HaveLocalPrAnswer",
            "HaveRemoteOffer",
            "HaveRemotePrAnswer",
            "Closed"};
        return (0 <= Val && Val <= webrtc::PeerConnectionInterface::kClosed) ? SignallingStatesStr[Val] : Printf("Invalid `webrtc::PeerConnectionInterface::SignalingState` value: %d", static_cast<uint32>(Val)).c_str();
        // return ensureMsgf(0 <= Val && Val <= webrtc::PeerConnectionInterface::kClosed, "Invalid `webrtc::PeerConnectionInterface::SignalingState` value: %d"), static_cast<uint32>(Val)) ? SignallingStatesStr[Val] : "Unknown");
    }

    inline const char *EnumToString(webrtc::PeerConnectionInterface::IceConnectionState Val)
    {
        char const *IceConnectionStatsStr[] = {
            "IceConnectionNew",
            "IceConnectionChecking",
            "IceConnectionConnected",
            "IceConnectionCompleted",
            "IceConnectionFailed",
            "IceConnectionDisconnected",
            "IceConnectionClosed"};

        return (0 <= Val && Val < webrtc::PeerConnectionInterface::kIceConnectionMax) ? IceConnectionStatsStr[Val] : Printf("Invalid `webrtc::PeerConnectionInterface::IceConnectionState` value: %d", static_cast<uint32>(Val)).c_str();
    }

    inline const char *EnumToString(webrtc::PeerConnectionInterface::IceGatheringState Val)
    {
        char const *IceGatheringStatsStr[] = {
            "IceGatheringNew",
            "IceGatheringGathering",
            "IceGatheringComplete"};

        return (0 <= Val && Val <= webrtc::PeerConnectionInterface::kIceGatheringComplete)
                   ? IceGatheringStatsStr[Val]
                   : Printf("Invalid `webrtc::PeerConnectionInterface::IceGatheringState` value: %d", static_cast<uint32>(Val)).c_str();
    }

    FPlayerSession::FPlayerSession(IPixelStreamingSessions *InSessions, FSignallingServerConnection *InSignallingServerConnection, FPixelStreamingPlayerId InPlayerId)
        : SignallingServerConnection(InSignallingServerConnection), PlayerId(InPlayerId), DataChannelObserver(InSessions, InPlayerId)
    {
    }

    FPlayerSession::~FPlayerSession()
    {

        DataChannelObserver.Unregister();
        DataChannel = nullptr;

        if (PeerConnection)
        {
            PeerConnection->Close();
            PeerConnection = nullptr;
        }
    }

    FDataChannelObserver &FPlayerSession::GetDataChannelObserver()
    {
        return DataChannelObserver;
    }

    webrtc::PeerConnectionInterface &FPlayerSession::GetPeerConnection()
    {
        return *PeerConnection;
    }

    void FPlayerSession::SetPeerConnection(const rtc::scoped_refptr<webrtc::PeerConnectionInterface> &InPeerConnection)
    {
        PeerConnection = InPeerConnection;
    }

    void FPlayerSession::SetDataChannel(const rtc::scoped_refptr<webrtc::DataChannelInterface> &InDataChannel)
    {
        DataChannel = InDataChannel;
    }

    void FPlayerSession::SetVideoSource(const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> &InVideoSource)
    {
        VideoSource = InVideoSource;
    }

    FPixelStreamingPlayerId FPlayerSession::GetPlayerId() const
    {
        return PlayerId;
    }

    void FPlayerSession::AddSinkToAudioTrack()
    {
        // Get remote audio track from transceiver and if it exists then add our audio sink to it.
        std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> Transceivers = PeerConnection->GetTransceivers();
        for (rtc::scoped_refptr<webrtc::RtpTransceiverInterface> &Transceiver : Transceivers)
        {
            if (Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_AUDIO)
            {
                rtc::scoped_refptr<webrtc::RtpReceiverInterface> Receiver = Transceiver->receiver();
                rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> Track = Receiver->track();
                if (Track)
                {
                    webrtc::AudioTrackInterface *AudioTrack = static_cast<webrtc::AudioTrackInterface *>(Track.get());
                    AudioTrack->AddSink(&AudioSink);
                }
            }
        }
    }

    void FPlayerSession::OnAnswer(std::string Sdp)
    {
        webrtc::SdpParseError Error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> SessionDesc = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, Sdp, &Error);
        if (!SessionDesc)
        {
            return;
        }

        FSetSessionDescriptionObserver *SetRemoteDescriptionObserver = FSetSessionDescriptionObserver::Create(
            [this]() // on success
            {
                // AddSinkToAudioTrack();
            },
            [](const std::string &Error) // on failure
            {
                ML_LOG(LogPixelStreaming, Error, "Failed to set remote description: %s", Error.c_str());
            });

        PeerConnection->SetRemoteDescription(SetRemoteDescriptionObserver, SessionDesc.release());
    }

    void FPlayerSession::OnRemoteIceCandidate(const std::string &SdpMid, int SdpMLineIndex, const std::string &Sdp)
    {
        webrtc::SdpParseError Error;
        std::unique_ptr<webrtc::IceCandidateInterface> Candidate(webrtc::CreateIceCandidate(SdpMid, SdpMLineIndex, Sdp, &Error));
        if (Candidate)
        {
            PeerConnection->AddIceCandidate(std::move(Candidate), [this](webrtc::RTCError error)
                                            {
			if (!error.ok())
			{
				ML_LOG(LogPixelStreaming, Error, "AddIceCandidate failed (%s): %s", PlayerId.c_str(), error.message());
			} });
        }
        else
        {
            ML_LOG(LogPixelStreaming, Error, "Could not create ice candidate for player %s", PlayerId.c_str());
            ML_LOG(LogPixelStreaming, Error, "Bad sdp at line %s | Description: %s", Error.line.c_str(), Error.description.c_str());
        }
    }

    void FPlayerSession::DisconnectPlayer(const std::string &Reason)
    {
        if (bDisconnecting)
        {
            return; // already notified SignallingServer to disconnect this player
        }

        bDisconnecting = true;
        SignallingServerConnection->SendDisconnectPlayer(PlayerId, Reason);
    }

    FAudioSink &FPlayerSession::GetAudioSink()
    {
        return AudioSink;
    }

    bool FPlayerSession::Send(EToPlayerMsg Type, const std::string &Descriptor) const
    {
        if (!DataChannel)
        {
            return false;
        }
        if (Type == EToPlayerMsg::VideoContent) {
            const uint8 MessageType = static_cast<uint8>(Type);
            const size_t DescriptorSize = Descriptor.size() * sizeof(char);
            rtc::CopyOnWriteBuffer Buffer(sizeof(MessageType) + DescriptorSize);

            size_t Pos = 0;
            Pos = SerializeToBuffer(Buffer, Pos, &MessageType, sizeof(MessageType));
            Pos = SerializeToBuffer(Buffer, Pos, Descriptor.c_str(), DescriptorSize);

            return DataChannel->Send(webrtc::DataBuffer(Buffer, true));
        } else {
            std::wstring Message = FDataConverter::ToWString(Descriptor);
            const uint8 MessageType = static_cast<uint8>(Type);
            const size_t DescriptorSize = Message.size() * sizeof(wchar_t);

        rtc::CopyOnWriteBuffer Buffer(sizeof(MessageType) + DescriptorSize);

        size_t Pos = 0;
        Pos = SerializeToBuffer(Buffer, Pos, &MessageType, sizeof(MessageType));
        Pos = SerializeToBuffer(Buffer, Pos, Message.c_str(), DescriptorSize);


            return DataChannel->Send(webrtc::DataBuffer(Buffer, true));
        }
    }

    void FPlayerSession::SendFreezeFrame(const std::vector<uint8> &JpegBytes) const
    {
        FPlayerSession::SendArbitraryData(JpegBytes, static_cast<uint8>(EToPlayerMsg::FreezeFrame));
    }

    void FPlayerSession::SendUnfreezeFrame() const
    {
        if (!DataChannel)
        {
            return;
        }

        const uint8 MessageType = static_cast<uint8>(EToPlayerMsg::UnfreezeFrame);

        rtc::CopyOnWriteBuffer Buffer(sizeof(MessageType));

        size_t Pos = 0;
        Pos = SerializeToBuffer(Buffer, Pos, &MessageType, sizeof(MessageType));

        if (!DataChannel->Send(webrtc::DataBuffer(Buffer, true)))
        {
            ML_LOG(LogPixelStreaming, Error, "failed to send unfreeze frame");
        }
    }

    void FPlayerSession::SendArbitraryData(const std::vector<uint8> &DataBytes, const uint8 MessageType) const
    {
        if (!DataChannel)
        {
            return;
        }

        // int32 results in a maximum 4GB file (4,294,967,296 bytes)
        const int32 DataSize = static_cast<int32>(DataBytes.size());

        // Maximum size of a single buffer should be 16KB as this is spec compliant message length for a single data channel transmission
        const int32 MaxBufferBytes = 16 * 1024;
        const int32 MessageHeader = sizeof(MessageType) + sizeof(DataSize);
        const int32 MaxDataBytesPerMsg = MaxBufferBytes - MessageHeader;

        int32 BytesTransmitted = 0;

        while (BytesTransmitted < DataSize)
        {
            int32 RemainingBytes = DataSize - BytesTransmitted;
            int32 BytesToTransmit = std::min(MaxDataBytesPerMsg, RemainingBytes);

            rtc::CopyOnWriteBuffer Buffer(MessageHeader + BytesToTransmit);

            size_t Pos = 0;

            // Write message type
            Pos = SerializeToBuffer(Buffer, Pos, &MessageType, sizeof(MessageType));
            // Write size of payload
            Pos = SerializeToBuffer(Buffer, Pos, &DataSize, sizeof(DataSize));
            // Write the data bytes payload
            Pos = SerializeToBuffer(Buffer, Pos, DataBytes.data() + BytesTransmitted, BytesToTransmit);

            uint64_t BufferBefore = DataChannel->buffered_amount();
            while (BufferBefore + BytesToTransmit >= 16 * 1024 * 1024) // 16MB (WebRTC Data Channel buffer size)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(1)); //睡眠1微秒
                BufferBefore = DataChannel->buffered_amount();
            }

            if (!DataChannel->Send(webrtc::DataBuffer(Buffer, true)))
            {
                ML_LOG(LogPixelStreaming, Error, "Failed to send data channel packet");
                return;
            }

            // Increment the number of bytes transmitted
            BytesTransmitted += BytesToTransmit;
        }
    }

    //
    // webrtc::PeerConnectionObserver implementation.
    //

    void FPlayerSession::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState NewState)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s, NewState=%s", "FPlayerSession::OnSignalingChange", PlayerId.c_str(), EnumToString(NewState));
    }

    // Called when a remote stream is added
    void FPlayerSession::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> Stream)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s, Stream=%s", "FPlayerSession::OnAddStream", PlayerId.c_str(), Stream->id().c_str());
    }

    void FPlayerSession::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> Stream)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s, Stream=%s", "FPlayerSession::OnRemoveStream", PlayerId.c_str(), Stream->id().c_str());
    }

    void FPlayerSession::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> InDataChannel)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s", "FPlayerSession::OnDataChannel", PlayerId.c_str());
        DataChannel = InDataChannel;
        DataChannelObserver.Register(InDataChannel);
    }

    void FPlayerSession::OnRenegotiationNeeded()
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s", "FPlayerSession::OnRenegotiationNeeded", PlayerId.c_str());
    }

    void FPlayerSession::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState NewState)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s, NewState=%s", "FPlayerSession::OnIceConnectionChange", PlayerId.c_str(), EnumToString(NewState));
    }

    void FPlayerSession::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState NewState)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s, NewState=%s", "FPlayerSession::OnIceGatheringChange", PlayerId.c_str(), EnumToString(NewState));
    }

    void FPlayerSession::OnIceCandidate(const webrtc::IceCandidateInterface *Candidate)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s", "FPlayerSession::OnIceCandidate", PlayerId.c_str());

        SignallingServerConnection->SendIceCandidate(PlayerId, *Candidate);
    }

    void FPlayerSession::OnIceCandidatesRemoved(const std::vector<cricket::Candidate> &candidates)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s", "FPlayerSession::OnIceCandidatesRemoved", PlayerId.c_str());
    }

    void FPlayerSession::OnIceConnectionReceivingChange(bool Receiving)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s, Receiving=%d", "FPlayerSession::OnIceConnectionReceivingChange", PlayerId.c_str(), *reinterpret_cast<int8 *>(&Receiving));
    }

    void FPlayerSession::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s", "FPlayerSession::OnTrack", PlayerId.c_str());

        AddSinkToAudioTrack();

        // print out track type
        cricket::MediaType mediaType = transceiver->media_type();
        switch (mediaType)
        {
        case cricket::MediaType::MEDIA_TYPE_AUDIO:
            ML_LOG(LogPixelStreaming, Log, "Track was type: audio");
            break;
        case cricket::MediaType::MEDIA_TYPE_VIDEO:
            ML_LOG(LogPixelStreaming, Log, "Track was type: video");
            break;
        case cricket::MediaType::MEDIA_TYPE_DATA:
            ML_LOG(LogPixelStreaming, Log, "Track was type: data");
            break;
        default:
            ML_LOG(LogPixelStreaming, Log, "Track was an unsupported type");
            break;
        }

        // print out track direction
        webrtc::RtpTransceiverDirection Direction = transceiver->direction();
        switch (Direction)
        {
        case webrtc::RtpTransceiverDirection::kSendRecv:
            ML_LOG(LogPixelStreaming, Log, "Track direction: send+recv");
            break;
        case webrtc::RtpTransceiverDirection::kSendOnly:
            ML_LOG(LogPixelStreaming, Log, "Track direction: send only");
            break;
        case webrtc::RtpTransceiverDirection::kRecvOnly:
            ML_LOG(LogPixelStreaming, Log, "Track direction: recv only");
            break;
        case webrtc::RtpTransceiverDirection::kInactive:
            ML_LOG(LogPixelStreaming, Log, "Track direction: inactive");
            break;
        case webrtc::RtpTransceiverDirection::kStopped:
            ML_LOG(LogPixelStreaming, Log, "Track direction: stopped");
            break;
        }

        rtc::scoped_refptr<webrtc::RtpReceiverInterface> Receiver = transceiver->receiver();
        if (mediaType != cricket::MediaType::MEDIA_TYPE_AUDIO)
        {
            return;
        }

        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> MediaStreamTrack = Receiver->track();
        std::string TrackEnabledStr = MediaStreamTrack->enabled() ? std::string("Enabled") : std::string("Disabled");
        std::string TrackStateStr = MediaStreamTrack->state() == webrtc::MediaStreamTrackInterface::TrackState::kLive ? std::string("Live") : std::string("Ended");
        ML_LOG(LogPixelStreaming, Log, "MediaStreamTrack id: %s | Is enabled: %s | State: %s", MediaStreamTrack->id().c_str(), TrackEnabledStr.c_str(), TrackStateStr.c_str());

        webrtc::AudioTrackInterface *AudioTrackPtr = static_cast<webrtc::AudioTrackInterface *>(MediaStreamTrack.get());
        webrtc::AudioSourceInterface *AudioSourcePtr = AudioTrackPtr->GetSource();
        std::string AudioSourceStateStr = AudioSourcePtr->state() == webrtc::MediaSourceInterface::SourceState::kLive ? std::string("Live") : std::string("Not live");
        std::string AudioSourceRemoteStr = AudioSourcePtr->remote() ? std::string("Remote") : std::string("Local");
        ML_LOG(LogPixelStreaming, Log, "AudioSource | State: %s | Locality: %s", AudioSourceStateStr.c_str(), AudioSourceRemoteStr.c_str());
    }

    void FPlayerSession::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
    {
        ML_LOG(LogPixelStreaming, Log, "%s : PlayerId=%s", "FPlayerSession::OnRemoveTrack", PlayerId.c_str());
    }

    size_t FPlayerSession::SerializeToBuffer(rtc::CopyOnWriteBuffer &Buffer, size_t Pos, const void *Data, size_t DataSize) const
    {
        memcpy(const_cast<uint8_t *>(Buffer.data() + Pos), reinterpret_cast<const uint8_t *>(Data), DataSize);
        return Pos + DataSize;
    }

    void FPlayerSession::SendQualityControlStatus(bool bIsQualityController) const
    {
        // FStats::Get()->StorePeerStat(PlayerId, FStatData(PixelStreamingStatNames::QualityController, bIsQualityController ? 1.0 : 0.0, 0));
        if (!DataChannel)
        {
            return;
        }

        const uint8 MessageType = static_cast<uint8>(EToPlayerMsg::QualityControlOwnership);
        const uint8 ControlsQuality = bIsQualityController ? 1 : 0;

        rtc::CopyOnWriteBuffer Buffer(sizeof(MessageType) + sizeof(ControlsQuality));

        size_t Pos = 0;
        Pos = SerializeToBuffer(Buffer, Pos, &MessageType, sizeof(MessageType));
        Pos = SerializeToBuffer(Buffer, Pos, &ControlsQuality, sizeof(ControlsQuality));

        if (!DataChannel->Send(webrtc::DataBuffer(Buffer, true)))
        {
            ML_LOG(LogPixelStreaming, Error, "failed to send quality control status");
        }
    }
} // namespace OpenZI::CloudRender