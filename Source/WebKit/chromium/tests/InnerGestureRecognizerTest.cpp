/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "GestureRecognizerChromium.h"

#include <gtest/gtest.h>

using namespace WebCore;

namespace {

class InspectableInnerGestureRecognizer : public InnerGestureRecognizer {
public:
    InspectableInnerGestureRecognizer()
        : WebCore::InnerGestureRecognizer()
    {
    }

    int signature(State gestureState, unsigned id, PlatformTouchPoint::State touchType, bool handled)
    {
        return InnerGestureRecognizer::signature(gestureState, id, touchType, handled);
    };

    IntPoint firstTouchPosition() { return m_firstTouchPosition; };

    void setFirstTouchTime(double t) { m_firstTouchTime = t; };
    double firstTouchTime() { return m_firstTouchTime; };

    void setLastTouchTime(double t) { m_lastTouchTime = t; };
    double lastTouchTime() { return m_lastTouchTime; };

    InnerGestureRecognizer::GestureTransitionFunction edgeFunction(int hash)
    {
        return m_edgeFunctions.get(hash);
    };

    virtual void updateValues(double d, const PlatformTouchPoint &p)
    {
        InnerGestureRecognizer::updateValues(d, p);
    };

    void addEdgeFunction(State state, unsigned finger, PlatformTouchPoint::State touchState, bool touchHandledByJavaScript, GestureTransitionFunction function)
    {
        InnerGestureRecognizer::addEdgeFunction(state, finger, touchState, touchHandledByJavaScript, function);
    };
};

class BuildablePlatformTouchPoint : public WebCore::PlatformTouchPoint {
public:
    BuildablePlatformTouchPoint();
    BuildablePlatformTouchPoint(int x, int y);

    void setX(int x)
    {
        m_pos.setX(x);
        m_screenPos.setX(x);
    };

