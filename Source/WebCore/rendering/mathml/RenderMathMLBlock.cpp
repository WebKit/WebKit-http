/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
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

#if ENABLE(MATHML)

#include "RenderMathMLBlock.h"

#include "FontSelector.h"
#include "GraphicsContext.h"
#include "MathMLNames.h"
#include "RenderInline.h"
#include "RenderText.h"

namespace WebCore {
    
using namespace MathMLNames;
    
RenderMathMLBlock::RenderMathMLBlock(Node* container) 
    : RenderBlock(container) 
{
}

bool RenderMathMLBlock::isChildAllowed(RenderObject* child, RenderStyle*) const
{
    return child->node() && child->node()->nodeType() == Node::ELEMENT_NODE;
}

PassRefPtr<RenderStyle> RenderMathMLBlock::makeBlockStyle()
{
    RefPtr<RenderStyle> newStyle = RenderStyle::create();
    newStyle->inheritFrom(style());
    newStyle->setDisplay(BLOCK);
    return newStyle;
}

int RenderMathMLBlock::nonOperatorHeight() const
{
    if (!isRenderMathMLOperator())
        return offsetHeight();
        
    return 0;
}

void RenderMathMLBlock::stretchToHeight(int height) 
{
    for (RenderObject* current = firstChild(); current; current = current->nextSibling())
       if (current->isRenderMathMLBlock()) {
          RenderMathMLBlock* block = toRenderMathMLBlock(current);
          block->stretchToHeight(height);
       }
}

#if ENABLE(DEBUG_MATH_LAYOUT)
void RenderMathMLBlock::paint(PaintInfo& info, const LayoutPoint& paintOffset)
{
    RenderBlock::paint(info, paintOffset);
    
    if (info.context->paintingDisabled() || info.phase != PaintPhaseForeground)
        return;

    LayoutPoint adjustedPaintOffset = paintOffset + location();

    GraphicsContextStateSaver stateSaver(*info.context);
    
    info.context->setStrokeThickness(1.0f);
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeColor(Color(0, 0, 255), ColorSpaceSRGB);
    
    info.context->drawLine(adjustedPaintOffset, LayoutPoint(adjustedPaintOffset.x() + offsetWidth(), adjustedPaintOffset.y()));
    info.context->drawLine(LayoutPoint(adjustedPaintOffset.x() + offsetWidth(), adjustedPaintOffset.y()), LayoutPoint(adjustedPaintOffset.x() + offsetWidth(), adjustedPaintOffset.y() + offsetHeight()));
    info.context->drawLine(LayoutPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + offsetHeight()), LayoutPoint(adjustedPaintOffset.x() + offsetWidth(), adjustedPaintOffset.y() + offsetHeight()));
    info.context->drawLine(adjustedPaintOffset, LayoutPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + offsetHeight()));
    
    int topStart = paddingTop();
    
    info.context->setStrokeColor(Color(0, 255, 0), ColorSpaceSRGB);
    
    info.context->drawLine(LayoutPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + topStart), LayoutPoint(adjustedPaintOffset.x() + offsetWidth(), adjustedPaintOffset.y() + topStart));
    
    int baseline = baselinePosition(AlphabeticBaseline, true, HorizontalLine);
    
    info.context->setStrokeColor(Color(255, 0, 0), ColorSpaceSRGB);
    
    info.context->drawLine(LayoutPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + baseline), LayoutPoint(adjustedPaintOffset.x() + offsetWidth(), adjustedPaintOffset.y() + baseline));
}
#endif // ENABLE(DEBUG_MATH_LAYOUT)


}    

#endif
