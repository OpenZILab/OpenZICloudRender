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

#pragma once
#include "CommonRenderResources.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "RHIResources.h"
#include "RHIStaticStates.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ScreenRendering.h"

namespace OpenZI::CloudRender {
    namespace Utils {
        inline void CopyTexture(FTexture2DRHIRef SourceTexture, FTexture2DRHIRef DestinationTexture,
                                FGPUFenceRHIRef& CopyFence) {
#if (ENGINE_MAJOR_VERSION <= 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
            FRHICommandListImmediate& RHICmdList =
                FRHICommandListExecutor::GetImmediateCommandList();

            IRendererModule* RendererModule =
                &FModuleManager::GetModuleChecked<IRendererModule>("Renderer");

            // #todo-renderpasses there's no explicit resolve here? Do we need one?
            FRHIRenderPassInfo RPInfo(DestinationTexture, ERenderTargetActions::Load_Store);

            RHICmdList.BeginRenderPass(RPInfo, TEXT("CopyBackbuffer"));

            {
                RHICmdList.SetViewport(0, 0, 0.0f, DestinationTexture->GetSizeX(),
                                       DestinationTexture->GetSizeY(), 1.0f);

                FGraphicsPipelineStateInitializer GraphicsPSOInit;
                RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
                GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
                GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                GraphicsPSOInit.DepthStencilState =
                    TStaticDepthStencilState<false, CF_Always>::GetRHI();

                // New engine version...
                FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
                TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
                TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

                GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                    GFilterVertexDeclaration.VertexDeclarationRHI;
                GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

                GraphicsPSOInit.PrimitiveType = PT_TriangleList;
#if ENGINE_MAJOR_VERSION >= 5
                SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
#else
                SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit,
                                         EApplyRendertargetOption::DoNothing);
#endif
                if (DestinationTexture->GetSizeX() != SourceTexture->GetSizeX() ||
                    DestinationTexture->GetSizeY() != SourceTexture->GetSizeY()) {
                    PixelShader->SetParameters(
                        RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), SourceTexture);
                } else {
                    PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(),
                                               SourceTexture);
                }

                RendererModule->DrawRectangle(RHICmdList, 0, 0,               // Dest X, Y
                                              DestinationTexture->GetSizeX(), // Dest Width
                                              DestinationTexture->GetSizeY(), // Dest Height
                                              0, 0,                           // Source U, V
                                              1, 1,                           // Source USize, VSize
                                              DestinationTexture->GetSizeXY(), // Target buffer size
                                              FIntPoint(1, 1), // Source texture size
                                              VertexShader, EDRF_Default);
            }

            RHICmdList.EndRenderPass();

            RHICmdList.WriteGPUFence(CopyFence);

            RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForOutstandingTasksOnly);
