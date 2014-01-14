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
#include "URL.h"
#include "ResourceResponse.h"
#include <Messenger.h>
#include <unicode/uidna.h>
#include <wtf/Forward.h>

class BWebFrame;
class BWebPage;

namespace WebCore {

class AuthenticationChallenge;
class DocumentLoader;
class Element;
class FormState;
class FrameView;
class HistoryItem;
class NavigationAction;
class PluginView;
class ResourceLoader;

struct LoadErrorResetToken;

class FrameLoaderClientHaiku : public FrameLoaderClient {
public:
    FrameLoaderClientHaiku(BWebPage*);
    void setFrame(BWebFrame* frame) {m_webFrame = frame;}
    BWebFrame* webFrame() { return m_webFrame; }

    void setDispatchTarget(const BMessenger& messenger);
    BWebPage* page() const;

    virtual void frameLoaderDestroyed();

    virtual bool hasWebView() const;

    virtual void makeRepresentation(DocumentLoader*);
    virtual void forceLayout();
    virtual void forceLayoutForNonHTML();

    virtual void setCopiesOnScroll();

    virtual void detachedFromParent2();
    virtual void detachedFromParent3();

    virtual void download(ResourceHandle*, const ResourceRequest&, const ResourceResponse&);

    virtual void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&);

