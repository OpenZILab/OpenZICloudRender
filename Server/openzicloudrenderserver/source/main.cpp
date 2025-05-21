#include "Config.h"
#include "D3D11DynamicRHI.h"
#include "D3D12DynamicRHI.h"
#include "Logger/CommandLogger.h"
#include "PixelStreamingStreamer.h"
#include "UnrealReceiveThread.h"
#include <Windows.h>

#include <chrono>
#include <ctime>

#include <exception>
#include <fstream>
#include <filesystem>

#ifdef BUILD_WITH_CRASHPAD
#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#endif

using namespace std;
using namespace OpenZI;
using namespace OpenZI::CloudRender;

/* This function is called by the Windows function DispatchMessage( ) */
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) /* handle the messages */
    {
    case WM_DESTROY:
        PostQuitMessage(0); /* send a WM_QUIT to the message queue */
        break;
    default: /* for messages that we don't deal with */
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void InitDynamicRHI() {
    if (GDynamicRHI)
        return;
    std::string RHIName = FAppConfig::Get().RHIName;
    if (RHIName == "D3D11") {
#if MLCRS_ENABLE_D3D11
        GDynamicRHI = std::make_shared<FD3D11DynamicRHI>();
#else
        ML_LOG(LogMain, Error,
               "Current OpenZICloudRenderServer is not support D3D11, please modify "
               "OpenZICloudRenderServer.json");
        std::exit(1);
#endif
    } else if (RHIName == "D3D12") {
#if MLCRS_ENABLE_D3D12
        GDynamicRHI = std::make_shared<FD3D12DynamicRHI>();
#else
        ML_LOG(LogMain, Error,
               "Current OpenZICloudRenderServer is not support D3D12, please modify "
               "OpenZICloudRenderServer.json");
        std::exit(1);
#endif
    } else {
        ML_LOG(LogMain, Error,
               "RHI %s is not support, please modify the property named RHIName in "
               "OpenZICloudRenderServer.json",
               RHIName.c_str());
        std::exit(1);
    }
}

int main(int argc, char** argv) {
    ML_LOG(LogMain, Success, "OpenZICloudRenderServer started.");

#ifdef BUILD_WITH_CRASHPAD
    wstring exeDir = std::filesystem::current_path().wstring();
    base::FilePath handler(exeDir + L"/crashpad_handler.exe");
    base::FilePath reportsDir(exeDir + L"/Crashes");
    base::FilePath metricsDir(exeDir + L"/Crashes");
    map<string, string> annotations;
    annotations["format"] = "minidump";
    annotations["product"] = "OpenZIEditorLauncher";
    annotations["version"] = "1.0.0";
    annotations["user"] = "support@cengzi.com";

    vector<string> arguments;
    arguments.emplace_back("--no-rate-limit");
    vector<base::FilePath> attachments;
    base::FilePath attachment(exeDir + L"/Crashes/attachment.txt");
    attachments.emplace_back(attachment);
    unique_ptr<crashpad::CrashReportDatabase> database =
        crashpad::CrashReportDatabase::Initialize(reportsDir);
    crashpad::Settings* settings = database->GetSettings();
    settings->SetUploadsEnabled(false);
    crashpad::CrashpadClient* client = new crashpad::CrashpadClient();
    client->StartHandler(handler, reportsDir, metricsDir, "", annotations, arguments, false, true,
                         attachments);
#endif
    // {
    //     // 获取当前系统时间点
    //     auto now = std::chrono::system_clock::now();

    //     // 将时间点转换为时间结构
    //     std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    //     // 将时间结构转换为本地时间
    //     std::tm* localTime = std::localtime(&nowTime);

    //     // 提取日期信息
    //     int year = localTime->tm_year + 1900; // 年份需要加上1900
    //     int month = localTime->tm_mon + 1;    // 月份从0开始，需要加1
    //     int day = localTime->tm_mday;         // 日期

    //     if (year != 2023) {
    //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //         exit(0);
    //     }
    //     if (month != 7) {
    //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //         ML_LOGW(Main, Error, L"OpenZI云渲染程序已过期");
    //         exit(0);
    //     }
    // }

    if (argc >= 2) {
        // ML_LOG(LogMain, Success, "ShareMemorySuffix=%s", argv[1]);
        FAppConfig::Get().ShareMemorySuffix = argv[1];
    }
    if (argc >= 3 && FAppConfig::Get().bUseGraphicsAdapter) {
        FAppConfig::Get().GpuIndex = atoi(argv[2]);
        ML_LOG(LogMain, Success, "graphicsadapter=%d", FAppConfig::Get().GpuIndex);
    }
    if (argc >= 6) {
        FAppConfig::Get().AdapterLuidLowPart = atoi(argv[3]);
        FAppConfig::Get().AdapterLuidHighPart = atoi(argv[4]);
        FAppConfig::Get().AdapterDeviceId = atoi(argv[5]);
        ML_LOG(LogMain, Success, "LuidLowPart=%d LuidHighPart=%d DeviceId=%d",
               FAppConfig::Get().AdapterLuidLowPart, FAppConfig::Get().AdapterLuidHighPart,
               FAppConfig::Get().AdapterDeviceId);
    }
    InitDynamicRHI();
    std::unique_ptr<FUnrealAudioReceiver> UnrealAudioReceiver = nullptr;
    if (FAppConfig::Get().WebRTCDisableTransmitAudio == false) {
        UnrealAudioReceiver = std::make_unique<FUnrealAudioReceiver>();
        UnrealAudioReceiver->Start();
    }
    auto PixelStreamingStreamer = std::make_unique<FPixelStreamingStreamer>();
    auto UnrealVedioReceiver = std::make_unique<FUnrealVideoReceiver>();
    UnrealVedioReceiver->Start();

    /* Here messages to the application are saved */
    // MSG messages;
    /* Run the message loop. It will run until GetMessage( ) returns 0 */
    // while (GetMessage(&messages, NULL, 0, 0))
    // {
    //     TranslateMessage(&messages);
    //     DispatchMessage(&messages);
    // }
    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    /* The program return-value is 0 - The value that PostQuitMessage( ) gave */

    // return (int)(messages.wParam);
    return 0;
}
