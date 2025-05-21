// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/31 14:21
// 
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