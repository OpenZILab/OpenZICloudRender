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

#include "FdSocket.h"

#if PLATFORM_LINUX
#include "OpenZICloudRenderModule.h"
#include "HAL/RunnableThread.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"

#include <vector>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace OpenZI::CloudRender {

    ZFdSocket::ZFdSocket(char* InAddrPath) : AddrPath(InAddrPath), bExiting(false), SocketFd(-1) {}
    ZFdSocket::~ZFdSocket() {
        if (SocketFd > 0) {
            close(SocketFd);
        }
    }
    void ZFdSocket::SetNoBlock(int fd) {
        int option = fcntl(fd, F_GETFD);
        if (option < 0) {
            return;
        }

        option = option | O_NONBLOCK | FD_CLOEXEC;
        option = fcntl(fd, F_SETFD, option);
        if (option < 0) {
            return;
        }
        return;
    }

    ZFdSocketServer::ZFdSocketServer(char* InAddrPath, int InMaxClient)
        : ZFdSocket(InAddrPath), MaxClient(InMaxClient), ClientFd(-1) {
        // TODO: Check vaild
        StartThread();
    }

    ZFdSocketServer::~ZFdSocketServer() {
        StopThread();
        if (ClientFd > 0) {
            close(ClientFd);
            ClientFd = -1;
        }
        if (SocketFd > 0) {
            close(SocketFd);
            SocketFd = -1;
        }
    }

    int ZFdSocketServer::Listen() {
        SocketFd = socket(PF_UNIX, SOCK_STREAM, 0);

        unlink(AddrPath);
        struct sockaddr_un serverAddr;
        int ret = 0;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sun_family = AF_UNIX;
        strcpy(serverAddr.sun_path, AddrPath);
        uint32_t size = offsetof(struct sockaddr_un, sun_path) + strlen(AddrPath);
        ret = bind(SocketFd, (struct sockaddr*)&serverAddr, size);
        SetNoBlock(SocketFd);
        ret = listen(SocketFd, MaxClient);
        if (ret < 0) {
            UE_LOG(LogTemp, Error, TEXT("FdSocket listen error, ret=%d"), ret);
        }
        return SocketFd;
    }

    int ZFdSocketServer::Accept() {
        struct sockaddr_un clientAddr;
        socklen_t addrlen = sizeof(clientAddr);
        bzero(&clientAddr, addrlen);
        int fd = accept(SocketFd, (struct sockaddr*)&clientAddr, &addrlen);
        UE_LOG(LogTemp, Log, TEXT("Accept socket client fd %d %s"), fd, clientAddr.sun_path);
        return fd;
    }

    uint32 ZFdSocketServer::Run() {

        int maxevents = 5;
        int epollFd = 0;
        epollFd = epoll_create(maxevents);
        if (epollFd == 0) {
            UE_LOG(LogTemp, Error, TEXT("Failed to create epoll fd"));
        }
        if (access(AddrPath, F_OK)) {
            unlink(AddrPath); // 删除该文件
        }
        int lfd = Listen();
        struct epoll_event event = {};
        event.events = EPOLLIN;
        // 绑定事件fd
        event.data.fd = lfd;
        if (lfd < 0) {
            UE_LOG(LogTemp, Error, TEXT("Failed to create listen socket"));
        }
        int ret = epoll_ctl(epollFd, EPOLL_CTL_ADD, lfd, &event);
        if (ret < 0) {
            UE_LOG(LogTemp, Error, TEXT("Failed to epoll_ctl, ret=%d"), ret);
        }
        struct epoll_event waitEvents[5] = {};

        while (!ExitRequest.GetValue()) {
            int number = epoll_wait(epollFd, waitEvents, 5, -1);
            for (int index = 0; index < number; index++) {
                if ((waitEvents[index].events & EPOLLIN) == EPOLLIN &&
                    waitEvents[index].data.fd == lfd) {
                    int confd = Accept();
                    ClientFd = confd;
                    struct epoll_event newevent = {};
                    newevent.events = EPOLLIN;
                    newevent.data.fd = confd;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, confd, &newevent);
                    continue;
                }
                if ((waitEvents[index].events & EPOLLIN) == EPOLLIN) {
                    int rfd = waitEvents[index].data.fd;
                    // char buff[1024];
                    // recv(rfd, buff, 1024, 0);
                    // send(rfd, "hello", 6, 0);
                    // 构造消息
                }
            }
        }
        close(epollFd);
        return 0;
    }

    int ZFdSocketServer::Send(int Fd, char* Data, int Size) {
        if (ClientFd < 0) {
            return -1;
        }
        // 构造消息
        struct iovec iov[1];
        iov[0].iov_base = Data;
        iov[0].iov_len = Size;

        char cmsgbuf[CMSG_SPACE(sizeof(int))]; // 存放控制消息的缓冲区
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgbuf;
        msg.msg_controllen = sizeof(cmsgbuf);

        // 构造控制消息，将文件描述符添加到控制消息中
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        *((int*)CMSG_DATA(cmsg)) = Fd;
        ssize_t send_result = sendmsg(ClientFd, &msg, 0);
        if (send_result == -1) {
            UE_LOG(LogTemp, Verbose, TEXT("Failed to sendmsg"));
        } else {
            UE_LOG(LogTemp, Verbose, TEXT("sendmsg, fd=%d"), Fd);
        }
        return 0;
    }

    void ZFdSocketServer::StartThread() {
        Thread =
            FRunnableThread::Create(this, TEXT("FdSocketServer"), 0, EThreadPriority::TPri_Highest);
    }

    void ZFdSocketServer::StopThread() {
        if (Thread != nullptr) {
            Thread->Kill(true);
            delete Thread;
            Thread = nullptr;
        }
    }

    bool ZFdSocketServer::Init() {
        ExitRequest.Set(false);
        return true;
    }

    void ZFdSocketServer::Stop() { ExitRequest.Set(true); }

    void ZFdSocketServer::Exit() {
        // empty
    }
    ZInputSocketReceiver::ZInputSocketReceiver(char* InAddrPath) : ZFdSocketServer(InAddrPath) {}

    uint32 ZInputSocketReceiver::Run() {
        FInputDevice& InputDevice =
            static_cast<FInputDevice&>(IOpenZICloudRenderModule::Get().GetInputDevice());
        int maxevents = 5;
        int epollFd = 0;
        epollFd = epoll_create(maxevents);
        if (epollFd == 0) {
            UE_LOG(LogTemp, Error, TEXT("Failed to create epoll fd"));
        }
        if (access(AddrPath, F_OK)) {
            unlink(AddrPath); // 删除该文件
        }
        int lfd = Listen();
        struct epoll_event event = {};
        event.events = EPOLLIN;
        // 绑定事件fd
        event.data.fd = lfd;
        if (lfd < 0) {
            UE_LOG(LogTemp, Error, TEXT("Failed to create listen socket"));
        }
        int ret = epoll_ctl(epollFd, EPOLL_CTL_ADD, lfd, &event);
        if (ret < 0) {
            UE_LOG(LogTemp, Error, TEXT("Failed to epoll_ctl, ret=%d"), ret);
        }
        struct epoll_event waitEvents[5] = {};

        while (!ExitRequest.GetValue()) {
            int number = epoll_wait(epollFd, waitEvents, 5, -1);
            for (int index = 0; index < number; index++) {
                if ((waitEvents[index].events & EPOLLIN) == EPOLLIN &&
                    waitEvents[index].data.fd == lfd) {
                    int confd = Accept();
                    ClientFd = confd;
                    struct epoll_event newevent = {};
                    newevent.events = EPOLLIN;
                    newevent.data.fd = confd;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, confd, &newevent);
                    continue;
                }
            }
            if (ClientFd < 0) {
                continue;
            }
            struct iovec iov[1];
            std::vector<uint8> Buffer(1024);
            iov[0].iov_base = Buffer.data();
            iov[0].iov_len = Buffer.size();
            struct msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_name = NULL;
            msg.msg_namelen = 0;
            msg.msg_iov = iov;
            msg.msg_iovlen = 1;

            ssize_t recv_result = recvmsg(ClientFd, &msg, 0);
            if (recv_result == -1) {

            } else {
                if (GEngine && GEngine->GameViewport != nullptr) {
                    InputDevice.OnMessage(Buffer.data(), Buffer.size());
                }
            }
        }
        close(epollFd);
        return 0;
    }
}

#endif