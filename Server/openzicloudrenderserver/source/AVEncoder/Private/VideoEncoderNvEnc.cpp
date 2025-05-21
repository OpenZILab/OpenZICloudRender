// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/16 10:44
// 
// #include "WebSocketThread.h"
#include "VideoEncoderNvEnc.h"
#include "NvEncoder.h"
#include "NvCodecUtils.h"
#include "NvEncoderCLIOptions.h"
#if PLATFORM_WINDOWS
#include "NvEncoderD3D11.h"
#include "NvEncoderD3D12.h"
#include "CudaConverter.h"
#include "D3D11DynamicRHI.h"
#include "D3D12DynamicRHI.h"
#include "D3D12/D3D12CudaConverter.h"
#elif PLATFORM_LINUX
#include <vulkan/vulkan.h>
#include "VulkanDynamicRHI.h"
#include "Vulkan/VulkanCudaConverter.h"
#endif
#include "NvEncoderCuda.h"
#include "Config.h"
#include "SystemTime.h"
#include "Logger/CommandLogger.h"
#include <fstream>
#include <iostream>
#include "video_converter.h"
#include "MessageCenter/MessageCenter.h"
#include <fstream>

namespace OpenZI::CloudRender
{
    namespace NvEncOptionHelper {
        NV_ENC_PARAMS_RC_MODE ParseRateControlMode(std::string InRateControlMode) {
            if (InRateControlMode == "CONSTQP")
                return NV_ENC_PARAMS_RC_CONSTQP;
            if (InRateControlMode == "CBR")
                return NV_ENC_PARAMS_RC_CBR;
            if (InRateControlMode == "VBR")
                return NV_ENC_PARAMS_RC_VBR;
            if (InRateControlMode == "CBR_HQ")
                return NV_ENC_PARAMS_RC_CBR_HQ;
            if (InRateControlMode == "CBR_LOWDELAY_HQ")
                return NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
            if (InRateControlMode == "VBR_HQ")
                return NV_ENC_PARAMS_RC_VBR_HQ;
            if (InRateControlMode == "VBR_MINQP")
                return NV_ENC_PARAMS_RC_VBR_MINQP;
            return NV_ENC_PARAMS_RC_CONSTQP;
        }

        NV_ENC_MULTI_PASS ParseMultiPass(std::string InMultiPass) {
            if (InMultiPass == "DISABLED")
                return NV_ENC_MULTI_PASS_DISABLED;
            if (InMultiPass == "QUARTER")
                return NV_ENC_TWO_PASS_QUARTER_RESOLUTION;
            if (InMultiPass == "FULL")
                return NV_ENC_TWO_PASS_FULL_RESOLUTION;
            return NV_ENC_MULTI_PASS_DISABLED;
        }

        GUID ParseH264Profile(std::string InH264Profile) {
            if (InH264Profile == "AUTO")
                return NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
            if (InH264Profile == "BASELINE")
                return NV_ENC_H264_PROFILE_BASELINE_GUID;
            if (InH264Profile == "MAIN")
                return NV_ENC_H264_PROFILE_MAIN_GUID;
            if (InH264Profile == "HIGH")
                return NV_ENC_H264_PROFILE_HIGH_GUID;
            if (InH264Profile == "HIGH444")
                return NV_ENC_H264_PROFILE_HIGH_444_GUID;
            if (InH264Profile == "STEREO")
                return NV_ENC_H264_PROFILE_STEREO_GUID;
            if (InH264Profile == "PROGRESSIVE_HIGH")
                return NV_ENC_H264_PROFILE_PROGRESSIVE_HIGH_GUID;
            if (InH264Profile == "CONSTRAINED_HIGH")
                return NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID;
            return NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
        }

        GUID ParseH265Profile(std::string InH265Profile) {
            if (InH265Profile == "AUTO")
                return NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
            if (InH265Profile == "MAIN")
                return NV_ENC_HEVC_PROFILE_MAIN_GUID;
            if (InH265Profile == "MAIN10")
                return NV_ENC_HEVC_PROFILE_MAIN10_GUID;
            return NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
        }

