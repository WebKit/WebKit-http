/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2010 François Sausset (sausset@gmail.com). All rights reserved.
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

#include "RenderMathMLSquareRoot.h"

#include "GraphicsContext.h"
#include "MathMLNames.h"
#include "PaintInfo.h"

namespace WebCore {
    
using namespace MathMLNames;

// Lower the radical sign's bottom point (px)
const int gRadicalBottomPointLower = 3;
// Threshold above which the radical shape is modified to look nice with big bases (em)
const float gThresholdBaseHeightEms = 1.5f;
// Front width (em)
const float gFrontWidthEms = 0.75f;
// Horizontal position of the bottom point of the radical (* frontWidth)
const float gRadicalBottomPointXFront = 0.5f;
// Horizontal position of the top left point of the radical "dip" (* frontWidth)
const float gRadicalDipLeftPointXFront = 0.2f;
// Vertical position of the top left point of the radical "dip" (* baseHeight)
const float gRadicalDipLeftPointYPos = 0.5f; 
// Vertical shift of the left end point of the radical (em)
const float gRadicalLeftEndYShiftEms = 0.05f;
// Additional bottom root padding if baseHeight > threshold (em)
const float gBigRootBottomPaddingEms = 0.2f;

// Radical line thickness (em)
const float gRadicalLineThicknessEms = 0.02f;
// Radical thick line thickness (em)
const float gRadicalThickLineThicknessEms = 0.1f;
    
RenderMathMLSquareRoot::RenderMathMLSquareRoot(Element* element)
    : RenderMathMLBlock(element)
{
}

void RenderMathMLSquareRoot::paint(PaintInfo& info, const LayoutPoint& paintOffset)
{
    RenderMathMLBlock::paint(info, paintOffset);
    
    if (info.context->paintingDisabled())
        return;
    
    IntPoint adjustedPaintOffset = roundedIntPoint(paintOffset + location());
    
    int baseHeight = 0;
    int overbarWidth = 0;
    RenderObject* current = firstChild();
    while (current) {
        if (current->isBoxModelObject()) {
            RenderBoxModelObject* box = toRenderBoxModelObject(current);
            
            // Check to see if this box has a larger height
            // FIXME: We should consider the height above & below the baseline separately. This will be
            // fixed by an <mrow> base wrapper, which is required anyway by the MathML spec.
            if (box->pixelSnappedOffsetHeight() > baseHeight)
                baseHeight = box->pixelSnappedOffsetHeight();
            overbarWidth += box->pixelSnappedOffsetWidth();
        }
        current = current->nextSibling();
    }
    // default to the font size in pixels if we're empty
    if (!baseHeight)
        baseHeight = style()->fontSize();
    
    int frontWidth = static_cast<int>(style()->fontSize() * gFrontWidthEms);
    int overbarLeftPointShift = 0;
    // Base height above which the shape of the root changes
    int thresholdHeight = static_cast<int>(gThresholdBaseHeightEms * style()->fontSize());
    
    if (baseHeight > thresholdHeight && thresholdHeight) {
        float shift = (baseHeight - thresholdHeight) / static_cast<float>(thresholdHeight);
        if (shift > 1.)
            shift = 1.0f;
        overbarLeftPointShift = static_cast<int>(gRadicalBottomPointXFront * frontWidth * shift);
    }
    
    overbarWidth += overbarLeftPointShift;
    
    FloatPoint overbarLeftPoint(adjustedPaintOffset.x() + frontWidth - overbarLeftPointShift, adjustedPaintOffset.y());
    FloatPoint bottomPoint(adjustedPaintOffset.x() + frontWidth * gRadicalBottomPointXFront, adjustedPaintOffset.y() + baseHeight + gRadicalBottomPointLower);
    FloatPoint dipLeftPoint(adjustedPaintOffset.x() + frontWidth * gRadicalDipLeftPointXFront, adjustedPaintOffset.y() + gRadicalDipLeftPointYPos * baseHeight);
    FloatPoint leftEnd(adjustedPaintOffset.x(), dipLeftPoint.y() + gRadicalLeftEndYShiftEms * style()->fontSize());
    
    GraphicsContextStateSaver stateSaver(*info.context);
    
    info.context->setStrokeThickness(gRadicalLineThicknessEms * style()->fontSize());
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeColor(style()->visitedDependentColor(CSSPropertyColor), ColorSpaceDeviceRGB);
    info.context->setLineJoin(MiterJoin);
    info.context->setMiterLimit(style()->fontSize());
    
    Path root;
    
    root.moveTo(FloatPoint(overbarLeftPoint.x() + overbarWidth, adjustedPaintOffset.y()));
    // draw top
    root.addLineTo(overbarLeftPoint);
    // draw from top left corner to bottom point of radical
    root.addLineTo(bottomPoint);
    // draw from bottom point to top of left part of radical base "dip"
    root.addLineTo(dipLeftPoint);
    // draw to end
    root.addLineTo(leftEnd);
    
    info.context->strokePath(root);
    
    GraphicsContextStateSaver maskStateSaver(*info.context);
    
    // Build a mask to draw the thick part of the root.
    Path mask;
    
    mask.moveTo(overbarLeftPoint);
    mask.addLineTo(bottomPoint);
    mask.addLineTo(dipLeftPoint);
    mask.addLineTo(FloatPoint(2 * dipLeftPoint.x() - leftEnd.x(), 2 * dipLeftPoint.y() - leftEnd.y()));
    
    info.context->clip(mask);
    
    // Draw the thick part of the root.
    info.context->setStrokeThickness(gRadicalThickLineThicknessEms * style()->fontSize());
    info.context->setLineCap(SquareCap);
    
    Path line;
    line.moveTo(bottomPoint);
    line.addLineTo(dipLeftPoint);
    
    info.context->strokePath(line);
}

void RenderMathMLSquareRoot::layout()
{
    int baseHeight = 0;
    
    RenderObject* current = firstChild();
    while (current) {
        if (current->isBoxModelObject()) {
            RenderBoxModelObject* box = toRenderBoxModelObject(current);
            
            if (box->pixelSnappedOffsetHeight() > baseHeight)
                baseHeight = box->pixelSnappedOffsetHeight();
            
            // FIXME: Can box->style() be modified?
            box->style()->setVerticalAlign(BASELINE);
        }
        current = current->nextSibling();
    }
    
    if (!baseHeight)
        baseHeight = style()->fontSize();
    
    // FIXME: Can style() be modified? And don't we need styleDidChange()?
    if (baseHeight > static_cast<int>(gThresholdBaseHeightEms * style()->fontSize()))
        style()->setPaddingBottom(Length(static_cast<int>(gBigRootBottomPaddingEms * style()->fontSize()), Fixed));
    
    RenderBlock::layout();
}
    
}

#endif // ENABLE(MATHML)
