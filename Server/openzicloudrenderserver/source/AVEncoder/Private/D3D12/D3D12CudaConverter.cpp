//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/24 17:01
//

#include "D3D12/D3D12CudaConverter.h"
#include "D3D12DynamicRHI.h"
#include "Logger/CommandLogger.h"
#include "RGBToNV12.cuh"

#if MLCRS_WITH_WATER_MARK
#include "Config.h"
#include "SystemTime.h"
#include "WaterMark/LogoPixels.h"
#endif

namespace OpenZI::CloudRender {
    FD3D12CudaConverter::FD3D12CudaConverter(uint32 InWidth, uint32 InHeight)
        : Width(InWidth), Height(InHeight) {
#if MLCRS_WITH_WATER_MARK
        LogoMat = cv::Mat(200, 200, CV_8UC4, LogoPixels.data());
        TotalFrames = FAppConfig::Get().WebRTCFps * 10;
        StartTimestamp = FSystemTime::GetInSeconds();
#endif
        InitCudaContext();
        cudaChannelFormatDesc desc =
            cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);
        cudaMallocArray(&DeviceTexture, &desc, Width, Height);

        // QueryPerformanceFrequency(&tc);
    }

    FD3D12CudaConverter::~FD3D12CudaConverter() {
        // CUresult Result = cuCtxPushCurrent(cudaContext);
        // if (Result == CUDA_SUCCESS) {
        //     DestroyCudaResources();
        //     cuCtxPopCurrent(NULL);
        // }

        cuCtxDestroy(cudaContext);
        cudaFreeArray(DeviceTexture);
    }

    void FD3D12CudaConverter::InitCudaContext() {
        ML_LOG(D3D12CudaConverter, Log, "InitCudaContext");
        CUresult result = cuInit(0);
        if (result != CUDA_SUCCESS) {
            // throw MakeException(L"cuInit failed.");
        }
        int device_count = 0;
        CUresult deviceCountErr = cuDeviceGetCount(&device_count);
        for (int current_device = 0; current_device < device_count; ++current_device) {
            CUresult getDeviceErr = cuDeviceGet(&cuDevice, current_device);
            if (getDeviceErr != CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Warning, "Could not get CUDA device at device %d.",
                       0);
                continue;
            }
            char deviceNameArr[256];
            CUresult getNameErr = cuDeviceGetName(deviceNameArr, 256, cuDevice);
            if (getNameErr == CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Log, "Found device %d called \"%s\".", current_device,
                       deviceNameArr);
            } else {
                ML_LOG(LogD3D12CudaConverter, Warning, "Could not get name of CUDA device %d.",
                       current_device);
            }
            // Compare the CUDA device with current RHI selected device.
            if (IsRHISelectedDevice(cuDevice)) {
                ML_LOG(LogD3D12CudaConverter, Log,
                       "Attempting to create CUDA context on GPU Device %d...", current_device);

                CUresult createCtxErr = cuCtxCreate(&cudaContext, 0, current_device);
                if (createCtxErr == CUDA_SUCCESS) {
                    ML_LOG(LogD3D12CudaConverter, Log, "Created CUDA context on device \"%s\".",
                           deviceNameArr);
                    break;
                } else {
                    ML_LOG(LogD3D12CudaConverter, Error,
                           TEXT("CUDA module could not create CUDA context on RHI device \"%s\"."),
                           deviceNameArr);
                    return;
                }
            } else {
                ML_LOG(LogD3D12CudaConverter, Log,
                       TEXT("CUDA device %d \"%s\" does not match RHI device."), current_device,
                       deviceNameArr);
            }
        }
    }
    bool FD3D12CudaConverter::IsRHISelectedDevice(CUdevice IncuDevice) {
        char* DeviceLuid = new char[64];
        unsigned int DeviceNodeMask;
        CUresult getLuidErr = cuDeviceGetLuid(DeviceLuid, &DeviceNodeMask, IncuDevice);
        if (getLuidErr != CUDA_SUCCESS) {
            ML_LOG(LogD3D12CudaConverter, Warning,
                   TEXT("Could not get CUDA device LUID at device %d."), IncuDevice);
            delete DeviceLuid;
            return false;
        }

        TRefCountPtr<ID3D12Device> D3D12Device =
            static_cast<FD3D12DynamicRHI*>(GDynamicRHI.get())->GetDevice();

        LUID AdapterLUID = D3D12Device->GetAdapterLuid();
        bool bSelected =
            ((memcmp(&AdapterLUID.LowPart, DeviceLuid, sizeof(AdapterLUID.LowPart)) == 0) &&
             (memcmp(&AdapterLUID.HighPart, DeviceLuid + sizeof(AdapterLUID.LowPart),
                     sizeof(AdapterLUID.HighPart)) == 0));
        delete DeviceLuid;
        return bSelected;
    }

    CUcontext FD3D12CudaConverter::GetContext() { return cudaContext; }

    void FD3D12CudaConverter::SetTextureCudaD3D12(HANDLE D3D12TextureHandle,
                                                  const TRefCountPtr<ID3D12Resource>& SrcResource,
                                                  const NvEncInputFrame* DstResource,
                                                  uint64 TextureMemorySize, bool bBGRA,
                                                  uint32 SharedHandleNameSuffix) {
        FScopeLock Lock(&SourcesGuard);
        if (!D3D12TextureHandle)
            return;

        // QueryPerformanceCounter(&t1);
        CUresult Result = cuCtxPushCurrent(cudaContext);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(LogD3D12CudaConverter, Error, "Failed to push cuda context");
            return;
        }


        // generate a cudaExternalMemoryHandleDesc
        CUDA_EXTERNAL_MEMORY_HANDLE_DESC CudaExtMemHandleDesc = {};
        CudaExtMemHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE;
        CudaExtMemHandleDesc.handle.win32.name = NULL;
        CudaExtMemHandleDesc.handle.win32.handle = D3D12TextureHandle;
        CudaExtMemHandleDesc.size = TextureMemorySize;
        // Necessary for committed resources (DX11 and committed DX12 resources)
        CudaExtMemHandleDesc.flags |= CUDA_EXTERNAL_MEMORY_DEDICATED;

        // import external memory
        Result = cuImportExternalMemory(&MappedExternalMemory, &CudaExtMemHandleDesc);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(LogD3D12CudaConverter, Error,
                   "Failed to import external memory from d3d12 error: %d", Result);
            cuCtxPopCurrent(NULL);
            return;
        }

        CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC MipmapDesc = {};
        MipmapDesc.numLevels = 1;
        MipmapDesc.offset = 0;
        MipmapDesc.arrayDesc.Width = Width;
        MipmapDesc.arrayDesc.Height = Height;
        MipmapDesc.arrayDesc.Depth = 1;
        MipmapDesc.arrayDesc.NumChannels = 4;
        MipmapDesc.arrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
        MipmapDesc.arrayDesc.Flags = CUDA_ARRAY3D_SURFACE_LDST | CUDA_ARRAY3D_COLOR_ATTACHMENT;

        // get the CUarray from the external memory
        Result = cuExternalMemoryGetMappedMipmappedArray(&MappedMipArray, MappedExternalMemory,
                                                         &MipmapDesc);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(LogD3D12CudaConverter, Error, "Failed to bind mipmappedArray error: %d", Result);
            if (MappedExternalMemory) {
                Result = cuDestroyExternalMemory(MappedExternalMemory);
                if (Result != CUDA_SUCCESS) {
                    ML_LOG(LogD3D12CudaConverter, Error,
                           "Failed to destroy MappedExternalMemoryArray: %d", Result);
                }
                MappedExternalMemory = nullptr;
            }
            cuCtxPopCurrent(NULL);
            return;
        }

        // get the CUarray from the external memory
        Result = cuMipmappedArrayGetLevel(&MappedArray, MappedMipArray, 0);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(LogD3D12CudaConverter, Error, "Failed to bind to mip 0.");
            if (MappedMipArray) {
                Result = cuMipmappedArrayDestroy(MappedMipArray);
                if (Result != CUDA_SUCCESS) {
                    ML_LOG(LogD3D12CudaConverter, Error, "Failed to destroy MappedMipArray: %d",
                           Result);
                }
                MappedMipArray = nullptr;
            }

            if (MappedExternalMemory) {
                Result = cuDestroyExternalMemory(MappedExternalMemory);
                if (Result != CUDA_SUCCESS) {
                    ML_LOG(LogD3D12CudaConverter, Error,
                           "Failed to destroy MappedExternalMemoryArray: %d", Result);
                }
                MappedExternalMemory = nullptr;
            }
            cuCtxPopCurrent(NULL);
            return;
        }

        cudaError cuStatus;
