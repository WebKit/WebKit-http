/*
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
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
#include "WebView.h"

#include "NotImplemented.h"
#include "WebPage.h"
#include "WebViewConstants.h"
#include <Alert.h>
#include <AppDefs.h>
#include <Application.h>
#include <Bitmap.h>
#include <Region.h>
#include <Window.h>
#include <stdio.h>
#include <stdlib.h>

using namespace WebCore;

WebView::WebView(const char* name)
    : BView(name, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE)
    , m_offscreenBitmap(0)
    , m_offscreenViewClean(false)
    , m_webPage(new WebPage(this))
{
    m_webPage->init();
    // TODO: Should add this to the "current" looper, but that looper needs to
    // stay around regardless of windows opening/closing. Adding it to the
    // app looper is the safest bet for now.
    if (be_app->Lock()) {
        be_app->AddHandler(m_webPage);
        be_app->Unlock();
    }

    FrameResized(Bounds().Width(), Bounds().Height());

    SetViewColor(B_TRANSPARENT_COLOR);
}

WebView::~WebView()
{
}

// #pragma mark - BView hooks

void WebView::AttachedToWindow()
{
    m_webPage->setDispatchTarget(BMessenger(Window()));
}

void WebView::DetachedFromWindow()
{
// TODO: Needs locking.
//    m_webPage->setDispatchTarget(0);
    m_webPage->shutdown();
}

void WebView::Draw(BRect rect)
{
    if (!m_offscreenViewClean) {
// TODO: Disabled because WebCore manages invalidation internally and knows when
// to redraw. Since we are drawing a fixed/independent bitmap anyway, we don't
// need to manually trigger WebCore repaint events. These might be causing the
// weird hang in BitmapImage::draw() -> startAnimation(). That's why I disable
// it here for the time being to see if these hangs go away.
//       m_webPage->draw(rect);
        return;
    }

    if (!m_offscreenBitmap->Lock()) {
        SetHighColor(255, 0, 0);
        FillRect(rect);
        return;
    }

    m_offscreenView->Sync();
    DrawBitmap(m_offscreenBitmap, m_offscreenView->Bounds(),
        m_offscreenView->Bounds());

    m_offscreenBitmap->Unlock();
}

void WebView::FrameResized(float width, float height)
{
    resizeOffscreenView(width + 1, height + 1);
    m_webPage->frameResized(width, height);
}

void WebView::GetPreferredSize(float* width, float* height)
{
	// This needs to be implemented, since the default BView implementation
	// is to return the current width/height of the view. The default
	// implementations for Min/Max/PreferredSize() will then work for us.
	if (width)
		*width = 100;
	if (height)
		*height = 100;
}

void WebView::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case B_MOUSE_WHEEL_CHANGED: {
        BPoint where;
        uint32 buttons;
        GetMouse(&where, &buttons);
        BPoint screenWhere = ConvertToScreen(where);
        m_webPage->mouseWheelChanged(message, where, screenWhere);
        // NOTE: This solves a bug in WebKit itself, it should issue a mouse
        // event when scrolling by wheel event. The effects of the this bug
        // can be witnessed in Safari as well.
        BMessage mouseMessage(B_MOUSE_MOVED);
        mouseMessage.AddPoint("be:view_where", where);
        mouseMessage.AddPoint("screen_where", screenWhere);
        mouseMessage.AddInt32("buttons", buttons);
        mouseMessage.AddInt32("modifiers", modifiers());
        m_webPage->mouseEvent(&mouseMessage, where, screenWhere);
        break;
    }

    case B_SELECT_ALL:
    case B_COPY:
    case B_CUT:
    case B_PASTE:
    case B_UNDO:
    case B_REDO:
        m_webPage->standardShortcut(message);
        break;

    case FIND_STRING_RESULT:
        Window()->PostMessage(message);
        break;

    default:
        BView::MessageReceived(message);
        break;
    }
}

void WebView::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);
	m_webPage->focused(focused);
}

void WebView::WindowActivated(bool activated)
{
    m_webPage->activated(activated);
}

void WebView::MouseMoved(BPoint where, uint32, const BMessage*)
{
    dispatchMouseEvent(where, B_MOUSE_MOVED);
}

void WebView::MouseDown(BPoint where)
{
	MakeFocus(true);
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
    dispatchMouseEvent(where, B_MOUSE_DOWN);
}

void WebView::MouseUp(BPoint where)
{
    dispatchMouseEvent(where, B_MOUSE_UP);
}

void WebView::KeyDown(const char*, int32)
{
    dispatchKeyEvent(B_KEY_DOWN);
}

void WebView::KeyUp(const char*, int32)
{
    dispatchKeyEvent(B_KEY_UP);
}

// #pragma mark - API

void WebView::setBounds(BRect rect)
{
    BMessage message(RESIZING_REQUESTED);
    message.AddRect("rect", rect);
    message.AddPointer("view", this);
    Window()->PostMessage(&message);
}

BRect WebView::contentsSize() const
{
    return m_webPage->contentsSize();
}

BString WebView::mainFrameTitle() const
{
    return m_webPage->mainFrameTitle();
}

BString WebView::mainFrameURL() const
{
    return m_webPage->mainFrameURL();
}

void WebView::loadRequest(const char* urlString)
{
    m_webPage->loadURL(urlString);
}

void WebView::goBack()
{
    m_webPage->goBack();
}

void WebView::goForward()
{
    m_webPage->goForward();
}

void WebView::setToolbarsVisible(bool flag)
{
    m_toolbarsVisible = flag;

    BMessage message(TOOLBARS_VISIBILITY);
    message.AddBool("flag", flag);
    message.AddPointer("view", this);
    Window()->PostMessage(&message);
}

void WebView::setStatusbarVisible(bool flag)
{
    m_statusbarVisible = flag;

    BMessage message(STATUSBAR_VISIBILITY);
    message.AddBool("flag", flag);
    message.AddPointer("view", this);
    Window()->PostMessage(&message);
}

void WebView::setMenubarVisible(bool flag)
{
    m_menubarVisible = flag;

    BMessage message(MENUBAR_VISIBILITY);
    message.AddBool("flag", flag);
    message.AddPointer("view", this);
    Window()->PostMessage(&message);
}

void WebView::setResizable(bool flag)
{
    BMessage message(SET_RESIZABLE);
    message.AddBool("flag", flag);
    message.AddPointer("view", this);
    Window()->PostMessage(&message);
}

void WebView::closeWindow()
{
    Window()->PostMessage(B_QUIT_REQUESTED);
}

void WebView::setStatusText(const BString& text)
{
    BMessage message(SET_STATUS_TEXT);
    message.AddString("text", text);
    message.AddPointer("view", this);
    Window()->PostMessage(&message);
}

void WebView::linkHovered(const String& url, const String& title, const String& content)
{
printf("linkHovered()\n");
    notImplemented();
}

void WebView::increaseTextSize()
{
	m_webPage->changeTextSize(1);
}

void WebView::decreaseTextSize()
{
	m_webPage->changeTextSize(-1);
}

void WebView::resetTextSize()
{
	m_webPage->changeTextSize(0);
}

void WebView::findString(const char* string, bool forward ,
    bool caseSensitive, bool wrapSelection, bool startInSelection)
{
	m_webPage->findString(string, forward, caseSensitive,
	    wrapSelection, startInSelection);
}

// #pragma mark - API for WebPage only

void WebView::setOffscreenViewClean(BRect cleanRect, bool immediate)
{
    if (LockLooper()) {
        m_offscreenViewClean = true;
        if (immediate)
            Draw(cleanRect);
        else
            Invalidate(cleanRect);
        UnlockLooper();
    }
}

void WebView::invalidate()
{
    if (LockLooper()) {
        m_offscreenViewClean = false;
        Invalidate();
        UnlockLooper();
    }
}

// #pragma mark - private

void WebView::resizeOffscreenView(int width, int height)
{
    BRect bounds(0, 0, width - 1, height - 1);
    if (m_offscreenBitmap) {
        m_offscreenBitmap->Lock();
        if (m_offscreenBitmap->Bounds().Contains(bounds)) {
            // Just resize the view and clear the exposed parts, but never
            // shrink the bitmap).
            BRect oldViewBounds(m_offscreenView->Bounds());
            m_offscreenView->ResizeTo(width - 1, height - 1);
            BRegion exposed(m_offscreenView->Bounds());
            exposed.Exclude(oldViewBounds);
            m_offscreenView->FillRegion(&exposed, B_SOLID_LOW);
            m_offscreenBitmap->Unlock();
            return;
        }
    }
    BBitmap* oldBitmap = m_offscreenBitmap;
    BView* oldView = m_offscreenView;

    m_offscreenBitmap = new BBitmap(bounds, B_RGB32, true);
    if (m_offscreenBitmap->InitCheck() != B_OK) {
        BAlert* alert = new BAlert("Internal error", "Unable to create off-screen bitmap for WebKit contents.", "OK");
        alert->Go();
        exit(1);
    }
    m_offscreenView = new BView(bounds, "WebKit offscreen view", 0, 0);
    m_offscreenBitmap->Lock();
    m_offscreenBitmap->AddChild(m_offscreenView);

    if (oldBitmap) {
        // Transfer the old bitmap contents (just the visible part) and
        // clear the rest.
        BRegion region(m_offscreenView->Bounds());
        BRect oldViewBounds = oldView->Bounds();
        region.Exclude(oldViewBounds);
        m_offscreenView->DrawBitmap(oldBitmap, oldViewBounds, oldViewBounds);
        m_offscreenView->FillRegion(&region, B_SOLID_LOW);
        delete oldBitmap;
            // Takes care of old m_offscreenView too.
    }

    m_offscreenBitmap->Unlock();
}

void WebView::dispatchMouseEvent(const BPoint& where, uint32 sanityWhat)
{
    BMessage* message = Looper()->CurrentMessage();
    if (!message || message->what != sanityWhat)
        return;

	if (sanityWhat == B_MOUSE_UP) {
		// the activation click may contain no previous buttons
		if (!m_lastMouseButtons)
			return;

		message->AddInt32("previous buttons", m_lastMouseButtons);
	} else
		message->FindInt32("buttons", (int32*)&m_lastMouseButtons);

    m_webPage->mouseEvent(message, where, ConvertToScreen(where));
}

void WebView::dispatchKeyEvent(uint32 sanityWhat)
{
    BMessage* message = Looper()->CurrentMessage();
    if (!message || message->what != sanityWhat)
        return;

    m_webPage->keyEvent(message);
}

