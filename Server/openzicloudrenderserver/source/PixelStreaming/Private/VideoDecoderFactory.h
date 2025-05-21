#pragma once

#include "WebRTCIncludes.h"

namespace OpenZI {
namespace CloudRender {

class FVideoDecoderFactory : public webrtc::VideoDecoderFactory {
  public:
    virtual std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
    virtual std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat& format) override;
};

}
} // namespace OpenZI