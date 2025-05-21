//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/02 16:11
//
#include "ThreadSafePlayerSessions.h"
#include "Config.h"
#include "DataChannelObserver.h"
#include "Logger/CommandLogger.h"
#include "MessageCenter.h"
#include "rtc_base/location.h"

#define SUBMIT_TASK_WITH_RETURN(ReturnType, FuncWithReturnType)                                    \
    if (IsInSignallingThread()) {                                                                  \
        return FuncWithReturnType();                                                               \
    } else {                                                                                       \
        return WebRtcSignallingThread->Invoke<ReturnType>(                                         \
            RTC_FROM_HERE, [this]() -> ReturnType { return FuncWithReturnType(); });               \
    }

#define SUBMIT_TASK_WITH_PARAMS_AND_RETURN(ReturnType, FuncWithReturnType, ...)                    \
    if (IsInSignallingThread()) {                                                                  \
        return FuncWithReturnType(__VA_ARGS__);                                                    \
    } else {                                                                                       \
        return WebRtcSignallingThread->Invoke<ReturnType>(                                         \
            RTC_FROM_HERE,                                                                         \
            [this, __VA_ARGS__]() -> ReturnType { return FuncWithReturnType(__VA_ARGS__); });      \
    }

#define SUBMIT_TASK_NO_PARAMS(Func)                                                                \
    if (IsInSignallingThread()) {                                                                  \
        Func();                                                                                    \
    } else {                                                                                       \
        WebRtcSignallingThread->PostTask(RTC_FROM_HERE, [this]() { Func(); });                     \
    }

#define SUBMIT_TASK_WITH_PARAMS(Func, ...)                                                         \
    if (IsInSignallingThread()) {                                                                  \
        Func(__VA_ARGS__);                                                                         \
    } else {                                                                                       \
        WebRtcSignallingThread->PostTask(RTC_FROM_HERE,                                            \
                                         [this, __VA_ARGS__]() { Func(__VA_ARGS__); });            \
    }

namespace OpenZI::CloudRender {
    // NOTE: All public methods should be one-liner functions that call one of the above macros to
    // ensure threadsafety of this class.

    FThreadSafePlayerSessions::FThreadSafePlayerSessions(rtc::Thread* InWebRtcSignallingThread)
        : WebRtcSignallingThread(InWebRtcSignallingThread) {}

    bool FThreadSafePlayerSessions::IsInSignallingThread() const {
        if (WebRtcSignallingThread == nullptr) {
            return false;
        }

        return WebRtcSignallingThread->IsCurrent();
    }

    int FThreadSafePlayerSessions::GetNumPlayers() const {
        SUBMIT_TASK_WITH_RETURN(int, GetNumPlayers_SignallingThread);
    }

    IPixelStreamingAudioSink*
    FThreadSafePlayerSessions::GetAudioSink(FPixelStreamingPlayerId PlayerId) const {
        SUBMIT_TASK_WITH_PARAMS_AND_RETURN(IPixelStreamingAudioSink*, GetAudioSink_SignallingThread,
                                           PlayerId)}

    IPixelStreamingAudioSink* FThreadSafePlayerSessions::GetUnlistenedAudioSink() const {
        SUBMIT_TASK_WITH_RETURN(IPixelStreamingAudioSink*, GetUnlistenedAudioSink_SignallingThread)
    }

    bool FThreadSafePlayerSessions::IsQualityController(FPixelStreamingPlayerId PlayerId) const {
        // Due to how many theads this particular method is called on we choose not to schedule
        // reading it as a new task and instead just made the the variable atomic. Additionally some
        // of the calling threads were deadlocking waiting for each other to finish other methods
        // while calling this method. Not scheduling this as a task is the exception to the general
        // rule of this class though.
        FScopeLock Lock(&QualityControllerCS);
        return QualityControllingPlayer == PlayerId;
    }