        NV_ENC_TUNING_INFO ParseTuningInfo(std::string InTuningInfo) {
            if (InTuningInfo == "HIGH_QUALITY")
                return NV_ENC_TUNING_INFO_HIGH_QUALITY;
            if (InTuningInfo == "LOW_LATENCY")
                return NV_ENC_TUNING_INFO_LOW_LATENCY;
            if (InTuningInfo == "ULTRA_LOW_LATENCY")
                return NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
            if (InTuningInfo == "LOSSLESS")
                return NV_ENC_TUNING_INFO_LOSSLESS;
            if (InTuningInfo == "COUNT")
                return NV_ENC_TUNING_INFO_COUNT;
            return NV_ENC_TUNING_INFO_UNDEFINED;
        }
    }

    FVideoEncoderNvEnc::FVideoEncoderNvEnc(bool useNV12)
        : m_nFrame(0), m_useNV12(useNV12)
    {
#if PLATFORM_WINDOWS
        if (GDynamicRHI->GetName() == "D3D11")
        {
        }
        else if (GDynamicRHI->GetName() == "D3D12")
        {
        }
#elif PLATFORM_LINUX
        if (GDynamicRHI->GetName() == "Vulkan") {
            VulkanDynamicRHI = std::shared_ptr<FVulkanDynamicRHI>(static_cast<FVulkanDynamicRHI *>(GDynamicRHI.get()));
        }
#endif

        Initialize();
    }

    FVideoEncoderNvEnc::~FVideoEncoderNvEnc()
    {
        Shutdown();
    }

