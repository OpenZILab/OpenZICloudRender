#pragma once

#include <vector>
#include <string>

namespace OpenZI {
namespace CloudRender {

  class VideoConverter {
   public:
    static int WriteMp4Header(int width, int height, int fps, int64_t start_time);
    static int WriteMp4Frame(const std::vector<unsigned char>& frame, int64_t timestamp, bool key_frame);
    static std::vector<uint8_t> WriteMp4Trailer();
    static std::vector<uint8_t> GetMp4Buffer();
  };

}
}