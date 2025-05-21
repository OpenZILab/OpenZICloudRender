//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/05 09:26
//

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