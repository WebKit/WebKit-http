/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef SimpleFontData_h
#define SimpleFontData_h

#include "FontBaseline.h"
#include "FontData.h"
#include "FontMetrics.h"
#include "FontPlatformData.h"
#include "FloatRect.h"
#include "GlyphBuffer.h"
#include "GlyphMetricsMap.h"
#include "GlyphPageTreeNode.h"
#include "OpenTypeMathData.h"
#if ENABLE(OPENTYPE_VERTICAL)
#include "OpenTypeVerticalData.h"
#endif
#include "TypesettingFeatures.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringHash.h>

#if PLATFORM(COCOA)
#include "WebCoreSystemInterface.h"
#include <wtf/RetainPtr.h>
#endif

#if (PLATFORM(WIN) && !OS(WINCE))
#include <usp10.h>
#endif

#if USE(CAIRO)
#include <cairo.h>
#endif

#if USE(CG)
#if defined(__has_include) && __has_include(<CoreGraphics/CGFontRendering.h>)
#include <CoreGraphics/CGFontRendering.h>
#else
enum {
    kCGFontRenderingStyleAntialiasing = (1 << 0),
    kCGFontRenderingStyleSmoothing = (1 << 1),
    kCGFontRenderingStyleSubpixelPositioning = (1 << 2),
    kCGFontRenderingStyleSubpixelQuantization = (1 << 3),
    kCGFontRenderingStylePlatformNative = (1 << 9),
    kCGFontRenderingStyleMask = 0x20F
};
#endif
typedef uint32_t CGFontRenderingStyle;
#endif

namespace WebCore {

class FontDescription;
class SharedBuffer;
struct WidthIterator;

enum FontDataVariant { AutoVariant, NormalVariant, SmallCapsVariant, EmphasisMarkVariant, BrokenIdeographVariant };
enum Pitch { UnknownPitch, FixedPitch, VariablePitch };

class SimpleFontData final : public FontData {
public:
    class AdditionalFontData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        virtual ~AdditionalFontData() { }

        virtual void initializeFontData(SimpleFontData*, float fontSize) = 0;
        virtual float widthForSVGGlyph(Glyph, float fontSize) const = 0;
        virtual bool fillSVGGlyphPage(GlyphPage*, unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const SimpleFontData*) const = 0;
        virtual bool applySVGGlyphSelection(WidthIterator&, GlyphData&, bool mirror, int currentCharacter, unsigned& advanceLength) const = 0;
    };

    // Used to create platform fonts.
    static PassRefPtr<SimpleFontData> create(const FontPlatformData& platformData, bool isCustomFont = false, bool isLoading = false, bool isTextOrientationFallback = false)
    {
        return adoptRef(new SimpleFontData(platformData, isCustomFont, isLoading, isTextOrientationFallback));
    }

    // Used to create SVG Fonts.
    static PassRefPtr<SimpleFontData> create(std::unique_ptr<AdditionalFontData> fontData, float fontSize, bool syntheticBold, bool syntheticItalic)
    {
        return adoptRef(new SimpleFontData(WTF::move(fontData), fontSize, syntheticBold, syntheticItalic));
    }

    virtual ~SimpleFontData();

    static const SimpleFontData* systemFallback() { return reinterpret_cast<const SimpleFontData*>(-1); }

    const FontPlatformData& platformData() const { return m_platformData; }
    const OpenTypeMathData* mathData() const;
#if ENABLE(OPENTYPE_VERTICAL)
    const OpenTypeVerticalData* verticalData() const { return m_verticalData.get(); }
#endif

    PassRefPtr<SimpleFontData> smallCapsFontData(const FontDescription&) const;
    PassRefPtr<SimpleFontData> emphasisMarkFontData(const FontDescription&) const;
    PassRefPtr<SimpleFontData> brokenIdeographFontData() const;
    PassRefPtr<SimpleFontData> nonSyntheticItalicFontData() const;

