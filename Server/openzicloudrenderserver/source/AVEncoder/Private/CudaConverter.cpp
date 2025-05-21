//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/01 18:24
//

#include "CudaConverter.h"
#include "RGBToNV12.cuh"
#include "Logger/CommandLogger.h"

#if MLCRS_WITH_WATER_MARK
#include "Config.h"
#include "WaterMark/LogoPixels.h"
#include "SystemTime.h"
#endif


namespace OpenZI::CloudRender
{
#define MakeException(Msg)                                                                         \
    NVENCException::makeNVENCException(Msg, NV_ENC_SUCCESS, __FUNCTION__, __FILE__, __LINE__)

#if MLCRS_WITH_WATER_MARK
    std::string GetLogoFilePath()
    {
        char *Path = nullptr;
        Path = _getcwd(nullptr, 1);
        puts(Path);

        std::string FilePath(Path);
        FilePath.append("\\logo.png");
        delete Path;
        Path = nullptr;
        return FilePath;
    }
#endif

    CudaConverter::CudaConverter(TRefCountPtr<ID3D11Device> device, int width, int height)
        : Width(width), Height(height), m_registered(false)
    {
#if MLCRS_WITH_WATER_MARK
        LogoMat = cv::Mat(200, 200, CV_8UC4, LogoPixels.data());
        TotalFrames = FAppConfig::Get().WebRTCFps * 10;
        StartTimestamp = FSystemTime::GetInSeconds();
#endif
        InitCudaContext(device);
    }

    CudaConverter::~CudaConverter()
    {
        cudaGraphicsUnregisterResource(m_cudaResource);
        // cudaFree(m_cudaLinearMemory);
        cuCtxDestroy(m_cuContext);
    }

    CUcontext CudaConverter::GetContext()
    {
        return m_cuContext;
    }

    void CudaConverter::Convert(const TRefCountPtr<ID3D11Texture2D> &texture, const NvEncInputFrame *encoderInputFrame, bool bBGRA)
    {
        FScopeLock Lock(&SourcesGuard);
        if (!texture.IsValid()) {
            ML_LOG(Main, Error, "Texture is not valid");
            return;
        }
        cudaError cuStatus;

        CUresult result = cuCtxPushCurrent(m_cuContext);
        if (result != CUDA_SUCCESS)
        {
            ML_LOG(Main, Error, "Failed to push cuda context");
            throw MakeException("cuCtxPushCurrent failed.");
            return;
        }

        RegisterTexture(texture);
        if (m_cudaResource == nullptr) {
            // ML_LOG(Main, Error, "Cuda resource is empty");
            return;
        }
        
        cuStatus = cudaGraphicsMapResources(1, &m_cudaResource, 0);
        if (cuStatus != cudaSuccess)
        {
            ML_LOG(Main, Error, "Failed to map resource");
            throw MakeException("cudaGraphicsMapResources failed.");
            result = cuCtxPopCurrent(NULL);
            return;
        }

        cudaArray *cuArray;
        cuStatus = cudaGraphicsSubResourceGetMappedArray(&cuArray, m_cudaResource, 0, 0);
        if (cuStatus != cudaSuccess)
        {
            ML_LOG(Main, Error, "Failed to get mapped array");
            throw MakeException("cudaGraphicsSubResourceGetMappedArray failed.");
            cuStatus = cudaGraphicsUnmapResources(1, &m_cudaResource, 0);
            result = cuCtxPopCurrent(NULL);
            return;
        }
#if MLCRS_WITH_WATER_MARK
        static uint32 WaterMarkCounter = 0;
        static bool bUpOrder = true;
        if (bUpOrder)
        {
            ++WaterMarkCounter;
            if (WaterMarkCounter >= TotalFrames)
            {
                bUpOrder = false;
            }
        }
        else
        {
            --WaterMarkCounter;
            if (WaterMarkCounter <= 0)
            {
                bUpOrder = true;
            }
        }
        static auto BaseRatio = 16.f / 9.f;
        double Multip = (double)WaterMarkCounter / TotalFrames;
        auto NewHWRatio = FAppConfig::Get().ResolutionX * 1.f / FAppConfig::Get().ResolutionY;
        auto LogoScale = NewHWRatio >= BaseRatio ? FAppConfig::Get().ResolutionY * 1.f / Height : FAppConfig::Get().ResolutionX * 1.f / Width;
        auto WidthMultip = Width / 1920.0 / (FAppConfig::Get().ResolutionX * 1.f / Width) * LogoScale;
        auto HeightMultip = Height / 1080.0 / (FAppConfig::Get().ResolutionY * 1.f / Height) * LogoScale;
        LogoWidth = uint32(200 * WidthMultip);
        LogoHeight = uint32(200 * HeightMultip);
        cv::resize(LogoMat, Logo, cv::Size(LogoWidth, LogoHeight));
        uint32 LocationX = static_cast<uint32>((Width - LogoWidth) * Multip);
        uint32 LocationY = static_cast<uint32>((Height - LogoHeight) * Multip);
        CurrentTimestamp = FSystemTime::GetInSeconds();
        if (CurrentTimestamp - StartTimestamp > 1800) {
            cuStatus = bBGRA ? BGRA2NV12_WithWaterMark(cuArray, (uint8_t*)encoderInputFrame->inputPtr, encoderInputFrame->pitch, Width, Height, Logo.data, 4, LogoWidth, LogoHeight, LocationX, LocationY) : RGBA2NV12(cuArray, (uint8_t*)encoderInputFrame->inputPtr, encoderInputFrame->pitch, Width, Height);
        } else {
            cuStatus = bBGRA ? BGRA2NV12(cuArray, (uint8_t *)encoderInputFrame->inputPtr, encoderInputFrame->pitch, Width, Height) : RGBA2NV12(cuArray, (uint8_t *)encoderInputFrame->inputPtr, encoderInputFrame->pitch, Width, Height);
        }
#else
        cuStatus = bBGRA ? BGRA2NV12(cuArray, (uint8_t *)encoderInputFrame->inputPtr, encoderInputFrame->pitch, Width, Height) : RGBA2NV12(cuArray, (uint8_t *)encoderInputFrame->inputPtr, encoderInputFrame->pitch, Width, Height);
#endif

        if (cuStatus != cudaSuccess)
        {
            throw MakeException("Cuda kernel execution failed.");
            ML_LOG(Main, Error, "ProcessLowLevelTexture failed.");
            cuStatus = cudaGraphicsUnmapResources(1, &m_cudaResource, 0);
            result = cuCtxPopCurrent(NULL);
            return;
        }

        cuStatus = cudaGraphicsUnmapResources(1, &m_cudaResource, 0);
        if (cuStatus != cudaSuccess)
        {
            ML_LOG(Main, Error, "DestroyLowLevelTexture failed.");
            throw MakeException("cudaGraphicsUnmapResources failed.");
            result = cuCtxPopCurrent(NULL);
            return;
        }

        result = cuCtxPopCurrent(NULL);
        if (result != CUDA_SUCCESS)
        {
            throw MakeException("cuCtxPopCurrent failed.");
            ML_LOG(Main, Error, "PopLowLevelTexture failed.");
            return;
        }
    }

