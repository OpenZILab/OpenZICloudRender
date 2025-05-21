//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/15 22:51
//
#pragma once
#include "Containers/FormatString.h"
#include "Containers/Queue.h"
#include "Thread/Thread.h"
#include <map>
#include <string>
#include <iostream>
#if PLATFORM_WINDOWS
#include <Windows.h>
#elif PLATFORM_LINUX
#include <cstring>
#include <cwchar>
#endif

#if PLATFORM_WINDOWS
#pragma warning(push)
#pragma warning(disable : 4129)
#endif

namespace OpenZI
{

    class FCommandLogger : public FThread
    {
    public:
        static FCommandLogger &Get();
        void AddLog(std::string Log);
        void AddLog(std::wstring_view LogW);
        void Run() override;

    private:
        bool bExiting;
        ~FCommandLogger();
        FCommandLogger();
        FCommandLogger(const FCommandLogger &) = delete;
        FCommandLogger(FCommandLogger &&) = delete;
        FCommandLogger &operator=(const FCommandLogger &) = delete;
        FCommandLogger &operator=(FCommandLogger &&) = delete;

        TQueue<std::string> Logs;
    };

#if PLATFORM_WINDOWS
    std::string WideToMulti(std::wstring_view sourceStr, UINT pagecode);
#elif PLATFORM_LINUX
    std::string WideToMulti(std::wstring_view sourceStr);
#endif
    std::string WideToUtf8(std::wstring_view sourceWStr);
    std::string WideToOme(std::wstring_view sourceWStr);
    std::ostream &operator<<(std::ostream &os, std::wstring_view str);

#define CommandLogColorLog "\033[0;37m"
#define CommandLogColorWarning "\033[0;33m"
#define CommandLogColorError "\033[0;31m"
#define CommandLogColorSuccess "\033[0;32m"
#define CommandLogColorNone "\033[0m"
#define CommandLogColorBold "\033[1m"
#define CommandLogColorBlink "\033[5m"
#define CommandLogColorVerbose "\033[0;36m"

#ifdef NDEBUG
#define ML_LOG(Module, Type, ...) \
    FCommandLogger::Get().AddLog(Printf("[%s][%s]: %s", #Module, #Type, Printf(__VA_ARGS__).c_str()));

#define ML_LOGW(Module, Type, ...) \
    FCommandLogger::Get().AddLog(PrintfW(L"[%S][%S]: %s", #Module, #Type, PrintfW(__VA_ARGS__).c_str()));
#else
#define ML_LOG(Module, Type, ...) \
    FCommandLogger::Get().AddLog(Printf("[%s][%s]: %s", #Module, #Type, Printf(__VA_ARGS__).c_str()));

#define ML_LOGW(Module, Type, ...) \
    FCommandLogger::Get().AddLog(PrintfW(L"[%S][%S]: %s", #Module, #Type, PrintfW(__VA_ARGS__).c_str()));
// #define ML_LOG(Module, Type, ...) \
//     FCommandLogger::Get().AddLog(Printf("%s[%s][%s#%d][%s]: %s%s", CommandLogColor##Type, #Module, __FILE__, __LINE__, #Type, Printf(__VA_ARGS__).c_str(), CommandLogColorNone));

// #define ML_LOGW(Module, Type, ...) \
//     FCommandLogger::Get().AddLog(PrintfW(L"%S[%S][%S#%d][%S]: %s%S", CommandLogColor##Type, #Module, __FILE__, __LINE__, #Type, PrintfW(__VA_ARGS__).c_str(), CommandLogColorNone));
#endif
} // namespace OpenZI
#pragma warning(pop)