/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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


#include "config.h"
#include "WebPage.h"

#include "AtomicString.h"
#include "Cache.h"
#include "ChromeClientHaiku.h"
#include "ContextMenuClientHaiku.h"
#include "Cursor.h"
#include "DOMTimer.h"
#include "DragClientHaiku.h"
#include "EditorClientHaiku.h"
#include "FileChooser.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClientHaiku.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "InitializeThreading.h"
#include "InspectorClientHaiku.h"
#include "Logging.h"
#include "Page.h"
#include "PageCache.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformString.h"
#include "PlatformWheelEvent.h"
#include "ResourceHandle.h"
#include "Settings.h"
#include "TextEncoding.h"
#include "WebDownload.h"
#include "WebDownloadPrivate.h"
#include "WebFrame.h"
#include "WebFramePrivate.h"
#include "WebView.h"
#include "WebViewConstants.h"

#include <Bitmap.h>
#include <Entry.h>
#include <Font.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <Region.h>
#include <Window.h>

#include <wtf/Assertions.h>
#include <wtf/Threading.h>

/*
     The basic idea here is to dispatch all public methods to the BLooper
     to which the handler is attached (should be the be_app), such that
     the calls into WebCore code happen from within that thread *only*.
     In current WebCore with pthread threading backend, this must be the
     same thread that called WTF::initializeThreading(), respectively the
     initializeOnce() method of this class.
 */

enum {
    HANDLE_SHUTDOWN = 'sdwn',

    HANDLE_LOAD_URL = 'lurl',
    HANDLE_RELOAD = 'reld',
    HANDLE_GO_BACK = 'back',
    HANDLE_GO_FORWARD = 'fwrd',
    HANDLE_STOP_LOADING = 'stop',

    HANDLE_FOCUSED = 'focs',
    HANDLE_ACTIVATED = 'actd',

    HANDLE_SET_VISIBLE = 'vsbl',
    HANDLE_DRAW = 'draw',
    HANDLE_FRAME_RESIZED = 'rszd',

    HANDLE_CHANGE_TEXT_SIZE = 'txts',
    HANDLE_FIND_STRING = 'find',

    HANDLE_RESEND_NOTIFICATIONS = 'rsnt'
};

using namespace WebCore;

/*static*/ void BWebPage::InitializeOnce()
{
	// NOTE: This needs to be called when the BApplication is ready.
	// It won't work as static initialization.
    WebCore::InitializeLoggingChannelsIfNecessary();
    JSC::initializeThreading();
    WTF::initializeThreading();
    WebCore::AtomicString::init();
    WebCore::DOMTimer::setMinTimerInterval(0.008);
    WebCore::UTF8Encoding();
    WebCore::initPlatformCursors();
}

/*static*/ void BWebPage::SetCacheModel(BWebKitCacheModel model)
{
    // FIXME: Add disk cache handling when CURL has the API
    uint32 cacheTotalCapacity;
    uint32 cacheMinDeadCapacity;
    uint32 cacheMaxDeadCapacity;
    double deadDecodedDataDeletionInterval;
    uint32 pageCacheCapacity;

    switch (model) {
    case B_WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER:
        pageCacheCapacity = 0;
        cacheTotalCapacity = 0;
        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;
        deadDecodedDataDeletionInterval = 0;
        break;
    case B_WEBKIT_CACHE_MODEL_WEB_BROWSER:
        pageCacheCapacity = 3;
        cacheTotalCapacity = 32 * 1024 * 1024;
        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;
        deadDecodedDataDeletionInterval = 60;
        break;
    default:
        return;
    }

    cache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    cache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);
}

