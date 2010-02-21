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
    GO_BACK = 'goba',
    GO_FORWARD = 'gofo',
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

    CLOSE_TAB = 'ctab'
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
    m_tabView = new WebTabView("tabview", BMessenger(this));
    m_tabView->setTarget(BMessenger(this));

    if (toolbarPolicy == HaveToolbar) {
        // Menu
        m_menuBar = new BMenuBar("Main menu");
        BMenu* menu = new BMenu("Window");
        BMessage* newWindowMessage = new BMessage(NEW_WINDOW);
        newWindowMessage->AddString("url", "");
        BMenuItem* newItem = new BMenuItem("New window", newWindowMessage, 'N');
        menu->AddItem(newItem);
        newItem->SetTarget(be_app);
        BMessage* newTabMessage = new BMessage(NEW_TAB);
        newTabMessage->AddString("url", "");
        newTabMessage->AddPointer("window", this);
        newItem = new BMenuItem("New tab", newTabMessage, 'T');
        menu->AddItem(newItem);
        newItem->SetTarget(be_app);
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Close window", new BMessage(B_QUIT_REQUESTED), 'W'));
        menu->AddItem(new BMenuItem("Close tab", new BMessage(CLOSE_TAB), 'W', B_SHIFT_KEY));
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Show downloads", new BMessage(SHOW_DOWNLOAD_WINDOW), 'D'));
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

        // Back & Forward
        m_BackButton = new IconButton("Back", 0, NULL, new BMessage(GO_BACK));
        m_BackButton->SetIcon(201);
        m_BackButton->TrimIcon();

        m_ForwardButton = new IconButton("Forward", 0, NULL, new BMessage(GO_FORWARD));
        m_ForwardButton->SetIcon(202);
        m_ForwardButton->TrimIcon();

        // URL
        m_url = new BTextControl("url", "", "", new BMessage(GOTO_URL));
        m_url->SetDivider(50.0);

        // Go
        BButton* button = new BButton("", "Go", new BMessage(RELOAD));

        // Status Bar
        m_statusText = new BStringView("status", "");
        m_statusText->SetAlignment(B_ALIGN_LEFT);
        m_statusText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
        m_statusText->SetExplicitMinSize(BSize(150, 15));
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
//            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
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
//            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
            .Add(m_tabView)
            .Add(findGroup)
//            .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
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

        WebView* webView = new WebView("web_view");
        setCurrentWebView(webView);

        AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
            .Add(currentWebView())
        );
    }

    newTab("", true);
    currentWebView()->webPage()->SetDownloadListener(downloadListener);

    m_findGroup->SetVisible(false);

    AddShortcut('G', B_COMMAND_KEY, new BMessage(TEXT_FIND_NEXT));
    AddShortcut('G', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_FIND_PREVIOUS));
    AddShortcut('F', B_COMMAND_KEY, new BMessage(TEXT_SHOW_FIND_GROUP));
    AddShortcut('F', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_HIDE_FIND_GROUP));

    be_app->PostMessage(WINDOW_OPENED);
}

LauncherWindow::~LauncherWindow()
{
}

void LauncherWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case RELOAD:
        currentWebView()->loadRequest(m_url->Text());
        break;
    case GOTO_URL: {
        BString url = m_url->Text();
        message->FindString("url", &url);
        if (m_loadedURL != url)
            currentWebView()->loadRequest(url.String());
        break;
    }
    case GO_BACK:
        currentWebView()->goBack();
        break;
    case GO_FORWARD:
        currentWebView()->goForward();
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
        currentWebView()->loadRequest(path.Path());
        break;
    }

    case TEXT_SIZE_INCREASE:
        currentWebView()->increaseTextSize();
        break;
    case TEXT_SIZE_DECREASE:
        currentWebView()->decreaseTextSize();
        break;
    case TEXT_SIZE_RESET:
        currentWebView()->resetTextSize();
        break;

    case TEXT_FIND_NEXT:
        currentWebView()->findString(m_findTextControl->Text(), true,
            m_findCaseSensitiveCheckBox->Value());
        break;
    case TEXT_FIND_PREVIOUS:
        currentWebView()->findString(m_findTextControl->Text(), false,
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

    case CLOSE_TAB: {
    	printf("CLOSE_TAB\n");
        if (m_tabView->CountTabs() > 1)
            delete m_tabView->RemoveTab(m_tabView->FocusTab());
        break;
    }

    case TAB_CHANGED: {
        int32 index = message->FindInt32("index");
        setCurrentWebView(dynamic_cast<WebView*>(m_tabView->ViewForTab(index)));
        updateTitle(m_tabView->TabAt(index)->Label());
        m_url->TextView()->SetText(currentWebView()->mainFrameURL());
        // Trigger update of the interface to the new page, by requesting
        // to resend all notifications.
        currentWebView()->webPage()->ResendNotifications();
        break;
    }

    default:
        WebViewWindow::MessageReceived(message);
        break;
    }
}

