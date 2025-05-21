//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/24 16:59
//

#pragma once
#include <cuda.h>
#include <d3d12.h>

#include "Containers/RefCounting.h"
#include "CoreMinimal.h"
#include "Logger/CommandLogger.h"
#include "NvEncoder.h"
#include "Thread/CriticalSection.h"
#include <cuda_runtime_api.h>

#if MLCRS_WITH_WATER_MARK
#include "opencv2/opencv.hpp"
#endif

namespace OpenZI::CloudRender {

    class FD3D12CudaConverter {
    public:
        FD3D12CudaConverter(uint32 InWidth, uint32 InHeight);
        ~FD3D12CudaConverter();

        CUcontext GetContext();
        void InitCudaContext();

        void SetTextureCudaD3D12(HANDLE D3D12TextureHandle,
                                 const TRefCountPtr<ID3D12Resource>& SrcResource,
                                 const NvEncInputFrame* DstResource, uint64 TextureMemorySize,
                                 bool bBGRA = true, uint32 SharedHandleNameSuffix = 0);

    private:
        bool IsRHISelectedDevice(CUdevice IncuDevice);
        void DestroyCudaResources();
        CUdevice cuDevice;
        CUcontext cudaContext;
        FCriticalSection SourcesGuard;
        const uint32 Width;
        const uint32 Height;
        cudaArray* DeviceTexture;
#if MLCRS_WITH_WATER_MARK
        cv::Mat Logo;
        cv::Mat LogoMat;
        uint32 LogoWidth;
        uint32 LogoHeight;
        uint32 TotalFrames;
        double StartTimestamp = 0;
        double CurrentTimestamp = 0;
#endif
        // LARGE_INTEGER t1, t2, tc;
        CUexternalMemory MappedExternalMemory = nullptr;
        CUmipmappedArray MappedMipArray = nullptr;
        CUarray MappedArray = nullptr;
    };
} // namespace OpenZI::CloudRender