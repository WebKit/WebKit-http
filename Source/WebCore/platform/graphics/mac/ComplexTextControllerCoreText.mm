/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(CORE_TEXT)

#include "ComplexTextController.h"

#include "Font.h"
#include "FontCache.h"
#include "TextRun.h"
#include "WebCoreSystemInterface.h"

#if PLATFORM(WX)
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreText/CoreText.h>
#endif

#if defined(BUILDING_ON_LEOPARD)
// The following symbols are SPI in 10.5.
extern "C" {
void CTRunGetAdvances(CTRunRef run, CFRange range, CGSize buffer[]);
const CGSize* CTRunGetAdvancesPtr(CTRunRef run);
extern const CFStringRef kCTTypesetterOptionForcedEmbeddingLevel;
}
#endif

@interface WebCascadeList : NSArray {
    @private
    const WebCore::Font* _font;
    UChar32 _character;
    NSUInteger _count;
    Vector<RetainPtr<CTFontDescriptorRef>, 16> _fontDescriptors;
}

- (id)initWithFont:(const WebCore::Font*)font character:(UChar32)character;

@end

@implementation WebCascadeList

- (id)initWithFont:(const WebCore::Font*)font character:(UChar32)character
{
    if (!(self = [super init]))
        return nil;

    _font = font;
    _character = character;

    // By the time a WebCascadeList is used, the Font has already been asked to realize all of its
    // FontData, so this loop does not hit the FontCache.
    while (_font->fontDataAt(_count))
        _count++;

    return self;
}

- (NSUInteger)count
{
    return _count;
}

- (id)objectAtIndex:(NSUInteger)index
{
    CTFontDescriptorRef fontDescriptor;
    if (index < _fontDescriptors.size()) {
        if ((fontDescriptor = _fontDescriptors[index].get()))
            return (id)fontDescriptor;
    } else
        _fontDescriptors.grow(index + 1);

    const WebCore::SimpleFontData* fontData = _font->fontDataAt(index)->fontDataForCharacter(_character);
    fontDescriptor = CTFontCopyFontDescriptor(fontData->platformData().ctFont());
    _fontDescriptors[index] = RetainPtr<CTFontDescriptorRef>(AdoptCF, fontDescriptor);
    return (id)fontDescriptor;
}

@end

