/*
    Copyright (C) 2008 Holger Hans Peter Freyther
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2015 The Qt Company Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "config.h"
#include "FontPlatformData.h"

#include "FontCascade.h"
#include "SharedBuffer.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

// See http://www.w3.org/TR/css3-fonts/#font-weight-prop
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
static inline QFont::Weight toQFontWeight(FontSelectionValue fontWeight)
{
    if (fontWeight < FontSelectionValue(150))
        return QFont::Thin;
    if (fontWeight < FontSelectionValue(250))
        return QFont::ExtraLight;
    if (fontWeight < FontSelectionValue(350))
        return QFont::Light;
    if (fontWeight < FontSelectionValue(450))
        return QFont::Normal;
    if (fontWeight < FontSelectionValue(550))
        return QFont::Medium;
    if (fontWeight < FontSelectionValue(650))
        return QFont::DemiBold;
    if (fontWeight < FontSelectionValue(750))
        return QFont::Bold;
    if (fontWeight < FontSelectionValue(850))
        return QFont::ExtraBold;
    return QFont::Black;
}
#else
static inline QFont::Weight toQFontWeight(FontSelectionValue fontWeight)
{
    if (fontWeight < FontSelectionValue(350))
        return QFont::Light; // QFont::Light == Weight of 25
    if (fontWeight < FontSelectionValue(550))
        return QFont::Normal; // QFont::Normal == Weight of 50
    if (fontWeight < FontSelectionValue(650))
        return QFont::DemiBold; // QFont::DemiBold == Weight of 63
    if (fontWeight < FontSelectionValue(750))
        return QFont::Bold; // QFont::Bold == Weight of 75
    return QFont::Black; // QFont::Black == Weight of 87
}
#endif

FontPlatformData::FontPlatformData(const FontDescription& description, const AtomString& familyName)
    : m_data(adoptRef(new FontPlatformDataPrivate()))
{
    QFont font;
    int requestedSize = description.computedPixelSize();
    font.setFamily(familyName.string());
    if (requestedSize)
        font.setPixelSize(requestedSize);
    font.setItalic(isItalic(description.italic()));
    font.setWeight(toQFontWeight(description.weight()));

    if (!FontCascade::shouldUseSmoothing())
        font.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::NoAntialias | QFont::ForceOutline));
    else
        font.setStyleStrategy(QFont::ForceOutline);

    // WebKit allows font size zero but QFont does not. We will return
    // m_data->size if a font size of zero is requested and pixelSize()
    // otherwise.
    m_size = (!requestedSize) ? requestedSize : font.pixelSize();
    m_data->rawFont = QRawFont::fromFont(font, QFontDatabase::Any);
}

FontPlatformData FontPlatformData::cloneWithSize(const FontPlatformData& source, float size)
{
    FontPlatformData copy(source);
    copy.m_data = adoptRef(new FontPlatformDataPrivate());
    ASSERT(source.m_data);
    copy.m_data->rawFont = source.m_data->rawFont;
    copy.m_data->rawFont.setPixelSize(size);
    copy.m_size = copy.m_data->rawFont.pixelSize();
    return copy;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    if (m_data == other.m_data)
        return true;

    if (!m_data || !other.m_data)
        return false;

    return m_data->rawFont == other.m_data->rawFont;
}

RefPtr<SharedBuffer> FontPlatformData::openTypeTable(uint32_t table) const
{
    const char tag[4] = {
        char(table & 0xff),
        char((table & 0xff00) >> 8),
        char((table & 0xff0000) >> 16),
        char(table >> 24)
    };
    QByteArray tableData = m_data->rawFont.fontTable(tag);

    // TODO: Wrap SharedBuffer around QByteArray when it's possible
    return SharedBuffer::create(tableData.data(), tableData.size());
}

unsigned FontPlatformData::hash() const
{
    if (!m_data)
        return 0;
    if (isHashTableDeletedValue())
        return 1;
    return qHash(m_data->rawFont.familyName()) ^ qHash(m_data->rawFont.style())
            ^ qHash(m_data->rawFont.weight())
            ^ qHash(*reinterpret_cast<const quint32*>(&m_size));
}

#if !LOG_DISABLED
String FontPlatformData::description() const
{
    return nullString();
}
#endif

}
