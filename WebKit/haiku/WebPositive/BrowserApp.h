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
#ifndef BROWSER_APP_H
#define BROWSER_APP_H


#include <Application.h>
#include <Catalog.h>
#include <Rect.h>

class BNetworkCookieJar;
class DownloadWindow;
class BrowserWindow;
class SettingsMessage;
class SettingsWindow;


class BrowserApp : public BApplication {
public:
								BrowserApp();
	virtual						~BrowserApp();

	virtual	void				AboutRequested();
	virtual	void				ArgvReceived(int32 agrc, char** argv);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				ReadyToRun();
	virtual	bool				QuitRequested();

private:
			void				_RefsReceived(BMessage* message,
									int32* pagesCreated = NULL,
									bool* fullscreen = NULL);
			void				_CreateNewPage(const BString& url,
									bool fullscreen = false);
			void				_CreateNewWindow(const BString& url,
									bool fullscreen = false);
			void				_CreateNewTab(BrowserWindow* window,
									const BString& url, bool select);
			void				_ShowWindow(const BMessage* message,
									BWindow* window);

private:
			int					fWindowCount;
			BRect				fLastWindowFrame;
			BMessage*			fLaunchRefsMessage;
			bool				fInitialized;

			SettingsMessage*	fSettings;
			SettingsMessage*	fCookies;
			BNetworkCookieJar*	fCookieJar;

			DownloadWindow*		fDownloadWindow;
			SettingsWindow*		fSettingsWindow;

			BCatalog			fAppCatalog;
};


extern const char* kApplicationSignature;
extern const char* kApplicationName;


#endif // BROWSER_APP_H

