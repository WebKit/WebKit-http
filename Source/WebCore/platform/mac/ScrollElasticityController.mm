/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "ScrollElasticityController.h"

#if ENABLE(RUBBER_BANDING)

namespace WebCore {

static const float rubberbandStiffness = 20;

static float reboundDeltaForElasticDelta(float delta)
{
    return delta * rubberbandStiffness;
}

ScrollElasticityController::ScrollElasticityController(ScrollElasticityControllerClient* client)
    : m_client(client)
    , m_inScrollGesture(false)
    , m_momentumScrollInProgress(false)
    , m_ignoreMomentumScrolls(false)
    , m_scrollerInitiallyPinnedOnLeft(false)
    , m_scrollerInitiallyPinnedOnRight(false)
    , m_cumulativeHorizontalScroll(0)
    , m_didCumulativeHorizontalScrollEverSwitchToOppositeDirectionOfPin(false)
    , m_lastMomentumScrollTimestamp(0)
    , m_startTime(0)
    , m_snapRubberbandTimerIsActive(false)
{
}

void ScrollElasticityController::beginScrollGesture()
{
    m_inScrollGesture = true;
    m_momentumScrollInProgress = false;
    m_ignoreMomentumScrolls = false;
    m_lastMomentumScrollTimestamp = 0;
    m_momentumVelocity = FloatSize();
    m_scrollerInitiallyPinnedOnLeft = m_client->isHorizontalScrollerPinnedToMinimumPosition();
    m_scrollerInitiallyPinnedOnRight = m_client->isHorizontalScrollerPinnedToMaximumPosition();
    m_cumulativeHorizontalScroll = 0;
    m_didCumulativeHorizontalScrollEverSwitchToOppositeDirectionOfPin = false;
    
    IntSize stretchAmount = m_client->stretchAmount();
    m_stretchScrollForce.setWidth(reboundDeltaForElasticDelta(stretchAmount.width()));
    m_stretchScrollForce.setHeight(reboundDeltaForElasticDelta(stretchAmount.height()));
    
    m_overflowScrollDelta = FloatSize();

    stopSnapRubberbandTimer();
}

void ScrollElasticityController::stopSnapRubberbandTimer()
{
    m_client->stopSnapRubberbandTimer();
    m_snapRubberbandTimerIsActive = false;
}

} // namespace WebCore

#endif // ENABLE(RUBBER_BANDING)
