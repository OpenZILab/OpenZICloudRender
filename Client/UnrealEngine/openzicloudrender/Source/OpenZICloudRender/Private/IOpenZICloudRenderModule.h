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
#include "IInputDevice.h"
#include "IInputDeviceModule.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

/**
 * The public interface to this module
 */
class IOpenZICloudRenderModule : public IInputDeviceModule {
public:
    /**
     * Singleton-like access to this module's interface.
     * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
     *
     * @return Returns singleton instance, loading the module on demand if needed
     */
    static inline IOpenZICloudRenderModule& Get() {
        return FModuleManager::LoadModuleChecked<IOpenZICloudRenderModule>("OpenZICloudRender");
    }

    /**
     * Checks to see if this module is loaded.
     *
     * @return True if the module is loaded.
     */
    static inline bool IsAvailable() {
        return FModuleManager::Get().IsModuleLoaded("OpenZICloudRender");
    }

    /*
     * Event fired when internal streamer is initialized and the methods on this module are ready for use.
     */
    DECLARE_EVENT_OneParam(IOpenZICloudRenderModule, FReadyEvent, IOpenZICloudRenderModule&)

        /*
         * A getter for the OnReady event. Intent is for users to call IOpenZICloudRenderModule::Get().OnReady().AddXXX.
         * @return The bindable OnReady event.
         */
        virtual FReadyEvent& OnReady() = 0;

    /*
     * Is the OpenZICloudRender module actually ready to use? Is the streamer created.
     * @return True if Pixel Streaming module methods are ready for use.
     */
    virtual bool IsReady() = 0;

    /**
     * Returns a reference to the input device. The lifetime of this reference
     * is that of the underlying shared pointer.
     * @return A reference to the input device.
     */
    virtual IInputDevice& GetInputDevice() = 0;
};