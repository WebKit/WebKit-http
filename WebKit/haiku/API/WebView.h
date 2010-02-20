/*
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


#ifndef WebView_h
#define WebView_h

#include <String.h>
#include <View.h>

class BWebPage;

namespace WebCore {
class String;
};

class WebView : public BView {
public:
    WebView(const char* name);
    ~WebView();

    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();

    virtual void Draw(BRect);
    virtual void FrameResized(float width, float height);
    virtual void GetPreferredSize(float* width, float* height);
    virtual void MessageReceived(BMessage*);

    virtual void MakeFocus(bool = true);
    virtual void WindowActivated(bool);

    virtual void MouseMoved(BPoint, uint32, const BMessage*);
    virtual void MouseDown(BPoint);
    virtual void MouseUp(BPoint);

    virtual void KeyDown(const char*, int32);
    virtual void KeyUp(const char*, int32);

    BWebPage* webPage() const { return m_webPage; }

    void setBounds(BRect);
    BRect contentsSize() const;

    BString mainFrameTitle() const;
    BString mainFrameURL() const;

    void loadRequest(const char* urlString);
    void goBack();
    void goForward();

    void setToolbarsVisible(bool);
    bool areToolbarsVisible() const { return m_toolbarsVisible; }

    void setStatusbarVisible(bool);
    bool isStatusbarVisible() const { return m_statusbarVisible; }

    void setMenubarVisible(bool);
    bool isMenubarVisible() const { return m_menubarVisible; }

    void setResizable(bool);
    void closeWindow();
    void setStatusText(const BString&);
    void linkHovered(const WebCore::String&, const WebCore::String&,
        const WebCore::String&);

    void increaseTextSize();
    void decreaseTextSize();
    void resetTextSize();
    void findString(const char* string, bool forward = true,
        bool caseSensitive = false, bool wrapSelection = true,
        bool startInSelection = false);

private:
    friend class BWebPage;
    BView* offscreenView() const { return m_offscreenView; }
    void setOffscreenViewClean(BRect cleanRect, bool immediate);
    void invalidate();

    void resizeOffscreenView(int width, int height);
    void dispatchMouseEvent(const BPoint& where, uint32 sanityWhat);
    void dispatchKeyEvent(uint32 sanityWhat);

private:
    uint32 m_lastMouseButtons;

    bool m_toolbarsVisible;
    bool m_statusbarVisible;
    bool m_menubarVisible;

    BBitmap* m_offscreenBitmap;
    BView* m_offscreenView;
    bool m_offscreenViewClean;

    BWebPage* m_webPage;
};

#endif // WebView_h
