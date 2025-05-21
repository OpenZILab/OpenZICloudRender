//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2023/07/18 10:42
//
#include "Vulkan/VulkanCudaConverter.h"
#include "Config.h"
#include "Logger/CommandLogger.h"
#include "RGBToNV12.cuh"
#include "VulkanDynamicRHI.h"
#include <unistd.h>
#include <fcntl.h>

#if MLCRS_WITH_WATER_MARK
#include "Config.h"
#include "SystemTime.h"
#include "WaterMark/LogoPixels.h"
#endif

namespace OpenZI::CloudRender {
    FVulkanCudaConverter::FVulkanCudaConverter(uint32 InWidth, uint32 InHeight)
        : Width(InWidth), Height(InHeight) {
#if MLCRS_WITH_WATER_MARK
        LogoMat = cv::Mat(200, 200, CV_8UC4, LogoPixels.data());
        TotalFrames = FAppConfig::Get().WebRTCFps * 10;
        StartTimestamp = GetNowTicks();
#endif
        InitCudaContext();
        cudaChannelFormatDesc desc =
            cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);
        cudaMallocArray(&DeviceTexture, &desc, Width, Height);

        // QueryPerformanceFrequency(&tc);
    }

    FVulkanCudaConverter::~FVulkanCudaConverter() {
        cuCtxDestroy(cudaContext);
        cudaFreeArray(DeviceTexture);
    }

    void FVulkanCudaConverter::InitCudaContext() {
        ML_LOG(VulkanCudaConverter, Log, "InitCudaContext");
        CUresult result = cuInit(0);
        if (result != CUDA_SUCCESS) {
            // throw MakeException(L"cuInit failed.");
        }
        int device_count = 0;
        CUresult deviceCountErr = cuDeviceGetCount(&device_count);
        for (int current_device = 0; current_device < device_count; ++current_device) {
            CUresult getDeviceErr = cuDeviceGet(&cuDevice, current_device);
            if (getDeviceErr != CUDA_SUCCESS) {
                ML_LOG(VulkanCuda, Warning, "Could not get CUDA device at device %d.", 0);
                continue;
            }
            char deviceNameArr[256];
            CUresult getNameErr = cuDeviceGetName(deviceNameArr, 256, cuDevice);
            if (getNameErr == CUDA_SUCCESS) {
                ML_LOG(VulkanCuda, Log, "Found device %d called \"%s\".", current_device,
                       deviceNameArr);
            } else {
                ML_LOG(VulkanCuda, Warning, "Could not get name of CUDA device %d.",
                       current_device);
            }
            // Compare the CUDA device with current RHI selected device.
            if (IsRHISelectedDevice(cuDevice)) {
                ML_LOG(VulkanCuda, Log, "Attempting to create CUDA context on GPU Device %d...",
                       current_device);

                CUresult createCtxErr = cuCtxCreate(&cudaContext, 0, current_device);
                if (createCtxErr == CUDA_SUCCESS) {
                    ML_LOG(VulkanCuda, Log, "Created CUDA context on device \"%s\".",
                           deviceNameArr);
                    break;
                } else {
                    ML_LOG(VulkanCuda, Error,
                           "CUDA module could not create CUDA context on RHI device \"%s\".",
                           deviceNameArr);
                    return;
                }
            } else {
                ML_LOG(VulkanCuda, Log, "CUDA device %d \"%s\" does not match RHI device.",
                       current_device, deviceNameArr);
            }
        }
    }
    bool FVulkanCudaConverter::IsRHISelectedDevice(CUdevice IncuDevice) {
        CUuuid cudaDeviceUUID;
        CUresult getUuidErr = cuDeviceGetUuid(&cudaDeviceUUID, IncuDevice);

        if (getUuidErr != CUDA_SUCCESS) {
            ML_LOG(VulkanCuda, Warning, "Could not get CUDA device UUID at device %d.", IncuDevice);
            return false;
        }

        auto VulkanDynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI.get());
        auto& IDProps = VulkanDynamicRHI->GetIDProps();
        return memcmp(&cudaDeviceUUID, IDProps.deviceUUID, VK_UUID_SIZE) == 0;
    }

    CUcontext FVulkanCudaConverter::GetContext() { return cudaContext; }

    void FVulkanCudaConverter::SetTextureCudaVulkan(FTexture2DRHIRef Texture2DRHIRef,
                                                    const NvEncInputFrame* DstResource) {
        FScopeLock Lock(&SourcesGuard);
        // if (CacheTextures.Num() >= 500) {
        //     CacheTextures.Empty();
        // }
        // CacheTextures.Enqueue(Texture2DRHIRef);
        // LastTexture = Texture2DRHIRef;
        auto VulkanTexture2D = static_cast<FVulkanTexture2D*>(Texture2DRHIRef->GetTexture2D());
        uint64 TextureMemorySize = Texture2DRHIRef->GetTextureMemorySize();
        int VulkanTextureHandle = Texture2DRHIRef->GetSharedHandle();
        // ML_LOG(VulkanCuda, Verbose, "In Cuda fd=%d", VulkanTextureHandle);
        uint32 TextureMemoryOffset = VulkanTexture2D->GetTextureMemoryOffset();
        if (fcntl(VulkanTextureHandle, F_GETFD) != 0) {
            // ML_LOG(VulkanCuda, Error, "Failed to open fd %d", VulkanTextureHandle);
            return;
        }
        // QueryPerformanceCounter(&t1);
        CUresult Result = cuCtxPushCurrent(cudaContext);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(VulkanCuda, Error, "Failed to push cuda context");
            return;
        }

        CUexternalMemory MappedExternalMemory = nullptr;

        // generate a cudaExternalMemoryHandleDesc
        CUDA_EXTERNAL_MEMORY_HANDLE_DESC CudaExtMemHandleDesc = {};
        CudaExtMemHandleDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
        CudaExtMemHandleDesc.handle.fd = VulkanTextureHandle;
        CudaExtMemHandleDesc.size = TextureMemorySize;
        // CudaExtMemHandleDesc.flags |= CUDA_EXTERNAL_MEMORY_DEDICATED;

        // import external memory
        Result = cuImportExternalMemory(&MappedExternalMemory, &CudaExtMemHandleDesc);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(VulkanCuda, Error, "Failed to import external memory from vulkan error: %d, fd: %d",
                   Result, VulkanTextureHandle);
            // close(VulkanTextureHandle);
            cuCtxPopCurrent(NULL);
            return;
        }

        CUmipmappedArray MappedMipArray = nullptr;
        CUarray MappedArray = nullptr;
        CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC MipmapDesc = {};
        MipmapDesc.numLevels = 1;
        // need offset  UE: VulkanTexture->Surface.GetAllocationOffset();
        MipmapDesc.offset = TextureMemoryOffset;
        MipmapDesc.arrayDesc.Width = Width;
        MipmapDesc.arrayDesc.Height = Height;
        MipmapDesc.arrayDesc.Depth = 0;
        MipmapDesc.arrayDesc.NumChannels = 4;
        MipmapDesc.arrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
        MipmapDesc.arrayDesc.Flags = CUDA_ARRAY3D_SURFACE_LDST | CUDA_ARRAY3D_COLOR_ATTACHMENT;

        // get the CUarray from the external memory
        Result = cuExternalMemoryGetMappedMipmappedArray(&MappedMipArray, MappedExternalMemory,
                                                         &MipmapDesc);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(VulkanCuda, Error, "Failed to bind mipmappedArray error: %d", Result);
            if (MappedExternalMemory) {
                Result = cuDestroyExternalMemory(MappedExternalMemory);
                if (Result != CUDA_SUCCESS) {
                    ML_LOG(VulkanCuda, Error, "Failed to destroy MappedExternalMemoryArray: %d",
                           Result);
                }
                MappedExternalMemory = nullptr;
            }
            cuCtxPopCurrent(NULL);
            return;
        }

        // get the CUarray from the external memory
        Result = cuMipmappedArrayGetLevel(&MappedArray, MappedMipArray, 0);
        if (Result != CUDA_SUCCESS) {
            ML_LOG(VulkanCuda, Error, "Failed to bind to mip 0.");
            if (MappedMipArray) {
                Result = cuMipmappedArrayDestroy(MappedMipArray);
                if (Result != CUDA_SUCCESS) {
                    ML_LOG(VulkanCuda, Error, "Failed to destroy MappedMipArray: %d", Result);
                }
                MappedMipArray = nullptr;
            }

            if (MappedExternalMemory) {
                Result = cuDestroyExternalMemory(MappedExternalMemory);
                if (Result != CUDA_SUCCESS) {
                    ML_LOG(VulkanCuda, Error, "Failed to destroy MappedExternalMemoryArray: %d",
                           Result);
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
        CurrentTimestamp = GetNowTicks();
        if (GetTotalSeconds(CurrentTimestamp - StartTimestamp) > 1800) {
        cuStatus = BGRA2NV12_WithWaterMark(DeviceTexture, (uint8_t*)DstResource->inputPtr,
                                           DstResource->pitch, Width, Height, Logo.data, 4,
                                           LogoWidth, LogoHeight, LocationX, LocationY);
        } else {
            cuStatus = BGRA2NV12(DeviceTexture, (uint8_t*)DstResource->inputPtr,
            DstResource->pitch,
                                 Width, Height);
        }

#else
        cudaArray* SrcImage = reinterpret_cast<cudaArray*>(MappedArray);
        cudaMemcpyArrayToArray(DeviceTexture, 0, 0, SrcImage, 0, 0, Width * Height * 4);
        cuStatus = BGRA2NV12(DeviceTexture, (uint8_t*)DstResource->inputPtr, DstResource->pitch,
                             Width, Height);
#endif

        if (MappedArray) {
            Result = cuArrayDestroy(MappedArray);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(VulkanCuda, Error, "Failed to destroy MappedArray: %d", Result);
            }
            MappedArray = nullptr;
        }

        if (MappedMipArray) {
            Result = cuMipmappedArrayDestroy(MappedMipArray);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(VulkanCuda, Error, "Failed to destroy MappedMipArray: %d", Result);
            }
            MappedMipArray = nullptr;
        }

        if (MappedExternalMemory) {
            Result = cuDestroyExternalMemory(MappedExternalMemory);
            if (Result != CUDA_SUCCESS) {
                ML_LOG(VulkanCuda, Error, "Failed to destroy MappedExternalMemoryArray: %d",
                       Result);
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
} // namespace OpenZI::CloudRender