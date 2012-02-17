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

#include "RenderMathMLUnderOver.h"

#include "FontSelector.h"
#include "MathMLNames.h"

namespace WebCore {

using namespace MathMLNames;
    
static const double gOverSpacingAdjustment = 0.5;
    
RenderMathMLUnderOver::RenderMathMLUnderOver(Element* element)
    : RenderMathMLBlock(element)
{
    // Determine what kind of under/over expression we have by element name
    if (element->hasLocalName(MathMLNames::munderTag))
        m_kind = Under;
    else if (element->hasLocalName(MathMLNames::moverTag))
        m_kind = Over;
    else {
        ASSERT(element->hasLocalName(MathMLNames::munderoverTag));
        m_kind = UnderOver;
    }
}

RenderBoxModelObject* RenderMathMLUnderOver::base() const
{
    RenderObject* baseWrapper = firstChild();
    if ((m_kind == Over || m_kind == UnderOver) && baseWrapper)
        baseWrapper = baseWrapper->nextSibling();
    if (!baseWrapper)
        return 0;
    RenderObject* base = baseWrapper->firstChild();
    if (!base || !base->isBoxModelObject())
        return 0;
    return toRenderBoxModelObject(base);
}

void RenderMathMLUnderOver::addChild(RenderObject* child, RenderObject* beforeChild)
{    
    RenderMathMLBlock* row = new (renderArena()) RenderMathMLBlock(node());
    RefPtr<RenderStyle> rowStyle = createBlockStyle();
    row->setStyle(rowStyle.release());
    row->setIsAnonymous(true);
    
    // look through the children for rendered elements counting the blocks so we know what child
    // we are adding
    int blocks = 0;
    RenderObject* current = this->firstChild();
    while (current) {
        blocks++;
        current = current->nextSibling();
    }
    
    switch (blocks) {
    case 0:
        // this is the base so just append it
        RenderBlock::addChild(row, beforeChild);
        break;
    case 1:
        // the under or over
        // FIXME: text-align: center does not work
        row->style()->setTextAlign(CENTER);
        if (m_kind == Over) {
            // add the over as first
            RenderBlock::addChild(row, firstChild());
        } else {
            // add the under as last
            RenderBlock::addChild(row, beforeChild);
        }
        break;
    case 2:
        // the under or over
        // FIXME: text-align: center does not work
        row->style()->setTextAlign(CENTER);
        if (m_kind == UnderOver) {
            // add the over as first
            RenderBlock::addChild(row, firstChild());
        } else {
            // we really shouldn't get here as only munderover should have three children
            RenderBlock::addChild(row, beforeChild);
        }
        break;
    default:
        // munderover shouldn't have more than three children. In theory we shouldn't 
        // get here if the MathML is correctly formed, but that isn't a guarantee.
        // We will treat this as another under element and they'll get something funky.
        RenderBlock::addChild(row, beforeChild);
    }
    row->addChild(child);    
}

RenderMathMLOperator* RenderMathMLUnderOver::unembellishedOperator()
{
    RenderBoxModelObject* base = this->base();
    if (!base || !base->isRenderMathMLBlock())
        return 0;
    return toRenderMathMLBlock(base)->unembellishedOperator();
}

inline int getOffsetHeight(RenderObject* obj) 
{
    if (obj->isBoxModelObject()) {
        RenderBoxModelObject* box = toRenderBoxModelObject(obj);
        return box->offsetHeight();
    }
   
    return 0;
}

void RenderMathMLUnderOver::stretchToHeight(int height)
{
    RenderBoxModelObject* base = this->base();
    if (base && base->isRenderMathMLBlock()) {
        RenderMathMLBlock* block = toRenderMathMLBlock(base);
        block->stretchToHeight(height);
        setNeedsLayout(true);
    }
}

void RenderMathMLUnderOver::layout() 
{
    RenderBlock::layout();
    RenderObject* over = 0;
    RenderObject* base = 0;
    switch (m_kind) {
    case Over:
        // We need to calculate the baseline over the over versus the start of the base and 
        // adjust the placement of the base.
        over = firstChild();
        if (over) {
            // FIXME: descending glyphs intrude into base (e.g. lowercase y over base)
            // FIXME: bases that ascend higher than the line box intrude into the over
            if (!over->firstChild() || !over->firstChild()->isBoxModelObject())
                break;
            
            LayoutUnit overSpacing = static_cast<LayoutUnit>(gOverSpacingAdjustment * (getOffsetHeight(over) - toRenderBoxModelObject(over->firstChild())->baselinePosition(AlphabeticBaseline, true, HorizontalLine)));
            
            // base row wrapper
            base = over->nextSibling();
            if (base) {
                if (overSpacing > 0) 
                    base->style()->setMarginTop(Length(-overSpacing, Fixed));
                else 
                    base->style()->setMarginTop(Length(0, Fixed));
            }
            
        }
        break;
    case Under:
        // FIXME: Non-ascending glyphs in the under should be moved closer to the base

        // We need to calculate the baseline of the base versus the start of the under block and
        // adjust the placement of the under block.
        
        // base row wrapper
        base = firstChild();
        if (base) {
            LayoutUnit baseHeight = getOffsetHeight(base);
            // actual base
            base = base->firstChild();
            if (!base || !base->isBoxModelObject())
                break;
            
            // FIXME: We need to look at the space between a single maximum height of
            //        the line boxes and the baseline and squeeze them together
            LayoutUnit underSpacing = baseHeight - toRenderBoxModelObject(base)->baselinePosition(AlphabeticBaseline, true, HorizontalLine);
            
            // adjust the base's intrusion into the under
            RenderObject* under = lastChild();
            if (under && underSpacing > 0)
                under->style()->setMarginTop(Length(-underSpacing, Fixed));
        }
        break;
    case UnderOver:
        // FIXME: Non-descending glyphs in the over should be moved closer to the base
        // FIXME: Non-ascending glyphs in the under should be moved closer to the base
        
        // We need to calculate the baseline of the over versus the start of the base and 
        // adjust the placement of the base.
        
        over = firstChild();
        if (over) {
            // FIXME: descending glyphs intrude into base (e.g. lowercase y over base)
            // FIXME: bases that ascend higher than the line box intrude into the over
            if (!over->firstChild() || !over->firstChild()->isBoxModelObject())
                break;
            LayoutUnit overSpacing = static_cast<LayoutUnit>(gOverSpacingAdjustment * (getOffsetHeight(over) - toRenderBoxModelObject(over->firstChild())->baselinePosition(AlphabeticBaseline, true, HorizontalLine)));
            
            // base row wrapper
            base = over->nextSibling();
            
            if (base) {
                if (overSpacing > 0)
                    base->style()->setMarginTop(Length(-overSpacing, Fixed));
                
                // We need to calculate the baseline of the base versus the start of the under block and
                // adjust the placement of the under block.
                
                LayoutUnit baseHeight = getOffsetHeight(base);
                // actual base
                base = base->firstChild();
                if (!base || !base->isBoxModelObject())
                    break;

                // FIXME: We need to look at the space between a single maximum height of
                //        the line boxes and the baseline and squeeze them together
                LayoutUnit underSpacing = baseHeight - toRenderBoxModelObject(base)->baselinePosition(AlphabeticBaseline, true, HorizontalLine);
                
                RenderObject* under = lastChild();
                if (under && under->firstChild() && under->firstChild()->isRenderInline() && underSpacing > 0)
                    under->style()->setMarginTop(Length(-underSpacing, Fixed));
                
            }
        }
        break;
    }
    setNeedsLayout(true);
    RenderBlock::layout();
}

LayoutUnit RenderMathMLUnderOver::baselinePosition(FontBaseline, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    RenderObject* current = firstChild();
    if (!current || linePositionMode == PositionOfInteriorLineBoxes)
        return RenderBlock::baselinePosition(AlphabeticBaseline, firstLine, direction, linePositionMode);

    LayoutUnit baseline = 0;
    switch (m_kind) {
    case UnderOver:
    case Over:
        baseline += getOffsetHeight(current);
        current = current->nextSibling();
        if (current) {
            // actual base
            RenderObject* base = current->firstChild();
            if (!base || !base->isBoxModelObject())
                break;
            baseline += toRenderBoxModelObject(base)->baselinePosition(AlphabeticBaseline, firstLine, HorizontalLine, linePositionMode);
            // added the negative top margin
            baseline += current->style()->marginTop().value();
        }
        break;
    case Under:
        RenderObject* base = current->firstChild();
        if (base && base->isBoxModelObject())
            baseline += toRenderBoxModelObject(base)->baselinePosition(AlphabeticBaseline, true, HorizontalLine);
    }

    return baseline;
}


int RenderMathMLUnderOver::nonOperatorHeight() const 
{
    int nonOperators = 0;
    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        if (current->firstChild() && current->firstChild()->isRenderMathMLBlock()) {
            RenderMathMLBlock* block = toRenderMathMLBlock(current->firstChild());
            if (!block->isRenderMathMLOperator()) 
                nonOperators += getOffsetHeight(current);
        } else
            nonOperators += getOffsetHeight(current);
    }
    return nonOperators;
}

}

#endif // ENABLE(MATHML)
