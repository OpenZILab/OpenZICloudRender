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
#include "Containers/RefCounting.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#elif PLATFORM_LINUX
using HANDLE = int;
#endif

namespace OpenZI::CloudRender
{
    class FRHITexture2D
    {
    public:
        FRHITexture2D(){}
        FRHITexture2D(uint64 InTextureMemorySize, HANDLE InSharedHandle, uint32 InSharedHandleNameSuffix)
            : TextureMemorySize(InTextureMemorySize), SharedHandle(InSharedHandle), SharedHandleNameSuffix(InSharedHandleNameSuffix)
        {}

        virtual FRHITexture2D* GetTexture2D() { return this; }
        /**
         * Returns access to the platform-specific RHI texture baseclass.  This is designed to provide the RHI with fast access to its base classes in the face of multiple inheritance.
         * @return	The pointer to the platform-specific RHI texture baseclass or NULL if it not initialized or not supported for this RHI
         */
        virtual void *GetTextureBaseRHI()
        {
            // Override this in derived classes to expose access to the native texture resource
            return nullptr;
        }

        virtual uint64 GetTextureMemorySize()
        {
            return TextureMemorySize;
        }

        virtual HANDLE GetSharedHandle()
        {
            return SharedHandle;
        }

        virtual uint32 GetSharedHandleName()
        {
            return SharedHandleNameSuffix;
        }

        virtual uint32 AddRef() const
        {
            return 0;
        }

        virtual uint32 Release() const
        {
            return 0;
        }

        virtual uint32 GetRefCount() const
        {
            return 0;
        }
    protected:
        uint64 TextureMemorySize;
        HANDLE SharedHandle;
        uint32 SharedHandleNameSuffix;
    };

    using FTexture2DRHIRef = TRefCountPtr<FRHITexture2D>;
} // namespace OpenZI::CloudRender