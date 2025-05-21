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
#include "GPUFencePoller.h"

namespace OpenZI::CloudRender {
    /*
     * ------------- FGPUFencePoller ---------------------
     */

    FGPUFencePoller* FGPUFencePoller::Instance = nullptr;

    FGPUFencePoller* FGPUFencePoller::Get() {
        checkf(Instance, TEXT("You should not try to Get() and instance of the poller before it has been constructed somewhere yet."));
        return FGPUFencePoller::Instance;
    }

    FGPUFencePoller::FGPUFencePoller()
        : Runnable(), Thread(FRunnableThread::Create(&Runnable, TEXT("OpenZICloudRender GPUFencePoller"))) {
        FGPUFencePoller::Instance = this;
    }

    FGPUFencePoller::~FGPUFencePoller() {
        Shutdown();
        Thread->Kill(true);
    }

    void FGPUFencePoller::Shutdown() {
        Runnable.Stop();
    }

    void FGPUFencePoller::AddFenceJob(FGPUFenceRHIRef Fence, TSharedRef<bool, ESPMode::ThreadSafe> bKeepPolling, TFunction<void()> FenceDoneCallback) {
        checkf(Runnable.IsRunning(), TEXT("Poller runnable must be running to submit jobs to it."));
        Runnable.AddFenceJob({Fence, bKeepPolling, FenceDoneCallback});
    }

    /*
     * ------------------- FPollJob ----------------------
     */

    FGPUFencePoller::FPollJob::FPollJob(FGPUFenceRHIRef InFence, TSharedRef<bool, ESPMode::ThreadSafe> bInKeepPolling, TFunction<void()> InFenceDoneCallback)
        : Fence(InFence), bKeepPolling(bInKeepPolling), FenceDoneCallback(InFenceDoneCallback) {
    }

    FGPUFencePoller::FPollJob::FPollJob()
        : FPollJob(FGPUFenceRHIRef(), MakeShared<bool, ESPMode::ThreadSafe>(false), []() {}) {
    }

    /*
     * ------------- FPollerRunnable ---------------------
     */

    FGPUFencePoller::FPollerRunnable::FPollerRunnable()
        : bIsRunning(false), JobAddedEvent(FPlatformProcess::GetSynchEventFromPool(false)) {
    }

    bool FGPUFencePoller::FPollerRunnable::IsRunning() const {
        return bIsRunning;
    }

    bool FGPUFencePoller::FPollerRunnable::Init() {
        return true;
    }

    void FGPUFencePoller::FPollerRunnable::AddFenceJob(FPollJob Job) {
        // Note: this is threadsafe because use of TQueue
        JobsToAdd.Enqueue(Job);
        JobAddedEvent->Trigger();
    }

    uint32 FGPUFencePoller::FPollerRunnable::Run() {
        bIsRunning = true;

        while (bIsRunning) {

            // Add any new jobs
            while (!JobsToAdd.IsEmpty()) {
                FPollJob* Job = JobsToAdd.Peek();
                if (Job) {
                    PollJobs.Add(*Job);
                }
                JobsToAdd.Pop();
            }

            // Iterate backwards so we can remove elements if the job is completed
            for (int i = PollJobs.Num() - 1; i >= 0; i--) {
                FPollJob& Job = PollJobs[i];
                if (Job.bKeepPolling.Get() == false) {
                    PollJobs.RemoveAt(i);
                    continue;
                }

                bool bJobCompleted = Job.Fence->Poll();
                if (bJobCompleted) {
                    Job.FenceDoneCallback();
                    PollJobs.RemoveAt(i);
                }
            }

            // No jobs so sleep this thread waiting for a job to be added.
            if (PollJobs.Num() == 0) {
                JobAddedEvent->Wait();
            } else {
                // Free CPU usage up for other threads.
                FPlatformProcess::Sleep(0.0f);
            }
        }

        return 0;
    }

    void FGPUFencePoller::FPollerRunnable::Stop() {
        bIsRunning = false;
        JobAddedEvent->Trigger();
    }

    void FGPUFencePoller::FPollerRunnable::Exit() {
        bIsRunning = false;
        JobAddedEvent->Trigger();
    }
} // namespace OpenZI::CloudRender