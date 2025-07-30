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