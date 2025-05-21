//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2023/07/18 10:42
//

#pragma once

#include <cuda.h>

#include "CoreMinimal.h"
#include "Logger/CommandLogger.h"
#include "VulkanResources.h"
#include "NvEncoder.h"
#include "Thread/CriticalSection.h"
#include <cuda_runtime_api.h>

#if MLCRS_WITH_WATER_MARK
#include "opencv2/opencv.hpp"
#endif

namespace OpenZI::CloudRender {

    class FVulkanCudaConverter {
    public:
        FVulkanCudaConverter(uint32 InWidth, uint32 InHeight);
        ~FVulkanCudaConverter();

        CUcontext GetContext();
        void InitCudaContext();

        void SetTextureCudaVulkan(FTexture2DRHIRef Texture2DRHIRef,
                                  const NvEncInputFrame* DstResource);

    private:
        bool IsRHISelectedDevice(CUdevice IncuDevice);
        CUdevice cuDevice;
        CUcontext cudaContext;
        FCriticalSection SourcesGuard;
        const uint32 Width;
        const uint32 Height;
        cudaArray* DeviceTexture;
        FTexture2DRHIRef LastTexture;
        TQueue<FTexture2DRHIRef> CacheTextures;
#if MLCRS_WITH_WATER_MARK
        cv::Mat Logo;
        cv::Mat LogoMat;
        uint32 LogoWidth;
        uint32 LogoHeight;
        uint32 TotalFrames;
        int64 StartTimestamp = 0;
        int64 CurrentTimestamp = 0;
#endif
        // LARGE_INTEGER t1, t2, tc;
    };
} // namespace OpenZI::CloudRender