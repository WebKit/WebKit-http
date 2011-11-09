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

#include "config.h"
#include "WebKitCSSFilterValue.h"

#if ENABLE(CSS_FILTERS)

#include "CSSValueList.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

WebKitCSSFilterValue::WebKitCSSFilterValue(FilterOperationType operationType)
    : CSSValueList(WebKitCSSFilterClass, typeUsesSpaceSeparator(operationType))
    , m_type(operationType)
{
}

bool WebKitCSSFilterValue::typeUsesSpaceSeparator(FilterOperationType operationType)
{
#if ENABLE(CSS_SHADERS)
    return operationType != CustomFilterOperation;
#else
    return true;
#endif
}

String WebKitCSSFilterValue::customCssText() const
{
    String result;
    switch (m_type) {
    case ReferenceFilterOperation:
        result = "url(";
        break;
    case GrayscaleFilterOperation:
        result = "grayscale(";
        break;
    case SepiaFilterOperation:
        result = "sepia(";
        break;
    case SaturateFilterOperation:
        result = "saturate(";
        break;
    case HueRotateFilterOperation:
        result = "hue-rotate(";
        break;
    case InvertFilterOperation:
        result = "invert(";
        break;
    case OpacityFilterOperation:
        result = "opacity(";
        break;
    case GammaFilterOperation:
        result = "gamma(";
        break;
    case BlurFilterOperation:
        result = "blur(";
        break;
    case SharpenFilterOperation:
        result = "sharpen(";
        break;
    case DropShadowFilterOperation:
        result = "drop-shadow(";
        break;
#if ENABLE(CSS_SHADERS)
    case CustomFilterOperation:
        result = "custom(";
        break;
#endif
    default:
        break;
    }

    return result + CSSValueList::customCssText() + ")";
}

}

#endif // ENABLE(CSS_FILTERS)
