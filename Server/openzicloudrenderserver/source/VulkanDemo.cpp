#include "Config.h"
#include "Logger/CommandLogger.h"
#include "UnrealReceiveThread.h"
#include "VideoEncoderNvEnc.h"
#include "VulkanDynamicRHI.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <vulkan/vulkan.h>

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

// 定义VkImage对象的宽高
#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 1024
using namespace OpenZI;
using namespace OpenZI::CloudRender;
void SetNoBlock(int fd) {
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

static int CreatePipeSocket_(const char* server) {
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    SetNoBlock(fd);

    struct sockaddr_un serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, server);

    uint32_t size = offsetof(struct sockaddr_un, sun_path) + strlen(serverAddr.sun_path);

    int ret = connect(fd, (struct sockaddr*)&serverAddr, size);
    if (ret != 0) {
        printf("connect failed\n");
    }
    printf("CreatePipeSocket connect fd: %d server: %s\n", fd, serverAddr.sun_path);
    return fd;
}

std::unique_ptr<FVideoEncoder> Encoder = nullptr;

void InitDynamicRHI() {
    if (GDynamicRHI)
        return;
    std::string RHIName = FAppConfig::Get().RHIName;
    if (RHIName == "Vulkan") {
        GDynamicRHI = std::make_shared<FVulkanDynamicRHI>();
    }
}

VkDeviceMemory deviceMemory;

