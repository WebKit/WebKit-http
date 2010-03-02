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
#include "config.h"
#include "ChromeClientHaiku.h"

#include "FileChooser.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClientHaiku.h"
#include "FrameView.h"
#include "HitTestResult.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include "WebFrame.h"
#include "WebView.h"
#include "WebWindow.h"
#include "WindowFeatures.h"

#include <Alert.h>
#include <FilePanel.h>
#include <GroupLayout.h>
#include <Region.h>


namespace WebCore {

ChromeClientHaiku::ChromeClientHaiku(BWebPage* webPage, BWebView* webView)
    : m_webPage(webPage)
    , m_webView(webView)
{
}

ChromeClientHaiku::~ChromeClientHaiku()
{
}

void ChromeClientHaiku::chromeDestroyed()
{
    delete this;
}

void ChromeClientHaiku::setWindowRect(const FloatRect& rect)
{
    m_webPage->setWindowBounds(BRect(rect));
}

FloatRect ChromeClientHaiku::windowRect()
{
    return FloatRect(m_webPage->windowBounds());
}

FloatRect ChromeClientHaiku::pageRect()
{
	IntSize size = m_webPage->MainFrame()->Frame()->view()->contentsSize();
	return FloatRect(0, 0, size.width(), size.height());
}

float ChromeClientHaiku::scaleFactor()
{
    notImplemented();
    return 1;
}

void ChromeClientHaiku::focus()
{
    if (m_webView->LockLooper()) {
        m_webView->MakeFocus(true);
        m_webView->UnlockLooper();
    }
}

void ChromeClientHaiku::unfocus()
{
    if (m_webView->LockLooper()) {
        m_webView->MakeFocus(false);
        m_webView->UnlockLooper();
    }
}

bool ChromeClientHaiku::canTakeFocus(FocusDirection)
{
    return true;
}

void ChromeClientHaiku::takeFocus(FocusDirection)
{
}

void ChromeClientHaiku::focusedNodeChanged(Node* node)
{
    if (node)
        focus();
    else
        unfocus();
}

Page* ChromeClientHaiku::createWindow(Frame*, const FrameLoadRequest& request, const WebCore::WindowFeatures& features)
{
    BRect frame;
    if (features.xSet && features.ySet && features.widthSet && features.heightSet) {
        frame.left = features.x;
        frame.top = features.y;
        frame.right = features.x + features.width - 1;
        frame.bottom = features.y + features.height - 1;
    } else
        frame.Set(50, 50, 449, 449);

    window_look look = B_TITLED_WINDOW_LOOK;
    window_feel feel = B_NORMAL_WINDOW_FEEL;
    if (features.dialog) {
        look = B_MODAL_WINDOW_LOOK;
        feel = B_MODAL_APP_WINDOW_FEEL;
    }
    uint32 flags = B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS;
    if (!features.resizable)
        flags |= B_NOT_ZOOMABLE | B_NOT_RESIZABLE;

    BWebWindow* window = new BWebWindow(frame, "WebKit", look, feel, flags);
    BWebView* view = new BWebView("web view");
    window->AddChild(view);
    window->SetCurrentWebView(view);
    window->Show();

    view->LoadURL(BString(request.resourceRequest().url().string()));

    return view->WebPage()->page();
}

void ChromeClientHaiku::show()
{
    if (m_webView->LockLooper()) {
        if (m_webView->Window()->IsHidden())
            m_webView->Window()->Show();
        m_webView->UnlockLooper();
    }
}

bool ChromeClientHaiku::canRunModal()
{
    notImplemented();
    return false;
}

void ChromeClientHaiku::runModal()
{
    notImplemented();
}

void ChromeClientHaiku::setToolbarsVisible(bool visible)
{
    m_webPage->setToolbarsVisible(visible);
}

bool ChromeClientHaiku::toolbarsVisible()
{
    return m_webPage->areToolbarsVisible();
}

void ChromeClientHaiku::setStatusbarVisible(bool visible)
{
    m_webPage->setStatusbarVisible(visible);
}

bool ChromeClientHaiku::statusbarVisible()
{
    return m_webPage->isStatusbarVisible();
}

void ChromeClientHaiku::setScrollbarsVisible(bool visible)
{
    m_webPage->MainFrame()->SetAllowsScrolling(visible);
}

bool ChromeClientHaiku::scrollbarsVisible()
{
    return m_webPage->MainFrame()->AllowsScrolling();
}

void ChromeClientHaiku::setMenubarVisible(bool visible)
{
    m_webPage->setMenubarVisible(visible);
}

bool ChromeClientHaiku::menubarVisible()
{
    return m_webPage->isMenubarVisible();
}

void ChromeClientHaiku::setResizable(bool resizable)
{
    m_webPage->setResizable(resizable);
}

void ChromeClientHaiku::addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message,
                                            unsigned int lineNumber, const String& sourceID)
{
    printf("MESSAGE %s:%i %s\n", BString(sourceID).String(), lineNumber, BString(message).String());
}

bool ChromeClientHaiku::canRunBeforeUnloadConfirmPanel()
{
    return true;
}

bool ChromeClientHaiku::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    return runJavaScriptConfirm(frame, message);
}

void ChromeClientHaiku::closeWindowSoon()
{
    m_webPage->MainFrame()->Frame()->loader()->stopAllLoaders();
    m_webPage->closeWindow();
}

void ChromeClientHaiku::runJavaScriptAlert(Frame*, const String& msg)
{
    BAlert* alert = new BAlert("JavaScript", BString(msg).String(), "OK");
    alert->Go();
}

