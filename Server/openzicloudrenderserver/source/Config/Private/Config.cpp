// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 10:14
// 
#include "Config.h"
#include "PicoJson.h"
#include "Logger/CommandLogger.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace OpenZI::CloudRender
{
    static picojson::value ConfigJsonRoot;

    FAppConfig::FAppConfig()
        : Width(1920)
        , Height(1080)
        // , RenderWidth(1920)
        // , RenderHeight(1080)
        , ResolutionX(3840)
        , ResolutionY(2160)
        , FPS(60)
        , BitRate(1000000)
        , EncoderTargetBitrate(-1)
        , EncoderMaxBitrate(20000000)
        , DebugDumpFrame(false)
        , EncoderMinQP(0)
        , EncoderMaxQP(51)
        , EncoderRateControl("CBR")
        , EnableFillerData(false)
        , EncoderMultipass("FULL")
        , H264Profile("BASELINE")
        , H265Profile("AUTO")
        , EncoderKeyframeInterval(300)
        , EncoderCodec("H265")
        , DegradationPreference("MAINTAIN_FRAMERATE")
        , WebRTCFps(60)
        , WebRTCStartBitrate(1000000)
        , WebRTCMinBitrate(100000)
        , WebRTCMaxBitrate(100000000)
        , WebRTCLowQpThreshold(25)
        , WebRTCHighQpThreshold(37)
        , WebRTCDisableReceiveAudio(true)
        , WebRTCDisableTransmitAudio(true)
        , WebRTCDisableAudioSync(true)
        , WebRTCUseLegacyAudioDevice(false)
        , WebRTCDisableStats(true)
        , SendPlayerIdAsInteger(true)
        , SignallingServerUrl("http://127.0.0.1:8888/ws")
        , StreamerId("MLCRS")
        , RHIName("D3D11")
        , bEnableDynamicFps(false)
        , bEncoderUseLowPreset(true)
        , EncoderTuningInfo("LOW")
        , EncoderGopLength(250)
        , ShareMemorySuffix("")
        , bUseGraphicsAdapter(false)
        , GpuIndex(0)
        , AdapterLuidLowPart(0)
        , AdapterLuidHighPart(0)
        , AdapterDeviceId(0)
        , Guid(0)
        , PipelineUUID("")
    {
        InitializeDefaultConfigValue();
    }

    std::string GetConfigFilePath()
    {
        auto FilePath = std::filesystem::current_path() / "OpenZICloudRenderServer.json";
        ML_LOG(Config, Log, "CurrentConfigFilePath=%s", FilePath.generic_u8string().c_str());
        return FilePath.generic_u8string();
    }
    template <typename ValueType>
    ValueType TryReadConfig(std::string Key, ValueType &DefaultValue)
    {
        if (!ConfigJsonRoot.contains(Key) || !ConfigJsonRoot.get(Key).is<double>())
            return DefaultValue;
        ValueType Value = (ValueType)ConfigJsonRoot.get(Key).get<double>();
        return Value;
    }

    template <>
    std::string TryReadConfig(std::string Key, std::string &DefaultValue)
    {
        if (!ConfigJsonRoot.contains(Key) || !ConfigJsonRoot.get(Key).is<std::string>())
            return DefaultValue;
        std::string Value = ConfigJsonRoot.get(Key).get<std::string>();
        return Value;
    }

    template <>
    bool TryReadConfig(std::string Key, bool &DefaultValue)
    {
        if (!ConfigJsonRoot.contains(Key) || !ConfigJsonRoot.get(Key).is<bool>())
            return DefaultValue;
        bool Value = ConfigJsonRoot.get(Key).get<bool>();
        return Value;
    }

    void FAppConfig::InitializeDefaultConfigValue()
    {
        std::string ConfigFilePath = GetConfigFilePath();
        std::fstream FileStream;
        FileStream.open(ConfigFilePath, std::ios_base::in);
        if (FileStream.is_open())
        {
            std::stringstream JsonStringStream;
            JsonStringStream << FileStream.rdbuf();
            FileStream.close();
            auto JsonString = JsonStringStream.str();
            std::string ErrorStr = picojson::parse(ConfigJsonRoot, JsonString);
            if (!ErrorStr.empty())
            {
                ML_LOG(LogConfig, Error, "Parse config file error, %s", ErrorStr.c_str());
            }
            else
            {
#define READCONFIG(Type, Name) Name = TryReadConfig<Type>(#Name, Name);
                // READCONFIG(uint32, RenderWidth)
                // READCONFIG(uint32, RenderHeight)
                READCONFIG(uint32, FPS)
                READCONFIG(uint32, BitRate)

                READCONFIG(int32, EncoderTargetBitrate)
                READCONFIG(int32, EncoderMaxBitrate)
                READCONFIG(bool, DebugDumpFrame)
                READCONFIG(int32, EncoderMinQP)
                READCONFIG(int32, EncoderMaxQP)
                READCONFIG(std::string, EncoderRateControl)
                READCONFIG(bool, EnableFillerData)
                READCONFIG(std::string, EncoderMultipass)
                READCONFIG(std::string, H264Profile)
                READCONFIG(std::string, H265Profile)
                READCONFIG(int32, EncoderKeyframeInterval)
                READCONFIG(std::string, EncoderCodec)

                READCONFIG(std::string, DegradationPreference)
                READCONFIG(int32, WebRTCFps)
                READCONFIG(int32, WebRTCStartBitrate)
                READCONFIG(int32, WebRTCMinBitrate)
                READCONFIG(int32, WebRTCMaxBitrate)
                READCONFIG(int, WebRTCLowQpThreshold)
                READCONFIG(int, WebRTCHighQpThreshold)
                READCONFIG(bool, WebRTCDisableReceiveAudio)
                READCONFIG(bool, WebRTCDisableTransmitAudio)
                READCONFIG(bool, WebRTCDisableAudioSync)
                READCONFIG(bool, WebRTCUseLegacyAudioDevice)
                READCONFIG(bool, WebRTCDisableStats)

                READCONFIG(bool, AllowConsoleCommands)
                READCONFIG(bool, OnScreenStats)
                READCONFIG(bool, LogStats)

                READCONFIG(int32, FreezeFrameQuality)
                READCONFIG(bool, SendPlayerIdAsInteger)
                READCONFIG(bool, DisableLatencyTester)

                READCONFIG(std::string, SignallingServerUrl)
                READCONFIG(std::string, StreamerId)
                READCONFIG(std::string, RHIName)
                READCONFIG(bool, bEnableDynamicFps)
                READCONFIG(bool, bEncoderUseLowPreset)
                READCONFIG(std::string, EncoderTuningInfo)
                READCONFIG(int32, EncoderGopLength)

                READCONFIG(bool, bUseGraphicsAdapter)
                READCONFIG(int, GpuIndex)
#undef READCONFIG
            }
        }
        else
        {
            ML_LOG(LogConfig, Error, "Open config file error, please check file %s", ConfigFilePath.c_str());
        }
    }

    FAppConfig &FAppConfig::Get()
    {
        static FAppConfig AppConfig;
        return AppConfig;
    }

} // namespace OpenZI::CloudRender