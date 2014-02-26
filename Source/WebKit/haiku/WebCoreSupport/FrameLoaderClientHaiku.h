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

    void frameLoaderDestroyed() override;

    bool hasWebView() const override;

    void makeRepresentation(DocumentLoader*) override;
    void forceLayout() override;
    void forceLayoutForNonHTML() override;

    void setCopiesOnScroll() override;

    void detachedFromParent2() override;
    void detachedFromParent3() override;

    void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&) override;

    void dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest&,
                                         const ResourceResponse&) override;
    bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier) override;
    void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*,
                                                           unsigned long identifier,
                                                           const AuthenticationChallenge&) override;
    void dispatchDidCancelAuthenticationChallenge(DocumentLoader*,
                                                          unsigned long identifier,
                                                          const AuthenticationChallenge&) override;
    void dispatchDidReceiveResponse(DocumentLoader*, unsigned long,
                                            const ResourceResponse&) override;
    void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long, int) override;
    void dispatchDidFinishLoading(DocumentLoader*, unsigned long) override;
    void dispatchDidFailLoading(DocumentLoader*, unsigned long,
                                        const ResourceError&) override;

    void dispatchDidHandleOnloadEvents() override;
    void dispatchDidReceiveServerRedirectForProvisionalLoad() override;
    void dispatchDidCancelClientRedirect() override;
    void dispatchWillPerformClientRedirect(const URL&, double interval, double fireDate) override;
    void dispatchDidChangeLocationWithinPage() override;
    void dispatchDidPushStateWithinPage() override;
    void dispatchDidReplaceStateWithinPage() override;
    void dispatchDidPopStateWithinPage() override;
    void dispatchWillClose() override;
    void dispatchDidReceiveIcon() override;
    void dispatchDidStartProvisionalLoad() override;
    void dispatchDidReceiveTitle(const StringWithDirection&) override;
    void dispatchDidChangeIcons(IconType) override;
    void dispatchDidCommitLoad() override;
    void dispatchDidFailProvisionalLoad(const ResourceError&) override;
    void dispatchDidFailLoad(const ResourceError&) override;
    void dispatchDidFinishDocumentLoad() override;
    void dispatchDidFinishLoad() override;

    Frame* dispatchCreatePage(const NavigationAction&) override;
    void dispatchShow() override;

    void dispatchDecidePolicyForResponse(const ResourceResponse&, const ResourceRequest&, FramePolicyFunction) override;
    void dispatchDecidePolicyForNewWindowAction(const NavigationAction&,
        const ResourceRequest&, PassRefPtr<FormState>, const String&, FramePolicyFunction) override;
    void dispatchDecidePolicyForNavigationAction(const NavigationAction&,
                                                         const ResourceRequest&, PassRefPtr<FormState>, FramePolicyFunction) override;
    void cancelPolicyCheck() override;

    void dispatchUnableToImplementPolicy(const ResourceError&) override;

    void dispatchWillSendSubmitEvent(PassRefPtr<FormState>) override { }
    void dispatchWillSubmitForm(PassRefPtr<FormState>, FramePolicyFunction) override;

    void revertToProvisionalState(DocumentLoader*) override;
    void setMainDocumentError(DocumentLoader*, const ResourceError&) override;
    bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*,
                                                        const ResourceRequest&,
                                                        const ResourceResponse&, int) override;

    void setMainFrameDocumentReady(bool) override;

    void startDownload(const ResourceRequest&, const String& suggestedName = String()) override;

    void willChangeTitle(DocumentLoader*) override;
    void didChangeTitle(DocumentLoader*) override;

    void committedLoad(DocumentLoader*, const char*, int) override;
    void finishedLoading(DocumentLoader*) override;
    void updateGlobalHistory() override;
    void updateGlobalHistoryRedirectLinks() override;

    bool shouldGoToHistoryItem(HistoryItem*) const override;

    bool canCachePage() const override;
    void convertMainResourceLoadToDownload(DocumentLoader*, const ResourceRequest&, const ResourceResponse&) override;

    void didDisplayInsecureContent() override;

    void didRunInsecureContent(SecurityOrigin*, const URL&) override;
    void didDetectXSS(const URL&, bool didBlockEntirePage) override;

    ResourceError cancelledError(const ResourceRequest&) override;
    ResourceError blockedError(const ResourceRequest&) override;
    ResourceError cannotShowURLError(const ResourceRequest&) override;
    ResourceError interruptedForPolicyChangeError(const ResourceRequest&) override;


    ResourceError cannotShowMIMETypeError(const ResourceResponse&) override;
    ResourceError fileDoesNotExistError(const ResourceResponse&) override;
    ResourceError pluginWillHandleLoadError(const ResourceResponse&) override;

    bool shouldFallBack(const ResourceError&) override;

    String userAgent(const URL&) override;

    void savePlatformDataToCachedFrame(CachedFrame*) override;
    void transitionToCommittedFromCachedFrame(CachedFrame*) override;
    void transitionToCommittedForNewPage() override;

    bool canHandleRequest(const ResourceRequest&) const override;
    bool canShowMIMEType(const String& MIMEType) const override;
    bool canShowMIMETypeAsHTML(const String& MIMEType) const override;
    bool representationExistsForURLScheme(const String& URLScheme) const override;
    String generatedMIMETypeForURLScheme(const String& URLScheme) const override;

    void frameLoadCompleted() override;
    void saveViewStateToItem(HistoryItem*) override;
    void restoreViewState() override;
    void provisionalLoadStarted() override;
    void didFinishLoad() override;
    void prepareForDataSourceReplacement() override;
    PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&) override;

    void setTitle(const StringWithDirection&, const URL&) override;

    PassRefPtr<Frame> createFrame(const URL& url, const String& name, HTMLFrameOwnerElement*,
                                                   const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight) override;
    PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const URL&, const Vector<String>&,
                                                     const Vector<String>&, const String&, bool) override;
    void recreatePlugin(Widget*) override { }
    void redirectDataToPlugin(Widget* pluginWidget) override;

    PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const URL& baseURL,
                                                               const Vector<String>& paramNames, const Vector<String>& paramValues) override;

    ObjectContentType objectContentType(const URL&, const String& mimeType, bool shouldPreferPlugInsForImages) override;

    String overrideMediaType() const override;

    void didSaveToPageCache() override;
    void didRestoreFromPageCache() override;

    void dispatchDidBecomeFrameset(bool) override;

    void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld&) override;

    void registerForIconNotification(bool listen) override;

    PassRefPtr<FrameNetworkingContext> createNetworkingContext() override;

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

