/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#include <wtf/Forward.h>

namespace WebCore {

class Color;

template<typename> struct DisplayP3;
template<typename> struct LinearSRGBA;
template<typename> struct SRGBA;

// serializationForHTML returns the color serialized according to HTML5 <https://html.spec.whatwg.org/multipage/scripting.html#fill-and-stroke-styles> (10 September 2015)
// serializationForCSS returns the color serialized according to CSS
// serializationForRenderTreeAsText returns the color serialized for DumpRenderTree, #RRGGBB, #RRGGBBAA or the CSS serialization

WEBCORE_EXPORT String serializationForCSS(SRGBA<uint8_t>);
WEBCORE_EXPORT String serializationForHTML(SRGBA<uint8_t>);
WEBCORE_EXPORT String serializationForRenderTreeAsText(SRGBA<uint8_t>);

WEBCORE_EXPORT String serializationForCSS(const SRGBA<float>&);
WEBCORE_EXPORT String serializationForHTML(const SRGBA<float>&);
WEBCORE_EXPORT String serializationForRenderTreeAsText(const SRGBA<float>&);

WEBCORE_EXPORT String serializationForCSS(const LinearSRGBA<float>&);
WEBCORE_EXPORT String serializationForHTML(const LinearSRGBA<float>&);
WEBCORE_EXPORT String serializationForRenderTreeAsText(const LinearSRGBA<float>&);

WEBCORE_EXPORT String serializationForCSS(const DisplayP3<float>&);
WEBCORE_EXPORT String serializationForHTML(const DisplayP3<float>&);
WEBCORE_EXPORT String serializationForRenderTreeAsText(const DisplayP3<float>&);

WEBCORE_EXPORT String serializationForCSS(const Color&);
WEBCORE_EXPORT String serializationForHTML(const Color&);
WEBCORE_EXPORT String serializationForRenderTreeAsText(const Color&);

}