    void FVideoEncoderNvEnc::Initialize()
    {
        if (FAppConfig::Get().EncoderCodec == "H264" || FAppConfig::Get().EncoderCodec == "h264") {
            bH264 = true;
        } else {
            bH264 = false;
        }
        // TODO: 后续可能需要在这里加选项，从配置文件中读编码器选项，以NVENC接收的命令参数格式传入
        NvEncoderInitParam EncodeCLIOptions("");

        // Initialize Encoder
        NV_ENC_BUFFER_FORMAT format = NV_ENC_BUFFER_FORMAT_ARGB;


        // Log(L"Initializing CNvEncoder. Width=%d Height=%d Format=%d (useNV12:%d)", FAppConfig::Get().Width, FAppConfig::Get().Height, format, m_useNV12);
#if PLATFORM_WINDOWS
        if (GDynamicRHI->GetName() == "D3D11")
        {
            format = NV_ENC_BUFFER_FORMAT_NV12;
            m_Converter = std::make_shared<CudaConverter>(
                static_cast<FD3D11DynamicRHI*>(GDynamicRHI.get())->GetDevice(),
                FAppConfig::Get().Width, FAppConfig::Get().Height);
            ML_LOG(VideoEncoderNvEnc, Verbose, "After construct CudaConverter");
            m_NvNecoder = std::make_shared<NvEncoderCuda>(m_Converter->GetContext(), FAppConfig::Get().Width, FAppConfig::Get().Height, format, 0);
            ML_LOG(VideoEncoderNvEnc, Verbose, "After construct NvEncoderCuda");
        }
        else if (GDynamicRHI->GetName() == "D3D12")
        {
            ML_LOG(VideoEncoderNvEnc, Log, "RHIName=D3D12");
            format = NV_ENC_BUFFER_FORMAT_NV12;
            D3D12CudaConverter = std::make_unique<FD3D12CudaConverter>(FAppConfig::Get().Width, FAppConfig::Get().Height);
            m_NvNecoder = std::make_shared<NvEncoderCuda>(D3D12CudaConverter->GetContext(), FAppConfig::Get().Width, FAppConfig::Get().Height, format, 0);
        }
#elif PLATFORM_LINUX
        if (GDynamicRHI->GetName() == "Vulkan") {
            format = NV_ENC_BUFFER_FORMAT_NV12;
            VulkanCudaConverter = std::make_unique<FVulkanCudaConverter>(FAppConfig::Get().Width, FAppConfig::Get().Height);
            m_NvNecoder = std::make_shared<NvEncoderCuda>(VulkanCudaConverter->GetContext(), FAppConfig::Get().Width, FAppConfig::Get().Height, format, 0);
        }
#endif

        NV_ENC_INITIALIZE_PARAMS initializeParams = {NV_ENC_INITIALIZE_PARAMS_VER};
        NV_ENC_CONFIG encodeConfig = {NV_ENC_CONFIG_VER};

        initializeParams.encodeConfig = &encodeConfig;
        GUID EncoderGUID = bH264 ? NV_ENC_CODEC_H264_GUID : NV_ENC_CODEC_HEVC_GUID; // NV_ENC_CODEC_HEVC_GUID;
        GUID PresetGUID;
        NV_ENC_TUNING_INFO TuningInfo;
        // m_NvNecoder->CreateDefaultEncoderParams(&initializeParams, EncoderGUID, EncodeCLIOptions.GetPresetGUID(), EncodeCLIOptions.GetTuningInfo()); // NV_ENC_TUNING_INFO_LOW_LATENCY
        // if (FAppConfig::Get().Width <= 1920)
        // {
        //     PresetGUID = NV_ENC_PRESET_P3_GUID;
        //     TuningInfo = NV_ENC_TUNING_INFO_HIGH_QUALITY;
        // }
        // else if (FAppConfig::Get().Width <= 2560)
        // {
        //     PresetGUID = NV_ENC_PRESET_P3_GUID;
        //     TuningInfo = NV_ENC_TUNING_INFO_HIGH_QUALITY;
        // }
        // else if (FAppConfig::Get().Width <= 3840)
        // {
        //     PresetGUID = NV_ENC_PRESET_P4_GUID;
        //     TuningInfo = NV_ENC_TUNING_INFO_LOW_LATENCY;
        // }
        // else
        // {
        //     PresetGUID = NV_ENC_PRESET_P3_GUID;
        //     TuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
        // }
        PresetGUID = FAppConfig::Get().bEncoderUseLowPreset ? NV_ENC_PRESET_P3_GUID : NV_ENC_PRESET_P4_GUID;
        TuningInfo = NvEncOptionHelper::ParseTuningInfo(FAppConfig::Get().EncoderTuningInfo);
        m_NvNecoder->CreateDefaultEncoderParams(&initializeParams, EncoderGUID, PresetGUID, TuningInfo);
        // 设置编码分辨率
        // initializeParams.encodeWidth = FAppConfig::Get().Width;
        // initializeParams.encodeHeight = FAppConfig::Get().Height;
		initializeParams.darWidth = FAppConfig::Get().Width;
        initializeParams.darHeight = FAppConfig::Get().Height;
        initializeParams.frameRateNum = FAppConfig::Get().WebRTCFps;
        // 设置为1，不设置则默认值为2。会导致移动时画面像素边缘出现重影
        initializeParams.encodeConfig->gopLength = FAppConfig::Get().EncoderGopLength;
        // 从配置文件中读取配置项，设置编码器参数
        initializeParams.encodeConfig->frameIntervalP = FAppConfig::Get().EncoderKeyframeInterval;
        if (bH264) {
            initializeParams.encodeConfig->profileGUID = NvEncOptionHelper::ParseH264Profile(FAppConfig::Get().H264Profile);
        } else {
            initializeParams.encodeConfig->profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
            if (FAppConfig::Get().H265Profile == "AUTO")
                initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 0;
            if (FAppConfig::Get().H265Profile == "MAIN")
                initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 0;
            if (FAppConfig::Get().H265Profile == "MAIN10")
                initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 2;
        }
        initializeParams.encodeConfig->rcParams.rateControlMode = NvEncOptionHelper::ParseRateControlMode(FAppConfig::Get().EncoderRateControl);
        initializeParams.encodeConfig->rcParams.multiPass = NvEncOptionHelper::ParseMultiPass(FAppConfig::Get().EncoderMultipass);
        // initializeParams.encodeConfig->rcParams.enableMinQP = FAppConfig::Get().EncoderMinQP;
        // initializeParams.encodeConfig->rcParams.enableMaxQP = FAppConfig::Get().EncoderMaxQP;
        initializeParams.encodeConfig->rcParams.maxBitRate = FAppConfig::Get().EncoderMaxBitrate;
        // {
        // // initializeParams.darWidth = FAppConfig::Get().Width;
        // // initializeParams.darHeight = FAppConfig::Get().Height;
        // initializeParams.frameRateNum = FAppConfig::Get().WebRTCFps;
        // initializeParams.frameRateDen = 1;
        // // initializeParams.maxEncodeWidth = 4096;
        // // initializeParams.maxEncodeHeight = 4096;
        // // 设置为1，不设置则默认值为2。会导致移动时画面像素边缘出现重影
        // // initializeParams.encodeConfig->gopLength = FAppConfig::Get().EncoderGopLength;
        // // 从配置文件中读取配置项，设置编码器参数
        // // initializeParams.encodeConfig->frameIntervalP = FAppConfig::Get().EncoderKeyframeInterval;
        // initializeParams.encodeConfig->profileGUID = NvEncOptionHelper::ParseH264Profile(FAppConfig::Get().H264Profile);
        // initializeParams.encodeConfig->rcParams.rateControlMode = NvEncOptionHelper::ParseRateControlMode(FAppConfig::Get().EncoderRateControl);
        // initializeParams.encodeConfig->rcParams.multiPass = NvEncOptionHelper::ParseMultiPass(FAppConfig::Get().EncoderMultipass);
        // initializeParams.encodeConfig->rcParams.maxBitRate = FAppConfig::Get().EncoderMaxBitrate > -1 ? FAppConfig::Get().EncoderMaxBitrate : 1000000u;
        // initializeParams.encodeConfig->rcParams.averageBitRate =
        //     FAppConfig::Get().EncoderTargetBitrate > -1 ? FAppConfig::Get().EncoderTargetBitrate
        //                                                 : 1000000u;
        const uint32 MinQP = static_cast<uint32>(FAppConfig::Get().EncoderMinQP);
        const uint32 MaxQP = static_cast<uint32>(FAppConfig::Get().EncoderMaxQP);
        initializeParams.encodeConfig->rcParams.minQP = {MinQP, MinQP, MinQP};
        initializeParams.encodeConfig->rcParams.maxQP = {MaxQP, MaxQP, MaxQP};
        initializeParams.encodeConfig->rcParams.enableMinQP = FAppConfig::Get().EncoderMinQP > -1;
        initializeParams.encodeConfig->rcParams.enableMaxQP = FAppConfig::Get().EncoderMaxQP > -1;

        // // initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceMode = 0;
        // // initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceModeData = 0;
        // // initializeParams.encodeConfig->encodeCodecConfig.h264Config.outputFramePackingSEI = 1;
        // // initializeParams.encodeConfig->encodeCodecConfig.h264Config.outputRecoveryPointSEI = 1;
        // }

        if (EncoderGUID == NV_ENC_CODEC_H264_GUID) {
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.repeatSPSPPS = 1;
        }
        else
        {
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.repeatSPSPPS = 1;
        }

        EncodeCLIOptions.SetInitParams(&initializeParams, format);

        std::string parameterDesc = EncodeCLIOptions.FullParamToString(&initializeParams);
        // ML_LOG(LogAVEncoder, Log, "NvEnc Encoder Parameters:%s", parameterDesc.c_str());

        m_NvNecoder->CreateEncoder(&initializeParams);
        ML_LOG(VideoEncoderNvEnc, Verbose, "After Initialize");
    }

