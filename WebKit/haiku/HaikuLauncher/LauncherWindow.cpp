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
#include "LauncherWindow.h"

#include "AuthenticationPanel.h"
#include "WebProcess.h"
#include "WebView.h"
#include "WebViewConstants.h"
#include <Application.h>
#include <Button.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>

#include <stdio.h>

enum {
    GO_BACK = 'goba',
    GO_FORWARD = 'gofo',
    GOTO_URL = 'goul',
    RELOAD = 'reld',

	TEXT_SIZE_INCREASE = 'tsin',
	TEXT_SIZE_DECREASE = 'tsdc',
	TEXT_SIZE_RESET = 'tsrs',

    TEXT_SHOW_FIND_GROUP = 'sfnd',
    TEXT_HIDE_FIND_GROUP = 'hfnd',
    TEXT_FIND_NEXT = 'fndn',
    TEXT_FIND_PREVIOUS = 'fndp',

    TEST_AUTHENTICATION_PANEL = 'tapn'
};

using namespace WebCore;

static BLayoutItem* layoutItemFor(BView* view)
{
	BLayout* layout = view->Parent()->GetLayout();
	int32 index = layout->IndexOfView(view);
	return layout->ItemAt(index);
}

LauncherWindow::LauncherWindow(BRect frame, const BMessenger& downloadListener,
        ToolbarPolicy toolbarPolicy)
    : WebViewWindow(frame, "HaikuLauncher",
        B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
        B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS)
{
    if (toolbarPolicy == HaveToolbar) {
    	// Menu
    	m_menuBar = new BMenuBar("Main menu");
    	BMenu* menu = new BMenu("Window");
    	BMessage* newWindowMessage = new BMessage(NEW_WINDOW);
    	newWindowMessage->AddString("url", "");
    	BMenuItem* newItem = new BMenuItem("New", newWindowMessage, 'N');
    	menu->AddItem(newItem);
    	newItem->SetTarget(be_app);
    	menu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
    	menu->AddSeparatorItem();
    	menu->AddItem(new BMenuItem("Show Downloads", new BMessage(SHOW_DOWNLOAD_WINDOW), 'D'));
    	menu->AddSeparatorItem();
    	BMenuItem* quitItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
    	menu->AddItem(quitItem);
    	quitItem->SetTarget(be_app);
    	m_menuBar->AddItem(menu);

    	menu = new BMenu("Text");
    	menu->AddItem(new BMenuItem("Find", new BMessage(TEXT_SHOW_FIND_GROUP), 'F'));
    	menu->AddSeparatorItem();
    	menu->AddItem(new BMenuItem("Increase size", new BMessage(TEXT_SIZE_INCREASE), '+'));
    	menu->AddItem(new BMenuItem("Decrease size", new BMessage(TEXT_SIZE_DECREASE), '-'));
    	menu->AddItem(new BMenuItem("Reset size", new BMessage(TEXT_SIZE_RESET), '0'));
    	m_menuBar->AddItem(menu);

        // Back & Forward
        m_BackButton = new BButton("", "Back", new BMessage(GO_BACK));
        m_ForwardButton = new BButton("", "Forward", new BMessage(GO_FORWARD));

        // URL
        m_url = new BTextControl("url", "", "", new BMessage(GOTO_URL));
        m_url->SetDivider(50.0);

        // Go
        BButton* button = new BButton("", "Go", new BMessage(RELOAD));

        // Status Bar
        m_statusText = new BStringView("status", "");
        m_statusText->SetAlignment(B_ALIGN_LEFT);
        m_statusText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
        m_statusText->SetExplicitMinSize(BSize(150, 16));
        	// Prevent the window from growing to fit a long status message...
        BFont font(be_plain_font);
        font.SetSize(ceilf(font.Size() * 0.8));
        m_statusText->SetFont(&font, B_FONT_SIZE);

		// Loading progress bar
		m_loadingProgressBar = new BStatusBar("progress");
		m_loadingProgressBar->SetMaxValue(100);
		m_loadingProgressBar->Hide();
		m_loadingProgressBar->SetBarHeight(10);

        const float kInsetSpacing = 5;
        const float kElementSpacing = 7;

        m_findTextControl = new BTextControl("find", "Find:", "",
            new BMessage(TEXT_FIND_NEXT));
        m_findCaseSensitiveCheckBox = new BCheckBox("Match case");
        BView* findGroup = BGroupLayoutBuilder(B_VERTICAL)
            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
            .Add(BGroupLayoutBuilder(B_HORIZONTAL, kElementSpacing)
                .Add(m_findTextControl)
                .Add(new BButton("Previous", new BMessage(TEXT_FIND_PREVIOUS)))
                .Add(new BButton("Next", new BMessage(TEXT_FIND_NEXT)))
                .Add(m_findCaseSensitiveCheckBox)
                .Add(BSpaceLayoutItem::CreateGlue())
                .Add(new BButton("Close", new BMessage(TEXT_HIDE_FIND_GROUP)))
                .SetInsets(kInsetSpacing, kInsetSpacing,
                    kInsetSpacing, kInsetSpacing)
            )
        ;
        // Layout
        AddChild(BGroupLayoutBuilder(B_VERTICAL)
        	.Add(m_menuBar)
            .Add(BGridLayoutBuilder(kElementSpacing, kElementSpacing)
                .Add(m_BackButton, 0, 0)
                .Add(m_ForwardButton, 1, 0)
                .Add(m_url, 2, 0)
                .Add(button, 3, 0)
                .SetInsets(kInsetSpacing, kInsetSpacing, kInsetSpacing, kInsetSpacing)
            )
            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
            .Add(webView())
            .Add(findGroup)
            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
            .Add(BGroupLayoutBuilder(B_HORIZONTAL, kElementSpacing)
                .Add(m_statusText)
                .Add(m_loadingProgressBar, 0.2)
                .SetInsets(kInsetSpacing, 0, kInsetSpacing, 0)
            )
        );

        m_url->MakeFocus(true);

        m_findGroup = layoutItemFor(findGroup);
    } else {
    	m_BackButton = 0;
    	m_ForwardButton = 0;
        m_url = 0;
        m_menuBar = 0;
        m_statusText = 0;
        m_loadingProgressBar = 0;

        AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
            .Add(webView())
        );
    }

    webView()->webProcess()->setDownloadListener(downloadListener);

    m_findGroup->SetVisible(false);

    AddShortcut('G', B_COMMAND_KEY, new BMessage(TEXT_FIND_NEXT));
    AddShortcut('G', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_FIND_PREVIOUS));
    AddShortcut('F', B_COMMAND_KEY, new BMessage(TEXT_SHOW_FIND_GROUP));
    AddShortcut('F', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_HIDE_FIND_GROUP));
    AddShortcut('T', B_COMMAND_KEY, new BMessage(TEST_AUTHENTICATION_PANEL));

    navigationCapabilitiesChanged(false, false, false);

	be_app->PostMessage(WINDOW_OPENED);
}