bool LauncherWindow::QuitRequested()
{
    // TODO: Check for modified form data and ask user for confirmation, etc.

    // Do this here, so WebKit tear down happens earlier.
    // TODO: Iterator over all WebViews, if there are more then one...
    while (m_tabView->CountTabs() > 0)
        delete m_tabView->RemoveTab(0L);
    setCurrentWebView(0);

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

void LauncherWindow::newTab(const BString& url, bool select)
{
	// Executed in app thread (new BWebPage needs to be created in app thread).
    WebView* webView = new WebView("web_view");
    m_tabView->AddTab(webView);
    m_tabView->TabAt(m_tabView->CountTabs() - 1)->SetLabel("New tab");

	if (url.Length())
	    webView->loadRequest(url.String());

    if (select) {
	    m_tabView->Select(m_tabView->CountTabs() - 1);
		setCurrentWebView(webView);
        navigationCapabilitiesChanged(false, false, false, webView);
        if (m_url)
            m_url->MakeFocus(true);
    }
}

// #pragma mark - Notification API

void LauncherWindow::navigationRequested(const BString& url, WebView* view)
{
    // TODO: Move elsewhere, doesn't belong here.
    m_loadedURL = url;
    if (m_url)
        m_url->SetText(url.String());
}

void LauncherWindow::newWindowRequested(const BString& url)
{
    // Always open new windows in the application thread, since
    // creating a WebView will try to grab the application lock.
    // But our own WebPage may already try to lock us from within
    // the application thread -> dead-lock. Thus we can't wait for
    // a reply here.
    BMessage message(NEW_TAB);
    message.AddPointer("window", this);
    message.AddString("url", url);
    message.AddBool("select", false);
    be_app->PostMessage(&message);
}

void LauncherWindow::loadNegotiating(const BString& url, WebView* view)
{
    BString status("Requesting: ");
    status << url;
    statusChanged(status, view);
}

void LauncherWindow::loadTransfering(const BString& url, WebView* view)
{
    BString status("Loading: ");
    status << url;
    statusChanged(status, view);
}

void LauncherWindow::loadProgress(float progress, WebView* view)
{
    if (view != currentWebView())
        return;

    if (m_loadingProgressBar) {
        if (progress < 100 && m_loadingProgressBar->IsHidden())
            m_loadingProgressBar->Show();
        m_loadingProgressBar->SetTo(progress);
    }
}

void LauncherWindow::loadFailed(const BString& url, WebView* view)
{
    if (view != currentWebView())
        return;

    BString status(url);
    status << " failed.";
    statusChanged(status, view);
    if (m_loadingProgressBar && !m_loadingProgressBar->IsHidden())
        m_loadingProgressBar->Hide();
}

void LauncherWindow::loadFinished(const BString& url, WebView* view)
{
    if (view != currentWebView())
        return;

    m_loadedURL = url;
    BString status(url);
    status << " finished.";
    statusChanged(status, view);
    if (m_loadingProgressBar && !m_loadingProgressBar->IsHidden())
        m_loadingProgressBar->Hide();
}

void LauncherWindow::resizeRequested(float width, float height, WebView* view)
{
    if (view != currentWebView())
        return;

    // TODO: Ignore request when there is more than one WebView embedded!

    ResizeTo(width, height);
}

void LauncherWindow::setToolBarsVisible(bool flag, WebView* view)
{
    // TODO
    // TODO: Ignore request when there is more than one WebView embedded!
}

void LauncherWindow::setStatusBarVisible(bool flag, WebView* view)
{
    // TODO
    // TODO: Ignore request when there is more than one WebView embedded!
}

void LauncherWindow::setMenuBarVisible(bool flag, WebView* view)
{
    // TODO
    // TODO: Ignore request when there is more than one WebView embedded!
}

void LauncherWindow::setResizable(bool flag, WebView* view)
{
    // TODO: Ignore request when there is more than one WebView embedded!

    if (flag)
        SetFlags(Flags() & ~B_NOT_RESIZABLE);
    else
        SetFlags(Flags() | B_NOT_RESIZABLE);
}

void LauncherWindow::titleChanged(const BString& title, WebView* view)
{
    for (int32 i = 0; i < m_tabView->CountTabs(); i++) {
        if (m_tabView->ViewForTab(i) == view) {
            m_tabView->TabAt(i)->SetLabel(title);
			m_tabView->DrawTabs();
            break;
        }
    }
    if (view != currentWebView())
        return;

    updateTitle(title);
}

void LauncherWindow::statusChanged(const BString& statusText, WebView* view)
{
    if (view != currentWebView())
        return;

    if (m_statusText)
        m_statusText->SetText(statusText.String());
}

void LauncherWindow::navigationCapabilitiesChanged(bool canGoBackward,
    bool canGoForward, bool canStop, WebView* view)
{
    if (view != currentWebView())
        return;

    if (m_BackButton)
        m_BackButton->SetEnabled(canGoBackward);
    if (m_ForwardButton)
        m_ForwardButton->SetEnabled(canGoForward);
}

void LauncherWindow::updateGlobalHistory(const BString& url)
{
    BrowsingHistory::defaultInstance()->addItem(BrowsingHistoryItem(url));
}

void LauncherWindow::authenticationChallenge(BMessage* message)
{
    BString text;
    bool rememberCredentials = false;
    uint32 failureCount = 0;
    BString user;
    BString password;

    message->FindString("text", &text);
    message->FindString("user", &user);
    message->FindString("password", &password);
    message->FindUInt32("failureCount", &failureCount);

    AuthenticationPanel* panel = new AuthenticationPanel(Frame());
    if (!panel->getAuthentication(text, user, password, rememberCredentials,
            failureCount > 0, user, password, &rememberCredentials)) {
        message->SendReply((uint32)0);
        return;
    }

    BMessage reply;
    reply.AddString("user", user);
    reply.AddString("password", password);
    reply.AddBool("rememberCredentials", rememberCredentials);
    message->SendReply(&reply);
}

void LauncherWindow::updateTitle(const BString& title)
{
    BString windowTitle = title;
    if (windowTitle.Length() > 0)
        windowTitle << " - ";
    windowTitle << "HaikuLauncher";
    SetTitle(windowTitle.String());
}
