/*
 * Copyright (C) 2007 Kevin Ollivier <kevino@theolliviers.com>
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 
#ifndef FrameLoaderClientWx_H
#define FrameLoaderClientWx_H

#include "FrameLoaderClient.h"
#include "FrameLoader.h"
#include "KURL.h"
#include "PluginView.h"
#include "ResourceResponse.h"
#include "HTMLPlugInElement.h"
#include <wtf/Forward.h>

class wxWebFrame;
class wxWebView;

namespace WebCore {

    class AuthenticationChallenge;
    class DocumentLoader;
    class Element;
    class FormState;
    class NavigationAction;
    class ResourceLoader;

    struct LoadErrorResetToken;

    class FrameLoaderClientWx : public FrameLoaderClient {
    public:
        FrameLoaderClientWx();
        ~FrameLoaderClientWx();
        
        wxWebFrame* webFrame() { return m_webFrame; }
        void setFrame(wxWebFrame *frame);
        wxWebView* webView() { return m_webView; }
        void setWebView(wxWebView *webview);

        virtual bool hasWebView() const; // mainly for assertions

        virtual bool hasBackForwardList() const;
        virtual void resetBackForwardList();

        virtual bool provisionalItemIsTarget() const;

        virtual void makeRepresentation(DocumentLoader*);
        virtual void forceLayout();
        virtual void forceLayoutForNonHTML();

        virtual void updateHistoryForCommit();

        virtual void updateHistoryForBackForwardNavigation();
        virtual void updateHistoryForReload();
        virtual void updateHistoryForStandardLoad();
        virtual void updateHistoryForInternalLoad();

        virtual void updateHistoryAfterClientRedirect();

        virtual void setCopiesOnScroll();

        virtual LoadErrorResetToken* tokenForLoadErrorReset();
        virtual void resetAfterLoadError(LoadErrorResetToken*);
        virtual void doNotResetAfterLoadError(LoadErrorResetToken*);

        virtual void willCloseDocument();

        virtual void detachedFromParent2();
        virtual void detachedFromParent3();

        virtual void frameLoaderDestroyed();
        virtual bool canHandleRequest(const ResourceRequest&) const;

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
        virtual void dispatchDidReceiveTitle(const StringWithDirection& title);
        virtual void dispatchDidCommitLoad();
        virtual void dispatchDidFinishDocumentLoad();
        virtual void dispatchDidFinishLoad();
        virtual void dispatchDidFirstLayout();
        virtual void dispatchDidFirstVisuallyNonEmptyLayout();
        virtual void dispatchDidChangeIcons(WebCore::IconType);

        virtual void dispatchShow();
        virtual void cancelPolicyCheck();

        virtual void dispatchWillSendSubmitEvent(HTMLFormElement*) { }
        virtual void dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>);

        virtual void dispatchDidLoadMainResource(DocumentLoader*);
        virtual void revertToProvisionalState(DocumentLoader*);

        virtual void postProgressStartedNotification();
        virtual void postProgressEstimateChangedNotification();
        virtual void postProgressFinishedNotification();

        virtual void progressStarted();
        virtual void progressCompleted();
        virtual void setMainFrameDocumentReady(bool);
        virtual void willChangeTitle(DocumentLoader*);
        virtual void didChangeTitle(DocumentLoader*);
        virtual void finishedLoading(DocumentLoader*);

        virtual bool canShowMIMEType(const String& MIMEType) const;
        virtual bool canShowMIMETypeAsHTML(const String& MIMEType) const;
        virtual bool representationExistsForURLScheme(const String& URLScheme) const;
        virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const;

        virtual void frameLoadCompleted();
        virtual void saveViewStateToItem(HistoryItem*);
        virtual void restoreViewState();
        virtual void restoreScrollPositionAndViewState();
        virtual void provisionalLoadStarted();
        virtual bool shouldTreatURLAsSameAsCurrent(const KURL&) const;
        virtual void addHistoryItemForFragmentScroll();
        virtual void didFinishLoad();
        virtual void prepareForDataSourceReplacement();
        virtual void setTitle(const StringWithDirection&, const KURL&);

        virtual String userAgent(const KURL&);

        virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*);
        virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*);
        virtual void transitionToCommittedForNewPage();

        virtual void didSaveToPageCache();
        virtual void didRestoreFromPageCache();

        virtual void dispatchDidBecomeFrameset(bool);

        virtual void updateGlobalHistory();
        virtual void updateGlobalHistoryRedirectLinks();
        virtual bool shouldGoToHistoryItem(HistoryItem*) const;
        virtual bool shouldStopLoadingForHistoryItem(HistoryItem*) const;
        virtual void saveScrollPositionAndViewStateToItem(HistoryItem*);
        virtual bool canCachePage() const;
        
        virtual void didDisplayInsecureContent();
        virtual void didRunInsecureContent(SecurityOrigin*, const KURL&);
        virtual void didDetectXSS(const KURL&, bool didBlockEntirePage);

        virtual void setMainDocumentError(DocumentLoader*, const ResourceError&);
        virtual void committedLoad(DocumentLoader*, const char*, int);
        virtual ResourceError cancelledError(const ResourceRequest&);
        virtual ResourceError blockedError(const ResourceRequest&);
        virtual ResourceError cannotShowURLError(const ResourceRequest&);
        virtual ResourceError interruptedForPolicyChangeError(const ResourceRequest&);
        virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&);
        virtual ResourceError fileDoesNotExistError(const ResourceResponse&);
        virtual bool shouldFallBack(const ResourceError&);
        virtual WTF::PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&);
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
        virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int);

        virtual void dispatchDidFailProvisionalLoad(const ResourceError&);
        virtual void dispatchDidFailLoad(const ResourceError&);
        virtual Frame* dispatchCreatePage(const WebCore::NavigationAction&);
        virtual void dispatchDecidePolicyForResponse(FramePolicyFunction function, const ResourceResponse&, const ResourceRequest&);
        virtual void dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String&);
        virtual void dispatchDecidePolicyForNavigationAction(FramePolicyFunction function, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>);
        virtual void dispatchUnableToImplementPolicy(const ResourceError&);

        virtual void startDownload(const ResourceRequest&, const String& suggestedName = String());
        
        // FIXME: This should probably not be here, but it's needed for the tests currently
        virtual void partClearedInBegin();

        virtual PassRefPtr<Frame> createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
                                   const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight);
        virtual void didTransferChildFrameToNewDocument(Page*);
        virtual void transferLoadingResourceFromPage(ResourceLoader*, const ResourceRequest&, Page*);
        virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool loadManually) ;
        virtual void redirectDataToPlugin(Widget* pluginWidget);
        virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&);
        
        virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL& baseURL, const Vector<String>& paramNames, const Vector<String>& paramValues);

        virtual ObjectContentType objectContentType(const KURL&, const String& mimeType, bool shouldPreferPlugInsForImages);
        virtual String overrideMediaType() const;

        virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld*);
        virtual void documentElementAvailable();
        
        virtual void didPerformFirstNavigation() const;
        
        virtual void registerForIconNotification(bool listen = true);
        
        virtual bool shouldUsePluginDocument(const String &mimeType) const;
        
        virtual PassRefPtr<FrameNetworkingContext> createNetworkingContext();

    private:
        wxWebFrame *m_webFrame;
        Frame* m_frame;
        wxWebView *m_webView;
        PluginView* m_pluginView;
        bool m_hasSentResponseToPlugin;
        ResourceResponse m_response;
        bool m_firstData;
    };

}

#endif