LauncherWindow::~LauncherWindow()
{
}

void LauncherWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case RELOAD:
        webView()->loadRequest(m_url->Text());
        break;
    case GOTO_URL:
        if (m_loadedURL != m_url->Text())
            webView()->loadRequest(m_url->Text());
        break;
    case GO_BACK:
        webView()->goBack();
        break;
    case GO_FORWARD:
        webView()->goForward();
        break;

    case TEXT_SIZE_INCREASE:
        webView()->increaseTextSize();
        break;
    case TEXT_SIZE_DECREASE:
        webView()->decreaseTextSize();
        break;
    case TEXT_SIZE_RESET:
        webView()->resetTextSize();
        break;

    case TEXT_FIND_NEXT:
        webView()->findString(m_findTextControl->Text(), true,
            m_findCaseSensitiveCheckBox->Value());
        break;
    case TEXT_FIND_PREVIOUS:
        webView()->findString(m_findTextControl->Text(), false,
            m_findCaseSensitiveCheckBox->Value());
        break;
    case TEXT_SHOW_FIND_GROUP:
        if (!m_findGroup->IsVisible())
            m_findGroup->SetVisible(true);
        m_findTextControl->MakeFocus(true);
        break;
    case TEXT_HIDE_FIND_GROUP:
        if (m_findGroup->IsVisible())
            m_findGroup->SetVisible(false);
        break;

    case SHOW_DOWNLOAD_WINDOW:
        message->AddUInt32("workspaces", Workspaces());
        be_app->PostMessage(message);
        break;

    case TEST_AUTHENTICATION_PANEL: {
    	BString realm("Realm");
    	BString method("Method");
    	BString previousUser;
    	BString previousPass;
    	bool rememberCredentials = false;
    	bool badPassword = true;
    	BString user;
    	BString pass;
        AuthenticationPanel* panel = new AuthenticationPanel(Frame());
        bool success = panel->getAuthentication(realm, method, previousUser,
            previousPass, rememberCredentials, badPassword, user, pass,
            &rememberCredentials);
        printf("success: %d\n", success);
        break;
    }

    default:
        WebViewWindow::MessageReceived(message);
        break;
    }
}