namespace WebCore {

ComplexTextController::ComplexTextRun::ComplexTextRun(CTRunRef ctRun, const SimpleFontData* fontData, const UChar* characters, unsigned stringLocation, size_t stringLength, CFRange runRange)
    : m_fontData(fontData)
    , m_characters(characters)
    , m_stringLocation(stringLocation)
    , m_stringLength(stringLength)
    , m_indexEnd(runRange.location + runRange.length)
    , m_isMonotonic(true)
{
    m_glyphCount = CTRunGetGlyphCount(ctRun);
    m_coreTextIndices = CTRunGetStringIndicesPtr(ctRun);
    if (!m_coreTextIndices) {
        m_coreTextIndicesVector.grow(m_glyphCount);
        CTRunGetStringIndices(ctRun, CFRangeMake(0, 0), m_coreTextIndicesVector.data());
        m_coreTextIndices = m_coreTextIndicesVector.data();
    }

    m_glyphs = CTRunGetGlyphsPtr(ctRun);
    if (!m_glyphs) {
        m_glyphsVector.grow(m_glyphCount);
        CTRunGetGlyphs(ctRun, CFRangeMake(0, 0), m_glyphsVector.data());
        m_glyphs = m_glyphsVector.data();
    }

    m_advances = CTRunGetAdvancesPtr(ctRun);
    if (!m_advances) {
        m_advancesVector.grow(m_glyphCount);
        CTRunGetAdvances(ctRun, CFRangeMake(0, 0), m_advancesVector.data());
        m_advances = m_advancesVector.data();
    }
}

// Missing glyphs run constructor. Core Text will not generate a run of missing glyphs, instead falling back on
// glyphs from LastResort. We want to use the primary font's missing glyph in order to match the fast text code path.
void ComplexTextController::ComplexTextRun::createTextRunFromFontDataCoreText(bool ltr)
{
    m_coreTextIndicesVector.reserveInitialCapacity(m_stringLength);
    unsigned r = 0;
    while (r < m_stringLength) {
        m_coreTextIndicesVector.uncheckedAppend(r);
        if (U_IS_SURROGATE(m_characters[r])) {
            ASSERT(r + 1 < m_stringLength);
            ASSERT(U_IS_SURROGATE_LEAD(m_characters[r]));
            ASSERT(U_IS_TRAIL(m_characters[r + 1]));
            r += 2;
        } else
            r++;
    }
    m_glyphCount = m_coreTextIndicesVector.size();
    if (!ltr) {
        for (unsigned r = 0, end = m_glyphCount - 1; r < m_glyphCount / 2; ++r, --end)
            std::swap(m_coreTextIndicesVector[r], m_coreTextIndicesVector[end]);
    }
    m_coreTextIndices = m_coreTextIndicesVector.data();

    // Synthesize a run of missing glyphs.
    m_glyphsVector.fill(0, m_glyphCount);
    m_glyphs = m_glyphsVector.data();
    m_advancesVector.fill(CGSizeMake(m_fontData->widthForGlyph(0), 0), m_glyphCount);
    m_advances = m_advancesVector.data();
}

struct ProviderInfo {
    const UChar* cp;
    unsigned length;
    CFDictionaryRef attributes;
};

static const UniChar* provideStringAndAttributes(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void* refCon)
{
    ProviderInfo* info = static_cast<struct ProviderInfo*>(refCon);
    if (stringIndex < 0 || static_cast<unsigned>(stringIndex) >= info->length)
        return 0;

    *charCount = info->length - stringIndex;
    *attributes = info->attributes;
    return info->cp + stringIndex;
}

void ComplexTextController::collectComplexTextRunsForCharactersCoreText(const UChar* cp, unsigned length, unsigned stringLocation, const SimpleFontData* fontData)
{
    ASSERT_ARG(fontData, fontData);

    bool isSystemFallback = false;

    RetainPtr<CFDictionaryRef> stringAttributes;
    if (fontData == systemFallbackFontData()) {
        // FIXME: This code path does not support small caps.
        isSystemFallback = true;

        UChar32 baseCharacter;
        U16_GET(cp, 0, 0, length, baseCharacter);
        fontData = m_font.fontDataAt(0)->fontDataForCharacter(baseCharacter);

        RetainPtr<WebCascadeList> cascadeList(AdoptNS, [[WebCascadeList alloc] initWithFont:&m_font character:baseCharacter]);

        stringAttributes.adoptCF(CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, fontData->getCFStringAttributes(m_font.typesettingFeatures(), fontData->platformData().orientation())));
        static const void* attributeKeys[] = { kCTFontCascadeListAttribute };
        const void* values[] = { cascadeList.get() };
        RetainPtr<CFDictionaryRef> attributes(AdoptCF, CFDictionaryCreate(kCFAllocatorDefault, attributeKeys, values, sizeof(attributeKeys) / sizeof(*attributeKeys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        RetainPtr<CTFontDescriptorRef> fontDescriptor(AdoptCF, CTFontDescriptorCreateWithAttributes(attributes.get()));
        RetainPtr<CTFontRef> fontWithCascadeList(AdoptCF, CTFontCreateCopyWithAttributes(fontData->platformData().ctFont(), m_font.pixelSize(), 0, fontDescriptor.get()));
        CFDictionarySetValue(const_cast<CFMutableDictionaryRef>(stringAttributes.get()), kCTFontAttributeName, fontWithCascadeList.get());
    } else
        stringAttributes = fontData->getCFStringAttributes(m_font.typesettingFeatures(), fontData->platformData().orientation());

    RetainPtr<CTLineRef> line;

    if (!m_mayUseNaturalWritingDirection || m_run.directionalOverride()) {
        static const void* optionKeys[] = { kCTTypesetterOptionForcedEmbeddingLevel };
        const short ltrForcedEmbeddingLevelValue = 0;
        const short rtlForcedEmbeddingLevelValue = 1;
        static const void* ltrOptionValues[] = { CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &ltrForcedEmbeddingLevelValue) };
        static const void* rtlOptionValues[] = { CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &rtlForcedEmbeddingLevelValue) };
        static CFDictionaryRef ltrTypesetterOptions = CFDictionaryCreate(kCFAllocatorDefault, optionKeys, ltrOptionValues, WTF_ARRAY_LENGTH(optionKeys), &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        static CFDictionaryRef rtlTypesetterOptions = CFDictionaryCreate(kCFAllocatorDefault, optionKeys, rtlOptionValues, WTF_ARRAY_LENGTH(optionKeys), &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
        ProviderInfo info = { cp, length, stringAttributes.get() };
        RetainPtr<CTTypesetterRef> typesetter(AdoptCF, wkCreateCTTypesetterWithUniCharProviderAndOptions(&provideStringAndAttributes, 0, &info, m_run.ltr() ? ltrTypesetterOptions : rtlTypesetterOptions));
#else
        RetainPtr<CFStringRef> string(AdoptCF, CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault, cp, length, kCFAllocatorNull));
        RetainPtr<CFAttributedStringRef> attributedString(AdoptCF, CFAttributedStringCreate(kCFAllocatorDefault, string.get(), stringAttributes.get()));
        RetainPtr<CTTypesetterRef> typesetter(AdoptCF, CTTypesetterCreateWithAttributedStringAndOptions(attributedString.get(), m_run.ltr() ? ltrTypesetterOptions : rtlTypesetterOptions));
#endif

        line.adoptCF(CTTypesetterCreateLine(typesetter.get(), CFRangeMake(0, 0)));
    } else {
        ProviderInfo info = { cp, length, stringAttributes.get() };

        line.adoptCF(wkCreateCTLineWithUniCharProvider(&provideStringAndAttributes, 0, &info));
    }

    m_coreTextLines.append(line.get());

    CFArrayRef runArray = CTLineGetGlyphRuns(line.get());

    CFIndex runCount = CFArrayGetCount(runArray);

    for (CFIndex r = 0; r < runCount; r++) {
        CTRunRef ctRun = static_cast<CTRunRef>(CFArrayGetValueAtIndex(runArray, m_run.ltr() ? r : runCount - 1 - r));
        ASSERT(CFGetTypeID(ctRun) == CTRunGetTypeID());
        CFRange runRange = CTRunGetStringRange(ctRun);
        const SimpleFontData* runFontData = fontData;
        if (isSystemFallback) {
            CFDictionaryRef runAttributes = CTRunGetAttributes(ctRun);
            CTFontRef runFont = static_cast<CTFontRef>(CFDictionaryGetValue(runAttributes, kCTFontAttributeName));
            ASSERT(CFGetTypeID(runFont) == CTFontGetTypeID());
            if (!CFEqual(runFont, fontData->platformData().ctFont())) {
                // Rather than using runFont as an NSFont and wrapping it in a FontPlatformData, go through
                // the font cache and ultimately through NSFontManager in order to get an NSFont with the right
                // NSFontRenderingMode.
                RetainPtr<CFStringRef> fontName(AdoptCF, CTFontCopyPostScriptName(runFont));
                if (CFEqual(fontName.get(), CFSTR("LastResort"))) {
                    m_complexTextRuns.append(ComplexTextRun::create(m_font.primaryFont(), cp, stringLocation + runRange.location, runRange.length, m_run.ltr()));
                    continue;
                }
                runFontData = fontCache()->getCachedFontData(m_font.fontDescription(), fontName.get(), false, FontCache::DoNotRetain);
                if (m_fallbackFonts && runFontData != m_font.primaryFont())
                    m_fallbackFonts->add(runFontData);
            }
        }
        if (m_fallbackFonts && runFontData != m_font.primaryFont())
            m_fallbackFonts->add(fontData);

        m_complexTextRuns.append(ComplexTextRun::create(ctRun, runFontData, cp, stringLocation, length, runRange));
    }
}

} // namespace WebCore

#endif // USE(CORE_TEXT)
