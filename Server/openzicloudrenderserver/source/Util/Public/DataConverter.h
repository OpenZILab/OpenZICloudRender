// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/28 09:19
// 

#pragma once

#include <string>
namespace OpenZI::CloudRender
{
    class FDataConverter
    {
    public:
        static std::wstring ToWString(const std::string &Src);
        static std::string ToString(const std::wstring &Src);
        static std::string ToUTF8(const std::string& Src);
    };
} // namespace OpenZI::CloudRender