// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/09/01 11:13
// 

#pragma once

#include "WebRTCIncludes.h"

#include <string>
#include <memory>
#include <functional>

namespace OpenZI::CloudRender
{
    // FSetSessionDescriptionObserver
    // WebRTC requires an implementation of `webrtc::SetSessionDescriptionObserver` interface as a callback
    // for setting session description, either on receiving remote `offer` (`PeerConnection::SetRemoteDescription`)
    // of on sending `answer` (`PeerConnection::SetLocalDescription`)
    class FSetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver
    {
    public:
        using FSuccessCallback = std::function<void()>;
        using FFailureCallback = std::function<void(const std::string &)>;

        static FSetSessionDescriptionObserver *Create(FSuccessCallback &&successCallback, FFailureCallback &&failureCallback)
        {
            return new rtc::RefCountedObject<FSetSessionDescriptionObserver>(std::move(successCallback), std::move(failureCallback));
        }

        FSetSessionDescriptionObserver(FSuccessCallback &&successCallback, FFailureCallback &&failureCallback)
            : SuccessCallback(std::move(successCallback)), FailureCallback(std::move(failureCallback))
        {
        }

        // we don't need to do anything on success
        void OnSuccess() override
        {
            SuccessCallback();
        }

        // errors usually mean incompatibility between our session configuration (often H.264, its profile and level) and
        // player, malformed SDP or if player doesn't support UnifiedPlan (whatever was used by proxy)
        void OnFailure(webrtc::RTCError Error) override
        {
            FailureCallback(Error.message());
        }

    private:
        FSuccessCallback SuccessCallback;
        FFailureCallback FailureCallback;
    };

    class FCreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver
    {
    public:
        using FSuccessCallback = std::function<void(webrtc::SessionDescriptionInterface *)>;
        using FFailureCallback = std::function<void(const std::string &)>;

        static FCreateSessionDescriptionObserver *Create(FSuccessCallback &&successCallback, FFailureCallback &&failureCallback)
        {
            return new rtc::RefCountedObject<FCreateSessionDescriptionObserver>(std::move(successCallback), std::move(failureCallback));
        }

        FCreateSessionDescriptionObserver(FSuccessCallback &&successCallback, FFailureCallback &&failureCallback)
            : SuccessCallback(std::move(successCallback)), FailureCallback(std::move(failureCallback))
        {
        }

        void OnSuccess(webrtc::SessionDescriptionInterface *SDP) override
        {
            SuccessCallback(SDP);
        }

        void OnFailure(webrtc::RTCError Error) override
        {
            FailureCallback(Error.message());
        }

    private:
        FSuccessCallback SuccessCallback;
        FFailureCallback FailureCallback;
    };
} // namespace OpenZI::CloudRender