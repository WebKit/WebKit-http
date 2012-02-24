/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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

#include <windows.h>

#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "HWndDC.h"
#include <cairo.h>
#include <cairo-win32.h>
#include <mlang.h>
#include <wtf/MathExtras.h>

namespace WebCore {

void SimpleFontData::platformInit()
{
    m_syntheticBoldOffset = m_platformData.syntheticBold() ? 1.0f : 0.f;
    m_scriptCache = 0;
    m_scriptFontProperties = 0;
    m_isSystemFont = false;

    if (m_platformData.useGDI())
       return initGDIFont();

    if (!m_platformData.size()) {
        m_fontMetrics.reset();
        m_avgCharWidth = 0;
        m_maxCharWidth = 0;
        m_isSystemFont = false;
        m_scriptCache = 0;
        m_scriptFontProperties = 0;
        return;
    }

    HWndDC dc(0);
    SaveDC(dc);

    cairo_scaled_font_t* scaledFont = m_platformData.scaledFont();
    const double metricsMultiplier = cairo_win32_scaled_font_get_metrics_factor(scaledFont) * m_platformData.size();

    cairo_win32_scaled_font_select_font(scaledFont, dc);

    TEXTMETRIC textMetrics;
    GetTextMetrics(dc, &textMetrics);
    float ascent = textMetrics.tmAscent * metricsMultiplier;
    float descent = textMetrics.tmDescent * metricsMultiplier;
    float xHeight = ascent * 0.56f; // Best guess for xHeight for non-Truetype fonts.
    float lineGap = textMetrics.tmExternalLeading * metricsMultiplier;

    int faceLength = ::GetTextFace(dc, 0, 0);
    Vector<WCHAR> faceName(faceLength);
    ::GetTextFace(dc, faceLength, faceName.data());
    m_isSystemFont = !wcscmp(faceName.data(), L"Lucida Grande");
 
    ascent = ascentConsideringMacAscentHack(faceName.data(), ascent, descent);

    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));
    m_avgCharWidth = textMetrics.tmAveCharWidth * metricsMultiplier;
    m_maxCharWidth = textMetrics.tmMaxCharWidth * metricsMultiplier;

    cairo_text_extents_t extents;
    cairo_scaled_font_text_extents(scaledFont, "x", &extents);
    xHeight = -extents.y_bearing;

    m_fontMetrics.setXHeight(xHeight);
    cairo_win32_scaled_font_done_font(scaledFont);

    m_scriptCache = 0;
    m_scriptFontProperties = 0;

    RestoreDC(dc, -1);
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    if (m_platformData.useGDI())
        return boundsForGDIGlyph(glyph);
    //FIXME: Implement this
    return FloatRect();
}
    
float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (m_platformData.useGDI())
       return widthForGDIGlyph(glyph);

    if (!m_platformData.size())
        return 0;

    HWndDC dc(0);
    SaveDC(dc);

    cairo_scaled_font_t* scaledFont = m_platformData.scaledFont();
    cairo_win32_scaled_font_select_font(scaledFont, dc);

    int width;
    GetCharWidthI(dc, glyph, 1, 0, &width);

    cairo_win32_scaled_font_done_font(scaledFont);

    RestoreDC(dc, -1);

    const double metricsMultiplier = cairo_win32_scaled_font_get_metrics_factor(scaledFont) * m_platformData.size();
    return width * metricsMultiplier;
}

}
