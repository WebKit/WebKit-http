/*
 * Copyright (C) 2005, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#include "Font.h"
#include "FontCache.h"
#include "OpenTypeMathData.h"
#include <wtf/MathExtras.h>

#if ENABLE(OPENTYPE_VERTICAL)
#include "OpenTypeVerticalData.h"
#endif

namespace WebCore {

const float smallCapsFontSizeMultiplier = 0.7f;
const float emphasisMarkFontSizeMultiplier = 0.5f;

SimpleFontData::SimpleFontData(const FontPlatformData& platformData, bool isCustomFont, bool isLoading, bool isTextOrientationFallback)
    : m_maxCharWidth(-1)
    , m_avgCharWidth(-1)
    , m_platformData(platformData)
    , m_treatAsFixedPitch(false)
    , m_isCustomFont(isCustomFont)
    , m_isLoading(isLoading)
    , m_isTextOrientationFallback(isTextOrientationFallback)
    , m_isBrokenIdeographFallback(false)
    , m_mathData(nullptr)
#if ENABLE(OPENTYPE_VERTICAL)
    , m_verticalData(0)
#endif
    , m_hasVerticalGlyphs(false)
{
    platformInit();
    platformGlyphInit();
    platformCharWidthInit();
#if ENABLE(OPENTYPE_VERTICAL)
    if (platformData.orientation() == Vertical && !isTextOrientationFallback) {
        m_verticalData = platformData.verticalData();
        m_hasVerticalGlyphs = m_verticalData.get() && m_verticalData->hasVerticalMetrics();
    }
#endif
}

SimpleFontData::SimpleFontData(std::unique_ptr<AdditionalFontData> fontData, float fontSize, bool syntheticBold, bool syntheticItalic)
    : m_platformData(FontPlatformData(fontSize, syntheticBold, syntheticItalic))
    , m_fontData(WTF::move(fontData))
    , m_treatAsFixedPitch(false)
    , m_isCustomFont(true)
    , m_isLoading(false)
    , m_isTextOrientationFallback(false)
    , m_isBrokenIdeographFallback(false)
    , m_mathData(nullptr)
#if ENABLE(OPENTYPE_VERTICAL)
    , m_verticalData(0)
#endif
    , m_hasVerticalGlyphs(false)
#if PLATFORM(IOS)
    , m_shouldNotBeUsedForArabic(false)
#endif
{
    m_fontData->initializeFontData(this, fontSize);
}

// Estimates of avgCharWidth and maxCharWidth for platforms that don't support accessing these values from the font.
void SimpleFontData::initCharWidths()
{
    GlyphPage* glyphPageZero = GlyphPageTreeNode::getRootChild(this, 0)->page();

    // Treat the width of a '0' as the avgCharWidth.
    if (m_avgCharWidth <= 0.f && glyphPageZero) {
        static const UChar32 digitZeroChar = '0';
        Glyph digitZeroGlyph = glyphPageZero->glyphDataForCharacter(digitZeroChar).glyph;
        if (digitZeroGlyph)
            m_avgCharWidth = widthForGlyph(digitZeroGlyph);
    }

    // If we can't retrieve the width of a '0', fall back to the x height.
    if (m_avgCharWidth <= 0.f)
        m_avgCharWidth = m_fontMetrics.xHeight();

    if (m_maxCharWidth <= 0.f)
        m_maxCharWidth = std::max(m_avgCharWidth, m_fontMetrics.floatAscent());
}

void SimpleFontData::platformGlyphInit()
{
    GlyphPage* glyphPageZero = GlyphPageTreeNode::getRootChild(this, 0)->page();
    if (!glyphPageZero) {
        LOG_ERROR("Failed to get glyph page zero.");
        m_spaceGlyph = 0;
        m_spaceWidth = 0;
        m_zeroGlyph = 0;
        m_adjustedSpaceWidth = 0;
        determinePitch();
        m_zeroWidthSpaceGlyph = 0;
        m_missingGlyphData.fontData = this;
        m_missingGlyphData.glyph = 0;
        return;
    }

    m_zeroWidthSpaceGlyph = glyphPageZero->glyphDataForCharacter(0).glyph;

    // Nasty hack to determine if we should round or ceil space widths.
    // If the font is monospace or fake monospace we ceil to ensure that 
    // every character and the space are the same width.  Otherwise we round.
    m_spaceGlyph = glyphPageZero->glyphDataForCharacter(' ').glyph;
    float width = widthForGlyph(m_spaceGlyph);
    m_spaceWidth = width;
    m_zeroGlyph = glyphPageZero->glyphDataForCharacter('0').glyph;
    m_fontMetrics.setZeroWidth(widthForGlyph(m_zeroGlyph));
    determinePitch();
    m_adjustedSpaceWidth = m_treatAsFixedPitch ? ceilf(width) : roundf(width);

    // Force the glyph for ZERO WIDTH SPACE to have zero width, unless it is shared with SPACE.
    // Helvetica is an example of a non-zero width ZERO WIDTH SPACE glyph.
    // See <http://bugs.webkit.org/show_bug.cgi?id=13178>
    // Ask for the glyph for 0 to avoid paging in ZERO WIDTH SPACE. Control characters, including 0,
    // are mapped to the ZERO WIDTH SPACE glyph.
    if (m_zeroWidthSpaceGlyph == m_spaceGlyph) {
        m_zeroWidthSpaceGlyph = 0;
        LOG_ERROR("Font maps SPACE and ZERO WIDTH SPACE to the same glyph. Glyph width will not be overridden.");
    }

    m_missingGlyphData.fontData = this;
    m_missingGlyphData.glyph = 0;
}

SimpleFontData::~SimpleFontData()
{
    if (!m_fontData)
        platformDestroy();

    if (isCustomFont())
        GlyphPageTreeNode::pruneTreeCustomFontData(this);
    else
        GlyphPageTreeNode::pruneTreeFontData(this);
}

const SimpleFontData* SimpleFontData::fontDataForCharacter(UChar32) const
{
    return this;
}

Glyph SimpleFontData::glyphForCharacter(UChar32 character) const
{
    GlyphPageTreeNode* node = GlyphPageTreeNode::getRootChild(this, character / GlyphPage::size);
    return node->page() ? node->page()->glyphAt(character % GlyphPage::size) : 0;
}

bool SimpleFontData::isSegmented() const
{
    return false;
}

PassRefPtr<SimpleFontData> SimpleFontData::verticalRightOrientationFontData() const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->verticalRightOrientation) {
        FontPlatformData verticalRightPlatformData(m_platformData);
        verticalRightPlatformData.setOrientation(Horizontal);
        m_derivedFontData->verticalRightOrientation = create(verticalRightPlatformData, isCustomFont(), false, true);
    }
    return m_derivedFontData->verticalRightOrientation;
}

PassRefPtr<SimpleFontData> SimpleFontData::uprightOrientationFontData() const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->uprightOrientation)
        m_derivedFontData->uprightOrientation = create(m_platformData, isCustomFont(), false, true);
    return m_derivedFontData->uprightOrientation;
}

PassRefPtr<SimpleFontData> SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = createScaledFontData(fontDescription, smallCapsFontSizeMultiplier);

    return m_derivedFontData->smallCaps;
}

PassRefPtr<SimpleFontData> SimpleFontData::emphasisMarkFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = createScaledFontData(fontDescription, emphasisMarkFontSizeMultiplier);

    return m_derivedFontData->emphasisMark;
}

PassRefPtr<SimpleFontData> SimpleFontData::brokenIdeographFontData() const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->brokenIdeograph) {
        m_derivedFontData->brokenIdeograph = create(m_platformData, isCustomFont(), false);
        m_derivedFontData->brokenIdeograph->m_isBrokenIdeographFallback = true;
    }
    return m_derivedFontData->brokenIdeograph;
}

PassRefPtr<SimpleFontData> SimpleFontData::nonSyntheticItalicFontData() const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->nonSyntheticItalic) {
        FontPlatformData nonSyntheticItalicFontPlatformData(m_platformData);
#if PLATFORM(COCOA)
        nonSyntheticItalicFontPlatformData.m_syntheticOblique = false;
#endif
        m_derivedFontData->nonSyntheticItalic = create(nonSyntheticItalicFontPlatformData, isCustomFont(), false, true);
    }
    return m_derivedFontData->nonSyntheticItalic;
}

#ifndef NDEBUG
String SimpleFontData::description() const
{
    if (isSVGFont())
        return "[SVG font]";
    if (isCustomFont())
        return "[custom font]";

    return platformData().description();
}
#endif

const OpenTypeMathData* SimpleFontData::mathData() const
{
    if (m_isLoading)
        return nullptr;
    if (!m_mathData) {
        m_mathData = OpenTypeMathData::create(m_platformData);
        if (!m_mathData->hasMathData())
            m_mathData.clear();
    }
    return m_mathData.get();
}

PassOwnPtr<SimpleFontData::DerivedFontData> SimpleFontData::DerivedFontData::create(bool forCustomFont)
{
    return adoptPtr(new DerivedFontData(forCustomFont));
}

SimpleFontData::DerivedFontData::~DerivedFontData()
{
    if (!forCustomFont)
        return;

    if (smallCaps)
        GlyphPageTreeNode::pruneTreeCustomFontData(smallCaps.get());
    if (emphasisMark)
        GlyphPageTreeNode::pruneTreeCustomFontData(emphasisMark.get());
    if (brokenIdeograph)
        GlyphPageTreeNode::pruneTreeCustomFontData(brokenIdeograph.get());
    if (verticalRightOrientation)
        GlyphPageTreeNode::pruneTreeCustomFontData(verticalRightOrientation.get());
    if (uprightOrientation)
        GlyphPageTreeNode::pruneTreeCustomFontData(uprightOrientation.get());
#if PLATFORM(COCOA)
    if (compositeFontReferences) {
        CFDictionaryRef dictionary = CFDictionaryRef(compositeFontReferences.get());
        CFIndex count = CFDictionaryGetCount(dictionary);
        if (count > 0) {
            Vector<SimpleFontData*, 2> stash(count);
            SimpleFontData** fonts = stash.data();
            CFDictionaryGetKeysAndValues(dictionary, 0, (const void **)fonts);
            while (count-- > 0 && *fonts) {
                RefPtr<SimpleFontData> afont = adoptRef(*fonts++);
                GlyphPageTreeNode::pruneTreeCustomFontData(afont.get());
            }
        }
    }
#endif
}

PassRefPtr<SimpleFontData> SimpleFontData::createScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    // FIXME: Support scaled fonts that used AdditionalFontData.
    if (m_fontData)
        return 0;

    return platformCreateScaledFontData(fontDescription, scaleFactor);
}

} // namespace WebCore
