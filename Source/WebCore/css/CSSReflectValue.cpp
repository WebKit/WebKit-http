/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#include "config.h"
#include "CSSReflectValue.h"

#include "CSSPrimitiveValue.h"
#include "WebCoreMemoryInstrumentation.h"
#include <wtf/text/StringBuilder.h>

using namespace std;

namespace WebCore {

String CSSReflectValue::customCssText() const
{
    StringBuilder result;
    switch (m_direction) {
        case ReflectionBelow:
            result.appendLiteral("below ");
            break;
        case ReflectionAbove:
            result.appendLiteral("above ");
            break;
        case ReflectionLeft:
            result.appendLiteral("left ");
            break;
        case ReflectionRight:
            result.appendLiteral("right ");
            break;
        default:
            break;
    }

    result.append(m_offset->cssText());
    result.append(' ');
    if (m_mask)
        result.append(m_mask->cssText());
    return result.toString();
}

void CSSReflectValue::addSubresourceStyleURLs(ListHashSet<KURL>& urls, const StyleSheetContents* styleSheet) const
{
    if (m_mask)
        m_mask->addSubresourceStyleURLs(urls, styleSheet);
}

void CSSReflectValue::reportDescendantMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(m_offset);
    info.addMember(m_mask);
}

} // namespace WebCore
