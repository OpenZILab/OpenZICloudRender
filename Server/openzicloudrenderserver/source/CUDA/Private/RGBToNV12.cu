//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/27 14:18
//

// TODO: 1.texRef->texObj 2.cudaBindTextureToArray

#include <stdint.h>

#include "RGBToNV12.cuh"

using uint8 = uint8_t;
using uint32 = uint32_t;
struct FPixel_RGB10A2
{
    uint32 R : 10;
    uint32 G : 10;
    uint32 B : 10;
    uint32 A : 2;
};

__device__ inline float rgb2y(uchar4 c)
{
    return 0.257f * c.x + 0.504f * c.y + 0.098f * c.z + 16.0f;
}

__device__ inline float rgb2u(uchar4 c)
{
    return -0.148f * c.x - 0.291f * c.y + 0.439f * c.z + 128.0f;
}

__device__ inline float rgb2v(uchar4 c)
{
    return 0.439f * c.x - 0.368f * c.y - 0.071f * c.z + 128.0f;
}
__device__ inline float bgr2y(uchar4 c)
{
    // return 0.257f * c.z + 0.504f * c.y + 0.098f * c.x + 16.0f;
    return ((66 * c.z + 129 * c.y + 25 * c.x) >> 8) + 16;
    // return 0.299 * c.z + 0.587 * c.y + 0.114f * c.x ;
}

__device__ inline float bgr2u(uchar4 c)
{
    return -0.148f * c.z - 0.291f * c.y + 0.439f * c.x + 128.0f;
    // return -0.1687 * c.z - 0.3313f * c.y + 0.5f * c.x + 128.0f;
}

__device__ inline float bgr2v(uchar4 c)
{
    return 0.439f * c.z - 0.368f * c.y - 0.071f * c.x + 128.0f;
    // return 0.5 * c.z - 0.4187f * c.y - 0.0813f * c.x + 128.0f;
}
__device__ inline uchar4 rgba10torgba8(uchar4 &c)
{
    FPixel_RGB10A2 *SrcPixelPtr = (FPixel_RGB10A2 *)(&c);
    uchar4 dst;
    dst.x = (uint8)((float)SrcPixelPtr->R * 0.249266f);
    dst.y = (uint8)((float)SrcPixelPtr->G * 0.249266f);
    dst.z = (uint8)((float)SrcPixelPtr->B * 0.249266f);
    dst.w = 255;
    return dst;
}

texture<uchar4, cudaTextureType2D, cudaReadModeElementType> texRef;
// cudaTextureObject_t texObj;

__global__ void RGBA2NV12_kernel(uint8_t *dstImage, size_t destPitch,
                                 uint32_t width, uint32_t height)
{
    // Pad borders with duplicate pixels, and we multiply by 2 because we process 2 pixels per thread
    int32_t x = blockIdx.x * (blockDim.x << 1) + (threadIdx.x << 1);
    int32_t y = blockIdx.y * (blockDim.y << 1) + (threadIdx.y << 1);

    int x1 = x + 1;
    int y1 = y + 1;

    if (x1 >= width)
        return; // x = width - 1;

    if (y1 >= height)
        return; // y = height - 1;

    uchar4 c00 = tex2D(texRef, x, y);
    uchar4 c01 = tex2D(texRef, x1, y);
    uchar4 c10 = tex2D(texRef, x, y1);
    uchar4 c11 = tex2D(texRef, x1, y1);
    c00 = rgba10torgba8(c00);
    c01 = rgba10torgba8(c01);
    c10 = rgba10torgba8(c10);
    c11 = rgba10torgba8(c11);

    uint8_t y00 = (uint8_t)(rgb2y(c00) + 0.5f);
    uint8_t y01 = (uint8_t)(rgb2y(c01) + 0.5f);
    uint8_t y10 = (uint8_t)(rgb2y(c10) + 0.5f);
    uint8_t y11 = (uint8_t)(rgb2y(c11) + 0.5f);

    uint8_t u = (uint8_t)((rgb2u(c00) + rgb2u(c01) + rgb2u(c10) + rgb2u(c11)) * 0.25f + 0.5f);
    uint8_t v = (uint8_t)((rgb2v(c00) + rgb2v(c01) + rgb2v(c10) + rgb2v(c11)) * 0.25f + 0.5f);

    dstImage[destPitch * y + x] = y00;
    dstImage[destPitch * y + x1] = y01;
    dstImage[destPitch * y1 + x] = y10;
    dstImage[destPitch * y1 + x1] = y11;

    uint32_t chromaOffset = destPitch * height;
    int32_t x_chroma = x;
    int32_t y_chroma = y >> 1;

    dstImage[chromaOffset + destPitch * y_chroma + x_chroma] = u;
    dstImage[chromaOffset + destPitch * y_chroma + x_chroma + 1] = v;
}