int main(int argc, char** argv) {
    if (argc >= 2) {
        std::string GUID = argv[1];
        FAppConfig::Get().ShareMemorySuffix = GUID;
        auto Id = static_cast<uint32>(atol(GUID.c_str()));
        FAppConfig::Get().Guid = Id;
        ML_LOG(LogMain, Success, "ShmSuffix=%s,ShmId=%d",
               FAppConfig::Get().ShareMemorySuffix.c_str(), Id);
    }
    if (argc >= 3 && FAppConfig::Get().bUseGraphicsAdapter) {
        FAppConfig::Get().GpuIndex = atoi(argv[2]);
        ML_LOG(LogMain, Success, "graphicsadapter=%d", FAppConfig::Get().GpuIndex);
    }
    if (argc >= 5) {
        FAppConfig::Get().AdapterDeviceId = atoi(argv[3]);
        std::string UUID = argv[4];
        // ML_LOG(LogMain, Success, "DeviceId=%d UUID=%s", FAppConfig::Get().AdapterDeviceId,
        //        UUID.c_str());
        FAppConfig::Get().PipelineUUID = UUID;
    }
    InitDynamicRHI();
    auto VulkanDynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI.get());
    auto device = VulkanDynamicRHI->GetDevice();

    int confd = CreatePipeSocket_("/tmp/OpenZILab");
    struct msghdr msg;
    while (1) {
        sleep(1);
        // 接收消息（包含文件描述符）
        struct iovec iov[1];
        char dummy_data; // 接收数据，实际无意义
        iov[0].iov_base = &dummy_data;
        iov[0].iov_len = 1;

        char cmsgbuf[CMSG_SPACE(sizeof(int))]; // 存放控制消息的缓冲区
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgbuf;
        msg.msg_controllen = sizeof(cmsgbuf);
        bool bReceived = false;
        ssize_t recv_result = recvmsg(confd, &msg, 0);
        if (recv_result == -1) {
            perror("Error receiving message");
        } else {
            break;
        }
    }

    // 从控制消息中获取文件描述符
    int received_fd = -1;
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
        received_fd = *((int*)CMSG_DATA(cmsg));
    }

    // 设置文件描述符的文件描述符标志，确保进程B有权限访问传递的文件描述符
    // fcntl(received_fd, F_SETFD, fcntl(received_fd, F_GETFD) & ~FD_CLOEXEC);

    // {
    //     VkImage image;
    //     VkExternalMemoryImageCreateInfo ExternalMemoryImageCreateInfo;
    //     ZeroVulkanStruct(ExternalMemoryImageCreateInfo,
    //                      VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO);
    //     ExternalMemoryImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    //     VkImageCreateInfo imageCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    //                                          .imageType = VK_IMAGE_TYPE_2D,
    //                                          .format = VK_FORMAT_R8G8B8A8_UNORM,
    //                                          .extent = {IMAGE_WIDTH, IMAGE_HEIGHT, 1},
    //                                          .mipLevels = 1,
    //                                          .arrayLayers = 1,
    //                                          .samples = VK_SAMPLE_COUNT_1_BIT,
    //                                          .tiling = VK_IMAGE_TILING_OPTIMAL,
    //                                          .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    //                                                   VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    //                                          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    //                                          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //                                          .pNext = &ExternalMemoryImageCreateInfo};
    //     VkResult result = vkCreateImage(device, &imageCreateInfo, NULL, &image);
    //     if (result != VK_SUCCESS) {
    //         // 处理创建VkImage对象失败的情况
    //         ML_LOG(Main, Error, "Failed to create vulkan image");
    //     }

    //     VkMemoryRequirements memoryRequirements;
    //     vkGetImageMemoryRequirements(device, image, &memoryRequirements);

    //     uint32 memoryTypeIndex = VulkanDynamicRHI->FindMemoryTypeIndex(
    //         memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //     VkMemoryDedicatedAllocateInfo dedicatedMemoryInfo;
    //     ZeroVulkanStruct(dedicatedMemoryInfo, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO);
    //     dedicatedMemoryInfo.image = image;

    //     VkExportMemoryAllocateInfo extAllocateInfo = {
    //         .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
    //         .pNext = &dedicatedMemoryInfo,
    //         .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT};
    //     VkMemoryAllocateInfo allocateInfo = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //                                          .allocationSize = memoryRequirements.size,
    //                                          .memoryTypeIndex = memoryTypeIndex,
    //                                          .pNext = &extAllocateInfo};

    //     result = vkAllocateMemory(device, &allocateInfo, nullptr, &deviceMemory);
    //     if (result != VK_SUCCESS) {
    //         ML_LOG(Main, Error, "Failed to allocate vulkan memory");
    //     }

    //     // result = vkBindImageMemory(device, image, deviceMemory, 0);
    //     // if (result != VK_SUCCESS) {
    //     //     ML_LOG(Main, Error, "Failed to bind image memory");
    //     // }
    //     VkMemoryGetFdInfoKHR memoryGetFdInfo = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
    //                                             .pNext = nullptr,
    //                                             .memory = deviceMemory,
    //                                             .handleType =
    //                                                 VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT};

    //     static int fd = 0;
    //     // result = vkGetMemoryFdKHR(device, &memoryGetFdInfo, &fd);
    //     result = VulkanDynamicRHI->vkGetMemoryFdKHR(device, &memoryGetFdInfo, &received_fd);
    //     if (result != VK_SUCCESS) {
    //         // 处理获取句柄失败的情况
    //         ML_LOG(Main, Error, "Failed to get memory fd");
    //     }
    // }
    // Encoder = std::make_unique<FVideoEncoderNvEnc>();

    // VkImage image;
    // VkImageCreateInfo imageCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    //                                      .imageType = VK_IMAGE_TYPE_2D,
    //                                      .format = VK_FORMAT_R8G8B8A8_UNORM,
    //                                      .extent = {IMAGE_WIDTH, IMAGE_HEIGHT, 1},
    //                                      .mipLevels = 1,
    //                                      .arrayLayers = 1,
    //                                      .samples = VK_SAMPLE_COUNT_1_BIT,
    //                                      .tiling = VK_IMAGE_TILING_OPTIMAL,
    //                                      .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    //                                               VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    //                                      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    //                                      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    // VkResult result = vkCreateImage(device, &imageCreateInfo, NULL, &image);
    // if (result != VK_SUCCESS) {
    //     // 处理创建VkImage对象失败的情况
    //     ML_LOG(Main, Error, "Failed to create vulkan image");
    // }

    // VkMemoryRequirements memoryRequirements;
    // vkGetImageMemoryRequirements(device, image, &memoryRequirements);

    // uint32 memoryTypeIndex = VulkanDynamicRHI->FindMemoryTypeIndex(
    //     memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // VkMemoryAllocateInfo allocateInfo = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //                                      .allocationSize = memoryRequirements.size,
    //                                      .memoryTypeIndex = memoryTypeIndex,
    //                                      .pNext = nullptr};

    // result = vkAllocateMemory(device, &allocateInfo, nullptr, &deviceMemory);
    // if (result != VK_SUCCESS) {
    //     ML_LOG(Main, Error, "Failed to allocate vulkan memory");
    // }

    // result = vkBindImageMemory(device, image, deviceMemory, 0);
    // if (result != VK_SUCCESS) {
    //     ML_LOG(Main, Error, "Failed to bind image memory");
    // }
    // VkMemoryGetFdInfoKHR memoryGetFdInfo = {
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
    //     .pNext = NULL,
    //     .memory = deviceMemory,
    //     .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR};

    // int fd;

    // result = VulkanDynamicRHI->vkGetMemoryFdKHR(device, &memoryGetFdInfo, &fd);
    // if (result != VK_SUCCESS) {
    //     // 处理获取句柄失败的情况
    //     ML_LOG(Main, Error, "Failed to get memory fd");
    // }

    // auto SharedTexture = VulkanDynamicRHI->GetTextureRef(fd, memoryRequirements.size,
    //                                                 0);
    // if (SharedTexture) {
    //     Encoder->Transmit(SharedTexture, FBufferInfo());
    // }

    int fd = received_fd;
    uint64_t Size = 4194304;
    uint64_t Offset = 0;
    // auto SharedTexture = VulkanDynamicRHI->GetTextureRef(fd, Size,
    //                                                 Offset);
    // if (SharedTexture) {
    //     Encoder->Transmit(SharedTexture, FBufferInfo());
    // }

    VkImage image;
    VkExternalMemoryImageCreateInfo ExternalMemoryImageCreateInfo;
    ZeroVulkanStruct(ExternalMemoryImageCreateInfo,
                     VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO);
    ExternalMemoryImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    VkImageCreateInfo imageCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                         .imageType = VK_IMAGE_TYPE_2D,
                                         .format = VK_FORMAT_R8G8B8A8_UNORM,
                                         .extent = {IMAGE_WIDTH, IMAGE_HEIGHT, 1},
                                         .mipLevels = 1,
                                         .arrayLayers = 1,
                                         .samples = VK_SAMPLE_COUNT_1_BIT,
                                         .tiling = VK_IMAGE_TILING_OPTIMAL,
                                         .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                         .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                         .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                         .pNext = &ExternalMemoryImageCreateInfo};
    VkResult result = vkCreateImage(device, &imageCreateInfo, NULL, &image);
    if (result != VK_SUCCESS) {
        // 处理创建VkImage对象失败的情况
        ML_LOG(Main, Error, "Failed to create vulkan image");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);

    uint32 memoryTypeIndex = VulkanDynamicRHI->FindMemoryTypeIndex(
        memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryDedicatedAllocateInfo dedicatedMemoryInfo;
    ZeroVulkanStruct(dedicatedMemoryInfo, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO);
    dedicatedMemoryInfo.image = image;
    VkImportMemoryFdInfoKHR importMemoryInfo{};
    importMemoryInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
    importMemoryInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    importMemoryInfo.fd = fd;
    importMemoryInfo.pNext = &dedicatedMemoryInfo;
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = &importMemoryInfo;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;
    VkDeviceMemory textureMemory;
    result = vkAllocateMemory(device, &allocateInfo, nullptr, &textureMemory);
    if (result != VK_SUCCESS) {
        ML_LOG(Main, Error, "Failed to allocate memory vulkan texture, %d", result);
    } else {
        ML_LOG(Main, Log, "Success to allocate memory vulkan texture, %d", result);
    }
    // continue;

    // // for (int i = 0; i < 5; ++i)
    // {
    //     // import

    //     VkImportMemoryFdInfoKHR importMemoryInfo{};
    //     importMemoryInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
    //     importMemoryInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    //     importMemoryInfo.fd = fd;
    //     importMemoryInfo.pNext = nullptr;
    //     VkMemoryAllocateInfo allocateInfo{};
    //     allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    //     allocateInfo.pNext = &importMemoryInfo;
    //     allocateInfo.allocationSize = memoryRequirements.size;
    //     allocateInfo.memoryTypeIndex = memoryTypeIndex;
    //     VkDeviceMemory textureMemory;
    //     result = vkAllocateMemory(device, &allocateInfo, nullptr, &textureMemory);
    //     if (result != VK_SUCCESS) {
    //         ML_LOG(Main, Error, "Failed to allocate memory vulkan texture, %d", result);
    //     } else {
    //         VkImageCreateInfo imageCreateInfo{};
    //         VkImage textureImage;
    //         result = vkCreateImage(device, &imageCreateInfo, nullptr, &textureImage);
    //         if (result != VK_SUCCESS) {
    //             ML_LOG(Main, Error, "Failed to create image vulkan texture, %d", result);
    //             vkFreeMemory(device, textureMemory, nullptr);
    //         } else {
    //             vkBindImageMemory(device, textureImage, textureMemory, 0);
    //             // TODO: present image

    //             vkDestroyImage(device, textureImage, nullptr);
    //             vkFreeMemory(device, textureMemory, nullptr);
    //             // close(fd);
    //             ML_LOG(Main, Log, "Success create image");
    //         }
    //     }
    // }

    // std::this_thread::sleep_for(std::chrono::seconds(6000));

    // vkDestroyImage(device, image, nullptr);
    // vkFreeMemory(device, deviceMemory, nullptr);
    // close(fd);

    return 0;
}
