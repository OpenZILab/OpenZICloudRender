
//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/21 11:39
//

#include "VulkanDynamicRHI.h"
#include "Config.h"
#include "Containers/FormatString.h"
#include "Logger/CommandLogger.h"
#include "NvCodecUtils.h"
#include "NvEncoder.h"
#include <sstream>

namespace OpenZI::CloudRender {

    VkDevice GVulkanDevice = VK_NULL_HANDLE;

    uint32 FVulkanDynamicRHI::FindMemoryTypeIndex(uint32 memoryTypeBits,
                                                  VkMemoryPropertyFlags requiredProperties) {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memoryProperties);

        for (uint32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags &
                                                requiredProperties) == requiredProperties) {
                return i;
            }
        }

        return UINT32_MAX;
    }

    FVulkanDynamicRHI::FVulkanDynamicRHI() : Name("Vulkan") { CreateInstance(); }

    FVulkanDynamicRHI::~FVulkanDynamicRHI() {
        if (Device) {
            vkDestroyDevice(Device, nullptr);
            ML_LOG(VulkanRHI, Log, "Destroy vulkan Device");
        }
        if (Instance) {
            vkDestroyInstance(Instance, nullptr);
            ML_LOG(VulkanRHI, Log, "Destroy vulkan instance");
        }
    }

    void FVulkanDynamicRHI::CreateInstance() {
        VkInstanceCreateInfo InstInfo;
        ZeroVulkanStruct(InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        std::vector<const char*> instanceExtensionNames = {
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
            // VK_KHR_XCB_SURFACE_EXTENSION_NAME,
            // VK_KHR_XLIB_SURFACE_EXTENSION_NAME
            /*"VK_KHR_xlib_surface"*/};
        InstInfo.ppEnabledExtensionNames = &instanceExtensionNames[0];
        InstInfo.enabledExtensionCount = instanceExtensionNames.size();
        VkResult Result = vkCreateInstance(&InstInfo, nullptr, &Instance);
        if (Result == VK_SUCCESS) {
            ML_LOG(VulkanRHI, Log, "CreateInstance success");
        }

        uint32 GpuCount = 0;
        vkEnumeratePhysicalDevices(Instance, &GpuCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(GpuCount);
        vkEnumeratePhysicalDevices(Instance, &GpuCount, physicalDevices.data());
        for (const auto& physicalDevice : physicalDevices) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            ML_LOG(VulkanRHI, Log, "    PhysicalDeviceProps.DeviceName = %s",
                   properties.deviceName);
            ML_LOG(VulkanRHI, Log, "    PhysicalDeviceProps.DeviceID = %d", properties.deviceID);
            ML_LOG(VulkanRHI, Log, "    -------------------");
        }

        std::vector<uint8> pipelineUUID;
        std::string UUID = FAppConfig::Get().PipelineUUID;
        std::stringstream ss(UUID);
        std::string token;
        while (getline(ss, token, ',')) {
            uint8 number = static_cast<uint8>(std::stoi(token));
            pipelineUUID.emplace_back(number);
            // ML_LOG(VulkanRHI, Log, "token = %d", number);
        }

        if (pipelineUUID.size() != VK_UUID_SIZE) {
            ML_LOG(VulkanRHI, Error,
                   "Failed to parse str uuid because parsed array size %d is not equal %d",
                   pipelineUUID.size(), VK_UUID_SIZE);
        }

        CreateVulkanDevice(FAppConfig::Get().AdapterDeviceId, pipelineUUID.data());
    }

    // void FVulkanDynamicRHI::SelectAndInitDevice() {
    //     uint32 GpuCount = 0;
    //     VkReulst Result = vkEnumeratePhysicalDevices(Instance, &GpuCount, nullptr);
    //     if (Result == VK_SUCCESS) {
    //         ML_LOG(VulkanRHI, Log, "Enumerate physical devices success");
    //     }

    //     std::vector<VkPhysicalDevice> PhysicalDevices;
    //     PhysicalDevices.assign(GpuCount, nullptr);
    //     vkEnumeratePhysicalDevices(Instance, &GpuCount, PhysicalDevices.data());

    //     struct FDeviceInfo {
    //         VkDevice Device;
    //         VkPhysicalDevice Gpu;
    //         VkPhysicalDeviceProperties GpuProps;
    //         uint32 DeviceIndex;
    //     };

    //     std::vector<FDeviceInfo> DiscreteDevices;
    //     std::vector<FDeviceInfo> IntegratedDevices;
    //     std::vector<FDeviceInfo> OriginalOrderedDevices;

    //     ML_LOG(VulkanRHI, Log, "Found %d Device(s)", GpuCount);
    //     for (uint32 Index = 0; Index < GpuCount; ++Index) {

    //     }

    // }
    void FVulkanDynamicRHI::CreateVulkanDevice(uint32 targetDeviceId, uint8* targetDeviceUUID) {
        VkPhysicalDevice physicalDevice = FindPhysicalDevice(targetDeviceId, targetDeviceUUID);
        if (physicalDevice == VK_NULL_HANDLE) {
            ML_LOG(VulkanRHI, Error, "Failed to create vulkan Device");
            return;
        } else {
            PhysicalDevice = physicalDevice;
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            ML_LOG(VulkanRHI, Log, "    Chosen Adapter :");
            ML_LOG(VulkanRHI, Log, "      PhysicalDeviceProps.DeviceName = %s",
                   properties.deviceName);
            ML_LOG(VulkanRHI, Log, "      PhysicalDeviceProps.DeviceID = %d", properties.deviceID);
            ZeroVulkanStruct(PhysicalDeviceProperties2,
                             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR);
            PhysicalDeviceProperties2.pNext = &PhysicalDeviceIDProperties;
            ZeroVulkanStruct(PhysicalDeviceIDProperties,
                             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR);
            auto vkGetPhysicalDeviceProperties2KHR =
                reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(
                    vkGetInstanceProcAddr(Instance, "vkGetPhysicalDeviceProperties2KHR"));
            if (vkGetPhysicalDeviceProperties2KHR) {
                ML_LOG(VulkanRHI, Log, "Get vkGetPhysicalDeviceProperties2KHR");
                vkGetPhysicalDeviceProperties2KHR(physicalDevice, &PhysicalDeviceProperties2);
            }
        }
        VkPhysicalDeviceFeatures PhysicalFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &PhysicalFeatures);
        PhysicalFeatures.robustBufferAccess = VK_TRUE;
        VkDeviceCreateInfo DeviceInfo;
        ZeroVulkanStruct(DeviceInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        DeviceInfo.pEnabledFeatures = &PhysicalFeatures;
        // TODO: enable share extensions, UE: VulkanDevice.cpp 248 line
        std::vector<char const*> ExtensionsNames = {
            VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
            VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_KHR_MAINTENANCE2_EXTENSION_NAME,
            VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
            VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
            VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME,
            VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
            VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME};
        DeviceInfo.enabledExtensionCount = ExtensionsNames.size();
        DeviceInfo.ppEnabledExtensionNames = &ExtensionsNames[0];

        {
            // QueueFamily
            // 获取支持的队列族属性
            // VkQueueFamilyProperties queueFamilyProperties;
            // uint32_t queueFamilyCount;
            // vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
            // VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(
            //     queueFamilyCount * sizeof(VkQueueFamilyProperties));
            // vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
            //                                          queueFamilies);

            uint32 QueueCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &QueueCount, nullptr);
            std::vector<VkQueueFamilyProperties> QueueFamilyProps;
            QueueFamilyProps.assign(QueueCount, VkQueueFamilyProperties{});
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &QueueCount,
                                                     QueueFamilyProps.data());

            int GRHIAllowAsyncComputeCvar = 0;
            int GAllowPresentOnComputeQueue = 0;

            std::vector<VkDeviceQueueCreateInfo> QueueFamilyInfos;
            int32 GfxQueueFamilyIndex = -1;
            int32 ComputeQueueFamilyIndex = -1;
            int32 TransferQueueFamilyIndex = -1;
            ML_LOG(VulkanRHI, Log, "Found %d Queue Families", QueueFamilyProps.size());
            uint32 NumPriorities = 0;
            for (int32 FamilyIndex = 0; FamilyIndex < QueueFamilyProps.size(); ++FamilyIndex) {
                const VkQueueFamilyProperties& CurrProps = QueueFamilyProps[FamilyIndex];
                bool bIsValidQueue = false;
                if ((CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT) {
                    if (GfxQueueFamilyIndex == -1) {
                        GfxQueueFamilyIndex = FamilyIndex;
                        bIsValidQueue = true;
                    } else {
                    }
                }

                if ((CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT) {
                    if (ComputeQueueFamilyIndex == -1 && GfxQueueFamilyIndex != FamilyIndex &&
                        (GRHIAllowAsyncComputeCvar != 0 ||
                         GAllowPresentOnComputeQueue != 0) /*&& TODO: UE has other code*/) {
                        ComputeQueueFamilyIndex = FamilyIndex;
                        bIsValidQueue = true;
                    }
                }

                if ((CurrProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT) {
                    if (TransferQueueFamilyIndex == -1 &&
                        (CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != VK_QUEUE_GRAPHICS_BIT &&
                        (CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT) {
                        TransferQueueFamilyIndex = FamilyIndex;
                        bIsValidQueue = true;
                    }
                }

                if (!bIsValidQueue) {
                    ML_LOG(VulkanRHI, Log, "Skipping unnecessary Queue Family %d: %d queuesEnum %d",
                           FamilyIndex, CurrProps.queueCount, CurrProps.queueFlags);
                    continue;
                }

                int32 QueueIndex = QueueFamilyInfos.size();
                QueueFamilyInfos.emplace_back(VkDeviceQueueCreateInfo());
                VkDeviceQueueCreateInfo& CurrQueue = QueueFamilyInfos[QueueIndex];
                CurrQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                CurrQueue.queueFamilyIndex = FamilyIndex;
                CurrQueue.queueCount = CurrProps.queueCount;
                NumPriorities += CurrProps.queueCount;
                ML_LOG(VulkanRHI, Log, "Initializing Queue Family %d: %d queuesEnum %d",
                       FamilyIndex, CurrProps.queueCount, CurrProps.queueFlags);
            }

            std::vector<float> QueuePriorities;
            QueuePriorities.assign(NumPriorities, 0);
            float* CurrentPriority = QueuePriorities.data();
            for (int32 Index = 0; Index < QueueFamilyInfos.size(); ++Index) {
                VkDeviceQueueCreateInfo& CurrQueue = QueueFamilyInfos[Index];
                CurrQueue.pQueuePriorities = CurrentPriority;

                const VkQueueFamilyProperties& CurrProps =
                    QueueFamilyProps[CurrQueue.queueFamilyIndex];
                for (int32 QueueIndex = 0; QueueIndex < (int32)CurrProps.queueCount; ++QueueIndex) {
                    *CurrentPriority++ = 1.0f;
                }
            }

            DeviceInfo.queueCreateInfoCount = QueueFamilyInfos.size();
            DeviceInfo.pQueueCreateInfos = QueueFamilyInfos.data();

            // // 找到适合的队列族，这里假设选择第一个支持图形操作的队列族
            // uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
            // for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            //     if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            //         graphicsQueueFamilyIndex = i;
            //         break;
            //     }
            // }

            // if (graphicsQueueFamilyIndex == UINT32_MAX) {
            //     // 没有找到合适的队列族，处理错误
            //     free(queueFamilies);
            //     return;
            // }

            // // 创建队列创建信息
            // VkDeviceQueueCreateInfo queueCreateInfo = {
            //     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            //     .queueFamilyIndex = graphicsQueueFamilyIndex,
            //     .queueCount = 1,
            //     .pQueuePriorities = (const float[]){1.0f},
            // };

            // DeviceInfo.queueCreateInfoCount = 1;
            // DeviceInfo.pQueueCreateInfos = &queueCreateInfo;

            VkResult Result = vkCreateDevice(physicalDevice, &DeviceInfo, nullptr, &Device);
            if (Result == VK_SUCCESS) {
                ML_LOG(VulkanRHI, Log, "CreateVulkanDevice success");
            }

            // vkGetDeviceQueue(Device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
        }

        // VkResult Result = vkCreateDevice(physicalDevice, &DeviceInfo, nullptr, &Device);
        // if (Result == VK_SUCCESS) {
        //     ML_LOG(VulkanRHI, Log, "CreateVulkanDevice success");
        // }

        // get extensions of vulkan
        // VkPhysicalDeviceMemoryProperties memoryProperties;
        // vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        // ML_LOG(VulkanRHI, Log, "VulkanDevice memory type count=%d",
        //        memoryProperties.memoryTypeCount);
        // for (int i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        //     auto Type = memoryProperties.memoryTypes[i];
        //     ML_LOG(VulkanRHI, Log, "VulkanDevice memory types=%d, heapindex=%d",
        //     Type.propertyFlags,
        //            Type.heapIndex);
        // }
        VkPhysicalDeviceExternalMemoryHostPropertiesEXT externalMemoryHostProperties = {};
        externalMemoryHostProperties.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
        VkPhysicalDeviceProperties2 physicalDeviceProperties = {};
        physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physicalDeviceProperties.pNext = &externalMemoryHostProperties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);

        vkGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(
            vkGetInstanceProcAddr(Instance, "vkGetMemoryFdKHR"));
        if (vkGetMemoryFdKHR) {
            ML_LOG(VulkanRHI, Log, "Get vkGetMemoryFdKHR");
        }
        vkAllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(
            // vkGetInstanceProcAddr(Instance, "vkAllocateMemory")
            vkGetDeviceProcAddr(Device, "vkAllocateMemory"));
        if (vkAllocateMemory) {
            ML_LOG(VulkanRHI, Log, "Get vkAllocateMemory");
        }
    }
    bool FVulkanDynamicRHI::IsPhysicalDeviceMatch(VkPhysicalDevice physicalDevice,
                                                  uint32 targetDeviceId, uint8* targetDeviceUUID) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        if (properties.deviceID == targetDeviceId) {
            // if (memcmp(properties.pipelineCacheUUID, targetDeviceUUID, VK_UUID_SIZE) == 0) {
            return true;
            // }
        }

        return false;
    }

    VkPhysicalDevice FVulkanDynamicRHI::FindPhysicalDevice(uint32 targetDeviceId,
                                                           uint8* targetDeviceUUID) {
        uint32 GpuCount = 0;
        vkEnumeratePhysicalDevices(Instance, &GpuCount, nullptr);
        if (GpuCount == 0) {
            return VK_NULL_HANDLE;
        }
        std::vector<VkPhysicalDevice> physicalDevices(GpuCount);
        vkEnumeratePhysicalDevices(Instance, &GpuCount, physicalDevices.data());
        for (const auto& physicalDevice : physicalDevices) {
            if (IsPhysicalDeviceMatch(physicalDevice, targetDeviceId, targetDeviceUUID)) {
                return physicalDevice;
            }
        }

        return VK_NULL_HANDLE;
    }

    FTexture2DRHIRef FVulkanDynamicRHI::GetTexture(int SharedTextureHandle,
                                                   uint64 TextureMemorySize,
                                                   uint32 TextureMemoryOffset) {
        FTexture2DRHIRef NullRef;
        VkDeviceMemory textureMemory;
        VkResult result = VK_SUCCESS;
        VkImportMemoryFdInfoKHR importMemoryInfo{};
        importMemoryInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
        importMemoryInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
        importMemoryInfo.fd = SharedTextureHandle;
        importMemoryInfo.pNext = nullptr;

        VkMemoryRequirements memoryRequirements;
        VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .extent = {FAppConfig::Get().Width, FAppConfig::Get().Height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
        VkImage TempImage;
        result = vkCreateImage(Device, &imageCreateInfo, nullptr, &TempImage);
        if (result != VK_SUCCESS) {
            ML_LOG(Main, Error, "Failed to create  temp image %d", result);
            return NullRef;
        } else {
            vkGetImageMemoryRequirements(Device, TempImage, &memoryRequirements);
        }

        uint32 memoryTypeIndex = FindMemoryTypeIndex(
            memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = &importMemoryInfo;
        allocateInfo.allocationSize = TextureMemorySize;
        allocateInfo.memoryTypeIndex = memoryTypeIndex;
        result = vkAllocateMemory(Device, &allocateInfo, nullptr, &textureMemory);
        if (result != VK_SUCCESS) {
            ML_LOG(Main, Error, "Failed to allocate memory vulkan texture, %d ", result);
            return NullRef;
        } else {
            result = vkBindImageMemory(Device, TempImage, textureMemory, 0);
            if (result != VK_SUCCESS) {
                vkDestroyImage(Device, TempImage, nullptr);
                vkFreeMemory(Device, textureMemory, nullptr);
                return NullRef;
            } else {

                ML_LOG(Main, Log, "Success create image");
            }
        }
        return nullptr;
    }
} // namespace OpenZI::CloudRender