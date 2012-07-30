/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebScrollbarThemePainter_h
#define WebScrollbarThemePainter_h

#include "WebCanvas.h"

namespace WebCore {
class ScrollbarThemeComposite;
};

namespace WebKit {

class WebScrollbar;
struct WebRect;

class WebScrollbarThemePainter {
public:
    WebScrollbarThemePainter() : m_theme(0) { }
    WebScrollbarThemePainter(const WebScrollbarThemePainter& geometry) { assign(geometry); }
    virtual ~WebScrollbarThemePainter() { }
    WebScrollbarThemePainter& operator=(const WebScrollbarThemePainter& painter)
    {
        assign(painter);
        return *this;
    }

    bool isNull() const { return m_theme; }
    WEBKIT_EXPORT void assign(const WebScrollbarThemePainter&);

    WEBKIT_EXPORT void paintScrollbarBackground(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintTrackBackground(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintBackTrackPart(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintForwardTrackPart(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintBackButtonStart(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintBackButtonEnd(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintForwardButtonStart(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintForwardButtonEnd(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintTickmarks(WebCanvas*, WebScrollbar*, const WebRect&);
    WEBKIT_EXPORT void paintThumb(WebCanvas*, WebScrollbar*, const WebRect&);

#if WEBKIT_IMPLEMENTATION
    WebScrollbarThemePainter(WebCore::ScrollbarThemeComposite*);
#endif

private:
    // The theme is not owned by this class. It is assumed that the theme is a
    // static pointer and its lifetime is essentially infinite. The functions
    // called from the painter may not be thread-safe, so all calls must be made
    // from the same thread that it is created on.
    WebCore::ScrollbarThemeComposite* m_theme;
};

} // namespace WebKit

#endif
