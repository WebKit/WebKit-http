/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

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

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/
#include "config.h"
#include "FontCustomPlatformData.h"

#include "FontDescription.h"
#include "FontPlatformData.h"
#include "SharedBuffer.h"
#include <QStringList>

namespace WebCore {

FontPlatformData FontCustomPlatformData::fontPlatformData(const FontDescription& description, bool bold, bool italic, const FontFeatureSettings& fontFaceFeatures, const FontVariantSettings& fontFaceVariantSettings, FontSelectionSpecifiedCapabilities fontFaceCapabilities)
{
    Q_ASSERT(m_rawFont.isValid());
    int size = description.computedPixelSize();
    m_rawFont.setPixelSize(qreal(size));
    return FontPlatformData(m_rawFont);
}

std::unique_ptr<FontCustomPlatformData> createFontCustomPlatformData(SharedBuffer& buffer, const String&)
{
    const QByteArray fontData(buffer.data(), buffer.size());

    // Pixel size doesn't matter at this point, it is set in FontCustomPlatformData::fontPlatformData.
    QRawFont rawFont(fontData, /*pixelSize = */0, QFont::PreferVerticalHinting);
    if (!rawFont.isValid())
        return 0;

    auto data = std::make_unique<FontCustomPlatformData>();
    data->m_rawFont = rawFont;
    return data;
}

bool FontCustomPlatformData::supportsFormat(const String& format)
{
    return equalLettersIgnoringASCIICase(format, "truetype")
        || equalLettersIgnoringASCIICase(format, "opentype")
        || equalLettersIgnoringASCIICase(format, "woff")
#if USE(WOFF2)
        || equalLettersIgnoringASCIICase(format, "woff2")
#endif
    ;
}

}
