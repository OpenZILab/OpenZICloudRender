// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/01 16:08
// 
#pragma once

#include "CoreMinimal.h"
#include "WebRTCIncludes.h"
#include "PicoJson.h"
#include "PixelStreamingPlayerId.h"
#include <functional>
#include <memory>
#include <string>
#include <map>
#include <thread>

namespace OpenZI::CloudRender
{
    class FWebSocketClient;
    // callback interface for `FSignallingServerConnection`
    class FSignallingServerConnectionObserver
    {
    public:
        virtual ~FSignallingServerConnectionObserver()
        {
        }

        virtual void OnSignallingServerDisconnected() = 0;
        virtual void OnConfig(const webrtc::PeerConnectionInterface::RTCConfiguration &Config) = 0;
        virtual void OnSessionDescription(FPixelStreamingPlayerId PlayerId, webrtc::SdpType Type, const std::string &Sdp)
        {
            
        }

        // Streamer-only
        virtual void OnRemoteIceCandidate(FPixelStreamingPlayerId PlayerId, const std::string &SdpMid, int SdpMLineIndex, const std::string &Sdp)
        {
            
        }
        virtual void OnPlayerConnected(FPixelStreamingPlayerId PlayerId, int Flags)
        {
            
        }
        virtual void OnPlayerDisconnected(FPixelStreamingPlayerId PlayerId)
        {
            
        }

        // Player-only
        virtual void OnRemoteIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> Candidate)
        {
            
        }
        virtual void OnPlayerCount(uint32 Count)
        {
            
        }
    };
    
    class FSignallingServerConnection final
    {
    public:
        explicit FSignallingServerConnection(FSignallingServerConnectionObserver &Observer, const std::string &StreamerId);
        ~FSignallingServerConnection();
        void Connect(const std::string &Url);
        void Disconnect();

        void SendOffer(FPixelStreamingPlayerId PlayerId, const webrtc::SessionDescriptionInterface &SDP);
        void SendAnswer(FPixelStreamingPlayerId PlayerId, const webrtc::SessionDescriptionInterface &SDP);
        void SendIceCandidate(const webrtc::IceCandidateInterface &IceCandidate);
        void SendIceCandidate(FPixelStreamingPlayerId PlayerId, const webrtc::IceCandidateInterface &IceCandidate);
        void SendDisconnectPlayer(FPixelStreamingPlayerId PlayerId, const std::string &Reason);

    private:
        void KeepAlive();

        void OnConnected();
        void OnConnectionError(const std::string &Error);
        void OnClosed(int32 StatusCode, const std::string &Reason, bool bWasClean);
        void OnMessage(const std::string &Msg);

        using FJsonObject = picojson::object;
        using FJsonValue = picojson::value;
        using FJsonArray = picojson::array;

        void OnIdRequested();
        void OnConfig(const FJsonValue &Json);
        void OnSessionDescription(const FJsonValue &Json);
        void OnStreamerIceCandidate(const FJsonValue &Json);
        void OnPlayerIceCandidate(const FJsonValue &Json);
        void OnPlayerCount(const FJsonValue &Json);
        void OnPlayerConnected(const FJsonValue &Json);
        void OnPlayerDisconnected(const FJsonValue &Json);
        void SetPlayerIdJson(FJsonObject &Json, FPixelStreamingPlayerId PlayerId);
        bool GetPlayerIdJson(const FJsonValue &Json, FPixelStreamingPlayerId &OutPlayerId);
        void StopPingThread();
    private:
        FSignallingServerConnectionObserver &Observer;
        std::string StreamerId;
        std::thread *PingThread;
        std::atomic<bool> bPingThreadExiting;

        // FWebSocketClient* WS = nullptr;
        std::shared_ptr<FWebSocketClient> WS;
        friend class FWebSocketClient;
    };
} // namespace OpenZI::CloudRender