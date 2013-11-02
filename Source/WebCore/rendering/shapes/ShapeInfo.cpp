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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
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
#include "ShapeInfo.h"

#if ENABLE(CSS_SHAPES)

#include "RenderBlock.h"
#include "RenderBox.h"
#include "RenderRegion.h"
#include "RenderStyle.h"
#include "Shape.h"

namespace WebCore {


bool checkShapeImageOrigin(Document& document, CachedImage& cachedImage)
{
    if (cachedImage.isOriginClean(document.securityOrigin()))
        return true;

    const URL& url = cachedImage.url();
    String urlString = url.isNull() ? "''" : url.stringCenterEllipsizedToLength();
    document.addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, "Unsafe attempt to load URL " + urlString + ".");

    return false;
}

template<class RenderType>
const Shape* ShapeInfo<RenderType>::computedShape() const
{
    if (Shape* shape = m_shape.get())
        return shape;

    WritingMode writingMode = m_renderer->style().writingMode();
    Length margin = m_renderer->style().shapeMargin();
    Length padding = m_renderer->style().shapePadding();
    float shapeImageThreshold = m_renderer->style().shapeImageThreshold();
    const ShapeValue* shapeValue = this->shapeValue();
    ASSERT(shapeValue);

    switch (shapeValue->type()) {
    case ShapeValue::Shape:
        ASSERT(shapeValue->shape());
        m_shape = Shape::createShape(shapeValue->shape(), m_shapeLogicalSize, writingMode, margin, padding);
        break;
    case ShapeValue::Image:
        ASSERT(shapeValue->image());
        m_shape = Shape::createShape(shapeValue->image(), shapeImageThreshold, m_shapeLogicalSize, writingMode, margin, padding);
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    ASSERT(m_shape);
    return m_shape.get();
}

template<class RenderType>
SegmentList ShapeInfo<RenderType>::computeSegmentsForLine(LayoutUnit lineTop, LayoutUnit lineHeight) const
{
    ASSERT(lineHeight >= 0);
    SegmentList segments;

    getIntervals((lineTop - logicalTopOffset()), std::min(lineHeight, shapeLogicalBottom() - lineTop), segments);

    for (size_t i = 0; i < segments.size(); i++) {
        segments[i].logicalLeft += logicalLeftOffset();
        segments[i].logicalRight += logicalLeftOffset();
    }

    return segments;
}

template class ShapeInfo<RenderBlock>;
template class ShapeInfo<RenderBox>;
}
#endif
