// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/11 14:12
// 

#pragma once

#if PLATFORM_WINDOWS
#include <windows.h>
#elif PLATFORM_LINUX
#include <pthread.h>
#endif

namespace OpenZI
{
    class FCriticalSection
    {
    public:
        FCriticalSection();
        ~FCriticalSection();

        FCriticalSection(const FCriticalSection&) = delete;
        FCriticalSection& operator=(const FCriticalSection&) = delete;

        void Enter();
        void Leave();

    private:
#if PLATFORM_WINDOWS
        CRITICAL_SECTION CriticalSection;
#elif PLATFORM_LINUX
        // Linux only has mutex
        pthread_mutex_t CriticalSection;
#endif
    };

    class FScopeLock
    {
    public:
        explicit FScopeLock(FCriticalSection *InSyncObject);
        ~FScopeLock() noexcept;

        FScopeLock(const FScopeLock&) = delete;
        FScopeLock& operator=(const FScopeLock&) = delete;
    private:
        FCriticalSection* SyncObject;
    };
} // namespace OpenZI