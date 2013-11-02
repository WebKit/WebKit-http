/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
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

#include "config.h"
#include "LineWidth.h"

#include "RenderBlockFlow.h"
#include "RenderRubyRun.h"

namespace WebCore {

LineWidth::LineWidth(RenderBlockFlow& block, bool isFirstLine, IndentTextOrNot shouldIndentText)
    : m_block(block)
    , m_uncommittedWidth(0)
    , m_committedWidth(0)
    , m_overhangWidth(0)
    , m_trailingWhitespaceWidth(0)
    , m_trailingCollapsedWhitespaceWidth(0)
    , m_left(0)
    , m_right(0)
    , m_availableWidth(0)
#if ENABLE(CSS_SHAPES)
    , m_segment(0)
#endif
    , m_isFirstLine(isFirstLine)
    , m_shouldIndentText(shouldIndentText)
{
#if ENABLE(CSS_SHAPES)
    updateCurrentShapeSegment();
#endif
    updateAvailableWidth();
}

bool LineWidth::fitsOnLine(bool ignoringTrailingSpace) const
{
    return ignoringTrailingSpace ? fitsOnLineExcludingTrailingCollapsedWhitespace() : fitsOnLineIncludingExtraWidth(0);
}

bool LineWidth::fitsOnLineIncludingExtraWidth(float extra) const
{
    return currentWidth() + extra <= m_availableWidth;
}

bool LineWidth::fitsOnLineExcludingTrailingWhitespace(float extra) const
{
    return currentWidth() - m_trailingWhitespaceWidth + extra <= m_availableWidth;
}

void LineWidth::updateAvailableWidth(LayoutUnit replacedHeight)
{
    LayoutUnit height = m_block.logicalHeight();
    LayoutUnit logicalHeight = m_block.minLineHeightForReplacedRenderer(m_isFirstLine, replacedHeight);
    m_left = m_block.logicalLeftOffsetForLine(height, shouldIndentText(), logicalHeight);
    m_right = m_block.logicalRightOffsetForLine(height, shouldIndentText(), logicalHeight);

#if ENABLE(CSS_SHAPES)
    if (m_segment) {
        m_left = std::max<float>(m_segment->logicalLeft, m_left);
        m_right = std::min<float>(m_segment->logicalRight, m_right);
    }
#endif

    computeAvailableWidthFromLeftAndRight();
}

void LineWidth::shrinkAvailableWidthForNewFloatIfNeeded(FloatingObject* newFloat)
{
    LayoutUnit height = m_block.logicalHeight();
    if (height < m_block.logicalTopForFloat(newFloat) || height >= m_block.logicalBottomForFloat(newFloat))
        return;

#if ENABLE(CSS_SHAPES)
    // When floats with shape outside are stacked, the floats are positioned based on the margin box of the float,
    // not the shape's contour. Since we computed the width based on the shape contour when we added the float,
    // when we add a subsequent float on the same line, we need to undo the shape delta in order to position
    // based on the margin box. In order to do this, we need to walk back through the floating object list to find
    // the first previous float that is on the same side as our newFloat.
    ShapeOutsideInfo* previousShapeOutsideInfo = 0;
    const FloatingObjectSet& floatingObjectSet = m_block.m_floatingObjects->set();
    auto it = floatingObjectSet.end();
    auto begin = floatingObjectSet.begin();
    LayoutUnit lineHeight = m_block.lineHeight(m_isFirstLine, m_block.isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes);
    for (--it; it != begin; --it) {
        FloatingObject* previousFloat = it->get();
        if (previousFloat != newFloat && previousFloat->type() == newFloat->type()) {
            previousShapeOutsideInfo = previousFloat->renderer().shapeOutsideInfo();
            if (previousShapeOutsideInfo)
                previousShapeOutsideInfo->updateDeltasForContainingBlockLine(&m_block, previousFloat, m_block.logicalHeight(), lineHeight);
            break;
        }
    }

    ShapeOutsideInfo* shapeOutsideInfo = newFloat->renderer().shapeOutsideInfo();
    if (shapeOutsideInfo)
        shapeOutsideInfo->updateDeltasForContainingBlockLine(&m_block, newFloat, m_block.logicalHeight(), lineHeight);
#endif

    if (newFloat->type() == FloatingObject::FloatLeft) {
        float newLeft = m_block.logicalRightForFloat(newFloat);
#if ENABLE(CSS_SHAPES)
        if (previousShapeOutsideInfo)
            newLeft -= previousShapeOutsideInfo->rightMarginBoxDelta();
        if (shapeOutsideInfo)
            newLeft += shapeOutsideInfo->rightMarginBoxDelta();
#endif

        if (shouldIndentText() && m_block.style().isLeftToRightDirection())
            newLeft += floorToInt(m_block.textIndentOffset());
        m_left = std::max<float>(m_left, newLeft);
    } else {
        float newRight = m_block.logicalLeftForFloat(newFloat);
#if ENABLE(CSS_SHAPES)
        if (previousShapeOutsideInfo)
            newRight -= previousShapeOutsideInfo->leftMarginBoxDelta();
        if (shapeOutsideInfo)
            newRight += shapeOutsideInfo->leftMarginBoxDelta();
#endif

        if (shouldIndentText() && !m_block.style().isLeftToRightDirection())
            newRight -= floorToInt(m_block.textIndentOffset());
        m_right = std::min<float>(m_right, newRight);
    }

    computeAvailableWidthFromLeftAndRight();
}

void LineWidth::commit()
{
    m_committedWidth += m_uncommittedWidth;
    m_uncommittedWidth = 0;
}

void LineWidth::applyOverhang(RenderRubyRun* rubyRun, RenderObject* startRenderer, RenderObject* endRenderer)
{
    int startOverhang;
    int endOverhang;
    rubyRun->getOverhang(m_isFirstLine, startRenderer, endRenderer, startOverhang, endOverhang);

    startOverhang = std::min<int>(startOverhang, m_committedWidth);
    m_availableWidth += startOverhang;

    endOverhang = std::max(std::min<int>(endOverhang, m_availableWidth - currentWidth()), 0);
    m_availableWidth += endOverhang;
    m_overhangWidth += startOverhang + endOverhang;
}

void LineWidth::fitBelowFloats()
{
    ASSERT(!m_committedWidth);
    ASSERT(!fitsOnLine());

    LayoutUnit floatLogicalBottom;
    LayoutUnit lastFloatLogicalBottom = m_block.logicalHeight();
    float newLineWidth = m_availableWidth;
    float newLineLeft = m_left;
    float newLineRight = m_right;
    while (true) {
        floatLogicalBottom = m_block.nextFloatLogicalBottomBelow(lastFloatLogicalBottom, ShapeOutsideFloatShapeOffset);
        if (floatLogicalBottom <= lastFloatLogicalBottom)
            break;

        newLineLeft = m_block.logicalLeftOffsetForLine(floatLogicalBottom, shouldIndentText());
        newLineRight = m_block.logicalRightOffsetForLine(floatLogicalBottom, shouldIndentText());
        newLineWidth = std::max(0.0f, newLineRight - newLineLeft);
        lastFloatLogicalBottom = floatLogicalBottom;

#if ENABLE(CSS_SHAPES)
        // FIXME: This code should be refactored to incorporate with the code above.
        ShapeInsideInfo* shapeInsideInfo = m_block.layoutShapeInsideInfo();
        if (shapeInsideInfo) {
            LayoutUnit logicalOffsetFromShapeContainer = m_block.logicalOffsetFromShapeAncestorContainer(shapeInsideInfo->owner()).height();
            LayoutUnit lineHeight = m_block.lineHeight(false, m_block.isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes);
            shapeInsideInfo->updateSegmentsForLine(lastFloatLogicalBottom + logicalOffsetFromShapeContainer, lineHeight);
            updateCurrentShapeSegment();
            updateAvailableWidth();
        }
#endif
        if (newLineWidth >= m_uncommittedWidth)
            break;
    }

    if (newLineWidth > m_availableWidth) {
        m_block.setLogicalHeight(lastFloatLogicalBottom);
        m_availableWidth = newLineWidth + m_overhangWidth;
        m_left = newLineLeft;
        m_right = newLineRight;
    }
}

void LineWidth::setTrailingWhitespaceWidth(float collapsedWhitespace, float borderPaddingMargin)
{
    m_trailingCollapsedWhitespaceWidth = collapsedWhitespace;
    m_trailingWhitespaceWidth = collapsedWhitespace + borderPaddingMargin;
}

#if ENABLE(CSS_SHAPES)
void LineWidth::updateCurrentShapeSegment()
{
    if (ShapeInsideInfo* shapeInsideInfo = m_block.layoutShapeInsideInfo())
        m_segment = shapeInsideInfo->currentSegment();
}
#endif

void LineWidth::computeAvailableWidthFromLeftAndRight()
{
    m_availableWidth = std::max<float>(0, m_right - m_left) + m_overhangWidth;
}

bool LineWidth::fitsOnLineExcludingTrailingCollapsedWhitespace() const
{
    return currentWidth() - m_trailingCollapsedWhitespaceWidth <= m_availableWidth;
}

}
