/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "DisplayInlineRect.h"
#include "LayoutBox.h"
#include "RenderStyle.h"
#include "TextFlags.h"

namespace WebCore {

class CachedImage;

namespace Display {

struct Run {
    WTF_MAKE_STRUCT_FAST_ALLOCATED;
    struct TextContent {
        WTF_MAKE_STRUCT_FAST_ALLOCATED;
    public:
        TextContent(unsigned position, unsigned length, const String&, bool needsHyphen);

        unsigned start() const { return m_start; }
        unsigned end() const { return start() + length(); }
        unsigned length() const { return m_length; }
        StringView content() const { return StringView(m_contentString).substring(m_start, m_length); }

        bool needsHyphen() const { return m_needsHyphen; }
        void setNeedsHyphen() { m_needsHyphen = true; }

        void expand(unsigned delta) { m_length += delta; }
        void shrink(unsigned delta) { m_length -= delta; }

    private:
        unsigned m_start { 0 };
        unsigned m_length { 0 };
        bool m_needsHyphen { false };
        String m_contentString;
    };

    struct Expansion;
    Run(size_t lineIndex, const Layout::Box&, const InlineRect&, const InlineRect& inkOverflow, Expansion, Optional<TextContent> = WTF::nullopt);

    const InlineRect& rect() const { return m_rect; }
    const InlineRect& inkOverflow() const { return m_inkOverflow; }

    InlineLayoutPoint topLeft() const { return m_rect.topLeft(); }
    InlineLayoutUnit left() const { return m_rect.left(); }
    InlineLayoutUnit right() const { return m_rect.right(); }
    InlineLayoutUnit top() const { return m_rect.top(); }
    InlineLayoutUnit bottom() const { return m_rect.bottom(); }

    InlineLayoutUnit width() const { return m_rect.width(); }
    InlineLayoutUnit height() const { return m_rect.height(); }

    void moveVertically(InlineLayoutUnit);

    Optional<TextContent>& textContent() { return m_textContent; }
    const Optional<TextContent>& textContent() const { return m_textContent; }
    // FIXME: This information should be preserved at Run construction time.
    bool isLineBreak() const { return layoutBox().isLineBreakBox() || (textContent() && textContent()->content() == "\n" && style().preserveNewline()); }

    struct Expansion {
        ExpansionBehavior behavior { DefaultExpansion };
        InlineLayoutUnit horizontalExpansion { 0 };
    };
    Expansion expansion() const { return m_expansion; }

    CachedImage* image() const { return m_cachedImage; }

    const Layout::Box& layoutBox() const { return *m_layoutBox; }
    const RenderStyle& style() const { return m_layoutBox->style(); }

    size_t lineIndex() const { return m_lineIndex; }

private:
    // FIXME: Find out the Display::Run <-> paint style setup.
    const size_t m_lineIndex;
    WeakPtr<const Layout::Box> m_layoutBox;
    CachedImage* m_cachedImage { nullptr };
    InlineRect m_rect;
    InlineRect m_inkOverflow;
    Expansion m_expansion;
    Optional<TextContent> m_textContent;
};

inline Run::Run(size_t lineIndex, const Layout::Box& layoutBox, const InlineRect& rect, const InlineRect& inkOverflow, Expansion expansion, Optional<TextContent> textContent)
    : m_lineIndex(lineIndex)
    , m_layoutBox(makeWeakPtr(layoutBox))
    , m_rect(rect)
    , m_inkOverflow(inkOverflow)
    , m_expansion(expansion)
    , m_textContent(textContent)
{
}

inline Run::TextContent::TextContent(unsigned start, unsigned length, const String& contentString, bool needsHyphen)
    : m_start(start)
    , m_length(length)
    , m_needsHyphen(needsHyphen)
    , m_contentString(contentString)
{
}

inline void Run::moveVertically(InlineLayoutUnit offset)
{
    m_rect.moveVertically(offset);
    m_inkOverflow.moveVertically(offset);
}

}
}
#endif