    void FThreadSafePlayerSessions::SetQualityController(FPixelStreamingPlayerId PlayerId) {
        {
            FScopeLock Lock(&QualityControllerCS);
            QualityControllingPlayer = PlayerId;
        }

        SUBMIT_TASK_WITH_PARAMS(SetQualityController_SignallingThread, PlayerId)
    }

    bool FThreadSafePlayerSessions::Send(FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type,
                                         const std::string& Descriptor) const {
        SUBMIT_TASK_WITH_PARAMS_AND_RETURN(bool, SendMessage_SignallingThread, PlayerId, Type,
                                           Descriptor)
    }

    void FThreadSafePlayerSessions::SendLatestQP(FPixelStreamingPlayerId PlayerId,
                                                 int LatestQP) const {
        SUBMIT_TASK_WITH_PARAMS(SendLatestQP_SignallingThread, PlayerId, LatestQP)
    }

    void FThreadSafePlayerSessions::SendFreezeFrameTo(FPixelStreamingPlayerId PlayerId,
                                                      const std::vector<uint8>& JpegBytes) const {
        SUBMIT_TASK_WITH_PARAMS(SendFreezeFrameTo_SignallingThread, PlayerId, JpegBytes)
    }

    void FThreadSafePlayerSessions::SendFreezeFrame(const std::vector<uint8>& JpegBytes) {
        SUBMIT_TASK_WITH_PARAMS(SendFreezeFrame_SignallingThread, JpegBytes)
    }

    void FThreadSafePlayerSessions::SendUnfreezeFrame() {
        SUBMIT_TASK_NO_PARAMS(SendUnfreezeFrame_SignallingThread)
    }

    void FThreadSafePlayerSessions::SendFileData(const std::vector<uint8>& ByteData,
                                                 const std::string& MimeType,
                                                 const std::string& FileExtension){
        SUBMIT_TASK_WITH_PARAMS(SendFileData_SignallingThread, ByteData, MimeType, FileExtension)}

    webrtc::PeerConnectionInterface* FThreadSafePlayerSessions::CreatePlayerSession(
        FPixelStreamingPlayerId PlayerId,
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory,
        webrtc::PeerConnectionInterface::RTCConfiguration PeerConnectionConfig,
        FSignallingServerConnection* SignallingServerConnection, int Flags) {
        SUBMIT_TASK_WITH_PARAMS_AND_RETURN(
            webrtc::PeerConnectionInterface*, CreatePlayerSession_SignallingThread, PlayerId,
            PeerConnectionFactory, PeerConnectionConfig, SignallingServerConnection, Flags)
    }

    void FThreadSafePlayerSessions::SetPlayerSessionDataChannel(
        FPixelStreamingPlayerId PlayerId,
        const rtc::scoped_refptr<webrtc::DataChannelInterface>& DataChannel) {
        SUBMIT_TASK_WITH_PARAMS(SetPlayerSessionDataChannel_SignallingThread, PlayerId, DataChannel)
    }

    void FThreadSafePlayerSessions::SetVideoSource(
        FPixelStreamingPlayerId PlayerId,
        const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>& VideoSource) {
        SUBMIT_TASK_WITH_PARAMS(SetVideoSource_SignallingThread, PlayerId, VideoSource)
    }

    void FThreadSafePlayerSessions::DeleteAllPlayerSessions() {
        SUBMIT_TASK_NO_PARAMS(DeleteAllPlayerSessions_SignallingThread)
    }

    int FThreadSafePlayerSessions::DeletePlayerSession(FPixelStreamingPlayerId PlayerId) {
        SUBMIT_TASK_WITH_PARAMS_AND_RETURN(int, DeletePlayerSession_SignallingThread, PlayerId)
    }

    void FThreadSafePlayerSessions::DisconnectPlayer(FPixelStreamingPlayerId PlayerId,
                                                     const std::string& Reason){
        SUBMIT_TASK_WITH_PARAMS(DisconnectPlayer_SignallingThread, PlayerId, Reason)}