    void CudaConverter::InitCudaContext(TRefCountPtr<ID3D11Device > device)
    {
        TRefCountPtr<IDXGIDevice> DXGIDevice;
        TRefCountPtr<IDXGIAdapter> DXGIAdapter;

        HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), (void**)DXGIDevice.GetInitReference());
        if (FAILED(hr))
        {
            throw MakeException("Failed to query IDXGIDevice");
        }

        hr = DXGIDevice->GetAdapter(DXGIAdapter.GetInitReference());
        if (FAILED(hr))
        {
            throw MakeException("Failed to get IDXGIAdapter");
        }
        int cuDevice;
        cudaError cuStatus = cudaD3D11GetDevice(&cuDevice, DXGIAdapter.GetReference());
        if (cuStatus != cudaSuccess)
        {
            throw MakeException("Failed to get CUDA device.");
        }

        CUresult result = cuInit(0);
        if (result != CUDA_SUCCESS)
        {
            throw MakeException("cuInit failed.");
        }

        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, cuDevice);
        // Log(L"Using CUDA Device %d: %hs\n", cuDevice, deviceProp.name);

        result = cuCtxCreate(&m_cuContext, 0, cuDevice);
        if (result != CUDA_SUCCESS)
        {
            ML_LOG(Main, Error, "Failed to create cuda context.");
            throw MakeException("Failed to create CUDA context.");
        }
    }

    void CudaConverter::RegisterTexture(const TRefCountPtr<ID3D11Texture2D> &texture)
    {
        cudaGraphicsResource *cudaResource;
        if (m_registered)
        {
            cudaGraphicsUnregisterResource(m_cudaResource);
            if (m_cudaResource)
            {
                m_cudaResource = nullptr;
            }
        }
        cudaError cuStatus = cudaGraphicsD3D11RegisterResource(&cudaResource, texture.GetReference(), cudaGraphicsRegisterFlagsNone);
        
        m_cudaResource = cudaResource;
        m_registered = true;
        if (cuStatus != cudaSuccess)
        {
            // ML_LOG(Main, Error, "Failed to register resource, cudaError = %d", cuStatus);
            m_cudaResource = nullptr;
            m_registered = false;
            // throw MakeException("cudaGraphicsD3D11RegisterResource failed.");
        }
    }
} // namespace OpenZI::CloudRender