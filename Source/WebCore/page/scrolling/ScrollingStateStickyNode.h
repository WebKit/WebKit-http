/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScrollingStateStickyNode_h
#define ScrollingStateStickyNode_h

#if ENABLE(ASYNC_SCROLLING) || USE(COORDINATED_GRAPHICS)

#include "ScrollingConstraints.h"
#include "ScrollingStateNode.h"

#include <wtf/Forward.h>

namespace WebCore {

class StickyPositionViewportConstraints;

class ScrollingStateStickyNode final : public ScrollingStateNode {
public:
    static PassRefPtr<ScrollingStateStickyNode> create(ScrollingStateTree&, ScrollingNodeID);

    virtual PassRefPtr<ScrollingStateNode> clone(ScrollingStateTree&);

    virtual ~ScrollingStateStickyNode();

    enum {
        ViewportConstraints = NumStateNodeBits
    };

    void updateConstraints(const StickyPositionViewportConstraints&);
    const StickyPositionViewportConstraints& viewportConstraints() const { return m_constraints; }

private:
    ScrollingStateStickyNode(ScrollingStateTree&, ScrollingNodeID);
    ScrollingStateStickyNode(const ScrollingStateStickyNode&, ScrollingStateTree&);

    virtual void syncLayerPositionForViewportRect(const LayoutRect& viewportRect) override;

    virtual void dumpProperties(TextStream&, int indent) const override;

    StickyPositionViewportConstraints m_constraints;
};

SCROLLING_STATE_NODE_TYPE_CASTS(ScrollingStateStickyNode, isStickyNode());

} // namespace WebCore

#endif // ENABLE(ASYNC_SCROLLING) || USE(COORDINATED_GRAPHICS)

#endif // ScrollingStateStickyNode_h
