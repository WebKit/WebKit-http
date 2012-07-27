/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"
#include "ScrollbarThemeChromiumAndroid.h"

#include "LayoutTestSupport.h"
#include "PlatformContextSkia.h"
#include "PlatformMouseEvent.h"
#include "PlatformSupport.h"
#include "Scrollbar.h"
#include "TransformationMatrix.h"

#include <algorithm>

using namespace std;

namespace WebCore {

static const int scrollbarWidth = 8;
static const int scrollbarMargin = 5;

ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
    DEFINE_STATIC_LOCAL(ScrollbarThemeChromiumAndroid, theme, ());
    return &theme;
}

int ScrollbarThemeChromiumAndroid::scrollbarThickness(ScrollbarControlSize controlSize)
{
    if (isRunningLayoutTest()) {
        // Match Chromium-Linux for DumpRenderTree, so the layout test results
        // can be shared. The width of scrollbar down arrow should equal the
        // width of the vertical scrollbar.
        IntSize scrollbarSize = PlatformSupport::getThemePartSize(PlatformSupport::PartScrollbarDownArrow);
        return scrollbarSize.width();
    }

    return scrollbarWidth + scrollbarMargin;
}

bool ScrollbarThemeChromiumAndroid::usesOverlayScrollbars() const
{
    // In layout test mode, match Chromium-Linux.
    return !isRunningLayoutTest();
}

int ScrollbarThemeChromiumAndroid::thumbPosition(ScrollbarThemeClient* scrollbar)
{
    if (!scrollbar->totalSize())
        return 0;

    int trackLen = trackLength(scrollbar);
    float proportion = static_cast<float>(scrollbar->currentPos()) / scrollbar->totalSize();
    return round(proportion * trackLen);
}

int ScrollbarThemeChromiumAndroid::thumbLength(ScrollbarThemeClient* scrollbar)
{
    int trackLen = trackLength(scrollbar);

    if (!scrollbar->totalSize())
        return trackLen;

    float proportion = (float)scrollbar->visibleSize() / scrollbar->totalSize();
    int length = round(proportion * trackLen);
    length = min(max(length, minimumThumbLength(scrollbar)), trackLen);
    return length;
}

bool ScrollbarThemeChromiumAndroid::hasThumb(ScrollbarThemeClient* scrollbar)
{
    // In layout test mode, match Chromium-Linux.
    return !isRunningLayoutTest();
}

IntRect ScrollbarThemeChromiumAndroid::backButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool)
{
    return IntRect();
}

IntRect ScrollbarThemeChromiumAndroid::forwardButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool)
{
    return IntRect();
}

IntRect ScrollbarThemeChromiumAndroid::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    IntRect rect = scrollbar->frameRect();
    if (scrollbar->orientation() == HorizontalScrollbar)
        rect.inflateX(-scrollbarMargin);
    else
        rect.inflateY(-scrollbarMargin);
    return rect;
}

static void fillSmoothEdgedRect(GraphicsContext* context, const IntRect& rect, const Color& color)
{
    Color halfColor(color.red(), color.green(), color.blue(), color.alpha() / 2);

    IntRect topRect = rect;
    topRect.inflateX(-1);
    topRect.setHeight(1);
    context->fillRect(topRect, halfColor, ColorSpaceDeviceRGB);

    IntRect leftRect = rect;
    leftRect.inflateY(-1);
    leftRect.setWidth(1);
    context->fillRect(leftRect, halfColor, ColorSpaceDeviceRGB);

    IntRect centerRect = rect;
    centerRect.inflate(-1);
    context->fillRect(centerRect, color, ColorSpaceDeviceRGB);

    IntRect rightRect = rect;
    rightRect.inflateY(-1);
    rightRect.setX(centerRect.maxX());
    rightRect.setWidth(1);
    context->fillRect(rightRect, halfColor, ColorSpaceDeviceRGB);

    IntRect bottomRect = rect;
    bottomRect.inflateX(-1);
    bottomRect.setY(centerRect.maxY());
    bottomRect.setHeight(1);
    context->fillRect(bottomRect, halfColor, ColorSpaceDeviceRGB);
}

void ScrollbarThemeChromiumAndroid::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    IntRect thumbRect = rect;
    if (scrollbar->orientation() == HorizontalScrollbar)
        thumbRect.setHeight(thumbRect.height() - scrollbarMargin);
    else
        thumbRect.setWidth(thumbRect.width() - scrollbarMargin);
    fillSmoothEdgedRect(context, thumbRect, Color(128, 128, 128, 128));
}

void ScrollbarThemeChromiumAndroid::paintScrollbarBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar)
{
    // Paint black background in DumpRenderTree, otherwise the pixels in the scrollbar area depend
    // on their previous state, which makes the dumped result undetermined.
    if (isRunningLayoutTest())
        context->fillRect(scrollbar->frameRect(), Color::black, ColorSpaceDeviceRGB);
}

} // namespace WebCore
