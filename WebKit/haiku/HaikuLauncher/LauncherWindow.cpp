/*
 * Copyright (C) 2007 Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Michael Lotz <mmlr@mlotz.ch>
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
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
#include "BrowsingHistory.h"
#include "IconButton.h"
#include "LauncherApp.h"
#include "WebPage.h"
#include "WebTabView.h"
#include "WebView.h"
#include "WebViewConstants.h"
#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>

#include <stdio.h>

enum {
	OPEN_LOCATION = 'open',
    GO_BACK = 'goba',
    GO_FORWARD = 'gofo',
    STOP = 'stop',
    GOTO_URL = 'goul',
    RELOAD = 'reld',
    CLEAR_HISTORY = 'clhs',

    TEXT_SIZE_INCREASE = 'tsin',
    TEXT_SIZE_DECREASE = 'tsdc',
    TEXT_SIZE_RESET = 'tsrs',

    TEXT_SHOW_FIND_GROUP = 'sfnd',
    TEXT_HIDE_FIND_GROUP = 'hfnd',
    TEXT_FIND_NEXT = 'fndn',
    TEXT_FIND_PREVIOUS = 'fndp',
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
    : BWebWindow(frame, "HaikuLauncher",
        B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
        B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS)
    , m_downloadListener(downloadListener)
{
    BMessage* newTabMessage = new BMessage(NEW_TAB);
    newTabMessage->AddString("url", "");
    newTabMessage->AddPointer("window", this);
    newTabMessage->AddBool("select", true);
    m_tabManager = new TabManager(BMessenger(this), newTabMessage);

    if (toolbarPolicy == HaveToolbar) {
        // Menu
        m_menuBar = new BMenuBar("Main menu");
        BMenu* menu = new BMenu("Window");
        BMessage* newWindowMessage = new BMessage(NEW_WINDOW);
        newWindowMessage->AddString("url", "");
        BMenuItem* newItem = new BMenuItem("New window", newWindowMessage, 'N');
        menu->AddItem(newItem);
        newItem->SetTarget(be_app);
        newItem = new BMenuItem("New tab", new BMessage(*newTabMessage), 'T');
        menu->AddItem(newItem);
        newItem->SetTarget(be_app);
        menu->AddItem(new BMenuItem("Open location", new BMessage(OPEN_LOCATION), 'L'));
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Close window", new BMessage(B_QUIT_REQUESTED), 'W', B_SHIFT_KEY));
        menu->AddItem(new BMenuItem("Close tab", new BMessage(CLOSE_TAB), 'W'));
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Show downloads", new BMessage(SHOW_DOWNLOAD_WINDOW), 'J'));
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

        m_goMenu = new BMenu("Go");
        m_menuBar->AddItem(m_goMenu);

        // Back, Forward & Stop
        m_BackButton = new IconButton("Back", 0, NULL, new BMessage(GO_BACK));
        m_BackButton->SetIcon(201);
        m_BackButton->TrimIcon();

        m_ForwardButton = new IconButton("Forward", 0, NULL, new BMessage(GO_FORWARD));
        m_ForwardButton->SetIcon(202);
        m_ForwardButton->TrimIcon();

        m_StopButton = new IconButton("Stop", 0, NULL, new BMessage(STOP));
        m_StopButton->SetIcon(204);
        m_StopButton->TrimIcon();

        // URL
        m_url = new BTextControl("url", "", "", NULL);
        m_url->SetDivider(50.0);

        // Go
        m_goButton = new BButton("", "Go", new BMessage(GOTO_URL));

        // Status Bar
        m_statusText = new BStringView("status", "");
        m_statusText->SetAlignment(B_ALIGN_LEFT);
        m_statusText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
        m_statusText->SetExplicitMinSize(BSize(150, 12));
            // Prevent the window from growing to fit a long status message...
        BFont font(be_plain_font);
        font.SetSize(ceilf(font.Size() * 0.8));
        m_statusText->SetFont(&font, B_FONT_SIZE);

        // Loading progress bar
        m_loadingProgressBar = new BStatusBar("progress");
        m_loadingProgressBar->SetMaxValue(100);
        m_loadingProgressBar->Hide();
        m_loadingProgressBar->SetBarHeight(12);

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
            .Add(m_tabManager->TabGroup())
            .Add(BGridLayoutBuilder(kElementSpacing, kElementSpacing)
                .Add(m_BackButton, 0, 0)
                .Add(m_ForwardButton, 1, 0)
                .Add(m_StopButton, 2, 0)
                .Add(m_url, 3, 0)
                .Add(m_goButton, 4, 0)
                .SetInsets(kInsetSpacing, kInsetSpacing, kInsetSpacing, kInsetSpacing)
            )
            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
            .Add(m_tabManager->ContainerView())
            .Add(findGroup)
            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
            .Add(BGroupLayoutBuilder(B_HORIZONTAL, kElementSpacing)
                .Add(m_statusText)
                .Add(m_loadingProgressBar, 0.2)
                .AddStrut(12 - kElementSpacing)
                .SetInsets(kInsetSpacing, 0, kInsetSpacing, 0)
            )
        );

        m_url->MakeFocus(true);

        m_findGroup = layoutItemFor(findGroup);
        m_tabGroup = layoutItemFor(m_tabManager->TabGroup());
    } else {
        m_BackButton = 0;
        m_ForwardButton = 0;
        m_StopButton = 0;
        m_goButton = 0;
        m_url = 0;
        m_menuBar = 0;
        m_statusText = 0;
        m_loadingProgressBar = 0;

        BWebView* webView = new BWebView("web_view");
        SetCurrentWebView(webView);

        AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
            .Add(CurrentWebView())
        );
    }

    newTab("", true);

    m_findGroup->SetVisible(false);

    AddShortcut('G', B_COMMAND_KEY, new BMessage(TEXT_FIND_NEXT));
    AddShortcut('G', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_FIND_PREVIOUS));
    AddShortcut('F', B_COMMAND_KEY, new BMessage(TEXT_SHOW_FIND_GROUP));
    AddShortcut('F', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_HIDE_FIND_GROUP));
    AddShortcut('R', B_COMMAND_KEY, new BMessage(RELOAD));
	
    be_app->PostMessage(WINDOW_OPENED);
}

LauncherWindow::~LauncherWindow()
{
}

void LauncherWindow::DispatchMessage(BMessage* message, BHandler* target)
{
	if (m_url && message->what == B_KEY_DOWN && target == m_url->TextView()) {
		// Handle B_RETURN in the URL text control. This is the easiest
		// way to react *only* when the user presses the return key in the
		// address bar, as opposed to trying to load whatever is in there when
		// the text control just goes out of focus.
	    const char* bytes;
	    if (message->FindString("bytes", &bytes) == B_OK
	    	&& bytes[0] == B_RETURN) {
	    	// Do it in such a way that the user sees the Go-button go down.
	    	m_goButton->SetValue(B_CONTROL_ON);
	    	UpdateIfNeeded();
	    	m_goButton->Invoke();
	    	snooze(1000);
	    	m_goButton->SetValue(B_CONTROL_OFF);
	    }
	}
	BWebWindow::DispatchMessage(message, target);
}

void LauncherWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case OPEN_LOCATION:
        if (m_url) {
        	if (m_url->TextView()->IsFocus())
        	    m_url->TextView()->SelectAll();
        	else
        	    m_url->MakeFocus(true);
        }
    	break;
    case RELOAD:
        CurrentWebView()->Reload();
        break;
    case GOTO_URL: {
        BString url;
        if (message->FindString("url", &url) != B_OK)
        	url = m_url->Text();
        CurrentWebView()->LoadURL(url.String());
        break;
    }
    case GO_BACK:
        CurrentWebView()->GoBack();
        break;
    case GO_FORWARD:
        CurrentWebView()->GoForward();
        break;
    case STOP:
        CurrentWebView()->StopLoading();
        break;

    case CLEAR_HISTORY: {
        BrowsingHistory* history = BrowsingHistory::defaultInstance();
        if (history->countItems() == 0)
            break;
        BAlert* alert = new BAlert("Confirmation", "Do you really want to clear "
            "the browsing history?", "Clear", "Cancel");
        if (alert->Go() == 0)
            history->clear();
        break;
    }

    case B_SIMPLE_DATA: {
        // User possibly dropped files on this window.
        // If there is more than one entry_ref, let the app handle it (open one
        // new page per ref). If there is one ref, open it in this window.
        type_code type;
        int32 countFound;
        if (message->GetInfo("refs", &type, &countFound) != B_OK
            || type != B_REF_TYPE) {
            break;
        }
        if (countFound > 1) {
            message->what = B_REFS_RECEIVED;
            be_app->PostMessage(message);
            break;
        }
        entry_ref ref;
        if (message->FindRef("refs", &ref) != B_OK)
            break;
        BEntry entry(&ref, true);
        BPath path;
        if (!entry.Exists() || entry.GetPath(&path) != B_OK)
            break;
        CurrentWebView()->LoadURL(path.Path());
        break;
    }

    case TEXT_SIZE_INCREASE:
        CurrentWebView()->IncreaseTextSize();
        break;
    case TEXT_SIZE_DECREASE:
        CurrentWebView()->DecreaseTextSize();
        break;
    case TEXT_SIZE_RESET:
        CurrentWebView()->ResetTextSize();
        break;

    case TEXT_FIND_NEXT:
        CurrentWebView()->FindString(m_findTextControl->Text(), true,
            m_findCaseSensitiveCheckBox->Value());
        break;
    case TEXT_FIND_PREVIOUS:
        CurrentWebView()->FindString(m_findTextControl->Text(), false,
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

    case CLOSE_TAB:
        if (m_tabManager->CountTabs() > 1) {
	    	int32 index;
    		if (message->FindInt32("tab index", &index) != B_OK)
	    		index = m_tabManager->SelectedTabIndex();
            delete m_tabManager->RemoveTab(index);
            updateTabGroupVisibility();
        } else
            PostMessage(B_QUIT_REQUESTED);
        break;

    case TAB_CHANGED: {
    	// This message may be received also when the last tab closed, i.e. with index == -1.
        int32 index;
        if (message->FindInt32("tab index", &index) != B_OK)
            index = -1;
        BWebView* webView = dynamic_cast<BWebView*>(m_tabManager->ViewForTab(index));
        if (webView == CurrentWebView())
        	break;
        SetCurrentWebView(webView);
        if (webView)
            updateTitle(webView->MainFrameTitle());
        else
            updateTitle("");
        if (webView) {
            m_url->SetText(webView->MainFrameURL());
            // Trigger update of the interface to the new page, by requesting
            // to resend all notifications.
            webView->WebPage()->ResendNotifications();
        }
        break;
    }

    default:
        BWebWindow::MessageReceived(message);
        break;
    }
}

bool LauncherWindow::QuitRequested()
{
    // TODO: Check for modified form data and ask user for confirmation, etc.

    // Iterate over all tabs to delete all BWebViews.
    // Do this here, so WebKit tear down happens earlier.
    while (m_tabManager->CountTabs() > 0)
        delete m_tabManager->RemoveTab(0L);
    SetCurrentWebView(0);

    BMessage message(WINDOW_CLOSED);
    message.AddRect("window frame", Frame());
    be_app->PostMessage(&message);
    return true;
}

void LauncherWindow::MenusBeginning()
{
    BMenuItem* menuItem;
    while ((menuItem = m_goMenu->RemoveItem(0L)))
        delete menuItem;

    BrowsingHistory* history = BrowsingHistory::defaultInstance();
    if (!history->Lock())
        return;

    int32 count = history->countItems();
    for (int32 i = 0; i < count; i++) {
        BrowsingHistoryItem historyItem = history->historyItemAt(i);
        BMessage* message = new BMessage(GOTO_URL);
        message->AddString("url", historyItem.url().String());
        // TODO: More sophisticated menu structure... sorted by days/weeks...
        BString truncatedUrl(historyItem.url());
        be_plain_font->TruncateString(&truncatedUrl, B_TRUNCATE_END, 480);
        menuItem = new BMenuItem(truncatedUrl, message);
        m_goMenu->AddItem(menuItem);
    }


    if (m_goMenu->CountItems() > 3) {
        m_goMenu->AddSeparatorItem();
        m_goMenu->AddItem(new BMenuItem("Clear history", new BMessage(CLEAR_HISTORY)));
    }

    history->Unlock();
}

void LauncherWindow::newTab(const BString& url, bool select, BWebView* webView)
{
    // Executed in app thread (new BWebPage needs to be created in app thread).
    if (!webView)
        webView = new BWebView("web view");
    webView->WebPage()->SetDownloadListener(m_downloadListener);

    m_tabManager->AddTab(webView, "New tab");

    if (url.Length())
        webView->LoadURL(url.String());

    if (select) {
        m_tabManager->SelectTab(m_tabManager->CountTabs() - 1);
        SetCurrentWebView(webView);
        NavigationCapabilitiesChanged(false, false, false, webView);
        if (m_url) {
        	m_url->SetText(url.String());
            m_url->MakeFocus(true);
        }
    }

    updateTabGroupVisibility();
}

// #pragma mark - Notification API

void LauncherWindow::NavigationRequested(const BString& url, BWebView* view)
{
}

void LauncherWindow::NewWindowRequested(const BString& url, bool primaryAction)
{
    // Always open new windows in the application thread, since
    // creating a BWebView will try to grab the application lock.
    // But our own WebPage may already try to lock us from within
    // the application thread -> dead-lock. Thus we can't wait for
    // a reply here.
    BMessage message(NEW_TAB);
    message.AddPointer("window", this);
    message.AddString("url", url);
    message.AddBool("select", primaryAction);
    be_app->PostMessage(&message);
}

void LauncherWindow::NewPageCreated(BWebView* view)
{
    newTab(BString(), true, view);
}

void LauncherWindow::LoadNegotiating(const BString& url, BWebView* view)
{
    BString status("Requesting: ");
    status << url;
    StatusChanged(status, view);
}

void LauncherWindow::LoadCommitted(const BString& url, BWebView* view)
{
    if (view != CurrentWebView())
        return;

	// This hook is invoked when the load is commited.
    if (m_url)
        m_url->SetText(url.String());

    BString status("Loading: ");
    status << url;
    StatusChanged(status, view);
}

void LauncherWindow::LoadProgress(float progress, BWebView* view)
{
    if (view != CurrentWebView())
        return;

    if (m_loadingProgressBar) {
        if (progress < 100 && m_loadingProgressBar->IsHidden())
            m_loadingProgressBar->Show();
        m_loadingProgressBar->SetTo(progress);
    }
}

void LauncherWindow::LoadFailed(const BString& url, BWebView* view)
{
    if (view != CurrentWebView())
        return;

    BString status(url);
    status << " failed.";
    StatusChanged(status, view);
    if (m_loadingProgressBar && !m_loadingProgressBar->IsHidden())
        m_loadingProgressBar->Hide();
}

void LauncherWindow::LoadFinished(const BString& url, BWebView* view)
{
    if (view != CurrentWebView())
        return;

    BString status(url);
    status << " finished.";
    StatusChanged(status, view);
    if (m_loadingProgressBar && !m_loadingProgressBar->IsHidden())
        m_loadingProgressBar->Hide();

    NavigationCapabilitiesChanged(m_BackButton->IsEnabled(),
        m_ForwardButton->IsEnabled(), false, view);
}

void LauncherWindow::ResizeRequested(float width, float height, BWebView* view)
{
    if (view != CurrentWebView())
        return;

    // TODO: Ignore request when there is more than one BWebView embedded!

    ResizeTo(width, height);
}

void LauncherWindow::SetToolBarsVisible(bool flag, BWebView* view)
{
    // TODO
    // TODO: Ignore request when there is more than one BWebView embedded!
}

void LauncherWindow::SetStatusBarVisible(bool flag, BWebView* view)
{
    // TODO
    // TODO: Ignore request when there is more than one BWebView embedded!
}

void LauncherWindow::SetMenuBarVisible(bool flag, BWebView* view)
{
    // TODO
    // TODO: Ignore request when there is more than one BWebView embedded!
}

void LauncherWindow::SetResizable(bool flag, BWebView* view)
{
    // TODO: Ignore request when there is more than one BWebView embedded!

    if (flag)
        SetFlags(Flags() & ~B_NOT_RESIZABLE);
    else
        SetFlags(Flags() | B_NOT_RESIZABLE);
}

void LauncherWindow::TitleChanged(const BString& title, BWebView* view)
{
    for (int32 i = 0; i < m_tabManager->CountTabs(); i++) {
        if (m_tabManager->ViewForTab(i) == view) {
            m_tabManager->SetTabLabel(i, title);
            break;
        }
    }
    if (view != CurrentWebView())
        return;

    updateTitle(title);
}

void LauncherWindow::StatusChanged(const BString& statusText, BWebView* view)
{
    if (view != CurrentWebView())
        return;

    if (m_statusText)
        m_statusText->SetText(statusText.String());
}

void LauncherWindow::NavigationCapabilitiesChanged(bool canGoBackward,
    bool canGoForward, bool canStop, BWebView* view)
{
    if (view != CurrentWebView())
        return;

    if (m_BackButton)
        m_BackButton->SetEnabled(canGoBackward);
    if (m_ForwardButton)
        m_ForwardButton->SetEnabled(canGoForward);
    if (m_StopButton)
        m_StopButton->SetEnabled(canStop);
}

void LauncherWindow::UpdateGlobalHistory(const BString& url)
{
    BrowsingHistory::defaultInstance()->addItem(BrowsingHistoryItem(url));
}

bool LauncherWindow::AuthenticationChallenge(BString message, BString& inOutUser,
	BString& inOutPassword, bool& inOutRememberCredentials, uint32 failureCount,
	BWebView* view)
{
	// Switch to the page for which this authentication is required.
	if (view != CurrentWebView()) {
		m_tabManager->SelectTab(view);
		UpdateIfNeeded();
	}
    AuthenticationPanel* panel = new AuthenticationPanel(Frame());
    	// Panel auto-destructs.
    return panel->getAuthentication(message, inOutUser, inOutPassword,
    	inOutRememberCredentials, failureCount > 0, inOutUser, inOutPassword,
    	&inOutRememberCredentials);
}

void LauncherWindow::updateTitle(const BString& title)
{
    BString windowTitle = title;
    if (windowTitle.Length() > 0)
        windowTitle << " - ";
    windowTitle << "HaikuLauncher";
    SetTitle(windowTitle.String());
}

void LauncherWindow::updateTabGroupVisibility()
{
	if (Lock()) {
	    //m_tabGroup->SetVisible(m_tabManager->CountTabs() > 1);
	    m_tabManager->SetCloseButtonsAvailable(m_tabManager->CountTabs() > 1);
	    Unlock();
	}
}
