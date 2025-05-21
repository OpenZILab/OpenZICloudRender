//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/01 16:21
//
#include "SignallingServerConnection.h"
#include "Config.h"
#include "Logger/CommandLogger.h"
#include "MessageCenter.h"
#include "ProtocolDefs.h"
#include "SystemTime.h"
#include "WebSocketClientImplCurl.h"
using namespace ws;

namespace OpenZI::CloudRender {

    class FWebSocketClient : public WebSocketClientImplCurl {
    public:
        FWebSocketClient(FSignallingServerConnection* SignallingServerConnection)
            : WebSocketClientImplCurl(), Connection(SignallingServerConnection) {}
        virtual void OnConnect(ConnectResult result) override {
            if (result == ws::WebSocketClientImplCurl::Success) {
                // Message msg(ws::Text, message, strlen(message));

                // Send(msg);
                Connection->OnConnected();
            } else {
                // printf("Websocket connect failed.\n");
                std::string Error = "Websocket connect failed.";
                Connection->OnConnectionError(Error);
            }
        }
        virtual void OnRecv(Message msg, bool fin) override {
            std::string Message = std::string(msg.data, msg.len);
            Connection->OnMessage(Message);
        }
        // TODO: 新增一个OnClosed的处理函数
    private:
        std::shared_ptr<FSignallingServerConnection> Connection;
    };

    FSignallingServerConnection::FSignallingServerConnection(
        FSignallingServerConnectionObserver& InObserver, const std::string& InStreamerId)
        : Observer(InObserver), StreamerId(InStreamerId), bPingThreadExiting(false) {}

    void FSignallingServerConnection::Connect(const std::string& Url) {
        // Already have a websocket connection, no need to make another one
        if (!WS) {
            WS = std::make_shared<FWebSocketClient>(this);
        }
        if (WS->GetState() == ws::WebSocketClientImplCurl::State::Disconnected)
            WS->Connect(Url);
    }

    void FSignallingServerConnection::Disconnect() {
        StopPingThread();
        if (WS) {
            WS->Close();
        }
    }

    FSignallingServerConnection::~FSignallingServerConnection() {
        StopPingThread();
        Disconnect();
        WS = nullptr;
    }

    void FSignallingServerConnection::SendOffer(FPixelStreamingPlayerId PlayerId,
                                                const webrtc::SessionDescriptionInterface& SDP) {
        FJsonObject OfferJson;
        OfferJson["type"] = FJsonValue("offer");
        SetPlayerIdJson(OfferJson, PlayerId);

        std::string SdpStr;
        SDP.ToString(&SdpStr);
        OfferJson["sdp"] = FJsonValue(SdpStr);

        ML_LOG(LogPixelStreamingSS, Verbose, "-> SS: offer\n%s", SdpStr.c_str());
        WS->Send(Message(FJsonValue(OfferJson).serialize(), FrameType::Binary));
    }

    void FSignallingServerConnection::SendAnswer(FPixelStreamingPlayerId PlayerId,
                                                 const webrtc::SessionDescriptionInterface& SDP) {
        FJsonObject AnswerJson;
        AnswerJson["type"] = FJsonValue("answer");
        SetPlayerIdJson(AnswerJson, PlayerId);

        std::string SdpStr;
        SDP.ToString(&SdpStr);
        AnswerJson["sdp"] = FJsonValue(SdpStr);

        // ML_LOG(LogPixelStreamingSS, Verbose, "-> SS: answer\n%s", SdpStr.c_str());
        WS->Send(Message(FJsonValue(AnswerJson).serialize(), FrameType::Binary));
    }

    void FSignallingServerConnection::SetPlayerIdJson(FJsonObject& Json,
                                                      FPixelStreamingPlayerId PlayerId) {
        // bool bSendAsInteger = Settings::CVarSendPlayerIdAsInteger.GetValueOnAnyThread();
        bool bSendAsInteger = FAppConfig::Get().SendPlayerIdAsInteger;
        if (bSendAsInteger) {
            int32 PlayerIdAsInt = PlayerIdToInt(PlayerId);
            Json["playerId"] = FJsonValue(static_cast<int64_t>(PlayerIdAsInt));
        } else {
            Json["playerId"] = FJsonValue(PlayerId);
        }
    }

