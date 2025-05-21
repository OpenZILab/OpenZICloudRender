/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/
#include "TextureSource.h"
#include "Utils.h"

namespace OpenZI::CloudRender {

    /*
     * -------- FTextureSourceBackBuffer ----------------
     */

    FTextureSourceBackBuffer::FTextureSourceBackBuffer()
        : TTextureSourceBackBufferBase(){

          };

    FTextureSourceBackBuffer::FTextureSourceBackBuffer(float InScale, uint32 SizeX, uint32 SizeY, FSendThread* InSender)
        : TTextureSourceBackBufferBase(InScale){
            Width = SizeX;
            Height = SizeY;
            bUseCustomSize = true;
            Sender = InSender;
          };

    void FTextureSourceBackBuffer::CopyTexture(const FTexture2DRHIRef& SourceTexture, TRefCountPtr<FRHITexture2D> DestTexture, FGPUFenceRHIRef& CopyFence) {
        Utils::CopyTexture(SourceTexture, DestTexture, CopyFence);
    }

    TRefCountPtr<FRHITexture2D> FTextureSourceBackBuffer::CreateTexture(int Width, int Height) {
        return Utils::CreateTexture(Width, Height);
    }

    FTexture2DRHIRef FTextureSourceBackBuffer::ToTextureRef(TRefCountPtr<FRHITexture2D> Texture) {
        return Texture;
    }

    /*
     * -------- FTextureSourceBackBufferToCPU ----------------
     */

    FTextureSourceBackBufferToCPU::FTextureSourceBackBufferToCPU()
        : TTextureSourceBackBufferBase(){

          };

    FTextureSourceBackBufferToCPU::FTextureSourceBackBufferToCPU(float InScale)
        : TTextureSourceBackBufferBase(InScale){

          };

    void FTextureSourceBackBufferToCPU::CopyTexture(const FTexture2DRHIRef& SourceTexture, TRefCountPtr<FRawPixelsTexture> DestTexture, FGPUFenceRHIRef& CopyFence) {
        Utils::CopyTexture(SourceTexture, DestTexture->TextureRef, CopyFence);
        Utils::ReadTextureToCPU(FRHICommandListExecutor::GetImmediateCommandList(), DestTexture->TextureRef, DestTexture->RawPixels);
    }

    TRefCountPtr<FRawPixelsTexture> FTextureSourceBackBufferToCPU::CreateTexture(int Width, int Height) {
        FRawPixelsTexture* Tex = new FRawPixelsTexture(Utils::CreateTexture(Width, Height));
        return TRefCountPtr<FRawPixelsTexture>(Tex);
    }

    FTexture2DRHIRef FTextureSourceBackBufferToCPU::ToTextureRef(TRefCountPtr<FRawPixelsTexture> Texture) {
        return Texture->TextureRef;
    }
} // namespace OpenZI::CloudRender