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

#include "VideoDecoderFactory.h"
#include "absl/strings/match.h"

namespace OpenZI {
namespace CloudRender {
std::vector<webrtc::SdpVideoFormat> FVideoDecoderFactory::GetSupportedFormats() const {
  std::vector<webrtc::SdpVideoFormat> supported_formats = {};
  supported_formats.push_back(webrtc::SdpVideoFormat(cricket::kH265CodecName));
  return supported_formats;
}
std::unique_ptr<webrtc::VideoDecoder> FVideoDecoderFactory::CreateVideoDecoder(const webrtc::SdpVideoFormat& format) {
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName)) {
    //   return std::make_unique<VideoDecoderVPX>(8);
  } else if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName)) {
    //   return std::make_unique<VideoDecoderVPX>(9);
  } else if (absl::EqualsIgnoreCase(format.name, cricket::kH265CodecName)) {
    //   return std::make_unique<VideoDecoderH265>();
  }
//   return std::make_unique<FVideoDecoderStub>();
  return nullptr;
}
}
} // namespace OpenZI