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
#include "BrowserWindow.h"

#include "AuthenticationPanel.h"
#include "BrowserApp.h"
#include "BrowsingHistory.h"
#include "IconButton.h"
#include "TextControlCompleter.h"
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

	SELECT_TAB = 'sltb',
};


static BLayoutItem*
layoutItemFor(BView* view)
{
	BLayout* layout = view->Parent()->GetLayout();
	int32 index = layout->IndexOfView(view);
	return layout->ItemAt(index);
}


class URLChoice : public BAutoCompleter::Choice {
public:
	URLChoice(const BString& choiceText, const BString& displayText,
			int32 matchPos, int32 matchLen, int32 priority)
		:
		BAutoCompleter::Choice(choiceText, displayText, matchPos, matchLen),
		fPriority(priority)
	{
	}

	bool operator<(const URLChoice& other) const
	{
		if (fPriority > other.fPriority)
			return true;
		return DisplayText() < other.DisplayText();
	}

	bool operator==(const URLChoice& other) const
	{
		return fPriority == other.fPriority
			&& DisplayText() < other.DisplayText();
	}

private:
	int32 fPriority;
};


class BrowsingHistoryChoiceModel : public BAutoCompleter::ChoiceModel {
	virtual void FetchChoicesFor(const BString& pattern)
	{
		int32 count = CountChoices();
		for (int32 i = 0; i < count; i++) {
			delete reinterpret_cast<BAutoCompleter::Choice*>(
				fChoices.ItemAtFast(i));
		}
		fChoices.MakeEmpty();

		// Search through BrowsingHistory for any matches.
		BrowsingHistory* history = BrowsingHistory::defaultInstance();
		if (!history->Lock())
			return;

		BString lastBaseURL;
		int32 priority = INT_MAX;

		count = history->countItems();
		for (int32 i = 0; i < count; i++) {
			BrowsingHistoryItem item = history->historyItemAt(i);
			const BString& choiceText = item.url();
			int32 matchPos = choiceText.IFindFirst(pattern);
			if (matchPos < 0)
				continue;
			if (lastBaseURL.Length() > 0
				&& choiceText.FindFirst(lastBaseURL) >= 0) {
				priority--;
			} else
				priority = INT_MAX;
			int32 baseURLStart = choiceText.FindFirst("://") + 3;
			int32 baseURLEnd = choiceText.FindFirst("/", baseURLStart + 1);
			lastBaseURL.SetTo(choiceText.String() + baseURLStart,
				baseURLEnd - baseURLStart);
			fChoices.AddItem(new URLChoice(choiceText,
				choiceText, matchPos, pattern.Length(), priority));
		}

		history->Unlock();

		fChoices.SortItems(_CompareChoices);
	}

	virtual int32 CountChoices() const
	{
		return fChoices.CountItems();
	}

	virtual const BAutoCompleter::Choice* ChoiceAt(int32 index) const
	{
		return reinterpret_cast<BAutoCompleter::Choice*>(
			fChoices.ItemAt(index));
	}

	static int _CompareChoices(const void* a, const void* b)
	{
		const URLChoice* aChoice = *reinterpret_cast<const URLChoice* const *>(a);
		const URLChoice* bChoice = *reinterpret_cast<const URLChoice* const *>(b);
		if (*aChoice < *bChoice)
			return -1;
		else if (*aChoice == *bChoice)
			return 0;
		return 1;
	}

private:
	BList fChoices;
};


// #pragma mark - BrowserWindow


