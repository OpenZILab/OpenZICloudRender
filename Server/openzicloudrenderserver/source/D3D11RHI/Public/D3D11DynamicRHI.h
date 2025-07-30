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