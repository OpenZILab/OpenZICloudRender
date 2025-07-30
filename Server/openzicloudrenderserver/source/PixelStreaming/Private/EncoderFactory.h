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
#include "Thread/CriticalSection.h"
#include <optional>
#include <map>

namespace OpenZI::CloudRender
{
    class FVideoEncoderRTC;
    class FVideoEncoderH264Wrapper;

    class FVideoEncoderFactory : public webrtc::VideoEncoderFactory
    {
    public:
        using FHardwareEncoderId = uint64;
        FVideoEncoderFactory();
        virtual ~FVideoEncoderFactory() override;

        virtual std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

        // Always returns our H264 hardware encoder for now.
        virtual std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat &format) override;

        void ReleaseVideoEncoder(FVideoEncoderRTC* Encoder);
        void ForceKeyFrame(bool bLock = true);

        
        std::optional<FHardwareEncoderId> GetOrCreateHardwareEncoder(int Width, int Height, int MaxBitrate, int TargetBitrate, int MaxFramerate);
        std::shared_ptr<FVideoEncoderH264Wrapper> GetHardwareEncoder(FHardwareEncoderId Id);

        void OnEncodedImage(FHardwareEncoderId SourceEncoderId, const webrtc::EncodedImage &Encoded_image, const webrtc::CodecSpecificInfo *CodecSpecificInfo);

    private:
        static webrtc::SdpVideoFormat CreateH264Format(webrtc::H264Profile Profile, webrtc::H264Level Level);

        // These are the actual hardware encoders serving multiple pixelstreaming encoders
        std::map<FHardwareEncoderId, std::shared_ptr<FVideoEncoderH264Wrapper>> HardwareEncoders;
        FCriticalSection HardwareEncodersGuard;

        // Encoders assigned to each peer. Each one of these will be assigned to one of the hardware encoders
        std::vector<FVideoEncoderRTC*> ActiveEncoders;
        FCriticalSection ActiveEncodersGuard;
    };

    class FSimulcastEncoderFactory : public webrtc::VideoEncoderFactory
    {
    public:
        FSimulcastEncoderFactory();
        virtual ~FSimulcastEncoderFactory();
        virtual std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const;
        virtual std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat &format);

        FVideoEncoderFactory *GetRealFactory() const;

    private:
        // making this a unique ptr is a little cheeky since we pass around raw pointers to it
        // but the sole ownership lies in this class
        std::unique_ptr<FVideoEncoderFactory> RealFactory;
    };
} // namespace OpenZI::CloudRender