BWebPage::BWebPage(BWebView* webView)
    : BHandler("BWebPage")
    , m_webView(webView)
    , m_mainFrame(0)
    , m_page(0)
    , m_pageVisible(true)
    , m_pageDirty(false)
    , m_toolbarsVisible(true)
    , m_statusbarVisible(true)
    , m_menubarVisible(true)
{
    m_page = new WebCore::Page(new WebCore::ChromeClientHaiku(this, webView),
                               new WebCore::ContextMenuClientHaiku(),
                               new WebCore::EditorClientHaiku(this),
                               new WebCore::DragClientHaiku(webView),
                               new WebCore::InspectorClientHaiku(),
                               0,
                               0);

    // Default settings - We should have WebViewSettings class for this.
    WebCore::Settings* settings = m_page->settings();
    settings->setLoadsImagesAutomatically(true);
    settings->setMinimumFontSize(5);
    settings->setMinimumLogicalFontSize(5);
    settings->setShouldPrintBackgrounds(true);
    settings->setJavaScriptEnabled(true);
    settings->setShowsURLsInToolTips(true);
    settings->setShouldPaintCustomScrollbars(true);
    settings->setEditingBehavior(EditingMacBehavior);
//    settings->setLocalStorageEnabled(true);
//    settings->setLocalStorageDatabasePath();

    settings->setDefaultFixedFontSize(14);
    settings->setDefaultFontSize(14);

    // TODO: Init from system fonts or application settings.
    settings->setSerifFontFamily("DejaVu Serif");
    settings->setSansSerifFontFamily("DejaVu Sans");
    settings->setFixedFontFamily("DejaVu Sans Mono");
    settings->setStandardFontFamily("DejaVu Serif");
    settings->setDefaultTextEncodingName("UTF-8");
}

BWebPage::~BWebPage()
{
    delete m_mainFrame;
    delete m_page;
}

// #pragma mark - public

void BWebPage::Init()
{
	WebFramePrivate* data = new WebFramePrivate;
	data->page = m_page;

    m_mainFrame = new BWebFrame(this, 0, data);
}

void BWebPage::Shutdown()
{
    BMessage message(HANDLE_SHUTDOWN);
    Looper()->PostMessage(&message, this);
}

void BWebPage::SetListener(const BMessenger& listener)
{
	m_listener = listener;
    m_mainFrame->SetListener(listener);
}

void BWebPage::SetDownloadListener(const BMessenger& listener)
{
    m_downloadListener = listener;
}

void BWebPage::LoadURL(const char* urlString)
{
    BMessage message(HANDLE_LOAD_URL);
    message.AddString("url", urlString);
    Looper()->PostMessage(&message, this);
}

void BWebPage::Reload()
{
    BMessage message(HANDLE_RELOAD);
    Looper()->PostMessage(&message, this);
}

void BWebPage::GoBack()
{
    BMessage message(HANDLE_GO_BACK);
    Looper()->PostMessage(&message, this);
}

void BWebPage::GoForward()
{
    BMessage message(HANDLE_GO_FORWARD);
    Looper()->PostMessage(&message, this);
}

void BWebPage::StopLoading()
{
    BMessage message(HANDLE_STOP_LOADING);
    Looper()->PostMessage(&message, this);
}

void BWebPage::ChangeTextSize(float increment)
{
	BMessage message(HANDLE_CHANGE_TEXT_SIZE);
	message.AddFloat("increment", increment);
    Looper()->PostMessage(&message, this);
}

void BWebPage::FindString(const char* string, bool forward, bool caseSensitive,
    bool wrapSelection, bool startInSelection)
{
	BMessage message(HANDLE_FIND_STRING);
	message.AddString("string", string);
	message.AddBool("forward", forward);
	message.AddBool("case sensitive", caseSensitive);
	message.AddBool("wrap selection", wrapSelection);
	message.AddBool("start in selection", startInSelection);
    Looper()->PostMessage(&message, this);
}

void BWebPage::ResendNotifications()
{
    BMessage message(HANDLE_RESEND_NOTIFICATIONS);
    Looper()->PostMessage(&message, this);
}

BWebFrame* BWebPage::MainFrame() const
{
    return m_mainFrame;
};

BWebView* BWebPage::WebView() const
{
    return m_webView;
}

BString BWebPage::MainFrameTitle() const
{
    return m_mainFrame->Title();
}

BString BWebPage::MainFrameRequestedURL() const
{
    return m_mainFrame->RequestedURL();
}

BString BWebPage::MainFrameURL() const
{
    return m_mainFrame->URL();
}

// #pragma mark - BWebView API

void BWebPage::setVisible(bool visible)
{
    BMessage message(HANDLE_SET_VISIBLE);
    message.AddBool("visible", visible);
    Looper()->PostMessage(&message, this);
}

