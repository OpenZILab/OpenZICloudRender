//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/29 14:31
//
#pragma once
#include "CoreMinimal.h"
#include <map>
#include <string>
#include <vector>
#include <functional>

namespace OpenZI
{
    class FMessageWrapperBase
    {
    public:
        virtual ~FMessageWrapperBase() {}
    };

    template <typename... Args>
    class TMessageWrapper : public FMessageWrapperBase
    {
    public:
        TMessageWrapper(std::function<void(Args...)> &Callback)
            : Function(std::move(Callback))
        {
        }
        TMessageWrapper(const TMessageWrapper &Other)
        {
            Function = Other.Function;
        }
        void Call(Args &...InArgs)
        {
            Function(InArgs...);
        }

    private:
        std::function<void(Args...)> Function;
        TMessageWrapper() = delete;
    };

    class FMessageCenter
    {
    public:
        uint64 Subscribe(std::string &&Key, FMessageWrapperBase *Wrapper)
        {
            if (Container.find(Key) != Container.end())
            {
                auto &Callbacks = Container[Key];
                Callbacks.push_back(Wrapper);
            }
            else
            {
                std::vector<FMessageWrapperBase *> Callbacks;
                Callbacks.push_back(Wrapper);
                Container[Key] = Callbacks;
            }
            uint64 MessageId = ++MessageIdMaker;
            Wrappers[MessageId] = Wrapper;
            return MessageId;
        }
        void Unsubscribe(const std::string &Key, FMessageWrapperBase *Wrapper)
        {
            if (Container.find(Key) == Container.end())
                return;
            auto &Callbacks = Container[Key];
            for (int i = 0; i < Callbacks.size(); ++i)
            {
                if (Callbacks[i] == Wrapper)
                {
                    Callbacks.erase(Callbacks.begin() + i);
                    break;
                }
            }
        }
        void Unsubscribe(const std::string &Key, const uint64 MessageId)
        {
            if (Container.find(Key) == Container.end() || Wrappers.find(MessageId) == Wrappers.end())
                return;
            auto &Callbacks = Container[Key];
            for (int i = 0; i < Callbacks.size(); ++i)
            {
                if (Callbacks[i] == Wrappers[MessageId])
                {
                    Callbacks.erase(Callbacks.begin() + i);
                    // TODO: 内存泄漏，因为宏定义中是new出来的指针，没有将其delete。后续改为std::shared_ptr
                    delete Wrappers[MessageId];
                    Wrappers.erase(MessageId);
                    break;
                }
            }
        }
        // template <typename... Args>
        // static void Publish(std::string &&Key, Args &...InArgs)
        // {
        //     if (Container.find(Key) != Container.end())
        //     {
        //         auto Callbacks = Container[Key];
        //         for (auto Callback : Callbacks)
        //         {
        //             static_cast<TMessageWrapper<Args...> *>(Callback)->Call(InArgs...);
        //         }
        //     }
        // }

        template <typename... Args>
        void Publish(std::string &&Key, Args ...InArgs)
        {
            if (Container.find(Key) != Container.end())
            {
                auto Callbacks = Container[Key];
                for (auto Callback : Callbacks)
                {
                    static_cast<TMessageWrapper<Args...> *>(Callback)->Call(InArgs...);
                }
            }
        }
        static FMessageCenter &Get();

