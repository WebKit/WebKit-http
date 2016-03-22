/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 *
 * All rights reserved.
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
#ifndef FrameLoaderClientQt_h
#define FrameLoaderClientQt_h

// FIXME
#define PLUGIN_VIEW_IS_BROKEN 1

#include "FormState.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTMLFormElement.h"
#include "ProgressTrackerClient.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include "URL.h"
#include <QUrl>
#include <qobject.h>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

#if !PLUGIN_VIEW_IS_BROKEN
#include "WebCore/plugins/PluginView.h"
#endif

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

class QWebFrame;
class QWebFrameAdapter;

namespace WebCore {

class AuthenticationChallenge;
class DocumentLoader;
class Element;
class FormState;
class NavigationAction;
class FrameNetworkingContext;
class ResourceLoader;

struct LoadErrorResetToken;

class FrameLoaderClientQt : public QObject, public FrameLoaderClient {
    Q_OBJECT

    friend class ::QWebFrameAdapter;
    void callPolicyFunction(FramePolicyFunction, PolicyAction);
    bool callErrorPageExtension(const ResourceError&);

Q_SIGNALS:
    void titleChanged(const QString& title);
    void unsupportedContent(QNetworkReply*);

public:
    FrameLoaderClientQt();
    ~FrameLoaderClientQt();
    void frameLoaderDestroyed() override;

    void setFrame(QWebFrameAdapter*, Frame*);

    bool hasWebView() const override; // mainly for assertions

    void makeRepresentation(DocumentLoader*) override { }
    void forceLayoutForNonHTML() override;

    void setCopiesOnScroll() override;

    void detachedFromParent2() override;
    void detachedFromParent3() override;

    void assignIdentifierToInitialRequest(unsigned long identifier, WebCore::DocumentLoader*, const WebCore::ResourceRequest&) override;

