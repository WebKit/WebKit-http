/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RenderMultiColumnBlock.h"

using namespace std;

namespace WebCore {

RenderMultiColumnBlock::RenderMultiColumnBlock(Node* node)
    : RenderBlock(node)
    , m_columnCount(1)
    , m_columnWidth(0)
{
}

void RenderMultiColumnBlock::computeColumnCountAndWidth()
{
    // Calculate our column width and column count.
    // FIXME: Can overflow on fast/block/float/float-not-removed-from-next-sibling4.html, see https://bugs.webkit.org/show_bug.cgi?id=68744
    m_columnCount = 1;
    m_columnWidth = contentLogicalWidth();
    
    ASSERT(!style()->hasAutoColumnCount() || !style()->hasAutoColumnWidth());

    LayoutUnit availWidth = m_columnWidth;
    LayoutUnit colGap = columnGap();
    LayoutUnit colWidth = max<LayoutUnit>(1, LayoutUnit(style()->columnWidth()));
    int colCount = max<int>(1, style()->columnCount());

    if (style()->hasAutoColumnWidth() && !style()->hasAutoColumnCount()) {
        m_columnCount = colCount;
        m_columnWidth = max<LayoutUnit>(0, (availWidth - ((m_columnCount - 1) * colGap)) / m_columnCount);
    } else if (!style()->hasAutoColumnWidth() && style()->hasAutoColumnCount()) {
        m_columnCount = max<LayoutUnit>(1, (availWidth + colGap) / (colWidth + colGap));
        m_columnWidth = ((availWidth + colGap) / m_columnCount) - colGap;
    } else {
        m_columnCount = max<LayoutUnit>(min<LayoutUnit>(colCount, (availWidth + colGap) / (colWidth + colGap)), 1);
        m_columnWidth = ((availWidth + colGap) / m_columnCount) - colGap;
    }
}

bool RenderMultiColumnBlock::recomputeLogicalWidth()
{
    bool relayoutChildren = RenderBlock::recomputeLogicalWidth();
    LayoutUnit oldColumnWidth = m_columnWidth;
    computeColumnCountAndWidth();
    if (m_columnWidth != oldColumnWidth)
        relayoutChildren = true;
    return relayoutChildren;
}

const char* RenderMultiColumnBlock::renderName() const
{    
    if (isFloating())
        return "RenderMultiColumnBlock (floating)";
    if (isPositioned())
        return "RenderMultiColumnBlock (positioned)";
    if (isAnonymousBlock())
        return "RenderMultiColumnBlock (anonymous)";
    if (isAnonymous())
        return "RenderMultiColumnBlock (generated)";
    if (isRelPositioned())
        return "RenderMultiColumnBlock (relative positioned)";
    return "RenderMultiColumnBlock";
}

}