    FDataChannelObserver* FThreadSafePlayerSessions::GetDataChannelObserver(
        FPixelStreamingPlayerId PlayerId) {
        SUBMIT_TASK_WITH_PARAMS_AND_RETURN(FDataChannelObserver*,
                                           GetDataChannelObserver_SignallingThread, PlayerId)
    }

    void FThreadSafePlayerSessions::SendMessageAll(EToPlayerMsg Type,
                                                   const std::string& Descriptor) const {
        SUBMIT_TASK_WITH_PARAMS(SendMessageAll_SignallingThread, Type, Descriptor)
    }

    void FThreadSafePlayerSessions::SendLatestQPAllPlayers(int LatestQP) const {
        SUBMIT_TASK_WITH_PARAMS(SendLatestQPAllPlayers_SignallingThread, LatestQP)
    }

    void FThreadSafePlayerSessions::OnRemoteIceCandidate(FPixelStreamingPlayerId PlayerId,
                                                         const std::string& SdpMid,
                                                         int SdpMLineIndex,
                                                         const std::string& Sdp) {
        SUBMIT_TASK_WITH_PARAMS(OnRemoteIceCandidate_SignallingThread, PlayerId, SdpMid,
                                SdpMLineIndex, Sdp)
    }

    void FThreadSafePlayerSessions::OnAnswer(FPixelStreamingPlayerId PlayerId, std::string Sdp) {
        SUBMIT_TASK_WITH_PARAMS(OnAnswer_SignallingThread, PlayerId, Sdp)
    }

    void FThreadSafePlayerSessions::PollWebRTCStats() const {
        SUBMIT_TASK_NO_PARAMS(PollWebRTCStats_SignallingThread)
    }

    /////////////////////////
    // Internal methods
    /////////////////////////

