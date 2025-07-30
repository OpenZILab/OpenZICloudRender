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
#include <string>

namespace OpenZI::CloudRender
{
    using FPixelStreamingPlayerId = std::string;

    inline FPixelStreamingPlayerId ToPlayerId(std::string PlayerIdString)
    {
        return FPixelStreamingPlayerId(PlayerIdString);
    }

    inline FPixelStreamingPlayerId ToPlayerId(int32_t PlayerIdInteger)
    {
        return std::to_string(PlayerIdInteger);
    }

    inline int32_t PlayerIdToInt(FPixelStreamingPlayerId PlayerId)
    {
        return std::stoi(PlayerId);
    }

    static const FPixelStreamingPlayerId INVALID_PLAYER_ID = ToPlayerId("Invalid Player Id");
    static const FPixelStreamingPlayerId SFU_PLAYER_ID = FPixelStreamingPlayerId("1");
} // namespace OpenZI::CloudRender