void BWebPage::draw(const BRect& updateRect)
{
    BMessage message(HANDLE_DRAW);
    message.AddPointer("target", this);
    message.AddRect("update rect", updateRect);
    Looper()->PostMessage(&message, this);
}

void BWebPage::frameResized(float width, float height)
{
    BMessage message(HANDLE_FRAME_RESIZED);
    message.AddPointer("target", this);
    message.AddFloat("width", width);
    message.AddFloat("height", height);
    Looper()->PostMessage(&message, this);
}

void BWebPage::focused(bool focused)
{
    BMessage message(HANDLE_FOCUSED);
    message.AddBool("focused", focused);
    Looper()->PostMessage(&message, this);
}

void BWebPage::activated(bool activated)
{
    BMessage message(HANDLE_ACTIVATED);
    message.AddBool("activated", activated);
    Looper()->PostMessage(&message, this);
}

void BWebPage::mouseEvent(const BMessage* message,
    const BPoint& where, const BPoint& screenWhere)
{
    BMessage copiedMessage(*message);
    copiedMessage.AddPointer("target", this);
    Looper()->PostMessage(&copiedMessage, this);
}

void BWebPage::mouseWheelChanged(const BMessage* message,
    const BPoint& where, const BPoint& screenWhere)
{
    BMessage copiedMessage(*message);
    copiedMessage.AddPoint("be:view_where", where);
    copiedMessage.AddPoint("screen_where", screenWhere);
    copiedMessage.AddInt32("modifiers", modifiers());
    Looper()->PostMessage(&copiedMessage, this);
}

void BWebPage::keyEvent(const BMessage* message)
{
    BMessage copiedMessage(*message);
    Looper()->PostMessage(&copiedMessage, this);
}

void BWebPage::standardShortcut(const BMessage* message)
{
	// Simulate a B_KEY_DOWN event. The message is not complete,
	// but enough to trigger short cut generation in EditorClientHaiku.
    const char* bytes = 0;
    switch (message->what) {
    case B_SELECT_ALL:
        bytes = "a";
       break;
    case B_CUT:
        bytes = "x";
        break;
    case B_COPY:
        bytes = "c";
        break;
    case B_PASTE:
        bytes = "v";
        break;
    case B_UNDO:
        bytes = "z";
        break;
    case B_REDO:
        bytes = "Z";
        break;
    }
    BMessage keyDownMessage(B_KEY_DOWN);
    keyDownMessage.AddInt32("modifiers", modifiers() | B_COMMAND_KEY);
    keyDownMessage.AddString("bytes", bytes);
    keyDownMessage.AddInt64("when", system_time());
    Looper()->PostMessage(&keyDownMessage, this);
}

// #pragma mark - WebCoreSupport methods

WebCore::Page* BWebPage::page() const
{
    return m_page;
}

BRect BWebPage::windowBounds()
{
    BRect bounds;
    if (m_webView->LockLooper()) {
        bounds = m_webView->Window()->Bounds();
        m_webView->UnlockLooper();
    }
    return bounds;
}

void BWebPage::setWindowBounds(const BRect& bounds)
{
    if (m_webView->LockLooper()) {
        m_webView->Window()->MoveTo(bounds.LeftTop());
        m_webView->Window()->ResizeTo(bounds.Width(), bounds.Height());
        m_webView->UnlockLooper();
    }
}

BRect BWebPage::viewBounds()
{
    BRect bounds;
    if (m_webView->LockLooper()) {
        bounds = m_webView->Bounds();
        m_webView->UnlockLooper();
    }
    return bounds;
}

void BWebPage::setViewBounds(const BRect& bounds)
{
    if (m_webView->LockLooper()) {
        // TODO: Implement this with layout management, i.e. SetExplicitMinSize() or something...
        m_webView->UnlockLooper();
    }
}

void BWebPage::setToolbarsVisible(bool flag)
{
    m_toolbarsVisible = flag;

    BMessage message(TOOLBARS_VISIBILITY);
    message.AddBool("flag", flag);
    dispatchMessage(message);
}

void BWebPage::setStatusbarVisible(bool flag)
{
    m_statusbarVisible = flag;

    BMessage message(STATUSBAR_VISIBILITY);
    message.AddBool("flag", flag);
    dispatchMessage(message);
}

