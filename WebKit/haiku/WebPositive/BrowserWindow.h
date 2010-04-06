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
#ifndef BROWSER_WINDOW_H
#define BROWSER_WINDOW_H


#include "WebWindow.h"
#include <Messenger.h>
#include <String.h>

class BButton;
class BCheckBox;
class BDirectory;
class BFile;
class BLayoutItem;
class BMenu;
class BMenuItem;
class BPath;
class BStatusBar;
class BStringView;
class BTextControl;
class BWebView;
class IconButton;
class SettingsMessage;
class TabManager;
class TextControlCompleter;

enum ToolbarPolicy {
	HaveToolbar,
	DoNotHaveToolbar
};

enum {
	NEW_WINDOW = 'nwnd',
	NEW_TAB = 'ntab',
	WINDOW_OPENED = 'wndo',
	WINDOW_CLOSED = 'wndc',
	SHOW_DOWNLOAD_WINDOW = 'sdwd',
	SHOW_SETTINGS_WINDOW = 'sswd'
};

#define INTEGRATE_MENU_INTO_TAB_BAR 0


class BrowserWindow : public BWebWindow {
public:
								BrowserWindow(BRect frame,
									SettingsMessage* appSettings,
									ToolbarPolicy = HaveToolbar,
									BWebView* webView = NULL);
	virtual						~BrowserWindow();

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* target);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual	void				MenusBeginning();

			void				CreateNewTab(const BString& url, bool select,
									BWebView* webView = 0);

private:
	// WebPage notification API implementations
	virtual	void				NavigationRequested(const BString& url,
									BWebView* view);
	virtual	void				NewWindowRequested(const BString& url,
									bool primaryAction);
	virtual	void				CloseWindowRequested(BWebView* view);
	virtual	void				NewPageCreated(BWebView* view,
									BRect windowFrame, bool modalDialog,
									bool resizable);
	virtual	void				LoadNegotiating(const BString& url,
									BWebView* view);
	virtual	void				LoadCommitted(const BString& url,
									BWebView* view);
	virtual	void				LoadProgress(float progress, BWebView* view);
	virtual	void				LoadFailed(const BString& url, BWebView* view);
	virtual	void				LoadFinished(const BString& url,
									BWebView* view);
	virtual	void				TitleChanged(const BString& title,
									BWebView* view);
	virtual	void				IconReceived(const BBitmap* icon,
									BWebView* view);
	virtual	void				ResizeRequested(float width, float height,
									BWebView* view);
	virtual	void				SetToolBarsVisible(bool flag, BWebView* view);
	virtual	void				SetStatusBarVisible(bool flag, BWebView* view);
	virtual	void				SetMenuBarVisible(bool flag, BWebView* view);
	virtual	void				SetResizable(bool flag, BWebView* view);
	virtual	void				StatusChanged(const BString& status,
									BWebView* view);
	virtual	void				NavigationCapabilitiesChanged(
									bool canGoBackward, bool canGoForward,
									bool canStop, BWebView* view);
	virtual	void				UpdateGlobalHistory(const BString& url);
	virtual	bool				AuthenticationChallenge(BString message,
									BString& inOutUser, BString& inOutPassword,
									bool& inOutRememberCredentials,
									uint32 failureCount, BWebView* view);

private:
			void				_UpdateTitle(const BString &title);
			void				_UpdateTabGroupVisibility();
			void				_ShutdownTab(int32 index);
			void				_TabChanged(int32 index);

			status_t			_BookmarkPath(BPath& path) const;
			void				_CreateBookmark();
			void				_ShowBookmarks();
			bool				_CheckBookmarkExists(BDirectory& directory,
									const BString& fileName,
									const BString& url) const;
			bool				_ReadURLAttr(BFile& bookmarkFile,
									BString& url) const;
			void				_AddBookmarkURLsRecursively(
									BDirectory& directory,
									BMessage* message,
									uint32& addedCount) const;

private:
			BMenu*				fGoMenu;
			BMenuItem*			fZoomTextOnlyMenuItem;
			IconButton*			fBackButton;
			IconButton*			fForwardButton;
			IconButton*			fStopButton;
			BButton*			fGoButton;
			BTextControl*		fURLTextControl;
			TextControlCompleter* fURLAutoCompleter;
			BStringView*		fStatusText;
			BStatusBar*			fLoadingProgressBar;

			BLayoutItem*		fMenuGroup;
			BLayoutItem*		fTabGroup;
			BLayoutItem*		fNavigationGroup;
			BLayoutItem*		fFindGroup;
			BLayoutItem*		fStatusGroup;

			BTextControl*		fFindTextControl;
			BCheckBox*			fFindCaseSensitiveCheckBox;
			TabManager*			fTabManager;

			SettingsMessage*	fAppSettings;
			bool				fZoomTextOnly;
			bool				fShowTabsIfSinglePageOpen;
};


#endif // BROWSER_WINDOW_H

