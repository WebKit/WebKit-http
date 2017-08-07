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
#include "BackForwardController.h"
#include "Credential.h"
#include "CachedFrame.h"
#include "DocumentLoader.h"
#include "DumpRenderTreeClient.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameNetworkingContextHaiku.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLFormElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTTPParsers.h"
#include "HTTPStatusCodes.h"
#include "IconDatabase.h"
#include "MainFrame.h"
#include "MouseEvent.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PluginData.h"
#include "ProgressTracker.h"
#include "RenderFrame.h"
#include "ResourceRequest.h"
#include "SchemeRegistry.h"
#include "ScriptController.h"
#include "Settings.h"
#include "WebFrame.h"
#include "WebFramePrivate.h"
#include "WebKitInfo.h"
#include "WebPage.h"
#include "WebView.h"
#include "WebViewConstants.h"

#include <JavaScriptCore/APICast.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Entry.h>
#include <Locale.h>
#include <Message.h>
#include <MimeType.h>
#include <String.h>
#include <Url.h>
#include <assert.h>
#include <debugger.h>

//#define TRACE_FRAME_LOADER_CLIENT
#ifdef TRACE_FRAME_LOADER_CLIENT
#   define CLASS_NAME "FLC"
#   include "FunctionTracer.h"
#else
#    define CALLED(x...)
#    define TRACE(x...)
#endif

namespace WebCore {

FrameLoaderClientHaiku::FrameLoaderClientHaiku(BWebPage* webPage)
    : m_webPage(webPage)
    , m_webFrame(nullptr)
    , m_messenger()
    , m_loadingErrorPage(false)
    , m_uidna_context(nullptr)
{
    CALLED("BWebPage: %p", webPage);
    ASSERT(m_webPage);
}

void FrameLoaderClientHaiku::setDispatchTarget(const BMessenger& messenger)
{
    m_messenger = messenger;
}

BWebPage* FrameLoaderClientHaiku::page() const
{
    return m_webPage;
}

void FrameLoaderClientHaiku::frameLoaderDestroyed()
{
    CALLED();

    uidna_close(m_uidna_context);
    delete m_webFrame;
        // The frame takes care of deleting us. When that call returns, the
        // frame loader object is gone!
}

bool FrameLoaderClientHaiku::hasWebView() const
{
    return m_webPage->WebView();
}

void FrameLoaderClientHaiku::makeRepresentation(DocumentLoader*)
{
}

void FrameLoaderClientHaiku::forceLayoutForNonHTML()
{
}

void FrameLoaderClientHaiku::setCopiesOnScroll()
{
    // Other ports mention "apparently mac specific", but I believe it may have to
    // do with achieving that WebCore does not repaint the parts that we can scroll
    // by blitting.
}

void FrameLoaderClientHaiku::detachedFromParent2()
{
}

void FrameLoaderClientHaiku::detachedFromParent3()
{
}

bool FrameLoaderClientHaiku::dispatchDidLoadResourceFromMemoryCache(DocumentLoader* /*loader*/,
                                                                    const ResourceRequest& /*request*/,
                                                                    const ResourceResponse& /*response*/,
                                                                    int /*length*/)
{
    notImplemented();
    return false;
}

void FrameLoaderClientHaiku::assignIdentifierToInitialRequest(unsigned long /*identifier*/,
                                                              DocumentLoader* /*loader*/,
                                                              const ResourceRequest& /*request*/)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchWillSendRequest(DocumentLoader* /*loader*/, unsigned long /*identifier*/,
                                                     ResourceRequest& /*request*/,
                                                     const ResourceResponse& /*redirectResponse*/)
{
    notImplemented();
}

bool FrameLoaderClientHaiku::shouldUseCredentialStorage(DocumentLoader*, unsigned long)
{
    notImplemented();
    return false;
}

void FrameLoaderClientHaiku::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge& challenge)
{
    const ProtectionSpace& space = challenge.protectionSpace();
    String text = "Host \"" + space.host() + "\" requests authentication for realm \"" + space.realm() + "\"\n";
    text.append("Authentication Scheme: ");
    switch (space.authenticationScheme()) {
    case ProtectionSpaceAuthenticationSchemeHTTPBasic:
        text.append("Basic (data will be sent as plain text)");
        break;
    case ProtectionSpaceAuthenticationSchemeHTTPDigest:
        text.append("Digest (data will not be sent plain text)");
        break;
    default:
        text.append("Unknown (possibly plaintext)");
        break;
    }

    BMessage challengeMessage(AUTHENTICATION_CHALLENGE);
    challengeMessage.AddString("text", text);
    challengeMessage.AddString("user", challenge.proposedCredential().user());
    challengeMessage.AddString("password", challenge.proposedCredential().password());
    challengeMessage.AddUInt32("failureCount", challenge.previousFailureCount());
    challengeMessage.AddPointer("view", m_webPage->WebView());

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
    notImplemented();
}