    PassRefPtr<SimpleFontData> variantFontData(const FontDescription& description, FontDataVariant variant) const
    {
        switch (variant) {
        case SmallCapsVariant:
            return smallCapsFontData(description);
        case EmphasisMarkVariant:
            return emphasisMarkFontData(description);
        case BrokenIdeographVariant:
            return brokenIdeographFontData();
        case AutoVariant:
        case NormalVariant:
            break;
        }
        ASSERT_NOT_REACHED();
        return const_cast<SimpleFontData*>(this);
    }

    PassRefPtr<SimpleFontData> verticalRightOrientationFontData() const;
    PassRefPtr<SimpleFontData> uprightOrientationFontData() const;

    bool hasVerticalGlyphs() const { return m_hasVerticalGlyphs; }
    bool isTextOrientationFallback() const { return m_isTextOrientationFallback; }

    FontMetrics& fontMetrics() { return m_fontMetrics; }
    const FontMetrics& fontMetrics() const { return m_fontMetrics; }
    float sizePerUnit() const { return platformData().size() / (fontMetrics().unitsPerEm() ? fontMetrics().unitsPerEm() : 1); }

    float maxCharWidth() const { return m_maxCharWidth; }
    void setMaxCharWidth(float maxCharWidth) { m_maxCharWidth = maxCharWidth; }

    float avgCharWidth() const { return m_avgCharWidth; }
    void setAvgCharWidth(float avgCharWidth) { m_avgCharWidth = avgCharWidth; }

    FloatRect boundsForGlyph(Glyph) const;
    float widthForGlyph(Glyph glyph) const;
    FloatRect platformBoundsForGlyph(Glyph) const;
    float platformWidthForGlyph(Glyph) const;

    float spaceWidth() const { return m_spaceWidth; }
    float adjustedSpaceWidth() const { return m_adjustedSpaceWidth; }
    void setSpaceWidth(float spaceWidth) { m_spaceWidth = spaceWidth; }

#if USE(CG) || USE(CAIRO)
    float syntheticBoldOffset() const { return m_syntheticBoldOffset; }
#endif

    Glyph spaceGlyph() const { return m_spaceGlyph; }
    void setSpaceGlyph(Glyph spaceGlyph) { m_spaceGlyph = spaceGlyph; }
    Glyph zeroWidthSpaceGlyph() const { return m_zeroWidthSpaceGlyph; }
    void setZeroWidthSpaceGlyph(Glyph spaceGlyph) { m_zeroWidthSpaceGlyph = spaceGlyph; }
    bool isZeroWidthSpaceGlyph(Glyph glyph) const { return glyph == m_zeroWidthSpaceGlyph && glyph; }
    Glyph zeroGlyph() const { return m_zeroGlyph; }
    void setZeroGlyph(Glyph zeroGlyph) { m_zeroGlyph = zeroGlyph; }

    virtual const SimpleFontData* fontDataForCharacter(UChar32) const override;
    virtual bool containsCharacters(const UChar*, int length) const override;

    Glyph glyphForCharacter(UChar32) const;

    void determinePitch();
    Pitch pitch() const { return m_treatAsFixedPitch ? FixedPitch : VariablePitch; }

    AdditionalFontData* fontData() const { return m_fontData.get(); }
    bool isSVGFont() const { return m_fontData != nullptr; }

    virtual bool isCustomFont() const override { return m_isCustomFont; }
    virtual bool isLoading() const override { return m_isLoading; }
    virtual bool isSegmented() const override;

