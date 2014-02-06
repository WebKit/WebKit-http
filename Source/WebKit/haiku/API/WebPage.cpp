/*
 * Copyright (C) 2010 Ryan Leavengood <leavengood@gmail.com>
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


#include "BackForwardController.h"
#include "ChromeClientHaiku.h"
#include "ContextMenu.h"
#include "ContextMenuClientHaiku.h"
#include "ContextMenuController.h"
#include "CookieJar.h"
#include "Cursor.h"
#include "DOMTimer.h"
#include "DragClientHaiku.h"
#include "Editor.h"
#include "EditorClientHaiku.h"
#include "EventHandler.h"
#include "FileChooser.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClientHaiku.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "IconDatabase.h"
#include "InitializeThreading.h"
#include "InspectorClientHaiku.h"
#include "Logging.h"
#include "MemoryCache.h"
#include "Page.h"
#include "PageCache.h"
#include "PageGroup.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformStrategiesHaiku.h"
#include "PlatformWheelEvent.h"
#include "ProgressTrackerClient.h"
#include "ProgressTrackerHaiku.h"
#include "ResourceHandle.h"
#include "ResourceRequest.h"
#include "ScriptController.h"
#include "Settings.h"
#include "TextEncoding.h"
#include "WebDownload.h"
#include "WebDownloadPrivate.h"
#include "WebFrame.h"
#include "WebFramePrivate.h"
#include "WebSettings.h"
#include "WebView.h"
#include "WebViewConstants.h"

#include <Bitmap.h>
#include <Entry.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Window.h>

#include <wtf/text/AtomicString.h>
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

    HANDLE_CHANGE_ZOOM_FACTOR = 'zmfr',
    HANDLE_FIND_STRING = 'find',

    HANDLE_SET_STATUS_MESSAGE = 'stsm',
    HANDLE_RESEND_NOTIFICATIONS = 'rsnt',
    HANDLE_SEND_EDITING_CAPABILITIES = 'sedc',
    HANDLE_SEND_PAGE_SOURCE = 'spsc'
};

using namespace WebCore;

BMessenger BWebPage::sDownloadListener;

/*static*/ void BWebPage::InitializeOnce()
{
	// NOTE: This needs to be called when the BApplication is ready.
	// It won't work as static initialization.
#if !LOG_DISABLED
    WebCore::initializeLoggingChannelsIfNecessary();
#endif
    PlatformStrategiesHaiku::initialize();

    ScriptController::initializeThreading();
    WTF::initializeMainThread();
    WTF::AtomicString::init();
    Settings::setDefaultMinDOMTimerInterval(0.004);
    WebCore::UTF8Encoding();

    PageGroup::setShouldTrackVisitedLinks(true);
}

/*static*/ void BWebPage::ShutdownOnce()
{
    WebCore::iconDatabase().close();
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

    memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);
}

BWebPage::BWebPage(BWebView* webView)
    : BHandler("BWebPage")
    , fWebView(webView)
    , fMainFrame(NULL)
    , fSettings(NULL)
    , fPage(NULL)
    , fDumpRenderTree(NULL)
    , fLoadingProgress(100)
    , fStatusMessage()
    , fDisplayedStatusMessage()
    , fPageVisible(true)
    , fPageDirty(false)
    , fLayoutingView(false)
    , fToolbarsVisible(true)
    , fStatusbarVisible(true)
    , fMenubarVisible(true)
{
    Page::PageClients pageClients;
    pageClients.chromeClient = new ChromeClientHaiku(this, webView);
    pageClients.contextMenuClient = new ContextMenuClientHaiku(this);
    pageClients.editorClient = new EditorClientHaiku(this);
    pageClients.dragClient = new DragClientHaiku(webView);
    pageClients.inspectorClient = new InspectorClientHaiku();
    pageClients.loaderClientForMainFrame = new FrameLoaderClientHaiku(this);
    fProgressTracker = new ProgressTrackerClientHaiku(this);
    pageClients.progressTrackerClient = fProgressTracker;
    fPage = new Page(pageClients);

    fSettings = new BWebSettings(&fPage->settings());
}

