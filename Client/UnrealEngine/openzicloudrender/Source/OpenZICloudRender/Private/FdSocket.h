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
#if PLATFORM_LINUX
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/Guid.h"

namespace OpenZI::CloudRender {
    class ZFdSocket : public FRunnable {
    public:
        ZFdSocket(char* InAddrPath);
        virtual ~ZFdSocket();

    protected:
        // virtual bool Init() = 0;
        // virtual uint32 Run() = 0;
        // virtual void Stop() = 0;
        // virtual void Exit() = 0;

    protected:
        FThreadSafeCounter ExitRequest;
        FRunnableThread* Thread;

    protected:
        virtual void SetNoBlock(int fd);

        char* AddrPath;
        bool bExiting;
        int SocketFd;
    };

    class ZFdSocketServer : public ZFdSocket {
    public:
        ZFdSocketServer(char* InAddrPath, int InMaxClient = 1);
        virtual ~ZFdSocketServer();
        virtual int Send(int Fd, char* Data, int Size);
        virtual void StartThread();
        virtual void StopThread();
        virtual int Listen();
        virtual int Accept();

    protected:
        virtual bool Init() override;
        virtual uint32 Run() override;
        virtual void Stop() override;
        virtual void Exit() override;

    protected:
        int MaxClient;
        int ClientFd;
    };

    class ZInputSocketReceiver : public ZFdSocketServer {
    public:
        ZInputSocketReceiver(char* InAddrPath);
    protected:
        virtual uint32 Run() override;
    };
} // namespace OpenZI::CloudRender

#endif