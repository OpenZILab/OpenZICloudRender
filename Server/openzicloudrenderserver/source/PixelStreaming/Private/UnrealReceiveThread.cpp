//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/16 16:45
//
#include "UnrealReceiveThread.h"
#include "Config.h"
#include "Containers/RefCounting.h"
#include "D3D11DynamicRHI.h"
#include "D3D12DynamicRHI.h"
#include "DataConverter.h"
#include "IPCTypes.h"
#include "Logger/CommandLogger.h"
#include "MessageCenter/MessageCenter.h"
#include <chrono>
#include <thread>

#if PLATFORM_LINUX
#include "VulkanDynamicRHI.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <vulkan/vulkan.h>

#endif

namespace OpenZI::CloudRender {

    FUnrealVideoReceiver::FUnrealVideoReceiver() : bExiting(false) {
        std::string ShareMemoryNameTemp =
            "OpenZICloudRenderServer_" + FAppConfig::Get().ShareMemorySuffix;
        ShareMemoryName = FDataConverter::ToUTF8(ShareMemoryNameTemp);
        // ML_LOG(LogPixelStreaming, Error, "ShareMemoryName=%s", ShareMemoryName.c_str());
#if PLATFORM_WINDOWS
        Reader = std::make_unique<FShareMemoryReader>(ShareMemoryName.c_str());
        if (!Reader->Check()) {
            ML_LOG(LogPixelStreaming, Error, "Failed to open shm, guid = %d",
                   FAppConfig::Get().Guid);
            std::exit(1);
        }
#elif PLATFORM_LINUX
        // key_t ShmKey = FAppConfig::Get().Guid - 2;
        // Reader = std::make_unique<FShareMemoryReader>(ShmKey, 64);
        // ML_LOG(Main, Log, "new UnrealVideoReceiver");
#endif
    }

    FUnrealVideoReceiver::~FUnrealVideoReceiver() {
        bExiting = true;
        Join();
    }

