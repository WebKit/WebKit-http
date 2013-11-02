/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef SimpleLineLayout_h
#define SimpleLineLayout_h

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4200) // Disable "zero-sized array in struct/union" warning
#endif

namespace WebCore {

class RenderBlockFlow;

namespace SimpleLineLayout {

bool canUseFor(const RenderBlockFlow&);

struct Run {
    Run() { }
    Run(unsigned textOffset, float left)
        : textOffset(textOffset)
        , textLength(0)
        , isEndOfLine(false)
        , left(left)
        , right(left)
    { }

    unsigned textOffset;
    unsigned textLength : 31;
    unsigned isEndOfLine : 1;
    float left;
    float right;
};

class Layout {
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef Vector<Run, 10> RunVector;
    static std::unique_ptr<Layout> create(const RunVector&, unsigned lineCount);

    unsigned lineCount() const { return m_lineCount; }

    unsigned runCount() const { return m_runCount; }
    const Run& runAt(unsigned i) const { return m_runs[i]; }

private:
    Layout(const RunVector&, unsigned lineCount);

    unsigned m_lineCount;
    unsigned m_runCount;
    Run m_runs[0];
};

std::unique_ptr<Layout> create(RenderBlockFlow&);

}
}

#if COMPILER(MSVC)
#pragma warning(pop)
#endif

#endif
