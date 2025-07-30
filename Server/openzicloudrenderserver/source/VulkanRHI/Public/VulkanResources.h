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

#include "Logger/CommandLogger.h"
#include "RHIResources.h"
#include <vulkan/vulkan.h>
#include <unistd.h>

namespace OpenZI::CloudRender {
    //    class FVulkanTextureBase : public IRefCountedObject {
    //   public:
    //       FVulkanTextureBase(int InResourceFd) : ResourceFd(InResourceFd) {}

    //       int GetResource() const { return ResourceFd; }
    //       int ResourceFd;
    //    };

    class FVulkanTexture2D : public FRHITexture2D {
    public:
        FVulkanTexture2D(uint64 InTextureMemorySize, int InResourceFd, uint32 InTextureMemoryOffset)
            : FRHITexture2D(InTextureMemorySize, InResourceFd, 0),
              TextureMemoryOffset(InTextureMemoryOffset) {}

        uint32 TextureMemoryOffset;
        uint32 GetTextureMemoryOffset() { return TextureMemoryOffset; }

        virtual uint32 AddRef() const {
            ++NumRefs;
            // ML_LOG(VulkanResource, Verbose, "AddRef,%d", NumRefs);
            return 0;
        }

        virtual uint32 Release() const {
            // ML_LOG(VulkanResource, Verbose, "SubRef,%d", NumRefs);
            --NumRefs;
            if (NumRefs == 0) {
                // ML_LOG(VulkanResource, Verbose, "close fd=%d", SharedHandle);
                // close(SharedHandle);
                delete this;
            }
            return 0;
        }

        virtual uint32 GetRefCount() const { return NumRefs; }

    private:
        mutable uint32 NumRefs = 0;
    };

    class FVulkanImage : public FRHITexture2D {
    public:
        FVulkanImage(uint64 InTextureMemorySize, int InResourceFd, uint32 InTextureMemoryOffset,
                     VkDevice InDevice, VkImage InImage, VkDeviceMemory InMemory)
            : FRHITexture2D(InTextureMemorySize, InResourceFd, 0),
              TextureMemoryOffset(InTextureMemoryOffset), Device(InDevice), Image(InImage),
              Memory(InMemory) {}

        uint32 TextureMemoryOffset;
        uint32 GetTextureMemoryOffset() { return TextureMemoryOffset; }

        virtual uint32 AddRef() const {
            ++NumRefs;
            // ML_LOG(VulkanResource, Verbose, "AddRef,%d", NumRefs);
            return 0;
        }

        virtual uint32 Release() const {
            // ML_LOG(VulkanResource, Verbose, "SubRef,%d", NumRefs);
            --NumRefs;
            if (NumRefs == 0) {
                // ML_LOG(VulkanResource, Verbose, "close fd=%d", SharedHandle);
                vkDestroyImage(Device, Image, nullptr);
                vkFreeMemory(Device, Memory, nullptr);
                // TODO: check enable close
                close(SharedHandle);
                delete this;
            }
            return 0;
        }

        virtual uint32 GetRefCount() const { return NumRefs; }

    private:
        mutable uint32 NumRefs = 0;
        VkImage Image;
        VkDeviceMemory Memory;
        VkDevice Device;
    };
} // namespace OpenZI::CloudRender

#endif