void BWebPage::setMenubarVisible(bool flag)
{
    m_menubarVisible = flag;

    BMessage message(MENUBAR_VISIBILITY);
    message.AddBool("flag", flag);
    dispatchMessage(message);
}

void BWebPage::setResizable(bool flag)
{
    BMessage message(SET_RESIZABLE);
    message.AddBool("flag", flag);
    dispatchMessage(message);
}

void BWebPage::closeWindow()
{
	BMessage message(B_QUIT_REQUESTED);
    dispatchMessage(message);
}

void BWebPage::setStatusText(const BString& text)
{
    BMessage message(SET_STATUS_TEXT);
    message.AddString("text", text);
    dispatchMessage(message);
}

void BWebPage::linkHovered(const BString& url, const BString& title, const BString& content)
{
printf("BWebPage::linkHovered()\n");
}

void BWebPage::requestDownload(const WebCore::ResourceRequest& request)
{
    BWebDownload* download = new BWebDownload(new BPrivate::WebDownloadPrivate(request));
    downloadCreated(download);
}

void BWebPage::requestDownload(WebCore::ResourceHandle* handle,
    const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response)
{
    BWebDownload* download = new BWebDownload(new BPrivate::WebDownloadPrivate(handle, request, response));
    downloadCreated(download);
}

void BWebPage::downloadCreated(BWebDownload* download)
{
	download->Start();
	if (m_downloadListener.IsValid()) {
        BMessage message(B_DOWNLOAD_ADDED);
        message.AddPointer("download", download);
        // Block until the listener has pulled all the information...
        BMessage reply;
        m_downloadListener.SendMessage(&message, &reply);
	}
}

void BWebPage::paint(BRect rect, bool contentChanged, bool immediate,
    bool repaintContentOnly)
{
    // Block any drawing as long as the BWebView is hidden
    // (should be extended to when the containing BWebWindow is not
    // currently on screen either...)
    if (!m_pageVisible) {
        m_pageDirty = true;
        return;
    }

    // NOTE: m_mainFrame can be 0 because init() eventually ends up calling
    // paint()! BWebFrame seems to cause an initial page to be loaded, maybe
    // this ought to be avoided also for start-up speed reasons!
    if (!m_mainFrame)
        return;
    WebCore::Frame* frame = m_mainFrame->Frame();
    WebCore::FrameView* view = frame->view();

    if (!view || !frame->contentRenderer())
        return;

//    if (m_webView->LockLooperWithTimeout(5000) != B_OK)
    if (!m_webView->LockLooper())
        return;
    BView* offscreenView = m_webView->OffscreenView();

    // Lock the offscreen bitmap while we still have the
    // window locked. This cannot deadlock and makes sure
    // the window is not deleting the offscreen view right
    // after we unlock it and before locking the bitmap.
    offscreenView->LockLooper();
    m_webView->UnlockLooper();

    WebCore::GraphicsContext context(offscreenView);

    view->layoutIfNeededRecursive();
    offscreenView->PushState();
    if (!rect.IsValid())
        rect = offscreenView->Bounds();
    BRegion region(rect);
    offscreenView->ConstrainClippingRegion(&region);
    view->paint(&context, IntRect(rect));
    offscreenView->PopState();
    offscreenView->UnlockLooper();

    m_pageDirty = false;

    // Notify the window that it can now pull the bitmap in its own thread
    m_webView->SetOffscreenViewClean(rect, immediate);
}

// #pragma mark - private

