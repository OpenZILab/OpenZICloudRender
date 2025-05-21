/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/
#include "SendThread.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"
#include "IPCTypes.h"
#include "OpenZICloudRenderModule.h"
#include <string>

#include "TextureSource.h"

namespace OpenZI::CloudRender {
    FSendThread::FSendThread()
        : Thread(nullptr), ThreadName(TEXT("OpenZICloudRenderSendThread")), MaxCache(60) {
        StartThread();
    }

    FSendThread::FSendThread(FGuid Guid, uint32 SizeX, uint32 SizeY, uint32 InMaxCache) {

        FParse::Value(FCommandLine::Get(), TEXT("OpenZILab="), CVarExplicitAdapterValue);

        if (SizeX > 3840) {
            UE_LOG(LogTemp, Warning,
                   TEXT("Received 'FrameBufferSizeX' is %d, The width of the encoded texture "
                        "cannot exceed 3840, will use 3840 width"),
                   SizeX);
            SizeX = 3840;
        }
        if (SizeY > 2160) {
            UE_LOG(LogTemp, Warning,
                   TEXT("Received 'FrameBufferSizeY' is %d, The height of the encoded texture "
                        "cannot exceed 2160, will use 2160 height"),
                   SizeY);
            SizeY = 2160;
        }
        ShmId = Guid;
        FrameBufferSizeX = SizeX;
        FrameBufferSizeY = SizeY;
        Thread = nullptr;
        FString IdStr = ShmId.ToString();
        ThreadName = FString::Printf(TEXT("%s_%s"), TEXT("OpenZICloudRenderSendThread"), *IdStr);
        ShareMemoryName = FString::Printf(TEXT("%s_%s"), TEXT("OpenZICloudRenderServer"), *IdStr);
        ShareHandlePrefix = IdStr;
        MaxCache = InMaxCache;
#if PLATFORM_LINUX
        std::string SocketName = "/tmp/openzicloud";
        FdSocketServer = new ZFdSocketServer(SocketName.data());
#endif
        StartThread();
    }

    FSendThread::~FSendThread() { StopThread(); }

    void FSendThread::StartThread() {
        Thread = FRunnableThread::Create(this, *ThreadName, 0, EThreadPriority::TPri_Highest);
    }

    void FSendThread::StopThread() {
        if (Thread != nullptr) {
            Thread->Kill(true);
            delete Thread;
            Thread = nullptr;
        }
    }

    bool FSendThread::Init() {
        TextureSource =
            MakeShared<FTextureSourceBackBuffer>(1.0, FrameBufferSizeX, FrameBufferSizeY, this);
#if PLATFORM_WINDOWS
        char* Name = TCHAR_TO_UTF8(*ShareMemoryName);
        Writer = MakeShareable(new FShareMemoryWriter(Name, 64));
#elif PLATFORM_LINUX
        // key_t ShmKey = ShmId.A - 2;
        // Writer = MakeShareable(new FShareMemoryWriter(ShmKey, 64));
#endif
        // if (!Writer->Check()) {
        //     UE_LOG(LogTemp, Error, TEXT("FShareMemoryWriter init failed."));
        //     return false;
        // }
        RHIName = GDynamicRHI->GetName();
        ExitRequest.Set(true);
        return false;
    }

    uint32 FSendThread::Run() { return 0; }
    void FSendThread::Stop() { ExitRequest.Set(true); }

    void FSendThread::Exit() {
        // empty
    }