    void FThreadSafePlayerSessions::PollWebRTCStats_SignallingThread() const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        for (auto& Entry : Players) {
            FPlayerSession* Player = Entry.second;
            if (Player) {
                // Player->PollWebRTCStats();
            } else {
                ML_LOG(LogPixelStreaming, Log,
                       "Could not poll WebRTC stats for peer because peer was nullptr.");
            }
        }
    }

    void FThreadSafePlayerSessions::OnRemoteIceCandidate_SignallingThread(
        FPixelStreamingPlayerId PlayerId, const std::string& SdpMid, int SdpMLineIndex,
        const std::string& Sdp) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);
        if (Player) {
            Player->OnRemoteIceCandidate(SdpMid, SdpMLineIndex, Sdp);
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Could not pass remote ice candidate to player because Player %s no available.",
                   PlayerId.c_str());
        }
    }

    void FThreadSafePlayerSessions::OnAnswer_SignallingThread(FPixelStreamingPlayerId PlayerId,
                                                              std::string Sdp) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);
        if (Player) {
            Player->OnAnswer(Sdp);
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Could not pass answer to player because Player %s no available.",
                   PlayerId.c_str());
        }
    }

    IPixelStreamingAudioSink*
    FThreadSafePlayerSessions::GetUnlistenedAudioSink_SignallingThread() const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        for (auto& Entry : Players) {
            FPlayerSession* Session = Entry.second;
            FAudioSink& AudioSink = Session->GetAudioSink();
            if (!AudioSink.HasAudioConsumers()) {
                FAudioSink* AudioSinkPtr = &AudioSink;
                return static_cast<IPixelStreamingAudioSink*>(AudioSinkPtr);
            }
        }
        return nullptr;
    }

    IPixelStreamingAudioSink* FThreadSafePlayerSessions::GetAudioSink_SignallingThread(
        FPixelStreamingPlayerId PlayerId) const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* PlayerSession = GetPlayerSession_SignallingThread(PlayerId);

        if (PlayerSession) {
            FAudioSink* AudioSink = &PlayerSession->GetAudioSink();
            return static_cast<IPixelStreamingAudioSink*>(AudioSink);
        }
        return nullptr;
    }

    void FThreadSafePlayerSessions::SendLatestQPAllPlayers_SignallingThread(int LatestQP) const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        for (auto& PlayerEntry : Players) {
            // PlayerEntry.second->SendVideoEncoderQP(LatestQP);
        }
    }

    void FThreadSafePlayerSessions::SendLatestQP_SignallingThread(FPixelStreamingPlayerId PlayerId,
                                                                  int LatestQP) const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Session = GetPlayerSession_SignallingThread(PlayerId);
        if (Session) {
            // return Session->SendVideoEncoderQP(LatestQP);
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Could not send latest QP for PlayerId=%s because that player was not found.",
                   PlayerId.c_str());
        }
    }

    bool FThreadSafePlayerSessions::SendMessage_SignallingThread(
        FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type, const std::string& Descriptor) const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        ML_LOG(LogPixelStreaming, Log, "Send to: %s | Type: %d | Message: %s", PlayerId.c_str(),
               static_cast<int32>(Type), Descriptor.c_str());

        FPlayerSession* PlayerSession = GetPlayerSession_SignallingThread(PlayerId);

        if (PlayerSession) {
            return PlayerSession->Send(Type, Descriptor);
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Cannot send message to player: %s - player does not exist.", PlayerId.c_str());
            return false;
        }
    }

    void FThreadSafePlayerSessions::SendMessageAll_SignallingThread(
        EToPlayerMsg Type, const std::string& Descriptor) const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        ML_LOG(LogPixelStreaming, Log, "SendMessageAll: %d - %d", static_cast<int32>(Type),
               Descriptor.size());

        for (auto& PlayerEntry : Players) {
            PlayerEntry.second->Send(Type, Descriptor);
        }
    }

    FDataChannelObserver* FThreadSafePlayerSessions::GetDataChannelObserver_SignallingThread(
        FPixelStreamingPlayerId PlayerId) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);
        if (Player) {
            return &Player->GetDataChannelObserver();
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Cannot get data channel observer for player: %s - player does not exist.",
                   PlayerId.c_str());
            return nullptr;
        }
    }

    void
    FThreadSafePlayerSessions::DisconnectPlayer_SignallingThread(FPixelStreamingPlayerId PlayerId,
                                                                 const std::string& Reason) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);
        if (Player != nullptr) {
            Player->DisconnectPlayer(Reason);
        } else {
            ML_LOG(LogPixelStreaming, Log, "Cannot disconnect player: %s - player does not exist.",
                   PlayerId.c_str());
        }
    }

    int FThreadSafePlayerSessions::GetNumPlayers_SignallingThread() const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        // checkf(IsInSignallingThread(), TEXT("This method must be called on the signalling
        // thread."));

        return static_cast<int>(Players.size());
    }

    void FThreadSafePlayerSessions::SendFreezeFrame_SignallingThread(
        const std::vector<uint8>& JpegBytes) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        ML_LOG(LogPixelStreaming, Log, "Sending freeze frame to players: %d bytes",
               JpegBytes.size());
        {
            for (auto& PlayerEntry : Players) {
                PlayerEntry.second->SendFreezeFrame(JpegBytes);
            }
        }
    }

    void
    FThreadSafePlayerSessions::SendFileData_SignallingThread(const std::vector<uint8>& ByteData,
                                                             const std::string& MimeType,
                                                             const std::string& FileExtension) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        ML_LOG(LogPixelStreaming, Log, "Sending file with Mime type %s to all players: %d bytes",
               MimeType.c_str(), ByteData.size());
        {
            for (auto& PlayerEntry : Players) {
                // PlayerEntry.second->SendFileData(ByteData, MimeType, FileExtension);
            }
        }
    }

    void FThreadSafePlayerSessions::SendUnfreezeFrame_SignallingThread() {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        ML_LOG(LogPixelStreaming, Log, "Sending unfreeze message to players");

        for (auto& PlayerEntry : Players) {
            PlayerEntry.second->SendUnfreezeFrame();
        }
    }

    void FThreadSafePlayerSessions::SendFreezeFrameTo_SignallingThread(
        FPixelStreamingPlayerId PlayerId, const std::vector<uint8>& JpegBytes) const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);

        if (Player != nullptr) {
            Players.at(PlayerId)->SendFreezeFrame(JpegBytes);
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Cannot send freeze frame to player: %s - player does not exist.",
                   PlayerId.c_str());
        }
    }

    FPlayerSession* FThreadSafePlayerSessions::GetPlayerSession_SignallingThread(
        FPixelStreamingPlayerId PlayerId) const {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        if (Players.find(PlayerId) != Players.end()) {
            return Players.at(PlayerId);
        }
        return nullptr;
    }

    void FThreadSafePlayerSessions::SetPlayerSessionDataChannel_SignallingThread(
        FPixelStreamingPlayerId PlayerId,
        const rtc::scoped_refptr<webrtc::DataChannelInterface>& DataChannel) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);
        if (Player) {
            Player->SetDataChannel(DataChannel);
        }
    }

    void FThreadSafePlayerSessions::SetVideoSource_SignallingThread(
        FPixelStreamingPlayerId PlayerId,
        const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>& VideoSource) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);
        if (Player) {
            Player->SetVideoSource(VideoSource);
        }
    }

    void FThreadSafePlayerSessions::DeleteAllPlayerSessions_SignallingThread() {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        for (auto& Entry : Players) {
            FPixelStreamingPlayerId PlayerId = Entry.first;
            FPlayerSession* Player = Entry.second;
            delete Player;
            bool bWasQualityController = QualityControllingPlayer == PlayerId;

            PUBLISH_MESSAGE(OnClosedConnection, PlayerId, bWasQualityController);
            PUBLISH_MESSAGE(OnClosedConnectionNative, PlayerId, bWasQualityController);
        }

        Players.clear();
        QualityControllingPlayer = INVALID_PLAYER_ID;

        PUBLISH_EVENT(OnAllConnectionsClosed);
        PUBLISH_EVENT(OnAllConnectionsClosedNative);
    }

    int FThreadSafePlayerSessions::DeletePlayerSession_SignallingThread(
        FPixelStreamingPlayerId PlayerId) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }

        FPlayerSession* Player = GetPlayerSession_SignallingThread(PlayerId);
        if (!Player) {
            ML_LOG(LogPixelStreaming, Warning,
                   "Failed to delete player %s - that player was not found.", PlayerId.c_str());
            return GetNumPlayers_SignallingThread();
        }

        bool bWasQualityController = IsQualityController(PlayerId);

        // The actual modification to the players map
        Players.erase(PlayerId);
        delete Player;
        PUBLISH_MESSAGE(OnClosedConnection, PlayerId, bWasQualityController);
        PUBLISH_MESSAGE(OnClosedConnectionNative, PlayerId, bWasQualityController);

        // this is called from WebRTC signalling thread, the only thread were `Players` map is
        // modified, so no need to lock it
        if (Players.size() == 0) {
            // Inform the application-specific blueprint that nobody is viewing or
            // interacting with the app. This is an opportunity to reset the app.
            PUBLISH_EVENT(OnAllConnectionsClosed);
            PUBLISH_EVENT(OnAllConnectionsClosedNative);
        } else if (bWasQualityController) {
            // Quality Controller session has been just removed, set quality control to any of
            // remaining sessions
            for (auto& Entry : Players) {
                SetQualityController_SignallingThread(Entry.first);
                break;
            }
        }

        return GetNumPlayers_SignallingThread();
    }

    webrtc::PeerConnectionInterface*
    FThreadSafePlayerSessions::CreatePlayerSession_SignallingThread(
        FPixelStreamingPlayerId PlayerId,
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory,
        webrtc::PeerConnectionInterface::RTCConfiguration PeerConnectionConfig,
        FSignallingServerConnection* SignallingServerConnection, int Flags) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        // With unified plan, we get several calls to OnOffer, which in turn calls
        // this several times.
        // Therefore, we only try to create the player if not created already
        if (Players.find(PlayerId) != Players.end()) {
            return nullptr;
        }

        ML_LOG(LogPixelStreaming, Log, "Creating player session for PlayerId=%s", PlayerId.c_str());

        // this is called from WebRTC signalling thread, the only thread where `Players` map is
        // modified, so no need to lock it
        bool bMakeQualityController =
            PlayerId != SFU_PLAYER_ID &&
            Players.size() == 0; // first player controls quality by default
        FPlayerSession* Session = new FPlayerSession(this, SignallingServerConnection, PlayerId);

        rtc::scoped_refptr<webrtc::PeerConnectionInterface> PeerConnection =
            PeerConnectionFactory->CreatePeerConnection(
                PeerConnectionConfig, webrtc::PeerConnectionDependencies{Session});
        if (!PeerConnection) {
            ML_LOG(LogPixelStreaming, Error,
                   "Failed to created PeerConnection. This may indicate you passed malformed "
                   "peerConnectionOptions.");
            return nullptr;
        }

        // Setup suggested bitrate settings on the Peer Connection based on our CVars
        webrtc::BitrateSettings BitrateSettings;
        BitrateSettings.min_bitrate_bps = FAppConfig::Get().WebRTCMinBitrate;
        BitrateSettings.max_bitrate_bps = FAppConfig::Get().WebRTCMaxBitrate;
        BitrateSettings.start_bitrate_bps = FAppConfig::Get().WebRTCStartBitrate;
        PeerConnection->SetBitrate(BitrateSettings);

        Session->SetPeerConnection(PeerConnection);

        // The actual modification of the players map
        Players[PlayerId] = Session;

        if (bMakeQualityController) {
            SetQualityController_SignallingThread(PlayerId);
        }
        PUBLISH_MESSAGE(OnNewConnection, PlayerId, bMakeQualityController);
        PUBLISH_MESSAGE(OnNewConnectionNative, PlayerId, bMakeQualityController);

        return PeerConnection.get();
    }

    void FThreadSafePlayerSessions::SetQualityController_SignallingThread(
        FPixelStreamingPlayerId PlayerId) {
        if (!IsInSignallingThread()) {
            ML_LOG(ThreadSafePlayerSessions, Error,
                   "This method must be called on the signalling thread. %s", __LINE__);
        }
        // checkf(IsInSignallingThread(), TEXT("This method must be called on the signalling
        // thread."));
        if (Players.find(PlayerId) != Players.end()) {

            // The actual assignment of the quality controlling peeer
            {
                FScopeLock Lock(&QualityControllerCS);
                QualityControllingPlayer = PlayerId;
            }

            // Let any listeners know the quality controller has changed
            // PUBLISH_MESSAGE(OnQualityControllerChangedNative, PlayerId);
            ML_LOG(LogPixelStreaming, Log, "Quality controller is now PlayerId=%s.",
                   PlayerId.c_str());

            // Update quality controller status on the browser side too
            for (auto& Entry : Players) {
                FPlayerSession* Session = Entry.second;
                bool bIsQualityController = Entry.first == QualityControllingPlayer;
                if (Session) {
                    Session->SendQualityControlStatus(bIsQualityController);
                }
            }
        } else {
            ML_LOG(LogPixelStreaming, Log,
                   "Could not set quality controller for PlayerId=%s - that player does not exist.",
                   PlayerId.c_str());
        }
    }
} // namespace OpenZI::CloudRender

#undef SUBMIT_TASK_WITH_RETURN
#undef SUBMIT_TASK_WITH_PARAMS_AND_RETURN
#undef SUBMIT_TASK_NO_PARAMS
#undef SUBMIT_TASK_WITH_PARAMS