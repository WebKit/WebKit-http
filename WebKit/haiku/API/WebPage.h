/*
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


#ifndef _WEB_PAGE_H
#define _WEB_PAGE_H

#include <Handler.h>
#include <Messenger.h>
#include <Rect.h>
#include <String.h>

class BWebDownload;
class BWebFrame;
class BWebView;

namespace WebCore {
class ChromeClientHaiku;
class ContextMenuClientHaiku;
class DragClientHaiku;
class EditorClientHaiku;
class FrameLoaderClientHaiku;
class InspectorClientHaiku;

class Page;
class ResourceHandle;
class ResourceRequest;
class ResourceResponse;
};

namespace BPrivate {
class WebDownloadPrivate;
};

enum {
	B_FIND_STRING_RESULT	= 'fsrs',
	B_DOWNLOAD_ADDED		= 'dwna',
	B_DOWNLOAD_REMOVED		= 'dwnr'
};

typedef enum {
	B_WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER = 0,
	B_WEBKIT_CACHE_MODEL_WEB_BROWSER
} BWebKitCacheModel;

class BWebPage : private BHandler {
public:
	static	void				InitializeOnce();
	static	void				SetCacheModel(BWebKitCacheModel model);

			void				Init();
			void				Shutdown();

			void				SetListener(const BMessenger& listener);
			void				SetDownloadListener(const BMessenger& listener);

			BWebFrame*			MainFrame() const;
			BWebView*			WebView() const;
				// NOTE: Using the BWebView requires locking it's looper!

			void				LoadURL(const char* urlString);
			void				Reload();
			void				GoBack();
			void				GoForward();
			void				StopLoading();

			BString				MainFrameTitle() const;
			BString				MainFrameRequestedURL() const;
			BString				MainFrameURL() const;

			void				ChangeTextSize(float increment);
			void				FindString(const char* string,
									bool forward = true,
									bool caseSensitive = false,
									bool wrapSelection = true,
									bool startInSelection = false);

			void				ResendNotifications();

private:
	friend class BWebFrame;
	friend class BWebView;
	friend class BPrivate::WebDownloadPrivate;

								BWebPage(BWebView* webView);

	// These calls are private, since they are called from the BWebView only.
	void setVisible(bool visible);
	void draw(const BRect& updateRect);
	void frameResized(float width, float height);
	void focused(bool focused);
	void activated(bool activated);
	void mouseEvent(const BMessage* message, const BPoint& where,
        const BPoint& screenWhere);
	void mouseWheelChanged(const BMessage* message, const BPoint& where,
        const BPoint& screenWhere);
	void keyEvent(const BMessage* message);
	void standardShortcut(const BMessage* message);

private:
	// The following methods are only supposed to be called by the
	// ChromeClientHaiku and FrameLoaderHaiku code! Not from within the window
	// thread! This needs to go into a private class.
	friend class WebCore::ChromeClientHaiku;
	friend class WebCore::ContextMenuClientHaiku;
	friend class WebCore::DragClientHaiku;
	friend class WebCore::EditorClientHaiku;
	friend class WebCore::FrameLoaderClientHaiku;
	friend class WebCore::InspectorClientHaiku;

	WebCore::Page* page() const;

	BRect windowBounds();
	void setWindowBounds(const BRect& bounds);
	BRect viewBounds();
	void setViewBounds(const BRect& bounds);

	void setToolbarsVisible(bool);
	bool areToolbarsVisible() const { return m_toolbarsVisible; }

	void setStatusbarVisible(bool);
	bool isStatusbarVisible() const { return m_statusbarVisible; }

	void setMenubarVisible(bool);
	bool isMenubarVisible() const { return m_menubarVisible; }

	void setResizable(bool);
	void closeWindow();
	void setStatusText(const BString&);
	void linkHovered(const BString&, const BString&, const BString&);

	friend class BWebDownload;

	void requestDownload(const WebCore::ResourceRequest& request);
	void requestDownload(WebCore::ResourceHandle* handle,
		const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response);
	void downloadCreated(BWebDownload* download);

	void paint(BRect rect, bool contentChanged, bool immediate,
		bool repaintContentOnly);

private:
	virtual						~BWebPage();
	virtual	void				MessageReceived(BMessage* message);

	void skipToLastMessage(BMessage*& message);

	void handleLoadURL(const BMessage* message);
	void handleReload(const BMessage* message);
	void handleGoBack(const BMessage* message);
	void handleGoForward(const BMessage* message);
	void handleStop(const BMessage* message);
	void handleSetVisible(const BMessage* message);
	void handleFrameResized(const BMessage* message);
	void handleFocused(const BMessage* message);
	void handleActivated(const BMessage* message);
	void handleMouseEvent(const BMessage* message);
	void handleMouseWheelChanged(BMessage* message);
	void handleKeyEvent(BMessage* message);
	void handleChangeTextSize(BMessage* message);
	void handleFindString(BMessage* message);
	void handleResendNotifications(BMessage* message);

    status_t dispatchMessage(BMessage& message) const;

private:
    BMessenger m_listener;
	BMessenger m_downloadListener;
	BWebView* m_webView;
	BWebFrame* m_mainFrame;
	WebCore::Page* m_page;

    bool m_pageVisible;
    bool m_pageDirty;
    bool m_inPaint;

	bool m_toolbarsVisible;
	bool m_statusbarVisible;
	bool m_menubarVisible;
};

#endif // _WEB_PAGE_H
