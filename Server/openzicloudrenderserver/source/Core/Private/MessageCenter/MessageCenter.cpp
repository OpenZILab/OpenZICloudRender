// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 15:26
// 

#include "MessageCenter.h"

namespace OpenZI
{
    FMessageCenter &FMessageCenter::Get()
    {
        static FMessageCenter Instance;
        return Instance;
    }
} // namespace OpenZI