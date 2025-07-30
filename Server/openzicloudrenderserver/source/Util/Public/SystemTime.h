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