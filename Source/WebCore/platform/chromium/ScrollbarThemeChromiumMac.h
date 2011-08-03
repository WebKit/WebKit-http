/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScrollbarThemeChromiumMac_h
#define ScrollbarThemeChromiumMac_h

#include "ScrollbarOverlayUtilitiesChromiumMac.h"
#include "ScrollbarThemeComposite.h"

// This file (and its associated .mm file) is a clone of ScrollbarThemeMac.h.
// See the .mm file for details.

namespace WebCore {

class ScrollbarThemeChromiumMac : public ScrollbarThemeComposite {
public:
    ScrollbarThemeChromiumMac();
    virtual ~ScrollbarThemeChromiumMac();

    virtual bool paint(Scrollbar*, GraphicsContext* context, const IntRect& damageRect);

    virtual int scrollbarThickness(ScrollbarControlSize = RegularScrollbar);

    virtual bool supportsControlTints() const { return true; }
    virtual bool usesOverlayScrollbars() const;

    virtual double initialAutoscrollTimerDelay();
    virtual double autoscrollTimerDelay();

    virtual ScrollbarButtonsPlacement buttonsPlacement() const;

    virtual void registerScrollbar(Scrollbar*);
    virtual void unregisterScrollbar(Scrollbar*);

    void setNewPainterForScrollbar(Scrollbar*, WKScrollbarPainterRef);
    WKScrollbarPainterRef painterForScrollbar(Scrollbar*);

protected:
    virtual bool hasButtons(Scrollbar*);
    virtual bool hasThumb(Scrollbar*);

    virtual IntRect backButtonRect(Scrollbar*, ScrollbarPart, bool painting = false);
    virtual IntRect forwardButtonRect(Scrollbar*, ScrollbarPart, bool painting = false);
    virtual IntRect trackRect(Scrollbar*, bool painting = false);

    virtual int minimumThumbLength(Scrollbar*);

    virtual bool shouldCenterOnThumb(Scrollbar*, const PlatformMouseEvent&);
    virtual bool shouldDragDocumentInsteadOfThumb(Scrollbar*, const PlatformMouseEvent&);

    virtual void paintTickmarks(GraphicsContext*, Scrollbar*, const IntRect&);

public:
    void preferencesChanged();
};

}

#endif // ScrollbarThemeChromiumMac_h
