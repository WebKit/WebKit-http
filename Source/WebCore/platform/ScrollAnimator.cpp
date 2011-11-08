/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollAnimator.h"

#include "FloatPoint.h"
#include "PlatformWheelEvent.h"
#include "ScrollableArea.h"
#include <algorithm>
#include <wtf/PassOwnPtr.h>

using namespace std;

namespace WebCore {

#if !ENABLE(SMOOTH_SCROLLING) && !(PLATFORM(CHROMIUM) && OS(DARWIN))
PassOwnPtr<ScrollAnimator> ScrollAnimator::create(ScrollableArea* scrollableArea)
{
    return adoptPtr(new ScrollAnimator(scrollableArea));
}
#endif

ScrollAnimator::ScrollAnimator(ScrollableArea* scrollableArea)
    : m_scrollableArea(scrollableArea)
    , m_currentPosX(0)
    , m_currentPosY(0)
    , m_currentZoomScale(1)
    , m_currentZoomTransX(0)
    , m_currentZoomTransY(0)
{
}

ScrollAnimator::~ScrollAnimator()
{
}

bool ScrollAnimator::scroll(ScrollbarOrientation orientation, ScrollGranularity, float step, float multiplier)
{
    float* currentPos = (orientation == HorizontalScrollbar) ? &m_currentPosX : &m_currentPosY;
    float newPos = std::max(std::min(*currentPos + (step * multiplier), static_cast<float>(m_scrollableArea->scrollSize(orientation))), 0.0f);
    if (*currentPos == newPos)
        return false;
    *currentPos = newPos;

    notifyPositionChanged();

    return true;
}

void ScrollAnimator::scrollToOffsetWithoutAnimation(const FloatPoint& offset)
{
    m_currentPosX = offset.x();
    m_currentPosY = offset.y();
    notifyPositionChanged();
}

bool ScrollAnimator::handleWheelEvent(const PlatformWheelEvent& e)
{
    Scrollbar* horizontalScrollbar = m_scrollableArea->horizontalScrollbar();
    Scrollbar* verticalScrollbar = m_scrollableArea->verticalScrollbar();

    // Accept the event if we have a scrollbar in that direction and can still
    // scroll any further.
    float deltaX = horizontalScrollbar ? e.deltaX() : 0;
    float deltaY = verticalScrollbar ? e.deltaY() : 0;

    bool handled = false;

    IntSize maxForwardScrollDelta = m_scrollableArea->maximumScrollPosition() - m_scrollableArea->scrollPosition();
    IntSize maxBackwardScrollDelta = m_scrollableArea->scrollPosition() - m_scrollableArea->minimumScrollPosition();
    if ((deltaX < 0 && maxForwardScrollDelta.width() > 0)
        || (deltaX > 0 && maxBackwardScrollDelta.width() > 0)
        || (deltaY < 0 && maxForwardScrollDelta.height() > 0)
        || (deltaY > 0 && maxBackwardScrollDelta.height() > 0)) {
        handled = true;
        if (deltaY) {
            if (e.granularity() == ScrollByPageWheelEvent) {
                bool negative = deltaY < 0;
                deltaY = max(max(static_cast<float>(m_scrollableArea->visibleHeight()) * Scrollbar::minFractionToStepWhenPaging(), static_cast<float>(m_scrollableArea->visibleHeight() - Scrollbar::maxOverlapBetweenPages())), 1.0f);
                if (negative)
                    deltaY = -deltaY;
            }
            scroll(VerticalScrollbar, ScrollByPixel, verticalScrollbar->pixelStep(), -deltaY);
        }

        if (deltaX) {
            if (e.granularity() == ScrollByPageWheelEvent) {
                bool negative = deltaX < 0;
                deltaX = max(max(static_cast<float>(m_scrollableArea->visibleWidth()) * Scrollbar::minFractionToStepWhenPaging(), static_cast<float>(m_scrollableArea->visibleWidth() - Scrollbar::maxOverlapBetweenPages())), 1.0f);
                if (negative)
                    deltaX = -deltaX;
            }
            scroll(HorizontalScrollbar, ScrollByPixel, horizontalScrollbar->pixelStep(), -deltaX);
        }
    }

    return handled;
}

#if ENABLE(GESTURE_EVENTS)
void ScrollAnimator::handleGestureEvent(const PlatformGestureEvent&)
{
}
#endif

FloatPoint ScrollAnimator::currentPosition() const
{
    return FloatPoint(m_currentPosX, m_currentPosY);
}

void ScrollAnimator::notifyPositionChanged()
{
    m_scrollableArea->setScrollOffsetFromAnimation(IntPoint(m_currentPosX, m_currentPosY));
}

void ScrollAnimator::notifyZoomChanged(ZoomAnimationState state)
{
    m_scrollableArea->zoomAnimatorTransformChanged(m_currentZoomScale, m_currentZoomTransX, m_currentZoomTransY,
                                                   state == ZoomAnimationContinuing ? ScrollableArea::ZoomAnimationContinuing
                                                                                    : ScrollableArea::ZoomAnimationFinishing);
}

FloatPoint ScrollAnimator::zoomTranslation() const
{
    return FloatPoint(m_currentZoomTransX, m_currentZoomTransY);
}

void ScrollAnimator::resetZoom()
{
    m_currentZoomScale = 1;
    m_currentZoomTransX = 0;
    m_currentZoomTransY = 0;
}

void ScrollAnimator::setZoomParametersForTest(float scale, float x, float y)
{
    m_currentZoomScale = scale;
    m_currentZoomTransX = (1 - scale) * x;
    m_currentZoomTransY = (1 - scale) * y;
    notifyZoomChanged(ZoomAnimationContinuing); // Don't let page re-scale.
}

} // namespace WebCore
