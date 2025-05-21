// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/20 14:46
// 

#pragma once
#include <string>
#include <memory>

namespace OpenZI::CloudRender
{
    class FDynamicRHI
    {
    public:
        FDynamicRHI();
        std::string& GetName();
    protected:
        std::string Name;
    };

    extern std::shared_ptr<FDynamicRHI> GDynamicRHI;
} // namespace OpenZI::CloudRender