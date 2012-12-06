/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ExclusionShape_h
#define ExclusionShape_h

#include "BasicShapes.h"
#include "FloatRect.h"
#include "WritingMode.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

struct LineSegment {
    LineSegment(float logicalLeft, float logicalRight)
        : logicalLeft(logicalLeft)
        , logicalRight(logicalRight)
    {
    }

    float logicalLeft;
    float logicalRight;
};

typedef Vector<LineSegment> SegmentList;


// A representation of a BasicShape that enables layout code to determine how to break a line up into segments
// that will fit within or around a shape. The line is defined by a pair of logical Y coordinates and the
// computed segments are returned as pairs of logical X coordinates. The BasicShape itself is defined in
// physical coordinates.

class ExclusionShape {
public:
    static PassOwnPtr<ExclusionShape> createExclusionShape(const BasicShape*, float logicalBoxWidth, float logicalBoxHeight, WritingMode);

    virtual ~ExclusionShape() { }

    virtual FloatRect shapeLogicalBoundingBox() const = 0;
    virtual bool isEmpty() const = 0;
    virtual void getIncludedIntervals(float logicalTop, float logicalHeight, SegmentList&) const = 0;
    virtual void getExcludedIntervals(float logicalTop, float logicalHeight, SegmentList&) const = 0;
    virtual bool firstIncludedIntervalLogicalTop(float minLogicalIntervalTop, const FloatSize& minLogicalIntervalSize, float& result) const = 0;

protected:
    float minYForLogicalLine(float logicalTop, float logicalHeight) const { return isFlippedBlocksWritingMode(m_writingMode) ? m_logicalBoxHeight - logicalTop - logicalHeight : logicalTop; }
    float maxYForLogicalLine(float logicalTop, float logicalHeight) const { return  isFlippedBlocksWritingMode(m_writingMode) ? m_logicalBoxHeight - logicalTop : logicalTop + logicalHeight; }
    FloatRect internalToLogicalBoundingBox(const FloatRect& r) const { return isFlippedBlocksWritingMode(m_writingMode) ? FloatRect(r.x(), m_logicalBoxHeight - r.maxY(), r.width(), r.height()) : r; }
    float logicalTopForMinY(float minY, float logicalHeight) const { return isFlippedBlocksWritingMode(m_writingMode) ? m_logicalBoxHeight - minY - logicalHeight : minY; }

private:
    WritingMode m_writingMode;
    float m_logicalBoxWidth;
    float m_logicalBoxHeight;
    FloatRect m_boundingBox;
};

} // namespace WebCore

#endif // ExclusionShape_h
