/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 University of Washington.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "FrameSnapshotting.h"

#include "Document.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "Page.h"
#include "RenderObject.h"

namespace WebCore {

struct ScopedFramePaintingState {
    ScopedFramePaintingState(Frame& frame, Node* node)
        : frame(frame)
        , node(node)
        , paintBehavior(frame.view()->paintBehavior())
        , backgroundColor(frame.view()->baseBackgroundColor())
    {
        ASSERT(!node || node->renderer());
    }

    ~ScopedFramePaintingState()
    {
        frame.view()->setPaintBehavior(paintBehavior);
        frame.view()->setBaseBackgroundColor(backgroundColor);
        frame.view()->setNodeToDraw(nullptr);
    }

    const Frame& frame;
    const Node* node;
    const PaintBehavior paintBehavior;
    const Color backgroundColor;
};

std::unique_ptr<ImageBuffer> snapshotFrameRect(Frame& frame, const IntRect& imageRect, SnapshotOptions options)
{
    if (!frame.page())
        return nullptr;

    frame.document()->updateLayout();

    FrameView::SelectionInSnapshot shouldIncludeSelection = FrameView::IncludeSelection;
    if (options & SnapshotOptionsExcludeSelectionHighlighting)
        shouldIncludeSelection = FrameView::ExcludeSelection;

    FrameView::CoordinateSpaceForSnapshot coordinateSpace = FrameView::DocumentCoordinates;
    if (options & SnapshotOptionsInViewCoordinates)
        coordinateSpace = FrameView::ViewCoordinates;

    ScopedFramePaintingState state(frame, nullptr);

    PaintBehavior paintBehavior = state.paintBehavior;
    if (options & SnapshotOptionsForceBlackText)
        paintBehavior |= PaintBehaviorForceBlackText;
    if (options & SnapshotOptionsPaintSelectionOnly)
        paintBehavior |= PaintBehaviorSelectionOnly;

    // Other paint behaviors are set by paintContentsForSnapshot.
    frame.view()->setPaintBehavior(paintBehavior);

    std::unique_ptr<ImageBuffer> buffer = ImageBuffer::create(imageRect.size(), frame.page()->deviceScaleFactor(), ColorSpaceDeviceRGB);
    if (!buffer)
        return nullptr;
    buffer->context()->translate(-imageRect.x(), -imageRect.y());

    frame.view()->paintContentsForSnapshot(buffer->context(), imageRect, shouldIncludeSelection, coordinateSpace);
    return buffer;
}

std::unique_ptr<ImageBuffer> snapshotSelection(Frame& frame, SnapshotOptions options)
{
    if (!frame.selection().isRange())
        return nullptr;

    options |= SnapshotOptionsPaintSelectionOnly;
    return snapshotFrameRect(frame, enclosingIntRect(frame.selection().selectionBounds()), options);
}

std::unique_ptr<ImageBuffer> snapshotNode(Frame& frame, Node& node)
{
    if (!node.renderer())
        return nullptr;

    ScopedFramePaintingState state(frame, &node);

    frame.view()->setBaseBackgroundColor(Color::transparent);
    frame.view()->setNodeToDraw(&node);

    LayoutRect topLevelRect;
    return snapshotFrameRect(frame, pixelSnappedIntRect(node.renderer()->paintingRootRect(topLevelRect)));
}

} // namespace WebCore
