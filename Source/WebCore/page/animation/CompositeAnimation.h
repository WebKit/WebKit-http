/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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

#ifndef CompositeAnimation_h
#define CompositeAnimation_h

#include "ImplicitAnimation.h"
#include "KeyframeAnimation.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class AnimationControllerPrivate;
class AnimationController;
class RenderElement;
class RenderStyle;

// A CompositeAnimation represents a collection of animations that are running
// on a single RenderElement, such as a number of properties transitioning at once.
class CompositeAnimation : public RefCounted<CompositeAnimation> {
public:
    static PassRefPtr<CompositeAnimation> create(AnimationControllerPrivate* animationController)
    {
        return adoptRef(new CompositeAnimation(animationController));
    };

    ~CompositeAnimation();
    
    void clearRenderer();

    PassRef<RenderStyle> animate(RenderElement&, RenderStyle* currentStyle, RenderStyle& targetStyle);
    PassRefPtr<RenderStyle> getAnimatedStyle() const;

    double timeToNextService() const;
    
    AnimationControllerPrivate* animationController() const { return m_animationController; }

    void suspendAnimations();
    void resumeAnimations();
    bool isSuspended() const { return m_suspended; }
    
    bool hasAnimations() const  { return !m_transitions.isEmpty() || !m_keyframeAnimations.isEmpty(); }

    bool isAnimatingProperty(CSSPropertyID, bool acceleratedOnly, bool isRunningNow) const;

    PassRefPtr<KeyframeAnimation> getAnimationForProperty(CSSPropertyID) const;

    void overrideImplicitAnimations(CSSPropertyID);
    void resumeOverriddenImplicitAnimations(CSSPropertyID);

    bool pauseAnimationAtTime(const AtomicString& name, double t);
    bool pauseTransitionAtTime(CSSPropertyID, double);
    unsigned numberOfActiveAnimations() const;

private:
    CompositeAnimation(AnimationControllerPrivate*);

    void updateTransitions(RenderElement*, RenderStyle* currentStyle, RenderStyle* targetStyle);
    void updateKeyframeAnimations(RenderElement*, RenderStyle* currentStyle, RenderStyle* targetStyle);
    
    typedef HashMap<int, RefPtr<ImplicitAnimation>> CSSPropertyTransitionsMap;
    typedef HashMap<AtomicStringImpl*, RefPtr<KeyframeAnimation>>  AnimationNameMap;

    AnimationControllerPrivate* m_animationController;
    CSSPropertyTransitionsMap m_transitions;
    AnimationNameMap m_keyframeAnimations;
    Vector<AtomicStringImpl*> m_keyframeAnimationOrderMap;
    bool m_suspended;
};

} // namespace WebCore

#endif // CompositeAnimation_h
