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
#include "LaunchCustomServerThread.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace OpenZI::CloudRender {
    FLaunchCustomServerThread::FLaunchCustomServerThread(TSharedPtr<FJsonObject> InConfigRoot,
                                                     FGuid Guid)
        : FLaunchServerThreadBase(InConfigRoot, Guid) {
        SignallingDirName = TEXT("SignallingWebServerCustom");
        SignallingWebServerPath = TEXT("node");
#if PLATFORM_WINDOWS
        TurnWebServerPath = FPaths::Combine(ProgramsPath, SignallingDirName, TEXT("platform_scripts/cmd/coturn/turnserver.exe"));
#elif PLATFORM_LINUX
        TurnWebServerPath = FPaths::Combine(ProgramsPath, TEXT("CloudRenderServer/turnserver"));
#endif
        Thread = FRunnableThread::Create(this, TEXT("LaunchCustomServerThread"), 0, EThreadPriority::TPri_Normal);
        if (!FPaths::FileExists(CloudRenderServerPath)) {
            bWatchCloudRenderServer = false;
        }
    }
} // namespace OpenZI::CloudRender