void BWebPage::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case HANDLE_SHUTDOWN:
        // NOTE: This message never arrives here when the BApplication is already
        // processing B_QUIT_REQUESTED. Then the view will be detached and instruct
        // the BWebPage handler to shut itself down, but BApplication will not
        // process additional messages. That's why the windows containing WebViews
        // are detaching the views already in their QuitRequested() hooks and
        // LauncherApp calls these hooks already in its own QuitRequested() hook.
        Looper()->RemoveHandler(this);
        delete this;
        // TOAST!
        return;
    case HANDLE_LOAD_URL:
        handleLoadURL(message);
        break;
    case HANDLE_RELOAD:
    	handleReload(message);
    	break;
    case HANDLE_GO_BACK:
        handleGoBack(message);
        break;
    case HANDLE_GO_FORWARD:
        handleGoForward(message);
        break;
    case HANDLE_STOP_LOADING:
        handleStop(message);
        break;

    case HANDLE_SET_VISIBLE:
        handleSetVisible(message);
        break;

    case HANDLE_DRAW: {
        bool first = true;
        BMessageQueue* queue = Looper()->MessageQueue();
        BRect updateRect;
        message->FindRect("update rect", &updateRect);
        int32 index = 0;
        while (BMessage* nextMessage = queue->FindMessage(message->what, index)) {
            BHandler* target = 0;
            nextMessage->FindPointer("target", reinterpret_cast<void**>(&target));
            if (target != this) {
                index++;
                continue;
            }

            if (!first) {
                delete message;
                first = false;
            }
            
            message = nextMessage;
            queue->RemoveMessage(message);

            BRect rect;
            message->FindRect("update rect", &rect);
            updateRect = updateRect | rect;
        }
        paint(updateRect, true, false, true);
        break;
    }
    case HANDLE_FRAME_RESIZED:
        skipToLastMessage(message);
        handleFrameResized(message);
        break;

    case HANDLE_FOCUSED:
        handleFocused(message);
        break;
    case HANDLE_ACTIVATED:
        handleActivated(message);
        break;

    case B_MOUSE_MOVED:
        skipToLastMessage(message);
    case B_MOUSE_DOWN:
    case B_MOUSE_UP:
        handleMouseEvent(message);
        break;
    case B_MOUSE_WHEEL_CHANGED:
        handleMouseWheelChanged(message);
        break;
    case B_KEY_DOWN:
    case B_KEY_UP:
        handleKeyEvent(message);
        break;

	case HANDLE_CHANGE_TEXT_SIZE:
		handleChangeTextSize(message);
		break;
    case HANDLE_FIND_STRING:
        handleFindString(message);
        break;

    case HANDLE_RESEND_NOTIFICATIONS:
        handleResendNotifications(message);
        break;

    case B_REFS_RECEIVED: {
		RefPtr<FileChooser>* chooser;
        if (message->FindPointer("chooser", reinterpret_cast<void**>(&chooser)) == B_OK) {
            type_code type;
            int32 count = 0;
            entry_ref ref;
            BPath path;
            message->GetInfo("refs", &type, &count);
            Vector<String> filenames;
            for (int32 i = 0; i < count; i++) {
                message->FindRef("refs", i, &ref);
                path.SetTo(&ref);
                filenames.append(String(path.Path()));
            }
            (*chooser)->chooseFiles(filenames);
            delete chooser;
        }
    	break;
    }

    default:
        BHandler::MessageReceived(message);
    }
}

void BWebPage::skipToLastMessage(BMessage*& message)
{
	// NOTE: All messages that are fast-forwarded like this
	// need to be flagged with the intended target BWebPage,
	// or else we steal or process messages intended for another
	// BWebPage here!
    bool first = true;
    BMessageQueue* queue = Looper()->MessageQueue();
    int32 index = 0;
    while (BMessage* nextMessage = queue->FindMessage(message->what, index)) {
    	BHandler* target = 0;
    	nextMessage->FindPointer("target", reinterpret_cast<void**>(&target));
    	if (target != this) {
    		index++;
    	    continue;
    	}
        if (!first)
            delete message;
        message = nextMessage;
        queue->RemoveMessage(message);
        first = false;
    }
}

void BWebPage::handleLoadURL(const BMessage* message)
{
    const char* urlString;
    if (message->FindString("url", &urlString) != B_OK)
        return;

    m_mainFrame->LoadURL(urlString);
}

void BWebPage::handleReload(const BMessage* message)
{
    m_mainFrame->Reload();
}

void BWebPage::handleGoBack(const BMessage* message)
{
    m_page->goBack();
}

void BWebPage::handleGoForward(const BMessage* message)
{
    m_page->goForward();
}

void BWebPage::handleStop(const BMessage* message)
{
    m_mainFrame->StopLoading();
}

