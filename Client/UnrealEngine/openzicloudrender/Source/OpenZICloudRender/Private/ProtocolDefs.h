/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/
#pragma once

#include "HAL/Platform.h"

namespace OpenZI::CloudRender {
    namespace Protocol {
        enum EPlayerFlags {
            PSPFlag_None = 0x00,
            PSPFlag_SupportsDataChannel = 0x01,
            PSPFlag_IsSFU = 0x02,
        };

        enum class EToStreamerMsg : uint8 {
            /**********************************************************************/

            /*
             * Control Messages. Range = 0..49.
             */

            IFrameRequest = 0,
            RequestQualityControl = 1,
            FpsRequest = 2,
            AverageBitrateRequest = 3,
            StartStreaming = 4,
            StopStreaming = 5,
            LatencyTest = 6,
            RequestInitialSettings = 7,

            /**********************************************************************/

            /*
             * Input Messages. Range = 50..89.
             */

            // Generic Input Messages. Range = 50..59.
            UIInteraction = 50,
            Command = 51,

            // Keyboard Input Message. Range = 60..69.
            KeyDown = 60,
            KeyUp = 61,
            KeyPress = 62,

            // Mouse Input Messages. Range = 70..79.
            MouseEnter = 70,
            MouseLeave = 71,
            MouseDown = 72,
            MouseUp = 73,
            MouseMove = 74,
            MouseWheel = 75,

            // Touch Input Messages. Range = 80..89.
            TouchStart = 80,
            TouchEnd = 81,
            TouchMove = 82,

            // Gamepad Input Messages. Range = 90..99
            GamepadButtonPressed = 90,
            GamepadButtonReleased = 91,
            GamepadAnalog = 92,

            /**********************************************************************/

            /*
             * Ensure Count is the final entry.
             */
            Count

            /**********************************************************************/
        };

        //! Messages that can be sent to the webrtc players
        enum class EToPlayerMsg : uint8 {
            QualityControlOwnership = 0,
            Response = 1,
            Command = 2,
            FreezeFrame = 3,
            UnfreezeFrame = 4,
            VideoEncoderAvgQP = 5, // average Quantisation Parameter value of Video Encoder, roughly depicts video encoding quality
            LatencyTest = 6,
            InitialSettings = 7,
            FileExtension = 8,
            FileMimeType = 9,
            FileContents = 10
        };

        template <typename T>
        static const T& ParseBuffer(const uint8*& Data, uint32& Size) {
            checkf(sizeof(T) <= Size, TEXT("%d - %d"), sizeof(T), Size);
            const T& Value = *reinterpret_cast<const T*>(Data);
            Data += sizeof(T);
            Size -= sizeof(T);
            return Value;
        }

        static FString ParseString(const uint8* Data, const size_t Size, const size_t Offset = 0) {
            FString Res;
            if (Size > Offset) {
                size_t StringLength = (Size - Offset) / sizeof(TCHAR);
                Res.GetCharArray().SetNumUninitialized(StringLength + 1);
                FMemory::Memcpy(Res.GetCharArray().GetData(), Data + Offset, StringLength * sizeof(TCHAR));
                Res.GetCharArray()[StringLength] = 0;
            }
            return Res;
        }

    }; // namespace Protocol
} // namespace OpenZI::CloudRender