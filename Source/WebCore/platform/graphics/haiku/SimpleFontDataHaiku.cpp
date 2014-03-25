/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com> All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "SimpleFontData.h"

#include "FloatRect.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "NotImplemented.h"
#include "TextEncoding.h"

#include <wtf/text/CString.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/unorm.h>
#include <Rect.h>
#include <UnicodeChar.h>


namespace WebCore {

static const float smallCapsFraction = 0.7;
static const float emphasisMarkFraction = 0.5;

void SimpleFontData::platformInit()
{
    const BFont* font = m_platformData.font();
    if (!font)
        return;

    font_height height;
    font->GetHeight(&height);
    m_fontMetrics.setAscent(height.ascent);
    m_fontMetrics.setDescent(height.descent);
    m_fontMetrics.setLineSpacing(height.ascent + height.descent);
    m_fontMetrics.setLineGap(height.leading);

    BRect rect;
    font->GetBoundingBoxesAsGlyphs("x", 1, B_SCREEN_METRIC, &rect);

    m_fontMetrics.setXHeight(rect.Height() / 1.25);
        // FIXME we shouldn't need to divide here, but it passes this test:
        // css2.1/20110323/c541-word-sp-000.htm
}

void SimpleFontData::platformCharWidthInit()
{
    m_avgCharWidth = 0.f;
    m_maxCharWidth = 0.f;
    initCharWidths();
}

void SimpleFontData::platformDestroy()
{
}

PassRefPtr<SimpleFontData> SimpleFontData::platformCreateScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    const float scaledSize = lroundf(fontDescription.computedSize() * scaleFactor);
    return SimpleFontData::create(FontPlatformData(m_platformData, scaledSize), isCustomFont(), false);
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    const BFont* font = m_platformData.font();
    if (!font)
        return false;

    bool result[length];
    icu::UnicodeString source(characters, length);
    std::string utf8;
    source.toUTF8String(utf8);

    font->GetHasGlyphs(utf8.c_str(), length, result);
    for (int i = 0; i < length; i++) {
        if (!result[i])
            return false;
    }
    return true;
}

void SimpleFontData::determinePitch()
{
    m_treatAsFixedPitch = m_platformData.font() && m_platformData.font()->IsFixed();
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    const BFont* font = m_platformData.font();
    if (!font)
        return FloatRect();

    BRect rect;
    char string[5] = { 0 };
    char* ptr = string;
    BUnicodeChar::ToUTF8((uint32)glyph, &ptr);
    font->GetBoundingBoxesAsGlyphs(string, 1, B_SCREEN_METRIC, &rect);
    return rect;
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (!glyph || !platformData().size())
        return 0;

    float escapements[1];
    char string[5] = { 0 };
    char* ptr = string;
    BUnicodeChar::ToUTF8((uint32)glyph, &ptr);
    m_platformData.font()->GetEscapements(string, 1, escapements);
    return escapements[0] * m_platformData.font()->Size();
}

} // namespace WebCore

