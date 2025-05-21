#include "Config.h"
#include "Logger/CommandLogger.h"
#include "PixelStreamingStreamer.h"
#include "UnrealReceiveThread.h"
#include "FdSocket.h"
#include "VulkanDynamicRHI.h"
#include <signal.h>

using namespace OpenZI;
using namespace OpenZI::CloudRender;
// std::unique_ptr<FUnrealVideoReceiver> UnrealVedioReceiver;
std::unique_ptr<FUnrealAudioReceiver> UnrealAudioReceiver;
std::unique_ptr<ZFdSocketClient> FdSocketClient;
std::unique_ptr<FPixelStreamingStreamer> PixelStreamingStreamer;

void InitDynamicRHI() {
    if (GDynamicRHI)
        return;
    std::string RHIName = FAppConfig::Get().RHIName;
    ML_LOG(Main, Log, "RHIName = %s", RHIName.c_str());
    if (RHIName == "Vulkan") {
        GDynamicRHI = std::make_shared<FVulkanDynamicRHI>();
    }
}

void handle_signal(int sig) {
    // UnrealVedioReceiver.reset();
    UnrealAudioReceiver.reset();
    // PixelStreamingStreamer.reset();
    GDynamicRHI.reset();
    // exit(0);
}

int main(int argc, char** argv) {
    // signal(SIGINT, handle_signal);
    // signal(SIGTERM, handle_signal);
    // signal(SIGILL, handle_signal);
    // signal(SIGABRT, handle_signal);
    // signal(SIGFPE, handle_signal);
    // signal(SIGSEGV, handle_signal);
    //     // {
    //     //     // 获取当前系统时间点
    //     //     auto now = std::chrono::system_clock::now();

    //     //     // 将时间点转换为时间结构
    //     //     std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    //     //     // 将时间结构转换为本地时间
    //     //     std::tm* localTime = std::localtime(&nowTime);

    //     //     // 提取日期信息
    //     //     int year = localTime->tm_year + 1900; // 年份需要加上1900
    //     //     int month = localTime->tm_mon + 1;    // 月份从0开始，需要加1
    //     //     int day = localTime->tm_mday;         // 日期

    //     //     if (year != 2023) {
    //     //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //     //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //     //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //     //         exit(0);
    //     //     }
    //     //     if (month != 7) {
    //     //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //     //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //     //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //     //         exit(0);
    //     //     }
    //     // }
    if (argc >= 2) {
        std::string GUID = argv[1];
        FAppConfig::Get().ShareMemorySuffix = GUID;
        auto Id = static_cast<uint32>(atol(GUID.c_str()));
        FAppConfig::Get().Guid = Id;
        ML_LOG(LogMain, Success, "ShmSuffix=%s,ShmId=%d",
               FAppConfig::Get().ShareMemorySuffix.c_str(), Id);
    }
    if (argc >= 3 && FAppConfig::Get().bUseGraphicsAdapter) {
        FAppConfig::Get().GpuIndex = atoi(argv[2]);
        ML_LOG(LogMain, Success, "graphicsadapter=%d", FAppConfig::Get().GpuIndex);
    }
    if (argc >= 5) {
        FAppConfig::Get().AdapterDeviceId = atoi(argv[3]);
        std::string UUID = argv[4];
        ML_LOG(LogMain, Success, "DeviceId=%d UUID=%s", FAppConfig::Get().AdapterDeviceId,
               UUID.c_str());
        FAppConfig::Get().PipelineUUID = UUID;
    }
    InitDynamicRHI();
    // UnrealVedioReceiver = std::make_unique<FUnrealVideoReceiver>();
    // UnrealVedioReceiver->Start();

    if (FAppConfig::Get().WebRTCDisableTransmitAudio == false) {
        UnrealAudioReceiver = std::make_unique<FUnrealAudioReceiver>();
        UnrealAudioReceiver->Start();
    }
    std::string SocketName = "/tmp/openzicloud";
    FdSocketClient = std::make_unique<ZFdSocketClient>(SocketName);
    FdSocketClient->Start();
    PixelStreamingStreamer = std::make_unique<FPixelStreamingStreamer>();
    bool done = false;
    while (!done) {
        // UnrealVedioReceiver->Run();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}