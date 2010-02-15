/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
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

#ifndef WebViewWindow_h
#define WebViewWindow_h

#include <Window.h>

class WebView;

class WebViewWindow : public BWindow {
public:
    WebViewWindow(BRect frame, const char* name, window_look look,
        window_feel feel, uint32 flags,
        uint32 workspace = B_CURRENT_WORKSPACE);
    virtual ~WebViewWindow();

    virtual void MessageReceived(BMessage* message);
    virtual bool QuitRequested();

    WebView* webView() const { return m_webView; }

    // Derived windows should implement this notification API
    virtual void navigationRequested(const BString& url);
    virtual void newWindowRequested(const BString& url);
    virtual void loadNegociating(const BString& url);
    virtual void loadTransfering(const BString& url);
    virtual void loadProgress(float progress);
    virtual void loadFailed(const BString& url);
    virtual void loadFinished(const BString& url);
    virtual void titleChanged(const BString& title);
    virtual void resizeRequested(float width, float height);
    virtual void statusChanged(const BString& status);
    virtual void navigationCapabilitiesChanged(bool canGoBackward,
        bool canGoForward, bool canStop);
    virtual void updateGlobalHistory(const BString& url);
    virtual void authenticationChallenge(BMessage* challenge);

private:
    WebView* m_webView;
};

#endif // WebViewWindow_h

