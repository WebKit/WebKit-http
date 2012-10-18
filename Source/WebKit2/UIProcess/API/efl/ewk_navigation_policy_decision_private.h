/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ewk_navigation_policy_decision_private_h
#define ewk_navigation_policy_decision_private_h

#include "WKAPICast.h"
#include "WKBase.h"
#include "WKEinaSharedString.h"
#include "WKEvent.h"
#include "WKFramePolicyListener.h"
#include "WKPageLoadTypes.h"
#include "WKRetainPtr.h"
#include "ewk_url_request_private.h"
#include <wtf/PassRefPtr.h>

/**
 * \struct  Ewk_Navigation_Policy_Decision
 * @brief   Contains the navigation policy decision data.
 */
class Ewk_Navigation_Policy_Decision : public RefCounted<Ewk_Navigation_Policy_Decision> {
public:
    WKRetainPtr<WKFramePolicyListenerRef> listener;
    bool actedUponByClient;
    Ewk_Navigation_Type navigationType;
    Event_Mouse_Button mouseButton;
    Event_Modifier_Keys modifiers;
    RefPtr<Ewk_Url_Request> request;
    WKEinaSharedString frameName;

    ~Ewk_Navigation_Policy_Decision()
    {
        // This is the default choice for all policy decisions in WebPageProxy.cpp.
        if (!actedUponByClient)
            WKFramePolicyListenerUse(listener.get());
    }

    static PassRefPtr<Ewk_Navigation_Policy_Decision> create(WKFrameNavigationType navigationType, WKEventMouseButton mouseButton, WKEventModifiers modifiers, WKURLRequestRef requestRef, const char* frameName, WKFramePolicyListenerRef listener)
    {
        RefPtr<Ewk_Url_Request> request = Ewk_Url_Request::create(requestRef);
        return adoptRef(new Ewk_Navigation_Policy_Decision(listener, static_cast<Ewk_Navigation_Type>(navigationType), static_cast<Event_Mouse_Button>(mouseButton), static_cast<Event_Modifier_Keys>(modifiers), request.release(), frameName));
    }

private:
    Ewk_Navigation_Policy_Decision(WKFramePolicyListenerRef listener, Ewk_Navigation_Type navigationType, Event_Mouse_Button mouseButton, Event_Modifier_Keys modifiers, PassRefPtr<Ewk_Url_Request> request, const char* frameName)
        : listener(listener)
        , actedUponByClient(false)
        , navigationType(navigationType)
        , mouseButton(mouseButton)
        , modifiers(modifiers)
        , request(request)
        , frameName(frameName)
    { }
};

#endif // ewk_navigation_policy_decision_private_h