    void FSendThread::SendTextureHandleToShareMemory(FTexture2DRHIRef FrameBuffer) {
        FRHITexture2D* TextureRHI = FrameBuffer.GetReference();
        if (RHIName == TEXT("D3D11")) {
#if PLATFORM_WINDOWS
#if (ENGINE_MAJOR_VERSION <= 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
            FD3D11Texture2D* Texture = static_cast<FD3D11Texture2D*>(TextureRHI);
            ID3D11Texture2D* D3D11Texture = static_cast<ID3D11Texture2D*>(Texture->GetResource());
            TRefCountPtr<IDXGIResource> OtherResource(nullptr);
            if (D3D11Texture) {
                D3D11Texture->QueryInterface(__uuidof(IDXGIResource), (void**)&OtherResource);
            }
            if (OtherResource) {
                HANDLE SharedHandle = nullptr;
                if (OtherResource->GetSharedHandle(&SharedHandle) == S_OK &&
                    SharedHandle != SharedHandleCache) {
                    SharedHandleCache = SharedHandle;
                    // if (Cache.Num() >= MaxCache) {
                    //     Cache.RemoveAt(0);
                    // }
                    // Cache.Emplace(D3D11Texture);
                    FIntPoint TextureSize = Texture->GetSizeXY();
                    FIntPoint Resolution;
                    if (GEngine) {
                        Resolution = GEngine->GetGameUserSettings()->GetScreenResolution();
                    }
                    extern ENGINE_API float GAverageFPS;
                    TArray<uint32> Data;
                    Data.Add(Resolution.X);
                    Data.Add(Resolution.Y);
                    Data.Add((uint32)GAverageFPS);
                    Writer->Write((FShareMemoryData*)Data.GetData(), Data.Num() * sizeof(uint32),
                                  TextureSize.X, TextureSize.Y, SharedHandle);
                }
            }
#endif
#endif
        }
#if PLATFORM_WINDOWS
#if ENGINE_MAJOR_VERSION >= 5
        else if (RHIName == TEXT("D3D12")) {
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0)
            FD3D12Texture2D* D3D12Texture = static_cast<FD3D12Texture2D*>(TextureRHI);
            uint32 TextureMemorySize = (uint32)D3D12Texture->GetMemorySize();
#else
            FD3D12Texture* D3D12Texture = static_cast<FD3D12Texture*>(TextureRHI);
            auto D3D12DynamicRHI = static_cast<ID3D12DynamicRHI*>(GDynamicRHI);
            uint32 TextureMemorySize = D3D12DynamicRHI->RHIGetResourceMemorySize(TextureRHI);
#endif
            TRefCountPtr<ID3D12Resource> D3D12Resource = D3D12Texture->GetResource()->GetResource();
            TRefCountPtr<ID3D12Device> Device(nullptr);
            if (D3D12Resource) {
                D3D12Resource->GetDevice(IID_PPV_ARGS(Device.GetInitReference()));
            }
            if (Device) {
                HANDLE SharedHandle = nullptr;
                static uint32 Suffix = 0;
                FString HandleName = FString::Printf(TEXT("%s_%d"), *ShareHandlePrefix, ++Suffix);
                if (Device->CreateSharedHandle(D3D12Resource.GetReference(), nullptr, GENERIC_ALL,
                                               *HandleName, &SharedHandle) == S_OK) {
                    if (D3D12SharedHandleCaches.Num() >= MaxCache) {
                        // D3D12ResourceCaches.RemoveAt(0);
                        CloseHandle(D3D12SharedHandleCaches[0]);
                        D3D12SharedHandleCaches.RemoveAt(0);
                    }
                    D3D12SharedHandleCaches.Emplace(SharedHandle);
                    // D3D12ResourceCaches.Emplace(D3D12Resource);
                    FIntPoint TextureSize = D3D12Texture->GetSizeXY();
                    FIntPoint Resolution = GEngine->GetGameUserSettings()->GetScreenResolution();
                    extern ENGINE_API float GAverageFPS;
                    TArray<uint32> Data;
                    Data.Add(Resolution.X);
                    Data.Add(Resolution.Y);
                    Data.Add(Suffix);
                    Data.Add(TextureMemorySize);
                    Data.Add((uint32)GAverageFPS);
                    Writer->Write((FShareMemoryData*)Data.GetData(), Data.Num() * sizeof(uint32),
                                  TextureSize.X, TextureSize.Y, SharedHandle);
                }
            }
        }
#endif
#endif

#if PLATFORM_LINUX
        else if (RHIName == TEXT("Vulkan")) {
            if (CVarExplicitAdapterValue == 1) {
                return;
            }
            static uint32 Suffix = 0;
#if (ENGINE_MAJOR_VERSION == 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
            FVulkanTexture2D* VulkanTexture = static_cast<FVulkanTexture2D*>(TextureRHI);
#else
            FVulkanTexture* VulkanTexture = static_cast<FVulkanTexture*>(TextureRHI);
#endif
            VkDevice Device =
                static_cast<FVulkanDynamicRHI*>(GDynamicRHI)->GetDevice()->GetInstanceHandle();

            int Fd = 0;
            // TODO: maybe not shared, so try to change code
            VkMemoryGetFdInfoKHR MemoryGetFdInfoKHR = {};
            MemoryGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
            MemoryGetFdInfoKHR.pNext = NULL;
#if (ENGINE_MAJOR_VERSION == 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
            MemoryGetFdInfoKHR.memory = VulkanTexture->Surface.GetAllocationHandle();
#else
            MemoryGetFdInfoKHR.memory = VulkanTexture->GetAllocationHandle();
#endif
            MemoryGetFdInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#pragma warning(push)
#pragma warning(disable : 4191)
            PFN_vkGetMemoryFdKHR FPGetMemoryFdKHR =
                (PFN_vkGetMemoryFdKHR)VulkanRHI::vkGetDeviceProcAddr(Device, "vkGetMemoryFdKHR");
            VkResult Result = FPGetMemoryFdKHR(Device, &MemoryGetFdInfoKHR, &Fd);
            if (Result != VK_SUCCESS) {
                UE_LOG(LogTemp, Error,
                       TEXT("OpenZICloudRender failed to get vulkan texture memory fd khr"));
            } else {
                if (VulkanFdCaches.Num() >= MaxCache) {
                    int ToCloseFd = VulkanFdCaches[0];
                    VulkanFdCaches.RemoveAt(0);
                    // if (fcntl(ToCloseFd, F_GETFD) == 0) {
                    close(ToCloseFd);
                        // UE_LOG(LogTemp, Verbose, TEXT("close fd %d"), ToCloseFd);
                    // }
                }
                VulkanFdCaches.Emplace(Fd);
                if (CVarExplicitAdapterValue == 2) {
                    return;
                }
#if (ENGINE_MAJOR_VERSION == 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
                uint64 MemorySize = VulkanTexture->Surface.GetAllocationOffset() +
                                    VulkanTexture->Surface.GetMemorySize();
                uint64 MemoryOffset = VulkanTexture->Surface.GetAllocationOffset();
#else
                uint64 MemorySize = VulkanTexture->GetAllocationOffset() +
                                    VulkanTexture->GetMemorySize();
                uint64 MemoryOffset = VulkanTexture->GetAllocationOffset();
#endif
                FIntPoint Resolution = GEngine->GetGameUserSettings()->GetScreenResolution();
                TArray<uint32> Data;
                FIntPoint TextureSize = VulkanTexture->GetSizeXY();
                Data.Add(MemorySize);
                Data.Add(MemoryOffset);
                Data.Add(Resolution.X);
                Data.Add(Resolution.Y);
                Data.Add(TextureSize.X);
                Data.Add(TextureSize.Y);
                // Writer->Write((FShareMemoryData*)Data.GetData(), Data.Num() * sizeof(uint32),
                // TextureSize.X, TextureSize.Y,
                //               Fd);
                char* pData = (char*)Data.GetData();
                int pSize = Data.Num() * sizeof(uint32);
                if (CVarExplicitAdapterValue == 3) {
                    return;
                }
                FdSocketServer->Send(Fd, pData, pSize);
            }
#pragma warning(pop)
        }
#endif
    }

} // namespace OpenZI::CloudRender