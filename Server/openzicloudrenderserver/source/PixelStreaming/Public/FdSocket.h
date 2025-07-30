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

#if PLATFORM_LINUX
#include "Thread/Thread.h"
#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include <string>

namespace OpenZI::CloudRender {
    class ZFdSocket : public FThread {
    public:
        ZFdSocket(std::string InAddrPath);
        virtual ~ZFdSocket();
        virtual void Run() = 0;
    protected:

        virtual void SetNoBlock(int fd);

        std::string AddrPath;
        bool bExiting;
        int SocketFd;
    };


    class ZFdSocketServer : public ZFdSocket {
    public:
        ZFdSocketServer(std::string InAddrPath, int InMaxClient = 1);
        virtual ~ZFdSocketServer();
        virtual void Run() override;
        virtual int SendFd(int Fd);
        int Listen();
        int Accept();
    protected:
        int MaxClient;
        int ClientFd;
    };

    class ZFdSocketClient : public ZFdSocket {
    public:
        ZFdSocketClient(std::string InAddrPath, int InDelaySeconds = 0);
        virtual ~ZFdSocketClient();
        virtual int RecvFd();
        virtual void Run() override;
        virtual int Connect();
    };

    class ZInputSocketSender : public ZFdSocket {
    public:
        ZInputSocketSender(std::string InAddrPath);
        virtual ~ZInputSocketSender();
        virtual int Connect();
        virtual void Run() override;
        virtual int Send(uint8* Data, int Size);
        virtual void Enqueue(std::vector<uint8>&& Buffer);

        TQueue<std::vector<uint8>> ToSendQueue;
    };

} // namespace OpenZI::CloudRender

#endif