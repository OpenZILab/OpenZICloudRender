//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/26 11:47
//
#include "Thread.h"

namespace OpenZI
{
    FThread::FThread()
        : Thread(nullptr)
    {
    }

    FThread::~FThread()
    {
        Join();
    }

    void FThread::Start()
    {
        if (Init())
        {
            Thread = new std::thread(&FThread::Run, this);
        }
    }

    void FThread::Join()
    {
        if (Thread)
        {
            Thread->join();
            delete Thread;
            Thread = nullptr;
        }
    }
} // namespace OpenZI