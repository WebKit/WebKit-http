/*
 * Copyright (C) 2003-2020 Apple Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "ColorComponents.h"
#include "ColorTypes.h"
#include "ColorUtilities.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

// Color value with 8-bit components for red, green, blue, and alpha.
// For historical reasons, stored as a 32-bit integer, with alpha in the high bits: ARGB.
class SimpleColor {
public:
    constexpr SimpleColor(uint32_t value = 0) : m_value { value } { }
    constexpr SimpleColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : m_value { static_cast<unsigned>(a << 24 | r << 16 | g << 8 | b) } { }

    constexpr uint32_t valueAsARGB() const { return m_value; }
    constexpr uint32_t value() const { return m_value; }

    constexpr uint8_t redComponent() const { return m_value >> 16; }
    constexpr uint8_t greenComponent() const { return m_value >> 8; }
    constexpr uint8_t blueComponent() const { return m_value; }
    constexpr uint8_t alphaComponent() const { return m_value >> 24; }

    constexpr float alphaComponentAsFloat() const { return convertToComponentFloat(alphaComponent()); }

    constexpr bool isOpaque() const { return alphaComponent() == 0xFF; }
    constexpr bool isVisible() const { return alphaComponent(); }

    constexpr SimpleColor colorWithAlpha(uint8_t alpha) const
    {
        return { (m_value & 0x00FFFFFF) | alpha << 24 };
    }

    constexpr SimpleColor invertedColorWithAlpha(uint8_t alpha) const
    {
        return { static_cast<uint8_t>(0xFF - redComponent()), static_cast<uint8_t>(0xFF - greenComponent()), static_cast<uint8_t>(0xFF - blueComponent()), alpha };
    }

    template<typename T> constexpr SRGBA<T> asSRGBA() const;

    template<std::size_t N>
    constexpr uint8_t get() const
    {
        static_assert(N < 4);
        if constexpr (!N)
            return redComponent();
        else if constexpr (N == 1)
            return greenComponent();
        else if constexpr (N == 2)
            return blueComponent();
        else if constexpr (N == 3)
            return alphaComponent();
    }

private:
    uint32_t m_value { 0 };
};

bool operator==(SimpleColor, SimpleColor);
bool operator!=(SimpleColor, SimpleColor);

constexpr SimpleColor makeSimpleColor(int r, int g, int b);
constexpr SimpleColor makeSimpleColor(int r, int g, int b, int a);
constexpr SimpleColor makeSimpleColor(const SRGBA<uint8_t>&);
SimpleColor makeSimpleColor(const SRGBA<float>&);

SimpleColor premultiplyFlooring(SimpleColor);
SimpleColor premultiplyCeiling(SimpleColor);
SimpleColor unpremultiply(SimpleColor);

inline bool operator==(SimpleColor a, SimpleColor b)
{
    return a.value() == b.value();
}

inline bool operator!=(SimpleColor a, SimpleColor b)
{
    return !(a == b);
}

constexpr SimpleColor makeSimpleColor(int r, int g, int b)
{
    return { static_cast<uint8_t>(std::clamp(r, 0, 0xFF)), static_cast<uint8_t>(std::clamp(g, 0, 0xFF)), static_cast<uint8_t>(std::clamp(b, 0, 0xFF)), 0xFF };
}

constexpr SimpleColor makeSimpleColor(int r, int g, int b, int a)
{
    return { static_cast<uint8_t>(std::clamp(r, 0, 0xFF)), static_cast<uint8_t>(std::clamp(g, 0, 0xFF)), static_cast<uint8_t>(std::clamp(b, 0, 0xFF)), static_cast<uint8_t>(std::clamp(a, 0, 0xFF)) };
}

constexpr SimpleColor makeSimpleColor(const SRGBA<uint8_t>& sRGBA)
{
    auto [r, g, b, a] = sRGBA;
    return { r, g, b, a };
}

inline SimpleColor makeSimpleColor(const SRGBA<float>& sRGBA)
{
    auto [r, g, b, a] = sRGBA;
    return { convertToComponentByte(r), convertToComponentByte(g), convertToComponentByte(b), convertToComponentByte(a) };
}

template<typename T> constexpr SRGBA<T> SimpleColor::asSRGBA() const
{
    if constexpr (std::is_floating_point_v<T>)
        return { convertToComponentFloat(redComponent()), convertToComponentFloat(greenComponent()), convertToComponentFloat(blueComponent()),  convertToComponentFloat(alphaComponent()) };
    else
        return { redComponent(), greenComponent(), blueComponent(), alphaComponent() };
}


} // namespace WebCore

namespace std {

template<> class tuple_size<WebCore::SimpleColor> : public std::integral_constant<std::size_t, 4> {
};

template<std::size_t N> class tuple_element<N, WebCore::SimpleColor> {
public:
    using type = uint8_t;
};

}
