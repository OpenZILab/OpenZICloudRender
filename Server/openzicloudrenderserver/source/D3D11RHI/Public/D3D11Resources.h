// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/20 15:55
// 
#pragma once

#if PLATFORM_WINDOWS

#include "RHIResources.h"
#include <d3d11.h>

namespace OpenZI::CloudRender
{
    class FD3D11TextureBase : public IRefCountedObject
    {
    public:
        FD3D11TextureBase(ID3D11Resource* InResource)
            : Resource(InResource)
        {

        }
        ID3D11Resource* GetResource() const { return Resource; }

    protected:
        TRefCountPtr<ID3D11Resource> Resource;
    };

    template<typename BaseResourceType>
    class TD3D11Texture2D : public BaseResourceType, public FD3D11TextureBase
    {
    public:
        TD3D11Texture2D(ID3D11Texture2D* InResource)
            : FD3D11TextureBase(InResource), NumRefs(0)
        {

        }

        virtual ~TD3D11Texture2D() {}

        ID3D11Texture2D* GetResource() const { return (ID3D11Texture2D*)FD3D11TextureBase::GetResource(); }

        virtual void* GetTextureBaseRHI() override final
        {
            return static_cast<FD3D11TextureBase*>(this);
        }

        virtual uint32 AddRef() const
        {
            ++NumRefs;
            if (Resource.IsValid())
            {
                return Resource->AddRef();
            }
            return 0;
        }

        virtual uint32 Release() const
        {
            --NumRefs;
            if (Resource.IsValid())
            {
                Resource->Release();
            }
            if (NumRefs == 0)
            {
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
    class FD3D11BaseTexture2D : public FRHITexture2D
    {
    public:
        FD3D11BaseTexture2D(){}
    };

    using FD3D11Texture2D = TD3D11Texture2D<FD3D11BaseTexture2D>;
} // namespace OpenZI::CloudRender

#endif