/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef CSSLineBoxContainValue_h
#define CSSLineBoxContainValue_h

#include "CSSValue.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSPrimitiveValue;

enum LineBoxContainFlags { LineBoxContainNone = 0x0, LineBoxContainBlock = 0x1, LineBoxContainInline = 0x2, LineBoxContainFont = 0x4, LineBoxContainGlyphs = 0x8,
                           LineBoxContainReplaced = 0x10, LineBoxContainInlineBox = 0x20 };
typedef unsigned LineBoxContain;

// Used for text-CSSLineBoxContain and box-CSSLineBoxContain
class CSSLineBoxContainValue : public CSSValue {
public:
    static PassRefPtr<CSSLineBoxContainValue> create(LineBoxContain value)
    {
        return adoptRef(new CSSLineBoxContainValue(value));
    }

    String customCssText() const;

    LineBoxContain value() const { return m_value; }

    void reportDescendantMemoryUsage(MemoryObjectInfo*) const;

private:
    LineBoxContain m_value;

private:
    CSSLineBoxContainValue(LineBoxContain);
};

} // namespace

#endif
