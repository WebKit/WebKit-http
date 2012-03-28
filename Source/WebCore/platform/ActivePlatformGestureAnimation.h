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

#ifndef ActivePlatformGestureAnimation_h
#define ActivePlatformGestureAnimation_h

#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class PlatformGestureCurve;
class PlatformGestureCurveTarget;

// Implements a gesture animation (fling scroll, etc.) using a curve with a generic interface
// to define the animation parameters as a function of time, and applies the animation
// to a target, again via a generic interface. It is assumed that animate() is called
// on a more-or-less regular basis by the owner.
class ActivePlatformGestureAnimation {
    WTF_MAKE_NONCOPYABLE(ActivePlatformGestureAnimation);
public:
    static PassOwnPtr<ActivePlatformGestureAnimation> create(PassOwnPtr<PlatformGestureCurve>, PlatformGestureCurveTarget*);
    static PassOwnPtr<ActivePlatformGestureAnimation> create(PassOwnPtr<PlatformGestureCurve>, PlatformGestureCurveTarget*, double startTime);
    ~ActivePlatformGestureAnimation();

    bool animate(double time);

private:
    // Assumes a valid PlatformGestureCurveTarget that outlives the animation.
    ActivePlatformGestureAnimation(PassOwnPtr<PlatformGestureCurve>, PlatformGestureCurveTarget*);
    ActivePlatformGestureAnimation(PassOwnPtr<PlatformGestureCurve>, PlatformGestureCurveTarget*, double startTime);

    double m_startTime;
    bool m_waitingForFirstTick;
    OwnPtr<PlatformGestureCurve> m_curve;
    PlatformGestureCurveTarget* m_target;
};

} // namespace WebCore

#endif
