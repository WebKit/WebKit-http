/*
 * Copyright (C) 2006, 2007 Apple Computer, Inc.
 * Copyright (c) 2006, 2007, 2008, 2009, Google Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Font.h"

#include "FontFallbackList.h"
#include "GlyphBuffer.h"
#include "NotImplemented.h"
#include "PlatformSupport.h"
#include "PlatformContextSkia.h"
#include "SimpleFontData.h"
#include "SkiaFontWin.h"
#include "UniscribeHelperTextRun.h"

#if !USE(SKIA_TEXT)
#include "TransparencyWin.h"
#include "skia/ext/skia_utils_win.h"
#endif

#include <windows.h>

using namespace std;

namespace WebCore {

#if !USE(SKIA_TEXT)
namespace {

class TransparencyAwareFontPainter {
public:
    TransparencyAwareFontPainter(GraphicsContext*, const FloatPoint&);
    ~TransparencyAwareFontPainter();

protected:
    // Called by our subclass' constructor to initialize GDI if necessary. This
    // is a separate function so it can be called after the subclass finishes
    // construction (it calls virtual functions).
    void init();

    virtual IntRect estimateTextBounds() = 0;

    // Use the context from the transparency helper when drawing with GDI. It
    // may point to a temporary one.
    GraphicsContext* m_graphicsContext;
    PlatformGraphicsContext* m_platformContext;

    FloatPoint m_point;

    // Set when Windows can handle the type of drawing we're doing.
    bool m_useGDI;

    // These members are valid only when m_useGDI is set.
    HDC m_hdc;
    TransparencyWin m_transparency;

private:
    // Call when we're using GDI mode to initialize the TransparencyWin to help
    // us draw GDI text.
    void initializeForGDI();

    bool m_createdTransparencyLayer; // We created a layer to give the font some alpha.
};

TransparencyAwareFontPainter::TransparencyAwareFontPainter(GraphicsContext* context,
                                                           const FloatPoint& point)
    : m_graphicsContext(context)
    , m_platformContext(context->platformContext())
    , m_point(point)
    , m_useGDI(windowsCanHandleTextDrawing(context))
    , m_hdc(0)
    , m_createdTransparencyLayer(false)
{
}

void TransparencyAwareFontPainter::init()
{
    if (m_useGDI)
        initializeForGDI();
}

void TransparencyAwareFontPainter::initializeForGDI()
{
    m_graphicsContext->save();
    SkColor color = m_platformContext->effectiveFillColor();
    // Used only when m_createdTransparencyLayer is true.
    float layerAlpha = 0.0f;
    if (SkColorGetA(color) != 0xFF) {
        // When the font has some transparency, apply it by creating a new
        // transparency layer with that opacity applied. We'll actually create
        // a new transparency layer after we calculate the bounding box.
        m_createdTransparencyLayer = true;
        layerAlpha = SkColorGetA(color) / 255.0f;
        // The color should be opaque now.
        color = SkColorSetRGB(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color));
    }

    TransparencyWin::LayerMode layerMode;
    IntRect layerRect;
    if (m_platformContext->isDrawingToImageBuffer()) {
        // Assume if we're drawing to an image buffer that the background
        // is not opaque and we have to undo ClearType. We may want to
        // enhance this to actually check, since it will often be opaque
        // and we could do ClearType in that case.
        layerMode = TransparencyWin::TextComposite;
        layerRect = estimateTextBounds();
        m_graphicsContext->clip(layerRect);
        if (m_createdTransparencyLayer)
            m_graphicsContext->beginTransparencyLayer(layerAlpha);

        // The transparency helper requires that we draw text in black in
        // this mode and it will apply the color.
        m_transparency.setTextCompositeColor(color);
        color = SkColorSetRGB(0, 0, 0);
    } else if (m_createdTransparencyLayer || m_platformContext->canvas()->isDrawingToLayer()) {
        // When we're drawing a web page, we know the background is opaque,
        // but if we're drawing to a layer, we still need extra work.
        layerMode = TransparencyWin::OpaqueCompositeLayer;
        layerRect = estimateTextBounds();
        m_graphicsContext->clip(layerRect);
        if (m_createdTransparencyLayer)
            m_graphicsContext->beginTransparencyLayer(layerAlpha);
    } else {
        // Common case of drawing onto the bottom layer of a web page: we
        // know everything is opaque so don't need to do anything special.
        layerMode = TransparencyWin::NoLayer;
    }

    // Bug 26088 - init() might fail if layerRect is invalid. Given this, we
    // need to be careful to check for null pointers everywhere after this call
    m_transparency.init(m_graphicsContext, layerMode, 
                        TransparencyWin::KeepTransform, layerRect);

    // Set up the DC, using the one from the transparency helper.
    if (m_transparency.platformContext()) {
        m_hdc = skia::BeginPlatformPaint(m_transparency.platformContext()->canvas());
        SetTextColor(m_hdc, skia::SkColorToCOLORREF(color));
        SetBkMode(m_hdc, TRANSPARENT);
    }
}

TransparencyAwareFontPainter::~TransparencyAwareFontPainter()
{
    if (!m_useGDI || !m_graphicsContext || !m_platformContext)
        return;
    m_transparency.composite();
    if (m_createdTransparencyLayer)
        m_graphicsContext->endTransparencyLayer();
    m_graphicsContext->restore();
    if (m_transparency.platformContext())
        skia::EndPlatformPaint(m_transparency.platformContext()->canvas());
}

// Specialization for simple GlyphBuffer painting.
class TransparencyAwareGlyphPainter : public TransparencyAwareFontPainter {
 public:
    TransparencyAwareGlyphPainter(GraphicsContext*,
                                  const SimpleFontData*,
                                  const GlyphBuffer&,
                                  int from, int numGlyphs,
                                  const FloatPoint&);
    ~TransparencyAwareGlyphPainter();

    // Draws the partial string of glyphs, starting at |startAdvance| to the
    // left of m_point. We express it this way so that if we're using the Skia
    // drawing path we can use floating-point positioning, even though we have
    // to use integer positioning in the GDI path.
    bool drawGlyphs(int numGlyphs, const WORD* glyphs, const int* advances, float startAdvance) const;

 private:
    virtual IntRect estimateTextBounds();

    const SimpleFontData* m_font;
    const GlyphBuffer& m_glyphBuffer;
    int m_from;
    int m_numGlyphs;

    // When m_useGdi is set, this stores the previous HFONT selected into the
    // m_hdc so we can restore it.
    HGDIOBJ m_oldFont; // For restoring the DC to its original state.
};

TransparencyAwareGlyphPainter::TransparencyAwareGlyphPainter(
    GraphicsContext* context,
    const SimpleFontData* font,
    const GlyphBuffer& glyphBuffer,
    int from, int numGlyphs,
    const FloatPoint& point)
    : TransparencyAwareFontPainter(context, point)
    , m_font(font)
    , m_glyphBuffer(glyphBuffer)
    , m_from(from)
    , m_numGlyphs(numGlyphs)
    , m_oldFont(0)
{
    init();

    if (m_hdc)
        m_oldFont = ::SelectObject(m_hdc, m_font->platformData().hfont());
}

TransparencyAwareGlyphPainter::~TransparencyAwareGlyphPainter()
{
    if (m_useGDI && m_hdc)
        ::SelectObject(m_hdc, m_oldFont);
}


// Estimates the bounding box of the given text. This is copied from
// FontCGWin.cpp, it is possible, but a lot more work, to get the precide
// bounds.
IntRect TransparencyAwareGlyphPainter::estimateTextBounds()
{
    int totalWidth = 0;
    for (int i = 0; i < m_numGlyphs; i++)
        totalWidth += lroundf(m_glyphBuffer.advanceAt(m_from + i));

    const FontMetrics& fontMetrics = m_font->fontMetrics();
    return IntRect(m_point.x() - (fontMetrics.ascent() + fontMetrics.descent()) / 2,
                   m_point.y() - fontMetrics.ascent() - fontMetrics.lineGap(),
                   totalWidth + fontMetrics.ascent() + fontMetrics.descent(),
                   fontMetrics.lineSpacing()); 
}

bool TransparencyAwareGlyphPainter::drawGlyphs(int numGlyphs,
                                               const WORD* glyphs,
                                               const int* advances,
                                               float startAdvance) const
{
    if (!m_useGDI) {
        SkPoint origin = m_point;
        origin.fX += SkFloatToScalar(startAdvance);
        paintSkiaText(m_graphicsContext, m_font->platformData(),
                      numGlyphs, glyphs, advances, 0, &origin);
        return true;
    }

    if (!m_graphicsContext || !m_hdc)
        return true;

    // Windows' origin is the top-left of the bounding box, so we have
    // to subtract off the font ascent to get it.
    int x = lroundf(m_point.x() + startAdvance);
    int y = lroundf(m_point.y() - m_font->fontMetrics().ascent());

    // If there is a non-blur shadow and both the fill color and shadow color 
    // are opaque, handle without skia. 
    FloatSize shadowOffset;
    float shadowBlur;
    Color shadowColor;
    ColorSpace shadowColorSpace;
    if (m_graphicsContext->getShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace)) {
        // If there is a shadow and this code is reached, windowsCanHandleDrawTextShadow()
        // will have already returned true during the ctor initiatization of m_useGDI
        ASSERT(shadowColor.alpha() == 255);
        ASSERT(m_graphicsContext->fillColor().alpha() == 255);
        ASSERT(!shadowBlur);
        COLORREF textColor = skia::SkColorToCOLORREF(SkColorSetARGB(255, shadowColor.red(), shadowColor.green(), shadowColor.blue()));
        COLORREF savedTextColor = GetTextColor(m_hdc);
        SetTextColor(m_hdc, textColor);
        ExtTextOut(m_hdc, x + shadowOffset.width(), y + shadowOffset.height(), ETO_GLYPH_INDEX, 0, reinterpret_cast<const wchar_t*>(&glyphs[0]), numGlyphs, &advances[0]);
        SetTextColor(m_hdc, savedTextColor);
    }
    
    return !!ExtTextOut(m_hdc, x, y, ETO_GLYPH_INDEX, 0, reinterpret_cast<const wchar_t*>(&glyphs[0]), numGlyphs, &advances[0]);
}

class TransparencyAwareUniscribePainter : public TransparencyAwareFontPainter {
 public:
    TransparencyAwareUniscribePainter(GraphicsContext*,
                                      const Font*,
                                      const TextRun&,
                                      int from, int to,
                                      const FloatPoint&);
    ~TransparencyAwareUniscribePainter();

    // Uniscibe will draw directly into our buffer, so we need to expose our DC.
    HDC hdc() const { return m_hdc; }

 private:
    virtual IntRect estimateTextBounds();

    const Font* m_font;
    const TextRun& m_run;
    int m_from;
    int m_to;
};

TransparencyAwareUniscribePainter::TransparencyAwareUniscribePainter(
    GraphicsContext* context,
    const Font* font,
    const TextRun& run,
    int from, int to,
    const FloatPoint& point)
    : TransparencyAwareFontPainter(context, point)
    , m_font(font)
    , m_run(run)
    , m_from(from)
    , m_to(to)
{
    init();
}

TransparencyAwareUniscribePainter::~TransparencyAwareUniscribePainter()
{
}

IntRect TransparencyAwareUniscribePainter::estimateTextBounds()
{
    // This case really really sucks. There is no convenient way to estimate
    // the bounding box. So we run Uniscribe twice. If we find this happens a
    // lot, the way to fix it is to make the extra layer after the
    // UniscribeHelper has measured the text.
    IntPoint intPoint(lroundf(m_point.x()),
                      lroundf(m_point.y()));

    UniscribeHelperTextRun state(m_run, *m_font);
    int left = lroundf(m_point.x()) + state.characterToX(m_from);
    int right = lroundf(m_point.x()) + state.characterToX(m_to);
    
    // Adjust for RTL script since we just want to know the text bounds.
    if (left > right)
        std::swap(left, right);

    // This algorithm for estimating how much extra space we need (the text may
    // go outside the selection rect) is based roughly on
    // TransparencyAwareGlyphPainter::estimateTextBounds above.
    const FontMetrics& fontMetrics = m_font->fontMetrics();
    return IntRect(left - (fontMetrics.ascent() + fontMetrics.descent()) / 2,
                   m_point.y() - fontMetrics.ascent() - fontMetrics.lineGap(),
                   (right - left) + fontMetrics.ascent() + fontMetrics.descent(),
                   fontMetrics.lineSpacing());
}

} // namespace
#endif

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

#if USE(SKIA_TEXT)
void Font::drawGlyphs(GraphicsContext* graphicsContext,
                      const SimpleFontData* font,
                      const GlyphBuffer& glyphBuffer,
                      int from,
                      int numGlyphs,
                      const FloatPoint& point) const
{
    SkColor color = graphicsContext->platformContext()->effectiveFillColor();
    unsigned char alpha = SkColorGetA(color);
    // Skip 100% transparent text; no need to draw anything.
    if (!alpha && graphicsContext->platformContext()->getStrokeStyle() == NoStroke && !graphicsContext->hasShadow())
        return;

    // We draw the glyphs in chunks to avoid having to do a heap allocation for
    // the arrays of characters and advances.
    const int kMaxBufferLength = 256;
    Vector<int, kMaxBufferLength> advances;
    int glyphIndex = 0;  // The starting glyph of the current chunk.

    float horizontalOffset = point.x(); // The floating point offset of the left side of the current glyph.
#if ENABLE(OPENTYPE_VERTICAL)
    const OpenTypeVerticalData* verticalData = font->verticalData();
    if (verticalData) {
        Vector<FloatPoint, kMaxBufferLength> translations;
        Vector<GOFFSET, kMaxBufferLength> offsets;

        // Skia doesn't have matrix for glyph coordinate space, so we rotate back the CTM.
        AffineTransform savedMatrix = graphicsContext->getCTM();
        graphicsContext->concatCTM(AffineTransform(0, -1, 1, 0, point.x(), point.y()));
        graphicsContext->concatCTM(AffineTransform(1, 0, 0, 1, -point.x(), -point.y()));

        const FontMetrics& metrics = font->fontMetrics();
        SkScalar verticalOriginX = SkFloatToScalar(point.x() + metrics.floatAscent() - metrics.floatAscent(IdeographicBaseline));
        while (glyphIndex < numGlyphs) {
            // How many chars will be in this chunk?
            int curLen = std::min(kMaxBufferLength, numGlyphs - glyphIndex);

            const Glyph* glyphs = glyphBuffer.glyphs(from + glyphIndex);
            translations.resize(curLen);
            verticalData->getVerticalTranslationsForGlyphs(font, &glyphs[0], curLen, reinterpret_cast<float*>(&translations[0]));
            // To position glyphs vertically, we use offsets instead of advances.
            advances.resize(curLen);
            advances.fill(0);
            offsets.resize(curLen);
            float currentWidth = 0;
            for (int i = 0; i < curLen; ++i, ++glyphIndex) {
                offsets[i].du = lroundf(translations[i].x());
                offsets[i].dv = -lroundf(currentWidth - translations[i].y());
                currentWidth += glyphBuffer.advanceAt(from + glyphIndex);
            }
            SkPoint origin;
            origin.set(verticalOriginX, SkFloatToScalar(point.y() + horizontalOffset - point.x()));
            horizontalOffset += currentWidth;
            paintSkiaText(graphicsContext, font->platformData(), curLen, &glyphs[0], &advances[0], &offsets[0], &origin);
        }

        graphicsContext->setCTM(savedMatrix);
        return;
    }
#endif

    // In order to round all offsets to the correct pixel boundary, this code keeps track of the absolute position
    // of each glyph in floating point units and rounds to integer advances at the last possible moment.

    int lastHorizontalOffsetRounded = lroundf(horizontalOffset); // The rounded offset of the left side of the last glyph rendered.
    Vector<WORD, kMaxBufferLength> glyphs;
    while (glyphIndex < numGlyphs) {
        // How many chars will be in this chunk?
        int curLen = std::min(kMaxBufferLength, numGlyphs - glyphIndex);
        glyphs.resize(curLen);
        advances.resize(curLen);

        float currentWidth = 0;
        for (int i = 0; i < curLen; ++i, ++glyphIndex) {
            glyphs[i] = glyphBuffer.glyphAt(from + glyphIndex);
            horizontalOffset += glyphBuffer.advanceAt(from + glyphIndex);
            advances[i] = lroundf(horizontalOffset) - lastHorizontalOffsetRounded;
            lastHorizontalOffsetRounded += advances[i];
            currentWidth += glyphBuffer.advanceAt(from + glyphIndex);
            
            // Bug 26088 - very large positive or negative runs can fail to
            // render so we clamp the size here. In the specs, negative
            // letter-spacing is implementation-defined, so this should be
            // fine, and it matches Safari's implementation. The call actually
            // seems to crash if kMaxNegativeRun is set to somewhere around
            // -32830, so we give ourselves a little breathing room.
            const int maxNegativeRun = -32768;
            const int maxPositiveRun =  32768;
            if ((currentWidth + advances[i] < maxNegativeRun) || (currentWidth + advances[i] > maxPositiveRun)) 
                advances[i] = 0;
        }

        SkPoint origin = point;
        origin.fX += SkFloatToScalar(horizontalOffset - point.x() - currentWidth);
        paintSkiaText(graphicsContext, font->platformData(), curLen, &glyphs[0], &advances[0], 0, &origin);
    }
}
#else
static void drawGlyphsWin(GraphicsContext* graphicsContext,
                          const SimpleFontData* font,
                          const GlyphBuffer& glyphBuffer,
                          int from,
                          int numGlyphs,
                          const FloatPoint& point) {
    TransparencyAwareGlyphPainter painter(graphicsContext, font, glyphBuffer, from, numGlyphs, point);

    // We draw the glyphs in chunks to avoid having to do a heap allocation for
    // the arrays of characters and advances. Since ExtTextOut is the
    // lowest-level text output function on Windows, there should be little
    // penalty for splitting up the text. On the other hand, the buffer cannot
    // be bigger than 4094 or the function will fail.
    const int kMaxBufferLength = 256;
    Vector<WORD, kMaxBufferLength> glyphs;
    Vector<int, kMaxBufferLength> advances;
    int glyphIndex = 0; // The starting glyph of the current chunk.

    // In order to round all offsets to the correct pixel boundary, this code keeps track of the absolute position
    // of each glyph in floating point units and rounds to integer advances at the last possible moment.

    float horizontalOffset = point.x(); // The floating point offset of the left side of the current glyph.
    int lastHorizontalOffsetRounded = lroundf(horizontalOffset); // The rounded offset of the left side of the last glyph rendered.
    while (glyphIndex < numGlyphs) {
        // How many chars will be in this chunk?
        int curLen = min(kMaxBufferLength, numGlyphs - glyphIndex);
        glyphs.resize(curLen);
        advances.resize(curLen);

        float currentWidth = 0;
        for (int i = 0; i < curLen; ++i, ++glyphIndex) {
            glyphs[i] = glyphBuffer.glyphAt(from + glyphIndex);
            horizontalOffset += glyphBuffer.advanceAt(from + glyphIndex);
            advances[i] = lroundf(horizontalOffset) - lastHorizontalOffsetRounded;
            lastHorizontalOffsetRounded += advances[i];
            currentWidth += glyphBuffer.advanceAt(from + glyphIndex);
            
            // Bug 26088 - very large positive or negative runs can fail to
            // render so we clamp the size here. In the specs, negative
            // letter-spacing is implementation-defined, so this should be
            // fine, and it matches Safari's implementation. The call actually
            // seems to crash if kMaxNegativeRun is set to somewhere around
            // -32830, so we give ourselves a little breathing room.
            const int maxNegativeRun = -32768;
            const int maxPositiveRun =  32768;
            if ((currentWidth + advances[i] < maxNegativeRun) || (currentWidth + advances[i] > maxPositiveRun)) 
                advances[i] = 0;
        }

        // Actually draw the glyphs (with retry on failure).
        bool success = false;
        for (int executions = 0; executions < 2; ++executions) {
            success = painter.drawGlyphs(curLen, &glyphs[0], &advances[0], horizontalOffset - point.x() - currentWidth);
            if (!success && !executions) {
                // Ask the browser to load the font for us and retry.
                PlatformSupport::ensureFontLoaded(font->platformData().hfont());
                continue;
            }
            break;
        }

        if (!success)
            LOG_ERROR("Unable to draw the glyphs after second attempt");
    }
}

void Font::drawGlyphs(GraphicsContext* graphicsContext,
                      const SimpleFontData* font,
                      const GlyphBuffer& glyphBuffer,
                      int from,
                      int numGlyphs,
                      const FloatPoint& point) const
{
    SkColor color = graphicsContext->platformContext()->effectiveFillColor();
    unsigned char alpha = SkColorGetA(color);
    // Skip 100% transparent text; no need to draw anything.
    if (!alpha && graphicsContext->platformContext()->getStrokeStyle() == NoStroke && !graphicsContext->hasShadow())
        return;
    if (!alpha || windowsCanHandleDrawTextShadow(graphicsContext) || !windowsCanHandleTextDrawingWithoutShadow(graphicsContext)) {
        drawGlyphsWin(graphicsContext, font, glyphBuffer, from, numGlyphs, point);
        return;
    }
    // Draw in two passes: skia for the shadow, GDI for foreground text
    // pass1: shadow (will use skia)
    graphicsContext->save();
    graphicsContext->setFillColor(Color::transparent, graphicsContext->fillColorSpace());
    drawGlyphsWin(graphicsContext, font, glyphBuffer, from, numGlyphs, point);
    graphicsContext->restore();
    // pass2: foreground text (will use GDI)
    FloatSize shadowOffset;
    float shadowBlur;
    Color shadowColor;
    ColorSpace shadowColorSpace;
    graphicsContext->getShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace);
    graphicsContext->setShadow(shadowOffset, shadowBlur, Color::transparent, shadowColorSpace);
    drawGlyphsWin(graphicsContext, font, glyphBuffer, from, numGlyphs, point);
    graphicsContext->setShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace);
}
#endif

FloatRect Font::selectionRectForComplexText(const TextRun& run,
                                            const FloatPoint& point,
                                            int h,
                                            int from,
                                            int to) const
{
    UniscribeHelperTextRun state(run, *this);
    float left = static_cast<float>(point.x() + state.characterToX(from));
    float right = static_cast<float>(point.x() + state.characterToX(to));

    // If the text is RTL, left will actually be after right.
    if (left < right)
        return FloatRect(left, point.y(),
                       right - left, static_cast<float>(h));

    return FloatRect(right, point.y(),
                     left - right, static_cast<float>(h));
}

void Font::drawComplexText(GraphicsContext* graphicsContext,
                           const TextRun& run,
                           const FloatPoint& point,
                           int from,
                           int to) const
{
    PlatformGraphicsContext* context = graphicsContext->platformContext();
    UniscribeHelperTextRun state(run, *this);

    SkColor color = graphicsContext->platformContext()->effectiveFillColor();
    unsigned char alpha = SkColorGetA(color);
    // Skip 100% transparent text; no need to draw anything.
    if (!alpha && graphicsContext->platformContext()->getStrokeStyle() == NoStroke)
        return;

#if USE(SKIA_TEXT)
    HDC hdc = 0;
#else
    TransparencyAwareUniscribePainter painter(graphicsContext, this, run, from, to, point);

    HDC hdc = painter.hdc();
    if (windowsCanHandleTextDrawing(graphicsContext) && !hdc)
        return;

    // TODO(maruel): http://b/700464 SetTextColor doesn't support transparency.
    // Enforce non-transparent color.
    color = SkColorSetRGB(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color));
    if (hdc) {
        SetTextColor(hdc, skia::SkColorToCOLORREF(color));
        SetBkMode(hdc, TRANSPARENT);
    }

    // If there is a non-blur shadow and both the fill color and shadow color 
    // are opaque, handle without skia. 
    FloatSize shadowOffset;
    float shadowBlur;
    Color shadowColor;
    ColorSpace shadowColorSpace;
    if (graphicsContext->getShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace) && windowsCanHandleDrawTextShadow(graphicsContext)) {
        COLORREF textColor = skia::SkColorToCOLORREF(SkColorSetARGB(255, shadowColor.red(), shadowColor.green(), shadowColor.blue()));
        COLORREF savedTextColor = GetTextColor(hdc);
        SetTextColor(hdc, textColor);
        state.draw(graphicsContext, hdc, static_cast<int>(point.x()) + shadowOffset.width(),
                   static_cast<int>(point.y() - fontMetrics().ascent()) + shadowOffset.height(), from, to);
        SetTextColor(hdc, savedTextColor); 
    }
#endif
    // Uniscribe counts the coordinates from the upper left, while WebKit uses
    // the baseline, so we have to subtract off the ascent.
    state.draw(graphicsContext, hdc, lroundf(point.x()), lroundf(point.y() - fontMetrics().ascent()), from, to);
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    notImplemented();
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* /* fallbackFonts */, GlyphOverflow* /* glyphOverflow */) const
{
    UniscribeHelperTextRun state(run, *this);
    return static_cast<float>(state.width());
}

int Font::offsetForPositionForComplexText(const TextRun& run, float xFloat,
                                          bool includePartialGlyphs) const
{
    // FIXME: This truncation is not a problem for HTML, but only affects SVG, which passes floating-point numbers
    // to Font::offsetForPosition(). Bug http://webkit.org/b/40673 tracks fixing this problem.
    int x = static_cast<int>(xFloat);

    // Mac code ignores includePartialGlyphs, and they don't know what it's
    // supposed to do, so we just ignore it as well.
    UniscribeHelperTextRun state(run, *this);
    int charIndex = state.xToCharacter(x);

    // XToCharacter will return -1 if the position is before the first
    // character (we get called like this sometimes).
    if (charIndex < 0)
        charIndex = 0;
    return charIndex;
}

} // namespace WebCore
