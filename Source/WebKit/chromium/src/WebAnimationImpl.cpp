/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WebAnimationImpl.h"

#include "CCActiveAnimation.h"
#include "CCAnimationCurve.h"
#include "WebFloatAnimationCurveImpl.h"
#include "WebTransformAnimationCurveImpl.h"
#include <public/WebAnimation.h>
#include <public/WebAnimationCurve.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

using WebCore::CCActiveAnimation;

namespace WebKit {

WebAnimation* WebAnimation::create(const WebAnimationCurve& curve, TargetProperty targetProperty, int animationId)
{
    return new WebAnimationImpl(curve, targetProperty, animationId, 0);
}

WebAnimationImpl::WebAnimationImpl(const WebAnimationCurve& webCurve, TargetProperty targetProperty, int animationId, int groupId)
{
    static int nextAnimationId = 1;
    static int nextGroupId = 1;
    if (!animationId)
        animationId = nextAnimationId++;
    if (!groupId)
        groupId = nextGroupId++;

    WebAnimationCurve::AnimationCurveType curveType = webCurve.type();
    OwnPtr<WebCore::CCAnimationCurve> curve;
    switch (curveType) {
    case WebAnimationCurve::AnimationCurveTypeFloat: {
        const WebFloatAnimationCurveImpl* floatCurveImpl = static_cast<const WebFloatAnimationCurveImpl*>(&webCurve);
        curve = floatCurveImpl->cloneToCCAnimationCurve();
        break;
    }
    case WebAnimationCurve::AnimationCurveTypeTransform: {
        const WebTransformAnimationCurveImpl* transformCurveImpl = static_cast<const WebTransformAnimationCurveImpl*>(&webCurve);
        curve = transformCurveImpl->cloneToCCAnimationCurve();
        break;
    }
    }
    m_animation = CCActiveAnimation::create(curve.release(), animationId, groupId, static_cast<WebCore::CCActiveAnimation::TargetProperty>(targetProperty));
}

WebAnimationImpl::~WebAnimationImpl()
{
}

int WebAnimationImpl::id()
{
    return m_animation->id();
}

WebAnimation::TargetProperty WebAnimationImpl::targetProperty() const
{
    return static_cast<WebAnimationImpl::TargetProperty>(m_animation->targetProperty());
}

int WebAnimationImpl::iterations() const
{
    return m_animation->iterations();
}

void WebAnimationImpl::setIterations(int n)
{
    m_animation->setIterations(n);
}

double WebAnimationImpl::startTime() const
{
    return m_animation->startTime();
}

void WebAnimationImpl::setStartTime(double monotonicTime)
{
    m_animation->setStartTime(monotonicTime);
}

double WebAnimationImpl::timeOffset() const
{
    return m_animation->timeOffset();
}

void WebAnimationImpl::setTimeOffset(double monotonicTime)
{
    m_animation->setTimeOffset(monotonicTime);
}

bool WebAnimationImpl::alternatesDirection() const
{
    return m_animation->alternatesDirection();
}

void WebAnimationImpl::setAlternatesDirection(bool alternates)
{
    m_animation->setAlternatesDirection(alternates);
}

PassOwnPtr<WebCore::CCActiveAnimation> WebAnimationImpl::cloneToCCAnimation()
{
    OwnPtr<WebCore::CCActiveAnimation> toReturn(m_animation->clone(WebCore::CCActiveAnimation::NonControllingInstance));
    toReturn->setNeedsSynchronizedStartTime(true);
    return toReturn.release();
}

} // namespace WebKit
