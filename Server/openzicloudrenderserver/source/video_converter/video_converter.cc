#include "video_converter.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace OpenZI {
namespace CloudRender {

#define USE_OPEN_CLOSE_TO_WRITE_FRAME 0

AVFormatContext* outputFormatContext = nullptr;
AVStream* videoStream = nullptr;
AVDictionary *opts = nullptr;
int64_t memoryOffset = 0;

int VideoConverter::WriteMp4Header(int width, int height, int fps, int64_t start_time) {
    // 打开输出文件
    if (avformat_alloc_output_context2(&outputFormatContext, nullptr, "mp4", nullptr) <
        0) {
        // 处理打开输出文件失败的情况
        return -1;
    }
    outputFormatContext->start_time_realtime = start_time;
    // 添加视频流
    videoStream = avformat_new_stream(outputFormatContext, nullptr);
    if (!videoStream) {
        // 处理添加视频流失败的情况
        return -1;
    }
    // 设置视频流参数（编解码器类型、时间基、帧率等等）
    videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    videoStream->codecpar->codec_id = AV_CODEC_ID_HEVC; // H.265
    videoStream->codecpar->width = width;
    videoStream->codecpar->height = height;
    videoStream->avg_frame_rate = {fps, 1};
    videoStream->time_base = {1, 1000000};
    // videoStream->start_time = start_time;
    // 其他视频参数设置
	// av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov", 0);
	av_dict_set(&opts, "movflags", "frag_keyframe", 0);
	// av_dict_set_int(&opts, "g", 58, 0);
    // 打开输出流
    if (avio_open_dyn_buf(&outputFormatContext->pb) < 0) {
        return -1;
    }

    // 写文件头
    if (avformat_write_header(outputFormatContext, &opts) < 0) {
        // 处理写文件头失败的情况
        av_dict_free(&opts);
        return -1;
    }
    return 0;
}

int VideoConverter::WriteMp4Frame(const std::vector<unsigned char>& frame, int64_t timestamp, bool key_frame) {
#if USE_OPEN_CLOSE_TO_WRITE_FRAME
    if (avio_open_dyn_buf(&outputFormatContext->pb) < 0) {
        av_log(nullptr, 1, "write mp4 frame open dyn buf error");
        return -1;
    }
#else
#endif
    // 将已编码的 H.265 帧数据填充到 AVPacket 中，并设置正确的时间戳等信息
    AVPacket* pkt = av_packet_alloc();
    // pkt->duration = 0;
    pkt->data = const_cast<uint8_t*>(frame.data());
    pkt->size = static_cast<int>(frame.size());
    // 设置时间戳、帧类型等信息
    pkt->pts = timestamp;          // 设置帧的时间戳
    pkt->dts = pkt->pts;            // 设置解码时间戳
    if (key_frame) {
        pkt->flags |= AV_PKT_FLAG_KEY; // 如果是关键帧，设置关键帧标志
    }
    pkt->stream_index = videoStream->index;

    if (av_write_frame(outputFormatContext, pkt) < 0) {
        // 处理写帧数据失败的情况
        av_packet_free(&pkt);
        return -1;
    }
    av_packet_free(&pkt);
    return 0;
}

std::vector<uint8_t> VideoConverter::GetMp4Buffer() {
    uint8_t* data = nullptr;
#if USE_OPEN_CLOSE_TO_WRITE_FRAME
    int size = avio_close_dyn_buf(outputFormatContext->pb, &data);
#else
    int size = avio_get_dyn_buf(outputFormatContext->pb, &data);
#endif
    if (data == nullptr || size <= 0) {
        av_log(nullptr, 1, "get mp4 buffer is empty");
        return {};
    }
#if USE_OPEN_CLOSE_TO_WRITE_FRAME
    std::vector<uint8_t> mp4buffer;
    mp4buffer.resize(size);
    memcpy(&mp4buffer[0], data, size);
    av_free(data);
#else
    std::vector<uint8_t> mp4buffer(data + memoryOffset, data + size);
    memoryOffset = size;
#endif
    return std::move(mp4buffer);
}

std::vector<uint8_t> VideoConverter::WriteMp4Trailer() {
    av_log(nullptr, 1, "write mp4 trailer\n");
#if USE_OPEN_CLOSE_TO_WRITE_FRAME
    if (avio_open_dyn_buf(&outputFormatContext->pb) < 0) {
        return {};
    }
#endif
    // 写文件尾
    if (av_write_trailer(outputFormatContext) < 0) {
        // 处理写文件尾失败的情况
        return {};
    }
    uint8_t* data = nullptr;
    int size = avio_close_dyn_buf(outputFormatContext->pb, &data);
#if USE_OPEN_CLOSE_TO_WRITE_FRAME
    std::vector<uint8_t> mp4buffer;
    mp4buffer.resize(size);
    memcpy(&mp4buffer[0], data, size);
#else
    std::vector<uint8_t> mp4buffer(data + memoryOffset, data + size);
    memoryOffset = size;
#endif
    av_free(data);
    avformat_free_context(outputFormatContext);
    av_dict_free(&opts);
    return mp4buffer;
}
}
}
