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