    const GlyphData& missingGlyphData() const { return m_missingGlyphData; }
    void setMissingGlyphData(const GlyphData& glyphData) { m_missingGlyphData = glyphData; }

#ifndef NDEBUG
    virtual String description() const override;
#endif

#if USE(APPKIT)
    const SimpleFontData* getCompositeFontReferenceFontData(NSFont *key) const;
    NSFont* getNSFont() const { return m_platformData.font(); }
#endif

#if PLATFORM(IOS)
    CTFontRef getCTFont() const { return m_platformData.font(); }
    bool shouldNotBeUsedForArabic() const { return m_shouldNotBeUsedForArabic; };
#endif
#if PLATFORM(COCOA)
    CFDictionaryRef getCFStringAttributes(TypesettingFeatures, FontOrientation) const;
#endif

#if PLATFORM(COCOA) || USE(HARFBUZZ)
    bool canRenderCombiningCharacterSequence(const UChar*, size_t) const;
#endif

    bool applyTransforms(GlyphBufferGlyph* glyphs, GlyphBufferAdvance* advances, size_t glyphCount, TypesettingFeatures typesettingFeatures) const
    {
        if (isSVGFont())
            return false;
#if PLATFORM(IOS) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED > 1080)
        wkCTFontTransformOptions options = (typesettingFeatures & Kerning ? wkCTFontTransformApplyPositioning : 0) | (typesettingFeatures & Ligatures ? wkCTFontTransformApplyShaping : 0);
        return wkCTFontTransformGlyphs(m_platformData.ctFont(), glyphs, reinterpret_cast<CGSize*>(advances), glyphCount, options);
#else
        UNUSED_PARAM(glyphs);
        UNUSED_PARAM(advances);
        UNUSED_PARAM(glyphCount);
        UNUSED_PARAM(typesettingFeatures);
        return false;
#endif
    }

#if PLATFORM(WIN)
    bool isSystemFont() const { return m_isSystemFont; }
#if !OS(WINCE) // disable unused members to save space
    SCRIPT_FONTPROPERTIES* scriptFontProperties() const;
    SCRIPT_CACHE* scriptCache() const { return &m_scriptCache; }
#endif
    static void setShouldApplyMacAscentHack(bool);
    static bool shouldApplyMacAscentHack();
    static float ascentConsideringMacAscentHack(const WCHAR*, float ascent, float descent);
#endif

private:
    SimpleFontData(const FontPlatformData&, bool isCustomFont = false, bool isLoading = false, bool isTextOrientationFallback = false);

    SimpleFontData(std::unique_ptr<AdditionalFontData>, float fontSize, bool syntheticBold, bool syntheticItalic);

    void platformInit();
    void platformGlyphInit();
    void platformCharWidthInit();
    void platformDestroy();

    void initCharWidths();

    PassRefPtr<SimpleFontData> createScaledFontData(const FontDescription&, float scaleFactor) const;
    PassRefPtr<SimpleFontData> platformCreateScaledFontData(const FontDescription&, float scaleFactor) const;

#if (PLATFORM(WIN) && !OS(WINCE))
    void initGDIFont();
    void platformCommonDestroy();
    FloatRect boundsForGDIGlyph(Glyph glyph) const;
    float widthForGDIGlyph(Glyph glyph) const;
#endif

#if USE(CG)
    bool canUseFastGlyphAdvanceGetter(Glyph glyph, CGSize& advance, bool& populatedAdvance) const;
    CGFontRenderingStyle renderingStyle() const;
    bool advanceForColorBitmapFont(Glyph, CGSize& result) const; // Returns true if the font is a color bitmap font
#endif

    FontMetrics m_fontMetrics;
    float m_maxCharWidth;
    float m_avgCharWidth;

    FontPlatformData m_platformData;
    std::unique_ptr<AdditionalFontData> m_fontData;

    mutable OwnPtr<GlyphMetricsMap<FloatRect>> m_glyphToBoundsMap;
    mutable GlyphMetricsMap<float> m_glyphToWidthMap;

    bool m_treatAsFixedPitch;
    bool m_isCustomFont;  // Whether or not we are custom font loaded via @font-face
    bool m_isLoading; // Whether or not this custom font is still in the act of loading.

