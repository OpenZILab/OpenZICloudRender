//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/05 09:26
//

#include "FdSocket.h"

#if PLATFORM_LINUX

#include "Config.h"
#include "Logger/CommandLogger.h"
#include "MessageCenter/MessageCenter.h"
#include "VulkanDynamicRHI.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <set>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace OpenZI::CloudRender {

    ZFdSocket::ZFdSocket(std::string InAddrPath)
        : AddrPath(InAddrPath), bExiting(false), SocketFd(-1) {}
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

    ZFdSocketServer::ZFdSocketServer(std::string InAddrPath, int InMaxClient)
        : ZFdSocket(InAddrPath), MaxClient(InMaxClient), ClientFd(-1) {
        // TODO: Check vaild
        Start();
    }

    ZFdSocketServer::~ZFdSocketServer() {
        bExiting = true;
        Join();
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

        unlink(AddrPath.c_str());
        struct sockaddr_un serverAddr;
        int ret = 0;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sun_family = AF_UNIX;
        strcpy(serverAddr.sun_path, AddrPath.c_str());
        uint32_t size = offsetof(struct sockaddr_un, sun_path) + strlen(AddrPath.c_str());
        ret = bind(SocketFd, (struct sockaddr*)&serverAddr, size);
        SetNoBlock(SocketFd);
        ret = listen(SocketFd, MaxClient);
        if (ret < 0) {
            ML_LOG(Core, Error, "FdSocket listen error, ret=%d", ret);
        }
        ML_LOG(Core, Log, "FdSocket listen fd: %d server:%s", SocketFd, serverAddr.sun_path);
        return SocketFd;
    }

    int ZFdSocketServer::Accept() {
        struct sockaddr_un clientAddr;
        socklen_t addrlen = sizeof(clientAddr);
        bzero(&clientAddr, addrlen);
        int fd = accept(SocketFd, (struct sockaddr*)&clientAddr, &addrlen);
        ML_LOG(Core, Log, "Accept socket client fd %d %s", fd, clientAddr.sun_path);
        return fd;
    }

    void ZFdSocketServer::Run() {
        int maxevents = 5;
        int epollFd = 0;
        epollFd = epoll_create(maxevents);
        if (epollFd == 0) {
            ML_LOG(Core, Error, "Failed to create epoll fd");
        }
        if (access(AddrPath.c_str(), F_OK)) {
            unlink(AddrPath.c_str()); // 删除该文件
        }
        int lfd = Listen();
        struct epoll_event event = {};
        event.events = EPOLLIN;
        // 绑定事件fd
        event.data.fd = lfd;
        if (lfd < 0) {
            ML_LOG(Core, error, "Failed to create listen socket");
        }
        int ret = epoll_ctl(epollFd, EPOLL_CTL_ADD, lfd, &event);
        if (ret < 0) {
            ML_LOG(Core, error, "Failed to epoll_ctl, ret=%d", ret);
        }
        struct epoll_event waitEvents[5] = {};

        while (!bExiting) {
            int number = epoll_wait(epollFd, waitEvents, 5, -1);
            for (int index = 0; index < number; index++) {
                if (waitEvents[index].events & EPOLLIN == EPOLLIN &&
                    waitEvents[index].data.fd == lfd) {
                    int confd = Accept();
                    ClientFd = confd;
                    struct iovec iov[1];
                    char dummy_data = 'D'; // 消息数据不可为空
                    iov[0].iov_base = &dummy_data;
                    iov[0].iov_len = 1;

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
                    // TODO: replace 1
                    *((int*)CMSG_DATA(cmsg)) = 1;
                    ssize_t send_result = sendmsg(confd, &msg, 0);
                    if (send_result == -1) {
                        perror("Error sending message");
                        // return 1;
                    }
                    struct epoll_event event = {};
                    event.events = EPOLLIN;
                    event.data.fd = confd;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, confd, &event);
                    continue;
                }
                if (waitEvents[index].events & EPOLLIN == EPOLLIN) {
                    int rfd = waitEvents[index].data.fd;
                    // char buff[1024];
                    // recv(rfd, buff, 1024, 0);
                    // send(rfd, "hello", 6, 0);
                    // 构造消息
                }
            }
        }
        close(epollFd);
    }
    int ZFdSocketServer::SendFd(int Fd) {
        if (ClientFd < 0) {
            return -1;
        }
        // 构造消息
        struct iovec iov[1];
        char dummy_data = 'D'; // 消息数据不可为空
        iov[0].iov_base = &dummy_data;
        iov[0].iov_len = 1;

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
            ML_LOG(Core, Error, "Failed to sendmsg");
        } else {
            ML_LOG(Core, Verbose, "sendmsg, fd=%d", Fd);
        }
        return 0;
    }

    ZFdSocketClient::ZFdSocketClient(std::string InAddrPath, int InDelaySeconds)
        : ZFdSocket(InAddrPath) {}

    ZFdSocketClient::~ZFdSocketClient() {
        bExiting = true;
        Join();
        if (SocketFd > 0) {
            close(SocketFd);
            SocketFd = -1;
        }
    }

    int ZFdSocketClient::RecvFd() { return -1; }

    void ZFdSocketClient::Run() {
        Connect();
        struct msghdr msg;
        int SharedHandle = 0;
        std::shared_ptr<FVulkanDynamicRHI> VulkanDynamicRHI;
        if (GDynamicRHI->GetName() == "Vulkan") {
            VulkanDynamicRHI = std::shared_ptr<FVulkanDynamicRHI>(
                static_cast<FVulkanDynamicRHI*>(GDynamicRHI.get()));
        }

        FTexture2DRHIRef SharedTexture;
        std::set<int> ToCloseCaches;
        while (!bExiting) {
            {
                if (ToCloseCaches.size() >= 100) {
                    for (const auto& FdCache : ToCloseCaches) {
                        if (fcntl(FdCache, F_GETFD) == 0) {
                            close(FdCache);
                            // ML_LOG(FdSocket, Verbose, "close fd %d", FdCache);
                        }
                    }
                    ToCloseCaches.clear();
                    // ML_LOG(FdSocket, Verbose, "Clear fds");
                }
            }
            struct iovec iov[1];
            std::vector<char> Buffer(32);
            iov[0].iov_base = Buffer.data();
            iov[0].iov_len = Buffer.size();

            char cmsgbuf[CMSG_SPACE(sizeof(int))]; // 存放控制消息的缓冲区
            memset(&msg, 0, sizeof(msg));
            msg.msg_name = NULL;
            msg.msg_namelen = 0;
            msg.msg_iov = iov;
            msg.msg_iovlen = 1;
            msg.msg_control = cmsgbuf;
            msg.msg_controllen = sizeof(cmsgbuf);

            ssize_t recv_result = recvmsg(SocketFd, &msg, 0);
            if (recv_result == -1) {
                ML_LOG(FdSocket, Error, "recvmsg error");
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                continue;
            } else {
                // 从控制消息中获取文件描述符
                int received_fd = -1;
                struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
                if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
                    received_fd = *((int*)CMSG_DATA(cmsg));
                    ToCloseCaches.emplace(received_fd);
                    auto ReadData = Buffer.data();
                    // send texture
                    uint32 ReceiveTextureMemorySize = *(uint32*)ReadData;
                    uint32 ReceiveTextureMemoryOffset = *(uint32*)(ReadData + sizeof(uint32));
                    FAppConfig::Get().ResolutionX = *(uint32*)(ReadData + sizeof(uint32) * 2);
                    FAppConfig::Get().ResolutionY = *(uint32*)(ReadData + sizeof(uint32) * 3);
                    FAppConfig::Get().Width = *(uint32*)(ReadData + sizeof(uint32) * 4);
                    FAppConfig::Get().Height = *(uint32*)(ReadData + sizeof(uint32) * 5);
                    // ML_LOG(FdSocket, Verbose, "Res=%dx%d", FAppConfig::Get().ResolutionX,
                    // FAppConfig::Get().ResolutionY);
                    SharedTexture = VulkanDynamicRHI->GetTextureRef(
                        received_fd, ReceiveTextureMemorySize, ReceiveTextureMemoryOffset);
                    if (SharedTexture) {
                        PUBLISH_MESSAGE(OnTextureReceived, SharedTexture);
                        // ML_LOG(FdSocket, Verbose, "Cache fd: %d", received_fd);
                    }
                } else {
                    ML_LOG(FdSocket, Verbose, "Failed to recv cmsg");
                }
            }
        }
    }

    int ZFdSocketClient::Connect() {
        SocketFd = socket(PF_UNIX, SOCK_STREAM, 0);
        SetNoBlock(SocketFd);

        struct sockaddr_un serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));

        serverAddr.sun_family = AF_UNIX;
        strcpy(serverAddr.sun_path, AddrPath.c_str());

        uint32_t size = offsetof(struct sockaddr_un, sun_path) + strlen(serverAddr.sun_path);

        int ret = connect(SocketFd, (struct sockaddr*)&serverAddr, size);
        if (ret != 0) {
            printf("connect failed\n");
        }
        printf("CreatePipeSocket connect fd: %d server: %s\n", SocketFd, serverAddr.sun_path);
        return SocketFd;
    }

    ZInputSocketSender::ZInputSocketSender(std::string InAddrPath) : ZFdSocket(InAddrPath) {
        Start();
    }

    void ZInputSocketSender::Run() {
        Connect();

        while (!bExiting) {
            std::vector<uint8> Buffer;
            while (ToSendQueue.Dequeue(Buffer)) {
                Send(Buffer.data(), Buffer.size());
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

    int ZInputSocketSender::Send(uint8* Data, int Size) {
        struct iovec iov[1];
        iov[0].iov_base = Data;
        iov[0].iov_len = Size;
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_control = nullptr;
        msg.msg_controllen = 0;
        // ssize_t send_result = sendmsg(SocketFd, &msg, 0);
        ssize_t send_result = send(SocketFd, Data, Size, 0);
        if (send_result == -1) {
            ML_LOG(InputSocket, Error, "Failed to send input msg");
        } else {
            ML_LOG(InputSocket, Verbose, "Succeed to send input msg");
        }
    }

    void ZInputSocketSender::Enqueue(std::vector<uint8>&& Buffer) { ToSendQueue.Enqueue(Buffer); }

    ZInputSocketSender::~ZInputSocketSender() {
        bExiting = true;
        Join();
        if (SocketFd > 0) {
            close(SocketFd);
            SocketFd = -1;
        }
    }
    int ZInputSocketSender::Connect() {

        SocketFd = socket(PF_UNIX, SOCK_STREAM, 0);
        SetNoBlock(SocketFd);

        struct sockaddr_un serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));

        serverAddr.sun_family = AF_UNIX;
        strcpy(serverAddr.sun_path, AddrPath.c_str());

        uint32_t size = offsetof(struct sockaddr_un, sun_path) + strlen(serverAddr.sun_path);

        int ret = connect(SocketFd, (struct sockaddr*)&serverAddr, size);
        if (ret != 0) {
            printf("connect failed\n");
        }
        printf("CreatePipeSocket connect fd: %d server: %s\n", SocketFd, serverAddr.sun_path);
        return SocketFd;
    }
}

#endif