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


#ifndef WebProcess_h
#define WebProcess_h

#include <Handler.h>
#include <Rect.h>
#include <String.h>

class WebFrame;
class WebView;

namespace WebCore {
class Page;
};

enum {
    FIND_STRING_RESULT = 'fsrs'
};

class WebProcess : public BHandler {
public:
    static void initializeOnce();

    WebProcess(WebView* webView);

    void init();
    void shutdown();

    void setDispatchTarget(BHandler* handler);

    void loadURL(const char* urlString);
    void goBack();
    void goForward();
    void draw(const BRect& updateRect);
    void frameResized(float width, float height);
    void focused(bool focused);
    void activated(bool activated);
    void mouseEvent(const BMessage* message, const BPoint& where,
        const BPoint& screenWhere);
    void mouseWheelChanged(const BMessage* message,
        const BPoint& where, const BPoint& screenWhere);
    void keyEvent(const BMessage* message);

    void changeTextSize(float increment);
    void findString(const char* string, bool forward, bool caseSensitive,
        bool wrapSelection, bool startInSelection);

    // The following methods are only supposed to be called by the
    // ChromeClientHaiku and FrameLoaderHaiku code! Not from within the window
    // thread!
    WebFrame* mainFrame() const;
    WebCore::Page* page() const;
    WebView* webView() const;
        // NOTE: Using the WebView requires locking it's looper!

    BRect contentsSize();
    BRect windowBounds();
    void setWindowBounds(const BRect& bounds);
    BRect viewBounds();
    void setViewBounds(const BRect& bounds);
    BString mainFrameTitle();
    BString mainFrameURL();

    void paint(const BRect& rect, bool contentChanged, bool immediate,
        bool repaintContentOnly);

private:
    virtual ~WebProcess();

    virtual void MessageReceived(BMessage* message);

    void skipToLastMessage(BMessage*& message);

    void handleLoadURL(const BMessage* message);
    void handleGoBack(const BMessage* message);
    void handleGoForward(const BMessage* message);
    void handleDraw(const BMessage* message);
    void handleFrameResized(const BMessage* message);
    void handleFocused(const BMessage* message);
    void handleActivated(const BMessage* message);
    void handleMouseEvent(const BMessage* message);
    void handleMouseWheelChanged(BMessage* message);
    void handleKeyEvent(BMessage* message);
    void handleChangeTextSize(BMessage* message);
    void handleFindString(BMessage* message);

private:
    WebView* m_webView;
    WebFrame* m_mainFrame;
    WebCore::Page* m_page;
};

#endif // WebProcess_h
