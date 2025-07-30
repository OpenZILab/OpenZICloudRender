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

#include "TextureSource.h"
#include "Config.h"

namespace OpenZI::CloudRender
{
    FTextureSourceBackBuffer::FTextureSourceBackBuffer()
        : TTextureSourceBackBufferBase()
    {
        SUBSCRIBE_MESSAGE_OneParam(OnTextureReceived, this, &FTextureSourceBackBuffer::OnTextureReceived, FTexture2DRHIRef, Texture);
    }

    FTextureSourceBackBuffer::FTextureSourceBackBuffer(float InScale)
        : TTextureSourceBackBufferBase(InScale)
    {
        SUBSCRIBE_MESSAGE_OneParam(OnTextureReceived, this, &FTextureSourceBackBuffer::OnTextureReceived, FTexture2DRHIRef, Texture);
    }

    FTextureSourceBackBuffer::~FTextureSourceBackBuffer()
    {
        UNSUBSCRIBE_MESSAGE(OnTextureReceived);
    }

    FTexture2DRHIRef FTextureSourceBackBuffer::GetTexture()
    {
        FTexture2DRHIRef Texture;
        if (TextureCaches.Dequeue(Texture))
        {
            LastTexture = Texture;
        }
        return LastTexture;
    }

    void FTextureSourceBackBuffer::OnTextureReceived(FTexture2DRHIRef Texture)
    {
        if (!bInitialized)
        {
            Initialize(FAppConfig::Get().Width * (int)FrameScale, FAppConfig::Get().Height * (int)FrameScale);
        }
        if (!IsEnabled())
        {
            return;
        }
        if (TextureCaches.Num() >= 2)
        {
            FTexture2DRHIRef TempTexture;
            TextureCaches.Dequeue(TempTexture);
        }
        TextureCaches.Enqueue(Texture);
    }
} // namespace OpenZI::CloudRender