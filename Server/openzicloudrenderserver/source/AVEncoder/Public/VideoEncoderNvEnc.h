// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/16 10:44
// 

#pragma once
#include "VideoEncoder.h"
#if PLATFORM_WINDOWS
#include <d3d11.h>
#include <d3d12.h>
#elif PLATFORM_LINUX
#endif
#include <memory>

class NvEncoder;
class NvEncoderD3D12;
namespace OpenZI::CloudRender
{
    class FD3D11DynamicRHI;
    class CudaConverter;
    class FD3D12CudaConverter;
    class FVulkanDynamicRHI;
    class FVulkanCudaConverter;

    class FVideoEncoderNvEnc : public FVideoEncoder
    {
    public:
        FVideoEncoderNvEnc(bool useNV12 = true);
        ~FVideoEncoderNvEnc();

        void Initialize() override;
        void Shutdown() override;
        void Transmit(FTexture2DRHIRef Texture2DRHIRef, FBufferInfo BufferInfo);
        // Delay loading for Cuda driver API to correctly work on non-NVIDIA GPU.
        void SetOnEncodedPacket(std::function<void(uint32, std::vector<uint8> &, const FBufferInfo)> InOnEncodedPacket) override
        {
            OnEncodedPacket = std::move(InOnEncodedPacket);
        }
    private:
#if PLATFORM_WINDOWS
        void TransmitD3D11(TRefCountPtr<ID3D11Texture2D> pTexture, FBufferInfo BufferInfo);
        void TransmitD3D12(TRefCountPtr<ID3D12Resource> pTexture, FBufferInfo BufferInfo, uint64 TextureMemorySize, HANDLE SharedHandle = nullptr, uint32 SharedHandleNameSuffix = 0);
        std::shared_ptr<NvEncoderD3D12> m_NvEncoderD3D12;
        std::shared_ptr<FD3D11DynamicRHI> D3D11DynamicRHI;
        std::shared_ptr<CudaConverter> m_Converter;
        std::unique_ptr<FD3D12CudaConverter> D3D12CudaConverter;
#elif PLATFORM_LINUX
        void TransmitVulkan(FTexture2DRHIRef Texture2DRHIRef, FBufferInfo BufferInfo);
        std::shared_ptr<FVulkanDynamicRHI> VulkanDynamicRHI;
        std::unique_ptr<FVulkanCudaConverter> VulkanCudaConverter;
#endif
        FTexture2DRHIRef LastTexture;
        std::shared_ptr<NvEncoder> m_NvNecoder;
        int m_nFrame;
        const bool m_useNV12;
        std::vector<uint8_t> EncodedBuffer;
        std::function<void(uint32, std::vector<uint8>&, const FBufferInfo)> OnEncodedPacket;
        bool bH264;
    };
} // namespace OpenZI::CloudRender