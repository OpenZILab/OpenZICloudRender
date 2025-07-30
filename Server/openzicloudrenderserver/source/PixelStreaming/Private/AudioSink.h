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
#include "IPixelStreamingAudioSink.h"
#include "IPixelStreamingAudioConsumer.h"
#include <set>

namespace OpenZI::CloudRender
{
    // This is a PixelStreaming sink AND a WebRTC sink.
    // It collects audio coming in from a WebRTC audio source and passes into into UE's audio system.
    class FAudioSink : public webrtc::AudioTrackSinkInterface, public IPixelStreamingAudioSink
    {
    public:
        FAudioSink()
            : AudioConsumers(){};

        // Note: destructor will call destroy on any attached audio consumers
        virtual ~FAudioSink();

        // Begin AudioTrackSinkInterface
        void OnData(const void *audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames, absl::optional<int64_t> absolute_capture_timestamp_ms) override;
        // End AudioTrackSinkInterface

        // Begin IPixelStreamingAudioSink
        void AddAudioConsumer(IPixelStreamingAudioConsumer *AudioConsumer) override;
        void RemoveAudioConsumer(IPixelStreamingAudioConsumer *AudioConsumer) override;
        bool HasAudioConsumers() override;
        // End IPixelStreamingAudioSink

    private:
        void UpdateAudioSettings(int InNumChannels, int InSampleRate);

    private:
        std::set<IPixelStreamingAudioConsumer *> AudioConsumers;
    };
} // namespace OpenZI::CloudRender