BrowserWindow::BrowserWindow(BRect frame, const BMessenger& downloadListener,
		ToolbarPolicy toolbarPolicy)
	: BWebWindow(frame, kApplicationName,
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS)
	, fDownloadListener(downloadListener)
{
	BMessage* newTabMessage = new BMessage(NEW_TAB);
	newTabMessage->AddString("url", "");
	newTabMessage->AddPointer("window", this);
	newTabMessage->AddBool("select", true);
	fTabManager = new TabManager(BMessenger(this), newTabMessage);

	if (toolbarPolicy == HaveToolbar) {
		// Menu
#if INTEGRATE_MENU_INTO_TAB_BAR
		BMenu* mainMenu = fTabManager->Menu();
#else
		BMenu* mainMenu = new BMenuBar("Main menu");
#endif
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
		menu->AddItem(new BMenuItem("Show settings", new BMessage(SHOW_SETTINGS_WINDOW)));
		menu->AddSeparatorItem();
		BMenuItem* quitItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
		menu->AddItem(quitItem);
		quitItem->SetTarget(be_app);
		mainMenu->AddItem(menu);

		menu = new BMenu("Text");
		menu->AddItem(new BMenuItem("Find", new BMessage(TEXT_SHOW_FIND_GROUP), 'F'));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Increase size", new BMessage(TEXT_SIZE_INCREASE), '+'));
		menu->AddItem(new BMenuItem("Decrease size", new BMessage(TEXT_SIZE_DECREASE), '-'));
		menu->AddItem(new BMenuItem("Reset size", new BMessage(TEXT_SIZE_RESET), '0'));
		mainMenu->AddItem(menu);

		fGoMenu = new BMenu("Go");
		mainMenu->AddItem(fGoMenu);

		// Back, Forward & Stop
		fBackButton = new IconButton("Back", 0, NULL, new BMessage(GO_BACK));
		fBackButton->SetIcon(201);
		fBackButton->TrimIcon();

		fForwardButton = new IconButton("Forward", 0, NULL, new BMessage(GO_FORWARD));
		fForwardButton->SetIcon(202);
		fForwardButton->TrimIcon();

		fStopButton = new IconButton("Stop", 0, NULL, new BMessage(STOP));
		fStopButton->SetIcon(204);
		fStopButton->TrimIcon();

		// URL
		fURLTextControl = new BTextControl("url", "", "", NULL);
		fURLTextControl->SetDivider(50.0);

		// Go
		fGoButton = new BButton("", "Go", new BMessage(GOTO_URL));

		// Status Bar
		fStatusText = new BStringView("status", "");
		fStatusText->SetAlignment(B_ALIGN_LEFT);
		fStatusText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
		fStatusText->SetExplicitMinSize(BSize(150, 12));
			// Prevent the window from growing to fit a long status message...
		BFont font(be_plain_font);
		font.SetSize(ceilf(font.Size() * 0.8));
		fStatusText->SetFont(&font, B_FONT_SIZE);

		// Loading progress bar
		fLoadingProgressBar = new BStatusBar("progress");
		fLoadingProgressBar->SetMaxValue(100);
		fLoadingProgressBar->Hide();
		fLoadingProgressBar->SetBarHeight(12);

		const float kInsetSpacing = 3;
		const float kElementSpacing = 5;

		fFindTextControl = new BTextControl("find", "Find:", "",
			new BMessage(TEXT_FIND_NEXT));
		fFindCaseSensitiveCheckBox = new BCheckBox("Match case");
		BView* findGroup = BGroupLayoutBuilder(B_VERTICAL)
			.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, kElementSpacing)
				.Add(fFindTextControl)
				.Add(new BButton("Previous", new BMessage(TEXT_FIND_PREVIOUS)))
				.Add(new BButton("Next", new BMessage(TEXT_FIND_NEXT)))
				.Add(fFindCaseSensitiveCheckBox)
				.Add(BSpaceLayoutItem::CreateGlue())
				.Add(new BButton("Close", new BMessage(TEXT_HIDE_FIND_GROUP)))
				.SetInsets(kInsetSpacing, kInsetSpacing,
					kInsetSpacing, kInsetSpacing)
			)
		;
		// Layout
		AddChild(BGroupLayoutBuilder(B_VERTICAL)
#if !INTEGRATE_MENU_INTO_TAB_BAR
			.Add(mainMenu)
#endif
			.Add(fTabManager->TabGroup())
			.Add(BGridLayoutBuilder(kElementSpacing, kElementSpacing)
				.Add(fBackButton, 0, 0)
				.Add(fForwardButton, 1, 0)
				.Add(fStopButton, 2, 0)
				.Add(fURLTextControl, 3, 0)
				.Add(fGoButton, 4, 0)
				.SetInsets(kInsetSpacing, kInsetSpacing, kInsetSpacing, kInsetSpacing)
			)
			.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
			.Add(fTabManager->ContainerView())
			.Add(findGroup)
			.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, kElementSpacing)
				.Add(fStatusText)
				.Add(fLoadingProgressBar, 0.2)
				.AddStrut(12 - kElementSpacing)
				.SetInsets(kInsetSpacing, 0, kInsetSpacing, 0)
			)
		);

		fURLTextControl->MakeFocus(true);

		fURLAutoCompleter = new TextControlCompleter(fURLTextControl,
			new BrowsingHistoryChoiceModel());

		fFindGroup = layoutItemFor(findGroup);
		fTabGroup = layoutItemFor(fTabManager->TabGroup());
	} else {
		fBackButton = 0;
		fForwardButton = 0;
		fStopButton = 0;
		fGoButton = 0;
		fURLTextControl = 0;
		fURLAutoCompleter = 0;
		fStatusText = 0;
		fLoadingProgressBar = 0;

		BWebView* webView = new BWebView("web_view");
		SetCurrentWebView(webView);

		AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
			.Add(CurrentWebView())
		);
	}

	CreateNewTab("", true);

	fFindGroup->SetVisible(false);

	AddShortcut('G', B_COMMAND_KEY, new BMessage(TEXT_FIND_NEXT));
	AddShortcut('G', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_FIND_PREVIOUS));
	AddShortcut('F', B_COMMAND_KEY, new BMessage(TEXT_SHOW_FIND_GROUP));
	AddShortcut('F', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(TEXT_HIDE_FIND_GROUP));
	AddShortcut('R', B_COMMAND_KEY, new BMessage(RELOAD));

	// Add shortcuts to select a particular tab
	for (int32 i = 0; i <= 9; i++) {
		BMessage *selectTab = new BMessage(SELECT_TAB);
		if (i == 0)
			selectTab->AddInt32("tab index", 9);
		else
			selectTab->AddInt32("tab index", i - 1);
		char numStr[2];
		snprintf(numStr, sizeof(numStr), "%d", (int) i);
		AddShortcut(numStr[0], B_COMMAND_KEY, selectTab);
	}
	
	be_app->PostMessage(WINDOW_OPENED);
}


