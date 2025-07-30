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

#include "IPixelStreamingAudioSink.h"
#include "PixelStreamingPlayerId.h"
#include "ProtocolDefs.h"
#include <string>
#include <vector>

namespace OpenZI::CloudRender
{
    // `FPlayerSession` is only directly accessible to `FThreadSafePlayerSessions` as it completely owns the lifecycle of those objects.
    // This is nice from a safety point of view; however, some other objects in Pixel Streaming need to interface with `FPlayerSession`.
    // By using `FPixelStreamingPlayerId` users can use this `IPixelStreamingSessions` interface to perform a limited set of allowable
    // actions on `FPlayerSession` objects.
    class IPixelStreamingSessions
    {
    public:
        virtual int GetNumPlayers() const = 0;
        virtual IPixelStreamingAudioSink *GetAudioSink(FPixelStreamingPlayerId PlayerId) const = 0;
        virtual IPixelStreamingAudioSink *GetUnlistenedAudioSink() const = 0;
        virtual bool IsQualityController(FPixelStreamingPlayerId PlayerId) const = 0;
        virtual void SetQualityController(FPixelStreamingPlayerId PlayerId) = 0;
        virtual bool Send(FPixelStreamingPlayerId PlayerId, EToPlayerMsg Type, const std::string &Descriptor) const = 0;
        virtual void SendLatestQP(FPixelStreamingPlayerId PlayerId, int LatestQP) const = 0;
        virtual void SendFreezeFrameTo(FPixelStreamingPlayerId PlayerId, const std::vector<uint8> &JpegBytes) const = 0;
        virtual void PollWebRTCStats() const = 0;
    };
} // namespace OpenZI::CloudRender