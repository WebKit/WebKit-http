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
#include "WebViewWindow.h"

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

using namespace WebCore;

WebViewWindow::WebViewWindow(BRect frame, const char* name, window_look look,
        window_feel feel, uint32 flags, uint32 workspace)
    : BWindow(frame, name, look, feel, flags, workspace)
    , m_webView(0)
{
    SetLayout(new BGroupLayout(B_HORIZONTAL));

    // NOTE: Do NOT change these because you think Redo should be on Cmd-Y!
    AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO));
    AddShortcut('Y', B_COMMAND_KEY, new BMessage(B_UNDO));
    AddShortcut('Z', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(B_REDO));
    AddShortcut('Y', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(B_REDO));
}

WebViewWindow::~WebViewWindow()
{
}

void WebViewWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case LOAD_ONLOAD_HANDLE:
        printf("LOAD_ONLOAD_HANDLE\n");
        break;
    case LOAD_DL_COMPLETED:
        printf("LOAD_DL_COMPLETED\n");
        break;
    case LOAD_DOC_COMPLETED:
        printf("LOAD_DOC_COMPLETED\n");
        break;
    case JAVASCRIPT_WINDOW_OBJECT_CLEARED:
        printf("JAVASCRIPT_WINDOW_OBJECT_CLEARED\n");
        break;
    case NAVIGATION_REQUESTED: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            navigationRequested(url, webViewForMessage(message));
        break;
    }
    case UPDATE_HISTORY: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            updateGlobalHistory(url);
        break;
    }
    case NEW_WINDOW_REQUESTED: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            newWindowRequested(url);
        break;
    }
    case LOAD_NEGOTIATING: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            loadNegotiating(url, webViewForMessage(message));
        break;
    }
    case LOAD_TRANSFERRING: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            loadTransferring(url, webViewForMessage(message));
        break;
    }
    case LOAD_PROGRESS: {
        float progress;
        if (message->FindFloat("progress", &progress) == B_OK)
            loadProgress(progress, webViewForMessage(message));
        break;
    }
    case LOAD_FAILED: {
        BString url;
        if (message->FindString("url", &url) == B_OK)
            loadFailed(url, webViewForMessage(message));
        break;
    }
    case LOAD_FINISHED: {
        BString title;
        if (message->FindString("url", &title) == B_OK)
            loadFinished(title, webViewForMessage(message));
        break;
    }
    case TITLE_CHANGED: {
        BString title;
        if (message->FindString("title", &title) == B_OK)
            titleChanged(title, webViewForMessage(message));
        break;
    }
    case RESIZING_REQUESTED: {
        BRect rect;
        if (message->FindRect("rect", &rect) == B_OK)
            resizeRequested(rect.Width(), rect.Height(), webViewForMessage(message));
        break;
    }
    case SET_STATUS_TEXT: {
        BString text;
        if (message->FindString("text", &text) == B_OK)
            statusChanged(text, webViewForMessage(message));
        break;
    }
    case UPDATE_NAVIGATION_INTERFACE: {
        bool canGoBackward = false;
        bool canGoForward = false;
        bool canStop = false;
        message->FindBool("can go backward", &canGoBackward);
        message->FindBool("can go forward", &canGoForward);
        message->FindBool("can stop", &canStop);
        navigationCapabilitiesChanged(canGoBackward, canGoForward, canStop, webViewForMessage(message));
        break;
    }
    case AUTHENTICATION_CHALLENGE: {
        authenticationChallenge(message);
        break;
    }

    case TOOLBARS_VISIBILITY: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            setToolBarsVisible(flag, webViewForMessage(message));
        break;
    }
    case STATUSBAR_VISIBILITY: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            setStatusBarVisible(flag, webViewForMessage(message));
        break;
    }
    case MENUBAR_VISIBILITY: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            setMenuBarVisible(flag, webViewForMessage(message));
        break;
    }
    case SET_RESIZABLE: {
        bool flag;
        if (message->FindBool("flag", &flag) == B_OK)
            setResizable(flag, webViewForMessage(message));
        break;
    }

    case B_MOUSE_WHEEL_CHANGED:
        currentWebView()->MessageReceived(message);
        break;

    default:
        BWindow::MessageReceived(message);
        break;
    }
}

bool WebViewWindow::QuitRequested()
{
    // Do this here, so WebKit tear down happens earlier.
    if (WebView* view = currentWebView()) {
        view->RemoveSelf();
        delete view;
    }

    return true;
}

// #pragma mark -

void WebViewWindow::setCurrentWebView(WebView* view)
{
	m_webView = view;
}

void WebViewWindow::navigationRequested(const BString& url, WebView* view)
{
}

void WebViewWindow::newWindowRequested(const BString& url)
{
}

void WebViewWindow::loadNegotiating(const BString& url, WebView* view)
{
}

void WebViewWindow::loadTransferring(const BString& url, WebView* view)
{
}

void WebViewWindow::loadProgress(float progress, WebView* view)
{
}

void WebViewWindow::loadFailed(const BString& url, WebView* view)
{
}

void WebViewWindow::loadFinished(const BString& url, WebView* view)
{
}

void WebViewWindow::titleChanged(const BString& title, WebView* view)
{
    SetTitle(title.String());
}

void WebViewWindow::resizeRequested(float width, float height, WebView* view)
{
    ResizeTo(width, height);
}

void WebViewWindow::setToolBarsVisible(bool flag, WebView* view)
{
}

void WebViewWindow::setStatusBarVisible(bool flag, WebView* view)
{
}

void WebViewWindow::setMenuBarVisible(bool flag, WebView* view)
{
}

void WebViewWindow::setResizable(bool flag, WebView* view)
{
    if (flag)
        SetFlags(Flags() & ~B_NOT_RESIZABLE);
    else
        SetFlags(Flags() | B_NOT_RESIZABLE);
}

void WebViewWindow::statusChanged(const BString& statusText, WebView* view)
{
}

void WebViewWindow::navigationCapabilitiesChanged(bool canGoBackward,
    bool canGoForward, bool canStop, WebView* view)
{
}

void WebViewWindow::updateGlobalHistory(const BString& url)
{
}

void WebViewWindow::authenticationChallenge(BMessage* challenge)
{
}

// #pragma mark - private

WebView* WebViewWindow::webViewForMessage(const BMessage* message) const
{
	// Default to the current WebView if the message does not contain a view.
	WebView* view = m_webView;
	message->FindPointer("view", reinterpret_cast<void**>(&view));
	return view;
}