BWebPage::~BWebPage()
{
	// We need to make sure there are no more timers running, since those
	// arrive to a different, global handler (the timer handler), and the
	// timer functions would then operate on stale pointers.
	// Calling detachFromParent() on the FrameLoader will recursively detach
	// all child frames, as well as stop all loaders before doing that.
    if (fMainFrame && fMainFrame->Frame())
        fMainFrame->Frame()->loader().detachFromParent();

    // NOTE: The m_webFrame member will be deleted by the
    // FrameLoaderClientHaiku, when the WebCore::Frame/FrameLoader instance is
    // free'd. For sub-frames, we don't maintain them anyway, and for the
    // main frame, the same mechanism is used.
    delete fSettings;
    delete fPage;
    // Deleting the BWebView is deferred here to keep it alive in
    // case some timers still fired after the view calling Shutdown() but
    // before we processed the shutdown message. If the BWebView had already
    // deleted itself before we reach the shutdown message, there would be
    // a race condition and chance to operate on a stale BWebView pointer.
    delete fWebView;
}

// #pragma mark - public

void BWebPage::Init()
{
	WebFramePrivate* data = new WebFramePrivate;
	data->page = fPage;

    fMainFrame = new BWebFrame(this, 0, data);
}

void BWebPage::Shutdown()
{
    Looper()->PostMessage(HANDLE_SHUTDOWN, this);
}

void BWebPage::SetListener(const BMessenger& listener)
{
	fListener = listener;
    fMainFrame->SetListener(listener);
    fProgressTracker->setDispatchTarget(listener);
}

void BWebPage::SetDownloadListener(const BMessenger& listener)
{
    sDownloadListener = listener;
}

BUrlContext** BWebPage::GetContext()
{
    return &WebView()->fContext;
}

void BWebPage::LoadURL(const char* urlString)
{
    BMessage message(HANDLE_LOAD_URL);
    message.AddString("url", urlString);
    Looper()->PostMessage(&message, this);
}

void BWebPage::Reload()
{
    Looper()->PostMessage(HANDLE_RELOAD, this);
}

void BWebPage::GoBack()
{
    Looper()->PostMessage(HANDLE_GO_BACK, this);
}

void BWebPage::GoForward()
{
    Looper()->PostMessage(HANDLE_GO_FORWARD, this);
}

void BWebPage::StopLoading()
{
    Looper()->PostMessage(HANDLE_STOP_LOADING, this);
}

void BWebPage::ChangeZoomFactor(float increment, bool textOnly)
{
	BMessage message(HANDLE_CHANGE_ZOOM_FACTOR);
	message.AddFloat("increment", increment);
	message.AddBool("text only", textOnly);
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

void BWebPage::SetDeveloperExtrasEnabled(bool enable)
{
    page()->settings().setDeveloperExtrasEnabled(enable);
}

void BWebPage::SetStatusMessage(const BString& status)
{
    BMessage message(HANDLE_SET_STATUS_MESSAGE);
    message.AddString("string", status);
    Looper()->PostMessage(&message, this);
}

void BWebPage::ResendNotifications()
{
    Looper()->PostMessage(HANDLE_RESEND_NOTIFICATIONS, this);
}

void BWebPage::SendEditingCapabilities()
{
    Looper()->PostMessage(HANDLE_SEND_EDITING_CAPABILITIES, this);
}

void BWebPage::SendPageSource()
{
	Looper()->PostMessage(HANDLE_SEND_PAGE_SOURCE, this);
}

/*static*/ void BWebPage::RequestDownload(const BString& url)
{
	ResourceRequest request(url);
	requestDownload(request, false);
}

BWebFrame* BWebPage::MainFrame() const
{
    return fMainFrame;
};

BWebSettings* BWebPage::Settings() const
{
    return fSettings;
};

BWebView* BWebPage::WebView() const
{
    return fWebView;
}

BString BWebPage::MainFrameTitle() const
{
    return fMainFrame->Title();
}

BString BWebPage::MainFrameRequestedURL() const
{
    return fMainFrame->RequestedURL();
}

BString BWebPage::MainFrameURL() const
{
    return fMainFrame->URL();
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
    const BPoint& /*where*/, const BPoint& /*screenWhere*/)
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
    return fPage;
}

