/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "cc/CCLayerSorter.h"

#include <gtest/gtest.h>

using namespace WebCore;

namespace {

TEST(CCLayerSorterTest, PointInTriangle)
{
    FloatPoint a(10.0, 10.0);
    FloatPoint b(30.0, 10.0);
    FloatPoint c(20.0, 20.0);

    // Point in the center is in the triangle.
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 15.0), a, b, c));

    // Permuting the corners doesn't change the result.
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 15.0), a, c, b));
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 15.0), b, a, c));
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 15.0), b, c, a));
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 15.0), c, a, b));
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 15.0), c, b, a));

    // Points on the edges are not in the triangle.
    EXPECT_FALSE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 10.0), a, b, c));
    EXPECT_FALSE(CCLayerSorter::pointInTriangle(FloatPoint(15.0, 15.0), a, b, c));
    EXPECT_FALSE(CCLayerSorter::pointInTriangle(FloatPoint(25.0, 15.0), a, b, c));

    // Points just inside the edges are in the triangle.
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(20.0, 10.01), a, b, c));
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(15.01, 15.0), a, b, c));
    EXPECT_TRUE(CCLayerSorter::pointInTriangle(FloatPoint(24.99, 15.0), a, b, c));

    // Zero-area triangle doesn't intersect any point.
    EXPECT_FALSE(CCLayerSorter::pointInTriangle(FloatPoint(15.0, 10.0), a, b, FloatPoint(20.0, 10.0)));
}

TEST(CCLayerSorterTest, CalculateZDiff)
{
    // This should be bigger than the range of z values used.
    const float threshold = 10.0;

    // Trivial test, with one layer directly obscuring the other.

    CCLayerSorter::LayerShape front(
        FloatPoint3D(-1.0, 1.0, 5.0),
        FloatPoint3D(1.0, 1.0, 5.0),
        FloatPoint3D(1.0, -1.0, 5.0),
        FloatPoint3D(-1.0, -1.0, 5.0));

    CCLayerSorter::LayerShape back(
        FloatPoint3D(-1.0, 1.0, 4.0),
        FloatPoint3D(1.0, 1.0, 4.0),
        FloatPoint3D(1.0, -1.0, 4.0),
        FloatPoint3D(-1.0, -1.0, 4.0));

    EXPECT_GT(CCLayerSorter::calculateZDiff(front, back, threshold), 0.0);
    EXPECT_LT(CCLayerSorter::calculateZDiff(back, front, threshold), 0.0);

    // When comparing a layer with itself, zDiff is always 0.
    EXPECT_EQ(CCLayerSorter::calculateZDiff(front, front, threshold), 0.0);
    EXPECT_EQ(CCLayerSorter::calculateZDiff(back, back, threshold), 0.0);

    // Same again but with two layers that intersect only at one point (0,0).
    // This *does* count as obscuring, so we should get the same results.

    front = CCLayerSorter::LayerShape(
        FloatPoint3D(-1.0, 0.0, 5.0),
        FloatPoint3D(0.0, 0.0, 5.0),
        FloatPoint3D(0.0, -1.0, 5.0),
        FloatPoint3D(-1.0, -1.0, 5.0));

    back = CCLayerSorter::LayerShape(
        FloatPoint3D(0.0, 1.0, 4.0),
        FloatPoint3D(1.0, 1.0, 4.0),
        FloatPoint3D(1.0, 0.0, 4.0),
        FloatPoint3D(0.0, 0.0, 4.0));

    EXPECT_GT(CCLayerSorter::calculateZDiff(front, back, threshold), 0.0);
    EXPECT_LT(CCLayerSorter::calculateZDiff(back, front, threshold), 0.0);
    EXPECT_EQ(CCLayerSorter::calculateZDiff(front, front, threshold), 0.0);
    EXPECT_EQ(CCLayerSorter::calculateZDiff(back, back, threshold), 0.0);

    // Trickier test with layers at an angle.
    //
    //   -x . . . . 0 . . . . +x
    // -z             /
    //  :            /----B----
    //  0           C
    //  : ----A----/
    // +z         /
    //
    // C is in front of A and behind B (not what you'd expect by comparing centers).
    // A and B don't overlap, so they're incomparable (zDiff = 0).

    const float yHi = 10.0;
    const float yLo = -10.0;
    const float zA = 1.0;
    const float zB = -1.0;

    CCLayerSorter::LayerShape layerA(
        FloatPoint3D(-10.0, yHi, zA),
        FloatPoint3D(-2.0, yHi, zA),
        FloatPoint3D(-2.0, yLo, zA),
        FloatPoint3D(-10.0, yLo, zA));

    CCLayerSorter::LayerShape layerB(
        FloatPoint3D(2.0, yHi, zB),
        FloatPoint3D(10.0, yHi, zB),
        FloatPoint3D(10.0, yLo, zB),
        FloatPoint3D(2.0, yLo, zB));

    CCLayerSorter::LayerShape layerC(
        FloatPoint3D(-5.0, yHi, 5.0),
        FloatPoint3D(5.0, yHi, -5.0),
        FloatPoint3D(5.0, yLo, -5.0),
        FloatPoint3D(-5.0, yLo, 5.0));

    EXPECT_EQ(CCLayerSorter::calculateZDiff(layerA, layerA, threshold), 0.0);
    EXPECT_EQ(CCLayerSorter::calculateZDiff(layerA, layerB, threshold), 0.0);
    EXPECT_LT(CCLayerSorter::calculateZDiff(layerA, layerC, threshold), 0.0);

    EXPECT_EQ(CCLayerSorter::calculateZDiff(layerB, layerA, threshold), 0.0);
    EXPECT_EQ(CCLayerSorter::calculateZDiff(layerB, layerB, threshold), 0.0);
    EXPECT_GT(CCLayerSorter::calculateZDiff(layerB, layerC, threshold), 0.0);

    EXPECT_GT(CCLayerSorter::calculateZDiff(layerC, layerA, threshold), 0.0);
    EXPECT_LT(CCLayerSorter::calculateZDiff(layerC, layerB, threshold), 0.0);
    EXPECT_EQ(CCLayerSorter::calculateZDiff(layerC, layerC, threshold), 0.0);
}

} // namespace