    void FUnrealVideoReceiver::Run() {
        // SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_MOST_URGENT);
        uint32 Suffix = 0;
        FShareMemoryCallback Callback;
        FShareMemoryData* ReadData = nullptr;
#if PLATFORM_WINDOWS
        HANDLE SharedHandle = nullptr;
        std::shared_ptr<FDynamicRHI> DynamicRHI = GDynamicRHI;
        std::shared_ptr<FD3D11DynamicRHI> D3D11DynamicRHI;
        std::shared_ptr<FD3D12DynamicRHI> D3D12DynamicRHI;
        if (GDynamicRHI->GetName() == "D3D11") {
            D3D11DynamicRHI = std::shared_ptr<FD3D11DynamicRHI>(
                static_cast<FD3D11DynamicRHI*>(GDynamicRHI.get()));
        } else if (GDynamicRHI->GetName() == "D3D12") {
            D3D12DynamicRHI = std::shared_ptr<FD3D12DynamicRHI>(
                static_cast<FD3D12DynamicRHI*>(GDynamicRHI.get()));
        }
#elif PLATFORM_LINUX
        // int SharedHandle = 0;
        // std::shared_ptr<FVulkanDynamicRHI> VulkanDynamicRHI;
        // if (GDynamicRHI->GetName() == "Vulkan") {
        //     VulkanDynamicRHI = std::shared_ptr<FVulkanDynamicRHI>(
        //         static_cast<FVulkanDynamicRHI*>(GDynamicRHI.get()));
        // }
#endif

#if OpenZI_DUMP_TEXTURE_PROCESSTIME
        LARGE_INTEGER t1, t2, tc;
        QueryPerformanceFrequency(&tc);
#endif
        while (!bExiting) {
#if OpenZI_DUMP_TEXTURE_PROCESSTIME
            QueryPerformanceCounter(&t1);
#endif
            if (Reader->Read(ReadData, &Callback) >= 1) {
                FTexture2DRHIRef SharedTexture;
#if PLATFORM_WINDOWS
                if (GDynamicRHI->GetName() == "D3D11" &&
                    SharedHandle != Reader->GetSharedHandle()) {
                    uint32 SizeX;
                    uint32 SizeY;
                    Reader->GetSizeXY(SizeX, SizeY);
                    FAppConfig::Get().ResolutionX = *(uint32*)ReadData;
                    FAppConfig::Get().ResolutionY = *(uint32*)(ReadData + sizeof(uint32));
                    if (FAppConfig::Get().bEnableDynamicFps) {
                        uint32 UnrealFps = *(uint32*)(ReadData + sizeof(uint32) * 2);
                        UnrealFps = UnrealFps < 60 ? UnrealFps : 60;
                        if (UnrealFps < 1) {
                            UnrealFps = 1;
                            FAppConfig::Get().WebRTCFps = UnrealFps;
                        }
                    }
                    FAppConfig::Get().Width = SizeX;
                    FAppConfig::Get().Height = SizeY;
                    SharedHandle = Reader->GetSharedHandle();
                    SharedTexture = D3D11DynamicRHI->GetSharedTexture(SharedHandle);
                    if (SharedTexture.IsValid()) {
                        PUBLISH_MESSAGE(OnTextureReceived, SharedTexture);
                    }
                } else if (GDynamicRHI->GetName() == "D3D12") {
                    uint32 SizeX;
                    uint32 SizeY;
                    Reader->GetSizeXY(SizeX, SizeY);
                    FAppConfig::Get().Width = SizeX;
                    FAppConfig::Get().Height = SizeY;
                    FAppConfig::Get().ResolutionX = *(uint32*)ReadData;
                    FAppConfig::Get().ResolutionY = *(uint32*)(ReadData + sizeof(uint32));
                    // ML_LOG(LogReceive, Log, "Res=(%d,%d)", FAppConfig::Get().ResolutionX,
                    // FAppConfig::Get().ResolutionY);
                    uint32 ReceiveSuffix = *(uint32*)(ReadData + sizeof(uint32) * 2);
                    uint32 ReceiveTextureMemorySize = *(uint32*)(ReadData + sizeof(uint32) * 3);
                    if (FAppConfig::Get().bEnableDynamicFps) {
                        uint32 UnrealFps = *(uint32*)(ReadData + sizeof(uint32) * 4);
                        UnrealFps = UnrealFps < 60 ? UnrealFps : 60;
                        if (UnrealFps < 1) {
                            UnrealFps = 1;
                            FAppConfig::Get().WebRTCFps = UnrealFps;
                        }
                    }
                    if (Suffix != ReceiveSuffix) {
                        Suffix = ReceiveSuffix;
                        SharedHandle = Reader->GetSharedHandle();
                        SharedTexture = D3D12DynamicRHI->GetSharedTextureBySuffix(
                            ReceiveSuffix, SharedHandle, ReceiveTextureMemorySize);
                        if (SharedHandle) {
                            PUBLISH_MESSAGE(OnTextureReceived, SharedTexture);
                        }
                    }
                }
#elif PLATFORM_LINUX
                // if (GDynamicRHI->GetName() == "Vulkan" && FdCaches.Num() > 0) {
                //     uint32 SizeX;
                //     uint32 SizeY;
                //     Reader->GetSizeXY(SizeX, SizeY);
                //     FAppConfig::Get().Width = SizeX;
                //     FAppConfig::Get().Height = SizeY;
                //     uint32 ReceiveTextureMemorySize = *(uint32*)ReadData;
                //     uint32 ReceiveTextureMemoryOffset = *(uint32*)(ReadData + sizeof(uint32));
                //     uint32 ReceiveSuffix = *(uint32*)(ReadData + sizeof(uint32) * 2);
                //     FAppConfig::Get().ResolutionX = *(uint32*)(ReadData + sizeof(uint32) * 3);
                //     FAppConfig::Get().ResolutionY = *(uint32*)(ReadData + sizeof(uint32) * 4);
                //     int FdCache = 0;
                //     FdCaches.Dequeue(FdCache);
                //     SharedTexture = VulkanDynamicRHI->GetTextureRef(
                //         FdCache, ReceiveTextureMemorySize, ReceiveTextureMemoryOffset);
                //     if (SharedTexture) {
                //         PUBLISH_MESSAGE(OnTextureReceived, SharedTexture);
                //     }

                //     // TODO: remove test
                //     if (false) {
                //         auto device = VulkanDynamicRHI->GetDevice();
                //         // auto device = GVulkanDevice;
                //         if (!device) {
                //             ML_LOG(Main, Verbose, "Failed to get device");
                //         }
                //         VkDeviceMemory textureMemory;
                //         int textureFd = SharedHandle;
                //         VkResult result = VK_SUCCESS;

                //         //   VkMemoryGetFdInfoKHR getFdInfo{};
                //         //   getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
                //         //   getFdInfo.memory = textureMemory;
                //         //   getFdInfo.handleType =
                //         //   VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

                //         //   ML_LOG(Main, Verbose, "Ready to get memory");
                //         //   VkResult result =
                //         //       VulkanDynamicRHI->vkGetMemoryFdKHR(device, &getFdInfo, &textureFd);
                //         //   if (result != VK_SUCCESS) {
                //         //       ML_LOG(Main, Error, "Failed to get external memory vulkan
                //         //       texture");
                //         //   } else {
                //         uint64 textureMemorySize = ReceiveTextureMemorySize;
                //         VkImportMemoryFdInfoKHR importMemoryInfo{};
                //         importMemoryInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
                //         importMemoryInfo.handleType =
                //             VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
                //         importMemoryInfo.fd = textureFd;
                //         importMemoryInfo.pNext = nullptr;

                //         VkMemoryRequirements memoryRequirements;
                //         VkImageCreateInfo imageCreateInfo = {
                //             .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                //             .imageType = VK_IMAGE_TYPE_2D,
                //             .format = VK_FORMAT_R8G8B8A8_UNORM,
                //             .extent = {SizeX, SizeY, 1},
                //             .mipLevels = 1,
                //             .arrayLayers = 1,
                //             .samples = VK_SAMPLE_COUNT_1_BIT,
                //             .tiling = VK_IMAGE_TILING_OPTIMAL,
                //             .usage =
                //                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                //             .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                //             .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
                //         VkImage TempImage;
                //         result = vkCreateImage(device, &imageCreateInfo, nullptr, &TempImage);
                //         if (result != VK_SUCCESS) {
                //             ML_LOG(Main, Error, "Failed to create  temp image %d", result);
                //         } else {
                //             vkGetImageMemoryRequirements(device, TempImage, &memoryRequirements);
                //             vkDestroyImage(device, TempImage, nullptr);
                //         }

                //         uint32 memoryTypeIndex = VulkanDynamicRHI->FindMemoryTypeIndex(
                //             memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                //         VkMemoryAllocateInfo allocateInfo{};
                //         allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                //         allocateInfo.pNext = &importMemoryInfo;
                //         allocateInfo.allocationSize = textureMemorySize;
                //         allocateInfo.memoryTypeIndex = memoryTypeIndex;
                //         result = vkAllocateMemory(device, &allocateInfo, nullptr, &textureMemory);
                //         if (result != VK_SUCCESS) {
                //             ML_LOG(Main, Error, "Failed to allocate memory vulkan texture, %d",
                //                    result);
                //         } else {
                //             VkImageCreateInfo imageCreateInfo{};
                //             VkImage textureImage;
                //             result =
                //                 vkCreateImage(device, &imageCreateInfo, nullptr, &textureImage);
                //             if (result != VK_SUCCESS) {
                //                 ML_LOG(Main, Error, "Failed to create image vulkan texture, %d",
                //                        result);
                //                 vkFreeMemory(device, textureMemory, nullptr);
                //             } else {
                //                 vkBindImageMemory(device, textureImage, textureMemory, 0);
                //                 // TODO: present image

                //                 vkDestroyImage(device, textureImage, nullptr);
                //                 vkFreeMemory(device, textureMemory, nullptr);
                //                 ML_LOG(Main, Log, "Success create image");
                //             }
                //         }
                //     }
                // }
                // //  }

#endif
            } else {
                // ML_LOG(UnrealReceiveThread, Error, "Failed to read data from share memory");
            }
#if OpenZI_DUMP_TEXTURE_PROCESSTIME
            QueryPerformanceCounter(&t2);
            auto ProcessTimeMs = (t2.QuadPart - t1.QuadPart) * 1000.0 / tc.QuadPart;
            ML_LOG(LogPixelStreaming, Log, "Process texture time(us): %f", ProcessTimeMs * 1000);
#endif
            std::this_thread::sleep_for(std::chrono::microseconds(10)); // 睡眠1微秒
        }
        Callback.Free(ReadData);
    }

