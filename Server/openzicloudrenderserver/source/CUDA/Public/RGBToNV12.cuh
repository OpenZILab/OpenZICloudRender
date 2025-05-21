//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/27 14:18
//
#pragma once

#include <cuda.h>

extern "C" cudaError_t RGBA2NV12(cudaArray *srcImage,
                                 uint8_t *dstImage, size_t destPitch,
                                 uint32_t width, uint32_t height);

extern "C" cudaError_t BGRA2NV12(cudaArray *srcImage,
                                 uint8_t *dstImage, size_t destPitch,
                                 uint32_t width, uint32_t height);

extern "C" cudaError_t BGRA2NV12_WithWaterMark(cudaArray *srcImage,
                                 uint8_t *dstImage, size_t destPitch,
                                 uint32_t width, uint32_t height,
                                 uint8_t *waterMarkImage, size_t waterMarkPitch,
                                 uint32_t markWidth, uint32_t markHeight,
                                 uint32_t markLocationX, uint32_t markLocationY);