    void FVideoEncoderNvEnc::Shutdown()
    {
        // std::vector<uint8_t> Mp4Buffer = VideoConverter::WriteMp4Trailer();
        // if (Mp4Buffer.size() > 0) {
        //     std::ofstream test("output_once.mp4", std::ios_base::binary | std::ios_base::app);
        //     test.write(reinterpret_cast<const char*>(Mp4Buffer.data()), Mp4Buffer.size());
        //     ML_LOG(Temp, VeryVerbose, "mp4buffer.size=%d", Mp4Buffer.size());
        // }
        std::vector<uint8_t> vPacket;
        m_NvNecoder->EndEncode(vPacket);
        m_NvNecoder->DestroyEncoder();
        m_NvNecoder.reset();
    }

#if PLATFORM_WINDOWS
    void FVideoEncoderNvEnc::TransmitD3D11(TRefCountPtr<ID3D11Texture2D> pTexture, FBufferInfo BufferInfo)
    {
        const NvEncInputFrame *encoderInputFrame = m_NvNecoder->GetNextInputFrame();

        try
        {
            m_Converter->Convert(pTexture, encoderInputFrame);
        }
        catch (NVENCException e)
        {
            ML_LOGW(Main, Error, L"Crashed by NVENC:%hs", e.what());
            // FatalLog(L"Exception:%hs", e.what());
            return;
        }

        NV_ENC_PIC_PARAMS picParams = {};
        picParams.frameIdx = static_cast<decltype(picParams.frameIdx)>(BufferInfo.FrameIndex);
        picParams.inputTimeStamp = BufferInfo.TimestampRTP;
        if (BufferInfo.IsKeyFrame)
        {
            // ML_LOG(LogAVEncoder, Log, "Insert key frame.");
            picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
        }
        auto StartTs = GetTimestampUs();
        m_NvNecoder->EncodeFrame(EncodedBuffer, &picParams);
        auto FinishTs = GetTimestampUs();
        BufferInfo.StartTimestampUs = StartTs;
        BufferInfo.FinishTimestampUs = FinishTs;
        OnEncodedPacket(0, EncodedBuffer, BufferInfo);
    }

