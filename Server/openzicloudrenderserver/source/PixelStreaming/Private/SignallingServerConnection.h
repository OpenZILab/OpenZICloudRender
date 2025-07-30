/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3, (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.gnu.org/licenses/agpl-3.0.en.html#license-text
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/

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