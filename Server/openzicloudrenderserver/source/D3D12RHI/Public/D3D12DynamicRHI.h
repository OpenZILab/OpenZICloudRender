// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/21 11:37
// 
#pragma once
#if PLATFORM_WINDOWS

#include "DynamicRHI.h"
#include "D3D12Resources.h"
#include "Containers/RefCounting.h"
#include <d3d12.h>

namespace OpenZI::CloudRender
{
    class FD3D12DynamicRHI : public FDynamicRHI
    {
    public:
        FD3D12DynamicRHI();
        virtual ~FD3D12DynamicRHI();

        FTexture2DRHIRef GetSharedTexture(HANDLE SharedTextureHandle);
        FTexture2DRHIRef GetSharedTextureBySuffix(uint32 Suffix, HANDLE SharedTextureHandle, uint64 TextureMemorySize);
        TRefCountPtr<ID3D12Device> GetDevice() { return Device; }

    protected:
        std::string Name;
        std::string HandlePrefix;
    private:
        TRefCountPtr<ID3D12Device> Device;
        TRefCountPtr<ID3D12Debug> DebugController;
    };
} // namespace OpenZI::CloudRender

#endif