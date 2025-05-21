//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/26 11:45
//
#pragma once

#include <thread>

namespace OpenZI
{
#define THREAD_PRIORITY_MOST_URGENT 15
    class FThread
    {
    public:
        FThread();
        virtual ~FThread();
        virtual bool Init() { return true; }
        virtual void Run() = 0;
        void Start();
        void Join();

    private:
        std::thread *Thread;
    };
} // namespace OpenZI
