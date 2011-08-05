/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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

#ifndef RenderedPosition_h
#define RenderedPosition_h

#include "InlineBox.h"
#include "LayoutTypes.h"
#include "TextAffinity.h"

namespace WebCore {

class Position;
class RenderObject;
class VisiblePosition;

class RenderedPosition {
public:
    RenderedPosition();
    explicit RenderedPosition(const VisiblePosition&);
    explicit RenderedPosition(const Position&, EAffinity);

    bool isNull() const { return !m_renderer; }
    RootInlineBox* rootBox() { return m_inlineBox ? m_inlineBox->root() : 0; }

    LayoutRect absoluteRect() const { return absoluteRect(0); }
    LayoutRect absoluteRect(int& extraWidthToEndOfLine) const { return absoluteRect(&extraWidthToEndOfLine); }

private:
    LayoutRect absoluteRect(int* extraWidthToEndOfLine) const;

    RenderObject* m_renderer;
    InlineBox* m_inlineBox;
    int m_offset;
};

inline RenderedPosition::RenderedPosition()
    : m_renderer(0)
    , m_inlineBox(0)
    , m_offset(0)
{
}

};

#endif // RenderedPosition_h
