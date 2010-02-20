/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com> All rights reserved.
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com> All rights reserved.
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FrameLoaderClientHaiku_h
#define FrameLoaderClientHaiku_h

#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "KURL.h"
#include "ResourceResponse.h"
#include <Messenger.h>

class WebFrame;
class BWebPage;

namespace WebCore {
class AuthenticationChallenge;
class DocumentLoader;
class Element;
class FormState;
class FrameView;
class HistoryItem;
class NavigationAction;
class ResourceLoader;
class String;

struct LoadErrorResetToken;

class FrameLoaderClientHaiku : public FrameLoaderClient {
public:
    FrameLoaderClientHaiku(BWebPage*, WebFrame*);

    void setDispatchTarget(const BMessenger& messenger);

    virtual void frameLoaderDestroyed();

    virtual bool hasWebView() const;

    virtual void makeRepresentation(DocumentLoader*);
    virtual void forceLayout();
    virtual void forceLayoutForNonHTML();

    virtual void setCopiesOnScroll();

    virtual void detachedFromParent2();
    virtual void detachedFromParent3();

    virtual void download(ResourceHandle*, const ResourceRequest&, const ResourceRequest&, const ResourceResponse&);

    virtual void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&);

    virtual void dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest&, const ResourceResponse&);
    virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier);
    virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
    virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
    virtual void dispatchDidReceiveResponse(DocumentLoader*, unsigned long, const ResourceResponse&);
    virtual void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long, int);
    virtual void dispatchDidFinishLoading(DocumentLoader*, unsigned long);
    virtual void dispatchDidFailLoading(DocumentLoader*, unsigned long, const ResourceError&);

    virtual void dispatchDidHandleOnloadEvents();
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad();
    virtual void dispatchDidCancelClientRedirect();
    virtual void dispatchWillPerformClientRedirect(const KURL&, double interval, double fireDate);
    virtual void dispatchDidChangeLocationWithinPage();
    virtual void dispatchDidPushStateWithinPage();
    virtual void dispatchDidReplaceStateWithinPage();
    virtual void dispatchDidPopStateWithinPage();
    virtual void dispatchWillClose();
    virtual void dispatchDidReceiveIcon();
    virtual void dispatchDidStartProvisionalLoad();
    virtual void dispatchDidReceiveTitle(const String& title);
    virtual void dispatchDidCommitLoad();
    virtual void dispatchDidFailProvisionalLoad(const ResourceError&);
    virtual void dispatchDidFailLoad(const ResourceError&);
    virtual void dispatchDidFinishDocumentLoad();
    virtual void dispatchDidFinishLoad();
    virtual void dispatchDidFirstLayout();
    virtual void dispatchDidFirstVisuallyNonEmptyLayout();

    virtual Frame* dispatchCreatePage();
    virtual void dispatchShow();

    virtual void dispatchDecidePolicyForMIMEType(FramePolicyFunction, const String&, const ResourceRequest&);
    virtual void dispatchDecidePolicyForNewWindowAction(FramePolicyFunction, const NavigationAction&,
                                                        const ResourceRequest&, PassRefPtr<FormState>, const String&);
    virtual void dispatchDecidePolicyForNavigationAction(FramePolicyFunction, const NavigationAction&,
                                                         const ResourceRequest&, PassRefPtr<FormState>);
    virtual void cancelPolicyCheck();

    virtual void dispatchUnableToImplementPolicy(const ResourceError&);

    virtual void dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>);

    virtual void dispatchDidLoadMainResource(DocumentLoader*);
    virtual void revertToProvisionalState(DocumentLoader*);
    virtual void setMainDocumentError(DocumentLoader*, const ResourceError&);
    virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int);
    virtual void dispatchDidLoadResourceByXMLHttpRequest(unsigned long, const ScriptString&);

    virtual void postProgressStartedNotification();
    virtual void postProgressEstimateChangedNotification();
    virtual void postProgressFinishedNotification();

    virtual void setMainFrameDocumentReady(bool);

    virtual void startDownload(const ResourceRequest&);

    virtual void willChangeTitle(DocumentLoader*);
    virtual void didChangeTitle(DocumentLoader*);

    virtual void committedLoad(DocumentLoader*, const char*, int);
    virtual void finishedLoading(DocumentLoader*);
    virtual void updateGlobalHistory();
    virtual void updateGlobalHistoryRedirectLinks();

    virtual bool shouldGoToHistoryItem(HistoryItem*) const;

    virtual void dispatchDidAddBackForwardItem(HistoryItem*) const;
    virtual void dispatchDidRemoveBackForwardItem(HistoryItem*) const;
    virtual void dispatchDidChangeBackForwardIndex() const;
    virtual void saveScrollPositionAndViewStateToItem(HistoryItem*);

    virtual bool canCachePage() const;

    virtual void didDisplayInsecureContent();
    virtual void didRunInsecureContent(WebCore::SecurityOrigin*);

    virtual ResourceError cancelledError(const ResourceRequest&);
    virtual ResourceError blockedError(const ResourceRequest&);
    virtual ResourceError cannotShowURLError(const ResourceRequest&);
    virtual ResourceError interruptForPolicyChangeError(const ResourceRequest&);

    virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&);
    virtual ResourceError fileDoesNotExistError(const ResourceResponse&);
    virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&);

    virtual bool shouldFallBack(const ResourceError&);

    virtual String userAgent(const KURL&);

    virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*);
    virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*);
    virtual void transitionToCommittedForNewPage();

    virtual bool canHandleRequest(const ResourceRequest&) const;
    virtual bool canShowMIMEType(const String& MIMEType) const;
    virtual bool representationExistsForURLScheme(const String& URLScheme) const;
    virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const;

    virtual void frameLoadCompleted();
    virtual void saveViewStateToItem(WebCore::HistoryItem*);
    virtual void restoreViewState();
    virtual void provisionalLoadStarted();
    virtual void didFinishLoad();
    virtual void prepareForDataSourceReplacement();
    virtual PassRefPtr<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&);

    virtual void setTitle(const String& title, const KURL&);

    virtual PassRefPtr<WebCore::Frame> createFrame(const WebCore::KURL& url, const WebCore::String& name, WebCore::HTMLFrameOwnerElement*,
                                                   const WebCore::String& referrer, bool allowsScrolling, int marginWidth, int marginHeight);
    virtual void didTransferChildFrameToNewDocument();
    virtual PassRefPtr<WebCore::Widget> createPlugin(const WebCore::IntSize&, WebCore::HTMLPlugInElement*, const WebCore::KURL&, const Vector<WebCore::String>&,
                                                     const Vector<WebCore::String>&, const WebCore::String&, bool);
    virtual void redirectDataToPlugin(WebCore::Widget* pluginWidget);

    virtual PassRefPtr<WebCore::Widget> createJavaAppletWidget(const WebCore::IntSize&, WebCore::HTMLAppletElement*, const WebCore::KURL& baseURL,
                                                               const Vector<WebCore::String>& paramNames, const Vector<WebCore::String>& paramValues);

    virtual WebCore::ObjectContentType objectContentType(const WebCore::KURL& url, const WebCore::String& mimeType);
    virtual WebCore::String overrideMediaType() const;

    virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld*);
    virtual void documentElementAvailable();
    virtual void didPerformFirstNavigation() const;

    virtual void registerForIconNotification(bool listen);

private:
    void callPolicyFunction(FramePolicyFunction, PolicyAction);
    void triggerNavigationHistoryUpdate() const;
    void postCommitFrameViewSetup(WebFrame*, FrameView*, bool) const;
    status_t dispatchMessage(BMessage& message) const;

private:
    BWebPage* m_webPage;
    WebFrame* m_webFrame;
    BMessenger m_messenger;

    ResourceResponse m_response;
};

} // namespace WebCore

#endif // FrameLoaderClientHaiku_h

