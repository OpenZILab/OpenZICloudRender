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