WebCore::Page* BWebPage::createNewPage(BRect frame, bool modalDialog,
    bool resizable, bool activate)
{
    // Creating the BWebView in the application thread is exactly what we need anyway.
	BWebView* view = new BWebView("web view");
	BWebPage* page = view->WebPage();

    BMessage message(NEW_PAGE_CREATED);
    message.AddPointer("view", view);
    if (frame.IsValid())
        message.AddRect("frame", frame);
    message.AddBool("modal", modalDialog);
    message.AddBool("resizable", resizable);
    message.AddBool("activate", activate);

    // Block until some window has embedded this view.
    BMessage reply;
    fListener.SendMessage(&message, &reply);

    return page->page();
}

BRect BWebPage::windowFrame()
{
    BRect frame;
    if (fWebView->LockLooper()) {
        frame = fWebView->Window()->Frame();
        fWebView->UnlockLooper();
    }
    return frame;
}

BRect BWebPage::windowBounds()
{
    return windowFrame().OffsetToSelf(B_ORIGIN);
}

void BWebPage::setWindowBounds(const BRect& bounds)
{
	BMessage message(RESIZING_REQUESTED);
	message.AddRect("rect", bounds);
	BMessenger windowMessenger(fWebView->Window());
	if (windowMessenger.IsValid()) {
		// Better make this synchronous, since I don't know if it is
		// perhaps meant to be (called from ChromeClientHaiku::setWindowRect()).
		BMessage reply;
		windowMessenger.SendMessage(&message, &reply);
	}
}

BRect BWebPage::viewBounds()
{
    BRect bounds;
    if (fWebView->LockLooper()) {
        bounds = fWebView->Bounds();
        fWebView->UnlockLooper();
    }
    return bounds;
}

void BWebPage::setViewBounds(const BRect& /*bounds*/)
{
    if (fWebView->LockLooper()) {
        // TODO: Implement this with layout management, i.e. SetExplicitMinSize() or something...
        fWebView->UnlockLooper();
    }
}

void BWebPage::setToolbarsVisible(bool flag)
{
    fToolbarsVisible = flag;

    BMessage message(TOOLBARS_VISIBILITY);
    message.AddBool("flag", flag);
    dispatchMessage(message);
}

void BWebPage::setStatusbarVisible(bool flag)
{
    fStatusbarVisible = flag;

    BMessage message(STATUSBAR_VISIBILITY);
    message.AddBool("flag", flag);
    dispatchMessage(message);
}

void BWebPage::setMenubarVisible(bool flag)
{
    fMenubarVisible = flag;

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
	BMessage message(CLOSE_WINDOW_REQUESTED);
    dispatchMessage(message);
}

void BWebPage::linkHovered(const BString& url, const BString& /*title*/, const BString& /*content*/)
{
	if (url.Length())
		setDisplayedStatusMessage(url);
	else
		setDisplayedStatusMessage(fStatusMessage);
}

/*static*/ void BWebPage::requestDownload(const WebCore::ResourceRequest& request,
	bool isAsynchronousRequest)
{
    BWebDownload* download = new BWebDownload(new BPrivate::WebDownloadPrivate(request));
    downloadCreated(download, isAsynchronousRequest);
}

/*static*/ void BWebPage::downloadCreated(BWebDownload* download,
	bool isAsynchronousRequest)
{
	if (sDownloadListener.IsValid()) {
        BMessage message(B_DOWNLOAD_ADDED);
        message.AddPointer("download", download);
        if (isAsynchronousRequest) {
	        // Block until the listener has pulled all the information...
	        BMessage reply;
	        sDownloadListener.SendMessage(&message, &reply);
        } else {
	        sDownloadListener.SendMessage(&message);
        }
	} else {
		BPath desktopPath;
		find_directory(B_DESKTOP_DIRECTORY, &desktopPath);
        download->Start(desktopPath);
	}
}