#if MLCRS_WITH_WATER_MARK
        static uint32 WaterMarkCounter = 0;
        static bool bUpOrder = true;
        if (bUpOrder) {
            ++WaterMarkCounter;
            if (WaterMarkCounter >= TotalFrames) {
                bUpOrder = false;
            }
        } else {
            --WaterMarkCounter;
            if (WaterMarkCounter <= 0) {
                bUpOrder = true;
            }
        }
        static auto BaseRatio = 16.f / 9.f;
        double Multip = (double)WaterMarkCounter / TotalFrames;
        auto NewHWRatio = FAppConfig::Get().ResolutionX * 1.f / FAppConfig::Get().ResolutionY;
        auto LogoScale = NewHWRatio >= BaseRatio ? FAppConfig::Get().ResolutionY * 1.f / Height
                                                 : FAppConfig::Get().ResolutionX * 1.f / Width;
        auto WidthMultip =
            Width / 1920.0 / (FAppConfig::Get().ResolutionX * 1.f / Width) * LogoScale;
        auto HeightMultip =
            Height / 1080.0 / (FAppConfig::Get().ResolutionY * 1.f / Height) * LogoScale;
        LogoWidth = uint32(200 * WidthMultip);
        LogoHeight = uint32(200 * HeightMultip);

        cv::resize(LogoMat, Logo, cv::Size(LogoWidth, LogoHeight));
        uint32 LocationX = static_cast<uint32>((Width - LogoWidth) * Multip);
        uint32 LocationY = static_cast<uint32>((Height - LogoHeight) * Multip);
        /*
         * The types ::CUarray and struct ::cudaArray * represent the same data type and may be
         * used interchangeably by casting the two types between each other.
         *
         * In order to use a ::CUarray in a CUDA Runtime API function which takes a struct
         * ::cudaArray *, it is necessary to explicitly cast the ::CUarray to a struct
         * ::cudaArray *.
         *
         * In order to use a struct ::cudaArray * in a CUDA Driver API function which takes a
         * ::CUarray, it is necessary to explicitly cast the struct ::cudaArray * to a ::CUarray
         * .
         */

        cudaArray* SrcImage = reinterpret_cast<cudaArray*>(MappedArray);
        cudaMemcpyArrayToArray(DeviceTexture, 0, 0, SrcImage, 0, 0, Width * Height * 4);
        CurrentTimestamp = FSystemTime::GetInSeconds();
        if (CurrentTimestamp - StartTimestamp > 1800) {
            cuStatus = BGRA2NV12_WithWaterMark(DeviceTexture, (uint8_t*)DstResource->inputPtr,
                                               DstResource->pitch, Width, Height, Logo.data, 4,
                                               LogoWidth, LogoHeight, LocationX, LocationY);
        } else {
            cuStatus = BGRA2NV12(DeviceTexture, (uint8_t*)DstResource->inputPtr, DstResource->pitch,
                                 Width, Height);
        }

