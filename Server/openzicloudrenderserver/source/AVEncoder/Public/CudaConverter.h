/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3, (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.gnu.org/licenses/agpl-3.0.en.html#license-text
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/

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