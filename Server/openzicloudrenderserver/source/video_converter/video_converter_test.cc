#include "video_converter.h"

#include <iostream>

int main() {
    auto A = OpenZI::CloudRender::VideoConverter::WriteMp4Header(1920, 1080);
    // A = OpenZI::CloudRender::VideoConverter::WriteMp4Header(1920, 1080);
    A = OpenZI::CloudRender::VideoConverter::WriteMp4Tailer(1920, 1080);
    return 0;
}