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

#include "WebRTCIncludes.h"
#include "TextureSource.h"
#include <memory>
#include <vector>

namespace OpenZI::CloudRender
{
    enum FFrameBufferType
    {
        Initialize,
        Simulcast,
        Layer
    };

    /*
     * ----------------- FFrameBuffer -----------------
     * The base framebuffer that extends the WebRTC type.
     */
    class FFrameBuffer : public webrtc::VideoFrameBuffer
    {
    public:
        virtual ~FFrameBuffer() {}

        virtual FFrameBufferType GetFrameBufferType() const = 0;

        // Begin webrtc::VideoFrameBuffer interface
        virtual webrtc::VideoFrameBuffer::Type type() const override
        {
            return webrtc::VideoFrameBuffer::Type::kNative;
        }

        virtual int width() const = 0;
        virtual int height() const = 0;

        virtual rtc::scoped_refptr<webrtc::I420BufferInterface> ToI420() override
        {
            return nullptr;
        }

        virtual const webrtc::I420BufferInterface *GetI420() const override
        {
            return nullptr;
        }
        // End webrtc::VideoFrameBuffer interface
        virtual void AddRef() const {}
        virtual rtc::RefCountReleaseStatus Release() const { return rtc::RefCountReleaseStatus::kOtherRefsRemained;}
    };

    /*
     * ----------------- FInitializeFrameBuffer -----------------
     * We use this frame to force webrtc to create encoders that will siphon off other streams
     * but never get real frames directly pumped to them.
     */
    class FInitializeFrameBuffer : public FFrameBuffer
    {
    public:
        FInitializeFrameBuffer(std::shared_ptr<ITextureSource> InTextureSource);
        virtual ~FInitializeFrameBuffer();

        virtual FFrameBufferType GetFrameBufferType() const { return Initialize; }

        virtual int width() const override;
        virtual int height() const override;

    private:
        std::shared_ptr<ITextureSource> TextureSource;
    };

    /*
     * ----------------- FSimulcastFrameBuffer -----------------
     * Holds a number of textures (called layers) and each is assigned an index.
     */
    class FSimulcastFrameBuffer : public FFrameBuffer
    {
    public:
        FSimulcastFrameBuffer(std::vector<std::shared_ptr<ITextureSource>> &InTextureSources);
        virtual ~FSimulcastFrameBuffer();

        virtual FFrameBufferType GetFrameBufferType() const { return Simulcast; }
        std::shared_ptr<ITextureSource> GetLayerFrameSource(int LayerIndex) const;
        int GetNumLayers() const;

        virtual int width() const override;
        virtual int height() const override;

    private:
        std::vector<std::shared_ptr<ITextureSource>> &TextureSources;
    };

    /*
     * ----------------- FLayerFrameBuffer -----------------
     * Holds a genuine single texture for encoding.
     */
    class FLayerFrameBuffer : public FFrameBuffer
    {
    public:
        FLayerFrameBuffer(std::shared_ptr<ITextureSource> InTextureSource);
        virtual ~FLayerFrameBuffer();

        virtual FFrameBufferType GetFrameBufferType() const { return Layer; }
        FTexture2DRHIRef GetFrame() const;

        virtual int width() const override;
        virtual int height() const override;

    private:
        std::shared_ptr<ITextureSource> TextureSource;
    };

} // namespace OpenZI::CloudRender