    virtual void dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest&,
                                         const ResourceResponse&);
    virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier);
    virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*,
                                                           unsigned long identifier,
                                                           const AuthenticationChallenge&);
    virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*,
                                                          unsigned long identifier,
                                                          const AuthenticationChallenge&);
    virtual void dispatchDidReceiveResponse(DocumentLoader*, unsigned long,
                                            const ResourceResponse&);
    virtual void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long, int);
    virtual void dispatchDidFinishLoading(DocumentLoader*, unsigned long);
    virtual void dispatchDidFailLoading(DocumentLoader*, unsigned long,
                                        const ResourceError&);

    virtual void dispatchDidHandleOnloadEvents();
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad();
    virtual void dispatchDidCancelClientRedirect();
    virtual void dispatchWillPerformClientRedirect(const URL&, double interval, double fireDate);
    virtual void dispatchDidChangeLocationWithinPage();
    virtual void dispatchDidPushStateWithinPage();
    virtual void dispatchDidReplaceStateWithinPage();
    virtual void dispatchDidPopStateWithinPage();
    virtual void dispatchWillClose();
    virtual void dispatchDidReceiveIcon();
    virtual void dispatchDidStartProvisionalLoad();
    virtual void dispatchDidReceiveTitle(const StringWithDirection&);
    virtual void dispatchDidChangeIcons(IconType);
    virtual void dispatchDidCommitLoad();
    virtual void dispatchDidFailProvisionalLoad(const ResourceError&);
    virtual void dispatchDidFailLoad(const ResourceError&);
    virtual void dispatchDidFinishDocumentLoad();
    virtual void dispatchDidFinishLoad();
    virtual void dispatchDidFirstLayout();
    virtual void dispatchDidFirstVisuallyNonEmptyLayout();

    virtual Frame* dispatchCreatePage(const NavigationAction&);
    virtual void dispatchShow();

    virtual void dispatchDecidePolicyForResponse(const ResourceResponse&, const ResourceRequest&, FramePolicyFunction);
    virtual void dispatchDecidePolicyForNewWindowAction(const NavigationAction&,
        const ResourceRequest&, PassRefPtr<FormState>, const String&, FramePolicyFunction);
    virtual void dispatchDecidePolicyForNavigationAction(const NavigationAction&,
                                                         const ResourceRequest&, PassRefPtr<FormState>, FramePolicyFunction);
    virtual void cancelPolicyCheck();

    virtual void dispatchUnableToImplementPolicy(const ResourceError&);

    virtual void dispatchWillSendSubmitEvent(PassRefPtr<FormState>) { }
    virtual void dispatchWillSubmitForm(PassRefPtr<FormState>, FramePolicyFunction);

    virtual void dispatchDidLoadMainResource(DocumentLoader*);
    virtual void revertToProvisionalState(DocumentLoader*);
    virtual void setMainDocumentError(DocumentLoader*, const ResourceError&);
    virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*,
                                                        const ResourceRequest&,
                                                        const ResourceResponse&, int);

    virtual void postProgressStartedNotification();
    virtual void postProgressEstimateChangedNotification();
    virtual void postProgressFinishedNotification();

    virtual void setMainFrameDocumentReady(bool);

    virtual void startDownload(const ResourceRequest&, const String& suggestedName = String());

    virtual void willChangeTitle(DocumentLoader*);
    virtual void didChangeTitle(DocumentLoader*);

    virtual void committedLoad(DocumentLoader*, const char*, int);
    virtual void finishedLoading(DocumentLoader*);
    virtual void updateGlobalHistory();
    virtual void updateGlobalHistoryRedirectLinks();

    virtual bool shouldGoToHistoryItem(HistoryItem*) const;
    virtual bool shouldStopLoadingForHistoryItem(HistoryItem*) const;

    virtual void saveScrollPositionAndViewStateToItem(HistoryItem*);

    virtual bool canCachePage() const;
    virtual void convertMainResourceLoadToDownload(DocumentLoader*, const ResourceRequest&, const ResourceResponse&);

    virtual void didDisplayInsecureContent();

    virtual void didRunInsecureContent(SecurityOrigin*, const URL&);
    virtual void didDetectXSS(const URL&, bool didBlockEntirePage);

    virtual ResourceError cancelledError(const ResourceRequest&);
    virtual ResourceError blockedError(const ResourceRequest&);
    virtual ResourceError cannotShowURLError(const ResourceRequest&);
    virtual ResourceError interruptedForPolicyChangeError(const ResourceRequest&);


    virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&);
    virtual ResourceError fileDoesNotExistError(const ResourceResponse&);
    virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&);

    virtual bool shouldFallBack(const ResourceError&);

    virtual String userAgent(const URL&);

    virtual void savePlatformDataToCachedFrame(CachedFrame*);
    virtual void transitionToCommittedFromCachedFrame(CachedFrame*);
    virtual void transitionToCommittedForNewPage();

    virtual bool canHandleRequest(const ResourceRequest&) const;
    virtual bool canShowMIMEType(const String& MIMEType) const;
    virtual bool canShowMIMETypeAsHTML(const String& MIMEType) const;
    virtual bool representationExistsForURLScheme(const String& URLScheme) const;
    virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const;

    virtual void frameLoadCompleted();
    virtual void saveViewStateToItem(HistoryItem*);
    virtual void restoreViewState();
    virtual void provisionalLoadStarted();
    virtual void didFinishLoad();
    virtual void prepareForDataSourceReplacement();
    virtual PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&);

    virtual void setTitle(const StringWithDirection&, const URL&);

    virtual PassRefPtr<Frame> createFrame(const URL& url, const String& name, HTMLFrameOwnerElement*,
                                                   const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight);
    virtual void didTransferChildFrameToNewDocument();
    virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const URL&, const Vector<String>&,
                                                     const Vector<String>&, const String&, bool);
    virtual void recreatePlugin(Widget*) { }
    virtual void redirectDataToPlugin(Widget* pluginWidget);

    virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const URL& baseURL,
                                                               const Vector<String>& paramNames, const Vector<String>& paramValues);

    virtual ObjectContentType objectContentType(const URL&, const String& mimeType, bool shouldPreferPlugInsForImages);

    virtual String overrideMediaType() const;

    virtual void didSaveToPageCache();
    virtual void didRestoreFromPageCache();

    virtual void dispatchDidBecomeFrameset(bool);

    virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld&);
    virtual void documentElementAvailable();
    virtual void didPerformFirstNavigation() const;

    virtual void registerForIconNotification(bool listen);

    virtual PassRefPtr<FrameNetworkingContext> createNetworkingContext();

private:
    void callPolicyFunction(FramePolicyFunction, PolicyAction);
    void triggerNavigationHistoryUpdate() const;
    void postCommitFrameViewSetup(BWebFrame*, FrameView*, bool) const;
    bool isTertiaryMouseButton(const NavigationAction& action) const;

	status_t dispatchNavigationRequested(const ResourceRequest& request) const;
    status_t dispatchMessage(BMessage& message, bool allowChildFrame = false) const;

private:
    BWebPage* m_webPage;
    BWebFrame* m_webFrame;
    BMessenger m_messenger;

    ResourceResponse m_response;
    bool m_loadingErrorPage;

    // Plugin view to redirect data to
    PluginView* m_pluginView;
    bool m_hasSentResponseToPlugin;

    // IDNA domain encoding and decoding.
    UIDNA* m_uidna_context;
};

} // namespace WebCore

#endif // FrameLoaderClientHaiku_h

