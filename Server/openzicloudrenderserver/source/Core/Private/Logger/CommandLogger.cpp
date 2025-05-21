// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/15 22:48
// 
#include "CommandLogger.h"

namespace OpenZI
{
#if PLATFORM_WINDOWS
    std::string WideToMulti(std::wstring_view sourceStr, UINT pagecode)
    {
        auto newLen = WideCharToMultiByte(pagecode, 0, sourceStr.data(), static_cast<int>(sourceStr.size()),
                                          nullptr, 0, nullptr, nullptr);

        std::string targetStr;
        targetStr.resize(newLen);
        WideCharToMultiByte(pagecode, 0, sourceStr.data(), static_cast<int>(sourceStr.size()),
                            &targetStr[0], static_cast<int>(targetStr.size()), nullptr, nullptr);
        return targetStr;
    }

    std::string WideToUtf8(std::wstring_view sourceWStr)
    {
        return WideToMulti(sourceWStr, 65001);
    }
    std::string WideToOme(std::wstring_view sourceWStr)
    {
        return WideToMulti(sourceWStr, CP_OEMCP);
    }
    std::ostream &operator<<(std::ostream &os, std::wstring_view str)
    {
        return os << WideToOme(str);
    }
#elif PLATFORM_LINUX
    std::string WideToMulti(std::wstring_view sourceStr)
    {
        return std::string(sourceStr.begin(), sourceStr.end());
    }

    std::string WideToUtf8(std::wstring_view sourceWStr)
    {
        return WideToMulti(sourceWStr);
    }
    std::string WideToOme(std::wstring_view sourceWStr)
    {
        return WideToMulti(sourceWStr);
    }
    std::ostream &operator<<(std::ostream &os, std::wstring_view str)
    {
        return os << WideToOme(str);
    }
#endif
    // #define NONE "\e[0m"        //清除颜色，即之后的打印为正常输出，之前的不受影响
    // #define BLACK "\e[0;30m"    //深黑
    // #define L_BLACK "\e[1;30m"  //亮黑，偏灰褐
    // #define RED "\e[0;31m"      //深红，暗红
    // #define L_RED "\e[1;31m"    //鲜红
    // #define GREEN "\e[0;32m"    //深绿，暗绿
    // #define L_GREEN "\e[1;32m"  //鲜绿
    // #define BROWN "\e[0;33m"    //深黄，暗黄
    // #define YELLOW "\e[1;33m"   //鲜黄
    // #define BLUE "\e[0;34m"     //深蓝，暗蓝
    // #define L_BLUE "\e[1;34m"   //亮蓝，偏白灰
    // #define PURPLE "\e[0;35m"   //深粉，暗粉，偏暗紫
    // #define L_PURPLE "\e[1;35m" //亮粉，偏白灰
    // #define CYAN "\e[0;36m"     //暗青色
    // #define L_CYAN "\e[1;36m"   //鲜亮青色
    // #define GRAY "\e[0;37m"     //灰色
    // #define WHITE "\e[1;37m"    //白色，字体粗一点，比正常大，比bold小
    // #define BOLD "\e[1m"        //白色，粗体
    // #define UNDERLINE "\e[4m"   //下划线，白色，正常大小
    // #define BLINK "\e[5m"       //闪烁，白色，正常大小
    // #define REVERSE "\e[7m"     //反转，即字体背景为白色，字体为黑色
    // #define HIDE "\e[8m"        //隐藏
    // #define CLEAR "\e[2J"       //清除
    // #define CLRLINE "\r\e[K"    //清除行

    FCommandLogger &FCommandLogger::Get()
    {
        static FCommandLogger CommandLogger;
        return CommandLogger;
    }

    void FCommandLogger::AddLog(std::string Log)
    {
        Logs.Enqueue(std::move(Log));
    }
    void FCommandLogger::AddLog(std::wstring_view LogW)
    {
#if PLATFORM_WINDOWS
        auto Log = WideToMulti(LogW, CP_OEMCP);
        AddLog(Log);
#elif PLATFORM_LINUX
        auto Log = WideToMulti(LogW);
        AddLog(Log);
#endif
    }

    FCommandLogger::FCommandLogger()
        : bExiting(false)
    {
        Start();
    }

    FCommandLogger::~FCommandLogger()
    {
        bExiting = true;
        Join();
    }

    void FCommandLogger::Run()
    {
        std::string Log;
        while (!bExiting)
        {
            while (Logs.Dequeue(Log))
            {
                std::cout << Log << std::endl;
            }
            // Sleep(33);
        }
    }
} // namespace OpenZI