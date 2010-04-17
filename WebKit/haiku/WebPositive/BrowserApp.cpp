/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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
#include "BrowserApp.h"

#include "BrowserWindow.h"
#include "BrowsingHistory.h"
#include "DownloadWindow.h"
#include "SettingsMessage.h"
#include "SettingsWindow.h"
#include "svn_revision.h"
#include "NetworkCookieJar.h"
#include "WebPage.h"
#include "WebSettings.h"
#include "WebView.h"
#include <Alert.h>
#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Path.h>
#include <Screen.h>
#include <debugger.h>
#include <stdio.h>


const char* kApplicationSignature = "application/x-vnd.Haiku-WebPositive";
const char* kApplicationName = "WebPositive";
static const uint32 PRELOAD_BROWSING_HISTORY = 'plbh';

#define ENABLE_NATIVE_COOKIES 0


BrowserApp::BrowserApp()
	:
	BApplication(kApplicationSignature),
	fWindowCount(0),
	fLastWindowFrame(100, 100, 700, 750),
	fLaunchRefsMessage(0),
	fInitialized(false),
	fSettings(NULL),
	fCookies(NULL),
	fCookieJar(NULL),
	fDownloadWindow(NULL),
	fSettingsWindow(NULL)
{
	be_locale->GetAppCatalog(&fAppCatalog);
}


BrowserApp::~BrowserApp()
{
	delete fLaunchRefsMessage;
	delete fSettings;
	delete fCookies;
	delete fCookieJar;
}


void
BrowserApp::AboutRequested()
{
	BString aboutText("WebPositive\n\nby Ryan Leavengood, Andrea Anzani, "
		"Maxime Simon, Michael Lotz, Rene Gollent and Stephan Aßmus");
	aboutText << "\n\nSVN revision: " << kSVNRevision;

	BAlert* alert = new BAlert("About WebPositive", aboutText.String(),
		"Sweet!");
	alert->Go();
}


void
BrowserApp::ArgvReceived(int32 argc, char** argv)
{
	BMessage message(B_REFS_RECEIVED);
	for (int i = 1; i < argc; i++) {
		const char* url = argv[i];
		BEntry entry(argv[i], true);
		BPath path;
		if (entry.Exists() && entry.GetPath(&path) == B_OK)
			url = path.Path();
		message.AddString("url", url);
	}
	// Upon program launch, it will buffer a copy of the message, since
	// ArgReceived() is called before ReadyToRun().
	RefsReceived(&message);
}


void
BrowserApp::ReadyToRun()
{
	// Since we will essentially run the GUI...
	set_thread_priority(Thread(), B_DISPLAY_PRIORITY);

	BWebPage::InitializeOnce();
	BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(kApplicationName) == B_OK
		&& create_directory(path.Path(), 0777) == B_OK) {

		BWebSettings::SetPersistentStoragePath(path.Path());
	}

	BString mainSettingsPath(kApplicationName);
	mainSettingsPath << "/Application";
	fSettings = new SettingsMessage(B_USER_SETTINGS_DIRECTORY,
		mainSettingsPath.String());
#if ENABLE_NATIVE_COOKIES
	mainSettingsPath = kApplicationName;
	mainSettingsPath << "/Cookies";
	fCookies = new SettingsMessage(B_USER_SETTINGS_DIRECTORY,
		mainSettingsPath.String());
	BMessage cookieArchive;
	cookieArchive = fCookies->GetValue("cookies", cookieArchive);
	fCookieJar = new BNetworkCookieJar(cookieArchive);
	BWebPage::SetCookieJar(fCookieJar);
#endif

	fLastWindowFrame = fSettings->GetValue("window frame", fLastWindowFrame);
	BRect downloadWindowFrame = fSettings->GetValue("downloads window frame",
		BRect(100, 100, 300, 250));
	BRect settingsWindowFrame = fSettings->GetValue("settings window frame",
		BRect());
	bool showDownloads = fSettings->GetValue("show downloads", false);

	fDownloadWindow = new DownloadWindow(downloadWindowFrame, showDownloads,
		fSettings);
	fSettingsWindow = new SettingsWindow(settingsWindowFrame, fSettings);

	BWebPage::SetDownloadListener(BMessenger(fDownloadWindow));

	fInitialized = true;

	int32 pagesCreated = 0;
	if (fLaunchRefsMessage) {
		_RefsReceived(fLaunchRefsMessage, &pagesCreated);
		delete fLaunchRefsMessage;
		fLaunchRefsMessage = 0;
	}
	if (pagesCreated == 0) {
		BrowserWindow* window = new BrowserWindow(fLastWindowFrame, fSettings);
		window->Show();
	}
	PostMessage(PRELOAD_BROWSING_HISTORY);
}


