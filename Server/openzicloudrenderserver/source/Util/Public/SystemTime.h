// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/27 14:25
// 
#pragma once

#if PLATFORM_WINDOWS

#include <stdint.h>

namespace OpenZI::CloudRender
{
    class FSystemTime
    {
    public:
        // Automatically invoked, but can be called to specify a common base ticks for synchronization between processes.
        static void Init(uint64_t InBaseTicks = 0);
        // Returns the base ticks (for synchronizing with another process).
        static uint64_t GetBaseTicks();
        // Returns current system time in ticks.
        static uint64_t GetInTicks();
        // Returns current system time in seconds.
        static double GetInSeconds();
        // Converts ticks to seconds.
        static double GetInSeconds(uint64_t Ticks);
        static double GetInMillSeconds(uint64_t Ticks);

    private:
        static bool bInitialized;
        static uint64_t TicksPerSecond;
        static uint64_t BaseTicks;
        static double SecondsPerTick;
        static double MillSecondsPerTick;
    };
    uint64_t GetTimestampUs();
} // namespace OpenZI::CloudRender

#elif PLATFORM_LINUX

#include <stdint.h>
#include <chrono>
#include <ctime>
#include "CoreMinimal.h"

namespace OpenZI::CloudRender
{
    class FSystemTime
    {
    public:
        // Automatically invoked, but can be called to specify a common base ticks for synchronization between processes.
        static void Init(uint64_t InBaseTicks = 0);
        // Returns the base ticks (for synchronizing with another process).
        static uint64_t GetBaseTicks();
        // Returns current system time in ticks.
        static uint64_t GetInTicks();
        // Returns current system time in seconds.
        static double GetInSeconds();
        // Converts ticks to seconds.
        static double GetInSeconds(uint64_t Ticks);
        static double GetInMillSeconds(uint64_t Ticks);

    private:
        static bool bInitialized;
        static uint64_t TicksPerSecond;
        static uint64_t BaseTicks;
        static double SecondsPerTick;
        static double MillSecondsPerTick;
    };
    uint64_t GetTimestampUs();
    int64 GetNowTicks();
    double GetTotalMilliseconds(int64 Ticks);
    double GetTotalSeconds(int64 Ticks);
} // namespace OpenZI::CloudRender

#endif