    void setY(int y)
    {
        m_pos.setY(y);
        m_screenPos.setY(y);
    };
};

BuildablePlatformTouchPoint::BuildablePlatformTouchPoint()
{
    m_id = 0;
    m_state = PlatformTouchPoint::TouchStationary;
    m_pos = IntPoint::zero();
    m_screenPos = IntPoint::zero();
};

BuildablePlatformTouchPoint::BuildablePlatformTouchPoint(int x, int y)
{
    m_id = 0;
    m_state = PlatformTouchPoint::TouchStationary;
    m_pos = IntPoint(x, y);
    m_screenPos = IntPoint(x, y);
};


class TestGestureRecognizer : public testing::Test {
public:
    TestGestureRecognizer() { }

protected:
    virtual void SetUp() { }
    virtual void TearDown() { }
};

TEST_F(TestGestureRecognizer, hash)
{
    InspectableInnerGestureRecognizer testGm;
    const unsigned FirstFinger = 0;
    const unsigned SecondFinger = 1;

    ASSERT_EQ(1 + 0, testGm.signature(InnerGestureRecognizer::NoGesture, FirstFinger, PlatformTouchPoint::TouchReleased, false));

    ASSERT_EQ(1 + ((8 | 1) << 1), testGm.signature(InnerGestureRecognizer::NoGesture, FirstFinger, PlatformTouchPoint::TouchPressed, true));

    ASSERT_EQ(1 + ((0x10000 | 2) << 1), testGm.signature(InnerGestureRecognizer::PendingSyntheticClick, FirstFinger, PlatformTouchPoint::TouchMoved, false));

    ASSERT_EQ(1 + (0x20000 << 1), testGm.signature(InnerGestureRecognizer::Scroll, FirstFinger, PlatformTouchPoint::TouchReleased, false));

    ASSERT_EQ(1 + ((0x20000 | 1) << 1), testGm.signature(InnerGestureRecognizer::Scroll, FirstFinger, PlatformTouchPoint::TouchPressed, false));

    ASSERT_EQ(1 + ((0x20000 | 0x10 | 8 | 1) << 1), testGm.signature(InnerGestureRecognizer::Scroll, SecondFinger, PlatformTouchPoint::TouchPressed, true));
}

TEST_F(TestGestureRecognizer, state)
{
    InspectableInnerGestureRecognizer testGm;

    ASSERT_EQ(0, testGm.state());
    testGm.setState(InnerGestureRecognizer::PendingSyntheticClick);
    ASSERT_EQ(InnerGestureRecognizer::PendingSyntheticClick, testGm.state());
}

TEST_F(TestGestureRecognizer, isInsideManhattanSquare)
{
    InspectableInnerGestureRecognizer gm;
    BuildablePlatformTouchPoint p;

    ASSERT_EQ(0.0, gm.firstTouchPosition().x());
    ASSERT_EQ(0.0, gm.firstTouchPosition().y());

    p.setX(0.0);
    p.setY(19.999);
    ASSERT_TRUE(gm.isInsideManhattanSquare(p));

    p.setX(19.999);
    p.setY(0.0);
    ASSERT_TRUE(gm.isInsideManhattanSquare(p));

    p.setX(20.0);
    p.setY(0.0);
    ASSERT_FALSE(gm.isInsideManhattanSquare(p));

    p.setX(0.0);
    p.setY(20.0);
    ASSERT_FALSE(gm.isInsideManhattanSquare(p));

    p.setX(-20.0);
    p.setY(0.0);
    ASSERT_FALSE(gm.isInsideManhattanSquare(p));

    p.setX(0.0);
    p.setY(-20.0);
    ASSERT_FALSE(gm.isInsideManhattanSquare(p));
}

TEST_F(TestGestureRecognizer, isInClickTimeWindow)
{
    InspectableInnerGestureRecognizer gm;

    gm.setFirstTouchTime(0.0);
    gm.setLastTouchTime(0.0);
    ASSERT_FALSE(gm.isInClickTimeWindow());

    gm.setFirstTouchTime(0.0);
    gm.setLastTouchTime(0.010001);
    ASSERT_TRUE(gm.isInClickTimeWindow());

    gm.setFirstTouchTime(0.0);
    gm.setLastTouchTime(0.8 - .00000001);
    ASSERT_TRUE(gm.isInClickTimeWindow());

    gm.setFirstTouchTime(0.0);
    gm.setLastTouchTime(0.80001);
    ASSERT_FALSE(gm.isInClickTimeWindow());
}

TEST_F(TestGestureRecognizer, addEdgeFunction)
{
    InspectableInnerGestureRecognizer gm;
    gm.addEdgeFunction(InnerGestureRecognizer::Scroll, 0, PlatformTouchPoint::TouchReleased, true, (InnerGestureRecognizer::GestureTransitionFunction)4096);

    ASSERT_EQ((InnerGestureRecognizer::GestureTransitionFunction)4096, gm.edgeFunction(gm.signature(InnerGestureRecognizer::Scroll, 0, PlatformTouchPoint::TouchReleased, true)));
}

TEST_F(TestGestureRecognizer, updateValues)
{
    InspectableInnerGestureRecognizer gm;

    ASSERT_EQ(0.0, gm.firstTouchTime());
    ASSERT_EQ(0.0, gm.lastTouchTime());
    ASSERT_EQ(InnerGestureRecognizer::NoGesture, gm.state());

    BuildablePlatformTouchPoint p1(10, 11);
    gm.updateValues(1.1, p1);

    ASSERT_EQ(10, gm.firstTouchPosition().x());
    ASSERT_EQ(11, gm.firstTouchPosition().y());
    ASSERT_EQ(1.1, gm.firstTouchTime());
    ASSERT_EQ(0.0, gm.lastTouchTime() - gm.firstTouchTime());

    BuildablePlatformTouchPoint p2(13, 14);
    gm.setState(InnerGestureRecognizer::PendingSyntheticClick);
    gm.updateValues(2.0, p2);

    ASSERT_EQ(10, gm.firstTouchPosition().x());
    ASSERT_EQ(11, gm.firstTouchPosition().y());
    ASSERT_EQ(1.1, gm.firstTouchTime());
    ASSERT_EQ(2.0 - 1.1, gm.lastTouchTime() - gm.firstTouchTime());

    BuildablePlatformTouchPoint p3(23, 34);
    gm.setState(InnerGestureRecognizer::NoGesture);
    gm.updateValues(3.0, p3);

    ASSERT_EQ(23, gm.firstTouchPosition().x());
    ASSERT_EQ(34, gm.firstTouchPosition().y());
    ASSERT_EQ(3.0, gm.firstTouchTime());
    ASSERT_EQ(0.0, gm.lastTouchTime() - gm.firstTouchTime());
}

} // namespace
