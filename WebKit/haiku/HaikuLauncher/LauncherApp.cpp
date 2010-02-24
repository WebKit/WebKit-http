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
#include "LauncherApp.h"

#include "DownloadWindow.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "LauncherWindow.h"
#include "WebPage.h"
#include "WebView.h"
#include "WebViewConstants.h"
#include <Alert.h>
#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <stdio.h>

extern const char* kApplicationSignature = "application/x-vnd.RJL-HaikuLauncher";
enum {
    LOAD_AT_STARTING = 'lost'
};


LauncherApp::LauncherApp()
    : BApplication(kApplicationSignature)
    , m_windowCount(0)
    , m_lastWindowFrame(100, 100, 700, 750)
    , m_launchRefsMessage(0)
    , m_initialized(false)
    , m_downloadWindow(0)
{
}

LauncherApp::~LauncherApp()
{
	delete m_launchRefsMessage;
}

void LauncherApp::AboutRequested()
{
    BAlert* alert = new BAlert("About HaikuLauncher",
        "For testing WebKit...\n\nby Ryan Leavengood", "Sweet!");
    alert->Go();
}

void LauncherApp::ArgvReceived(int32 argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		const char* url = argv[i];
		BEntry entry(argv[i], true);
		BPath path;
		if (entry.Exists() && entry.GetPath(&path) == B_OK)
		    url = path.Path();
	    BMessage message(LOAD_AT_STARTING);
	    message.AddString("url", url);
	    message.AddBool("new window", m_initialized || i > 1);
	    PostMessage(&message);
	}
}

void LauncherApp::ReadyToRun()
{
	// Since we will essentially run the GUI...
	set_thread_priority(Thread(), B_DISPLAY_PRIORITY);

    BWebPage::InitializeOnce();
    BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);

	BFile settingsFile;
	BRect windowFrameFromSettings = m_lastWindowFrame;
	BRect downloadWindowFrame(100, 100, 300, 250);
	bool showDownloads = false;
	if (openSettingsFile(settingsFile, B_READ_ONLY)) {
		BMessage settingsArchive;
		settingsArchive.Unflatten(&settingsFile);
		settingsArchive.FindRect("window frame", &windowFrameFromSettings);
		settingsArchive.FindRect("downloads window frame", &downloadWindowFrame);
		settingsArchive.FindBool("show downloads", &showDownloads);
	}
	m_lastWindowFrame = windowFrameFromSettings;

    m_downloadWindow = new DownloadWindow(downloadWindowFrame, showDownloads);

	m_initialized = true;

	if (m_launchRefsMessage) {
		RefsReceived(m_launchRefsMessage);
		delete m_launchRefsMessage;
		m_launchRefsMessage = 0;
	} else {
	    LauncherWindow* window = new LauncherWindow(m_lastWindowFrame,
	        BMessenger(m_downloadWindow));
	    window->Show();
	}
}

void LauncherApp::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case LOAD_AT_STARTING: {
        BString url;
        if (message->FindString("url", &url) != B_OK)
        	break;
        bool openNewWindow = false;
        message->FindBool("new window", &openNewWindow);
        LauncherWindow* webWindow = NULL;
        for (int i = 0; BWindow* window = WindowAt(i); i++) {
            webWindow = dynamic_cast<LauncherWindow*>(window);
            if (!webWindow)
            	continue;
            if (!openNewWindow) {
            	// stop at the first window
	            break;
            }
        }
        if (webWindow) {
        	// There should always be at least one window open. If not, maybe we are about
        	// to quit anyway...
        	if (openNewWindow) {
        		// open a new window with an offset to the last window
                newWindow(url);
        	} else {
            	// load the URL in the first window
                webWindow->CurrentWebView()->LoadURL(url.String());
        	}
        }
        break;
    }
    case B_SILENT_RELAUNCH:
    	newWindow("");
    	break;
    case NEW_WINDOW: {
		BString url;
		if (message->FindString("url", &url) != B_OK)
			break;
    	newWindow(url);
    	break;
    }
    case NEW_TAB: {
    	LauncherWindow* window;
		if (message->FindPointer("window", reinterpret_cast<void**>(&window)) != B_OK)
			break;
    	BString url;
		message->FindString("url", &url);
    	bool select = false;
		message->FindBool("select", &select);
    	newTab(window, url, select);
        break;
    }
    case WINDOW_OPENED:
    	m_windowCount++;
    	break;
    case WINDOW_CLOSED:
    	m_windowCount--;
        message->FindRect("window frame", &m_lastWindowFrame);
    	if (m_windowCount <= 0)
    		PostMessage(B_QUIT_REQUESTED);
    	break;

    case SHOW_DOWNLOAD_WINDOW: {
    	BAutolock _(m_downloadWindow);
        uint32 workspaces;
        if (message->FindUInt32("workspaces", &workspaces) == B_OK)
            m_downloadWindow->SetWorkspaces(workspaces);
        if (m_downloadWindow->IsHidden())
            m_downloadWindow->Show();
        else
            m_downloadWindow->Activate();
    }

    default:
        BApplication::MessageReceived(message);
        break;
    }
}

void LauncherApp::RefsReceived(BMessage* message)
{
	if (!m_initialized) {
		delete m_launchRefsMessage;
		m_launchRefsMessage = new BMessage(*message);
		return;
	}

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
		newWindow(url);
	}
}

bool LauncherApp::QuitRequested()
{
    for (int i = 0; BWindow* window = WindowAt(i); i++) {
        LauncherWindow* webWindow = dynamic_cast<LauncherWindow*>(window);
        if (!webWindow)
        	continue;
        if (!webWindow->Lock())
        	continue;
        if (webWindow->QuitRequested()) {
        	m_lastWindowFrame = webWindow->Frame();
        	webWindow->Quit();
        	i--;
        } else {
        	webWindow->Unlock();
        	return false;
        }
    }

	BFile settingsFile;
	if (openSettingsFile(settingsFile, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY)) {
		BMessage settingsArchive;
		settingsArchive.AddRect("window frame", m_lastWindowFrame);
		if (m_downloadWindow->Lock()) {
    	    settingsArchive.AddRect("downloads window frame", m_downloadWindow->Frame());
	        settingsArchive.AddBool("show downloads", !m_downloadWindow->IsHidden());
	        m_downloadWindow->Unlock();
		}
		settingsArchive.Flatten(&settingsFile);
	}

    return true;
}

bool LauncherApp::openSettingsFile(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("HaikuLauncher") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}

void LauncherApp::newWindow(const BString& url)
{
	m_lastWindowFrame.OffsetBy(20, 20);
	if (!BScreen().Frame().Contains(m_lastWindowFrame))
		m_lastWindowFrame.OffsetTo(50, 50);

	LauncherWindow* window = new LauncherWindow(m_lastWindowFrame,
	    BMessenger(m_downloadWindow));
	window->Show();
	if (url.Length())
	    window->CurrentWebView()->LoadURL(url.String());
}

void LauncherApp::newTab(LauncherWindow* window, const BString& url, bool select)
{
    if (!window->Lock())
        return;
    window->newTab(url, select);
    window->Unlock();
}

// #pragma mark -

int main(int, char**)
{
    new LauncherApp();
    be_app->Run();
    delete be_app;

    return 0;
}