    FUnrealAudioReceiver::FUnrealAudioReceiver() : bExiting(false) {
        std::string ShareMemoryNameTemp = "OpenZICloudAudio_" + FAppConfig::Get().ShareMemorySuffix;
        ShareMemoryName = FDataConverter::ToUTF8(ShareMemoryNameTemp);
        // ML_LOG(LogPixelStreaming, Error, "ShareMemoryName=%s", ShareMemoryName.c_str());
#if PLATFORM_WINDOWS
        Reader = std::make_unique<FShareMemoryReader>(ShareMemoryName.c_str());
#elif PLATFORM_LINUX
        key_t ShmKey = FAppConfig::Get().Guid - 3;
        Reader = std::make_unique<FShareMemoryWriter>(ShmKey, 64 * 1024);
#endif
        if (!Reader->Check()) {
            ML_LOG(LogPixelStreaming, Error, "Falied to read shm, guid = %d",
                   FAppConfig::Get().Guid);
            std::exit(1);
        } else {
            // ML_LOG(Audio, Log, "------------------------------------Create Audio Thread");
        }
    }

    FUnrealAudioReceiver::~FUnrealAudioReceiver() {
        bExiting = true;
        Join();
    }

    void FUnrealAudioReceiver::Run() {
        // SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_MOST_URGENT);
#if PLATFORM_WINDOWS
        HANDLE SharedHandle = nullptr;
#elif PLATFORM_LINUX
        int SharedHandle = 0;
#endif
        uint32 Suffix = 0;
        FShareMemoryCallback Callback;
        FShareMemoryData* ReadData = new FShareMemoryData[64 * 1024];
        while (!bExiting) {
#if OpenZI_DUMP_TEXTURE_PROCESSTIME
            QueryPerformanceCounter(&t1);
#endif
            if (Reader->Read(ReadData, &Callback) >= 1) {
                if (SharedHandle != Reader->GetSharedHandle()) {
                    SharedHandle = Reader->GetSharedHandle();
                    // ML_LOG(Main, Log, "Received audio data, size: %d Bytes, Suffix = %d",
                    // Reader->GetContentSize() / 8, SharedHandle);
                    PUBLISH_MESSAGE(OnAudioReceived, Reader->GetContent(),
                                    Reader->GetContentSize());
                }

            } else {
                // ML_LOG(UnrealReceiveThread, Error, "Failed to read data from share memory");
            }
#if OpenZI_DUMP_TEXTURE_PROCESSTIME
            QueryPerformanceCounter(&t2);
            auto ProcessTimeMs = (t2.QuadPart - t1.QuadPart) * 1000.0 / tc.QuadPart;
            ML_LOG(LogPixelStreaming, Log, "Process texture time(us): %f", ProcessTimeMs * 1000);
#endif
            std::this_thread::sleep_for(std::chrono::microseconds(1)); // 睡眠1微秒
        }
        Callback.Free(ReadData);
    }
} // namespace OpenZI::CloudRender