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

#include "AudioSink.h"

namespace OpenZI::CloudRender
{
    FAudioSink::~FAudioSink()
    {
        for (IPixelStreamingAudioConsumer *AudioConsumer : AudioConsumers)
        {
            if (AudioConsumer != nullptr)
            {
                AudioConsumer->OnConsumerRemoved();
            }
        }
        AudioConsumers.clear();
    }

    void FAudioSink::OnData(const void *audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames, absl::optional<int64_t> absolute_capture_timestamp_ms)
    {

        // This data is populated from the internals of WebRTC, basically each audio track sent from the browser has its RTP audio source received and decoded.
        // The sample rate and number of channels here has absolutely no relationship with PixelStreamingAudioDeviceModule.
        // The sample rate and number of channels here is determined adaptively by WebRTC's NetEQ class that selects sample rate/number of channels
        // based on network conditions and other factors.

        if (!HasAudioConsumers())
        {
            return;
        }

        // Iterate audio consumers and pass this data to their buffers
        for (IPixelStreamingAudioConsumer *AudioConsumer : AudioConsumers)
        {
            AudioConsumer->ConsumeRawPCM(static_cast<const int16_t *>(audio_data), sample_rate, number_of_channels, number_of_frames);
        }

        return;
    }

    void FAudioSink::AddAudioConsumer(IPixelStreamingAudioConsumer *AudioConsumer)
    {
        bool bAlreadyInSet = AudioConsumers.find(AudioConsumer) != AudioConsumers.end();
        if (!bAlreadyInSet)
        {
            AudioConsumer->OnConsumerAdded();
        }
        AudioConsumers.insert(AudioConsumer);
    }

    void FAudioSink::RemoveAudioConsumer(IPixelStreamingAudioConsumer *AudioConsumer)
    {
        if (AudioConsumers.find(AudioConsumer) != AudioConsumers.end())
        {
            AudioConsumers.erase(AudioConsumer);
            AudioConsumer->OnConsumerRemoved();
        }
    }

    bool FAudioSink::HasAudioConsumers()
    {
        return AudioConsumers.size() > 0;
    }
} // namespace OpenZI::CloudRender