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

#include "FontCascade.h"
#include "FontDescription.h"
#include "FontSelector.h"
#include "GraphicsContext.h"
#include "TextEncoding.h"
#include "NotImplemented.h"
#include <wtf/text/CString.h>
#include <Font.h>
#include <String.h>
#include <UnicodeChar.h>
#include <View.h>


namespace WebCore {

bool FontCascade::canReturnFallbackFontsForComplexText()
{
    return false;
}

void FontCascade::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    notImplemented();
}

void FontCascade::drawGlyphs(GraphicsContext* graphicsContext, const Font* font,
                      const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point) const
{
    BView* view = graphicsContext->platformContext();
    view->PushState();

    rgb_color color = graphicsContext->fillColor();

    if (color.alpha < 255 || graphicsContext->isInTransparencyLayer())
        view->SetDrawingMode(B_OP_ALPHA);
    else
        view->SetDrawingMode(B_OP_OVER);
    view->SetHighColor(color);
    view->SetFont(font->platformData().font());

    const GlyphBufferGlyph* glyphs = glyphBuffer.glyphs(from);


	BPoint offsets[numGlyphs];
    char buffer[4];
    BString utf8;
	float offset = point.x();
    for (int i = 0; i < numGlyphs; i++) {
        offsets[i].x = offset;
        offsets[i].y = point.y();
        offset += glyphBuffer.advanceAt(from + i).width();

        char* tmp = buffer;
        BUnicodeChar::ToUTF8(glyphs[i], &tmp);
        utf8.Append(buffer, tmp - buffer);
    }

    view->DrawString(utf8, offsets, numGlyphs);
    view->PopState();
}

bool FontCascade::canExpandAroundIdeographsInComplexText()
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
float FontCascade::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point,
                           int from, int to) const
{
    BView* view = context->platformContext();
    view->SetFont(primaryFont().platformData().font());

    const char* string = run.subRun(from,to).string().utf8().data();
    view->DrawString(string, point);

    return view->StringWidth(string);
}


float FontCascade::floatWidthForComplexText(const TextRun& run, HashSet<const Font*>* /*fallbackFonts*/, GlyphOverflow* /*glyphOverflow*/) const
{
    const BFont* font = primaryFont().platformData().font();
    ASSERT(font);
    float width = font->StringWidth(run.string().utf8().data());
    return width;
}

int FontCascade::offsetForPositionForComplexText(const TextRun& run, float position,
    bool includePartialGlyphs) const
{
    return offsetForPositionForSimpleText(run, position, includePartialGlyphs);
}

void FontCascade::adjustSelectionRectForComplexText(const TextRun& run,
    LayoutRect& rect, int from, int to) const
{
    adjustSelectionRectForSimpleText(run, rect, from, to);
}

} // namespace WebCore

