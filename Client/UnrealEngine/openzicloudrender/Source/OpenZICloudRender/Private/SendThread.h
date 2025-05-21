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
#include "GameFramework/GameUserSettings.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/Guid.h"
#include "Runtime/Launch/Resources/Version.h"

#include "IPCTypes.h"

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <Windows.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#if (ENGINE_MAJOR_VERSION <= 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
#include "D3D11RHIBasePrivate.h"
#include "D3D11RHIPrivate.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5
#include "D3D12RHIPrivate.h"
#endif

#elif PLATFORM_LINUX

#include "FdSocket.h"
#include "VulkanRHIPrivate.h"

#endif

namespace OpenZI::CloudRender {
    class ITextureSource;
    class ZFdSocketServer;

    class FSendThread : public FRunnable {
    public:
        FSendThread();
        FSendThread(FGuid Guid, uint32 SizeX, uint32 SizeY, uint32 InMaxCache = 300);
        ~FSendThread();
        void StartThread();
        void StopThread();
        void SendTextureHandleToShareMemory(FTexture2DRHIRef FrameBuffer);
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
        FString ShareHandlePrefix;
        TSharedPtr<ITextureSource> TextureSource;
#if PLATFORM_WINDOWS
        TSharedPtr<FShareMemoryWriter> Writer;
        HANDLE SharedHandleCache = nullptr;
#if (ENGINE_MAJOR_VERSION <= 4 || (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION <= 0))
        // TArray<TRefCountPtr<ID3D11Texture2D>> Cache;
#endif
#if ENGINE_MAJOR_VERSION >= 5
        // TArray<TRefCountPtr<ID3D12Resource>> D3D12ResourceCaches;
        TArray<HANDLE> D3D12SharedHandleCaches;
#endif

#elif PLATFORM_LINUX
        TArray<int> VulkanFdCaches;
        ZFdSocketServer* FdSocketServer;
#endif
        FString RHIName;
        FTexture2DRHIRef LastTexture2DRHIRef;
        uint32 FrameBufferSizeX = 2560;
        uint32 FrameBufferSizeY = 1440;
        int32 MaxCache;
        FGuid ShmId;
        int32 CVarExplicitAdapterValue = -1;
    };
} // namespace OpenZI::CloudRender