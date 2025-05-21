// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/20 14:51
//
#include "DynamicRHI.h"
#include "Config.h"

namespace OpenZI::CloudRender
{
    FDynamicRHI::FDynamicRHI()
        : Name(FAppConfig::Get().RHIName)
    {
    }
    std::string &FDynamicRHI::GetName()
    {
        return Name;
    }

    std::shared_ptr<FDynamicRHI> GDynamicRHI = nullptr;
} // namespace OpenZI::CloudRender