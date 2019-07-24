/*
    Copyright (C) 2008, 2009, 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Holger Hans Peter Freyther

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
#include "Font.h"

#include "FontDescription.h"
#include "NotImplemented.h"

namespace WebCore {

void Font::determinePitch()
{
    notImplemented();
    m_treatAsFixedPitch = false;
}

float Font::platformWidthForGlyph(Glyph glyph) const
{
    if (!glyph || !platformData().size())
        return 0;

    QVector<quint32> glyphIndexes;
    glyphIndexes.append(glyph);
    QVector<QPointF> advances = platformData().rawFont().advancesForGlyphIndexes(glyphIndexes);
    ASSERT(!advances.isEmpty());
    return advances.at(0).x();
}

RefPtr<Font> Font::platformCreateScaledFont(const FontDescription& fontDescription, float scaleFactor) const
{
    const float scaledSize = lroundf(fontDescription.computedSize() * scaleFactor);
    return Font::create(FontPlatformData::cloneWithSize(m_platformData, scaledSize));
}

FloatRect Font::platformBoundsForGlyph(Glyph glyph) const
{
    return m_platformData.rawFont().boundingRect(glyph);
}

void Font::platformInit()
{
    if (!m_platformData.size()) {
         m_fontMetrics.reset();
         m_avgCharWidth = 0;
         m_maxCharWidth = 0;
         return;
    }

    QRawFont rawFont(m_platformData.rawFont());
    float descent = rawFont.descent();
    float ascent = rawFont.ascent();
    float xHeight = rawFont.xHeight();
    float lineSpacing = ascent + descent + rawFont.leading();

    QVector<quint32> indexes = rawFont.glyphIndexesForString(QStringLiteral(" "));
    QVector<QPointF> advances = rawFont.advancesForGlyphIndexes(indexes);
    float spaceWidth = advances.at(0).x();

    indexes = rawFont.glyphIndexesForString(QStringLiteral("0"));
    advances = rawFont.advancesForGlyphIndexes(indexes);
    float zeroWidth = advances.at(0).x();

#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    indexes = rawFont.glyphIndexesForString(QStringLiteral("H"));
    float capHeight = rawFont.boundingRect(indexes.at(0)).height();
#else
    float capHeight = rawFont.capHeight();
#endif

    // The line spacing should always be >= (ascent + descent), but this
    // may be false in some cases due to misbehaving platform libraries.
    // Workaround from SimpleFontPango.cpp and SimpleFontFreeType.cpp
    if (lineSpacing < ascent + descent)
        lineSpacing = ascent + descent;

    // QFontMetricsF::leading() may return negative values on platforms
    // such as FreeType. Calculate the line gap manually instead.
    float lineGap = lineSpacing - ascent - descent;

    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setCapHeight(capHeight);
    // WebKit expects the descent to be positive.
    m_fontMetrics.setDescent(qAbs(descent));
    m_fontMetrics.setLineSpacing(lineSpacing);
    m_fontMetrics.setXHeight(xHeight);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setZeroWidth(zeroWidth);
    m_spaceWidth = spaceWidth;
}

void Font::platformCharWidthInit()
{
    if (!m_platformData.size())
        return;
    QRawFont rawFont(m_platformData.rawFont());
    m_avgCharWidth = rawFont.averageCharWidth();
    m_maxCharWidth = rawFont.maxCharWidth();
}

// QTFIXME: Copied from pathForGlyphs() from FontCascadeQt.cpp
Path Font::platformPathForGlyph(Glyph glyph) const
{
    Path path;
    QPainterPath platformPath = path.ensurePlatformPath();
    QRawFont rawFont(m_platformData.rawFont());
    QPainterPath glyphPath = rawFont.pathForGlyph(glyph);
    platformPath.addPath(glyphPath);
    return path;
}

void Font::platformDestroy()
{
}

}
