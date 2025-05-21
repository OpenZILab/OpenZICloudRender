
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/27 14:28
//

#include "SystemTime.h"
#include "Logger/CommandLogger.h"

#if PLATFORM_WINDOWS

#include <Windows.h>

namespace OpenZI::CloudRender {
   bool FSystemTime::bInitialized = false;
   uint64_t FSystemTime::TicksPerSecond = 0;
   uint64_t FSystemTime::BaseTicks = 0;
   double FSystemTime::SecondsPerTick = 0;
   double FSystemTime::MillSecondsPerTick = 0;

   void FSystemTime::Init(uint64_t InBaseTicks) {
      bInitialized = true;
      LARGE_INTEGER Frequency;
      TicksPerSecond = QueryPerformanceFrequency(&Frequency) ? Frequency.QuadPart : 1000;
      SecondsPerTick = 1.0 / TicksPerSecond;
      MillSecondsPerTick = SecondsPerTick * 1000.f;
      BaseTicks = InBaseTicks ? InBaseTicks : GetInTicks();
   }

   uint64_t FSystemTime::GetBaseTicks() {
      if (!bInitialized)
         Init();
      return BaseTicks;
   }

   uint64_t FSystemTime::GetInTicks() {
      LARGE_INTEGER Counter;
      return QueryPerformanceCounter(&Counter) ? Counter.QuadPart : GetTickCount64();
   }

   double FSystemTime::GetInSeconds() {
      if (!bInitialized)
         Init();
      return (GetInTicks() - BaseTicks) * SecondsPerTick;
   }

   double FSystemTime::GetInSeconds(uint64_t Ticks) {
      if (!bInitialized)
         Init();
      return ((int64_t)(Ticks - BaseTicks)) * SecondsPerTick;
   }

   double FSystemTime::GetInMillSeconds(uint64_t Ticks) {
      if (!bInitialized)
         Init();
      return ((int64_t)(Ticks - BaseTicks)) * MillSecondsPerTick;
   }

   uint64_t GetTimestampUs() {
      FILETIME ft;
      GetSystemTimeAsFileTime(&ft);

      uint64_t Current = (((uint64_t)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
      // Convert to Unix Epoch
      Current -= 116444736000000000LL;
      Current /= 10;

      return Current;
   }
} // namespace OpenZI::CloudRender

#elif PLATFORM_LINUX

#include <ctime>
#include <cmath>

namespace OpenZI::CloudRender {

   namespace ETimespan {
      constexpr int64 MaxTicks = 9223372036854775807;
      constexpr int64 MinTicks = -9223372036854775807 - 1;
      constexpr int64 NanosecondsPerTick = 100;
      constexpr int64 TicksPerMicrosecond = 10;
      constexpr int64 TicksPerMillisecond = 10000;
      constexpr int64 TicksPerSecond = 10000000;
   }

   int64 GetNowTicks() {
      struct timespec ts;
      int ClockSource = -1;
      if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
         ClockSource = CLOCK_REALTIME;
      } else {
         ClockSource = CLOCK_MONOTONIC;
      }
      clock_gettime(ClockSource, &ts);
      double Seconds = static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1e9;
      return (int64)floor(Seconds * ETimespan::TicksPerSecond + 0.5);
   }

   double GetTotalMilliseconds(int64 Ticks) {
      return ((double)Ticks / ETimespan::TicksPerMillisecond);
   }

   double GetTotalSeconds(int64 Ticks) {
      return ((double)Ticks / ETimespan::TicksPerSecond);
   }

   bool FSystemTime::bInitialized = false;
   uint64_t FSystemTime::TicksPerSecond = 0;
   uint64_t FSystemTime::BaseTicks = 0;
   double FSystemTime::SecondsPerTick = 0;
   double FSystemTime::MillSecondsPerTick = 0;

   void FSystemTime::Init(uint64_t InBaseTicks) {
      bInitialized = true;
      struct timespec resolution;
      if (clock_getres(CLOCK_MONOTONIC, &resolution) == 0) {
         TicksPerSecond = static_cast<uint64_t>(1e9 / resolution.tv_nsec);
         SecondsPerTick = 1.0 / TicksPerSecond;
         // TicksPerSecond = static_cast<uint64_t>(1.0 / SecondsPerTick);
         MillSecondsPerTick = SecondsPerTick * 1000.f;
         BaseTicks = InBaseTicks ? InBaseTicks : GetInTicks();
      }
   }

   uint64_t FSystemTime::GetBaseTicks() {
      if (!bInitialized)
         Init();
      return BaseTicks;
   }

   uint64_t FSystemTime::GetInTicks() {
      // std::chrono::high_resolution_clock::duration Ticks =
      //     std::chrono::high_resolution_clock::now().time_since_epoch();
      // return Ticks.count();
      auto now = std::chrono::high_resolution_clock::now();
      auto duration = now.time_since_epoch();
      return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
      // auto now = std::chrono::high_resolution_clock::now();
      // return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
   }

   double FSystemTime::GetInSeconds() {
      if (!bInitialized)
         Init();
      return (GetInTicks() - BaseTicks) * SecondsPerTick;
   }

   double FSystemTime::GetInSeconds(uint64_t Ticks) {
      if (!bInitialized)
         Init();
      return ((int64_t)(Ticks - BaseTicks)) * SecondsPerTick;
   }

   double FSystemTime::GetInMillSeconds(uint64_t Ticks) {
      if (!bInitialized)
         Init();
      return ((int64_t)(Ticks - BaseTicks)) * MillSecondsPerTick;
   }

   uint64_t GetTimestampUs() {
      // return FSystemTime::GetInTicks();
      auto now = std::chrono::high_resolution_clock::now();
      auto duration = now.time_since_epoch();
      return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
      struct timespec ts;
      if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
         perror("Error getting UTC time");
         return 0;
      }
      return static_cast<uint64_t>(ts.tv_sec) * 1000000 + static_cast<uint64_t>(ts.tv_nsec) / 1000;
   }
} // namespace OpenZI::CloudRender

#endif