void BWebPage::paint(BRect rect, bool immediate)
{
	if (fLayoutingView || !rect.IsValid())
		return;
    // Block any drawing as long as the BWebView is hidden
    // (should be extended to when the containing BWebWindow is not
    // currently on screen either...)
    if (!fPageVisible) {
        fPageDirty = true;
        return;
    }

    // NOTE: fMainFrame can be 0 because init() eventually ends up calling
    // paint()! BWebFrame seems to cause an initial page to be loaded, maybe
    // this ought to be avoided also for start-up speed reasons!
    if (!fMainFrame)
        return;
    WebCore::Frame* frame = fMainFrame->Frame();
    WebCore::FrameView* view = frame->view();

    if (!view || !frame->contentRenderer())
        return;

	// Since calling layoutIfNeededRecursive can cycle back into paint(),
	// call this method before locking the window and before holding the
	// offscreen view lock.
	fLayoutingView = true;
	view->layout(true);
	fLayoutingView = false;

    if (!fWebView->LockLooper())
        return;
    BView* offscreenView = fWebView->OffscreenView();

    // Lock the offscreen bitmap while we still have the
    // window locked. This cannot deadlock and makes sure
    // the window is not deleting the offscreen view right
    // after we unlock it and before locking the bitmap.
    if (!offscreenView->LockLooper()) {
    	fWebView->UnlockLooper();
    	return;
    }
    fWebView->UnlockLooper();

    if (!rect.IsValid())
        rect = offscreenView->Bounds();
    BRegion region(rect);
    internalPaint(offscreenView, view, &region);

    offscreenView->UnlockLooper();

    fPageDirty = false;

    // Notify the window that it can now pull the bitmap in its own thread
    fWebView->SetOffscreenViewClean(rect, immediate);
}

void BWebPage::scroll(int xOffset, int yOffset, const BRect& rectToScroll,
	const BRect& clipRect)
{
    if (!fWebView->LockLooper())
        return;
    BBitmap* bitmap = fWebView->OffscreenBitmap();
    BView* offscreenView = fWebView->OffscreenView();

    // Lock the offscreen bitmap while we still have the
    // window locked. This cannot deadlock and makes sure
    // the window is not deleting the offscreen view right
    // after we unlock it and before locking the bitmap.
    if (!bitmap->Lock()) {
    	fWebView->UnlockLooper();
    	return;
    }
    fWebView->UnlockLooper();

	BRect clip = offscreenView->Bounds();
	if (clipRect.IsValid())
		clip = clip & clipRect;
	BRect rectAtSrc = rectToScroll;
	BRect rectAtDst = rectAtSrc.OffsetByCopy(xOffset, yOffset);
	BRegion repaintRegion(rectAtSrc);
	if (clip.Intersects(rectAtSrc) && clip.Intersects(rectAtDst)) {
		// clip source rect
		rectAtSrc = rectAtSrc & clip;
		// clip dest rect
		rectAtDst = rectAtDst & clip;
		// move dest back over source and clip source to dest
		rectAtDst.OffsetBy(-xOffset, -yOffset);
		rectAtSrc = rectAtSrc & rectAtDst;
		// remember the part that will be clean
		rectAtDst.OffsetBy(xOffset, yOffset);
		repaintRegion.Exclude(rectAtDst);

		offscreenView->CopyBits(rectAtSrc, rectAtDst);
	}

	if (repaintRegion.Frame().IsValid()) {
        WebCore::Frame* frame = fMainFrame->Frame();
        WebCore::FrameView* view = frame->view();
        // Make sure the view is layouted, since it will refuse to paint
        // otherwise.
        fLayoutingView = true;
        view->layout(true);
        fLayoutingView = false;

		internalPaint(offscreenView, view, &repaintRegion);
	}

    bitmap->Unlock();

    // Notify the view that it can now pull the bitmap in its own thread
    fWebView->SetOffscreenViewClean(rectToScroll, false);
}

