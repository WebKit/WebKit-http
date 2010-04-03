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
#include "NavMenu.h"
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
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
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

	CREATE_BOOKMARK = 'crbm',
	SHOW_BOOKMARKS = 'shbm',

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


static BString
baseURL(const BString string)
{
	int32 baseURLStart = string.FindFirst("://") + 3;
	int32 baseURLEnd = string.FindFirst("/", baseURLStart + 1);
	BString result;
	result.SetTo(string.String() + baseURLStart, baseURLEnd - baseURLStart);
	return result;
}


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
		BrowsingHistory* history = BrowsingHistory::DefaultInstance();
		if (!history->Lock())
			return;

		BString lastBaseURL;
		int32 priority = INT_MAX;

		count = history->CountItems();
		for (int32 i = 0; i < count; i++) {
			BrowsingHistoryItem item = history->HistoryItemAt(i);
			const BString& choiceText = item.URL();
			int32 matchPos = choiceText.IFindFirst(pattern);
			if (matchPos < 0)
				continue;
			if (lastBaseURL.Length() > 0
				&& choiceText.FindFirst(lastBaseURL) >= 0) {
				priority--;
			} else
				priority = INT_MAX;
			lastBaseURL = baseURL(choiceText);
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


class BookmarkMenu : public BNavMenu {
public:
	BookmarkMenu(const char* title, BHandler* target, const entry_ref* navDir)
		:
		BNavMenu(title, B_REFS_RECEIVED, target)
	{
		SetNavDir(navDir);
	}

	virtual void AttachedToWindow()
	{
		RemoveItems(0, CountItems(), true);
		ForceRebuild();
		BNavMenu::AttachedToWindow();
		if (CountItems() > 0)
			AddItem(new BSeparatorItem(), 0);
		AddItem(new BMenuItem("Manage bookmarks",
			new BMessage(SHOW_BOOKMARKS)), 0);
		AddItem(new BMenuItem("Bookmark this page",
			new BMessage(CREATE_BOOKMARK), 'B'), 0);
		DoLayout();
	}
};


// #pragma mark - BrowserWindow


BrowserWindow::BrowserWindow(BRect frame, ToolbarPolicy toolbarPolicy)
	:
	BWebWindow(frame, kApplicationName,
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS)
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

		BPath bookmarkPath;
		entry_ref bookmarkRef;
		if (_BookmarkPath(bookmarkPath) == B_OK
			&& get_ref_for_path(bookmarkPath.Path(), &bookmarkRef) == B_OK) {
			BMenu* bookmarkMenu
				= new BookmarkMenu("Bookmarks", this, &bookmarkRef);
			mainMenu->AddItem(bookmarkMenu);
		}

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

		AddChild(BGroupLayoutBuilder(B_VERTICAL)
			.Add(fTabManager->ContainerView())
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
		BrowsingHistory* history = BrowsingHistory::DefaultInstance();
		if (history->CountItems() == 0)
			break;
		BAlert* alert = new BAlert("Confirmation", "Do you really want to "
			"clear the browsing history?", "Clear", "Cancel");
		if (alert->Go() == 0)
			history->Clear();
		break;
	}

	case CREATE_BOOKMARK:
		_CreateBookmark();
		break;
	case SHOW_BOOKMARKS:
		_ShowBookmarks();
		break;

	case B_REFS_RECEIVED: {
		// Currently the only source of these messages is the bookmarks menu.
		// Filter refs into URLs, this also gets rid of refs for folders.
		// For clicks on sub-folders in the bookmarks menu, we have Tracker
		// open the corresponding folder.
		entry_ref ref;
		uint32 addedCount = 0;
		for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
printf("B_REFS_RECEIVED: %s\n", ref.name);
			BEntry entry(&ref);
			uint32 addedSubCount = 0;
			if (entry.IsDirectory()) {
				BDirectory directory(&entry);
printf("  directory ok\n");
				_AddBookmarkURLsRecursively(directory, message, addedSubCount);
			} else {
				BFile file(&ref, B_READ_ONLY);
printf("  file ok\n");
				BString url;
				if (_ReadURLAttr(file, url)) {
					message->AddString("url", url.String());
					addedSubCount++;
				}
			}
			if (addedSubCount == 0) {
				// Don't know what to do with this entry, just pass it
				// on to the system to handle. Note that this may result
				// in us opening other supported files via the application
				// mechanism.
				be_roster->Launch(&ref);
			}
			addedCount += addedSubCount;
		}
		message->RemoveName("refs");
		if (addedCount > 10) {
			BString string;
			string << "Do you want to open " << addedCount;
			string << " bookmarks all at once?";
			BAlert* alert = new BAlert("Open bookmarks confirmation",
				string.String(), "Cancel", "Open all");
			if (alert->Go() == 0)
				break;
		}
		be_app->PostMessage(message);
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
		_TabChanged(index);
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


static void
addItemToMenuOrSubmenu(BMenu* menu, BMenuItem* newItem)
{
	BString baseURLLabel = baseURL(BString(newItem->Label()));
	for (int32 i = menu->CountItems() - 1; i >= 0; i--) {
		BMenuItem* item = menu->ItemAt(i);
		BString label = item->Label();
		if (label.FindFirst(baseURLLabel) >= 0) {
			if (item->Submenu()) {
				// Submenu was already added in previous iteration.
				item->Submenu()->AddItem(newItem);
				return;
			} else {
				menu->RemoveItem(item);
				BMenu* subMenu = new BMenu(baseURLLabel.String());
				subMenu->AddItem(item);
				subMenu->AddItem(newItem);
				// Add common submenu for this base URL, clickable.
				BMessage* message = new BMessage(GOTO_URL);
				message->AddString("url", baseURLLabel.String());
				menu->AddItem(new BMenuItem(subMenu, message), i);
				return;
			}
		}
	}
	menu->AddItem(newItem);
}


static void
addOrDeleteMenu(BMenu* menu, BMenu* toMenu)
{
	if (menu->CountItems() > 0)
		toMenu->AddItem(menu);
	else
		delete menu;
}


void
BrowserWindow::MenusBeginning()
{
	BMenuItem* menuItem;
	while ((menuItem = fGoMenu->RemoveItem(0L)))
		delete menuItem;

	BrowsingHistory* history = BrowsingHistory::DefaultInstance();
	if (!history->Lock())
		return;

	int32 count = history->CountItems();
	BMenuItem* clearHistoryItem = new BMenuItem("Clear history",
		new BMessage(CLEAR_HISTORY));
	clearHistoryItem->SetEnabled(count > 0);
	fGoMenu->AddItem(clearHistoryItem);
	if (count == 0) {
		history->Unlock();
		return;
	}
	fGoMenu->AddSeparatorItem();

	BDateTime todayStart = BDateTime::CurrentDateTime(B_LOCAL_TIME);
	todayStart.SetTime(BTime(0, 0, 0));

	BDateTime oneDayAgoStart = todayStart;
	oneDayAgoStart.Date().AddDays(-1);

	BDateTime twoDaysAgoStart = oneDayAgoStart;
	twoDaysAgoStart.Date().AddDays(-1);

	BDateTime threeDaysAgoStart = twoDaysAgoStart;
	threeDaysAgoStart.Date().AddDays(-1);

	BDateTime fourDaysAgoStart = threeDaysAgoStart;
	fourDaysAgoStart.Date().AddDays(-1);

	BDateTime fiveDaysAgoStart = fourDaysAgoStart;
	fiveDaysAgoStart.Date().AddDays(-1);

	BMenu* todayMenu = new BMenu("Today");
	BMenu* yesterdayMenu = new BMenu("Yesterday");
	BMenu* twoDaysAgoMenu = new BMenu(
		twoDaysAgoStart.Date().LongDayName().String());
	BMenu* threeDaysAgoMenu = new BMenu(
		threeDaysAgoStart.Date().LongDayName().String());
	BMenu* fourDaysAgoMenu = new BMenu(
		fourDaysAgoStart.Date().LongDayName().String());
	BMenu* fiveDaysAgoMenu = new BMenu(
		fiveDaysAgoStart.Date().LongDayName().String());
	BMenu* earlierMenu = new BMenu("Earlier");

	for (int32 i = 0; i < count; i++) {
		BrowsingHistoryItem historyItem = history->HistoryItemAt(i);
		BMessage* message = new BMessage(GOTO_URL);
		message->AddString("url", historyItem.URL().String());

		BString truncatedUrl(historyItem.URL());
		be_plain_font->TruncateString(&truncatedUrl, B_TRUNCATE_END, 480);
		menuItem = new BMenuItem(truncatedUrl, message);

		if (historyItem.DateTime() < fiveDaysAgoStart)
			addItemToMenuOrSubmenu(earlierMenu, menuItem);
		else if (historyItem.DateTime() < fourDaysAgoStart)
			addItemToMenuOrSubmenu(fiveDaysAgoMenu, menuItem);
		else if (historyItem.DateTime() < threeDaysAgoStart)
			addItemToMenuOrSubmenu(fourDaysAgoMenu, menuItem);
		else if (historyItem.DateTime() < twoDaysAgoStart)
			addItemToMenuOrSubmenu(threeDaysAgoMenu, menuItem);
		else if (historyItem.DateTime() < oneDayAgoStart)
			addItemToMenuOrSubmenu(twoDaysAgoMenu, menuItem);
		else if (historyItem.DateTime() < todayStart)
			addItemToMenuOrSubmenu(yesterdayMenu, menuItem);
		else
			addItemToMenuOrSubmenu(todayMenu, menuItem);
	}
	history->Unlock();

	addOrDeleteMenu(todayMenu, fGoMenu);
	addOrDeleteMenu(yesterdayMenu, fGoMenu);
	addOrDeleteMenu(twoDaysAgoMenu, fGoMenu);
	addOrDeleteMenu(fourDaysAgoMenu, fGoMenu);
	addOrDeleteMenu(fiveDaysAgoMenu, fGoMenu);
	addOrDeleteMenu(earlierMenu, fGoMenu);
}


void
BrowserWindow::CreateNewTab(const BString& url, bool select, BWebView* webView)
{
	// Executed in app thread (new BWebPage needs to be created in app thread).
	if (!webView)
		webView = new BWebView("web view");

	fTabManager->AddTab(webView, "New tab");

	if (url.Length())
		webView->LoadURL(url.String());

	if (select) {
		fTabManager->SelectTab(fTabManager->CountTabs() - 1);
		SetCurrentWebView(webView);
		webView->WebPage()->ResendNotifications();
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
BrowserWindow::CloseWindowRequested(BWebView* view)
{
	int32 index = fTabManager->TabForView(view);
	if (index < 0) {
		// Tab is already gone.
		return;
	}
	BMessage message(CLOSE_TAB);
	message.AddInt32("tab index", index);
	PostMessage(&message, this);
}


void
BrowserWindow::LoadNegotiating(const BString& url, BWebView* view)
{
	BString status("Requesting: ");
	status << url;
	view->WebPage()->SetStatusMessage(status);
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
	view->WebPage()->SetStatusMessage(status);
}


void
BrowserWindow::LoadProgress(float progress, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	if (fLoadingProgressBar) {
		if (progress < 100 && fLoadingProgressBar->IsHidden())
			fLoadingProgressBar->Show();
		else if (progress == 100 && !fLoadingProgressBar->IsHidden())
			fLoadingProgressBar->Hide();
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
	view->WebPage()->SetStatusMessage(status);
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
	view->WebPage()->SetStatusMessage(status);
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

	// Ignore request when there is more than one BWebView embedded.
	if (fTabManager->CountTabs() > 1)
		return;

	// Make sure the new frame is not larger than the screen frame minus
	// window decorator border.
	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	BRect decoratorFrame = DecoratorFrame();
	BRect frame = Frame();

	screenFrame.left += decoratorFrame.left - frame.left;
	screenFrame.right += decoratorFrame.right - frame.right;
	screenFrame.top += decoratorFrame.top - frame.top;
	screenFrame.bottom += decoratorFrame.bottom - frame.bottom;

	width = min_c(width, screen.Frame().Width());
	height = min_c(height, screen.Frame().Height());

	frame.right = frame.left + width;
	frame.bottom = frame.top + height;

	// frame is now not larger than screenFrame, but may still be partly outside
	if (!screenFrame.Contains(frame)) {
		if (frame.left < screenFrame.left)
			frame.OffsetBy(screenFrame.left - frame.left, 0);
		else if (frame.right > screenFrame.right)
			frame.OffsetBy(screenFrame.right - frame.right, 0);
		if (frame.top < screenFrame.top)
			frame.OffsetBy(screenFrame.top - frame.top, 0);
		else if (frame.bottom > screenFrame.bottom)
			frame.OffsetBy(screenFrame.bottom - frame.bottom, 0);
	}

	MoveTo(frame.left, frame.top);
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
	BrowsingHistory::DefaultInstance()->AddItem(BrowsingHistoryItem(url));
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
//		fTabGroup->SetVisible(fTabManager->CountTabs() > 1);
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


void
BrowserWindow::_TabChanged(int32 index)
{
	BWebView* webView = dynamic_cast<BWebView*>(fTabManager->ViewForTab(index));
	if (webView == CurrentWebView())
		return;
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
}


status_t
BrowserWindow::_BookmarkPath(BPath& path) const
{
	status_t ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK)
		return ret;

	ret = path.Append(kApplicationName);
	if (ret != B_OK)
		return ret;

	ret = path.Append("Bookmarks");
	if (ret != B_OK)
		return ret;

	return create_directory(path.Path(), 0777);
}


void
BrowserWindow::_CreateBookmark()
{
	BPath path;
	status_t status = _BookmarkPath(path);
	if (status != B_OK) {
		BString message("There was an error retrieving the bookmark "
			"folder.\n\n");
		message << "Error: " << strerror(status);
		BAlert* alert = new BAlert("Bookmark error", message.String(), "OK",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return;
	}
	BWebView* webView = CurrentWebView();
	BString url(webView->MainFrameURL());
	// Create a bookmark file
	BFile bookmarkFile;
	BString bookmarkName(webView->MainFrameTitle());
	if (bookmarkName.Length() == 0) {
		bookmarkName = url;
		int32 leafPos = bookmarkName.FindLast('/');
		if (leafPos >= 0)
			bookmarkName.Remove(0, leafPos + 1);
	}

	// Check that the bookmark exists nowhere in the bookmark hierarchy,
	// though the intended file name must match, we don't search the stored
	// URLs, only for matching file names.
	BDirectory directory(path.Path());
	if (status == B_OK && _CheckBookmarkExists(directory, bookmarkName, url)) {
		BString message("A bookmark for this page (");
		message << bookmarkName;
		message << ") already exists.";
		BAlert* alert = new BAlert("Bookmark info", message.String(), "OK");
		alert->Go();
		return;
	}

	BPath entryPath(path);
	status = entryPath.Append(bookmarkName);
	BEntry entry;
	if (status == B_OK)
		status = entry.SetTo(entryPath.Path(), true);
	if (status == B_OK) {
		int32 tries = 1;
		while (entry.Exists()) {
			// Find a unique name for the bookmark, there may still be a
			// file in the way that stores a different URL.
			bookmarkName = webView->MainFrameTitle();
			bookmarkName << " " << tries++;
			entryPath = path;
			status = entryPath.Append(bookmarkName);
			if (status == B_OK)
				status = entry.SetTo(entryPath.Path(), true);
			if (status != B_OK)
				break;
		}
	}
	if (status == B_OK) {
		status = bookmarkFile.SetTo(&entry,
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	}

	// Write bookmark meta data
	if (status == B_OK)
		status = bookmarkFile.WriteAttrString("META:url", &url);
	if (status == B_OK) {
		BString title = webView->MainFrameTitle();
		bookmarkFile.WriteAttrString("META:title", &title);
	}

	BNodeInfo nodeInfo(&bookmarkFile);
	if (status == B_OK)
		status = nodeInfo.SetType("application/x-vnd.Be-bookmark");

	if (status != B_OK) {
		BString message("There was an error creating the bookmark "
			"file.\n\n");
		message << "Error: " << strerror(status);
		BAlert* alert = new BAlert("Bookmark error", message.String(), "OK",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return;
	}
}


void
BrowserWindow::_ShowBookmarks()
{
	BPath path;
	entry_ref ref;
	status_t status = _BookmarkPath(path);
	if (status == B_OK)
		status = get_ref_for_path(path.Path(), &ref);
	if (status == B_OK)
		status = be_roster->Launch(&ref);

	if (status != B_OK && status != B_ALREADY_RUNNING) {
		BString message("There was an error trying to show the Bookmarks "
			"folder.\n\n");
		message << "Error: " << strerror(status);
		BAlert* alert = new BAlert("Bookmark error", message.String(), "OK",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return;
	}
}


bool BrowserWindow::_CheckBookmarkExists(BDirectory& directory,
	const BString& bookmarkName, const BString& url) const
{
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory()) {
			BDirectory subBirectory(&entry);
			// At least preserve the entry file handle when recursing into
			// sub-folders... eventually we will run out, though, with very
			// deep hierarchy.
			entry.Unset();
			if (_CheckBookmarkExists(subBirectory, bookmarkName, url))
				return true;
		} else {
			char entryName[B_FILE_NAME_LENGTH];
			if (entry.GetName(entryName) != B_OK || bookmarkName != entryName)
				continue;
			BString storedURL;
			BFile file(&entry, B_READ_ONLY);
			if (_ReadURLAttr(file, storedURL)) {
				// Just bail if the bookmark already exists
				if (storedURL == url)
					return true;
			}
		}
	}
	return false;
}


bool
BrowserWindow::_ReadURLAttr(BFile& bookmarkFile, BString& url) const
{
	return bookmarkFile.InitCheck() == B_OK
		&& bookmarkFile.ReadAttrString("META:url", &url) == B_OK;
}


void
BrowserWindow::_AddBookmarkURLsRecursively(BDirectory& directory,
	BMessage* message, uint32& addedCount) const
{
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory()) {
			BDirectory subBirectory(&entry);
			// At least preserve the entry file handle when recursing into
			// sub-folders... eventually we will run out, though, with very
			// deep hierarchy.
			entry.Unset();
			_AddBookmarkURLsRecursively(subBirectory, message, addedCount);
		} else {
			BString storedURL;
			BFile file(&entry, B_READ_ONLY);
			if (_ReadURLAttr(file, storedURL)) {
				message->AddString("url", storedURL.String());
				addedCount++;
			}
		}
	}
}


