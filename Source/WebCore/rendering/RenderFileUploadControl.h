/*
 * Copyright (C) 2006, 2007, 2009, 2012 Apple Inc. All rights reserved.
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

#ifndef RenderFileUploadControl_h
#define RenderFileUploadControl_h

#include "RenderBlockFlow.h"

namespace WebCore {

class HTMLInputElement;

// Each RenderFileUploadControl contains a RenderButton (for opening the file chooser), and
// sufficient space to draw a file icon and filename. The RenderButton has a shadow node
// associated with it to receive click/hover events.

class RenderFileUploadControl FINAL : public RenderBlockFlow {
public:
    RenderFileUploadControl(HTMLInputElement&, PassRef<RenderStyle>);
    virtual ~RenderFileUploadControl();

    String buttonValue();
    String fileTextValue() const;

    HTMLInputElement& inputElement() const;
    
private:
    void element() const WTF_DELETED_FUNCTION;

    virtual bool isFileUploadControl() const OVERRIDE { return true; }

    virtual const char* renderName() const OVERRIDE { return "RenderFileUploadControl"; }

    virtual bool canBeReplacedWithInlineRunIn() const OVERRIDE;
    virtual void updateFromElement() OVERRIDE;
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const OVERRIDE;
    virtual void computePreferredLogicalWidths() OVERRIDE;
    virtual void paintObject(PaintInfo&, const LayoutPoint&) OVERRIDE;

    virtual bool requiresForcedStyleRecalcPropagation() const OVERRIDE { return true; }

    int maxFilenameWidth() const;
    
    virtual VisiblePosition positionForPoint(const LayoutPoint&) OVERRIDE;

    HTMLInputElement* uploadButton() const;

    bool m_canReceiveDroppedFiles;
};

RENDER_OBJECT_TYPE_CASTS(RenderFileUploadControl, isFileUploadControl())

} // namespace WebCore

#endif // RenderFileUploadControl_h
