/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/
#pragma once

#include "Containers/Array.h"
#include "Containers/Queue.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/Event.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "RHI.h"
#include "RHIResources.h"
#include "Templates/SharedPointer.h"

namespace OpenZI::CloudRender {
    /*
     * A thread with a single purpose, to poll GPU fences until they are done.
     * Polling jobs can be submitted to this thread using its static methods.
     * Once a polling job is complete a callback is called on the submitted job.
     */
    class FGPUFencePoller {
    public:
        FGPUFencePoller();
        void Shutdown();
        virtual ~FGPUFencePoller();
        void AddFenceJob(FGPUFenceRHIRef Fence, TSharedRef<bool, ESPMode::ThreadSafe> bKeepPolling, TFunction<void()> FenceDoneCallback);
        static FGPUFencePoller* Get();

    private:
        struct FPollJob {
            FPollJob();
            FPollJob(FGPUFenceRHIRef InFence, TSharedRef<bool, ESPMode::ThreadSafe> bInKeepPolling, TFunction<void()> InFenceDoneCallback);
            FGPUFenceRHIRef Fence;
            TSharedRef<bool, ESPMode::ThreadSafe> bKeepPolling;
            TFunction<void()> FenceDoneCallback;
        };

        class FPollerRunnable : public FRunnable {
        public:
            FPollerRunnable();
            void AddFenceJob(FPollJob Job);
            bool IsRunning() const;
            // Begin FRunnable interface.
            virtual bool Init() override;
            virtual uint32 Run() override;
            virtual void Stop() override;
            virtual void Exit() override;
            // End FRunnable interface
        private:
            bool bIsRunning;
            FEvent* JobAddedEvent;
            TArray<FPollJob> PollJobs;
            TQueue<FPollJob, EQueueMode::Mpsc> JobsToAdd;
        };

    private:
        FPollerRunnable Runnable;
        FRunnableThread* Thread;

        static FGPUFencePoller* Instance;
    };
} // namespace OpenZI::CloudRender