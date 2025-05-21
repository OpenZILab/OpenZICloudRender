//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/29 12:24
//

#pragma once
#include <exception>
#include <d3d11.h>
#include <cuda.h>

#include <cuda_runtime_api.h>
#include <cuda_d3d11_interop.h>
#include "NvEncoder.h"
#include "Thread/CriticalSection.h"
#include "Containers/RefCounting.h"
#include "CoreMinimal.h"

#if MLCRS_WITH_WATER_MARK
#include "opencv2/opencv.hpp"
#include <direct.h>
#endif

namespace OpenZI::CloudRender
{

    class CudaConverter
    {
    public:
        CudaConverter(TRefCountPtr<ID3D11Device> device, int width, int height);
        ~CudaConverter();

        CUcontext GetContext();
        void Convert(const TRefCountPtr<ID3D11Texture2D > &texture, const NvEncInputFrame *encoderInputFrame, bool bBGRA = true);
        void InitCudaContext(TRefCountPtr<ID3D11Device> device);
        void RegisterTexture(const TRefCountPtr<ID3D11Texture2D> &texture);

    private:
        CUcontext m_cuContext;
        bool m_registered;
        cudaGraphicsResource *m_cudaResource;
        FCriticalSection SourcesGuard;
        void *m_cudaLinearMemory;
        size_t m_pitch;
        const int Width;
        const int Height;
#if MLCRS_WITH_WATER_MARK
        cv::Mat Logo;
        cv::Mat LogoMat;
        uint32 LogoWidth;
        uint32 LogoHeight;
        uint32 TotalFrames;
        uint32 RenderWidth;
        uint32 RenderHeight;
        double StartTimestamp = 0;
        double CurrentTimestamp = 0;
#endif
    };
} // namespace OpenZI::CloudRender