__global__ void BGRA2NV12_kernel(uint8_t *dstImage, size_t destPitch,
                                 uint32_t width, uint32_t height)
{
    // Pad borders with duplicate pixels, and we multiply by 2 because we process 2 pixels per thread
    int32_t x = blockIdx.x * (blockDim.x << 1) + (threadIdx.x << 1);
    int32_t y = blockIdx.y * (blockDim.y << 1) + (threadIdx.y << 1);

    int x1 = x + 1;
    int y1 = y + 1;

    if (x1 >= width)
        return; // x = width - 1;

    if (y1 >= height)
        return; // y = height - 1;

    uchar4 c00 = tex2D(texRef, x, y);
    uchar4 c01 = tex2D(texRef, x1, y);
    uchar4 c10 = tex2D(texRef, x, y1);
    uchar4 c11 = tex2D(texRef, x1, y1);

    uint8_t y00 = (uint8_t)(bgr2y(c00) + 0.5f);
    uint8_t y01 = (uint8_t)(bgr2y(c01) + 0.5f);
    uint8_t y10 = (uint8_t)(bgr2y(c10) + 0.5f);
    uint8_t y11 = (uint8_t)(bgr2y(c11) + 0.5f);

    uint8_t u = (uint8_t)((bgr2u(c00) + bgr2u(c01) + bgr2u(c10) + bgr2u(c11)) * 0.25f + 0.5f);
    uint8_t v = (uint8_t)((bgr2v(c00) + bgr2v(c01) + bgr2v(c10) + bgr2v(c11)) * 0.25f + 0.5f);

    dstImage[destPitch * y + x] = y00;
    dstImage[destPitch * y + x1] = y01;
    dstImage[destPitch * y1 + x] = y10;
    dstImage[destPitch * y1 + x1] = y11;

    uint32_t chromaOffset = destPitch * height;
    int32_t x_chroma = x;
    int32_t y_chroma = y >> 1;

    dstImage[chromaOffset + destPitch * y_chroma + x_chroma] = u;
    dstImage[chromaOffset + destPitch * y_chroma + x_chroma + 1] = v;
}

__device__ uchar4 GetPixelWithWaterMark(uint32_t x, uint32_t y, uint32_t areaX, uint32_t areaY, uint32_t areaWidth, uint32_t areaHeight, uint8_t *waterMarkImage, size_t waterMarkPitch)
{
    uchar4 Pixel = tex2D(texRef, x, y);

    if (x >= areaX && x <= (areaX + areaWidth) && y >= areaY && y <= (areaY + areaHeight))
    {
        uint32_t locX = x - areaX;
        uint32_t locY = y - areaY;
        uint32_t MarkPixelOffset = (locY * waterMarkPitch * areaWidth) + locX * waterMarkPitch;
        uint8 markR = *(waterMarkImage + MarkPixelOffset);
        uint8 markG = *(waterMarkImage + MarkPixelOffset + 1);
        uint8 markB = *(waterMarkImage + MarkPixelOffset + 2);
        uint8 markA = *(waterMarkImage + MarkPixelOffset + 3);
        Pixel.x = uint8(markR * markA / 255.0 + Pixel.x * (1 - markA / 255.0));
        Pixel.y = uint8(markG * markA / 255.0 + Pixel.y * (1 - markA / 255.0));
        Pixel.z = uint8(markB * markA / 255.0 + Pixel.z * (1 - markA / 255.0));
    }

    return Pixel;
}

__global__ void BGRA2NV12_WithWaterMark_kernel(uint8_t *dstImage, size_t destPitch,
                                               uint32_t width, uint32_t height,
                                               uint8_t *waterMarkImage, size_t waterMarkPitch,
                                               uint32_t markWidth, uint32_t markHeight,
                                               uint32_t markLocationX, uint32_t markLocationY)
{
    // Pad borders with duplicate pixels, and we multiply by 2 because we process 2 pixels per thread
    int32_t x = blockIdx.x * (blockDim.x << 1) + (threadIdx.x << 1);
    int32_t y = blockIdx.y * (blockDim.y << 1) + (threadIdx.y << 1);

    int x1 = x + 1;
    int y1 = y + 1;

    if (x1 >= width)
        return; // x = width - 1;

    if (y1 >= height)
        return; // y = height - 1;

    uchar4 c00 = GetPixelWithWaterMark(x, y, markLocationX, markLocationY, markWidth, markHeight, waterMarkImage, waterMarkPitch);
    uchar4 c01 = GetPixelWithWaterMark(x1, y, markLocationX, markLocationY, markWidth, markHeight, waterMarkImage, waterMarkPitch);
    uchar4 c10 = GetPixelWithWaterMark(x, y1, markLocationX, markLocationY, markWidth, markHeight, waterMarkImage, waterMarkPitch);
    uchar4 c11 = GetPixelWithWaterMark(x1, y1, markLocationX, markLocationY, markWidth, markHeight, waterMarkImage, waterMarkPitch);

    uint8_t y00 = (uint8_t)(bgr2y(c00) + 0.5f);
    uint8_t y01 = (uint8_t)(bgr2y(c01) + 0.5f);
    uint8_t y10 = (uint8_t)(bgr2y(c10) + 0.5f);
    uint8_t y11 = (uint8_t)(bgr2y(c11) + 0.5f);

    uint8_t u = (uint8_t)((bgr2u(c00) + bgr2u(c01) + bgr2u(c10) + bgr2u(c11)) * 0.25f + 0.5f);
    uint8_t v = (uint8_t)((bgr2v(c00) + bgr2v(c01) + bgr2v(c10) + bgr2v(c11)) * 0.25f + 0.5f);

    dstImage[destPitch * y + x] = y00;
    dstImage[destPitch * y + x1] = y01;
    dstImage[destPitch * y1 + x] = y10;
    dstImage[destPitch * y1 + x1] = y11;

    uint32_t chromaOffset = destPitch * height;
    int32_t x_chroma = x;
    int32_t y_chroma = y >> 1;

    dstImage[chromaOffset + destPitch * y_chroma + x_chroma] = u;
    dstImage[chromaOffset + destPitch * y_chroma + x_chroma + 1] = v;
}