void
BrowserApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case PRELOAD_BROWSING_HISTORY:
		// Accessing the default instance will load the history from disk.
		BrowsingHistory::DefaultInstance();
		break;
	case B_SILENT_RELAUNCH:
		_CreateNewPage("");
		break;
	case NEW_WINDOW: {
		BString url;
		if (message->FindString("url", &url) != B_OK)
			break;
		_CreateNewWindow(url);
		break;
	}
	case NEW_TAB: {
		BrowserWindow* window;
		if (message->FindPointer("window", reinterpret_cast<void**>(&window)) != B_OK)
			break;
		BString url;
		message->FindString("url", &url);
		bool select = false;
		message->FindBool("select", &select);
		_CreateNewTab(window, url, select);
		break;
	}
	case WINDOW_OPENED:
		fWindowCount++;
		break;
	case WINDOW_CLOSED:
		fWindowCount--;
		message->FindRect("window frame", &fLastWindowFrame);
		if (fWindowCount <= 0)
			PostMessage(B_QUIT_REQUESTED);
		break;

	case SHOW_DOWNLOAD_WINDOW:
		_ShowWindow(message, fDownloadWindow);
		break;
	case SHOW_SETTINGS_WINDOW:
		_ShowWindow(message, fSettingsWindow);
		break;

	default:
		BApplication::MessageReceived(message);
		break;
	}
}


void
BrowserApp::RefsReceived(BMessage* message)
{
	if (!fInitialized) {
		delete fLaunchRefsMessage;
		fLaunchRefsMessage = new BMessage(*message);
		return;
	}

	_RefsReceived(message);
}


bool
BrowserApp::QuitRequested()
{
	for (int i = 0; BWindow* window = WindowAt(i); i++) {
		BrowserWindow* webWindow = dynamic_cast<BrowserWindow*>(window);
		if (!webWindow)
			continue;
		if (!webWindow->Lock())
			continue;
		if (webWindow->QuitRequested()) {
			fLastWindowFrame = webWindow->Frame();
			webWindow->Quit();
			i--;
		} else {
			webWindow->Unlock();
			return false;
		}
	}

	BWebPage::ShutdownOnce();

	fSettings->SetValue("window frame", fLastWindowFrame);
	if (fDownloadWindow->Lock()) {
		fSettings->SetValue("downloads window frame", fDownloadWindow->Frame());
		fSettings->SetValue("show downloads", !fDownloadWindow->IsHidden());
		fDownloadWindow->Unlock();
	}
	if (fSettingsWindow->Lock()) {
		fSettings->SetValue("settings window frame", fSettingsWindow->Frame());
		fSettingsWindow->Unlock();
	}

	BMessage cookieArchive;
	if (fCookieJar != NULL && fCookieJar->Archive(&cookieArchive) == B_OK)
		fCookies->SetValue("cookies", cookieArchive);

	return true;
}


void
BrowserApp::_RefsReceived(BMessage* message, int32* _pagesCreated)
{
	int32 pagesCreated = 0;

	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BEntry entry(&ref, true);
		if (!entry.Exists())
			continue;
		BPath path;
		if (entry.GetPath(&path) != B_OK)
			continue;
		BString url;
		url << path.Path();
		_CreateNewPage(url);
		pagesCreated++;
	}

	BString url;
	for (int32 i = 0; message->FindString("url", i, &url) == B_OK; i++) {
		_CreateNewPage(url);
		pagesCreated++;
	}

	if (_pagesCreated != NULL)
		*_pagesCreated = pagesCreated;
}


void
BrowserApp::_CreateNewPage(const BString& url)
{
	uint32 workspace = 1 << current_workspace();

	bool loadedInWindowOnCurrentWorkspace = false;
	for (int i = 0; BWindow* window = WindowAt(i); i++) {
		BrowserWindow* webWindow = dynamic_cast<BrowserWindow*>(window);
		if (!webWindow)
			continue;
		if (webWindow->Lock()) {
			if (webWindow->Workspaces() & workspace) {
				webWindow->CreateNewTab(url, true);
				webWindow->Activate();
				loadedInWindowOnCurrentWorkspace = true;
			}
			webWindow->Unlock();
		}
		if (loadedInWindowOnCurrentWorkspace)
			return;
	}
	_CreateNewWindow(url);
}


void
BrowserApp::_CreateNewWindow(const BString& url)
{
	// Offset the window frame unless this is the first window created in the
	// session.
	if (fWindowCount > 0)
		fLastWindowFrame.OffsetBy(20, 20);
	if (!BScreen().Frame().Contains(fLastWindowFrame))
		fLastWindowFrame.OffsetTo(50, 50);

	BrowserWindow* window = new BrowserWindow(fLastWindowFrame, fSettings);
	window->Show();
	if (url.Length())
		window->CurrentWebView()->LoadURL(url.String());
}


void
BrowserApp::_CreateNewTab(BrowserWindow* window, const BString& url,
	bool select)
{
	if (!window->Lock())
		return;
	window->CreateNewTab(url, select);
	window->Unlock();
}


void
BrowserApp::_ShowWindow(const BMessage* message, BWindow* window)
{
	BAutolock _(window);
	uint32 workspaces;
	if (message->FindUInt32("workspaces", &workspaces) == B_OK)
		window->SetWorkspaces(workspaces);
	if (window->IsHidden())
		window->Show();
	else
		window->Activate();
}


// #pragma mark -


int
main(int, char**)
{
	try {
		new BrowserApp();
		be_app->Run();
		delete be_app;
	} catch (...) {
		debugger("Exception caught.");
	}

	return 0;
}

