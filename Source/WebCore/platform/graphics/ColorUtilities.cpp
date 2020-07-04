/*
 * Copyright (C) 2017, 2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ColorUtilities.h"

#include "ColorConversion.h"
#include "ColorTypes.h"

namespace WebCore {

float lightness(const SRGBA<float>& color)
{
    auto [r, g, b, a] = color;
    auto [min, max] = std::minmax({ r, g, b });
    return 0.5f * (max + min);
}

float luminance(const SRGBA<float>& color)
{
    // NOTE: This is the equivalent of toXYZA(toLinearSRGBA(color)).y
    // FIMXE: If we can generalize ColorMatrix a bit more, it might be nice to write this as:
    //      return toLinearSRGBA(color) * linearSRGBToXYZMatrix.row(1);
    auto [r, g, b, a] = toLinearSRGBA(color);
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

float contrastRatio(const SRGBA<float>& colorA, const SRGBA<float>& colorB)
{
    // Uses the WCAG 2.0 definition of contrast ratio.
    // https://www.w3.org/TR/WCAG20/#contrast-ratiodef
    float lighterLuminance = luminance(colorA);
    float darkerLuminance = luminance(colorB);

    if (lighterLuminance < darkerLuminance)
        std::swap(lighterLuminance, darkerLuminance);

    return (lighterLuminance + 0.05) / (darkerLuminance + 0.05);
}

SRGBA<float> premultiplied(const SRGBA<float>& color)
{
    auto [r, g, b, a] = color;
    return { r * a, g * a, b * a, a };
}

} // namespace WebCore
