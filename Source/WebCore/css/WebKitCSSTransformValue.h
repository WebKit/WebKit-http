/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebKitCSSTransformValue_h
#define WebKitCSSTransformValue_h

#include "CSSValueList.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class WebKitCSSTransformValue : public CSSValueList {
public:
    // NOTE: these have to match the values in the IDL
    enum TransformOperationType {
        UnknownTransformOperation,
        TranslateTransformOperation,
        TranslateXTransformOperation,
        TranslateYTransformOperation,
        RotateTransformOperation,
        ScaleTransformOperation,
        ScaleXTransformOperation,
        ScaleYTransformOperation,
        SkewTransformOperation,
        SkewXTransformOperation,
        SkewYTransformOperation,
        MatrixTransformOperation,
        TranslateZTransformOperation,
        Translate3DTransformOperation,
        RotateXTransformOperation,
        RotateYTransformOperation,
        RotateZTransformOperation,
        Rotate3DTransformOperation,
        ScaleZTransformOperation,
        Scale3DTransformOperation,
        PerspectiveTransformOperation,
        Matrix3DTransformOperation
    };

    static PassRefPtr<WebKitCSSTransformValue> create(TransformOperationType type)
    {
        return adoptRef(new WebKitCSSTransformValue(type));
    }

    String customCssText() const;
#if ENABLE(CSS_VARIABLES)
    String customSerializeResolvingVariables(const HashMap<AtomicString, String>&) const;
#endif

    TransformOperationType operationType() const { return m_type; }
    
    PassRefPtr<WebKitCSSTransformValue> cloneForCSSOM() const;

    void reportDescendantMemoryUsage(MemoryObjectInfo*) const;

private:
    WebKitCSSTransformValue(TransformOperationType);
    WebKitCSSTransformValue(const WebKitCSSTransformValue& cloneFrom);

    TransformOperationType m_type;
};

}

#endif
