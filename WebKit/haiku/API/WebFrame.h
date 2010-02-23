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

#ifndef WebFrame_h
#define WebFrame_h

#include <String.h>

class BMessenger;
class BWebPage;

namespace WebCore {
class ChromeClientHaiku;
class Frame;
class FrameLoaderClientHaiku;
class KURL;
}

class WebFramePrivate;

class WebFrame {
public:
    virtual ~WebFrame();

    void setListener(const BMessenger& listener);

    void loadRequest(BString url);
    // TODO: Remove this as well (no WebCore API in public WebKit API):
    void loadRequest(WebCore::KURL);

    BString url() const;

    void stopLoading();
    void reload();

    bool canCopy();
    bool canCut();
    bool canPaste();

    void copy();
    void cut();
    void paste();

    bool canUndo();
    bool canRedo();

    void undo();
    void redo();

    bool allowsScrolling();
    void setAllowsScrolling(bool);

    BString getPageSource() const;
    void setPageSource(const BString& source);

    void setTransparent(bool);
    bool isTransparent() const;

    BString getInnerText();
    BString getAsMarkup();
    BString getExternalRepresentation();

    bool findString(const char* string, bool forward = true,
        bool caseSensitive = false, bool wrapSelection = true,
        bool startInSelection = true);

    bool canIncreaseTextSize() const;
    bool canDecreaseTextSize() const;

    void increaseTextSize();
    void decreaseTextSize();

    void resetTextSize();

    void setEditable(bool editable) { m_isEditable = editable; }
    bool isEditable() const { return m_isEditable; }

    BString title() const { return m_title; }
    void setTitle(BString title) { m_title = title; }

private:
    friend class BWebPage;
    friend class WebCore::ChromeClientHaiku;
    friend class WebCore::FrameLoaderClientHaiku;

    WebFrame(BWebPage*, WebFrame*, WebFramePrivate*);

    WebCore::Frame* frame() const;

private:
    float m_textMagnifier;
    bool m_isEditable;
    bool m_beingDestroyed;
    BString m_title;

    WebFramePrivate* m_data;
};

#endif // WebFrame_h
