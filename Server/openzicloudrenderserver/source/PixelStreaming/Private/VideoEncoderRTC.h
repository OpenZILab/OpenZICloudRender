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
#include "PixelStreamingPlayerId.h"
#include "WebRTCIncludes.h"
#include "VideoEncoder.h"
#include <memory>
#include <optional>

namespace OpenZI::CloudRender
{
    class FVideoEncoderFactory;

    // Implementation that is a WebRTC video encoder that allows us tie to our actually underlying non-WebRTC video encoder.
    class FVideoEncoderRTC : public webrtc::VideoEncoder
    {
    public:
        FVideoEncoderRTC(FVideoEncoderFactory &InFactory);
        virtual ~FVideoEncoderRTC() override;

        // WebRTC Interface
        virtual int InitEncode(webrtc::VideoCodec const *codec_settings, webrtc::VideoEncoder::Settings const &settings) override;
        virtual int32 RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback *callback) override;
        virtual int32 Release() override;
        virtual int32 Encode(webrtc::VideoFrame const &frame, std::vector<webrtc::VideoFrameType> const *frame_types) override;
        virtual void SetRates(RateControlParameters const &parameters) override;
        virtual webrtc::VideoEncoder::EncoderInfo GetEncoderInfo() const override;

        // Note: These funcs can also be overriden but are not pure virtual
        // virtual void SetFecControllerOverride(FecControllerOverride* fec_controller_override) override;
        // virtual void OnPacketLossRateUpdate(float packet_loss_rate) override;
        // virtual void OnRttUpdate(int64_t rtt_ms) override;
        // virtual void OnLossNotification(const LossNotification& loss_notification) override;
        // End WebRTC Interface.

        void SendEncodedImage(uint64 SourceEncoderId, webrtc::EncodedImage const &encoded_image, webrtc::CodecSpecificInfo const *codec_specific_info);

    private:
        void UpdateConfig();
        FVideoEncoder::FLayerConfig CreateEncoderConfigFromCVars(FVideoEncoder::FLayerConfig BaseEncoderConfig) const;

        FVideoEncoderFactory &Factory;

        uint64 HardwareEncoderId;

        // We store this so we can restore back to it if the user decides to use then stop using the PixelStreaming.Encoder.TargetBitrate CVar.
        int32 WebRtcProposedTargetBitrate = 5000000;

        webrtc::EncodedImageCallback *OnEncodedImageCallback = nullptr;

        // WebRTC may request a bitrate/framerate change using SetRates(), we only respect this if this encoder is actually encoding
        // so we use this optional object to store a rate change and act upon it when this encoder does its next call to Encode().
        std::optional<RateControlParameters> PendingRateChange;

        // Used to send an initial keyframe
        // see notes in SendEncodedImage implementation
        int FirstKeyframeCountdown = 2;


    };
} // namespace OpenZI::CloudRender