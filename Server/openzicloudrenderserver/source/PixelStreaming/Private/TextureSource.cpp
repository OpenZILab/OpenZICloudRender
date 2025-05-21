//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/30 23:27
//
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