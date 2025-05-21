// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/14 10:37
// 

#pragma once
#include "CoreMinimal.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>

namespace OpenZI
{
    static inline int32 GetVarArgs(ANSICHAR *Dest, size_t DestSize, const ANSICHAR *&Fmt, va_list ArgPtr)
    {
        int32 Result = vsnprintf(Dest, DestSize, Fmt, ArgPtr);
        va_end(ArgPtr);
        return (Result != -1 && Result < (int32)DestSize) ? Result : -1;
    }

    static inline int32 GetVarArgs(WIDECHAR *Dest, size_t DestSize, const WIDECHAR *&Fmt, va_list ArgPtr)
    {
        int32 Result = vswprintf(Dest, DestSize, Fmt, ArgPtr);
        va_end(ArgPtr);
        return Result;
    }

#define GET_VARARGS_RESULT(msg, msgsize, len, lastarg, fmt, result) \
    {                                                               \
        va_list ap;                                                 \
        va_start(ap, lastarg);                                      \
        result = GetVarArgs(msg, msgsize, fmt, ap);                 \
        if (result >= msgsize)                                      \
        {                                                           \
            result = -1;                                            \
        }                                                           \
        va_end(ap);                                                 \
    }



#define STARTING_BUFFER_SIZE 512

    static std::string PrintfImpl(const char *Fmt, ...)
    {

        int32 BufferSize = STARTING_BUFFER_SIZE;
        char StartingBuffer[STARTING_BUFFER_SIZE];
        char *Buffer = StartingBuffer;
        int32 Result = -1;

        // First try to print to a stack allocated location
        GET_VARARGS_RESULT(Buffer, BufferSize, BufferSize - 1, Fmt, Fmt, Result);

        // If that fails, start allocating regular memory
        if (Result == -1)
        {
            Buffer = nullptr;
            while (Result == -1)
            {
                BufferSize *= 2;
                Buffer = (char *)realloc(Buffer, BufferSize * sizeof(char));
                GET_VARARGS_RESULT(Buffer, BufferSize, BufferSize - 1, Fmt, Fmt, Result);
            };
        }

        Buffer[Result] = '\0';

        std::string ResultString(Buffer);

        if (BufferSize != STARTING_BUFFER_SIZE)
        {
            free(Buffer);
        }

        return ResultString;
    }

    static std::wstring PrintfImpl(const TCHAR *Fmt, ...)
    {

        int32 BufferSize = STARTING_BUFFER_SIZE;
        TCHAR StartingBuffer[STARTING_BUFFER_SIZE];
        TCHAR *Buffer = StartingBuffer;
        int32 Result = -1;

        // First try to print to a stack allocated location
        GET_VARARGS_RESULT(Buffer, BufferSize, BufferSize - 1, Fmt, Fmt, Result);

        // If that fails, start allocating regular memory
        if (Result == -1)
        {
            Buffer = nullptr;
            while (Result == -1)
            {
                BufferSize *= 2;
                Buffer = (TCHAR *)realloc(Buffer, BufferSize * sizeof(TCHAR));
                GET_VARARGS_RESULT(Buffer, BufferSize, BufferSize - 1, Fmt, Fmt, Result);
            };
        }

        Buffer[Result] = L'\0';

        std::wstring ResultString(Buffer);

        if (BufferSize != STARTING_BUFFER_SIZE)
        {
            free(Buffer);
        }

        return ResultString;
    }

    template <typename FmtType, typename... Types>
    [[nodiscard]] static std::string Printf(const FmtType &Fmt, Types... Args)
    {
        return std::move(PrintfImpl(Fmt, Args...));
    }

    template <typename FmtType, typename... Types>
    [[nodiscard]] static std::wstring PrintfW(const FmtType &Fmt, Types... Args)
    {
        return std::move(PrintfImpl(Fmt, Args...));
    }
} // namespace OpenZI