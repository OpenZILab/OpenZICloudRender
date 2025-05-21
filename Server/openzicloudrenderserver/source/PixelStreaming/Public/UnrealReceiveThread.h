// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/16 16:43
// 
#pragma once
#include "Thread/Thread.h"
#include "Containers/Queue.h"
#include "IPCTypes.h"
#include "MessageCenter/MessageCenter.h"
#include <memory>
#include <atomic>
#include <string>

namespace OpenZI::CloudRender
{
    // class FShareMemoryReader;

    class FUnrealVideoReceiver : public FThread
    {
    public:
        FUnrealVideoReceiver();
        ~FUnrealVideoReceiver();

        void Run() override;
    private:
        bool bExiting;
        std::unique_ptr<FShareMemoryReader> Reader;
        std::string ShareMemoryName;
    };

    class FUnrealAudioReceiver : public FThread
    {
    public:
        FUnrealAudioReceiver();
        ~FUnrealAudioReceiver();

        void Run() override;

    private:
        bool bExiting;
        std::unique_ptr<FShareMemoryReader> Reader;
        std::string ShareMemoryName;
    };
} // namespace OpenZI::CloudRender