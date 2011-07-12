/*
 * Copyright (C) 2003, 2006, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RoundedRect_h
#define RoundedRect_h

#include "LayoutTypes.h"

namespace WebCore {


class RoundedRect {
public:
    class Radii {
    public:
        Radii() {}
        Radii(const LayoutSize& topLeft, const LayoutSize& topRight, const LayoutSize& bottomLeft, const LayoutSize& bottomRight)
            : m_topLeft(topLeft)
            , m_topRight(topRight)
            , m_bottomLeft(bottomLeft)
            , m_bottomRight(bottomRight)
        {
        }

        void setTopLeft(const LayoutSize& size) { m_topLeft = size; }
        void setTopRight(const LayoutSize& size) { m_topRight = size; }
        void setBottomLeft(const LayoutSize& size) { m_bottomLeft = size; }
        void setBottomRight(const LayoutSize& size) { m_bottomRight = size; }
        const LayoutSize& topLeft() const { return m_topLeft; }
        const LayoutSize& topRight() const { return m_topRight; }
        const LayoutSize& bottomLeft() const { return m_bottomLeft; }
        const LayoutSize& bottomRight() const { return m_bottomRight; }

        bool isZero() const;

        void includeLogicalEdges(const Radii& edges, bool isHorizontal, bool includeLogicalLeftEdge, bool includeLogicalRightEdge);
        void excludeLogicalEdges(bool isHorizontal, bool excludeLogicalLeftEdge, bool excludeLogicalRightEdge);

        void scale(float factor);
        void expand(LayoutUnit topWidth, LayoutUnit bottomWidth, LayoutUnit leftWidth, LayoutUnit rightWidth);
        void expand(LayoutUnit size) { expand(size, size, size, size); }
        void shrink(LayoutUnit topWidth, LayoutUnit bottomWidth, LayoutUnit leftWidth, LayoutUnit rightWidth) { expand(-topWidth, -bottomWidth, -leftWidth, -rightWidth); }
        void shrink(LayoutUnit size) { shrink(size, size, size, size); }

    private:
        LayoutSize m_topLeft;
        LayoutSize m_topRight;
        LayoutSize m_bottomLeft;
        LayoutSize m_bottomRight;
    };

    explicit RoundedRect(const LayoutRect&, const Radii& = Radii());
    RoundedRect(LayoutUnit x, LayoutUnit y, LayoutUnit width, LayoutUnit height);
    RoundedRect(const LayoutRect&, const LayoutSize& topLeft, const LayoutSize& topRight, const LayoutSize& bottomLeft, const LayoutSize& bottomRight);

    const LayoutRect& rect() const { return m_rect; }
    const Radii& radii() const { return m_radii; }
    bool isRounded() const { return !m_radii.isZero(); }
    bool isEmpty() const { return m_rect.isEmpty(); }

    void setRect(const LayoutRect& rect) { m_rect = rect; }
    void setRadii(const Radii& radii) { m_radii = radii; }

    void move(const LayoutSize& size) { m_rect.move(size); }
    void inflate(LayoutUnit size) { m_rect.inflate(size);  }
    void inflateWithRadii(LayoutUnit size);
    void expandRadii(LayoutUnit size) { m_radii.expand(size); }
    void shrinkRadii(LayoutUnit size) { m_radii.shrink(size); }

    void includeLogicalEdges(const Radii& edges, bool isHorizontal, bool includeLogicalLeftEdge, bool includeLogicalRightEdge);
    void excludeLogicalEdges(bool isHorizontal, bool excludeLogicalLeftEdge, bool excludeLogicalRightEdge);

    bool isRenderable() const;

private:
    LayoutRect m_rect;
    Radii m_radii;
};

inline bool operator==(const RoundedRect::Radii& a, const RoundedRect::Radii& b)
{
    return a.topLeft() == b.topLeft() && a.topRight() == b.topRight() && a.bottomLeft() == b.bottomLeft() && a.bottomRight() == b.bottomRight();
}

inline bool operator==(const RoundedRect& a, const RoundedRect& b)
{
    return a.rect() == b.rect() && a.radii() == b.radii();
}


} // namespace WebCore

#endif // RoundedRect_h