extern "C" cudaError_t RGBA2NV12(cudaArray *srcImage,
                                 uint8_t *dstImage, size_t destPitch,
                                 uint32_t width, uint32_t height)
{
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);

    // Set texture parameters
    texRef.addressMode[0] = cudaAddressModeWrap;
    texRef.addressMode[1] = cudaAddressModeWrap;
    texRef.filterMode = cudaFilterModePoint;
    texRef.normalized = false;

    cudaError_t cudaStatus = cudaBindTextureToArray(texRef, srcImage, channelDesc);
    if (cudaStatus != cudaSuccess)
    {
        return cudaStatus;
    }

    dim3 block(32, 16, 1);
    dim3 grid((width + (2 * block.x - 1)) / (2 * block.x), (height + (2 * block.y - 1)) / (2 * block.y), 1);

    RGBA2NV12_kernel<<<grid, block>>>(dstImage, destPitch, width, height);

    cudaDeviceSynchronize();

    cudaStatus = cudaGetLastError();
    return cudaStatus;
}

extern "C" cudaError_t BGRA2NV12(cudaArray *srcImage,
                                 uint8_t *dstImage, size_t destPitch,
                                 uint32_t width, uint32_t height)
{
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);

    // Set texture parameters
    texRef.addressMode[0] = cudaAddressModeWrap;
    texRef.addressMode[1] = cudaAddressModeWrap;
    texRef.filterMode = cudaFilterModePoint;
    texRef.normalized = false;

    cudaError_t cudaStatus = cudaBindTextureToArray(texRef, srcImage, channelDesc);
    if (cudaStatus != cudaSuccess)
    {
        return cudaStatus;
    }

    dim3 block(32, 16, 1);
    dim3 grid((width + (2 * block.x - 1)) / (2 * block.x), (height + (2 * block.y - 1)) / (2 * block.y), 1);

    BGRA2NV12_kernel<<<grid, block>>>(dstImage, destPitch, width, height);

    cudaDeviceSynchronize();

    cudaStatus = cudaGetLastError();
    return cudaStatus;
}

extern "C" cudaError_t BGRA2NV12_WithWaterMark(cudaArray *srcImage,
                                               uint8_t *dstImage, size_t destPitch,
                                               uint32_t width, uint32_t height,
                                               uint8_t *waterMarkImage, size_t waterMarkPitch,
                                               uint32_t markWidth, uint32_t markHeight,
                                               uint32_t markLocationX, uint32_t markLocationY)
{
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);

    // Set texture parameters
    texRef.addressMode[0] = cudaAddressModeWrap;
    texRef.addressMode[1] = cudaAddressModeWrap;
    texRef.filterMode = cudaFilterModePoint;
    texRef.normalized = false;
    uint8_t* WaterMarkPtr;
    cudaMalloc((void **)&WaterMarkPtr, markWidth*markHeight*waterMarkPitch);
    cudaMemcpy(WaterMarkPtr, waterMarkImage, markWidth*markHeight*waterMarkPitch, cudaMemcpyHostToDevice);
    cudaError_t cudaStatus = cudaBindTextureToArray(texRef, srcImage, channelDesc);
    if (cudaStatus != cudaSuccess)
    {
        cudaFree(WaterMarkPtr);
        return cudaStatus;
    }
    dim3 block(32, 16, 1);
    dim3 grid((width + (2 * block.x - 1)) / (2 * block.x), (height + (2 * block.y - 1)) / (2 * block.y), 1);

    BGRA2NV12_WithWaterMark_kernel<<<grid, block>>>(dstImage, destPitch, width, height, WaterMarkPtr, waterMarkPitch, markWidth, markHeight, markLocationX, markLocationY);

    cudaDeviceSynchronize();
    cudaFree(WaterMarkPtr);

    cudaStatus = cudaGetLastError();
    return cudaStatus;
}