bool FrameLoaderClientHaiku::dispatchDidReceiveInvalidCertificate(DocumentLoader*,
    const CertificateInfo& certificate, const char* message)
{
	// FIXME use m_messenger as above, so that the alert is generated from the
	// API side and can be overriden by clients with a fancier one (or hidden
	// completely in case of DumpRenderTree).
    String text = "The SSL certificate received from " + 
        m_webFrame->Frame()->document()->url().string() + " could not be "
        "authenticated for the following reason: " + message + ".\n\n"
        "The secure connection to the website may be compromised, make sure "
        "to not send any sensitive information.";

    BMessage warningMessage(SSL_CERT_ERROR);
    warningMessage.AddString("text", text.utf8().data());
    warningMessage.AddPointer("certificate info", &certificate);

    BMessage reply;
    m_messenger.SendMessage(&warningMessage, &reply);

    bool continueAnyway = reply.FindBool("continue");

    return continueAnyway;
}


void FrameLoaderClientHaiku::dispatchDidReceiveResponse(DocumentLoader* loader,
                                                        unsigned long identifier,
                                                        const ResourceResponse& coreResponse)
{
    loader->writer().setEncoding(coreResponse.textEncodingName(), false);

    BMessage message(RESPONSE_RECEIVED);
    message.AddInt32("status", coreResponse.httpStatusCode());
    message.AddInt32("identifier", identifier);
    message.AddString("url", coreResponse.url().string());
    message.AddString("mimeType", coreResponse.mimeType());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidReceiveContentLength(DocumentLoader* /*loader*/,
                                                             unsigned long /*id*/, int /*length*/)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidFinishLoading(DocumentLoader* /*loader*/, unsigned long /*identifier*/)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidFailLoading(DocumentLoader* loader, unsigned long, const ResourceError& error)
{
    if (error.isCancellation())
        return;
    BMessage message(LOAD_FAILED);
    message.AddString("url", loader->url().string());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidDispatchOnloadEvents()
{
    CALLED();
    BMessage message(LOAD_ONLOAD_HANDLE);
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidCancelClientRedirect()
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchWillPerformClientRedirect(const URL&, double /*interval*/, double /*fireDate*/)
{
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDidChangeLocationWithinPage()
{
    BMessage message(LOAD_DOC_COMPLETED);
    message.AddPointer("frame", m_webFrame);
    message.AddString("url", m_webFrame->Frame()->document()->url().string());
    dispatchMessage(message);
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
    if (m_loadingErrorPage)
        return;

    BMessage message(ICON_RECEIVED);
    message.AddString("url", m_webFrame->Frame()->document()->url().string());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidStartProvisionalLoad()
{
    CALLED();
    if (m_loadingErrorPage) {
        TRACE("m_loadingErrorPage\n");
        m_loadingErrorPage = false;
    }

    BMessage message(LOAD_NEGOTIATING);
    message.AddString("url", m_webFrame->Frame()->loader().provisionalDocumentLoader()->request().url().string());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidReceiveTitle(const StringWithDirection& title)
{
    CALLED();
    if (m_loadingErrorPage) {
        TRACE("m_loadingErrorPage\n");
        return;
    }

    m_webFrame->SetTitle(title.string());

    BMessage message(TITLE_CHANGED);
    message.AddString("title", title.string());
    message.AddBool("ltr", isLeftToRightDirection(title.direction()));
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidChangeIcons(WebCore::IconType iconType)
{
    if (!m_webFrame)
        return;

    // Other icon types are apple-specific
    ASSERT_UNUSED(iconType, iconType == WebCore::Favicon);

    BMessage message(ICON_CHANGED);
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidCommitLoad()
{
    CALLED();
    if (m_loadingErrorPage) {
        TRACE("m_loadingErrorPage\n");
        return;
    }

    BMessage message(LOAD_COMMITTED);
    URL url = m_webFrame->Frame()->loader().documentLoader()->request().url();
    BUrl decoded(url);

    // In WebKit URL, the host may be IDN-encoded. Decode it for displaying.
    char dest[2048];
    UErrorCode error = U_ZERO_ERROR;
    if (!m_uidna_context)
        m_uidna_context = uidna_openUTS46(UIDNA_DEFAULT, &error);
    UIDNAInfo info = UIDNA_INFO_INITIALIZER;

    uidna_nameToUnicodeUTF8(m_uidna_context, url.host().utf8().data(),
        -1 /* NULL-terminated */, dest, sizeof(dest), &info, &error);
   
    if (U_SUCCESS(error) && info.errors == 0)
        decoded.SetHost(dest);

    message.AddString("url", decoded);
    dispatchMessage(message);

    // We should assume first the frame has no title. If it has, then the above
    // dispatchDidReceiveTitle() will be called very soon with the correct title.
    // This properly resets the title when we navigate to a URI without a title.
    BMessage titleMessage(TITLE_CHANGED);
    titleMessage.AddString("title", "");
    dispatchMessage(titleMessage);
}

void FrameLoaderClientHaiku::dispatchDidFailProvisionalLoad(const ResourceError& error)
{
    dispatchDidFailLoad(error);
}

void FrameLoaderClientHaiku::dispatchDidFailLoad(const ResourceError& error)
{
    CALLED();
    if (m_loadingErrorPage) {
        TRACE("m_loadingErrorPage\n");
        return;
    }
    if (!shouldFallBack(error)) {
        TRACE("should not fall back\n");
        return;
    }

    m_loadingErrorPage = true;

    // NOTE: This could be used to display the error right in the page. However, I find
    // the error alert somehow better to manage. For example, on a partial load error,
    // at least some content stays readable if we don't overwrite it with the error text... :-)
//    BString content("<html><body>");
//    content << error.localizedDescription().utf8().data();
//    content << "</body></html>";
//
//    m_webFrame->SetFrameSource(content);
}

void FrameLoaderClientHaiku::dispatchDidFinishDocumentLoad()
{
    BMessage message(LOAD_DOC_COMPLETED);
    message.AddPointer("frame", m_webFrame);
    message.AddString("url", m_webFrame->Frame()->document()->url().string());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchDidFinishLoad()
{
    CALLED();
    if (m_loadingErrorPage) {
        m_loadingErrorPage = false;
        TRACE("m_loadingErrorPage\n");
        return;
    }

    BMessage message(LOAD_FINISHED);
    message.AddPointer("frame", m_webFrame);
    message.AddString("url", m_webFrame->Frame()->document()->url().string());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::dispatchWillSubmitForm(PassRefPtr<FormState>, FramePolicyFunction function)
{
    CALLED();
    notImplemented();
    // FIXME: Send an event to allow for alerts and cancellation.
    function(PolicyUse);
}

Frame* FrameLoaderClientHaiku::dispatchCreatePage(const NavigationAction& /*action*/)
{
    CALLED();
    WebCore::Page* page = m_webPage->createNewPage();
    if (page)
        return &page->mainFrame();

    return 0;
}

void FrameLoaderClientHaiku::dispatchShow()
{
    CALLED();
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchDecidePolicyForResponse(const WebCore::ResourceResponse& response, const WebCore::ResourceRequest& request, FramePolicyFunction function)
{
    if (request.isNull()) {
        function(PolicyIgnore);
        return;
    }
    // we need to call directly here
    if (!response.isAttachment() && canShowMIMEType(response.mimeType())) {
        function(PolicyUse);
    } else if (!request.url().isLocalFile() && response.mimeType() != "application/x-shockwave-flash") {
        function(PolicyDownload);
    } else {
        function(PolicyIgnore);
    }
}

void FrameLoaderClientHaiku::dispatchDecidePolicyForNewWindowAction(const NavigationAction& action,
    const ResourceRequest& request, PassRefPtr<FormState> /*formState*/, const String& /*targetName*/, FramePolicyFunction function)
{
    ASSERT(function);
    if (!function)
        return;

    if (request.isNull()) {
        function(PolicyIgnore);
        return;
    }

    if (!m_messenger.IsValid() || !isTertiaryMouseButton(action)) {
        dispatchNavigationRequested(request);
        function(PolicyUse);
        return;
    }

    // Clicks with the tertiary mouse button shall open a new window,
    // (or tab respectively depending on browser) - *ignore* the request for this page
    // then, since we create it ourself.
    BMessage message(NEW_WINDOW_REQUESTED);
    message.AddString("url", request.url().string());

    bool switchTab = false;

    // Switch to the new tab, when shift is pressed.
    if (action.event() && action.event()->isMouseEvent()) {
        const MouseEvent* mouseEvent = static_cast<const MouseEvent*>(action.event());
        switchTab = (mouseEvent && mouseEvent->shiftKey());
    }

    message.AddBool("primary", switchTab);
    dispatchMessage(message, true);

    if (action.type() == NavigationType::FormSubmitted || action.type() == NavigationType::FormResubmitted)
        m_webFrame->Frame()->loader().resetMultipleFormSubmissionProtection();

    if (action.type() == NavigationType::LinkClicked) {
        ResourceRequest emptyRequest;
        m_webFrame->Frame()->loader().activeDocumentLoader()->setLastCheckedRequest(emptyRequest);
    }

    function(PolicyIgnore);
}

void FrameLoaderClientHaiku::dispatchDecidePolicyForNavigationAction(const NavigationAction& action,
    const ResourceRequest& request, PassRefPtr<FormState> formState, FramePolicyFunction function)
{
    // Potentially we want to open a new window, when the user clicked with the
    // tertiary mouse button. That's why we can reuse the other method.
    dispatchDecidePolicyForNewWindowAction(action, request, formState, String(), function);
}

void FrameLoaderClientHaiku::cancelPolicyCheck()
{
    CALLED();
    notImplemented();
}

void FrameLoaderClientHaiku::dispatchUnableToImplementPolicy(const ResourceError&)
{
    CALLED();
    notImplemented();
}

void FrameLoaderClientHaiku::revertToProvisionalState(DocumentLoader*)
{
    CALLED();
    notImplemented();
}

void FrameLoaderClientHaiku::setMainDocumentError(WebCore::DocumentLoader* /*loader*/, const WebCore::ResourceError& error)
{
    CALLED();

    if (error.isCancellation())
        return;

    BMessage message(MAIN_DOCUMENT_ERROR);
    message.AddString("url", error.failingURL().string().utf8().data());
    message.AddString("error", error.localizedDescription());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::setMainFrameDocumentReady(bool)
{
    // this is only interesting once we provide an external API for the DOM
}

void FrameLoaderClientHaiku::startDownload(const ResourceRequest& request, const String& /*suggestedName*/)
{
    m_webPage->requestDownload(request);
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
    CALLED();

    ASSERT(loader->frame());
    loader->commitData(data, length);

#if 0
    Frame* coreFrame = loader->frame();
    if (coreFrame && coreFrame->document()->isMediaDocument())
        loader->cancelMainResourceLoad(coreFrame->loader().client().pluginWillHandleLoadError(loader->response()));
#endif
}

void FrameLoaderClientHaiku::finishedLoading(DocumentLoader* /*documentLoader*/)
{
    CALLED();
}

void FrameLoaderClientHaiku::updateGlobalHistory()
{
    WebCore::Frame* frame = m_webFrame->Frame();
    if (!frame)
        return;

    BMessage message(UPDATE_HISTORY);
    message.AddString("url", frame->loader().documentLoader()->urlForHistory().string());
    dispatchMessage(message);
}

void FrameLoaderClientHaiku::updateGlobalHistoryRedirectLinks()
{
    updateGlobalHistory();
}

bool FrameLoaderClientHaiku::shouldGoToHistoryItem(WebCore::HistoryItem*) const
{
    // FIXME: Should probably be refuse to go to the item when it contains
    // form data that has already been sent or something.
    return true;
}

void FrameLoaderClientHaiku::didDisplayInsecureContent()
{
}

void FrameLoaderClientHaiku::didRunInsecureContent(WebCore::SecurityOrigin*, const WebCore::URL&)
{
    notImplemented();
}

void FrameLoaderClientHaiku::didDetectXSS(const URL&, bool)
{
    notImplemented();
}

void FrameLoaderClientHaiku::convertMainResourceLoadToDownload(DocumentLoader*,
    WebCore::SessionID, const ResourceRequest& request, const ResourceResponse&)
{
    startDownload(request);
}

WebCore::ResourceError FrameLoaderClientHaiku::cancelledError(const WebCore::ResourceRequest& request)
{
    ResourceError error = ResourceError(String(), WebKitErrorCannotShowURL,
                                        request.url(), "Load request cancelled");
    error.setIsCancellation(true);
    return error;
}

WebCore::ResourceError FrameLoaderClientHaiku::blockedError(const ResourceRequest& request)
{
    return ResourceError(String(), WebKitErrorCannotUseRestrictedPort,
                         request.url(), "Not allowed to use restricted network port");
}

WebCore::ResourceError FrameLoaderClientHaiku::blockedByContentBlockerError(const ResourceRequest& request)
{
    return ResourceError(String(), WebKitErrorCannotShowURL,
        request.url(), "Blocked by content blocker");
}

WebCore::ResourceError FrameLoaderClientHaiku::cannotShowURLError(const WebCore::ResourceRequest& request)
{
    return ResourceError(String(), WebKitErrorCannotShowURL,
                         request.url(), "URL cannot be shown");
}

WebCore::ResourceError FrameLoaderClientHaiku::interruptedForPolicyChangeError(const WebCore::ResourceRequest& request)
{
    ResourceError error = ResourceError(String(), WebKitErrorFrameLoadInterruptedByPolicyChange,
                                        request.url(), "Frame load was interrupted");
    error.setIsCancellation(true);
    return error;
}

WebCore::ResourceError FrameLoaderClientHaiku::cannotShowMIMETypeError(const WebCore::ResourceResponse& response)
{
    // FIXME: This can probably be used to automatically close pages that have no content,
    // but only triggered a download. Since BWebPage is used for initiating a BWebDownload,
    // it could remember doing so and then we could ask here if we are the main frame,
    // have no content, but did download something -- then we could asked to be closed.
    return ResourceError(String(), WebKitErrorCannotShowMIMEType,
                         response.url(), "Content with the specified MIME type cannot be shown");
}

WebCore::ResourceError FrameLoaderClientHaiku::fileDoesNotExistError(const WebCore::ResourceResponse& response)
{
    return ResourceError(String(), WebKitErrorCannotShowURL,
                         response.url(), "File does not exist");
}

ResourceError FrameLoaderClientHaiku::pluginWillHandleLoadError(const ResourceResponse& response)
{
    return ResourceError(String(), WebKitErrorPlugInWillHandleLoad,
                         response.url(), "Plugin will handle load");
}

bool FrameLoaderClientHaiku::shouldFallBack(const WebCore::ResourceError& error)
{
    return !(error.isCancellation()
             || error.errorCode() == WebKitErrorFrameLoadInterruptedByPolicyChange
             || error.errorCode() == WebKitErrorPlugInWillHandleLoad);
}

bool FrameLoaderClientHaiku::canHandleRequest(const WebCore::ResourceRequest&) const
{
    // notImplemented();
    return true;
}

bool FrameLoaderClientHaiku::canShowMIMETypeAsHTML(const String& /*MIMEType*/) const
{
    notImplemented();
    return false;
}

bool FrameLoaderClientHaiku::canShowMIMEType(const String& mimeType) const
{
    CALLED("%s", mimeType.utf8().data());
    // FIXME: Usually, the mime type will have been detexted. This is supposed to work around
    // downloading some empty files, that can be observed.
    if (!mimeType.length())
        return true;

    if (MIMETypeRegistry::canShowMIMEType(mimeType))
        return true;

#if 0
    Frame* frame = m_webFrame->Frame();
    if (frame && frame->settings() && frame->settings()->arePluginsEnabled()
        && PluginDatabase::installedPlugins()->isMIMETypeRegistered(mimeType))
        return true;
#endif

    return false;
}

bool FrameLoaderClientHaiku::representationExistsForURLScheme(const String& /*URLScheme*/) const
{
    return false;
}

String FrameLoaderClientHaiku::generatedMIMETypeForURLScheme(const String& /*URLScheme*/) const
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
    // This seems unimportant, the Qt port mentions this for it's corresponding signal:
    //   "This signal is emitted when the load of \a frame is finished and the application
    //   may now update its state accordingly."
    // Could be this is important for ports which use actual platform widgets.
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
    // notImplemented(); // Nor does any port except Apple one.
}

WTF::Ref<DocumentLoader> FrameLoaderClientHaiku::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    CALLED("request: %s", request.url().string().utf8().data());
    return DocumentLoader::create(request, substituteData);
}

void FrameLoaderClientHaiku::setTitle(const StringWithDirection&, const URL&)
{
    // no need for, dispatchDidReceiveTitle is the right callback
}

void FrameLoaderClientHaiku::savePlatformDataToCachedFrame(CachedFrame* /*cachedPage*/)
{
    CALLED();
    // Nothing to be done here for the moment. We don't associate any platform data
}

void FrameLoaderClientHaiku::transitionToCommittedFromCachedFrame(CachedFrame* cachedFrame)
{
    CALLED();
    ASSERT(cachedFrame->view());

    // FIXME: I guess we would have to restore platform data from the cachedFrame here,
    // data associated in savePlatformDataToCachedFrame().

    cachedFrame->view()->setTopLevelPlatformWidget(m_webPage->WebView());
}

void FrameLoaderClientHaiku::transitionToCommittedForNewPage()
{
    CALLED();
    ASSERT(m_webFrame);

    Frame* frame = m_webFrame->Frame();

    assert(frame);

    BRect bounds = m_webPage->viewBounds();
    IntSize size = IntSize(bounds.IntegerWidth() + 1, bounds.IntegerHeight() + 1);

    bool transparent = m_webFrame->IsTransparent();
    Color backgroundColor = transparent ? WebCore::Color::transparent : WebCore::Color::white;

    frame->createView(size, backgroundColor, transparent);

    frame->view()->setTopLevelPlatformWidget(m_webPage->WebView());
}

String FrameLoaderClientHaiku::userAgent(const URL&)
{
    BLanguage language;
    BLocale::Default()->GetLanguage(&language);
    BString languageTag(language.Code());
    if (language.CountryCode() != NULL)
        languageTag << "-" << language.CountryCode();

    // FIXME: Get the app name from the app. Hardcoded WebPositive for now.
    // We have to look as close to Safari as possible for some sites like gmail.com 
    BString userAgent("Mozilla/5.0 (Macintosh; Intel Haiku R1 x86) AppleWebKit/%webkit% (KHTML, like Gecko) WebPositive/1.2 Version/9.2 Safari/%webkit%");
    userAgent.ReplaceAll("%webkit%", WebKitInfo::WebKitVersion().String());
    return userAgent;
}

bool FrameLoaderClientHaiku::canCachePage() const
{
    return true;
}

RefPtr<Frame> FrameLoaderClientHaiku::createFrame(const URL& url,
    const String& name, HTMLFrameOwnerElement* ownerElement,
    const String& referrer, bool /*allowsScrolling*/, int /*marginWidth*/,
    int /*marginHeight*/)
{
    // FIXME: We should apply the right property to the frameView.
    // (scrollbar,margins)
    ASSERT(m_webFrame);
    ASSERT(m_webPage);

    BWebFrame* subFrame = m_webFrame->AddChild(m_webPage, name, ownerElement);
    if (!subFrame)
        return nullptr;

    RefPtr<WebCore::Frame> coreSubFrame = subFrame->Frame();
    ASSERT(coreSubFrame);

    subFrame->SetListener(m_messenger);
    m_webFrame->Frame()->loader().loadURLIntoChildFrame(url, referrer, coreSubFrame.get());

    // The frame's onload handler may have removed it from the document.
    // See fast/dom/null-page-show-modal-dialog-crash.html for an example.
    if (!coreSubFrame->tree().parent()) {
        // The subframe will be deleted when coreSubFrame is dereferenced.
        return nullptr;
    }

    return coreSubFrame;
}

ObjectContentType FrameLoaderClientHaiku::objectContentType(const URL& url, const String& originalMimeType)
{
    CALLED();
    if (url.isEmpty() && !originalMimeType.length())
        return ObjectContentNone;

    String mimeType = originalMimeType;
    if (!mimeType.length()) {
        entry_ref ref;
        if (get_ref_for_path(url.path().utf8().data(), &ref) == B_OK) {
            BMimeType type;
            if (BMimeType::GuessMimeType(&ref, &type) == B_OK)
                mimeType = type.Type();
        } else {
            // For non-file URLs, try guessing from the extension (this happens
            // before the request so our content sniffing is of no use)
            mimeType = MIMETypeRegistry::getMIMETypeForExtension(url.path().substring(url.path().reverseFind('.') + 1));
        } 
    }

    if (!mimeType.length())
        return ObjectContentFrame;

    if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return ObjectContentImage;

#if 0
    if (PluginDatabase::installedPlugins()->isMIMETypeRegistered(mimeType))
        return ObjectContentNetscapePlugin;
#endif

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return ObjectContentFrame;

    if (url.protocol() == "about")
        return ObjectContentFrame;

    return ObjectContentNone;
}

RefPtr<Widget> FrameLoaderClientHaiku::createPlugin(const IntSize&, HTMLPlugInElement*, const URL&, const Vector<String>&,
                                                        const Vector<String>&, const String&, bool /*loadManually*/)
{
    CALLED();
    notImplemented();
    return nullptr;
}

void FrameLoaderClientHaiku::redirectDataToPlugin(Widget* pluginWidget)
{
    CALLED();
    debugger("plugins are not implemented on Haiku!");
}

PassRefPtr<Widget> FrameLoaderClientHaiku::createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const URL& /*baseURL*/,
                                                                  const Vector<String>& /*paramNames*/, const Vector<String>& /*paramValues*/)
{
    notImplemented();
    return 0;
}

String FrameLoaderClientHaiku::overrideMediaType() const
{
    // This will do, until we support printing.
    return "screen";
}

void FrameLoaderClientHaiku::didSaveToPageCache()
{
}

void FrameLoaderClientHaiku::didRestoreFromPageCache()
{
}

void FrameLoaderClientHaiku::dispatchDidBecomeFrameset(bool)
{
}

void FrameLoaderClientHaiku::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld& world)
{
    if (&world != &mainThreadNormalWorld())
        return;

    if (m_webFrame) {
        BMessage message(JAVASCRIPT_WINDOW_OBJECT_CLEARED);
        dispatchMessage(message);

    }

    if (m_webPage->fDumpRenderTree)
    {
        // DumpRenderTree registers the TestRunner JavaScript object using this
        // callback. This can't be done using the asynchronous BMessage above:
        // by the time the message is processed by the target, the JS test will
        // already have run!
        JSGlobalContextRef context = toGlobalRef(m_webFrame->Frame()->script().globalObject(mainThreadNormalWorld())->globalExec());
        JSObjectRef windowObject = toRef(m_webFrame->Frame()->script().globalObject(mainThreadNormalWorld()));
        m_webPage->fDumpRenderTree->didClearWindowObjectInWorld(world, context, windowObject);
    }

}

void FrameLoaderClientHaiku::registerForIconNotification(bool /*listen*/)
{
    notImplemented();
}

PassRefPtr<FrameNetworkingContext> FrameLoaderClientHaiku::createNetworkingContext()
{
    return FrameNetworkingContextHaiku::create(m_webFrame->Frame(), m_webPage->GetContext());
}

// #pragma mark - private

bool FrameLoaderClientHaiku::isTertiaryMouseButton(const NavigationAction& action) const
{
    if (action.event() && action.event()->isMouseEvent()) {
        const MouseEvent* mouseEvent = static_cast<const MouseEvent*>(action.event());
        return (mouseEvent && mouseEvent->button() == 1);
    }
    return false;
}

status_t FrameLoaderClientHaiku::dispatchNavigationRequested(const ResourceRequest& request) const
{
    BMessage message(NAVIGATION_REQUESTED);
    message.AddString("url", request.url().string());
    return dispatchMessage(message);
}

status_t FrameLoaderClientHaiku::dispatchMessage(BMessage& message, bool allowChildFrame) const
{
    message.AddPointer("view", m_webPage->WebView());
    message.AddPointer("frame", m_webFrame);

    // Most messages are only relevant when they come from the main frame
    // (setting the title, favicon, url, loading progress, etc). We intercept
    // the ones coming from child frames here.
    // Currently, the only exception is the message for navigation policy. This
    // allows opening a new tab by middle-clicking a link that's in a frame.
    if (allowChildFrame || m_webFrame == m_webPage->MainFrame())
        return m_messenger.SendMessage(&message);
    else
        return B_NOT_ALLOWED;
}

} // namespace WebCore

