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

    virtual void frameLoaderDestroyed() override;

    virtual bool hasWebView() const override;

    virtual void makeRepresentation(DocumentLoader*) override;
    virtual void forceLayout() override;
    virtual void forceLayoutForNonHTML() override;

    virtual void setCopiesOnScroll() override;

    virtual void detachedFromParent2() override;
    virtual void detachedFromParent3() override;

    virtual void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&) override;

    virtual void dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest&,
                                         const ResourceResponse&) override;
    virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier) override;
    virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*,
                                                           unsigned long identifier,
                                                           const AuthenticationChallenge&) override;
    virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*,
                                                          unsigned long identifier,
                                                          const AuthenticationChallenge&) override;
    virtual void dispatchDidReceiveResponse(DocumentLoader*, unsigned long,
                                            const ResourceResponse&) override;
    virtual void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long, int) override;
    virtual void dispatchDidFinishLoading(DocumentLoader*, unsigned long) override;
    virtual void dispatchDidFailLoading(DocumentLoader*, unsigned long,
                                        const ResourceError&) override;

    virtual void dispatchDidHandleOnloadEvents() override;
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() override;
    virtual void dispatchDidCancelClientRedirect() override;
    virtual void dispatchWillPerformClientRedirect(const URL&, double interval, double fireDate) override;
    virtual void dispatchDidChangeLocationWithinPage() override;
    virtual void dispatchDidPushStateWithinPage() override;
    virtual void dispatchDidReplaceStateWithinPage() override;
    virtual void dispatchDidPopStateWithinPage() override;
    virtual void dispatchWillClose() override;
    virtual void dispatchDidReceiveIcon() override;
    virtual void dispatchDidStartProvisionalLoad() override;
    virtual void dispatchDidReceiveTitle(const StringWithDirection&) override;
    virtual void dispatchDidChangeIcons(IconType) override;
    virtual void dispatchDidCommitLoad() override;
    virtual void dispatchDidFailProvisionalLoad(const ResourceError&) override;
    virtual void dispatchDidFailLoad(const ResourceError&) override;
    virtual void dispatchDidFinishDocumentLoad() override;
    virtual void dispatchDidFinishLoad() override;

    virtual Frame* dispatchCreatePage(const NavigationAction&) override;
    virtual void dispatchShow() override;

    virtual void dispatchDecidePolicyForResponse(const ResourceResponse&, const ResourceRequest&, FramePolicyFunction) override;
    virtual void dispatchDecidePolicyForNewWindowAction(const NavigationAction&,
        const ResourceRequest&, PassRefPtr<FormState>, const String&, FramePolicyFunction) override;
    virtual void dispatchDecidePolicyForNavigationAction(const NavigationAction&,
                                                         const ResourceRequest&, PassRefPtr<FormState>, FramePolicyFunction) override;
    virtual void cancelPolicyCheck() override;

    virtual void dispatchUnableToImplementPolicy(const ResourceError&) override;

    virtual void dispatchWillSendSubmitEvent(PassRefPtr<FormState>) override { }
    virtual void dispatchWillSubmitForm(PassRefPtr<FormState>, FramePolicyFunction) override;

    virtual void revertToProvisionalState(DocumentLoader*) override;
    virtual void setMainDocumentError(DocumentLoader*, const ResourceError&) override;
    virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*,
                                                        const ResourceRequest&,
                                                        const ResourceResponse&, int) override;

    virtual void setMainFrameDocumentReady(bool) override;

    virtual void startDownload(const ResourceRequest&, const String& suggestedName = String()) override;

    virtual void willChangeTitle(DocumentLoader*) override;
    virtual void didChangeTitle(DocumentLoader*) override;

    virtual void committedLoad(DocumentLoader*, const char*, int) override;
    virtual void finishedLoading(DocumentLoader*) override;
    virtual void updateGlobalHistory() override;
    virtual void updateGlobalHistoryRedirectLinks() override;

    virtual bool shouldGoToHistoryItem(HistoryItem*) const override;

    virtual bool canCachePage() const override;
    virtual void convertMainResourceLoadToDownload(DocumentLoader*, const ResourceRequest&, const ResourceResponse&) override;

    virtual void didDisplayInsecureContent() override;

    virtual void didRunInsecureContent(SecurityOrigin*, const URL&) override;
    virtual void didDetectXSS(const URL&, bool didBlockEntirePage) override;

    virtual ResourceError cancelledError(const ResourceRequest&) override;
    virtual ResourceError blockedError(const ResourceRequest&) override;
    virtual ResourceError cannotShowURLError(const ResourceRequest&) override;
    virtual ResourceError interruptedForPolicyChangeError(const ResourceRequest&) override;


    virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&) override;
    virtual ResourceError fileDoesNotExistError(const ResourceResponse&) override;
    virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&) override;

    virtual bool shouldFallBack(const ResourceError&) override;

    virtual String userAgent(const URL&) override;

    virtual void savePlatformDataToCachedFrame(CachedFrame*) override;
    virtual void transitionToCommittedFromCachedFrame(CachedFrame*) override;
    virtual void transitionToCommittedForNewPage() override;

    virtual bool canHandleRequest(const ResourceRequest&) const override;
    virtual bool canShowMIMEType(const String& MIMEType) const override;
    virtual bool canShowMIMETypeAsHTML(const String& MIMEType) const override;
    virtual bool representationExistsForURLScheme(const String& URLScheme) const override;
    virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const override;

    virtual void frameLoadCompleted() override;
    virtual void saveViewStateToItem(HistoryItem*) override;
    virtual void restoreViewState() override;
    virtual void provisionalLoadStarted() override;
    virtual void didFinishLoad() override;
    virtual void prepareForDataSourceReplacement() override;
    virtual PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&) override;

    virtual void setTitle(const StringWithDirection&, const URL&) override;

    virtual PassRefPtr<Frame> createFrame(const URL& url, const String& name, HTMLFrameOwnerElement*,
                                                   const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight) override;
    virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const URL&, const Vector<String>&,
                                                     const Vector<String>&, const String&, bool) override;
    virtual void recreatePlugin(Widget*) override { }
    virtual void redirectDataToPlugin(Widget* pluginWidget) override;

    virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const URL& baseURL,
                                                               const Vector<String>& paramNames, const Vector<String>& paramValues) override;

    virtual ObjectContentType objectContentType(const URL&, const String& mimeType, bool shouldPreferPlugInsForImages) override;

    virtual String overrideMediaType() const override;

    virtual void didSaveToPageCache() override;
    virtual void didRestoreFromPageCache() override;

    virtual void dispatchDidBecomeFrameset(bool) override;

    virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld&) override;

    virtual void registerForIconNotification(bool listen) override;

    virtual PassRefPtr<FrameNetworkingContext> createNetworkingContext() override;

private:
    void callPolicyFunction(FramePolicyFunction, PolicyAction);
    void postCommitFrameViewSetup(FrameView*) const;
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

