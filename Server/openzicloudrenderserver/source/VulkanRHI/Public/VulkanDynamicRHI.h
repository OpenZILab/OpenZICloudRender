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