bool LauncherWindow::QuitRequested()
{
    if (WebViewWindow::QuitRequested()) {
    	BMessage message(WINDOW_CLOSED);
    	message.AddRect("window frame", Frame());
        be_app->PostMessage(&message);
        return true;
    }
    return false;
}

// #pragma mark - Notification API

void LauncherWindow::navigationRequested(const BString& url)
{
	m_loadedURL = url;
    if (m_url)
        m_url->SetText(url.String());
}

void LauncherWindow::newWindowRequested(const BString& url)
{
	// Always open new windows in the application thread, since
	// creating a WebView will try to grab the application lock.
	// But our own WebProcess may already try to lock us from within
	// the application thread -> dead-lock.
	BMessage message(NEW_WINDOW);
	message.AddString("url", url);
	be_app->PostMessage(&message);
}

void LauncherWindow::loadNegociating(const BString& url)
{
	BString status("Requesting: ");
	status << url;
	statusChanged(status);
}

void LauncherWindow::loadTransfering(const BString& url)
{
	BString status("Loading: ");
	status << url;
	statusChanged(status);
}

void LauncherWindow::loadProgress(float progress)
{
	if (m_loadingProgressBar) {
	    if (progress < 100 && m_loadingProgressBar->IsHidden())
	        m_loadingProgressBar->Show();
	    m_loadingProgressBar->SetTo(progress);
	}
}

void LauncherWindow::loadFailed(const BString& url)
{
	BString status(url);
	status << " failed.";
	statusChanged(status);
	if (m_loadingProgressBar && !m_loadingProgressBar->IsHidden())
	    m_loadingProgressBar->Hide();
}

void LauncherWindow::loadFinished(const BString& url)
{
	m_loadedURL = url;
	BString status(url);
	status << " finished.";
	statusChanged(status);
    if (m_url)
        m_url->SetText(url.String());
	if (m_loadingProgressBar && !m_loadingProgressBar->IsHidden())
	    m_loadingProgressBar->Hide();
}

void LauncherWindow::titleChanged(const BString& title)
{
	BString windowTitle = title;
	if (windowTitle.Length() > 0)
		windowTitle << " - ";
	windowTitle << "HaikuLauncher";
	SetTitle(windowTitle.String());
}

void LauncherWindow::statusChanged(const BString& statusText)
{
	if (m_statusText)
        m_statusText->SetText(statusText.String());
}

void LauncherWindow::navigationCapabilitiesChanged(bool canGoBackward,
    bool canGoForward, bool canStop)
{
	if (m_BackButton)
	    m_BackButton->SetEnabled(canGoBackward);
	if (m_ForwardButton)
	    m_ForwardButton->SetEnabled(canGoForward);
}
