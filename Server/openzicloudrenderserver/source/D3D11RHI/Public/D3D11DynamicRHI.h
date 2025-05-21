// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/20 16:47
// 
#pragma once

#if PLATFORM_WINDOWS

#include "DynamicRHI.h"
#include "D3D11Resources.h"
#include "Containers/RefCounting.h"
#include <d3d11.h>

namespace OpenZI::CloudRender
{
    class FD3D11DynamicRHI : public FDynamicRHI
    {
    public:
        FD3D11DynamicRHI();
        virtual ~FD3D11DynamicRHI();

        FTexture2DRHIRef GetSharedTexture(HANDLE SharedTextureHandle);
        TRefCountPtr<ID3D11Device> GetDevice() { return Device; }
        TRefCountPtr<ID3D11DeviceContext> GetContext() { return DeviceContext; }

    protected:
        std::string Name;

    private:
        bool InitD3DDevice();
        bool RegenerateD3DDevice();
        bool CreateDevice(IDXGIAdapter *DXGIAdapter, ID3D11Device **pD3D11Device, ID3D11DeviceContext **pD3D11Context);

        TRefCountPtr<IDXGIFactory> DXGIFactory;
        TRefCountPtr<ID3D11Device> Device;
        TRefCountPtr<ID3D11DeviceContext> DeviceContext;
    };
} // namespace OpenZI::CloudRender

#endif