void BWebPage::internalPaint(BView* offscreenView,
                             WebCore::FrameView* frameView, BRegion* dirty)
{
    ASSERT(!frameView->needsLayout());
    offscreenView->PushState();
    offscreenView->ConstrainClippingRegion(dirty);
    WebCore::GraphicsContext context(offscreenView);
    frameView->paint(&context, IntRect(dirty->Frame()));
    offscreenView->PopState();
}

void BWebPage::setLoadingProgress(float progress)
{
	fLoadingProgress = progress;

	BMessage message(LOAD_PROGRESS);
	message.AddFloat("progress", progress);
	dispatchMessage(message);
}

void BWebPage::setStatusMessage(const BString& statusMessage)
{
	if (fStatusMessage == statusMessage)
		return;

	fStatusMessage = statusMessage;

	setDisplayedStatusMessage(statusMessage);
}

void BWebPage::setDisplayedStatusMessage(const BString& statusMessage, bool force)
{
	if (fDisplayedStatusMessage == statusMessage && !force)
		return;

	fDisplayedStatusMessage = statusMessage;

	BMessage message(SET_STATUS_TEXT);
	message.AddString("text", statusMessage);
	dispatchMessage(message);
}


void BWebPage::runJavaScriptAlert(const BString& text)
{
    BMessage message(SHOW_JS_ALERT);
	message.AddString("text", text);
	dispatchMessage(message);
}


bool BWebPage::runJavaScriptConfirm(const BString& text)
{
    BMessage message(SHOW_JS_CONFIRM);
	message.AddString("text", text);
    BMessage reply;
	dispatchMessage(message, &reply);

    return reply.FindBool("result");
}


