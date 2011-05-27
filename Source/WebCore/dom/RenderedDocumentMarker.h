/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#ifndef RenderedDocumentMarker_h
#define RenderedDocumentMarker_h

#include "DocumentMarker.h"

namespace WebCore {

class RenderedDocumentMarker : public DocumentMarker {
public:

    explicit RenderedDocumentMarker(const DocumentMarker& marker)
        : DocumentMarker(marker), m_renderedRect(invalidMarkerRect())
    {
    }

    bool isRendered() const { return invalidMarkerRect() != m_renderedRect; }
    bool contains(const IntPoint& point) const { return isRendered() && m_renderedRect.contains(point); }
    void setRenderedRect(const IntRect& r) { m_renderedRect = r; }
    const IntRect& renderedRect() const { return m_renderedRect; }
    void invalidate(const IntRect&);
    void invalidate() { m_renderedRect = invalidMarkerRect(); }

private:
    static const IntRect& invalidMarkerRect()
    {
        static const IntRect rect = IntRect(-1, -1, -1, -1);
        return rect;
    }

    IntRect m_renderedRect;
};

inline void RenderedDocumentMarker::invalidate(const IntRect& r)
{
    if (m_renderedRect.intersects(r))
        invalidate();
}

inline RenderedDocumentMarker* toRenderedDocumentMarker(DocumentMarker* marker)
{
    return static_cast<RenderedDocumentMarker*>(marker);
}

} // namespace

#endif
