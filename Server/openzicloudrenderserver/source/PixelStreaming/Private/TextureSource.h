// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/30 23:26
// 
#pragma once
#include "RHIResources.h"
#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "MessageCenter/MessageCenter.h"
#include <string>

namespace OpenZI::CloudRender
{
    /*
     * Interface for all texture sources in Pixel Streaming.
     * These texture sources are used to populate video sources for video tracks.
     */
    class ITextureSource
    {
    public:
        ITextureSource() = default;
        virtual ~ITextureSource() = default;
        virtual bool IsAvailable() const = 0;
        virtual bool IsEnabled() const = 0;
        virtual void SetEnabled(bool bInEnabled) = 0;
        virtual int GetSourceHeight() const = 0;
        virtual int GetSourceWidth() const = 0;
        virtual FTexture2DRHIRef GetTexture() = 0;
        virtual std::string &GetName() const = 0;
    };

    /*
     * Base class for TextureSources that get their textures from the UE backbuffer.
     * Textures are copied from the backbuffer using a triplebuffering mechanism so that texture read access is always thread safe while writes are occurring.
     * If no texture has been written since the last read then the same texture will be read again.
     * This class also has the additional functionality of scaling textures from the backbuffer.
     * Note: This is a template class that uses CRTP - see: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
     * For derived types of this class they must contain the following static methods:
     *
     * static void CopyTexture(const FTexture2DRHIRef& SourceTexture, TRefCountPtr<TextureType> DestTexture, FGPUFenceRHIRef& CopyFence)
     * static TRefCountPtr<TextureType> CreateTexture(int Width, int Height)
     * static FTexture2DRHIRef ToTextureRef(TRefCountPtr<TextureType> Texture)
     * static FString& GetName()
     */
    template <class DerivedType>
    class TTextureSourceBackBufferBase : public ITextureSource
    {
    public:
        TTextureSourceBackBufferBase(float InFrameScale)
            : FrameScale(InFrameScale)
        {
        }

        TTextureSourceBackBufferBase()
            : TTextureSourceBackBufferBase(1.0)
        {
        }

        virtual ~TTextureSourceBackBufferBase()
        {
        }


        /* Begin ITextureSource interface */
        virtual void SetEnabled(bool bInEnabled) override
        {
            bEnabled = bInEnabled;
            // This source has been disabled, so set `bInitialized` to false so `OnBackBufferReady_RenderThread`
            // will make new textures next time it is called.
            if (bInitialized && bInEnabled == false)
            {
                bInitialized = false;
            }
        }

        bool IsEnabled() const override { return bEnabled; }
        bool IsAvailable() const override { return bInitialized; }
        int GetSourceWidth() const override { return SourceWidth; }
        int GetSourceHeight() const override { return SourceHeight; }
        std::string &GetName() const override { return DerivedType::GetNameImpl(); };
        /* End ITextureSource interface */

        float GetFrameScaling() const { return FrameScale; }

    protected:
        void Initialize(int Width, int Height)
        {
            SourceWidth = Width;
            SourceHeight = Height;
            bInitialized = true;
        }

        bool bInitialized = false;
        const float FrameScale;
    private:
        int SourceWidth = 0;
        int SourceHeight = 0;
        bool bEnabled = true;
    };

    /*
     * Captures the backbuffer into a `FTexture2DRHIRef` in whatever pixel format the backbuffer is already using.
     */
    class FTextureSourceBackBuffer : public TTextureSourceBackBufferBase<FTextureSourceBackBuffer>
    {
    public:
        FTextureSourceBackBuffer();
        FTextureSourceBackBuffer(float InScale);
        virtual ~FTextureSourceBackBuffer();
        
        /* Begin TTextureSourceBackBufferBase "template interface" */
        static std::string &GetNameImpl() { return Name; }
        FTexture2DRHIRef GetTexture() override;
        /* End TTextureSourceBackBufferBase "template interface" */

    private:
        void OnTextureReceived(FTexture2DRHIRef Texture);
        inline static std::string Name = "FTextureSourceBackBuffer";
        TQueue<FTexture2DRHIRef> TextureCaches;
        FTexture2DRHIRef TextureCache;
        FTexture2DRHIRef LastTexture;
        DECLARE_MESSAGE(OnTextureReceived);
    };

} // namespace OpenZI::CloudRender