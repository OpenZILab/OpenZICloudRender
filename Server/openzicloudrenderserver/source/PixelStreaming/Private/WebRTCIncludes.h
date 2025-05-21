//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/25 18:08
//

#pragma once

#pragma warning(push)

#pragma warning(disable : 4003)
#pragma warning(disable : 4005)
#pragma warning(disable : 4125)
#pragma warning(disable : 4138)
#pragma warning(disable : 4265)
#pragma warning(disable : 4267)
#pragma warning(disable : 4456)
#pragma warning(disable : 4457)
#pragma warning(disable : 4458)
#pragma warning(disable : 4459)
#pragma warning(disable : 4510)
#pragma warning(disable : 4582)
#pragma warning(disable : 4610)
#pragma warning(disable : 4668)
#pragma warning(disable : 4701)
#pragma warning(disable : 4706)
#pragma warning(disable : 4800)
#pragma warning(disable : 4946)
#pragma warning(disable : 4995)
#pragma warning(disable : 4996)
#pragma warning(disable : 5038)
#pragma warning(disable : 6011)
#pragma warning(disable : 6101)
#pragma warning(disable : 6244)
#pragma warning(disable : 6287)
#pragma warning(disable : 6308)
#pragma warning(disable : 6326)
#pragma warning(disable : 6340)
#pragma warning(disable : 6385)
#pragma warning(disable : 6386)
#pragma warning(disable : 28182)
#pragma warning(disable : 28251)
#pragma warning(disable : 28252)
#pragma warning(disable : 28253)
#pragma warning(disable : 28301)

#if PLATFORM_WINDOWS
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "msdmo.lib")
#pragma comment(lib, "Strmiids.lib")
#elif PLATFORM_LINUX

#endif

#include "api/rtp_receiver_interface.h"
#include "api/media_types.h"
#include "api/media_stream_interface.h"
#include "api/data_channel_interface.h"
#include "api/peer_connection_interface.h"
#include "api/create_peerconnection_factory.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/audio_codecs/audio_format.h"
#include "api/audio_codecs/audio_decoder_factory_template.h"
#include "api/audio_codecs/audio_encoder_factory_template.h"
#include "api/audio_codecs/opus/audio_decoder_opus.h"
#include "api/audio_codecs/opus/audio_encoder_opus.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/video_decoder.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder_software_fallback_wrapper.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video/video_frame.h"
#include "api/video/video_rotation.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_sink_interface.h"

#include "rtc_base/thread.h"
#include "rtc_base/logging.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/net_helpers.h"
#include "rtc_base/string_utils.h"
// #include "rtc_base/signal_thread.h"
#include "rtc_base/experiments/rate_control_settings.h"

#include "pc/session_description.h"
#include "pc/video_track_source.h"

#include "media/engine/internal_decoder_factory.h"
#include "media/engine/internal_encoder_factory.h"
// #include "media/base/h264_profile_level_id.h"
#include "api/video_codecs/h264_profile_level_id.h"
// #include "api/video_codecs/h265_profile_tier_level.h"
#include "media/base/adapted_video_track_source.h"
#include "media/base/media_channel.h"
#include "media/base/video_common.h"

#include "modules/video_capture/video_capture_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
// #include "modules/video_coding/utility/framerate_controller.h"
#include "common_video/framerate_controller.h"
#include "modules/video_coding/utility/simulcast_rate_allocator.h"

#include "common_video/h264/h264_bitstream_parser.h"
#include "common_video/h264/h264_common.h"
// #include "common_video/h265/h265_bitstream_parser.h"
#include "common_video/h265/h265_common.h"

#include "media/base/video_broadcaster.h"

#include "system_wrappers/include/field_trial.h"
// because WebRTC uses STL
#include <string>
#include <memory>
#include <stack>

#pragma warning(pop)