BrowserWindow::~BrowserWindow()
{
	delete fURLAutoCompleter;
	delete fTabManager;
}


void
BrowserWindow::DispatchMessage(BMessage* message, BHandler* target)
{
	const char* bytes;
	int32 modifiers;
	if ((message->what == B_KEY_DOWN || message->what == B_UNMAPPED_KEY_DOWN)
		&& message->FindString("bytes", &bytes) == B_OK
		&& message->FindInt32("modifiers", &modifiers) == B_OK) {
		if (fURLTextControl && target == fURLTextControl->TextView()) {
			// Handle B_RETURN in the URL text control. This is the easiest
			// way to react *only* when the user presses the return key in the
			// address bar, as opposed to trying to load whatever is in there when
			// the text control just goes out of focus.
			if (bytes[0] == B_RETURN) {
				// Do it in such a way that the user sees the Go-button go down.
				fGoButton->SetValue(B_CONTROL_ON);
				UpdateIfNeeded();
				fGoButton->Invoke();
				snooze(1000);
				fGoButton->SetValue(B_CONTROL_OFF);
			}
		} else if (bytes[0] == B_LEFT_ARROW && (modifiers & B_COMMAND_KEY) != 0) {
			PostMessage(GO_BACK);
		} else if (bytes[0] == B_RIGHT_ARROW && (modifiers & B_COMMAND_KEY) != 0) {
			PostMessage(GO_FORWARD);
		}
	}
	BWebWindow::DispatchMessage(message, target);
}


