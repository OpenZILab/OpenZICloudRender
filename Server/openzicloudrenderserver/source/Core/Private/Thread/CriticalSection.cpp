// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/11 14:25
// 
#include "CriticalSection.h"

namespace OpenZI
{
#if PLATFORM_WINDOWS
    FCriticalSection::FCriticalSection()
    {
        InitializeCriticalSection(&CriticalSection);
    }

    FCriticalSection::~FCriticalSection()
    {
        DeleteCriticalSection(&CriticalSection);
    }


    void FCriticalSection::Enter()
    {
        EnterCriticalSection(&CriticalSection);
    }

    void FCriticalSection::Leave()
    {
        LeaveCriticalSection(&CriticalSection);
    }
#elif PLATFORM_LINUX
    FCriticalSection::FCriticalSection()
    {
        pthread_mutex_init(&CriticalSection, nullptr);
    }

    FCriticalSection::~FCriticalSection()
    {
        pthread_mutex_destroy(&CriticalSection);
    }


    void FCriticalSection::Enter()
    {
        pthread_mutex_lock(&CriticalSection);
    }

    void FCriticalSection::Leave()
    {
        pthread_mutex_unlock(&CriticalSection);
    }
#endif

    FScopeLock::FScopeLock(FCriticalSection *InSyncObject)
        : SyncObject(InSyncObject)
    {
        SyncObject->Enter();
    }

    FScopeLock::~FScopeLock() noexcept
    {
        if (SyncObject)
        {
            SyncObject->Leave();
            // SyncObject = nullptr;
        }
    }
} // namespace OpenZI