/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef ScrollbarThemeQStyle_h
#define ScrollbarThemeQStyle_h

#include "ScrollbarTheme.h"

#include <QtCore/qglobal.h>

namespace WebCore {

class QStyleFacade;

class ScrollbarThemeQStyle final : public ScrollbarTheme {
public:
    ScrollbarThemeQStyle();
    ~ScrollbarThemeQStyle() final;

    bool paint(Scrollbar&, GraphicsContext&, const IntRect& dirtyRect) final;
    void paintScrollCorner(ScrollView*, GraphicsContext&, const IntRect& cornerRect) final;

    ScrollbarPart hitTest(Scrollbar&, const IntPoint&) final;

    ScrollbarButtonPressAction handleMousePressEvent(Scrollbar&, const PlatformMouseEvent&, ScrollbarPart) override;

    void invalidatePart(Scrollbar&, ScrollbarPart) final;

    int thumbPosition(Scrollbar&) final;
    int thumbLength(Scrollbar&) final;
    int trackPosition(Scrollbar&) final;
    int trackLength(Scrollbar&) final;

    int scrollbarThickness(ScrollbarControlSize = RegularScrollbar) final;

    QStyleFacade* qStyle() { return m_qStyle.get(); }

private:
    std::unique_ptr<QStyleFacade> m_qStyle;
};

}
#endif