#else
        cuStatus = BGRA2NV12(DeviceTexture, (uint8_t*)DstResource->inputPtr, DstResource->pitch,
                             Width, Height);
#endif

        if (MappedArray) {
            Result = cuArrayDestroy(MappedArray);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Error, "Failed to destroy MappedArray: %d", Result);
            }
            MappedArray = nullptr;
        }

        if (MappedMipArray) {
            Result = cuMipmappedArrayDestroy(MappedMipArray);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Error, "Failed to destroy MappedMipArray: %d",
                       Result);
            }
            MappedMipArray = nullptr;
        }

        if (MappedExternalMemory) {
            Result = cuDestroyExternalMemory(MappedExternalMemory);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Error,
                       "Failed to destroy MappedExternalMemoryArray: %d", Result);
            }
            MappedExternalMemory = nullptr;
        }

        Result = cuCtxPopCurrent(NULL);
        // QueryPerformanceCounter(&t2);
        // auto ProcessTimeMs = (t2.QuadPart - t1.QuadPart) * 1000.0 / tc.QuadPart;
        // ML_LOG(LogPixelStreaming, Log, "time(ms): %.4f", ProcessTimeMs);
        if (Result != CUDA_SUCCESS) {
            // throw MakeException(L"cuCtxPopCurrent failed.");
            return;
        }
        // }
    }

    void FD3D12CudaConverter::DestroyCudaResources() {
        CUresult Result;
        if (MappedArray) {
            Result = cuArrayDestroy(MappedArray);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Error, "Failed to destroy MappedArray: %d", Result);
            }
            MappedArray = nullptr;
        }

        if (MappedMipArray) {
            Result = cuMipmappedArrayDestroy(MappedMipArray);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Error, "Failed to destroy MappedMipArray: %d",
                       Result);
            }
            MappedMipArray = nullptr;
        }

        if (MappedExternalMemory) {
            Result = cuDestroyExternalMemory(MappedExternalMemory);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(LogD3D12CudaConverter, Error,
                       "Failed to destroy MappedExternalMemoryArray: %d", Result);
            }
            MappedExternalMemory = nullptr;
        }
    }
} // namespace OpenZI::CloudRender