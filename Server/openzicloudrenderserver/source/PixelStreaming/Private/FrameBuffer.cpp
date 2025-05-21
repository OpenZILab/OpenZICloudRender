// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/31 13:51
// 

#include "FrameBuffer.h"

namespace OpenZI::CloudRender
{
    FInitializeFrameBuffer::FInitializeFrameBuffer(std::shared_ptr<ITextureSource> InTextureSource)
        : TextureSource(InTextureSource)
    {
    }

    FInitializeFrameBuffer::~FInitializeFrameBuffer()
    {
    }

    int FInitializeFrameBuffer::width() const
    {
        return TextureSource->GetSourceWidth();
    }

    int FInitializeFrameBuffer::height() const
    {
        return TextureSource->GetSourceHeight();
    }

    /*
     * ----------------- FSimulcastFrameBuffer -----------------
     */

    FSimulcastFrameBuffer::FSimulcastFrameBuffer(std::vector<std::shared_ptr<ITextureSource>> &InTextureSources)
        : TextureSources(InTextureSources)
    {
    }

    FSimulcastFrameBuffer::~FSimulcastFrameBuffer()
    {
    }

    int FSimulcastFrameBuffer::GetNumLayers() const
    {
        return static_cast<int>(TextureSources.size());
    }

    std::shared_ptr<ITextureSource> FSimulcastFrameBuffer::GetLayerFrameSource(int LayerIndex) const
    {
        //checkf(LayerIndex >= 0 && LayerIndex < TextureSources.size(), TEXT("Requested layer index was out of bounds."));
        return TextureSources[LayerIndex];
    }

    int FSimulcastFrameBuffer::width() const
    {
        //checkf(TextureSources.size() > 0, TEXT("Must be at least one texture source to get the width from."));
        return std::max(TextureSources[0]->GetSourceWidth(), TextureSources[TextureSources.size() - 1]->GetSourceWidth());
    }

    int FSimulcastFrameBuffer::height() const
    {
        //checkf(TextureSources.size() > 0, TEXT("Must be at least one texture source to get the height from."));
        return std::max(TextureSources[0]->GetSourceHeight(), TextureSources[TextureSources.size() - 1]->GetSourceHeight());
    }

    /*
     * ----------------- FLayerFrameBuffer -----------------
     */

    FLayerFrameBuffer::FLayerFrameBuffer(std::shared_ptr<ITextureSource> InTextureSource)
        : TextureSource(InTextureSource)
    {
    }

    FLayerFrameBuffer::~FLayerFrameBuffer()
    {
    }

    FTexture2DRHIRef FLayerFrameBuffer::GetFrame() const
    {
        return TextureSource->GetTexture();
    }

    int FLayerFrameBuffer::width() const
    {
        return TextureSource->GetSourceWidth();
    }

    int FLayerFrameBuffer::height() const
    {
        return TextureSource->GetSourceHeight();
    }
} // namespace OpenZI::CloudRender