bool ChromeClientHaiku::runJavaScriptConfirm(Frame*, const String& msg)
{
    BAlert* alert = new BAlert("JavaScript", BString(msg).String(), "Yes", "No");
    return !alert->Go();
}

bool ChromeClientHaiku::runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result)
{
printf("ChromeClientHaiku::runJavaScriptPrompt()\n");
    notImplemented();
    return false;
}

void ChromeClientHaiku::setStatusbarText(const String& message)
{
    m_webPage->setStatusText(BString(message));
}

bool ChromeClientHaiku::shouldInterruptJavaScript()
{
    notImplemented();
    return false;
}

bool ChromeClientHaiku::tabsToLinks() const
{
    return false;
}

IntRect ChromeClientHaiku::windowResizerRect() const
{
    return IntRect();
}

void ChromeClientHaiku::repaint(const IntRect& rect, bool contentChanged, bool immediate, bool repaintContentOnly)
{
//printf("ChromeClientHaiku::repaint(rect(%d, %d, %d, %d), contentChanged: %d, "
//"immediate: %d, repaintContentOnly: %d)\n",
//rect.x(), rect.y(), rect.right(), rect.bottom(), contentChanged, immediate, repaintContentOnly);
    // TODO: This deadlocks when called from the app thread during init (fortunately immediate is false then)
    // contentChanged == false is used for scrolling.
    if (contentChanged) {
	    if (immediate)
	        m_webPage->paint(BRect(rect), contentChanged, immediate, repaintContentOnly);
	    else
	        m_webPage->draw(BRect(rect));
    }
}

void ChromeClientHaiku::scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect)
{
//printf("ChromeClientHaiku::scroll(%d x %d, rectToScroll(%d, %d, %d, %d), "
//"clipRect(%d, %d, %d, %d))\n", scrollDelta.width(), scrollDelta.height(),
//rectToScroll.x(), rectToScroll.y(), rectToScroll.right(), rectToScroll.bottom(),
//clipRect.x(), clipRect.y(), clipRect.right(), clipRect.bottom());
    m_webPage->scroll(scrollDelta.width(), scrollDelta.height(), rectToScroll, clipRect);
    m_webView->SendFakeMouseMovedEvent();
}

IntPoint ChromeClientHaiku::screenToWindow(const IntPoint& point) const
{
    IntPoint windowPoint(point);
    if (m_webView->LockLooperWithTimeout(5000) == B_OK) {
        windowPoint = IntPoint(m_webView->ConvertFromScreen(BPoint(point)));
        m_webView->UnlockLooper();
    }
    return windowPoint;
}

IntRect ChromeClientHaiku::windowToScreen(const IntRect& rect) const
{
    IntRect screenRect(rect);
    if (m_webView->LockLooperWithTimeout(5000) == B_OK) {
        screenRect = IntRect(m_webView->ConvertToScreen(BRect(rect)));
        m_webView->UnlockLooper();
    }
    return screenRect;
}

PlatformPageClient ChromeClientHaiku::platformPageClient() const
{
    return m_webView;
}

void ChromeClientHaiku::contentsSizeChanged(Frame*, const IntSize&) const
{
}

void ChromeClientHaiku::scrollRectIntoView(const IntRect&, const ScrollView*) const
{
    // NOTE: Used for example to make the view scroll with the mouse when selecting.
}

void ChromeClientHaiku::mouseDidMoveOverElement(const HitTestResult& result, unsigned /*modifierFlags*/)
{
    TextDirection dir;
    if (result.absoluteLinkURL() != lastHoverURL
        || result.title(dir) != lastHoverTitle
        || result.textContent() != lastHoverContent) {
        lastHoverURL = result.absoluteLinkURL();
        lastHoverTitle = result.title(dir);
        lastHoverContent = result.textContent();
        m_webPage->linkHovered(lastHoverURL.prettyURL(), lastHoverTitle, lastHoverContent);
    }
}

void ChromeClientHaiku::setToolTip(const String& tip, TextDirection)
{
    if (!m_webView->LockLooper())
        return;

     m_webView->SetToolTip(BString(tip).String());
     m_webView->UnlockLooper();
}

void ChromeClientHaiku::print(Frame*)
{
    notImplemented();
}

#if ENABLE(DATABASE)
void ChromeClientHaiku::exceededDatabaseQuota(Frame*, const String& databaseName)
{
    notImplemented();
}
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
void ChromeClientWx::reachedMaxAppCacheSize(int64_t spaceNeeded)
{
    notImplemented();
}
#endif

void ChromeClientHaiku::runOpenPanel(Frame*, PassRefPtr<FileChooser> chooser)
{
    RefPtr<FileChooser> *ref = new RefPtr<FileChooser>(chooser);
    BMessage message(B_REFS_RECEIVED);
    message.AddPointer("chooser", ref);
    BMessenger target(m_webPage);
    BFilePanel* panel = new BFilePanel(B_OPEN_PANEL, &target, 0, 0, (*ref)->allowsMultipleFiles(), &message, NULL, true, true);
    panel->Show();
}

void ChromeClientHaiku::iconForFiles(const Vector<String>&, PassRefPtr<FileChooser>)
{
    notImplemented();
}

bool ChromeClientHaiku::setCursor(PlatformCursorHandle)
{
printf("ChromeClientHaiku::setCursor()\n");
    notImplemented();
    return false;
}

void ChromeClientHaiku::requestGeolocationPermissionForFrame(Frame*, Geolocation*)
{
    notImplemented();
}

} // namespace WebCore

