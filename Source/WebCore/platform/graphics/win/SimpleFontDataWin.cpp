/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
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

#include "Font.h"
#include "FontCache.h"
#include "FloatRect.h"
#include "FontDescription.h"
#include "HWndDC.h"
#include <mlang.h>
#include <winsock2.h>
#include <wtf/MathExtras.h>

#if USE(CG)
#include <ApplicationServices/ApplicationServices.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

using std::max;

const float cSmallCapsFontSizeMultiplier = 0.7f;

static bool g_shouldApplyMacAscentHack;

void SimpleFontData::setShouldApplyMacAscentHack(bool b)
{
    g_shouldApplyMacAscentHack = b;
}

bool SimpleFontData::shouldApplyMacAscentHack()
{
    return g_shouldApplyMacAscentHack;
}

float SimpleFontData::ascentConsideringMacAscentHack(const WCHAR* faceName, float ascent, float descent)
{
    if (!shouldApplyMacAscentHack())
        return ascent;

    // This code comes from FontDataMac.mm. We only ever do this when running regression tests so that our metrics will match Mac.

    // We need to adjust Times, Helvetica, and Courier to closely match the
    // vertical metrics of their Microsoft counterparts that are the de facto
    // web standard. The AppKit adjustment of 20% is too big and is
    // incorrectly added to line spacing, so we use a 15% adjustment instead
    // and add it to the ascent.
    if (!wcscmp(faceName, L"Times") || !wcscmp(faceName, L"Helvetica") || !wcscmp(faceName, L"Courier"))
        ascent += floorf(((ascent + descent) * 0.15f) + 0.5f);

    return ascent;
}

void SimpleFontData::initGDIFont()
{
    if (!m_platformData.size()) {
        m_fontMetrics.reset();
        m_avgCharWidth = 0;
        m_maxCharWidth = 0;
        return;
    }

     HWndDC hdc(0);
     HGDIOBJ oldFont = SelectObject(hdc, m_platformData.hfont());
     OUTLINETEXTMETRIC metrics;
     GetOutlineTextMetrics(hdc, sizeof(metrics), &metrics);
     TEXTMETRIC& textMetrics = metrics.otmTextMetrics;
     float ascent = textMetrics.tmAscent;
     float descent = textMetrics.tmDescent;
     float lineGap = textMetrics.tmExternalLeading;
     m_fontMetrics.setAscent(ascent);
     m_fontMetrics.setDescent(descent);
     m_fontMetrics.setLineGap(lineGap);
     m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));
     m_avgCharWidth = textMetrics.tmAveCharWidth;
     m_maxCharWidth = textMetrics.tmMaxCharWidth;
     float xHeight = ascent * 0.56f; // Best guess for xHeight if no x glyph is present.

     GLYPHMETRICS gm;
     MAT2 mat = { 1, 0, 0, 1 };
     DWORD len = GetGlyphOutline(hdc, 'x', GGO_METRICS, &gm, 0, 0, &mat);
     if (len != GDI_ERROR && gm.gmptGlyphOrigin.y > 0)
         xHeight = gm.gmptGlyphOrigin.y;

     m_fontMetrics.setXHeight(xHeight);
     m_fontMetrics.setUnitsPerEm(metrics.otmEMSquare);

     SelectObject(hdc, oldFont);
}

void SimpleFontData::platformCharWidthInit()
{
    // GDI Fonts init charwidths in initGDIFont.
    if (!m_platformData.useGDI()) {
        m_avgCharWidth = 0.f;
        m_maxCharWidth = 0.f;
        initCharWidths();
    }
}

void SimpleFontData::platformDestroy()
{
    ScriptFreeCache(&m_scriptCache);
    delete m_scriptFontProperties;
}

PassOwnPtr<SimpleFontData> SimpleFontData::createScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    float scaledSize = scaleFactor * m_platformData.size();
    if (isCustomFont()) {
        FontPlatformData scaledFont(m_platformData);
        scaledFont.setSize(scaledSize);
        return adoptPtr(new SimpleFontData(scaledFont, true, false));
    }

    LOGFONT winfont;
    GetObject(m_platformData.hfont(), sizeof(LOGFONT), &winfont);
    winfont.lfHeight = -lroundf(scaledSize * (m_platformData.useGDI() ? 1 : 32));
    HFONT hfont = CreateFontIndirect(&winfont);
    return adoptPtr(new SimpleFontData(FontPlatformData(hfont, scaledSize, m_platformData.syntheticBold(), m_platformData.syntheticOblique(), m_platformData.useGDI()), isCustomFont(), false));
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = createScaledFontData(fontDescription, cSmallCapsFontSizeMultiplier);

    return m_derivedFontData->smallCaps.get();
}

