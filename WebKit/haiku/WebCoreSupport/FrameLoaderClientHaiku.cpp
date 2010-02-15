/*
 * Copyright (C) 2006 Don Gibson <dgibson77@gmail.com>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com> All rights reserved.
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com> All rights reserved.
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Michael Lotz <mmlr@mlotz.ch>
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

#include "config.h"
#include "FrameLoaderClientHaiku.h"

#include "AuthenticationChallenge.h"
#include "Credential.h"
#include "CachedFrame.h"
#include "CString.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "MouseEvent.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformString.h"
#include "PluginData.h"
#include "PluginDatabase.h"
#include "ProgressTracker.h"
#include "RenderFrame.h"
#include "ResourceRequest.h"
#include "ScriptController.h"
#include "Settings.h"
#include "WebFrame.h"
#include "WebProcess.h"
#include "WebView.h"
#include "WebViewConstants.h"

#include <Alert.h>
#include <Entry.h>
#include <Message.h>
#include <MimeType.h>
#include <String.h>


namespace WebCore {

FrameLoaderClientHaiku::FrameLoaderClientHaiku(WebProcess* webProcess, WebFrame* webFrame)
    : m_webProcess(webProcess)
    , m_webFrame(webFrame)
    , m_messenger()
{
printf("%p->FrameLoaderClientHaiku::FrameLoaderClientHaiku()\n", this);
    ASSERT(m_webProcess);
    ASSERT(m_webFrame);
}

void FrameLoaderClientHaiku::setDispatchTarget(BHandler* handler)
{
    m_messenger = BMessenger(handler);
}

void FrameLoaderClientHaiku::frameLoaderDestroyed()
{
    m_webFrame = 0;
        // deleted by WebProcess
    delete this;
}

bool FrameLoaderClientHaiku::hasWebView() const
{
    return m_webProcess->webView();
}

void FrameLoaderClientHaiku::makeRepresentation(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClientHaiku::forceLayout()
{
    FrameView* view = m_webFrame->frame()->view();
    if (view)
        view->forceLayout(true);
}

void FrameLoaderClientHaiku::forceLayoutForNonHTML()
{
    FrameView* view = m_webFrame->frame()->view();
    if (view)
        view->forceLayout();
}

void FrameLoaderClientHaiku::setCopiesOnScroll()
{
    // Other ports mention "apparently mac specific", but I believe it may have to
    // do with achieving that WebCore does not repaint the parts that we can scroll
    // by blitting.
    notImplemented();
}

void FrameLoaderClientHaiku::detachedFromParent2()
{
}

void FrameLoaderClientHaiku::detachedFromParent3()
{
}

bool FrameLoaderClientHaiku::dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int)
{
    notImplemented();
    return false;
}

void FrameLoaderClientHaiku::dispatchDidLoadResourceByXMLHttpRequest(unsigned long, const ScriptString&)
{
    notImplemented();
}

void FrameLoaderClientHaiku::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest& request, const ResourceResponse& response)
{
    notImplemented();
}

bool FrameLoaderClientHaiku::shouldUseCredentialStorage(DocumentLoader*, unsigned long)
{
printf("FrameLoaderClientHaiku::shouldUseCredentialStorage()\n");
    notImplemented();
    return false;
}

void FrameLoaderClientHaiku::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge& challenge)
{
    const ProtectionSpace& space = challenge.protectionSpace();
    String text = "Host \"" + space.host() + "\" requestes authentication for realm \"" + space.realm() + "\"\n";
    text += "Authentication Scheme: ";
    switch (space.authenticationScheme()) {
    case ProtectionSpaceAuthenticationSchemeHTTPBasic:
        text += "Basic (data will be sent as plain text)";
        break;
    case ProtectionSpaceAuthenticationSchemeHTTPDigest:
        text += "Digest (data will not be sent plain text)";
        break;
    default:
        text += "Unknown (possibly plaintext)";
        break;
    }

    BMessage challengeMessage(AUTHENTICATION_CHALLENGE);
    challengeMessage.AddString("text", text);
    challengeMessage.AddString("user", challenge.proposedCredential().user());
    challengeMessage.AddString("password", challenge.proposedCredential().password());
    challengeMessage.AddUInt32("failureCount", challenge.previousFailureCount());

    BMessage authenticationReply;
    m_messenger.SendMessage(&challengeMessage, &authenticationReply);

    BString user;
    BString password;
    if (authenticationReply.FindString("user", &user) != B_OK
        || authenticationReply.FindString("password", &password) != B_OK) {
        challenge.authenticationClient()->receivedCancellation(challenge);
    } else {
        if (!user.Length() && !password.Length())
            challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);
        else {
            bool rememberCredentials = false;
            CredentialPersistence persistence = CredentialPersistenceForSession;
            if (authenticationReply.FindBool("rememberCredentials",
                &rememberCredentials) == B_OK && rememberCredentials) {
                persistence = CredentialPersistencePermanent;
            }

            Credential credential(user.String(), password.String(), persistence);
            challenge.authenticationClient()->receivedCredential(challenge, credential);
        }
    }
}

void FrameLoaderClientHaiku::dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge&)
{
printf("FrameLoaderClientHaiku::dispatchDidCancelAuthenticationChallenge()\n");
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidReceiveResponse(DocumentLoader* loader, unsigned long id, const ResourceResponse& response)
{
    m_response = response;
    if (response.httpStatusCode() != 200)
        printf("dispatchDidReceiveResponse(%s, %d)\n", response.url().string().utf8().data(), response.httpStatusCode());
}

void FrameLoaderClientHaiku::dispatchDidReceiveContentLength(DocumentLoader* loader, unsigned long id, int length)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidFinishLoading(DocumentLoader*, unsigned long)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidFailLoading(DocumentLoader* loader, unsigned long, const ResourceError&)
{
    BMessage message(LOAD_FAILED);
    message.AddString("url", loader->url().string());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::dispatchDidHandleOnloadEvents()
{
    BMessage message(LOAD_ONLOAD_HANDLE);
    message.AddString("url", m_webFrame->frame()->loader()->documentLoader()->request().url().string());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidCancelClientRedirect()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchWillPerformClientRedirect(const KURL&, double interval, double fireDate)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidChangeLocationWithinPage()
{
    BMessage message(LOAD_DOC_COMPLETED);
    message.AddString("url", m_webFrame->frame()->loader()->url().string());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::dispatchDidPushStateWithinPage()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidReplaceStateWithinPage()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidPopStateWithinPage()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchWillClose()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidReceiveIcon()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidStartProvisionalLoad()
{
    BMessage message(LOAD_NEGOCIATING);
    message.AddString("url", m_webFrame->frame()->loader()->provisionalDocumentLoader()->request().url().string());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::dispatchDidReceiveTitle(const String& title)
{
    m_webFrame->setTitle(title);

    BMessage message(TITLE_CHANGED);
    message.AddString("title", title);
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::dispatchDidCommitLoad()
{
    BMessage message(LOAD_TRANSFERRING);
    message.AddString("url", m_webFrame->frame()->loader()->documentLoader()->request().url().string());
    m_messenger.SendMessage(&message);

    // We should assume first the frame has no title. If it has, then the above
    // dispatchDidReceiveTitle() will be called very soon with the correct title.
    // This properly resets the title when we navigate to a URI without a title.
    BMessage titleMessage(TITLE_CHANGED);
    titleMessage.AddString("title", "");
    m_messenger.SendMessage(&titleMessage);
}

void FrameLoaderClientHaiku::dispatchDidFailProvisionalLoad(const ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidFailLoad(const ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidFinishDocumentLoad()
{
    BMessage message(LOAD_DOC_COMPLETED);
    message.AddString("url", m_webFrame->frame()->loader()->url().string());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::dispatchDidFinishLoad()
{
    BMessage message(LOAD_FINISHED);
    message.AddString("url", m_webFrame->frame()->loader()->url().string());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::dispatchDidFirstLayout()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidFirstVisuallyNonEmptyLayout()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchWillSubmitForm(FramePolicyFunction function, PassRefPtr<FormState>)
{
    notImplemented();
    // FIXME: Send an event to allow for alerts and cancellation.
    callPolicyFunction(function, PolicyUse);
}

void FrameLoaderClientHaiku::dispatchDidLoadMainResource(DocumentLoader*)
{
    notImplemented();
}

Frame* FrameLoaderClientHaiku::dispatchCreatePage()
{
printf("FrameLoaderClientHaiku::dispatchCreatePage()\n");
    notImplemented();
//    m_webFrame->webView()->createWebView();
    return 0;
}

void FrameLoaderClientHaiku::dispatchShow()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDecidePolicyForMIMEType(FramePolicyFunction function, const String& mimetype,
                                                             const ResourceRequest& request)
{
printf("FrameLoaderClientHaiku::dispatchDecidePolicyForMIMEType() -> ");
    if (request.isNull()) {
printf("ignore\n");
        callPolicyFunction(function, PolicyIgnore);
        return;
    }
    // we need to call directly here
    if (canShowMIMEType(mimetype)) {
printf("use\n");
        callPolicyFunction(function, PolicyUse);
    } else if (!request.url().isLocalFile()) {
printf("download\n");
        callPolicyFunction(function, PolicyDownload);
    } else {
printf("ignore (local)\n");
        callPolicyFunction(function, PolicyIgnore);
    }
}

void FrameLoaderClientHaiku::dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function,
    const NavigationAction& action, const ResourceRequest& request, PassRefPtr<FormState>, const String& targetName)
{
printf("dispatchDecidePolicyForNewWindowAction\n");
    if (request.isNull()) {
        callPolicyFunction(function, PolicyIgnore);
        return;
    }

    BMessage message(NEW_WINDOW_REQUESTED);
    message.AddString("url", request.url().string());
    if (m_messenger.SendMessage(&message) != B_OK) {
        if (action.type() == NavigationTypeFormSubmitted || action.type() == NavigationTypeFormResubmitted)
            m_webFrame->frame()->loader()->resetMultipleFormSubmissionProtection();

        if (action.type() == NavigationTypeLinkClicked) {
            ResourceRequest emptyRequest;
            m_webFrame->frame()->loader()->activeDocumentLoader()->setLastCheckedRequest(emptyRequest);
        }

        callPolicyFunction(function, PolicyIgnore);
        return;
    }

    callPolicyFunction(function, PolicyUse);
}

void FrameLoaderClientHaiku::dispatchDecidePolicyForNavigationAction(FramePolicyFunction function,
    const NavigationAction& action, const ResourceRequest& request, PassRefPtr<FormState>)
{
printf("dispatchDecidePolicyForNavigationAction\n");
    if (request.isNull()) {
        callPolicyFunction(function, PolicyIgnore);
        return;
    }

    uint32 what = NAVIGATION_REQUESTED;
    if (action.event() && action.event()->isMouseEvent()) {
        // Clicks with the middle mouse button shall open a new window
        // (or tab respectively depending on browser)
        const MouseEvent* mouseEvent = dynamic_cast<const MouseEvent*>(action.event());
        if (mouseEvent && mouseEvent->button() == 1)
            what = NEW_WINDOW_REQUESTED;
    }
    BMessage message(what);
    message.AddString("url", request.url().string());
    if (m_messenger.SendMessage(&message) != B_OK || what == NEW_WINDOW_REQUESTED) {
        if (action.type() == NavigationTypeFormSubmitted || action.type() == NavigationTypeFormResubmitted)
            m_webFrame->frame()->loader()->resetMultipleFormSubmissionProtection();

        if (action.type() == NavigationTypeLinkClicked) {
            ResourceRequest emptyRequest;
            m_webFrame->frame()->loader()->activeDocumentLoader()->setLastCheckedRequest(emptyRequest);
        }

        callPolicyFunction(function, PolicyIgnore);
        return;
    }

    callPolicyFunction(function, PolicyUse);
}

void FrameLoaderClientHaiku::cancelPolicyCheck()
{
printf("FrameLoaderClientHaiku::cancelPolicyCheck()\n");
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchUnableToImplementPolicy(const ResourceError&)
{
    notImplemented();
}

void FrameLoaderClientHaiku::revertToProvisionalState(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClientHaiku::setMainDocumentError(WebCore::DocumentLoader* loader, const WebCore::ResourceError& error)
{
    if (error.isCancellation())
        return;

    // FIXME: This should be moved into LauncherWindow.
    BString errorString("Error loading ");
    errorString << error.failingURL();
    errorString << ":\n\n";
    errorString << error.localizedDescription();
    BAlert* alert = new BAlert("Main document error", errorString.String(), "OK");
    alert->Go(NULL);
}

void FrameLoaderClientHaiku::postProgressStartedNotification()
{
    if (m_webFrame && m_webFrame->frame()->page()) {
        // A new load starts, so lets clear the previous error.
//        m_loadError = ResourceError();
        postProgressEstimateChangedNotification();
    }
    if (!m_webFrame || m_webFrame->frame()->tree()->parent())
        return;
    triggerNavigationHistoryUpdate();
}

void FrameLoaderClientHaiku::postProgressEstimateChangedNotification()
{
    BMessage message(LOAD_PROGRESS);
    message.AddFloat("progress", m_webFrame->frame()->page()->progress()->estimatedProgress() * 100);
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::postProgressFinishedNotification()
{
    BMessage message(LOAD_DL_COMPLETED);
    message.AddString("url", m_webFrame->frame()->loader()->url().string());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::setMainFrameDocumentReady(bool)
{
    notImplemented();
    // this is only interesting once we provide an external API for the DOM
}

void FrameLoaderClientHaiku::download(ResourceHandle* handle, const ResourceRequest& request, const ResourceRequest&, const ResourceResponse& response)
{
    m_webProcess->requestDownload(handle, request, response);
}

void FrameLoaderClientHaiku::startDownload(const ResourceRequest& request)
{
    m_webProcess->requestDownload(request);
}

void FrameLoaderClientHaiku::willChangeTitle(DocumentLoader*)
{
    // We act in didChangeTitle
}

void FrameLoaderClientHaiku::didChangeTitle(DocumentLoader* docLoader)
{
    setTitle(docLoader->title(), docLoader->url());
}

void FrameLoaderClientHaiku::committedLoad(WebCore::DocumentLoader* loader, const char* data, int length)
{
    Frame* frame = m_webFrame->frame();
    if (!frame)
        return;

    // Set the encoding. This only needs to be done once, but it's harmless to do it again later.
    String encoding = frame->loader()->documentLoader()->overrideEncoding();
    bool userChosen = !encoding.isNull();
    if (!userChosen)
        encoding = loader->response().textEncodingName();
    frame->loader()->setEncoding(encoding, userChosen);

    if (data)
        frame->loader()->addData(data, length);
}

void FrameLoaderClientHaiku::finishedLoading(DocumentLoader* loader)
{
}

void FrameLoaderClientHaiku::updateGlobalHistory()
{
    notImplemented();
}

void FrameLoaderClientHaiku::updateGlobalHistoryRedirectLinks()
{
    WebCore::Frame* frame = m_webFrame->frame();
    if (!frame)
        return;

    BMessage message(UPDATE_HISTORY);
    message.AddString("url", frame->loader()->documentLoader()->urlForHistory().prettyURL());
    m_messenger.SendMessage(&message);
}

bool FrameLoaderClientHaiku::shouldGoToHistoryItem(WebCore::HistoryItem*) const
{
    notImplemented();
    return true;
}

void FrameLoaderClientHaiku::dispatchDidAddBackForwardItem(WebCore::HistoryItem*) const
{
    triggerNavigationHistoryUpdate();
}

void FrameLoaderClientHaiku::dispatchDidRemoveBackForwardItem(WebCore::HistoryItem*) const
{
    triggerNavigationHistoryUpdate();
}

void FrameLoaderClientHaiku::dispatchDidChangeBackForwardIndex() const
{
    triggerNavigationHistoryUpdate();
}

void FrameLoaderClientHaiku::saveScrollPositionAndViewStateToItem(WebCore::HistoryItem*)
{
    notImplemented();
}

void FrameLoaderClientHaiku::didDisplayInsecureContent()
{
}

void FrameLoaderClientHaiku::didRunInsecureContent(SecurityOrigin*)
{
    notImplemented();
}

WebCore::ResourceError FrameLoaderClientHaiku::cancelledError(const WebCore::ResourceRequest& request)
{
    notImplemented();
    ResourceError error = ResourceError(String(), WebKitErrorCannotShowURL, request.url().string(), String());
    error.setIsCancellation(true);
    return error;
}

WebCore::ResourceError FrameLoaderClientHaiku::blockedError(const ResourceRequest& request)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowURL, request.url().string(), String());
}

WebCore::ResourceError FrameLoaderClientHaiku::cannotShowURLError(const WebCore::ResourceRequest& request)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowURL, request.url().string(), String());
}

WebCore::ResourceError FrameLoaderClientHaiku::interruptForPolicyChangeError(const WebCore::ResourceRequest& request)
{
    notImplemented();
    ResourceError error = ResourceError(String(), WebKitErrorFrameLoadInterruptedByPolicyChange, request.url().string(), String());
    error.setIsCancellation(true);
    return error;
}

WebCore::ResourceError FrameLoaderClientHaiku::cannotShowMIMETypeError(const WebCore::ResourceResponse& response)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowMIMEType, response.url().string(), String());
}

WebCore::ResourceError FrameLoaderClientHaiku::fileDoesNotExistError(const WebCore::ResourceResponse& response)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotShowURL, response.url().string(), String());
}

ResourceError FrameLoaderClientHaiku::pluginWillHandleLoadError(const ResourceResponse& response)
{
    notImplemented();
    return ResourceError(String(), WebKitErrorCannotLoadPlugIn, response.url().string(), String());
}

bool FrameLoaderClientHaiku::shouldFallBack(const WebCore::ResourceError& error)
{
    notImplemented();
    return false;
}

bool FrameLoaderClientHaiku::canHandleRequest(const WebCore::ResourceRequest&) const
{
    Frame* frame = m_webFrame->frame();
    Page* page = frame->page();

    return page && page->mainFrame() == frame;
}

bool FrameLoaderClientHaiku::canShowMIMEType(const String& MIMEType) const
{
    if (MIMETypeRegistry::isSupportedImageMIMEType(MIMEType))
        return true;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(MIMEType))
        return true;

    Frame* frame = m_webFrame->frame();
    if (frame && frame->settings() && frame->settings()->arePluginsEnabled()
        && PluginDatabase::installedPlugins()->isMIMETypeRegistered(MIMEType))
        return true;

    return false;
}

bool FrameLoaderClientHaiku::representationExistsForURLScheme(const String& URLScheme) const
{
    notImplemented();
    return false;
}

String FrameLoaderClientHaiku::generatedMIMETypeForURLScheme(const String& URLScheme) const
{
    notImplemented();
    return String();
}

void FrameLoaderClientHaiku::frameLoadCompleted()
{
}

void FrameLoaderClientHaiku::saveViewStateToItem(HistoryItem*)
{
    notImplemented();
}

void FrameLoaderClientHaiku::restoreViewState()
{
printf("FrameLoaderClientHaiku::restoreViewState()\n");
    notImplemented();
}

void FrameLoaderClientHaiku::provisionalLoadStarted()
{
    notImplemented();
}

void FrameLoaderClientHaiku::didFinishLoad()
{
    notImplemented();
}

void FrameLoaderClientHaiku::prepareForDataSourceReplacement()
{
    notImplemented();
}

WTF::PassRefPtr<DocumentLoader> FrameLoaderClientHaiku::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    RefPtr<DocumentLoader> loader = DocumentLoader::create(request, substituteData);
    if (substituteData.isValid())
        loader->setDeferMainResourceDataLoad(false);
    return loader.release();
}

void FrameLoaderClientHaiku::setTitle(const String& title, const KURL&)
{
    notImplemented();
}

void FrameLoaderClientHaiku::savePlatformDataToCachedFrame(CachedFrame*)
{
    // Nothing to be done here for the moment.
}

void FrameLoaderClientHaiku::transitionToCommittedFromCachedFrame(CachedFrame* cachedFrame)
{
    ASSERT(cachedFrame->view());

    Frame* frame = m_webFrame->frame();
    if (frame != frame->page()->mainFrame())
        return;

    postCommitFrameViewSetup(m_webFrame, cachedFrame->view(), false);
}

void FrameLoaderClientHaiku::transitionToCommittedForNewPage()
{
    ASSERT(m_webFrame);

    Frame* frame = m_webFrame->frame();

    BRect bounds = m_webProcess->viewBounds();
    IntSize size = IntSize(bounds.IntegerWidth() + 1, bounds.IntegerHeight() + 1);

    bool transparent = m_webFrame->isTransparent();
    Color backgroundColor = transparent ? WebCore::Color::transparent : WebCore::Color::white;

    frame->createView(size, backgroundColor, transparent, IntSize(), false);

    // We may need to do further manipulation on the FrameView if it was the mainFrame
    if (frame != frame->page()->mainFrame())
        return;

    postCommitFrameViewSetup(m_webFrame, frame->view(), true);
}

String FrameLoaderClientHaiku::userAgent(const KURL&)
{
    return String("Mozilla/5.0 (compatible; U; InfiNet 0.1; Haiku) AppleWebKit/420+ (KHTML, like Gecko)");
}

bool FrameLoaderClientHaiku::canCachePage() const
{
    return true;
}

PassRefPtr<Frame> FrameLoaderClientHaiku::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
    const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight)
{
    // FIXME: We should apply the right property to the frameView. (scrollbar,margins)
    ASSERT(m_webFrame);

    WebFrame* frame = new WebFrame(m_webProcess, m_webFrame->frame()->page(), m_webFrame->frame(), ownerElement, name);
    // As long as we don't return the Frame, we are responsible for deleting it.
    RefPtr<Frame> childFrame = frame->frame();

    // The creation of the frame may have run arbitrary JavaScript that removed it from the page already.
    if (!childFrame->page()) {
printf("   no page\n");
        delete frame;
        return 0;
    }

    childFrame->loader()->loadURLIntoChildFrame(url, referrer, childFrame.get());

    // The frame's onload handler may have removed it from the document.
    if (!childFrame->tree()->parent()) {
printf("   no parent\n");
        delete frame;
        return 0;
    }

    return childFrame.release();
}

ObjectContentType FrameLoaderClientHaiku::objectContentType(const KURL& url, const String& originalMimeType)
{
    if (url.isEmpty() && !originalMimeType.length())
        return ObjectContentNone;

    String mimeType = originalMimeType;
    if (!mimeType.length()) {
        entry_ref ref;
        if (get_ref_for_path(url.path().utf8().data(), &ref) == B_OK) {
            BMimeType type;
            if (BMimeType::GuessMimeType(&ref, &type) == B_OK)
                mimeType = type.Type();
        }
    }

    if (!mimeType.length())
        return ObjectContentFrame;

    if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return ObjectContentImage;

    if (PluginDatabase::installedPlugins()->isMIMETypeRegistered(mimeType))
        return ObjectContentNetscapePlugin;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return ObjectContentFrame;

    if (url.protocol() == "about")
        return ObjectContentFrame;

    return ObjectContentNone;
}

PassRefPtr<Widget> FrameLoaderClientHaiku::createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&,
                                                        const Vector<String>&, const String&, bool loadManually)
{
    notImplemented();
    return 0;
}

void FrameLoaderClientHaiku::redirectDataToPlugin(Widget* pluginWidget)
{
    notImplemented();
    return;
}

PassRefPtr<Widget> FrameLoaderClientHaiku::createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL& baseURL,
                                                                  const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    notImplemented();
    return 0;
}

String FrameLoaderClientHaiku::overrideMediaType() const
{
    notImplemented();
    return String();
}

void FrameLoaderClientHaiku::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld* world)
{
    if (world != mainThreadNormalWorld())
        return;

    if (m_webFrame) {
        BMessage message(JAVASCRIPT_WINDOW_OBJECT_CLEARED);
        m_messenger.SendMessage(&message);
    }
}

void FrameLoaderClientHaiku::documentElementAvailable()
{
}

void FrameLoaderClientHaiku::registerForIconNotification(bool listen)
{
    notImplemented();
}

void FrameLoaderClientHaiku::didPerformFirstNavigation() const
{
    triggerNavigationHistoryUpdate();
}

// #pragma mark - private

void FrameLoaderClientHaiku::callPolicyFunction(FramePolicyFunction function, PolicyAction action)
{
    (m_webFrame->frame()->loader()->policyChecker()->*function)(action);
}

void FrameLoaderClientHaiku::triggerNavigationHistoryUpdate() const
{
    WebCore::Page* page = m_webFrame->frame()->page();
    WebCore::FrameLoader* loader = m_webFrame->frame()->loader();
    if (!page || !loader)
        return;
    BMessage message(UPDATE_NAVIGATION_INTERFACE);
    message.AddBool("can go backward", page->canGoBackOrForward(-1));
    message.AddBool("can go forward", page->canGoBackOrForward(1));
    message.AddBool("can stop", loader->isLoading());
    m_messenger.SendMessage(&message);
}

void FrameLoaderClientHaiku::postCommitFrameViewSetup(WebFrame* frame, FrameView* view, bool resetValues) const
{
    // This method can be used to do adjustments on the main frame, since those
    // are the only ones directly embedded into a WebView.
    view->setTopLevelPlatformWidget(m_webProcess->webView());
}

} // namespace WebCore

