// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/21 11:36
// 
#pragma once

#if PLATFORM_WINDOWS

#include "RHIResources.h"
#include <d3d12.h>
#include "Logger/CommandLogger.h"

namespace OpenZI::CloudRender
{
    class FD3D12BaseTexture2D;
    class FD3D12TextureBase : public IRefCountedObject
    {
    public:
        FD3D12TextureBase(ID3D12Resource* InResource)
            : Resource(InResource)
        {

        }
        ID3D12Resource* GetResource() const { return Resource; }
    protected:
        TRefCountPtr<ID3D12Resource> Resource;
    };

    template<typename BaseResourceType = FRHITexture2D>
    class TD3D12Texture2D : public BaseResourceType, public FD3D12TextureBase
    {
    public:
        TD3D12Texture2D(ID3D12Resource *InResource, uint64 InTextureMemorySize, HANDLE InSharedHandle, uint32 InSharedHandleNameSuffix)
            : FD3D12TextureBase(InResource), FD3D12BaseTexture2D(InTextureMemorySize, InSharedHandle, InSharedHandleNameSuffix)
        {

        }

        virtual ~TD3D12Texture2D() {}

        ID3D12Resource *GetResource() const { return FD3D12TextureBase::GetResource(); }

        virtual void* GetTextureBaseRHI() override final
        {
            return static_cast<FD3D12TextureBase*>(this);
        }

        virtual uint32 AddRef() const
        {
            ++NumRefs;
            // ML_LOG(LogD3D12Resource, Log, "Add,R=%d,Suffix=%d", NumRefs, SharedHandleNameSuffix);
            if (Resource.IsValid())
            {
                return Resource->AddRef();
            }
            return 0;
        }

        virtual uint32 Release() const
        {
            // ML_LOG(LogD3D12Resource, Log, "Rel,R=%d,Suffix=%d", NumRefs, SharedHandleNameSuffix);
            --NumRefs;
            if (Resource.IsValid())
            {
                Resource->Release();
            }
            if (NumRefs == 0)
            {
                if (BaseResourceType::SharedHandle)
                {
                    CloseHandle(BaseResourceType::SharedHandle);
                }
                delete this;
            }
            return 0;
        }

        // 对于D3D资源来说，无需实现GetRefCount
        virtual uint32 GetRefCount() const
        {
            return 0;
        }

    private:
        mutable uint32 NumRefs = 0;
    };

    // TODO: 后续可能会新增纹理属性相关代码
    class FD3D12BaseTexture2D : public FRHITexture2D
    {
    public:
        FD3D12BaseTexture2D(){}
        FD3D12BaseTexture2D(uint64 InTextureMemorySize, HANDLE InSharedHandle, uint32 InSharedHandleNameSuffix)
            : FRHITexture2D(InTextureMemorySize, InSharedHandle, InSharedHandleNameSuffix) {}
    };

    using FD3D12Texture2D = TD3D12Texture2D<FD3D12BaseTexture2D>;
} // namespace OpenZI::CloudRender

#endif