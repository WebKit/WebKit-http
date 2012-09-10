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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "WrapShapeInfo.h"

#if ENABLE(CSS_EXCLUSIONS)

#include "NotImplemented.h"
#include "RenderBlock.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

typedef HashMap<const RenderBlock*, OwnPtr<WrapShapeInfo> > WrapShapeInfoMap;

static WrapShapeInfoMap& wrapShapeInfoMap()
{
    DEFINE_STATIC_LOCAL(WrapShapeInfoMap, staticWrapShapeInfoMap, ());
    return staticWrapShapeInfoMap;
}

WrapShapeInfo::WrapShapeInfo(RenderBlock* block)
    : m_block(block)
    , m_wrapShapeSizeDirty(true)
{
}

WrapShapeInfo::~WrapShapeInfo()
{
}

WrapShapeInfo* WrapShapeInfo::ensureWrapShapeInfoForRenderBlock(RenderBlock* block)
{
    WrapShapeInfoMap::AddResult result = wrapShapeInfoMap().add(block, create(block));
    return result.iterator->second.get();
}

WrapShapeInfo* WrapShapeInfo::wrapShapeInfoForRenderBlock(const RenderBlock* block)
{
    ASSERT(block->style()->wrapShapeInside());
    return wrapShapeInfoMap().get(block);
}

bool WrapShapeInfo::isWrapShapeInfoEnabledForRenderBlock(const RenderBlock* block)
{
    // FIXME: Bug 89705: Enable shape inside for vertical writing modes
    if (!block->isHorizontalWritingMode())
        return false;

    // FIXME: Bug 89707: Enable shape inside for non-rectangular shapes
    BasicShape* shape = block->style()->wrapShapeInside();
    return (shape && shape->type() == BasicShape::BASIC_SHAPE_RECTANGLE);
}

void WrapShapeInfo::removeWrapShapeInfoForRenderBlock(const RenderBlock* block)
{
    if (!block->style() || !block->style()->wrapShapeInside())
        return;
    wrapShapeInfoMap().remove(block);
}

bool WrapShapeInfo::hasSegments() const
{
    // All line positions within a shape should have at least one segment
    ASSERT(lineState() != LINE_INSIDE_SHAPE || m_segments.size());
    return m_segments.size();
}

void WrapShapeInfo::computeShapeSize(LayoutUnit logicalWidth, LayoutUnit logicalHeight)
{
    if (!m_wrapShapeSizeDirty && logicalWidth == m_logicalWidth && logicalHeight == m_logicalHeight)
        return;

    m_wrapShapeSizeDirty = false;
    m_logicalWidth = logicalWidth;
    m_logicalHeight = logicalHeight;

    // FIXME: Bug 89993: The wrap shape may come from the parent object
    BasicShape* shape = m_block->style()->wrapShapeInside();
    ASSERT(shape);

    m_shape = ExclusionShape::createExclusionShape(shape, logicalWidth, logicalHeight);
    ASSERT(m_shape);
}

bool WrapShapeInfo::computeSegmentsForLine(LayoutUnit lineTop)
{
    m_lineTop = lineTop;
    m_segments.clear();

    if (lineState() == LINE_INSIDE_SHAPE) {
        ASSERT(m_shape);

        Vector<ExclusionInterval> intervals;
        m_shape->getInsideIntervals(lineTop, lineTop, intervals); // FIXME: Bug 95479, workaround for now
        for (size_t i = 0; i < intervals.size(); i++) {
            LineSegment segment;
            segment.logicalLeft = intervals[i].x1;
            segment.logicalRight = intervals[i].x2;
            m_segments.append(segment);
        }
    }
    return m_segments.size();
}

}
#endif
