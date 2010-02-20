/*
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

#ifndef LauncherWindow_h
#define LauncherWindow_h

#include "WebViewWindow.h"
#include <String.h>

class BButton;
class BCheckBox;
class BLayoutItem;
class BMenu;
class BStatusBar;
class BStringView;
class BTextControl;
class WebView;

enum ToolbarPolicy {
    HaveToolbar,
    DoNotHaveToolbar
};

enum {
	NEW_WINDOW = 'nwnd',
	WINDOW_OPENED = 'wndo',
	WINDOW_CLOSED = 'wndc',
    SHOW_DOWNLOAD_WINDOW = 'sdwd'
};

class LauncherWindow : public WebViewWindow {
public:
    LauncherWindow(BRect frame, const BMessenger& downloadListener,
        ToolbarPolicy = HaveToolbar);
    virtual ~LauncherWindow();

    virtual void MessageReceived(BMessage* message);
    virtual bool QuitRequested();
    virtual void MenusBeginning();

private:
    // WebPage notification API implementations
    virtual void navigationRequested(const BString& url, WebView* view);
    virtual void newWindowRequested(const BString& url);
    virtual void loadNegotiating(const BString& url, WebView* view);
    virtual void loadTransfering(const BString& url, WebView* view);
    virtual void loadProgress(float progress, WebView* view);
    virtual void loadFailed(const BString& url, WebView* view);
    virtual void loadFinished(const BString& url, WebView* view);
    virtual void titleChanged(const BString& title, WebView* view);
    virtual void resizeRequested(float width, float height, WebView* view);
    virtual void setToolBarsVisible(bool flag, WebView* view);
    virtual void setStatusBarVisible(bool flag, WebView* view);
    virtual void setMenuBarVisible(bool flag, WebView* view);
    virtual void setResizable(bool flag, WebView* view);
    virtual void statusChanged(const BString& status, WebView* view);
    virtual void navigationCapabilitiesChanged(bool canGoBackward,
        bool canGoForward, bool canStop, WebView* view);
    virtual void updateGlobalHistory(const BString& url);
    virtual void authenticationChallenge(BMessage* challenge);

private:
    BMenuBar* m_menuBar;
    BMenu* m_goMenu;
    BButton* m_BackButton;
    BButton* m_ForwardButton;
    BTextControl* m_url;
    BString m_loadedURL;
    BStringView* m_statusText;
    BStatusBar* m_loadingProgressBar;
    BLayoutItem* m_findGroup;
    BTextControl* m_findTextControl;
    BCheckBox* m_findCaseSensitiveCheckBox;
};

#endif // LauncherWindow_h

