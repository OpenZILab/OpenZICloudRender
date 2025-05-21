// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/07/28 09:22
// 
#include "DataConverter.h"

#if PLATFORM_WINDOWS

#include <Windows.h>

namespace OpenZI::CloudRender
{
    std::wstring FDataConverter::ToWString(const std::string &Src)
    {
        std::wstring Result;
        int Size = static_cast<int>(Src.size());
        int Length = MultiByteToWideChar(CP_ACP, 0, Src.c_str(), Size, NULL, 0);
        WCHAR* Buffer = new WCHAR[Length + 1];

        MultiByteToWideChar(CP_ACP, 0, Src.c_str(), Size, Buffer, Length);
        Buffer[Length] = '\0';
        Result.append(Buffer);
        delete[] Buffer;
        return Result;
    }

    std::string FDataConverter::ToString(const std::wstring &Src)
    {
        std::string Result;
        int Size = static_cast<int>(Src.size());
        int Length = WideCharToMultiByte(CP_ACP, 0, Src.c_str(), Size, NULL, 0, NULL, NULL);
        char* Buffer = new char[Length + 1];
        WideCharToMultiByte(CP_ACP, 0, Src.c_str(), Size, Buffer, Length, NULL, NULL);
        Buffer[Length] = '\0';
        Result.append(Buffer);
        delete[] Buffer;
        return Result;
    }

    std::string FDataConverter::ToUTF8(const std::string& Src) {
        int nwLen = MultiByteToWideChar(CP_ACP, 0, Src.c_str(), -1, NULL, 0);

        wchar_t* pwBuf = new wchar_t[nwLen + 1]; // 一定要加1，不然会出现尾巴
        ZeroMemory(pwBuf, nwLen * 2 + 2);

        MultiByteToWideChar(CP_ACP, 0, Src.c_str(), (int)Src.length(), pwBuf, nwLen);

        int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

        char* pBuf = new char[nLen + 1];
        ZeroMemory(pBuf, nLen + 1);

        WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

        std::string retStr(pBuf);

        delete[] pwBuf;
        delete[] pBuf;

        pwBuf = NULL;
        pBuf = NULL;
        return retStr;
    }
} // namespace OpenZI::CloudRender

#elif PLATFORM_LINUX

#include <locale>
#include <codecvt>

namespace OpenZI::CloudRender
{
    std::wstring FDataConverter::ToWString(const std::string &Src)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(Src);
    }

    std::string FDataConverter::ToString(const std::wstring &Src)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(Src);
    }

    std::string FDataConverter::ToUTF8(const std::string& Src) {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        std::u32string utf32 = converter.from_bytes(Src);
        return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>{}.to_bytes(utf32);
    }
} // namespace OpenZI::CloudRender
#endif