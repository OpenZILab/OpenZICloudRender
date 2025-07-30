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

cmake_minimum_required(VERSION 3.20)
project(video_converter VERSION 0.1.0)

add_library(
    video_converter ""
)

target_sources(
    video_converter
        PRIVATE   
            video_converter.cc
        PUBLIC
            video_converter.h
)

target_include_directories(
    video_converter
        PRIVATE   
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/include
)

target_link_libraries(
    video_converter
        PRIVATE   
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/lib/avformat.lib
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/lib/avcodec.lib
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/lib/avutil.lib
)

add_executable(
    video_converter_test video_converter_test.cc
)

target_link_libraries(
    video_converter_test
    PRIVATE
      video_converter
)