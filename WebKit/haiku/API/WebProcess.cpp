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
#include "WebProcess.h"

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
#include "NotImplemented.h"
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
#include "WebFrame.h"
#include "WebView.h"
#include "WebViewConstants.h"

#include <Bitmap.h>
#include <Entry.h>
#include <Font.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
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
    HANDLE_GO_BACK = 'back',
    HANDLE_GO_FORWARD = 'fwrd',

    HANDLE_FOCUSED = 'focs',
    HANDLE_ACTIVATED = 'actd',

    HANDLE_DRAW = 'draw',

    HANDLE_FRAME_RESIZED = 'rszd',

    HANDLE_CHANGE_TEXT_SIZE = 'txts',
    HANDLE_FIND_STRING = 'find'
};

using namespace WebCore;

/*static*/ void WebProcess::initializeOnce()
{
	// NOTE: This needs to be called when the BApplication is ready.
	// It won't work as static initialization.
    WebCore::InitializeLoggingChannelsIfNecessary();
    JSC::initializeThreading();
    WTF::initializeThreading();
    WebCore::AtomicString::init();
    WebCore::DOMTimer::setMinTimerInterval(0.004);
    WebCore::UTF8Encoding();
    WebCore::initPlatformCursors();
}

/*static*/ void WebProcess::setCacheModel(WebKitCacheModel model)
{
    // FIXME: Add disk cache handling when CURL has the API
    uint32 cacheTotalCapacity;
    uint32 cacheMinDeadCapacity;
    uint32 cacheMaxDeadCapacity;
    double deadDecodedDataDeletionInterval;
    uint32 pageCacheCapacity;

    switch (model) {
    case WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER:
        pageCacheCapacity = 0;
        cacheTotalCapacity = 0;
        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;
        deadDecodedDataDeletionInterval = 0;
        break;
    case WEBKIT_CACHE_MODEL_WEB_BROWSER:
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

WebProcess::WebProcess(WebView* webView)
    : BHandler("WebView process")
    , m_webView(webView)
    , m_mainFrame(0)
    , m_page(0)
{
    WebCore::EditorClientHaiku* editorClient = new WebCore::EditorClientHaiku();

    m_page = new WebCore::Page(new WebCore::ChromeClientHaiku(this, webView),
                               new WebCore::ContextMenuClientHaiku(),
                               editorClient,
                               new WebCore::DragClientHaiku(webView),
                               new WebCore::InspectorClientHaiku(),
                               0,
                               0);

    editorClient->setPage(m_page);

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

WebProcess::~WebProcess()
{
printf("~WebProcess()\n");
    delete m_mainFrame;
    delete m_page;
}

// #pragma mark - public

void WebProcess::init()
{
    m_mainFrame = new WebFrame(this, m_page);
}

void WebProcess::shutdown()
{
    BMessage message(HANDLE_SHUTDOWN);
    Looper()->PostMessage(&message, this);
}

void WebProcess::setDispatchTarget(const BMessenger& messenger)
{
    m_mainFrame->setDispatchTarget(messenger);
}

void WebProcess::loadURL(const char* urlString)
{
    BMessage message(HANDLE_LOAD_URL);
    message.AddString("url", urlString);
    Looper()->PostMessage(&message, this);
}

void WebProcess::goBack()
{
    BMessage message(HANDLE_GO_BACK);
    Looper()->PostMessage(&message, this);
}

void WebProcess::goForward()
{
    BMessage message(HANDLE_GO_FORWARD);
    Looper()->PostMessage(&message, this);
}

void WebProcess::draw(const BRect& updateRect)
{
    BMessage message(HANDLE_DRAW);
    message.AddPointer("target", this);
    message.AddRect("update rect", updateRect);
    Looper()->PostMessage(&message, this);
}

void WebProcess::frameResized(float width, float height)
{
    BMessage message(HANDLE_FRAME_RESIZED);
    message.AddPointer("target", this);
    message.AddFloat("width", width);
    message.AddFloat("height", height);
    Looper()->PostMessage(&message, this);
}

void WebProcess::focused(bool focused)
{
    BMessage message(HANDLE_FOCUSED);
    message.AddBool("focused", focused);
    Looper()->PostMessage(&message, this);
}

void WebProcess::activated(bool activated)
{
    BMessage message(HANDLE_ACTIVATED);
    message.AddBool("activated", activated);
    Looper()->PostMessage(&message, this);
}

void WebProcess::mouseEvent(const BMessage* message,
    const BPoint& where, const BPoint& screenWhere)
{
    BMessage copiedMessage(*message);
    copiedMessage.AddPointer("target", this);
    Looper()->PostMessage(&copiedMessage, this);
}

void WebProcess::mouseWheelChanged(const BMessage* message,
    const BPoint& where, const BPoint& screenWhere)
{
    BMessage copiedMessage(*message);
    copiedMessage.AddPoint("be:view_where", where);
    copiedMessage.AddPoint("screen_where", screenWhere);
    copiedMessage.AddInt32("modifiers", modifiers());
    Looper()->PostMessage(&copiedMessage, this);
}

void WebProcess::keyEvent(const BMessage* message)
{
    BMessage copiedMessage(*message);
    Looper()->PostMessage(&copiedMessage, this);
}

void WebProcess::standardShortcut(const BMessage* message)
{
	// Simulate a B_KEY_DOWN event. The message is not complete,
	// but enough to trigger short cut generation in EditorClientHaiku.
    BMessage keyDownMessage(B_KEY_DOWN);
    keyDownMessage.AddInt32("modifiers", modifiers() | B_COMMAND_KEY);
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
    keyDownMessage.AddString("bytes", bytes);
    keyDownMessage.AddInt64("when", system_time());
    Looper()->PostMessage(&keyDownMessage, this);
}

void WebProcess::changeTextSize(float increment)
{
	BMessage message(HANDLE_CHANGE_TEXT_SIZE);
	message.AddFloat("increment", increment);
    Looper()->PostMessage(&message, this);
}

void WebProcess::findString(const char* string, bool forward, bool caseSensitive,
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

void WebProcess::setDownloadListener(const BMessenger& listener)
{
    m_downloadListener = listener;
}

// #pragma mark - WebCoreSupport methods

WebFrame* WebProcess::mainFrame() const
{
    return m_mainFrame;
};

WebCore::Page* WebProcess::page() const
{
    return m_page;
}

WebView* WebProcess::webView() const
{
    return m_webView;
}

BRect WebProcess::contentsSize()
{
	IntSize viewSize = m_mainFrame->frame()->view()->contentsSize();
    return BRect(B_ORIGIN, BPoint(viewSize.width() - 1, viewSize.height() - 1));
}

BRect WebProcess::windowBounds()
{
    BRect bounds;
    if (m_webView->LockLooper()) {
        bounds = m_webView->Window()->Bounds();
        m_webView->UnlockLooper();
    }
    return bounds;
}

void WebProcess::setWindowBounds(const BRect& bounds)
{
    if (m_webView->LockLooper()) {
        m_webView->Window()->MoveTo(bounds.LeftTop());
        m_webView->Window()->ResizeTo(bounds.Width(), bounds.Height());
        m_webView->UnlockLooper();
    }
}

BRect WebProcess::viewBounds()
{
    BRect bounds;
    if (m_webView->LockLooper()) {
        bounds = m_webView->Bounds();
        m_webView->UnlockLooper();
    }
    return bounds;
}

void WebProcess::setViewBounds(const BRect& bounds)
{
    if (m_webView->LockLooper()) {
        // TODO: Implement this with layout management, i.e. SetExplicitMinSize() or something...
        m_webView->UnlockLooper();
    }
}

BString WebProcess::mainFrameTitle()
{
    return m_mainFrame->title();
}

BString WebProcess::mainFrameURL()
{
    return m_mainFrame->url();
}

void WebProcess::requestDownload(const WebCore::ResourceRequest& request)
{
    WebDownload* download = new WebDownload(this, request);
    downloadCreated(download);
}

void WebProcess::requestDownload(WebCore::ResourceHandle* handle,
    const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response)
{
    WebDownload* download = new WebDownload(this, handle, request, response);
    downloadCreated(download);
}

void WebProcess::downloadCreated(WebDownload* download)
{
	download->start();
	if (m_downloadListener.IsValid()) {
        BMessage message(DOWNLOAD_ADDED);
        message.AddPointer("download", download);
        // Block until the listener has pulled all the information...
        BMessage reply;
        m_downloadListener.SendMessage(&message, &reply);
	}
}

void WebProcess::downloadFinished(WebCore::ResourceHandle* handle,
    WebDownload* download, uint32 status)
{
	handle->setClient(0);
	if (m_downloadListener.IsValid()) {
        BMessage message(DOWNLOAD_REMOVED);
        message.AddPointer("download", download);
        // Block until the listener has released the object on it's side...
        BMessage reply;
        m_downloadListener.SendMessage(&message, &reply);
	}
	delete download;
}

void WebProcess::paint(const BRect& rect, bool contentChanged, bool immediate,
    bool repaintContentOnly)
{
    // NOTE: m_mainFrame can be 0 because init() eventually ends up calling
    // paint()! WebFrame seems to cause an initial page to be loaded, maybe
    // this ought to be avoided also for start-up speed reasons!
    if (!m_mainFrame)
        return;
    WebCore::Frame* frame = m_mainFrame->frame();
    WebCore::FrameView* view = frame->view();

    if (!view || !frame->contentRenderer())
        return;

//    if (m_webView->LockLooperWithTimeout(5000) != B_OK)
    if (!m_webView->LockLooper())
        return;
    BView* offscreenView = m_webView->offscreenView();

    // Lock the offscreen bitmap while we still have the
    // window locked. This cannot deadlock and makes sure
    // the window is not deleting the offscreen view right
    // after we unlock it and before locking the bitmap.
    offscreenView->LockLooper();
    m_webView->UnlockLooper();

    WebCore::GraphicsContext context(offscreenView);

    view->layoutIfNeededRecursive();
    offscreenView->PushState();
    view->paint(&context, IntRect(rect));
    offscreenView->PopState();
    offscreenView->UnlockLooper();

    // Notify the window that it can now pull the bitmap in its own thread
    m_webView->setOffscreenViewClean(rect, immediate);
}

// #pragma mark - private

void WebProcess::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case HANDLE_SHUTDOWN:
        // TODO: This message never arrives here when the BApplication is already
        // processing B_QUIT_REQUESTED. Then the view will be detached and instruct
        // the WebProcess handler to shut itself down, but BApplication will not
        // process additional messages. In that regard, it's hard to perform the
        // shutdown in the main thread...
        Looper()->RemoveHandler(this);
        delete this;
        // TOAST!
        return;
    case HANDLE_LOAD_URL:
        handleLoadURL(message);
        break;
    case HANDLE_GO_BACK:
        handleGoBack(message);
        break;
    case HANDLE_GO_FORWARD:
        handleGoForward(message);
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
        
    case B_REFS_RECEIVED: {
            // TODO: this probably needs to be grabbed with a RefPtr
			FileChooser *chooser;
            if (message->FindPointer("chooser", reinterpret_cast<void **>(&chooser)) == B_OK) {
                type_code type;
                int32 count = 0;
                entry_ref ref;
                BPath path;
                message->GetInfo("refs", &type, &count);
                if (count == 1) {
                    message->FindRef("refs", &ref);
                    path.SetTo(&ref);
                    chooser->chooseFile(String(path.Path()));
                } else {
                	Vector<String> filenames;
                	for (int32 i = 0; i < count; i++) {
                		message->FindRef("refs", i, &ref);
                		path.SetTo(&ref);
                		filenames.append(String(path.Path()));
                	}
                	chooser->chooseFiles(filenames);
                }
            }
        }
    	break;

    default:
        BHandler::MessageReceived(message);
    }
}

void WebProcess::skipToLastMessage(BMessage*& message)
{
	// NOTE: All messages that are fast-forwarded like this
	// need to be flagged with the intended target WebProcess,
	// or else we steal or process messages intended for another
	// WebProcess here!
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

void WebProcess::handleLoadURL(const BMessage* message)
{
    const char* urlString;
    if (message->FindString("url", &urlString) != B_OK)
        return;

    m_mainFrame->loadRequest(urlString);
}

void WebProcess::handleGoBack(const BMessage* message)
{
    m_mainFrame->goBack();
}

void WebProcess::handleGoForward(const BMessage* message)
{
    m_mainFrame->goForward();
}

void WebProcess::handleDraw(const BMessage* message)
{
    BRect rect;
    message->FindRect("update rect", &rect);
    paint(rect, true, false, true);
}

void WebProcess::handleFrameResized(const BMessage* message)
{
    float width;
    float height;
    message->FindFloat("width", &width);
    message->FindFloat("height", &height);

    WebCore::Frame* frame = m_mainFrame->frame();

    frame->view()->resize(width + 1, height + 1);
    frame->view()->forceLayout();
    frame->view()->adjustViewSize();

    m_webView->invalidate();
}

void WebProcess::handleFocused(const BMessage* message)
{
    bool focused;
    message->FindBool("focused", &focused);

    FocusController* focusController = m_page->focusController();
    focusController->setFocused(focused);
    if (focused && !focusController->focusedFrame())
        focusController->setFocusedFrame(m_mainFrame->frame());
}

void WebProcess::handleActivated(const BMessage* message)
{
    bool activated;
    message->FindBool("activated", &activated);

    FocusController* focusController = m_page->focusController();
    focusController->setActive(activated);
}

void WebProcess::handleMouseEvent(const BMessage* message)
{
    WebCore::Frame* frame = m_mainFrame->frame();
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

void WebProcess::handleMouseWheelChanged(BMessage* message)
{
    WebCore::Frame* frame = m_mainFrame->frame();
    if (!frame->view() || !frame->document())
        return;

    PlatformWheelEvent event(message);
    frame->eventHandler()->handleWheelEvent(event);
}

void WebProcess::handleKeyEvent(BMessage* message)
{
    WebCore::Frame* frame = m_mainFrame->frame();
    if (!frame->view() || !frame->document())
        return;

    PlatformKeyboardEvent event(message);
    frame->eventHandler()->keyEvent(event);
}

void WebProcess::handleChangeTextSize(BMessage* message)
{
    float increment = 0;
    message->FindFloat("increment", &increment);
    if (increment > 0 && m_mainFrame->canIncreaseTextSize())
    	m_mainFrame->increaseTextSize();
    else if (increment < 0 && m_mainFrame->canDecreaseTextSize())
    	m_mainFrame->decreaseTextSize();
    else
    	m_mainFrame->resetTextSize();
}

void WebProcess::handleFindString(BMessage* message)
{
    BMessage reply(FIND_STRING_RESULT);

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

    bool result = m_mainFrame->findString(string, forward, caseSensitive,
        wrapSelection, startInSelection);

    reply.AddBool("result", result);
    message->SendReply(&reply);
}
