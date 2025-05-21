//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/21 11:37
//
#pragma once

#if PLATFORM_LINUX
#endif

#include "DynamicRHI.h"
#include "VulkanResources.h"
#include <algorithm>
#include <vulkan/vulkan.h>

namespace OpenZI::CloudRender {

    template <typename T> static void ZeroVulkanStruct(T& Struct, int32 InVkStructureType) {
        (int32&)Struct.sType = InVkStructureType;
        std::fill_n(((uint8*)&Struct) + sizeof(InVkStructureType),
                    sizeof(T) - sizeof(InVkStructureType), 0);
    }

    class FVulkanDynamicRHI : public FDynamicRHI {
    public:
        FVulkanDynamicRHI();
        virtual ~FVulkanDynamicRHI();

        FTexture2DRHIRef GetTextureRef(int SharedTextureHandle, uint64 TextureMemorySize,
                                       uint32 TextureMemoryOffset) {
            FRHITexture2D* Texture2D =
                new FVulkanTexture2D(TextureMemorySize, SharedTextureHandle, TextureMemoryOffset);
            return Texture2D;
        }

        FTexture2DRHIRef GetTexture(int SharedTextureHandle, uint64 TextureMemorySize,
                                       uint32 TextureMemoryOffset);

        uint32 FindMemoryTypeIndex(uint32 memoryTypeBits, VkMemoryPropertyFlags requiredProperties);

        VkDevice GetDevice() { return Device; }
        VkPhysicalDevice GetPhysicalDevice() { return PhysicalDevice; }
        VkPhysicalDeviceProperties2KHR& GetProps2() { return PhysicalDeviceProperties2; }
        VkPhysicalDeviceIDPropertiesKHR& GetIDProps() { return PhysicalDeviceIDProperties; }

        void CreateInstance();
        void CreateVulkanDevice(uint32 targetDeviceId, uint8* targetDeviceUUID);
        bool IsPhysicalDeviceMatch(VkPhysicalDevice physicalDevice, uint32 targetDeviceId,
                                   uint8* targetDeviceUUID);
        VkPhysicalDevice FindPhysicalDevice(uint32 targetDeviceId, uint8* targetDeviceUUID);
        PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
        PFN_vkAllocateMemory vkAllocateMemory;

    protected:
        VkInstance Instance;
        VkPhysicalDevice PhysicalDevice;
        std::string Name;
        std::string HandlePrefix;

        VkPhysicalDeviceProperties2KHR PhysicalDeviceProperties2;
        VkPhysicalDeviceIDPropertiesKHR PhysicalDeviceIDProperties;
        VkDevice Device;
        VkQueue graphicsQueue;
    };
    extern VkDevice GVulkanDevice;
} // namespace OpenZI::CloudRender