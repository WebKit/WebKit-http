/*
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/) 
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderTextControlMultiLine_h
#define RenderTextControlMultiLine_h

#include "RenderTextControl.h"

namespace WebCore {

class HTMLTextAreaElement;

class RenderTextControlMultiLine FINAL : public RenderTextControl {
public:
    RenderTextControlMultiLine(HTMLTextAreaElement&, PassRef<RenderStyle>);
    virtual ~RenderTextControlMultiLine();

    HTMLTextAreaElement& textAreaElement() const;

private:
    void element() const WTF_DELETED_FUNCTION;

    virtual bool isTextArea() const { return true; }

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) OVERRIDE;

    virtual float getAvgCharWidth(AtomicString family);
    virtual LayoutUnit preferredContentLogicalWidth(float charWidth) const;
    virtual LayoutUnit computeControlLogicalHeight(LayoutUnit lineHeight, LayoutUnit nonContentHeight) const OVERRIDE;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const;

    virtual PassRef<RenderStyle> createInnerTextStyle(const RenderStyle* startStyle) const;
    virtual RenderObject* layoutSpecialExcludedChild(bool relayoutChildren);
};

RENDER_OBJECT_TYPE_CASTS(RenderTextControlMultiLine, isTextArea())

}

#endif
