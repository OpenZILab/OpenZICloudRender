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
#include "WebRTCIncludes.h"
#include "VideoEncoder.h"
#include "CoreMinimal.h"
#include <memory>


namespace OpenZI::CloudRender
{
    class FVideoEncoderFactory;

    class FVideoEncoderH264Wrapper
    {
    public:
        FVideoEncoderH264Wrapper(uint64 EncoderId, std::unique_ptr<FVideoEncoder> Encoder);
        ~FVideoEncoderH264Wrapper();

        uint64 GetId() const { return Id; }

        void SetForceNextKeyframe() { bForceNextKeyframe = true; }

        void Encode(const webrtc::VideoFrame &WebRTCFrame, bool bKeyframe);

        static void OnEncodedPacket(uint64 SourceEncoderId,
                                    FVideoEncoderFactory *Factory,
                                    uint32 InLayerIndex, std::vector<uint8_t>& Buffer,
                                    const FBufferInfo BufferInfo
                                    );

    private:
        uint64 Id;
        std::unique_ptr<FVideoEncoder> Encoder;
        bool bForceNextKeyframe = false;
        FBufferInfo BufferInfo;
    };
} // namespace OpenZI::CloudRender