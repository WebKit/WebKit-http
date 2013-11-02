/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "MathMLTextElement.h"

#include "MathMLNames.h"
#include "RenderMathMLOperator.h"
#include "RenderMathMLSpace.h"

namespace WebCore {
    
using namespace MathMLNames;

inline MathMLTextElement::MathMLTextElement(const QualifiedName& tagName, Document& document)
    : MathMLElement(tagName, document)
{
    setHasCustomStyleResolveCallbacks();
}

PassRefPtr<MathMLTextElement> MathMLTextElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(new MathMLTextElement(tagName, document));
}

void MathMLTextElement::didAttachRenderers()
{
    MathMLElement::didAttachRenderers();
    if (renderer())
        renderer()->updateFromElement();
}

void MathMLTextElement::childrenChanged(const ChildChange& change)
{
    MathMLElement::childrenChanged(change);
    if (renderer())
        renderer()->updateFromElement();
}

RenderElement* MathMLTextElement::createRenderer(PassRef<RenderStyle> style)
{
    if (hasLocalName(MathMLNames::moTag))
        return new RenderMathMLOperator(*this, std::move(style));
    if (hasLocalName(MathMLNames::mspaceTag))
        return new RenderMathMLSpace(*this, std::move(style));

    return MathMLElement::createRenderer(std::move(style));
}

bool MathMLTextElement::childShouldCreateRenderer(const Node* child) const
{
    return child->isTextNode();
}

}

#endif // ENABLE(MATHML)
