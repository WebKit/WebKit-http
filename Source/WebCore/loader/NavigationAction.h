/*
 * Copyright (C) 2006-2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "AdClickAttribution.h"
#include "BackForwardItemIdentifier.h"
#include "FrameLoaderTypes.h"
#include "GlobalFrameIdentifier.h"
#include "LayoutPoint.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"
#include "UserGestureIndicator.h"
#include <wtf/Forward.h>
#include <wtf/Optional.h>

namespace WebCore {

class Document;
class Event;
class HistoryItem;
class MouseEvent;
class UIEventWithKeyState;

// NavigationAction should never hold a strong reference to the originating document either directly
// or indirectly as doing so prevents its destruction even after navigating away from it because
// DocumentLoader keeps around the NavigationAction for the last navigation.
class NavigationAction {
public:
    NavigationAction();
    WEBCORE_EXPORT NavigationAction(Document&, const ResourceRequest&, InitiatedByMainFrame, NavigationType = NavigationType::Other, ShouldOpenExternalURLsPolicy = ShouldOpenExternalURLsPolicy::ShouldNotAllow, Event* = nullptr, const AtomString& downloadAttribute = nullAtom());
    NavigationAction(Document&, const ResourceRequest&, InitiatedByMainFrame, FrameLoadType, bool isFormSubmission, Event* = nullptr, ShouldOpenExternalURLsPolicy = ShouldOpenExternalURLsPolicy::ShouldNotAllow, const AtomString& downloadAttribute = nullAtom());

    WEBCORE_EXPORT ~NavigationAction();

    WEBCORE_EXPORT NavigationAction(const NavigationAction&);
    NavigationAction& operator=(const NavigationAction&);

    NavigationAction(NavigationAction&&);
    NavigationAction& operator=(NavigationAction&&);

    class Requester {
    public:
        Requester(const Document&);

        const URL& url() const { return m_url; }
        const SecurityOrigin& securityOrigin() const { return *m_origin; }
        const GlobalFrameIdentifier& globalFrameIdentifier() const { return m_globalFrameIdentifier; }
    private:
        URL m_url;
        RefPtr<SecurityOrigin> m_origin;
        GlobalFrameIdentifier m_globalFrameIdentifier;
    };
    const Optional<Requester>& requester() const { return m_requester; }

    struct UIEventWithKeyStateData {
        UIEventWithKeyStateData(const UIEventWithKeyState&);

        bool isTrusted;
        bool shiftKey;
        bool ctrlKey;
        bool altKey;
        bool metaKey;
    };
    struct MouseEventData : UIEventWithKeyStateData {
        MouseEventData(const MouseEvent&);

        LayoutPoint absoluteLocation;
        FloatPoint locationInRootViewCoordinates;
        short button;
        unsigned short syntheticClickType;
        bool buttonDown;
    };
    const Optional<UIEventWithKeyStateData>& keyStateEventData() const { return m_keyStateEventData; }
    const Optional<MouseEventData>& mouseEventData() const { return m_mouseEventData; }

    NavigationAction copyWithShouldOpenExternalURLsPolicy(ShouldOpenExternalURLsPolicy) const;

    bool isEmpty() const { return !m_requester || m_requester->url().isEmpty() || m_resourceRequest.url().isEmpty(); }

    URL url() const { return m_resourceRequest.url(); }
    const ResourceRequest& resourceRequest() const { return m_resourceRequest; }

    NavigationType type() const { return m_type; }

    bool processingUserGesture() const { return m_userGestureToken ? m_userGestureToken->processingUserGesture() : false; }
    RefPtr<UserGestureToken> userGestureToken() const { return m_userGestureToken; }

    ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicy() const { return m_shouldOpenExternalURLsPolicy; }
    void setShouldOpenExternalURLsPolicy(ShouldOpenExternalURLsPolicy policy) {  m_shouldOpenExternalURLsPolicy = policy; }
    InitiatedByMainFrame initiatedByMainFrame() const { return m_initiatedByMainFrame; }

    const AtomString& downloadAttribute() const { return m_downloadAttribute; }

    bool treatAsSameOriginNavigation() const { return m_treatAsSameOriginNavigation; }

    bool hasOpenedFrames() const { return m_hasOpenedFrames; }
    void setHasOpenedFrames(bool value) { m_hasOpenedFrames = value; }

    bool openedByDOMWithOpener() const { return m_openedByDOMWithOpener; }
    void setOpenedByDOMWithOpener() { m_openedByDOMWithOpener = true; }

    void setTargetBackForwardItem(HistoryItem&);
    const Optional<BackForwardItemIdentifier>& targetBackForwardItemIdentifier() const { return m_targetBackForwardItemIdentifier; }

    void setSourceBackForwardItem(HistoryItem*);
    const Optional<BackForwardItemIdentifier>& sourceBackForwardItemIdentifier() const { return m_sourceBackForwardItemIdentifier; }

    LockHistory lockHistory() const { return m_lockHistory; }
    void setLockHistory(LockHistory lockHistory) { m_lockHistory = lockHistory; }

    LockBackForwardList lockBackForwardList() const { return m_lockBackForwardList; }
    void setLockBackForwardList(LockBackForwardList lockBackForwardList) { m_lockBackForwardList = lockBackForwardList; }

    const Optional<AdClickAttribution>& adClickAttribution() const { return m_adClickAttribution; };
    void setAdClickAttribution(AdClickAttribution&& adClickAttribution) { m_adClickAttribution = adClickAttribution; };

private:
    // Do not add a strong reference to the originating document or a subobject that holds the
    // originating document. See comment above the class for more details.
    Optional<Requester> m_requester;
    ResourceRequest m_resourceRequest;
    NavigationType m_type;
    ShouldOpenExternalURLsPolicy m_shouldOpenExternalURLsPolicy;
    InitiatedByMainFrame m_initiatedByMainFrame;
    Optional<UIEventWithKeyStateData> m_keyStateEventData;
    Optional<MouseEventData> m_mouseEventData;
    RefPtr<UserGestureToken> m_userGestureToken { UserGestureIndicator::currentUserGesture() };
    AtomString m_downloadAttribute;
    bool m_treatAsSameOriginNavigation;
    bool m_hasOpenedFrames { false };
    bool m_openedByDOMWithOpener { false };
    Optional<BackForwardItemIdentifier> m_targetBackForwardItemIdentifier;
    Optional<BackForwardItemIdentifier> m_sourceBackForwardItemIdentifier;
    LockHistory m_lockHistory { LockHistory::No };
    LockBackForwardList m_lockBackForwardList { LockBackForwardList::No };
    Optional<AdClickAttribution> m_adClickAttribution;
};

} // namespace WebCore
