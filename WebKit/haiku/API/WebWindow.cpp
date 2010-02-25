/*
 * Copyright (C) 2007 Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
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
#include "WebWindow.h"

#include "WebView.h"
#include "WebViewConstants.h"

#include <Application.h>
#include <Button.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <StringView.h>
#include <TextControl.h>

#include <stdio.h>

BWebWindow::BWebWindow(BRect frame, const char* title, window_look look,
        window_feel feel, uint32 flags, uint32 workspace)
    : BWindow(frame, title, look, feel, flags, workspace)
    , fWebView(0)
{
    SetLayout(new BGroupLayout(B_HORIZONTAL));

    // NOTE: Do NOT change these because you think Redo should be on Cmd-Y!
    AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO));
    AddShortcut('Y', B_COMMAND_KEY, new BMessage(B_UNDO));
    AddShortcut('Z', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(B_REDO));
    AddShortcut('Y', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(B_REDO));
}

BWebWindow::~BWebWindow()
{
}

void BWebWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case LOAD_ONLOAD_HANDLE:
        // NOTE: Supposedly this notification is sent when the so-called
        // "onload" event has been passed and executed to scripts in the
        // site.
        break;
    case LOAD_DL_COMPLETED:
        // NOTE: All loading has finished. We currently handle this via
        // the progress notification. But it doesn't hurt to call the hook
        // one more time.
        LoadProgress(100, _WebViewForMessage(message));
        break;
    case LOAD_DOC_COMPLETED:
        // NOTE: This events means the DOM document is ready.
        break;
    case JAVASCRIPT_WINDOW_OBJECT_CLEARED:
        // NOTE: No idea what this event actually means.
        break;
    case NAVIGATION_REQUESTED: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            NavigationRequested(url, _WebViewForMessage(message));
        break;
    }
    case UPDATE_HISTORY: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            UpdateGlobalHistory(url);
        break;
    }
    case NEW_WINDOW_REQUESTED: {
        bool primaryAction = false;
        message->FindBool("primary", &primaryAction);
        BString url;
        if (message->FindString("url", &url) == B_OK)
            NewWindowRequested(url, primaryAction);
        break;
    }
    case LOAD_NEGOTIATING: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            LoadNegotiating(url, _WebViewForMessage(message));
        break;
    }
    case LOAD_COMMITTED: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            LoadCommited(url, _WebViewForMessage(message));
        break;
    }
    case LOAD_PROGRESS: {
        float progress;
        if (message->FindFloat("progress", &progress) == B_OK)
            LoadProgress(progress, _WebViewForMessage(message));
        break;
    }
    case LOAD_FAILED: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            LoadFailed(url, _WebViewForMessage(message));
        break;
    }
    case LOAD_FINISHED: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            LoadFinished(url, _WebViewForMessage(message));
        break;
    }
    case TITLE_CHANGED: {
        BString title;
        if (message->FindString("title", &title) == B_OK)
            TitleChanged(title, _WebViewForMessage(message));
        break;
    }
    case RESIZING_REQUESTED: {
        BRect rect;
        if (message->FindRect("rect", &rect) == B_OK)
            ResizeRequested(rect.Width(), rect.Height(), _WebViewForMessage(message));
        break;
    }
    case SET_STATUS_TEXT: {
        BString text;
        if (message->FindString("text", &text) == B_OK)
            StatusChanged(text, _WebViewForMessage(message));
        break;
    }
    case UPDATE_NAVIGATION_INTERFACE: {
        bool canGoBackward = false;
        bool canGoForward = false;
        bool canStop = false;
        message->FindBool("can go backward", &canGoBackward);
        message->FindBool("can go forward", &canGoForward);
        message->FindBool("can stop", &canStop);
        NavigationCapabilitiesChanged(canGoBackward, canGoForward, canStop, _WebViewForMessage(message));
        break;
    }
    case AUTHENTICATION_CHALLENGE: {
        AuthenticationChallenge(message);
        break;
    }

    case TOOLBARS_VISIBILITY: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            SetToolBarsVisible(flag, _WebViewForMessage(message));
        break;
    }
    case STATUSBAR_VISIBILITY: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            SetStatusBarVisible(flag, _WebViewForMessage(message));
        break;
    }
    case MENUBAR_VISIBILITY: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            SetMenuBarVisible(flag, _WebViewForMessage(message));
        break;
    }
    case SET_RESIZABLE: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            SetResizable(flag, _WebViewForMessage(message));
        break;
    }

    case B_MOUSE_WHEEL_CHANGED:
    	if (fWebView)
        	fWebView->MessageReceived(message);
        break;

    default:
        BWindow::MessageReceived(message);
        break;
    }
}

bool BWebWindow::QuitRequested()
{
    // Do this here, so WebKit tear down happens earlier.
    if (fWebView) {
        fWebView->RemoveSelf();
        delete fWebView;
    }

    return true;
}

// #pragma mark -

void BWebWindow::SetCurrentWebView(BWebView* view)
{
	fWebView = view;
}

BWebView* BWebWindow::CurrentWebView() const
{
	return fWebView;
}

// #pragma mark - Notification API

void BWebWindow::NavigationRequested(const BString& url, BWebView* view)
{
}

void BWebWindow::NewWindowRequested(const BString& url, bool primaryAction)
{
}

void BWebWindow::LoadNegotiating(const BString& url, BWebView* view)
{
}

void BWebWindow::LoadCommited(const BString& url, BWebView* view)
{
}

void BWebWindow::LoadProgress(float progress, BWebView* view)
{
}

void BWebWindow::LoadFailed(const BString& url, BWebView* view)
{
}

void BWebWindow::LoadFinished(const BString& url, BWebView* view)
{
}

void BWebWindow::TitleChanged(const BString& title, BWebView* view)
{
    SetTitle(title.String());
}

void BWebWindow::ResizeRequested(float width, float height, BWebView* view)
{
    ResizeTo(width, height);
}

void BWebWindow::SetToolBarsVisible(bool flag, BWebView* view)
{
}

void BWebWindow::SetStatusBarVisible(bool flag, BWebView* view)
{
}

void BWebWindow::SetMenuBarVisible(bool flag, BWebView* view)
{
}

void BWebWindow::SetResizable(bool flag, BWebView* view)
{
    if (flag)
        SetFlags(Flags() & ~B_NOT_RESIZABLE);
    else
        SetFlags(Flags() | B_NOT_RESIZABLE);
}

void BWebWindow::StatusChanged(const BString& statusText, BWebView* view)
{
}

void BWebWindow::NavigationCapabilitiesChanged(bool canGoBackward,
    bool canGoForward, bool canStop, BWebView* view)
{
}

void BWebWindow::UpdateGlobalHistory(const BString& url)
{
}

void BWebWindow::AuthenticationChallenge(BMessage* challenge)
{
}

// #pragma mark - private

BWebView* BWebWindow::_WebViewForMessage(const BMessage* message) const
{
	// Default to the current BWebView, if there is none in the message.
	BWebView* view = fWebView;
	message->FindPointer("view", reinterpret_cast<void**>(&view));
	return view;
}

