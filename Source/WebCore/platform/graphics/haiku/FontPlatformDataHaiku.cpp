/*
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "FontPlatformData.h"

#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>

namespace WebCore {
font_family FontPlatformData::m_FallbackSansSerifFontFamily= "DejaVu Sans";
font_family FontPlatformData::m_FallbackSerifFontFamily = "DejaVu Serif";
font_family FontPlatformData::m_FallbackFixedFontFamily = "DejaVu Mono";
font_family FontPlatformData::m_FallbackStandardFontFamily = "DejaVu Serif";

static inline bool isEmtpyValue(const float size, const bool bold, const bool oblique)
{
     // this is the empty value by definition of the trait FontDataCacheKeyTraits
    return !bold && !oblique && size == 0;
}

void
FontPlatformData::findMatchingFontFamily(const AtomicString& familyName, font_family* fontFamily)
{
    if (BFont().SetFamilyAndStyle(familyName.string().utf8().data(), 0) == B_OK)
        strncpy(*fontFamily, familyName.string().utf8().data(), B_FONT_FAMILY_LENGTH + 1);
    else {
        // If no font family is found for the given name, we use a generic font.
        if (familyName.contains("Sans", false) != B_ERROR)
            strncpy(*fontFamily, m_FallbackSansSerifFontFamily, B_FONT_FAMILY_LENGTH + 1);
        else if (familyName.contains("Serif", false) != B_ERROR)
            strncpy(*fontFamily, m_FallbackSerifFontFamily, B_FONT_FAMILY_LENGTH + 1);
        else if (familyName.contains("Mono", false) != B_ERROR)
            strncpy(*fontFamily, m_FallbackFixedFontFamily, B_FONT_FAMILY_LENGTH + 1);
        else {
            // This is the fallback font.
            strncpy(*fontFamily, m_FallbackStandardFontFamily, B_FONT_FAMILY_LENGTH + 1);
        }
    }
}

static void findMatchingFontStyle(const font_family& fontFamily, bool bold, bool oblique, font_style* fontStyle)
{
    int32 numStyles = count_font_styles(const_cast<char*>(fontFamily));

    for (int i = 0; i < numStyles; i++) {
        if (get_font_style(const_cast<char*>(fontFamily), i, fontStyle) == B_OK) {
            String styleName(*fontStyle);
            if ((oblique && bold) && (styleName == "BoldItalic" || styleName == "Bold Oblique"))
                return;
            if ((oblique && !bold) && (styleName == "Italic" || styleName == "Oblique"))
                return;
            if ((!oblique && bold) && styleName == "Bold")
                return;
            if ((!oblique && !bold) && (styleName == "Roma" || styleName == "Book"
                || styleName == "Condensed" || styleName == "Regular" || styleName == "Medium")) {
                return;
            }
        }
    }

    // Didn't find matching style
    memset(fontStyle, 0, sizeof(font_style));
}

// #pragma mark -

class FontPlatformData::FontPlatformDataPrivate {
public:
    FontPlatformDataPrivate();
    FontPlatformDataPrivate(const float size, const bool bold, const bool oblique);
    FontPlatformDataPrivate(const BFont& font);

    void update();

    void deref();
    void addRef();

    BFont font;
    float size;
    bool bold : 1;
    bool oblique : 1;

private:
    ~FontPlatformDataPrivate();

private:
    unsigned refCount;
};

FontPlatformData::FontPlatformDataPrivate::FontPlatformDataPrivate()
    : refCount(1)
{
	update();
}

FontPlatformData::FontPlatformDataPrivate::FontPlatformDataPrivate(const float size, const bool bold, const bool oblique)
    : size(size)
    , bold(bold)
    , oblique(oblique)
    , refCount(1)
{
}

FontPlatformData::FontPlatformDataPrivate::FontPlatformDataPrivate(const BFont& font)
    : font(font)
    , refCount(1)
{
	update();
}

FontPlatformData::FontPlatformDataPrivate::~FontPlatformDataPrivate()
{
}

void FontPlatformData::FontPlatformDataPrivate::update()
{
    size = font.Size();
    bold = font.Flags() & B_BOLD_FACE;
    oblique = font.Flags() & B_ITALIC_FACE;
}

void FontPlatformData::FontPlatformDataPrivate::deref()
{
    --refCount;
    if (!refCount)
        delete this;
}

void FontPlatformData::FontPlatformDataPrivate::addRef()
{
    ++refCount;
}

// #pragma mark -

FontPlatformData::FontPlatformData()
    : m_data(0)
{
}

FontPlatformData::FontPlatformData(const FontDescription& fontDescription, const AtomicString& familyName)
    : m_data(new FontPlatformDataPrivate())
{
    m_data->font.SetSize(fontDescription.computedSize());

    font_family fontFamily;
    findMatchingFontFamily(familyName, &fontFamily);

    font_style fontStyle;
    findMatchingFontStyle(fontFamily, fontDescription.weight() == FontWeightBold, fontDescription.italic(), &fontStyle);

    m_data->font.SetFamilyAndStyle(fontFamily, fontStyle);

    m_data->update();
}

FontPlatformData::FontPlatformData(WTF::HashTableDeletedValueType)
    : m_data(reinterpret_cast<FontPlatformDataPrivate*>(-1))
{
}

FontPlatformData::FontPlatformData(const BFont& font)
    : m_data(new FontPlatformDataPrivate(font))
{
}

FontPlatformData::FontPlatformData(float size, bool bold, bool oblique)
    : m_data(0)
{
    // NOTE: This constructor is used from FontCache to initialize an
    // "empty" FontPlatformData. BUT FontDataCacheKeyTraits specifies
    // "emptyValueIsZero" with true, which means this constructor isn't
    // actually called for new FontPlatformData objects used as keys in
    // the global gFontDataCache HashMap! This only works because we
    // indeed don't need initialization for this case.
    if (!isEmtpyValue(size, bold, oblique))
        m_data = new FontPlatformDataPrivate(size, bold, oblique);
}

FontPlatformData::FontPlatformData(const FontPlatformData& other)
    : m_data(other.m_data)
{
    if (m_data && !isHashTableDeletedValue())
        m_data->addRef();
}

FontPlatformData::FontPlatformData(const FontPlatformData& other, float size)
    : m_data(new FontPlatformDataPrivate())
{
    m_data->font = other.m_data->font;
    m_data->bold = other.m_data->bold;
    m_data->oblique = other.m_data->oblique;
    m_data->font.SetSize(size);
    m_data->size = m_data->font.Size();
}

FontPlatformData::~FontPlatformData()
{
    if (!m_data || isHashTableDeletedValue())
        return;
    m_data->deref();
}

FontPlatformData& FontPlatformData::operator=(const FontPlatformData& other)
{
    if (m_data == other.m_data)
        return *this;
    if (m_data && !isHashTableDeletedValue())
         m_data->deref();

    m_data = other.m_data;
    if (m_data && !isHashTableDeletedValue())
        m_data->addRef();
    return *this;
}

bool FontPlatformData::operator==(const FontPlatformData& other) const
{
    if (m_data == other.m_data)
        return true;

    if (!m_data || !other.m_data
        || isHashTableDeletedValue() || other.isHashTableDeletedValue()) {
        return false;
    }

    return m_data->size == other.m_data->size
        && m_data->bold == other.m_data->bold
        && m_data->oblique == other.m_data->oblique
        && m_data->font == other.m_data->font;
}

const BFont* FontPlatformData::font() const
{
    return &(m_data->font);
}

bool FontPlatformData::isFixedPitch()
{
    ASSERT(!isHashTableDeletedValue());
    if (m_data)
        return m_data->font.Spacing() == B_FIXED_SPACING;
    return false;
}

float FontPlatformData::size() const
{
    ASSERT(!isHashTableDeletedValue());
    if (m_data)
        return m_data->size;
    return 0;
}

bool FontPlatformData::bold() const
{
    ASSERT(!isHashTableDeletedValue());
    if (m_data)
        return m_data->bold;
    return false;
}

bool FontPlatformData::oblique() const
{
    ASSERT(!isHashTableDeletedValue());
    if (m_data)
        return m_data->oblique;
    return false;
}

unsigned FontPlatformData::hash() const
{
    if (!m_data)
        return 0;
    if (isHashTableDeletedValue())
        return 1;
	String hashString = description();
    return StringHasher::hashMemory<sizeof(hashString.length())>(hashString.deprecatedCharacters());
}

bool FontPlatformData::isHashTableDeletedValue() const
{
    return m_data == reinterpret_cast<FontPlatformDataPrivate*>(-1);
}

String FontPlatformData::description() const
{
	font_family fontFamily;
	font_style fontStyle;
	float size = 0;
	bool isBold = false;
	bool isOblique = false;
	if (m_data) {
	    m_data->font.GetFamilyAndStyle(&fontFamily, &fontStyle);
	    size = m_data->size;
	    isBold = m_data->bold;
	    isOblique = m_data->oblique;
	} else {
		memset(&fontFamily, 0, sizeof(fontFamily));
		memset(&fontStyle, 0, sizeof(fontStyle));
	}
    return String(fontFamily) + "/" + String(fontStyle) + String::format("/%.1f/%d&%d", size, isBold, isOblique);
}

void
FontPlatformData::SetFallBackSerifFont(const BString& font)
{
	strncpy(m_FallbackSerifFontFamily, font.String(), B_FONT_FAMILY_LENGTH + 1);
}

void
FontPlatformData::SetFallBackSansSerifFont(const BString& font)
{
	strncpy(m_FallbackSansSerifFontFamily, font.String(), B_FONT_FAMILY_LENGTH + 1);
}

void
FontPlatformData::SetFallBackFixedFont(const BString& font)
{
	strncpy(m_FallbackFixedFontFamily, font.String(), B_FONT_FAMILY_LENGTH + 1);
}

void
FontPlatformData::SetFallBackStandardFont(const BString& font)
{
	strncpy(m_FallbackStandardFontFamily, font.String(), B_FONT_FAMILY_LENGTH + 1);
}

} // namespace WebCore