    void FVideoEncoderNvEnc::TransmitD3D12(TRefCountPtr<ID3D12Resource> pTexture, FBufferInfo BufferInfo, uint64 TextureMemorySize, HANDLE SharedHandle, uint32 SharedHandleNameSuffix)
    {
        if (!SharedHandle)
        {
            return;
        }

        const NvEncInputFrame *encoderInputFrame = m_NvNecoder->GetNextInputFrame();

        D3D12CudaConverter->SetTextureCudaD3D12(SharedHandle, pTexture, encoderInputFrame, TextureMemorySize, SharedHandleNameSuffix);
        NV_ENC_PIC_PARAMS picParams = {};
        picParams.frameIdx = static_cast<decltype(picParams.frameIdx)>(BufferInfo.FrameIndex);
        picParams.inputTimeStamp = BufferInfo.TimestampRTP;
        if (BufferInfo.IsKeyFrame)
        {
            // ML_LOG(LogAVEncoder, Log, "Insert key frame.");
            picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
        }
        auto StartTs = GetTimestampUs();
        m_NvNecoder->EncodeFrame(EncodedBuffer, &picParams);
        auto FinishTs = GetTimestampUs();
        BufferInfo.StartTimestampUs = StartTs;
        BufferInfo.FinishTimestampUs = FinishTs;
        if (OnEncodedPacket) {
            OnEncodedPacket(0, EncodedBuffer, BufferInfo);
        }
        static int framecount = 0;
        static bool bFirst = true;
        if (bFirst) {
            bFirst = false;
            VideoConverter::WriteMp4Header(FAppConfig::Get().Width, FAppConfig::Get().Height, FAppConfig::Get().WebRTCFps, BufferInfo.FinishTimestampUs);
            ML_LOG(Temp, Verbose, "Width=%d,Height=%d,Fps=%d",FAppConfig::Get().Width, FAppConfig::Get().Height, FAppConfig::Get().WebRTCFps);
            std::vector<uint8_t> HeaderBuffer = VideoConverter::GetMp4Buffer();
            std::string VideoContent = std::string(reinterpret_cast<const char*>( HeaderBuffer.data()), HeaderBuffer.size());
            PUBLISH_MESSAGE(OnSendVideo, std::move(VideoContent));
            std::ofstream test("output_once.mp4", std::ios_base::binary | std::ios_base::out);
            test.write(reinterpret_cast<const char*>(HeaderBuffer.data()), HeaderBuffer.size());
            ML_LOG(Temp, VeryVerbose, "mp4buffer.size=%d", HeaderBuffer.size());
        }
        VideoConverter::WriteMp4Frame(EncodedBuffer, BufferInfo.FinishTimestampUs, BufferInfo.IsKeyFrame);
        if (++framecount >= 3) {
            framecount = 0;
            std::vector<uint8_t> VideoBuffer = VideoConverter::GetMp4Buffer();
            std::ofstream test("output_once.mp4", std::ios_base::binary | std::ios_base::app);
            test.write(reinterpret_cast<const char*>(VideoBuffer.data()), VideoBuffer.size());
            ML_LOG(Temp, VeryVerbose, "mp4buffer.size=%d", VideoBuffer.size());
            std::string VideoContent = std::string(reinterpret_cast<const char*>(VideoBuffer.data()), VideoBuffer.size());
            PUBLISH_MESSAGE(OnSendVideo, std::move(VideoContent));
        }
        // static int frames = 0;
        // if (frames++ >= 2000) {
        //     std::vector<uint8_t> Mp4Buffer = VideoConverter::WriteMp4Trailer();
        //     if (Mp4Buffer.size() > 0) {
        //         std::cout << "trailer is not empty" << std::endl;
        //         ML_LOG(Temp, Error, "trailer is not empty!!!!!!!!!!");
        //         std::ofstream test("output_once.mp4", std::ios_base::binary | std::ios_base::app);
        //         test.write(reinterpret_cast<const char*>(Mp4Buffer.data()), Mp4Buffer.size());
        //         ML_LOG(Temp, VeryVerbose, "mp4buffer.size=%d", Mp4Buffer.size());
        //     } else {
        //         std::cout << "trailer is empty" << std::endl;
        //         ML_LOG(Temp, Error, "trailer is empty!!!!!!!!!!");
        //     }
        //     exit(0);
        // }
    }
#elif PLATFORM_LINUX
    void FVideoEncoderNvEnc::TransmitVulkan(FTexture2DRHIRef Texture2DRHIRef, FBufferInfo BufferInfo) {
        const NvEncInputFrame *encoderInputFrame = m_NvNecoder->GetNextInputFrame();
        auto StartTs = webrtc::Clock::GetRealTimeClock()->TimeInMilliseconds();
        VulkanCudaConverter->SetTextureCudaVulkan(Texture2DRHIRef, encoderInputFrame);
        NV_ENC_PIC_PARAMS picParams = {};
        picParams.frameIdx = static_cast<decltype(picParams.frameIdx)>(BufferInfo.FrameIndex);
        picParams.inputTimeStamp = BufferInfo.TimestampRTP;
        if (BufferInfo.IsKeyFrame)
        {
            // ML_LOG(LogAVEncoder, Log, "Insert key frame.");
            picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
        }
        m_NvNecoder->EncodeFrame(EncodedBuffer, &picParams);
        auto FinishTs = webrtc::Clock::GetRealTimeClock()->TimeInMilliseconds();
        BufferInfo.StartTimestampUs = StartTs;
        BufferInfo.FinishTimestampUs = FinishTs;
        OnEncodedPacket(0, EncodedBuffer, BufferInfo);
    }
#endif
    void FVideoEncoderNvEnc::Transmit(FTexture2DRHIRef Texture2DRHIRef, FBufferInfo BufferInfo)
    {
        LastTexture = Texture2DRHIRef;
#if PLATFORM_WINDOWS
        if (GDynamicRHI->GetName() == "D3D11")
        {
            auto D3D11Texture2D = static_cast<FD3D11Texture2D*>(Texture2DRHIRef->GetTexture2D());
            TransmitD3D11(D3D11Texture2D->GetResource(), BufferInfo);
        }
        else if (GDynamicRHI->GetName() == "D3D12")
        {
            auto D3D12Texture2D = static_cast<FD3D12Texture2D*>(Texture2DRHIRef->GetTexture2D());
            uint64 TextureMemorySize = Texture2DRHIRef->GetTextureMemorySize();
            HANDLE SharedHandle = Texture2DRHIRef->GetSharedHandle();
            uint32 SharedHandleNameSuffix = Texture2DRHIRef->GetSharedHandleName();
            TransmitD3D12(D3D12Texture2D->GetResource(), BufferInfo, TextureMemorySize, SharedHandle, SharedHandleNameSuffix);
        }
#elif PLATFORM_LINUX
        if (GDynamicRHI->GetName() == "Vulkan") {
            TransmitVulkan(Texture2DRHIRef, BufferInfo);
        }
#endif
    }

} // namespace OpenZI::CloudRender