#else
            FRHICommandListImmediate& RHICmdList =
                FRHICommandListExecutor::GetImmediateCommandList();
            auto& DestTexture = DestinationTexture;
            if (SourceTexture->GetDesc().Format == DestTexture->GetDesc().Format &&
                SourceTexture->GetDesc().Extent.X == DestTexture->GetDesc().Extent.X &&
                SourceTexture->GetDesc().Extent.Y == DestTexture->GetDesc().Extent.Y) {

                RHICmdList.Transition(
                    FRHITransitionInfo(SourceTexture, ERHIAccess::Unknown, ERHIAccess::CopySrc));
                RHICmdList.Transition(
                    FRHITransitionInfo(DestTexture, ERHIAccess::Unknown, ERHIAccess::CopyDest));

                // source and dest are the same. simple copy
                RHICmdList.CopyTexture(SourceTexture, DestTexture, {});
            } else {
                IRendererModule* RendererModule =
                    &FModuleManager::GetModuleChecked<IRendererModule>("Renderer");

                RHICmdList.Transition(
                    FRHITransitionInfo(SourceTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask));
                RHICmdList.Transition(
                    FRHITransitionInfo(DestTexture, ERHIAccess::Unknown, ERHIAccess::RTV));

                // source and destination are different. rendered copy
                FRHIRenderPassInfo RPInfo(DestTexture, ERenderTargetActions::Load_Store);
                RHICmdList.BeginRenderPass(RPInfo, TEXT("PixelStreaming::CopyTexture"));
                {
                    FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
                    TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
                    TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

                    RHICmdList.SetViewport(0, 0, 0.0f, DestTexture->GetDesc().Extent.X,
                                           DestTexture->GetDesc().Extent.Y, 1.0f);

                    FGraphicsPipelineStateInitializer GraphicsPSOInit;
                    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
                    GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
                    GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
                    GraphicsPSOInit.DepthStencilState =
                        TStaticDepthStencilState<false, CF_Always>::GetRHI();
                    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                        GFilterVertexDeclaration.VertexDeclarationRHI;
                    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                        VertexShader.GetVertexShader();
                    GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                    GraphicsPSOInit.PrimitiveType = PT_TriangleList;
                    SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

                    PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(),
                                               SourceTexture);

                    FIntPoint TargetBufferSize(DestTexture->GetDesc().Extent.X,
                                               DestTexture->GetDesc().Extent.Y);
                    RendererModule->DrawRectangle(RHICmdList, 0, 0,                // Dest X, Y
                                                  DestTexture->GetDesc().Extent.X, // Dest Width
                                                  DestTexture->GetDesc().Extent.Y, // Dest Height
                                                  0, 0,                            // Source U, V
                                                  1, 1,             // Source USize, VSize
                                                  TargetBufferSize, // Target buffer size
                                                  FIntPoint(1, 1),  // Source texture size
                                                  VertexShader, EDRF_Default);
                }

                RHICmdList.EndRenderPass();

                RHICmdList.Transition(
                    FRHITransitionInfo(SourceTexture, ERHIAccess::SRVMask, ERHIAccess::CopySrc));
                RHICmdList.Transition(
                    FRHITransitionInfo(DestTexture, ERHIAccess::RTV, ERHIAccess::CopyDest));

                RHICmdList.WriteGPUFence(CopyFence);
            }
#endif
        }

        inline FTexture2DRHIRef CreateTexture(uint32 Width, uint32 Height) {
#if (ENGINE_MAJOR_VERSION <= 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
            // Create empty texture
            FRHIResourceCreateInfo CreateInfo(TEXT("BlankTexture"));

            FTexture2DRHIRef Texture;
            FString RHIName = GDynamicRHI->GetName();

            if (RHIName == TEXT("Vulkan")) {
                Texture =
                    GDynamicRHI->RHICreateTexture2D(Width, Height, EPixelFormat::PF_B8G8R8A8, 1, 1,
                                                    TexCreate_External | TexCreate_RenderTargetable,
                                                    ERHIAccess::Present, CreateInfo);
            } else {
                Texture =
                    GDynamicRHI->RHICreateTexture2D(Width, Height, EPixelFormat::PF_B8G8R8A8, 1, 1,
                                                    TexCreate_Shared | TexCreate_RenderTargetable,
                                                    ERHIAccess::CopyDest, CreateInfo);
            }
            return Texture;
#else
            // Create empty texture
            FRHITextureCreateDesc TextureDesc =
                FRHITextureCreateDesc::Create2D(TEXT("PixelStreamingBlankTexture"), Width, Height,
                                                EPixelFormat::PF_B8G8R8A8)
                    .SetClearValue(FClearValueBinding::None)
                    .SetFlags(ETextureCreateFlags::RenderTargetable)
                    .SetInitialState(ERHIAccess::Present)
                    .DetermineInititialState();

            if (RHIGetInterfaceType() == ERHIInterfaceType::Vulkan) {
                TextureDesc.AddFlags(ETextureCreateFlags::External);
            } else {
                TextureDesc.AddFlags(ETextureCreateFlags::Shared);
                TextureDesc.SetInitialState(ERHIAccess::CopyDest);
            }

            return GDynamicRHI->RHICreateTexture(TextureDesc);
#endif
        }

        inline void ReadTextureToCPU(FRHICommandListImmediate& RHICmdList,
                                     FTexture2DRHIRef& TextureRef, TArray<FColor>& OutPixels) {
            FIntRect Rect(0, 0, TextureRef->GetSizeX(), TextureRef->GetSizeY());
            RHICmdList.ReadSurfaceData(TextureRef, Rect, OutPixels, FReadSurfaceDataFlags());
        }
    } // namespace Utils
} // namespace OpenZI::CloudRender