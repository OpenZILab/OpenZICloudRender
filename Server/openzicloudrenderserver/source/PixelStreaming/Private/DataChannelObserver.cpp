//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 13:54
//
#include "DataChannelObserver.h"
#include "Config.h"
#include "Containers/FormatString.h"
#include "DataConverter.h"
#include "FdSocket.h"
#include "IPCTypes.h"
#include "IPixelStreamingSessions.h"
#include "Logger/CommandLogger.h"
#include "MessageCenter.h"
#include "ProtocolDefs.h"

namespace OpenZI::CloudRender {

#if PLATFORM_LINUX
    // std::shared_ptr<ZInputSocketSender> InputSender = std::make_shared<ZInputSocketSender>("/tmp/openzicloudinput");
    // FCriticalSection Mutex;
#endif

    FDataChannelObserver::FDataChannelObserver(IPixelStreamingSessions* InPlayerSessions,
                                               FPixelStreamingPlayerId InPlayerId)
        : PlayerSessions(InPlayerSessions), PlayerId(InPlayerId) {
        std::string Name = "OpenZICloudRenderInput_" + FAppConfig::Get().ShareMemorySuffix;
#if PLATFORM_WINDOWS
        InputWriter = std::make_shared<FShareMemoryWriter>(Name.c_str(), 1024 * 1024 * 256);
#elif PLATFORM_LINUX
        int32 IntSuffix = atoi(FAppConfig::Get().ShareMemorySuffix.c_str());
        key_t ShmKey = IntSuffix - 1;
        InputWriter = std::make_shared<FShareMemoryWriter>(ShmKey, 4096);
        // std::string SocketName = "/tmp/openzicloudinput";
#endif
    }

    void
    FDataChannelObserver::Register(rtc::scoped_refptr<webrtc::DataChannelInterface> InDataChannel) {
        DataChannel = InDataChannel;
        DataChannel->RegisterObserver(this);
    }

    void FDataChannelObserver::Unregister() {
        if (DataChannel) {
            DataChannel->UnregisterObserver();
            DataChannel = nullptr;
        }
    }

    void FDataChannelObserver::OnStateChange() {
        if (!DataChannel) {
            return;
        }

        webrtc::DataChannelInterface::DataState State = DataChannel->state();
        if (State == webrtc::DataChannelInterface::DataState::kOpen) {
            PUBLISH_MESSAGE(OnDataChannelOpenNative, this->PlayerId, this->DataChannel.get());
        }
    }

    void FDataChannelObserver::OnMessage(const webrtc::DataBuffer& Buffer) {
        if (!DataChannel || Buffer.data.size() <= 0) {
            return;
        }

        EToStreamerMsg MsgType = static_cast<EToStreamerMsg>(Buffer.data.data()[0]);

        if (MsgType == EToStreamerMsg::RequestQualityControl) {
            ML_LOG(LogPixelStreaming, Log,
                   "Player %s has requested quality control through the data channel.",
                   PlayerId.c_str());
            PlayerSessions->SetQualityController(PlayerId);
        } else if (MsgType == EToStreamerMsg::LatencyTest) {
            SendLatencyReport();
        } else if (MsgType == EToStreamerMsg::RequestInitialSettings) {
            SendInitialSettings();
        } else {
            const uint8* Data = Buffer.data.data();
#if PLATFORM_WINDOWS
            InputWriter->Write(const_cast<uint8*>(Data), (uint32)Buffer.data.size(), 0, 0,
                               (HANDLE)(InputId++));
#elif PLATFORM_LINUX
            InputWriter->Write(const_cast<uint8*>(Data), (uint32)Buffer.data.size(), 0, 0,
                               InputId++);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
        }
    }

    void FDataChannelObserver::SendLatencyReport() const {
        // TODO: 代码未移植
    }

    void FDataChannelObserver::OnBufferedAmountChange(uint64 PreviousAmount) {}

    void FDataChannelObserver::SendInitialSettings() const {
        if (!DataChannel) {
            return;
        }

        // const std::string PixelStreamingPayload = Printf("{ \"AllowPixelStreamingCommands\": %s,
        // \"DisableLatencyTest\": %s }",
        //                                                       FAppConfig::Get().AllowConsoleCommands
        //                                                       ? "true" : "false",
        //                                                       FAppConfig::Get().DisableLatencyTester
        //                                                       ? "true" : "false");

        const std::string WebRTCPayload = Printf(
            "{ \"DegradationPref\": \"%s\", \"FPS\": %d, \"MinBitrate\": %d, \"MaxBitrate\": "
            "%d, \"LowQP\": %d, \"HighQP\": %d }",
            FAppConfig::Get().DegradationPreference.c_str(), FAppConfig::Get().WebRTCFps,
            FAppConfig::Get().WebRTCMinBitrate, FAppConfig::Get().WebRTCMaxBitrate,
            FAppConfig::Get().WebRTCLowQpThreshold, FAppConfig::Get().WebRTCHighQpThreshold);

        const std::string EncoderPayload = Printf(
            "{ \"TargetBitrate\": %d, \"MaxBitrate\": %d, \"MinQP\": %d, \"MaxQP\": %d, "
            "\"RateControl\": \"%s\", \"FillerData\": %d, \"MultiPass\": \"%s\" }",
            FAppConfig::Get().EncoderTargetBitrate, FAppConfig::Get().EncoderMaxBitrate,
            FAppConfig::Get().EncoderMinQP, FAppConfig::Get().EncoderMaxQP,
            FAppConfig::Get().EncoderRateControl.c_str(),
            FAppConfig::Get().EnableFillerData ? 1 : 0, FAppConfig::Get().EncoderMultipass.c_str());

        // const std::string FullPayload = Printf("{ \"PixelStreaming\": %s, \"Encoder\": %s,
        // \"WebRTC\": %s }", PixelStreamingPayload.c_str(), EncoderPayload.c_str(),
        // WebRTCPayload.c_str());
        const std::string FullPayload = Printf("{ \"Encoder\": %s, \"WebRTC\": %s }",
                                               EncoderPayload.c_str(), WebRTCPayload.c_str());

        if (!PlayerSessions->Send(PlayerId, EToPlayerMsg::InitialSettings, FullPayload)) {
            ML_LOG(LogPixelStreaming, Log, "Failed to send initial Pixel Streaming settings.");
        }
    }
} // namespace OpenZI::CloudRender