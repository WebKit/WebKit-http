/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "Font.h"
#include "FontData.h"
#include "FontDescription.h"
#include "FontSelector.h"
#include "GraphicsContext.h"
#include "TextEncoding.h"
#include "NotImplemented.h"
#include <wtf/text/CString.h>
#include <Font.h>
#include <String.h>
#include <View.h>


namespace WebCore {

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    notImplemented();
}

void Font::drawGlyphs(GraphicsContext* graphicsContext, const SimpleFontData* font,
                      const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point) const
{
    BView* view = graphicsContext->platformContext();
    view->PushState();

    rgb_color color = graphicsContext->fillColor();

    if (color.alpha < 255 || graphicsContext->inTransparencyLayer())
        view->SetDrawingMode(B_OP_ALPHA);
    else
        view->SetDrawingMode(B_OP_OVER);
    view->SetHighColor(color);
    view->SetFont(font->platformData().font());

    GlyphBufferGlyph* glyphs = const_cast<GlyphBufferGlyph*>(glyphBuffer.glyphs(from));
    CString converted = UTF8Encoding().encode((const UChar*)glyphs, numGlyphs, URLEncodedEntitiesForUnencodables);
	BPoint offsets[numGlyphs];
	float offset = point.x();
    for (int i = 0; i < numGlyphs; i++) {
        offsets[i].x = offset;
        offsets[i].y = point.y();
        offset += glyphBuffer.advanceAt(from + i).width();
    }

    view->DrawString(converted.data(), converted.length(), offsets, numGlyphs);
    view->PopState();
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

/* the "complex" text is used for text-rendering: optimizeLegibility, and other
 * cases such as SVG text. Other platforms use HarfBuzz for layouting the text,
 * but we should handle this all on BView size and make use of
 * ICU LayoutEngine - once properly integrated in the BeAPI, that is.
 *
 * For now, we just call the usual DrawString method. It's better to at least
 * try displaying something.
 */
float Font::drawComplexText(GraphicsContext* ctx, const TextRun& run, const FloatPoint& point,
                           int from, int to) const
{
    BView* view = ctx->platformContext();
    view->SetFont(primaryFont()->platformData().font());
    view->DrawString(run.string().utf8().data() + from, to - from + 1, point);

    return view->StringWidth(run.string().utf8().data() + from, to - from + 1);
}


float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    const BFont* font = primaryFont()->platformData().font();
    ASSERT(font);
    float width = font->StringWidth(run.string().utf8().data());
    return width;
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint&, int, int, int) const
{
    notImplemented();
    return FloatRect();
}

int Font::offsetForPositionForComplexText(const TextRun& run, float, bool) const
{
    notImplemented();
    return 0;
}

} // namespace WebCore