    bool FSignallingServerConnection::GetPlayerIdJson(const FJsonValue& Json,
                                                      FPixelStreamingPlayerId& OutPlayerId) {
        // bool bSendAsInteger = Settings::CVarSendPlayerIdAsInteger.GetValueOnAnyThread();
        // bool bSendAsInteger = FAppConfig::Get().SendPlayerIdAsInteger;
        // if (bSendAsInteger)
        // {
        uint32 PlayerIdInt;
        if (Json.get("playerId").is<int64_t>()) {
            PlayerIdInt = static_cast<uint32>(Json.get("playerId").get<int64_t>());
            OutPlayerId = ToPlayerId(PlayerIdInt);
            return true;
        }
        // }
        else if (Json.get("playerId").is<std::string>()) {
            OutPlayerId = Json.get("playerId").get<std::string>();
            return true;
        }

        // ML_LOG(LogPixelStreamingSS, Error, "Failed to extracted player id offer json: %s",
        //        Json.serialize().c_str());
        return false;
    }

    void FSignallingServerConnection::SendIceCandidate(
        const webrtc::IceCandidateInterface& IceCandidate) {
        FJsonObject IceCandidateJson;

        IceCandidateJson["type"] = FJsonValue("iceCandidate");

        FJsonObject CandidateJson;
        CandidateJson["sdpMid"] = FJsonValue(IceCandidate.sdp_mid().c_str());
        CandidateJson["sdpMLineIndex"] =
            FJsonValue(static_cast<double>(IceCandidate.sdp_mline_index()));

        std::string CandidateStr;
        IceCandidate.ToString(&CandidateStr);
        CandidateJson["candidate"] = FJsonValue(CandidateStr);

        IceCandidateJson["candidate"] = FJsonValue(CandidateJson);

        // ML_LOG(LogPixelStreamingSS, Verbose, "-> SS: ice-candidate\n%s",
        //        FJsonValue(IceCandidateJson).serialize().c_str());
        WS->Send(Message(FJsonValue(IceCandidateJson).serialize(), FrameType::Binary));
    }

    void FSignallingServerConnection::SendIceCandidate(
        FPixelStreamingPlayerId PlayerId, const webrtc::IceCandidateInterface& IceCandidate) {
        FJsonObject IceCandidateJson;

        IceCandidateJson["type"] = FJsonValue("iceCandidate");
        SetPlayerIdJson(IceCandidateJson, PlayerId);

        FJsonObject CandidateJson;
        CandidateJson["sdpMid"] = FJsonValue(IceCandidate.sdp_mid().c_str());
        CandidateJson["sdpMLineIndex"] =
            FJsonValue(static_cast<double>(IceCandidate.sdp_mline_index()));

        std::string CandidateStr;
        IceCandidate.ToString(&CandidateStr);
        CandidateJson["candidate"] = FJsonValue(CandidateStr);

        IceCandidateJson["candidate"] = FJsonValue(CandidateJson);

        // ML_LOG(LogPixelStreamingSS, Verbose, "-> SS: iceCandidate\n%s",
        //        FJsonValue(IceCandidateJson).serialize().c_str());
        WS->Send(Message(FJsonValue(IceCandidateJson).serialize(), FrameType::Binary));
    }

