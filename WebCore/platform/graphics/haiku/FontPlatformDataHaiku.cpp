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

#include "AtomicString.h"


namespace WebCore {

static inline bool isEmtpyValue(const float size, const bool bold, const bool oblique)
{
     // this is the empty value by definition of the trait FontDataCacheKeyTraits
    return !bold && !oblique && size == 0;
}

static void findMatchingFontFamily(const AtomicString& familyName, font_family* fontFamily)
{
    if (BFont().SetFamilyAndStyle(reinterpret_cast<const char*>(familyName.characters()), 0) == B_OK)
        strncpy(*fontFamily, reinterpret_cast<const char*>(familyName.characters()), B_FONT_FAMILY_LENGTH + 1);
    else {
        // If no font family is found for the given name, we use a generic font.
        if (familyName.contains("Sans", false) != B_ERROR)
            strncpy(*fontFamily, "DejaVu Sans", B_FONT_FAMILY_LENGTH + 1);
        else if (familyName.contains("Serif", false) != B_ERROR)
            strncpy(*fontFamily, "DejaVu Serif", B_FONT_FAMILY_LENGTH + 1);
        else if (familyName.contains("Mono", false) != B_ERROR)
            strncpy(*fontFamily, "DejaVu Mono", B_FONT_FAMILY_LENGTH + 1);
        else {
            // This is the fallback font.
            strncpy(*fontFamily, "DejaVu Serif", B_FONT_FAMILY_LENGTH + 1);
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

FontPlatformDataPrivate::FontPlatformDataPrivate()
    : refCount(1)
{
	printf("%p->FontPlatformDataPrivate()\n", this);
	update();
}

FontPlatformDataPrivate::FontPlatformDataPrivate(const float size, const bool bold, const bool oblique)
    : refCount(1)
    , size(size)
    , bold(bold)
    , oblique(oblique)
{
	printf("%p->FontPlatformDataPrivate()\n", this);
}

FontPlatformDataPrivate::FontPlatformDataPrivate(const BFont& font)
    : refCount(1)
    , font(font)
{
	printf("%p->FontPlatformDataPrivate()\n", this);
	update();
}

FontPlatformDataPrivate::~FontPlatformDataPrivate()
{
	printf("%p->~FontPlatformDataPrivate()\n", this);
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

FontPlatformData::FontPlatformData(float size, bool bold, bool oblique)
    : m_data(0)
{
    if (!isEmtpyValue(size, bold, oblique))
        m_data = new FontPlatformDataPrivate(size, bold, oblique);
}

FontPlatformData::FontPlatformData(const FontPlatformData& other)
    : m_data(other.m_data)
{
    if (m_data && !isHashTableDeletedValue())
        ++m_data->refCount;
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
        ++m_data->refCount;
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

unsigned FontPlatformData::hash() const
{
    if (!m_data)
        return 0;
    if (isHashTableDeletedValue())
        return 1;
	String hashString = description();
    return StringImpl::computeHash(hashString.characters(), hashString.length());
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

} // namespace WebCore

