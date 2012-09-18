/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
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

#include "RenderMathMLUnderOver.h"

#include "MathMLNames.h"

namespace WebCore {

using namespace MathMLNames;
    
RenderMathMLUnderOver::RenderMathMLUnderOver(Element* element)
    : RenderMathMLBlock(element)
{
    // Determine what kind of under/over expression we have by element name
    if (element->hasLocalName(MathMLNames::munderTag))
        m_kind = Under;
    else if (element->hasLocalName(MathMLNames::moverTag))
        m_kind = Over;
    else {
        ASSERT(element->hasLocalName(MathMLNames::munderoverTag));
        m_kind = UnderOver;
    }
}

RenderMathMLOperator* RenderMathMLUnderOver::unembellishedOperator()
{
    RenderObject* base = firstChild();
    if (!base || !base->isRenderMathMLBlock())
        return 0;
    return toRenderMathMLBlock(base)->unembellishedOperator();
}

LayoutUnit RenderMathMLUnderOver::firstLineBoxBaseline() const
{
    if (!firstChildBox())
        return -1;
    LayoutUnit baseline = firstChildBox()->firstLineBoxBaseline();
    if (baseline != -1 && m_kind != Under)
        baseline += lastChildBox()->pixelSnappedOffsetHeight(); // Add in the overscript's height.
    return baseline;
}

}

#endif // ENABLE(MATHML)
