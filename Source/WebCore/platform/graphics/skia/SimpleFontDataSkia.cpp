/*
 * Copyright (c) 2008, 2009, Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "SimpleFontData.h"

#include "FloatRect.h"
#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "Logging.h"
#include "SkFontHost.h"
#include "SkPaint.h"
#include "SkTime.h"
#include "SkTypeface.h"
#include "SkTypes.h"
#include "VDMXParser.h"
#include <unicode/normlzr.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

// This is the largest VDMX table which we'll try to load and parse.
static const size_t maxVDMXTableSize = 1024 * 1024; // 1 MB

void SimpleFontData::platformInit()
{
    if (!m_platformData.size()) {
        m_fontMetrics.reset();
        m_avgCharWidth = 0;
        m_maxCharWidth = 0;
        return;
    }

    SkPaint paint;
    SkPaint::FontMetrics metrics;

    m_platformData.setupPaint(&paint);
    paint.getFontMetrics(&metrics);
    const SkFontID fontID = m_platformData.uniqueID();

    static const uint32_t vdmxTag = SkSetFourByteTag('V', 'D', 'M', 'X');
    int pixelSize = m_platformData.size() + 0.5;
    int vdmxAscent, vdmxDescent;
    bool isVDMXValid = false;

    size_t vdmxSize = SkFontHost::GetTableSize(fontID, vdmxTag);
    if (vdmxSize && vdmxSize < maxVDMXTableSize) {
        uint8_t* vdmxTable = (uint8_t*) fastMalloc(vdmxSize);
        if (vdmxTable
            && SkFontHost::GetTableData(fontID, vdmxTag, 0, vdmxSize, vdmxTable) == vdmxSize
            && parseVDMX(&vdmxAscent, &vdmxDescent, vdmxTable, vdmxSize, pixelSize))
            isVDMXValid = true;
        fastFree(vdmxTable);
    }

    float ascent;
    float descent;

    // Beware those who step here: This code is designed to match Win32 font
    // metrics *exactly* (except the adjustment of ascent/descent on Linux/Android).
    if (isVDMXValid) {
        ascent = vdmxAscent;
        descent = -vdmxDescent;
    } else {
        SkScalar height = -metrics.fAscent + metrics.fDescent + metrics.fLeading;
        ascent = SkScalarRound(-metrics.fAscent);
        descent = SkScalarRound(height) - ascent;
#if OS(LINUX) || OS(ANDROID)
        // When subpixel positioning is enabled, if the descent is rounded down, the descent part
        // of the glyph may be truncated when displayed in a 'overflow: hidden' container.
        // To avoid that, borrow 1 unit from the ascent when possible.
        // FIXME: This can be removed if sub-pixel ascent/descent is supported.
        if (platformData().fontRenderStyle().useSubpixelPositioning && descent < SkScalarToFloat(metrics.fDescent) && ascent >= 1) {
            ++descent;
            --ascent;
        }
#endif
    }

    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);

    float xHeight;
    if (metrics.fXHeight) {
        xHeight = metrics.fXHeight;
        m_fontMetrics.setXHeight(xHeight);
    } else {
        xHeight = ascent * 0.56; // Best guess from Windows font metrics.
        m_fontMetrics.setXHeight(xHeight);
        m_fontMetrics.setHasXHeight(false);
    }


    float lineGap = SkScalarToFloat(metrics.fLeading);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));

    if (platformData().orientation() == Vertical && !isTextOrientationFallback()) {
        static const uint32_t vheaTag = SkSetFourByteTag('v', 'h', 'e', 'a');
        static const uint32_t vorgTag = SkSetFourByteTag('V', 'O', 'R', 'G');
        size_t vheaSize = SkFontHost::GetTableSize(fontID, vheaTag);
        size_t vorgSize = SkFontHost::GetTableSize(fontID, vorgTag);
        if ((vheaSize > 0) || (vorgSize > 0))
            m_hasVerticalGlyphs = true;
    }

    // In WebKit/WebCore/platform/graphics/SimpleFontData.cpp, m_spaceWidth is
    // calculated for us, but we need to calculate m_maxCharWidth and
    // m_avgCharWidth in order for text entry widgets to be sized correctly.

    SkScalar xRange = metrics.fXMax - metrics.fXMin;
    m_maxCharWidth = SkScalarRound(xRange * SkScalarRound(m_platformData.size()));

    if (metrics.fAvgCharWidth)
        m_avgCharWidth = SkScalarRound(metrics.fAvgCharWidth);
    else {
        m_avgCharWidth = xHeight;

        GlyphPage* glyphPageZero = GlyphPageTreeNode::getRootChild(this, 0)->page();

        if (glyphPageZero) {
            static const UChar32 xChar = 'x';
            const Glyph xGlyph = glyphPageZero->glyphDataForCharacter(xChar).glyph;

            if (xGlyph) {
                // In widthForGlyph(), xGlyph will be compared with
                // m_zeroWidthSpaceGlyph, which isn't initialized yet here.
                // Initialize it with zero to make sure widthForGlyph() returns
                // the right width.
                m_zeroWidthSpaceGlyph = 0;
                m_avgCharWidth = widthForGlyph(xGlyph);
            }
        }
    }

    if (int unitsPerEm = paint.getTypeface()->getUnitsPerEm())
        m_fontMetrics.setUnitsPerEm(unitsPerEm);
}

void SimpleFontData::platformCharWidthInit()
{
    // charwidths are set in platformInit.
}

void SimpleFontData::platformDestroy()
{
}

PassRefPtr<SimpleFontData> SimpleFontData::createScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    const float scaledSize = lroundf(fontDescription.computedSize() * scaleFactor);
    return SimpleFontData::create(FontPlatformData(m_platformData, scaledSize), isCustomFont(), false);
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    SkPaint paint;
    static const unsigned maxBufferCount = 64;
    uint16_t glyphs[maxBufferCount];

    m_platformData.setupPaint(&paint);
    paint.setTextEncoding(SkPaint::kUTF16_TextEncoding);

    while (length > 0) {
        int n = SkMin32(length, SK_ARRAY_COUNT(glyphs));

        // textToGlyphs takes a byte count so we double the character count.
        int count = paint.textToGlyphs(characters, n * 2, glyphs);
        for (int i = 0; i < count; i++) {
            if (!glyphs[i])
                return false; // missing glyph
        }

        characters += n;
        length -= n;
    }

    return true;
}

void SimpleFontData::determinePitch()
{
    m_treatAsFixedPitch = platformData().isFixedPitch();
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    if (!m_platformData.size())
        return FloatRect();

    SkASSERT(sizeof(glyph) == 2); // compile-time assert

    SkPaint paint;

    m_platformData.setupPaint(&paint);

    paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    SkRect bounds;
    paint.measureText(&glyph, 2, &bounds);
    if (!paint.isSubpixelText()) {
        SkIRect ir;
        bounds.round(&ir);
        bounds.set(ir);
    }
    return FloatRect(bounds);
}
    
float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (!m_platformData.size())
        return 0;

    SkASSERT(sizeof(glyph) == 2); // compile-time assert

    SkPaint paint;

    m_platformData.setupPaint(&paint);

    paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    SkScalar width = paint.measureText(&glyph, 2);
    if (!paint.isSubpixelText())
        width = SkScalarRound(width);
    return SkScalarToFloat(width);
}

#if USE(HARFBUZZ_NG)
bool SimpleFontData::canRenderCombiningCharacterSequence(const UChar* characters, size_t length) const
{
    if (!m_combiningCharacterSequenceSupport)
        m_combiningCharacterSequenceSupport = adoptPtr(new HashMap<String, bool>);

    WTF::HashMap<String, bool>::AddResult addResult = m_combiningCharacterSequenceSupport->add(String(characters, length), false);
    if (!addResult.isNewEntry)
        return addResult.iterator->value;

    UErrorCode error = U_ZERO_ERROR;
    Vector<UChar, 4> normalizedCharacters(length);
    int32_t normalizedLength = unorm_normalize(characters, length, UNORM_NFC, UNORM_UNICODE_3_2, &normalizedCharacters[0], length, &error);
    // Can't render if we have an error or no composition occurred.
    if (U_FAILURE(error) || (static_cast<size_t>(normalizedLength) == length))
        return false;

    SkPaint paint;
    m_platformData.setupPaint(&paint);
    paint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    if (paint.textToGlyphs(&normalizedCharacters[0], normalizedLength * 2, 0)) {
        addResult.iterator->value = true;
        return true;
    }
    return false;
}
#endif

} // namespace WebCore