    bool m_isTextOrientationFallback;
    bool m_isBrokenIdeographFallback;
    mutable RefPtr<OpenTypeMathData> m_mathData;
#if ENABLE(OPENTYPE_VERTICAL)
    RefPtr<OpenTypeVerticalData> m_verticalData;
#endif
    bool m_hasVerticalGlyphs;

    Glyph m_spaceGlyph;
    float m_spaceWidth;
    Glyph m_zeroGlyph;
    float m_adjustedSpaceWidth;

    Glyph m_zeroWidthSpaceGlyph;

    GlyphData m_missingGlyphData;

    struct DerivedFontData {
        static PassOwnPtr<DerivedFontData> create(bool forCustomFont);
        ~DerivedFontData();

        bool forCustomFont;
        RefPtr<SimpleFontData> smallCaps;
        RefPtr<SimpleFontData> emphasisMark;
        RefPtr<SimpleFontData> brokenIdeograph;
        RefPtr<SimpleFontData> verticalRightOrientation;
        RefPtr<SimpleFontData> uprightOrientation;
        RefPtr<SimpleFontData> nonSyntheticItalic;
#if PLATFORM(COCOA)
        mutable RetainPtr<CFMutableDictionaryRef> compositeFontReferences;
#endif

    private:
        DerivedFontData(bool custom)
            : forCustomFont(custom)
        {
        }
    };

    mutable OwnPtr<DerivedFontData> m_derivedFontData;

#if USE(CG) || USE(CAIRO)
    float m_syntheticBoldOffset;
#endif

#if PLATFORM(COCOA)
    mutable HashMap<unsigned, RetainPtr<CFDictionaryRef>> m_CFStringAttributes;
#endif

#if PLATFORM(COCOA) || USE(HARFBUZZ)
    mutable OwnPtr<HashMap<String, bool>> m_combiningCharacterSequenceSupport;
#endif

#if PLATFORM(WIN)
    bool m_isSystemFont;
#if !OS(WINCE) // disable unused members to save space
    mutable SCRIPT_CACHE m_scriptCache;
    mutable SCRIPT_FONTPROPERTIES* m_scriptFontProperties;
#endif
#endif
#if PLATFORM(IOS)
    bool m_shouldNotBeUsedForArabic;
#endif
};

ALWAYS_INLINE FloatRect SimpleFontData::boundsForGlyph(Glyph glyph) const
{
    if (isZeroWidthSpaceGlyph(glyph))
        return FloatRect();

    FloatRect bounds;
    if (m_glyphToBoundsMap) {
        bounds = m_glyphToBoundsMap->metricsForGlyph(glyph);
        if (bounds.width() != cGlyphSizeUnknown)
            return bounds;
    }

    bounds = platformBoundsForGlyph(glyph);
    if (!m_glyphToBoundsMap)
        m_glyphToBoundsMap = adoptPtr(new GlyphMetricsMap<FloatRect>);
    m_glyphToBoundsMap->setMetricsForGlyph(glyph, bounds);
    return bounds;
}

ALWAYS_INLINE float SimpleFontData::widthForGlyph(Glyph glyph) const
{
    if (isZeroWidthSpaceGlyph(glyph))
        return 0;

    float width = m_glyphToWidthMap.metricsForGlyph(glyph);
    if (width != cGlyphSizeUnknown)
        return width;

    if (m_fontData)
        width = m_fontData->widthForSVGGlyph(glyph, m_platformData.size());
#if ENABLE(OPENTYPE_VERTICAL)
    else if (m_verticalData)
#if USE(CG) || USE(CAIRO)
        width = m_verticalData->advanceHeight(this, glyph) + m_syntheticBoldOffset;
#else
        width = m_verticalData->advanceHeight(this, glyph);
#endif
#endif
    else
        width = platformWidthForGlyph(glyph);

    m_glyphToWidthMap.setMetricsForGlyph(glyph, width);
    return width;
}

} // namespace WebCore
#endif // SimpleFontData_h
