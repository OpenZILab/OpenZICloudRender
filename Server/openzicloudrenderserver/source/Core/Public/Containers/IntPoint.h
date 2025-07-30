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

#include "CoreMinimal.h"

namespace OpenZI
{
    struct FIntPoint
    {
    public:
        /** Holds the point's x-coordinate. */
        int32 X;

        /** Holds the point's y-coordinate. */
        int32 Y;

    public:
        /** Default constructor (no initialization). */
        FIntPoint() {}

        /**
         * Create and initialize a new instance with the specified coordinates.
         *
         * @param InX The x-coordinate.
         * @param InY The y-coordinate.
         */
        FIntPoint(int32 InX, int32 InY) 
            : X(InX), Y(InY)
        {
        }

        /**
         * Create and initialize a new instance with a single int.
         * Both X and Y will be initialized to this value
         *
         * @param InXY The x and y-coordinate.
         */
        FIntPoint(int32 InXY)
            : X(InXY), Y(InXY)
        {
        }
    };
} // namespace OpenZI