    void dispatchWillSendRequest(WebCore::DocumentLoader*, unsigned long, WebCore::ResourceRequest&, const WebCore::ResourceResponse&) override;
    bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier) override;
    void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&) override;
    void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&) override;
    void dispatchDidReceiveResponse(WebCore::DocumentLoader*, unsigned long, const WebCore::ResourceResponse&) override;
    void dispatchDidReceiveContentLength(WebCore::DocumentLoader*, unsigned long, int) override;
    void dispatchDidFinishLoading(WebCore::DocumentLoader*, unsigned long) override;
    void dispatchDidFailLoading(WebCore::DocumentLoader*, unsigned long, const WebCore::ResourceError&) override;
    bool dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int) override;

    void dispatchDidDispatchOnloadEvents() override;
    void dispatchDidReceiveServerRedirectForProvisionalLoad() override;
    void dispatchDidCancelClientRedirect() override;
    void dispatchWillPerformClientRedirect(const URL&, double interval, double fireDate) override;
    void dispatchDidNavigateWithinPage() override;
    void dispatchDidChangeLocationWithinPage() override;
    void dispatchDidPushStateWithinPage() override;
    void dispatchDidReplaceStateWithinPage() override;
    void dispatchDidPopStateWithinPage() override;
    void dispatchWillClose() override;
    void dispatchDidReceiveIcon() override;
    void dispatchDidStartProvisionalLoad() override;
    void dispatchDidReceiveTitle(const StringWithDirection&) override;
    void dispatchDidChangeIcons(WebCore::IconType) override;
    void dispatchDidCommitLoad() override;
    void dispatchDidFailProvisionalLoad(const ResourceError&) override;
    void dispatchDidFailLoad(const WebCore::ResourceError&) override;
    void dispatchDidFinishDocumentLoad() override;
    void dispatchDidFinishLoad() override;
    void dispatchDidLayout(WebCore::LayoutMilestones) override;

    WebCore::Frame* dispatchCreatePage(const WebCore::NavigationAction&) override;
    void dispatchShow() override;

    void dispatchDecidePolicyForResponse(const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, FramePolicyFunction) override;
    void dispatchDecidePolicyForNewWindowAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<FormState>, const WTF::String&, FramePolicyFunction) override;
    void dispatchDecidePolicyForNavigationAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, PassRefPtr<FormState>, FramePolicyFunction) override;
    void cancelPolicyCheck() override;

    void dispatchUnableToImplementPolicy(const WebCore::ResourceError&) override;

    void dispatchWillSendSubmitEvent(PassRefPtr<FormState>) override { }
    void dispatchWillSubmitForm(PassRefPtr<FormState>, FramePolicyFunction) override;

    void revertToProvisionalState(DocumentLoader*) override { }
    void setMainDocumentError(DocumentLoader*, const ResourceError&) override;

    void setMainFrameDocumentReady(bool) override;

    void startDownload(const WebCore::ResourceRequest&, const String& suggestedName = String()) override;

    void willChangeTitle(DocumentLoader*) override;
    void didChangeTitle(DocumentLoader*) override;

    void committedLoad(WebCore::DocumentLoader*, const char*, int) override;
    void finishedLoading(DocumentLoader*) override;

    void updateGlobalHistory() override;
    void updateGlobalHistoryRedirectLinks() override;
    bool shouldGoToHistoryItem(HistoryItem*) const override;
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

    bool canHandleRequest(const WebCore::ResourceRequest&) const override;
    bool canShowMIMEType(const String& MIMEType) const override;
    bool canShowMIMETypeAsHTML(const String& MIMEType) const override;
    bool representationExistsForURLScheme(const String& URLScheme) const override;
    String generatedMIMETypeForURLScheme(const String& URLScheme) const override;

    void frameLoadCompleted() override;
    void saveViewStateToItem(WebCore::HistoryItem*) override;
    void restoreViewState() override;
    void provisionalLoadStarted() override;
    void didFinishLoad() override;
    void prepareForDataSourceReplacement() override;

    Ref<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&) override;
    void setTitle(const StringWithDirection&, const URL&) override;

    String userAgent(const WebCore::URL&) override;

    void savePlatformDataToCachedFrame(WebCore::CachedFrame*) override;
    void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*) override;
    void transitionToCommittedForNewPage() override;

    void didSaveToPageCache() override;
    void didRestoreFromPageCache() override;

    void dispatchDidBecomeFrameset(bool) override;

    bool canCachePage() const override;
    void convertMainResourceLoadToDownload(DocumentLoader*,SessionID, const ResourceRequest&, const WebCore::ResourceResponse&) override;

    RefPtr<Frame> createFrame(const URL&, const String& name, HTMLFrameOwnerElement*, const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight) override;
    RefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const URL&, const Vector<String>&, const Vector<String>&, const String&, bool) override;
    void recreatePlugin(Widget*) override { }
    void redirectDataToPlugin(Widget* pluginWidget) override;

    PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const URL& baseURL, const Vector<String>& paramNames, const Vector<String>& paramValues) override;

    ObjectContentType objectContentType(const URL&, const String& mimeTypeIn) override;
    String overrideMediaType() const override;

    void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld&) override;

    void registerForIconNotification(bool) override;

    void willReplaceMultipartContent() override;
    void didReplaceMultipartContent() override;
    ResourceError blockedByContentBlockerError(const ResourceRequest &) override;
    void updateCachedDocumentLoader(DocumentLoader &) override;
    void prefetchDNS(const WTF::String &) override;

    QString chooseFile(const QString& oldFile);

    PassRefPtr<FrameNetworkingContext> createNetworkingContext() override;

    const URL& lastRequestedUrl() const { return m_lastRequestedUrl; }

    QWebFrameAdapter* webFrame() const;

    void originatingLoadStarted() { m_isOriginatingLoad = true; }

    static bool dumpFrameLoaderCallbacks;
    static bool dumpUserGestureInFrameLoaderCallbacks;
    static bool dumpResourceLoadCallbacks;
    static bool dumpResourceResponseMIMETypes;
    static bool dumpWillCacheResponseCallbacks;
    static QString dumpResourceLoadCallbacksPath;
    static bool sendRequestReturnsNullOnRedirect;
    static bool sendRequestReturnsNull;
    static QStringList sendRequestClearHeaders;
    static bool policyDelegateEnabled;
    static bool policyDelegatePermissive;
    static bool deferMainResourceDataLoad;
    static bool dumpHistoryCallbacks;
    static QMap<QString, QString> URLsToRedirect;

private Q_SLOTS:
    void onIconLoadedForPageURL(const QString&);

private:
    void emitLoadStarted();
    void emitLoadFinished(bool ok);

    Frame *m_frame;
    QWebFrameAdapter *m_webFrame;
    ResourceResponse m_response;

#if !PLUGIN_VIEW_IS_BROKEN
    // Plugin view to redirect data to
    WebCore::PluginView* m_pluginView;
#endif
    bool m_hasSentResponseToPlugin;

    URL m_lastRequestedUrl;
    bool m_isOriginatingLoad;
};

}

#endif
