/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "ScrollingCoordinator.h"

namespace WebCore {

class ScrollingCoordinatorPrivate {
};

#if !ENABLE(THREADED_SCROLLING)
PassRefPtr<ScrollingCoordinator> ScrollingCoordinator::create(Page* page)
{
    return adoptRef(new ScrollingCoordinator(page));
}

ScrollingCoordinator::~ScrollingCoordinator()
{
    ASSERT(!m_page);
}

void ScrollingCoordinator::frameViewHorizontalScrollbarLayerDidChange(FrameView*, GraphicsLayer*)
{
}

void ScrollingCoordinator::frameViewVerticalScrollbarLayerDidChange(FrameView*, GraphicsLayer*)
{
}

void ScrollingCoordinator::setScrollLayer(GraphicsLayer*)
{
}

void ScrollingCoordinator::setNonFastScrollableRegion(const Region&)
{
}

void ScrollingCoordinator::setScrollParameters(const ScrollParameters&)
{
}

void ScrollingCoordinator::setWheelEventHandlerCount(unsigned)
{
}

void ScrollingCoordinator::setShouldUpdateScrollLayerPositionOnMainThread(MainThreadScrollingReasons)
{
}

bool ScrollingCoordinator::supportsFixedPositionLayers() const
{
    return false;
}

void ScrollingCoordinator::setLayerIsContainerForFixedPositionLayers(GraphicsLayer*, bool)
{
}

void ScrollingCoordinator::setLayerIsFixedToContainerLayer(GraphicsLayer*, bool)
{
}

void ScrollingCoordinator::scrollableAreaScrollLayerDidChange(ScrollableArea*, GraphicsLayer*)
{
}
#endif // !ENABLE(THREADED_SCROLLING)

}