SimpleFontData* SimpleFontData::emphasisMarkFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = createScaledFontData(fontDescription, .5);

    return m_derivedFontData->emphasisMark.get();
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    // FIXME: Support custom fonts.
    if (isCustomFont())
        return false;

    // FIXME: Microsoft documentation seems to imply that characters can be output using a given font and DC
    // merely by testing code page intersection.  This seems suspect though.  Can't a font only partially
    // cover a given code page?
    IMLangFontLink2* langFontLink = fontCache()->getFontLinkInterface();
    if (!langFontLink)
        return false;

    HWndDC dc(0);

    DWORD acpCodePages;
    langFontLink->CodePageToCodePages(CP_ACP, &acpCodePages);

    DWORD fontCodePages;
    langFontLink->GetFontCodePages(dc, m_platformData.hfont(), &fontCodePages);

    DWORD actualCodePages;
    long numCharactersProcessed;
    long offset = 0;
    while (offset < length) {
        langFontLink->GetStrCodePages(characters, length, acpCodePages, &actualCodePages, &numCharactersProcessed);
        if ((actualCodePages & fontCodePages) == 0)
            return false;
        offset += numCharactersProcessed;
    }

    return true;
}

void SimpleFontData::determinePitch()
{
    if (isCustomFont()) {
        m_treatAsFixedPitch = false;
        return;
    }

    // TEXTMETRICS have this.  Set m_treatAsFixedPitch based off that.
    HWndDC dc(0);
    SaveDC(dc);
    SelectObject(dc, m_platformData.hfont());

    // Yes, this looks backwards, but the fixed pitch bit is actually set if the font
    // is *not* fixed pitch.  Unbelievable but true.
    TEXTMETRIC tm;
    GetTextMetrics(dc, &tm);
    m_treatAsFixedPitch = ((tm.tmPitchAndFamily & TMPF_FIXED_PITCH) == 0);

    RestoreDC(dc, -1);
}

FloatRect SimpleFontData::boundsForGDIGlyph(Glyph glyph) const
{
    HWndDC hdc(0);
    SetGraphicsMode(hdc, GM_ADVANCED);
    HGDIOBJ oldFont = SelectObject(hdc, m_platformData.hfont());
    
    GLYPHMETRICS gdiMetrics;
    static const MAT2 identity = { 0, 1,  0, 0,  0, 0,  0, 1 };
    GetGlyphOutline(hdc, glyph, GGO_METRICS | GGO_GLYPH_INDEX, &gdiMetrics, 0, 0, &identity);
    
    SelectObject(hdc, oldFont);
    
    return FloatRect(gdiMetrics.gmptGlyphOrigin.x, -gdiMetrics.gmptGlyphOrigin.y,
        gdiMetrics.gmBlackBoxX + m_syntheticBoldOffset, gdiMetrics.gmBlackBoxY); 
}
    
float SimpleFontData::widthForGDIGlyph(Glyph glyph) const
{
    HWndDC hdc(0);
    SetGraphicsMode(hdc, GM_ADVANCED);
    HGDIOBJ oldFont = SelectObject(hdc, m_platformData.hfont());

    GLYPHMETRICS gdiMetrics;
    static const MAT2 identity = { 0, 1,  0, 0,  0, 0,  0, 1 };
    GetGlyphOutline(hdc, glyph, GGO_METRICS | GGO_GLYPH_INDEX, &gdiMetrics, 0, 0, &identity);

    SelectObject(hdc, oldFont);

    return gdiMetrics.gmCellIncX + m_syntheticBoldOffset;
}

SCRIPT_FONTPROPERTIES* SimpleFontData::scriptFontProperties() const
{
    if (!m_scriptFontProperties) {
        m_scriptFontProperties = new SCRIPT_FONTPROPERTIES;
        memset(m_scriptFontProperties, 0, sizeof(SCRIPT_FONTPROPERTIES));
        m_scriptFontProperties->cBytes = sizeof(SCRIPT_FONTPROPERTIES);
        HRESULT result = ScriptGetFontProperties(0, scriptCache(), m_scriptFontProperties);
        if (result == E_PENDING) {
            HWndDC dc(0);
            SaveDC(dc);
            SelectObject(dc, m_platformData.hfont());
            ScriptGetFontProperties(dc, scriptCache(), m_scriptFontProperties);
            RestoreDC(dc, -1);
        }
    }
    return m_scriptFontProperties;
}

}
