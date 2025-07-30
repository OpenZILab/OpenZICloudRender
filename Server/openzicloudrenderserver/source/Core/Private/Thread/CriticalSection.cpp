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