    void FSignallingServerConnection::KeepAlive() {
        const int PingInterval = 60;
        int PingCounter = 0;
        while (!bPingThreadExiting) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (++PingCounter >= PingInterval) {
                PingCounter = 0;
                FJsonObject Json;
                // TODO: uint64强转double，不晓得会不会有异常
                double unixTime = static_cast<double>(GetTimestampUs());
                Json["type"] = FJsonValue("ping");
                Json["time"] = FJsonValue(unixTime);
                std::string Msg = FJsonValue(Json).serialize();
                if (WS != nullptr && WS->GetState() == WebSocketClientImplCurl::State::Connected) {
                    WS->Send(Message(Msg, FrameType::Binary));
                }
            }
        }
    }

    void FSignallingServerConnection::SendDisconnectPlayer(FPixelStreamingPlayerId PlayerId,
                                                           const std::string& Reason) {
        FJsonObject Json;

        Json["type"] = FJsonValue("disconnectPlayer");
        SetPlayerIdJson(Json, PlayerId);
        Json["reason"] = FJsonValue(Reason);

        std::string Msg = FJsonValue(Json).serialize();
        // ML_LOG(LogPixelStreamingSS, Verbose, "-> SS: iceCandidate\n%s", Msg.c_str());
        WS->Send(Message(Msg, FrameType::Binary));
    }

    void FSignallingServerConnection::OnConnected() {
        ML_LOG(LogPixelStreamingSS, Log, "Connected to SS");

        // Send message to keep connection alive every 60 seconds
        PingThread = new std::thread(&FSignallingServerConnection::KeepAlive, this);
        PUBLISH_EVENT(OnConnectedToSignallingServer);
    }
    void FSignallingServerConnection::StopPingThread() {
        if (PingThread) {
            bPingThreadExiting = true;
            PingThread->join();
            delete PingThread;
            PingThread = nullptr;
        }
    }
    void FSignallingServerConnection::OnConnectionError(const std::string& Error) {
        ML_LOG(LogPixelStreamingSS, Error, "Failed to connect to SS: %s", Error.c_str());
        Observer.OnSignallingServerDisconnected();
        // StopPingThread();
        PUBLISH_EVENT(OnDisconnectedFromSignallingServer);
    }

    void FSignallingServerConnection::OnClosed(int32 StatusCode, const std::string& Reason,
                                               bool bWasClean) {
        ML_LOG(LogPixelStreamingSS, Log,
               "Connection to SS closed: \n\tstatus %d\n\treason: %s\n\twas clean: %s", StatusCode,
               Reason.c_str(), bWasClean ? "true" : "false");
        Observer.OnSignallingServerDisconnected();
        StopPingThread();
        PUBLISH_EVENT(OnDisconnectedFromSignallingServer);
    }

    void FSignallingServerConnection::OnMessage(const std::string& Msg) {
        // ML_LOG(LogPixelStreamingSS, Log, "<- SS: %s", Msg.c_str());

        FJsonValue JsonMsg;
        std::string err = picojson::parse(JsonMsg, Msg);
        if (!err.empty()) {
            // ML_LOG(SignallingServerConnection, Error, "Failed to parse SS message");
            // HANDLE_SS_ERROR(TEXT("Failed to parse SS message:\n%s"), *Msg);
        }

        std::string MsgType;
        if (!JsonMsg.contains("type")) {
            // ML_LOG(SignallingServerConnection, Error, "Cannot find `type` field in SS message");
        } else {
            MsgType = JsonMsg.get("type").get<std::string>();
        }

        if (MsgType == "identify") {
            // ML_LOG(SignallingServerConnection, Log, "Type==identify in SS message");
            OnIdRequested();
        } else if (MsgType == "config") {
            // ML_LOG(SignallingServerConnection, Log, "Type==identify in SS message");
            OnConfig(JsonMsg);
        } else if (MsgType == "offer" || MsgType == "answer") {
            // ML_LOG(SignallingServerConnection, Log, "Type==%s in SS message", MsgType.c_str());
            OnSessionDescription(JsonMsg);
        } else if (MsgType == "iceCandidate") {
            // ML_LOG(SignallingServerConnection, Log, "Type==iceCandidate in SS message");
            if (JsonMsg.contains("playerId")) {
                OnPlayerIceCandidate(JsonMsg);
            } else {
                OnStreamerIceCandidate(JsonMsg);
            }
        } else if (MsgType == "playerCount") {
            // ML_LOG(SignallingServerConnection, Log, "Type==playerCount in SS message");
            OnPlayerCount(JsonMsg);
        } else if (MsgType == "playerConnected") {
            // ML_LOG(SignallingServerConnection, Log, "Type==playerConnected in SS message");
            OnPlayerConnected(JsonMsg);
        } else if (MsgType == "playerDisconnected") {
            // ML_LOG(SignallingServerConnection, Log, "Type==playerDisconnected in SS message");
            OnPlayerDisconnected(JsonMsg);
        } else if (MsgType == "pong") {
            // Do nothing, this is a keep alive message
        } else {
            ML_LOG(LogPixelStreamingSS, Error, "Unsupported message `%s` received from SS",
                   MsgType.c_str());
            MsgType = std::string("Unsupported message received: ") + MsgType;
            // WS->end(4001, MsgType);
        }
    }

    void FSignallingServerConnection::OnIdRequested() {
        FJsonObject Json;

        Json["type"] = FJsonValue("endpointId");
        Json["id"] = FJsonValue(StreamerId);

        std::string Msg = FJsonValue(Json).serialize();
        // ML_LOG(LogPixelStreamingSS, Verbose, "-> SS: endpointId\n%s", Msg.c_str());

        WS->Send(Message(Msg, FrameType::Binary));
    }

    void FSignallingServerConnection::OnConfig(const FJsonValue& Json) {
        // SS sends `config` that looks like:
        // `{peerConnectionOptions: { 'iceServers': [{'urls': ['stun:34.250.222.95:19302',
        // 'turn:34.250.222.95:19303']}] }}` where `peerConnectionOptions` is `RTCConfiguration`
        // (except in native `RTCConfiguration` "iceServers" = "servers"). As `RTCConfiguration`
        // doesn't implement parsing from a string (or `ToString` method), we just get `stun`/`turn`
        // URLs from it and ignore other options

        FJsonValue PeerConnectionOptions;

        if (!Json.contains("peerConnectionOptions") ||
            !Json.get("peerConnectionOptions").is<FJsonObject>()) {
            // HANDLE_SS_ERROR(TEXT("Cannot find `peerConnectionOptions` field in SS config\n%s"),
            // *ToString(Json));
        } else {
            PeerConnectionOptions = Json.get("peerConnectionOptions");
        }

        webrtc::PeerConnectionInterface::RTCConfiguration RTCConfig;

        FJsonArray IceServers;
        if (PeerConnectionOptions.contains("iceServers") &&
            PeerConnectionOptions.get("iceServers").is<FJsonArray>()) {
            IceServers = PeerConnectionOptions.get("iceServers").get<FJsonArray>();
            for (const auto& IceServerJson : IceServers) {

                RTCConfig.servers.push_back(webrtc::PeerConnectionInterface::IceServer{});
                webrtc::PeerConnectionInterface::IceServer& IceServer = RTCConfig.servers.back();

                // std::vector<std::string> Urls;
                if (IceServerJson.contains("urls") && IceServerJson.get("urls").is<FJsonArray>()) {
                    FJsonArray Urls = IceServerJson.get("urls").get<FJsonArray>();
                    for (const auto& Url : Urls) {
                        IceServer.urls.push_back(Url.get<std::string>());
                    }
                } else if (IceServerJson.contains("urls") &&
                           IceServerJson.get("urls").is<std::string>()) {
                    // in the RTC Spec, "urls" can be an array or a single string
                    // https://www.w3.org/TR/webrtc/#dictionary-rtciceserver-members
                    IceServer.urls.push_back(IceServerJson.get("urls").get<std::string>());
                }

                if (IceServerJson.contains("username")) {
                    IceServer.username = IceServerJson.get("username").get<std::string>();
                }

                if (IceServerJson.contains("credential")) {
                    IceServer.password = IceServerJson.get("credential").get<std::string>();
                }
            }
        }

        // force `UnifiedPlan` as we control both ends of WebRTC streaming
        RTCConfig.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

        Observer.OnConfig(RTCConfig);
    }

    void FSignallingServerConnection::OnSessionDescription(const FJsonValue& Json) {
        webrtc::SdpType Type = Json.get("type").get<std::string>() == "offer"
                                   ? webrtc::SdpType::kOffer
                                   : webrtc::SdpType::kAnswer;

        std::string Sdp;
        if (!Json.contains("sdp")) {
            ML_LOG(SignallingServerConnection, Error, "Cannot find `sdp` in Streamer's answer\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Cannot find `sdp` in Streamer's answer\n%s"), *ToString(Json));
        } else {
            Sdp = Json.get("sdp").get<std::string>();
        }

        FPixelStreamingPlayerId PlayerId;
        bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
        if (!bGotPlayerId) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `playerId` from `offer/answer` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `playerId` from `offer/answer` message\n%s"),
            // *ToString(Json));
        }

        Observer.OnSessionDescription(PlayerId, Type, Sdp);
    }

    void FSignallingServerConnection::OnStreamerIceCandidate(const FJsonValue& Json) {
        FJsonValue CandidateJson;
        if (!Json.contains("candidate")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `candiate` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `candiate` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            CandidateJson = Json.get("candidate");
        }

        std::string SdpMid;
        if (!CandidateJson.contains("sdpMid")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `sdpMid` from remote `iceCandidate` message");
            // HANDLE_SS_ERROR(TEXT("Failed to get `sdpMid` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            SdpMid = CandidateJson.get("sdpMid").get<std::string>();
        }

        int32 SdpMLineIndex = 0;
        if (!CandidateJson.contains("sdpMLineIndex")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `sdpMLineIndex` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `sdpMLineIndex` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            SdpMLineIndex = (int32)CandidateJson.get("sdpMLineIndex").get<int64_t>();
        }

        std::string CandidateStr;
        if (!CandidateJson.contains("candidate")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `candidate` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `candidate` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            CandidateStr = CandidateJson.get("candidate").get<std::string>();
        }

        webrtc::SdpParseError Error;
        std::unique_ptr<webrtc::IceCandidateInterface> Candidate(
            webrtc::CreateIceCandidate(SdpMid, SdpMLineIndex, CandidateStr, &Error));
        if (!Candidate) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to parse remote `iceCandidate` message\n%s", Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to parse remote `iceCandidate` message\n%s"),
            // *ToString(Json));
        }

        Observer.OnRemoteIceCandidate(
            std::unique_ptr<webrtc::IceCandidateInterface>{Candidate.release()});
    }

    void FSignallingServerConnection::OnPlayerIceCandidate(const FJsonValue& Json) {
        FPixelStreamingPlayerId PlayerId;
        bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
        if (!bGotPlayerId) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `playerId` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_PLAYER_SS_ERROR(PlayerId, TEXT("Failed to get `playerId` from remote
            // `iceCandidate` message\n%s"), *ToString(Json));
        }

        FJsonValue CandidateJson;
        if (!Json.contains("candidate")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `candiate` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `candiate` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            CandidateJson = Json.get("candidate");
        }

        std::string SdpMid;
        if (!CandidateJson.contains("sdpMid")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `sdpMid` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `sdpMid` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            SdpMid = CandidateJson.get("sdpMid").get<std::string>();
        }

        int32 SdpMLineIndex = 0;
        if (!CandidateJson.contains("sdpMLineIndex")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `sdpMLineIndex` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `sdpMLineIndex` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            SdpMLineIndex = (int32)CandidateJson.get("sdpMLineIndex").get<int64_t>();
        }

        std::string CandidateStr;
        if (!CandidateJson.contains("candidate")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `candidate` from remote `iceCandidate` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `candidate` from remote `iceCandidate`
            // message\n%s"), *ToString(Json));
        } else {
            CandidateStr = CandidateJson.get("candidate").get<std::string>();
        }

        Observer.OnRemoteIceCandidate(PlayerId, SdpMid, SdpMLineIndex, CandidateStr);
    }

    void FSignallingServerConnection::OnPlayerCount(const FJsonValue& Json) {
        uint32 Count;
        if (!Json.contains("count")) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `count` from `playerCount` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `count` from `playerCount` message\n%s"),
            // *ToString(Json));
        } else {
            Count = (uint32)Json.get("count").get<int64_t>();
        }

        Observer.OnPlayerCount(Count);
    }

    void FSignallingServerConnection::OnPlayerConnected(const FJsonValue& Json) {
        FPixelStreamingPlayerId PlayerId;
        bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
        if (!bGotPlayerId) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `playerId` from `join` message\n%s", Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `playerId` from `join` message\n%s"),
            // *ToString(Json));
        }
        int Flags = 0;

        // Default to always making datachannel, unless explicitly set to false.
        bool bMakeDataChannel = true;
        bMakeDataChannel = Json.get("dataChannel").get<bool>();

        // Default peer is not an SFU, unless explictly set as SFU
        bool bIsSFU = false;
        bIsSFU = Json.get("sfu").get<bool>();

        Flags |= bMakeDataChannel ? EPlayerFlags::PSPFlag_SupportsDataChannel : 0;
        Flags |= bIsSFU ? EPlayerFlags::PSPFlag_IsSFU : 0;
        Observer.OnPlayerConnected(PlayerId, Flags);
    }

    void FSignallingServerConnection::OnPlayerDisconnected(const FJsonValue& Json) {
        FPixelStreamingPlayerId PlayerId;
        bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
        if (!bGotPlayerId) {
            ML_LOG(SignallingServerConnection, Error,
                   "Failed to get `playerId` from `playerDisconnected` message\n%s",
                   Json.serialize().c_str());
            // HANDLE_SS_ERROR(TEXT("Failed to get `playerId` from `playerDisconnected`
            // message\n%s"), *ToString(Json));
        }

        Observer.OnPlayerDisconnected(PlayerId);
    }
} // namespace OpenZI::CloudRender