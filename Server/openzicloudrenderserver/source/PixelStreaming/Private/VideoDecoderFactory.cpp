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