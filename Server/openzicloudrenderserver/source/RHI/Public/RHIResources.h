// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/20 15:41
// 
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