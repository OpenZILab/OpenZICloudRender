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

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeCounter.h"
#include "IPCTypes.h"
#include "Misc/Guid.h"

namespace OpenZI::CloudRender {

    class FInputReceiveThread : FRunnable {
    public:
        FInputReceiveThread();
        FInputReceiveThread(FGuid Guid);
        ~FInputReceiveThread();
        void StartThread();
        void StopThread();

        FString GetThreadName() const;

    protected:
        virtual bool Init() override;
        virtual uint32 Run() override final;
        virtual void Stop() override;
        virtual void Exit() override;

    private:
        FThreadSafeCounter ExitRequest;
        FRunnableThread* Thread;
        FString ThreadName;
        FString ShareMemoryName;
        FGuid ShmId;
        TSharedPtr<FShareMemoryWriter> Reader;
    };
} // namespace OpenZI::CloudRender