    private:
        FMessageCenter() {}
        std::map<std::string, std::vector<FMessageWrapperBase *>> Container;
        std::map<uint64, FMessageWrapperBase*> Wrappers;
        uint64 MessageIdMaker = 0;
    };

#define SUBSCRIBE_MESSAGE(Key, ObjectPtr, FunctionPtr)        \
    auto Callback##Key = std::function([ObjectPtr]() { std::function<void(decltype(ObjectPtr))> Function(FunctionPtr); Function(ObjectPtr); });        \
    auto MessageWrapper##Key = new TMessageWrapper(Callback##Key); \
    MessageId##Key = FMessageCenter::Get().Subscribe(#Key, MessageWrapper##Key);

#define SUBSCRIBE_MESSAGE_OneParam(Key, ObjectPtr, FunctionPtr, VarType1, VarName1) \
    auto Callback##Key = std::function([ObjectPtr](VarType1 VarName1) { std::function<void(decltype(ObjectPtr), VarType1)> Function(FunctionPtr); Function(ObjectPtr, VarName1); });             \
    auto MessageWrapper##Key = new TMessageWrapper(Callback##Key);                       \
    MessageId##Key = FMessageCenter::Get().Subscribe(#Key, MessageWrapper##Key);

#define SUBSCRIBE_MESSAGE_TwoParams(Key, ObjectPtr, FunctionPtr, VarType1, VarName1, VarType2, VarName2) \
    auto Callback##Key = std::function([ObjectPtr](VarType1 VarName1, VarType2 VarName2) { std::function<void(decltype(ObjectPtr), VarType1, VarType2)> Function(FunctionPtr); Function(ObjectPtr, VarName1, VarName2); });          \
    auto MessageWrapper##Key = new TMessageWrapper(Callback##Key);                                       \
    MessageId##Key = FMessageCenter::Get().Subscribe(#Key, MessageWrapper##Key);

#define SUBSCRIBE_MESSAGE_ThreeParams(Key, ObjectPtr, FunctionPtr, VarType1, VarName1, VarType2, VarName2, VarType3, VarName3) \
    auto Callback##Key = std::function([ObjectPtr](VarType1 VarName1, VarType2 VarName2, VarType3 VarName3) { std::function<void(decltype(ObjectPtr), VarType1, VarType2, VarType3)> Function(FunctionPtr); Function(ObjectPtr, VarName1, VarName2, VarName3); });             \
    auto MessageWrapper##Key = new TMessageWrapper(Callback##Key);                                                             \
    MessageId##Key = FMessageCenter::Get().Subscribe(#Key, MessageWrapper##Key);

#define SUBSCRIBE_MESSAGE_FourParams(Key, ObjectPtr, FunctionPtr, VarType1, VarName1, VarType2, VarName2, VarType3, VarName3, VarType4, VarName4) \
    auto Callback##Key = std::function([ObjectPtr](VarType1 VarName1, VarType2 VarName2, VarType3 VarName3, VarType4 VarName4) { std::function<void(decltype(ObjectPtr), VarType1, VarType2, VarType3, VarType4)> Function(FunctionPtr); Function(ObjectPtr, VarName1, VarName2, VarName3, VarName4); });             \
    auto MessageWrapper##Key = new TMessageWrapper(Callback##Key);                                                                                \
    MessageId##Key = FMessageCenter::Get().Subscribe(#Key, MessageWrapper##Key);

#define SUBSCRIBE_MESSAGE_FiveParams(Key, ObjectPtr, FunctionPtr, VarType1, VarName1, VarType2, VarName2, VarType3, VarName3, VarType4, VarName4, VarType5, VarName5) \
    auto Callback##Key = std::function([ObjectPtr](VarType1 VarName1, VarType2 VarName2, VarType3 VarName3, VarType4 VarName4, VarType5 VarName5) { std::function<void(decltype(ObjectPtr), VarType1, VarType2, VarType3, VarType4, VarType5)> Function(FunctionPtr); Function(ObjectPtr, VarName1, VarName2, VarName3, VarName4, VarName5); });              \
    auto MessageWrapper##Key = new TMessageWrapper(Callback##Key);                                                                                                    \
    MessageId##Key = FMessageCenter::Get().Subscribe(#Key, MessageWrapper##Key);

#define PUBLISH_MESSAGE(Key, ...) \
    FMessageCenter::Get().Publish(#Key, __VA_ARGS__);

#define PUBLISH_EVENT(Key) \
    FMessageCenter::Get().Publish(#Key);

#define DECLARE_MESSAGE(Key) uint64 MessageId##Key;

#define UNSUBSCRIBE_MESSAGE(Key) FMessageCenter::Get().Unsubscribe(#Key, MessageId##Key);
} // namespace OpenZI