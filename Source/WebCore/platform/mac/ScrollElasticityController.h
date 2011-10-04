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

#ifndef ScrollElasticityController_h
#define ScrollElasticityController_h

#if ENABLE(RUBBER_BANDING)

#include <WebCore/FloatPoint.h>
#include <WebCore/FloatSize.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class ScrollElasticityControllerClient {
protected:
    virtual ~ScrollElasticityControllerClient() { } 

public:
    virtual bool isHorizontalScrollerPinnedToMinimumPosition() = 0;
    virtual bool isHorizontalScrollerPinnedToMaximumPosition() = 0;

    virtual IntSize stretchAmount() = 0;

    virtual void startSnapRubberbandTimer() = 0;
    virtual void stopSnapRubberbandTimer() = 0;
};

class ScrollElasticityController {
    WTF_MAKE_NONCOPYABLE(ScrollElasticityController);

public:
    explicit ScrollElasticityController(ScrollElasticityControllerClient*);

    void beginScrollGesture();

private:
    ScrollElasticityControllerClient* m_client;

    void stopSnapRubberbandTimer();

    // FIXME: These member variables should be private. They are currently public as a stop-gap measure, while
    // the rubber-band related code from ScrollAnimatorMac is being moved over.
public:
    bool m_inScrollGesture;
    bool m_momentumScrollInProgress;
    bool m_ignoreMomentumScrolls;
    bool m_scrollerInitiallyPinnedOnLeft;
    bool m_scrollerInitiallyPinnedOnRight;
    
    int m_cumulativeHorizontalScroll;
    bool m_didCumulativeHorizontalScrollEverSwitchToOppositeDirectionOfPin;

    CFTimeInterval m_lastMomentumScrollTimestamp;
    FloatSize m_overflowScrollDelta;
    FloatSize m_stretchScrollForce;
    FloatSize m_momentumVelocity;

    // Rubber band state.
    CFTimeInterval m_startTime;
    FloatSize m_startStretch;
    FloatPoint m_origOrigin;
    FloatSize m_origVelocity;

    bool m_snapRubberbandTimerIsActive;
};

} // namespace WebCore

#endif // ENABLE(RUBBER_BANDING)

#endif // ScrollElasticityController_h
