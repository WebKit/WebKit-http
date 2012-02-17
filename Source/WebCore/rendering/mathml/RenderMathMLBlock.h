/*
 * Copyright (C) 2010 Alex Milowski (alex@milowski.com). All rights reserved.
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

#ifndef RenderMathMLBlock_h
#define RenderMathMLBlock_h

#if ENABLE(MATHML)

#include "RenderBlock.h"

#define ENABLE_DEBUG_MATH_LAYOUT 0

namespace WebCore {
    
class RenderMathMLOperator;

class RenderMathMLBlock : public RenderBlock {
public:
    RenderMathMLBlock(Node* container);
    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;
    
    virtual bool isRenderMathMLBlock() const { return true; }
    virtual bool isRenderMathMLOperator() const { return false; }
    virtual bool isRenderMathMLRow() const { return false; }
    virtual bool isRenderMathMLMath() const { return false; }
    
    // MathML defines an "embellished operator" as roughly an <mo> that may have subscripts,
    // superscripts, underscripts, overscripts, or a denominator (as in d/dx, where "d" is some
    // differential operator). The padding, precedence, and stretchiness of the base <mo> should
    // apply to the embellished operator as a whole. unembellishedOperator() checks for being an
    // embellished operator, and omits any embellishments.
    // FIXME: We don't yet handle all the cases in the MathML spec. See
    // https://bugs.webkit.org/show_bug.cgi?id=78617.
    virtual RenderMathMLOperator* unembellishedOperator() { return 0; }
    virtual bool hasBase() const { return false; }
    virtual int nonOperatorHeight() const;
    virtual void stretchToHeight(int height);

#if ENABLE(DEBUG_MATH_LAYOUT)
    virtual void paint(PaintInfo&, const LayoutPoint&);
#endif
    
protected:
    static LayoutUnit getBoxModelObjectHeight(const RenderObject* object)
    {
        if (object && object->isBoxModelObject()) {
            const RenderBoxModelObject* box = toRenderBoxModelObject(object);
            return box->offsetHeight();
        }
        
        return 0;
    }
    static LayoutUnit getBoxModelObjectWidth(const RenderObject* object)
    {
        if (object && object->isBoxModelObject()) {
            const RenderBoxModelObject* box = toRenderBoxModelObject(object);
            return box->offsetWidth();
        }
        
        return 0;
    }
    virtual PassRefPtr<RenderStyle> createBlockStyle();

private:
    virtual const char* renderName() const { return isAnonymous() ? "RenderMathMLBlock (anonymous)" : "RenderMathMLBlock"; }
};

inline RenderMathMLBlock* toRenderMathMLBlock(RenderObject* object)
{ 
    ASSERT(!object || object->isRenderMathMLBlock());
    return static_cast<RenderMathMLBlock*>(object);
}

inline const RenderMathMLBlock* toRenderMathMLBlock(const RenderObject* object)
{ 
    ASSERT(!object || object->isRenderMathMLBlock());
    return static_cast<const RenderMathMLBlock*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMathMLBlock(const RenderMathMLBlock*);

}

#endif // ENABLE(MATHML)
#endif // RenderMathMLBlock_h
