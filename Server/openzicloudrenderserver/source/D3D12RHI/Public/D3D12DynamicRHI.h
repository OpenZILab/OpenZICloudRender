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