void BWebPage::addMessageToConsole(const BString& source, int lineNumber,
    int columnNumber, const BString& text)
{
    BMessage message(ADD_CONSOLE_MESSAGE);
    message.AddString("source", source);
    message.AddInt32("line", lineNumber);
    message.AddInt32("column", columnNumber);
    message.AddString("string", text);
	dispatchMessage(message);
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
        paint(updateRect, false);
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
        // fall through
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

	case HANDLE_CHANGE_ZOOM_FACTOR:
		handleChangeZoomFactor(message);
		break;
    case HANDLE_FIND_STRING:
        handleFindString(message);
        break;

	case HANDLE_SET_STATUS_MESSAGE: {
		BString status;
		if (message->FindString("string", &status) == B_OK)
			setStatusMessage(status);
		break;
	}

    case HANDLE_RESEND_NOTIFICATIONS:
        handleResendNotifications(message);
        break;
    case HANDLE_SEND_EDITING_CAPABILITIES:
        handleSendEditingCapabilities(message);
        break;
    case HANDLE_SEND_PAGE_SOURCE:
        handleSendPageSource(message);
        break;

    case B_REFS_RECEIVED: {
		RefPtr<FileChooser>* chooser;
        if (message->FindPointer("chooser", reinterpret_cast<void**>(&chooser)) == B_OK) {
            entry_ref ref;
            BPath path;
            Vector<String> filenames;
            for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
                path.SetTo(&ref);
                filenames.append(String(path.Path()));
            }
            (*chooser)->chooseFiles(filenames);
            delete chooser;
        }
    	break;
    }
    case B_CANCEL: {
    	int32 oldWhat;
    	BFilePanel* panel;
    	if (message->FindPointer("source", reinterpret_cast<void**>(&panel)) == B_OK
    		&& message->FindInt32("old_what", &oldWhat) == B_OK
    		&& oldWhat == B_REFS_RECEIVED) {
    		// TODO: Eventually it would be nice to reuse the same file panel...
    		// At least don't leak the file panel for now.
    		delete panel;
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

    fMainFrame->LoadURL(urlString);
}

void BWebPage::handleReload(const BMessage*)
{
    fMainFrame->Reload();
}

void BWebPage::handleGoBack(const BMessage*)
{
    fPage->backForward().goBack();
}

void BWebPage::handleGoForward(const BMessage*)
{
    fPage->backForward().goForward();
}

void BWebPage::handleStop(const BMessage*)
{
    fMainFrame->StopLoading();
}

void BWebPage::handleSetVisible(const BMessage* message)
{
    message->FindBool("visible", &fPageVisible);
    if (fMainFrame->Frame()->view())
        fMainFrame->Frame()->view()->setParentVisible(fPageVisible);
    // Trigger an internal repaint if the page was supposed to be repainted
    // while it was invisible.
    if (fPageVisible && fPageDirty)
        paint(viewBounds(), false);
}

void BWebPage::handleFrameResized(const BMessage* message)
{
    float width;
    float height;
    message->FindFloat("width", &width);
    message->FindFloat("height", &height);

    WebCore::Frame* frame = fMainFrame->Frame();
    frame->view()->resize(width + 1, height + 1);
    frame->view()->forceLayout();
    frame->view()->adjustViewSize();
}

void BWebPage::handleFocused(const BMessage* message)
{
    bool focused;
    message->FindBool("focused", &focused);

    FocusController& focusController = fPage->focusController();
    focusController.setFocused(focused);
    if (focused && !focusController.focusedFrame())
        focusController.setFocusedFrame(fMainFrame->Frame());
}

void BWebPage::handleActivated(const BMessage* message)
{
    bool activated;
    message->FindBool("activated", &activated);

    FocusController& focusController = fPage->focusController();
    focusController.setActive(activated);
}

void BWebPage::handleMouseEvent(const BMessage* message)
{
    WebCore::Frame* frame = fMainFrame->Frame();
    if (!frame->view() || !frame->document())
        return;

    PlatformMouseEvent event(message);
    switch (message->what) {
    case B_MOUSE_DOWN:
        // Handle context menus, if necessary.
        if (event.button() == RightButton) {
            fPage->contextMenuController().clearContextMenu();

            WebCore::Frame& focusedFrame = fPage->focusController().focusedOrMainFrame();
            focusedFrame.eventHandler().sendContextMenuEvent(event);
            // If the web page implements it's own context menu handling, then
            // the contextMenu() pointer will be zero. In this case, we should
            // also swallow the event.
            ContextMenu* contextMenu = fPage->contextMenuController().contextMenu();
            if (contextMenu) {
            	BMenu* platformMenu = contextMenu->releasePlatformDescription();
            	if (platformMenu) {
            		// Need to convert the BMenu into BPopUpMenu.
	            	BPopUpMenu* popupMenu = new BPopUpMenu("context menu");
					for (int32 i = platformMenu->CountItems() - 1; i >= 0; i--) {
					    BMenuItem* item = platformMenu->RemoveItem(i);
					    popupMenu->AddItem(item, 0);
					}
					BPoint screenLocation(event.globalPosition().x() + 2,
					    event.globalPosition().y() + 2);
            	    popupMenu->Go(screenLocation, true, true, true);
            	    delete platformMenu;
            	}
            }
            break;
    	}
    	// Handle regular mouse events.
        frame->eventHandler().handleMousePressEvent(event);
        break;
    case B_MOUSE_UP:
        frame->eventHandler().handleMouseReleaseEvent(event);
        break;
    case B_MOUSE_MOVED:
    default:
        frame->eventHandler().mouseMoved(event);
        break;
    }
}

void BWebPage::handleMouseWheelChanged(BMessage* message)
{
    WebCore::Frame* frame = fMainFrame->Frame();
    if (!frame->view() || !frame->document())
        return;

    PlatformWheelEvent event(message);
    frame->eventHandler().handleWheelEvent(event);
}

void BWebPage::handleKeyEvent(BMessage* message)
{
    WebCore::Frame& frame = fPage->focusController().focusedOrMainFrame();
    if (!frame.view() || !frame.document())
        return;

    PlatformKeyboardEvent event(message);
	// Try to let WebCore handle this event
	if (!frame.eventHandler().keyEvent(event) && message->what == B_KEY_DOWN) {
		// Handle keyboard scrolling (probably should be extracted to a method.)
		ScrollDirection direction;
		ScrollGranularity granularity;
		BString bytes = message->FindString("bytes");
		switch (bytes.ByteAt(0)) {
			case B_UP_ARROW:
				granularity = ScrollByLine;
				direction = ScrollUp;
				break;
			case B_DOWN_ARROW:
				granularity = ScrollByLine;
				direction = ScrollDown;
				break;
			case B_LEFT_ARROW:
				granularity = ScrollByLine;
				direction = ScrollLeft;
				break;
			case B_RIGHT_ARROW:
				granularity = ScrollByLine;
				direction = ScrollRight;
				break;
			case B_HOME:
				granularity = ScrollByDocument;
				direction = ScrollUp;
				break;
			case B_END:
				granularity = ScrollByDocument;
				direction = ScrollDown;
				break;
			case B_PAGE_UP:
				granularity = ScrollByPage;
				direction = ScrollUp;
				break;
			case B_PAGE_DOWN:
				granularity = ScrollByPage;
				direction = ScrollDown;
				break;
			default:
				return;
		}
		frame.eventHandler().scrollRecursively(direction, granularity);
	}
}

void BWebPage::handleChangeZoomFactor(BMessage* message)
{
    float increment;
    if (message->FindFloat("increment", &increment) != B_OK)
    	increment = 0;

    bool textOnly;
    if (message->FindBool("text only", &textOnly) != B_OK)
    	textOnly = true;

    if (increment > 0)
    	fMainFrame->IncreaseZoomFactor(textOnly);
    else if (increment < 0)
    	fMainFrame->DecreaseZoomFactor(textOnly);
    else
    	fMainFrame->ResetZoomFactor();
}

void BWebPage::handleFindString(BMessage* message)
{
    BMessage reply(B_FIND_STRING_RESULT);

    BString string;
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

    WebCore::FindOptions options = 0;
    if (!forward)
        options |= WebCore::Backwards;
    if (!caseSensitive)
        options |= WebCore::CaseInsensitive;
    if (wrapSelection)
        options |= WebCore::WrapAround;
    if (startInSelection)
        options |= WebCore::StartInSelection;

    bool result = fMainFrame->FindString(string, options);

    reply.AddBool("result", result);
    message->SendReply(&reply);
}

void BWebPage::handleResendNotifications(BMessage*)
{
	// Prepare navigation capabilities notification
    BMessage message(UPDATE_NAVIGATION_INTERFACE);
    message.AddBool("can go backward", fPage->backForward().canGoBackOrForward(-1));
    message.AddBool("can go forward", fPage->backForward().canGoBackOrForward(1));
    WebCore::FrameLoader& loader = fMainFrame->Frame()->loader();
    message.AddBool("can stop", loader.isLoading());
    dispatchMessage(message);
	// Send loading progress and status text notifications
    setLoadingProgress(fLoadingProgress);
    setDisplayedStatusMessage(fStatusMessage, true);
    // TODO: Other notifications...
}

void BWebPage::handleSendEditingCapabilities(BMessage*)
{
    bool canCut = false;
    bool canCopy = false;
    bool canPaste = false;

    WebCore::Frame& frame = fPage->focusController().focusedOrMainFrame();
    WebCore::Editor& editor = frame.editor();

    canCut = editor.canCut() || editor.canDHTMLCut();
    canCopy = editor.canCopy() || editor.canDHTMLCopy();
    canPaste = editor.canPaste() || editor.canDHTMLPaste();

    BMessage message(B_EDITING_CAPABILITIES_RESULT);
    message.AddBool("can cut", canCut);
    message.AddBool("can copy", canCopy);
    message.AddBool("can paste", canPaste);

    dispatchMessage(message);
}

void BWebPage::handleSendPageSource(BMessage*)
{
    BMessage message(B_PAGE_SOURCE_RESULT);
    message.AddString("source", fMainFrame->FrameSource());
    message.AddString("url", fMainFrame->URL());

    dispatchMessage(message);
}

// #pragma mark -

status_t BWebPage::dispatchMessage(BMessage& message, BMessage* reply) const
{
	message.AddPointer("view", fWebView);
    if (reply)
	    return fListener.SendMessage(&message, reply);
    else
	    return fListener.SendMessage(&message);
}