void
BrowserWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case OPEN_LOCATION:
		if (fURLTextControl) {
			if (fURLTextControl->TextView()->IsFocus())
				fURLTextControl->TextView()->SelectAll();
			else
				fURLTextControl->MakeFocus(true);
		}
		break;
	case RELOAD:
		CurrentWebView()->Reload();
		break;
	case GOTO_URL: {
		BString url;
		if (message->FindString("url", &url) != B_OK)
			url = fURLTextControl->Text();
		fTabManager->SetTabIcon(CurrentWebView(), NULL);
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
		BAlert* alert = new BAlert("Confirmation", "Do you really want to "
			"clear the browsing history?", "Clear", "Cancel");
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
		CurrentWebView()->FindString(fFindTextControl->Text(), true,
			fFindCaseSensitiveCheckBox->Value());
		break;
	case TEXT_FIND_PREVIOUS:
		CurrentWebView()->FindString(fFindTextControl->Text(), false,
			fFindCaseSensitiveCheckBox->Value());
		break;
	case TEXT_SHOW_FIND_GROUP:
		if (!fFindGroup->IsVisible())
			fFindGroup->SetVisible(true);
		fFindTextControl->MakeFocus(true);
		break;
	case TEXT_HIDE_FIND_GROUP:
		if (fFindGroup->IsVisible())
			fFindGroup->SetVisible(false);
		break;

	case SHOW_DOWNLOAD_WINDOW:
	case SHOW_SETTINGS_WINDOW:
		message->AddUInt32("workspaces", Workspaces());
		be_app->PostMessage(message);
		break;

	case CLOSE_TAB:
		if (fTabManager->CountTabs() > 1) {
			int32 index;
			if (message->FindInt32("tab index", &index) != B_OK)
				index = fTabManager->SelectedTabIndex();
			_ShutdownTab(index);
			_UpdateTabGroupVisibility();
		} else
			PostMessage(B_QUIT_REQUESTED);
		break;

	case SELECT_TAB: {
		int32 index;
		if (message->FindInt32("tab index", &index) == B_OK
			&& fTabManager->SelectedTabIndex() != index
			&& fTabManager->CountTabs() > index) {
			fTabManager->SelectTab(index);
		}
		
		break;
	}

	case TAB_CHANGED: {
		// This message may be received also when the last tab closed, i.e. with index == -1.
		int32 index;
		if (message->FindInt32("tab index", &index) != B_OK)
			index = -1;
		BWebView* webView = dynamic_cast<BWebView*>(fTabManager->ViewForTab(index));
		if (webView == CurrentWebView())
			break;
		SetCurrentWebView(webView);
		if (webView)
			_UpdateTitle(webView->MainFrameTitle());
		else
			_UpdateTitle("");
		if (webView) {
			fURLTextControl->SetText(webView->MainFrameURL());
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


bool
BrowserWindow::QuitRequested()
{
	// TODO: Check for modified form data and ask user for confirmation, etc.

	// Iterate over all tabs to delete all BWebViews.
	// Do this here, so WebKit tear down happens earlier.
	while (fTabManager->CountTabs() > 0)
		_ShutdownTab(0);
	SetCurrentWebView(0);

	BMessage message(WINDOW_CLOSED);
	message.AddRect("window frame", Frame());
	be_app->PostMessage(&message);
	return true;
}


void
BrowserWindow::MenusBeginning()
{
	BMenuItem* menuItem;
	while ((menuItem = fGoMenu->RemoveItem(0L)))
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
		fGoMenu->AddItem(menuItem);
	}


	if (fGoMenu->CountItems() > 3) {
		fGoMenu->AddSeparatorItem();
		fGoMenu->AddItem(new BMenuItem("Clear history",
			new BMessage(CLEAR_HISTORY)));
	}

	history->Unlock();
}


void
BrowserWindow::CreateNewTab(const BString& url, bool select, BWebView* webView)
{
	// Executed in app thread (new BWebPage needs to be created in app thread).
	if (!webView)
		webView = new BWebView("web view");
	webView->WebPage()->SetDownloadListener(fDownloadListener);

	fTabManager->AddTab(webView, "New tab");

	if (url.Length())
		webView->LoadURL(url.String());

	if (select) {
		fTabManager->SelectTab(fTabManager->CountTabs() - 1);
		SetCurrentWebView(webView);
		NavigationCapabilitiesChanged(false, false, false, webView);
		if (fURLTextControl) {
			fURLTextControl->SetText(url.String());
			fURLTextControl->MakeFocus(true);
		}
	}

	_UpdateTabGroupVisibility();
}


// #pragma mark - Notification API


void
BrowserWindow::NavigationRequested(const BString& url, BWebView* view)
{
}


void
BrowserWindow::NewWindowRequested(const BString& url, bool primaryAction)
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


void 
BrowserWindow::NewPageCreated(BWebView* view)
{
	CreateNewTab(BString(), true, view);
}


void
BrowserWindow::LoadNegotiating(const BString& url, BWebView* view)
{
	BString status("Requesting: ");
	status << url;
	StatusChanged(status, view);
}


void
BrowserWindow::LoadCommitted(const BString& url, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	// This hook is invoked when the load is commited.
	if (fURLTextControl)
		fURLTextControl->SetText(url.String());

	BString status("Loading: ");
	status << url;
	StatusChanged(status, view);
}


void
BrowserWindow::LoadProgress(float progress, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	if (fLoadingProgressBar) {
		if (progress < 100 && fLoadingProgressBar->IsHidden())
			fLoadingProgressBar->Show();
		fLoadingProgressBar->SetTo(progress);
	}
}


void
BrowserWindow::LoadFailed(const BString& url, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	BString status(url);
	status << " failed.";
	StatusChanged(status, view);
	if (fLoadingProgressBar && !fLoadingProgressBar->IsHidden())
		fLoadingProgressBar->Hide();
}


void
BrowserWindow::LoadFinished(const BString& url, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	BString status(url);
	status << " finished.";
	StatusChanged(status, view);
	if (fLoadingProgressBar && !fLoadingProgressBar->IsHidden())
		fLoadingProgressBar->Hide();

	NavigationCapabilitiesChanged(fBackButton->IsEnabled(),
		fForwardButton->IsEnabled(), false, view);
}


void
BrowserWindow::ResizeRequested(float width, float height, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	// TODO: Ignore request when there is more than one BWebView embedded!

	ResizeTo(width, height);
}


void
BrowserWindow::SetToolBarsVisible(bool flag, BWebView* view)
{
	// TODO
	// TODO: Ignore request when there is more than one BWebView embedded!
}


void
BrowserWindow::SetStatusBarVisible(bool flag, BWebView* view)
{
	// TODO
	// TODO: Ignore request when there is more than one BWebView embedded!
}


void
BrowserWindow::SetMenuBarVisible(bool flag, BWebView* view)
{
	// TODO
	// TODO: Ignore request when there is more than one BWebView embedded!
}


void
BrowserWindow::SetResizable(bool flag, BWebView* view)
{
	// TODO: Ignore request when there is more than one BWebView embedded!

	if (flag)
		SetFlags(Flags() & ~B_NOT_RESIZABLE);
	else
		SetFlags(Flags() | B_NOT_RESIZABLE);
}


void
BrowserWindow::TitleChanged(const BString& title, BWebView* view)
{
	for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
		if (fTabManager->ViewForTab(i) == view) {
			fTabManager->SetTabLabel(i, title);
			break;
		}
	}
	if (view != CurrentWebView())
		return;

	_UpdateTitle(title);
}


void
BrowserWindow::IconReceived(const BBitmap* icon, BWebView* view)
{
	fTabManager->SetTabIcon(view, icon);
}


void
BrowserWindow::StatusChanged(const BString& statusText, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	if (fStatusText)
		fStatusText->SetText(statusText.String());
}


void
BrowserWindow::NavigationCapabilitiesChanged(bool canGoBackward,
	bool canGoForward, bool canStop, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	if (fBackButton)
		fBackButton->SetEnabled(canGoBackward);
	if (fForwardButton)
		fForwardButton->SetEnabled(canGoForward);
	if (fStopButton)
		fStopButton->SetEnabled(canStop);
}


void
BrowserWindow::UpdateGlobalHistory(const BString& url)
{
	BrowsingHistory::defaultInstance()->addItem(BrowsingHistoryItem(url));
}


bool
BrowserWindow::AuthenticationChallenge(BString message, BString& inOutUser,
	BString& inOutPassword, bool& inOutRememberCredentials, uint32 failureCount,
	BWebView* view)
{
	// Switch to the page for which this authentication is required.
	if (view != CurrentWebView()) {
		fTabManager->SelectTab(view);
		UpdateIfNeeded();
	}
	AuthenticationPanel* panel = new AuthenticationPanel(Frame());
		// Panel auto-destructs.
	return panel->getAuthentication(message, inOutUser, inOutPassword,
		inOutRememberCredentials, failureCount > 0, inOutUser, inOutPassword,
		&inOutRememberCredentials);
}


// #pragma mark - private


void
BrowserWindow::_UpdateTitle(const BString& title)
{
	BString windowTitle = title;
	if (windowTitle.Length() > 0)
		windowTitle << " - ";
	windowTitle << kApplicationName;
	SetTitle(windowTitle.String());
}


void
BrowserWindow::_UpdateTabGroupVisibility()
{
	if (Lock()) {
		//fTabGroup->SetVisible(fTabManager->CountTabs() > 1);
		fTabManager->SetCloseButtonsAvailable(fTabManager->CountTabs() > 1);
		Unlock();
	}
}


void
BrowserWindow::_ShutdownTab(int32 index)
{
	BView* view = fTabManager->RemoveTab(index);
	BWebView* webView = dynamic_cast<BWebView*>(view);
	if (webView)
		webView->Shutdown();
	else
		delete view;
}