void BWebPage::handleSetVisible(const BMessage* message)
{
    message->FindBool("visible", &m_pageVisible);
    if (m_mainFrame->Frame()->view())
        m_mainFrame->Frame()->view()->setParentVisible(m_pageVisible);
    // Trigger an internal repaint if the page was supposed to be repainted
    // while it was invisible.
    if (m_pageVisible && m_pageDirty)
        paint(BRect(), false, false, true);
}

void BWebPage::handleDraw(const BMessage* message)
{
    BRect rect;
    message->FindRect("update rect", &rect);
    paint(rect, true, false, true);
}

void BWebPage::handleFrameResized(const BMessage* message)
{
    float width;
    float height;
    message->FindFloat("width", &width);
    message->FindFloat("height", &height);

    WebCore::Frame* frame = m_mainFrame->Frame();
    frame->view()->resize(width + 1, height + 1);
}

void BWebPage::handleFocused(const BMessage* message)
{
    bool focused;
    message->FindBool("focused", &focused);

    FocusController* focusController = m_page->focusController();
    focusController->setFocused(focused);
    if (focused && !focusController->focusedFrame())
        focusController->setFocusedFrame(m_mainFrame->Frame());
}

void BWebPage::handleActivated(const BMessage* message)
{
    bool activated;
    message->FindBool("activated", &activated);

    FocusController* focusController = m_page->focusController();
    focusController->setActive(activated);
}

void BWebPage::handleMouseEvent(const BMessage* message)
{
    WebCore::Frame* frame = m_mainFrame->Frame();
    if (!frame->view() || !frame->document())
        return;

    PlatformMouseEvent event(message);
    switch (message->what) {
    case B_MOUSE_DOWN:
        frame->eventHandler()->handleMousePressEvent(event);
        break;
    case B_MOUSE_UP:
        frame->eventHandler()->handleMouseReleaseEvent(event);
        break;
    case B_MOUSE_MOVED:
    default:
        frame->eventHandler()->handleMouseMoveEvent(event);
        break;
    }
}

void BWebPage::handleMouseWheelChanged(BMessage* message)
{
    WebCore::Frame* frame = m_mainFrame->Frame();
    if (!frame->view() || !frame->document())
        return;

    PlatformWheelEvent event(message);
    frame->eventHandler()->handleWheelEvent(event);
}

void BWebPage::handleKeyEvent(BMessage* message)
{
    WebCore::Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame->view() || !frame->document())
        return;

    PlatformKeyboardEvent event(message);
    frame->eventHandler()->keyEvent(event);
}

void BWebPage::handleChangeTextSize(BMessage* message)
{
    float increment = 0;
    message->FindFloat("increment", &increment);
    if (increment > 0)
    	m_mainFrame->IncreaseTextSize();
    else if (increment < 0)
    	m_mainFrame->DecreaseTextSize();
    else
    	m_mainFrame->ResetTextSize();
}

void BWebPage::handleFindString(BMessage* message)
{
    BMessage reply(B_FIND_STRING_RESULT);

    const char* string;
    bool forward;
    bool caseSensitive;
    bool wrapSelection;
    bool startInSelection;
    if (message->FindString("string", &string) != B_OK
        || message->FindBool("forward", &forward) != B_OK
        || message->FindBool("case sensitive", &caseSensitive) != B_OK
        || message->FindBool("wrap selection", &wrapSelection) != B_OK
        || message->FindBool("start in selection", &startInSelection) != B_OK) {
        message->SendReply(&reply);
    }

    bool result = m_mainFrame->FindString(string, forward, caseSensitive,
        wrapSelection, startInSelection);

    reply.AddBool("result", result);
    message->SendReply(&reply);
}

void BWebPage::handleResendNotifications(BMessage*)
{
	// Prepare navigation capabilities notification
    BMessage message(UPDATE_NAVIGATION_INTERFACE);
    message.AddBool("can go backward", m_page->canGoBackOrForward(-1));
    message.AddBool("can go forward", m_page->canGoBackOrForward(1));
    if (WebCore::FrameLoader* loader = m_mainFrame->Frame()->loader())
        message.AddBool("can stop", loader->isLoading());
    dispatchMessage(message);
    // TODO: Other notifications...
}

// #pragma mark -

status_t BWebPage::dispatchMessage(BMessage& message) const
{
	message.AddPointer("view", m_webView);
	return m_listener.SendMessage(&message);
}

