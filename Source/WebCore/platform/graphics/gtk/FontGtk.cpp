/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (c) 2007 Hiroyuki Ikezoe
 * Copyright (c) 2007 Kouhei Sutou
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 * Copyright (C) 2008 Nuanti Ltd.
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

#include "CairoUtilities.h"
#include "GOwnPtr.h"
#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "PlatformContextCairo.h"
#include "ShadowBlur.h"
#include "SimpleFontData.h"
#include "TextRun.h"
#include <cairo.h>
#include <gdk/gdk.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#if USE(FREETYPE)
#include <pango/pangofc-fontmap.h>
#endif

namespace WebCore {

#ifdef GTK_API_VERSION_2
typedef GdkRegion* PangoRegionType;

void destroyPangoRegion(PangoRegionType region)
{
    gdk_region_destroy(region);
}

IntRect getPangoRegionExtents(PangoRegionType region)
{
    GdkRectangle rectangle;
    gdk_region_get_clipbox(region, &rectangle);
    return IntRect(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}
#else
typedef cairo_region_t* PangoRegionType;

void destroyPangoRegion(PangoRegionType region)
{
    cairo_region_destroy(region);
}

IntRect getPangoRegionExtents(PangoRegionType region)
{
    cairo_rectangle_int_t rectangle;
    cairo_region_get_extents(region, &rectangle);
    return IntRect(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
}
#endif

#define IS_HIGH_SURROGATE(u)  ((UChar)(u) >= (UChar)0xd800 && (UChar)(u) <= (UChar)0xdbff)
#define IS_LOW_SURROGATE(u)  ((UChar)(u) >= (UChar)0xdc00 && (UChar)(u) <= (UChar)0xdfff)

static gchar* utf16ToUtf8(const UChar* aText, gint aLength, gint &length)
{
    gboolean needCopy = FALSE;

    for (int i = 0; i < aLength; i++) {
        if (!aText[i] || IS_LOW_SURROGATE(aText[i])) {
            needCopy = TRUE;
            break;
        } 

        if (IS_HIGH_SURROGATE(aText[i])) {
            if (i < aLength - 1 && IS_LOW_SURROGATE(aText[i+1]))
                i++;
            else {
                needCopy = TRUE;
                break;
            }
        }
    }

    GOwnPtr<UChar> copiedString;
    if (needCopy) {
        /* Pango doesn't correctly handle nuls.  We convert them to 0xff. */
        /* Also "validate" UTF-16 text to make sure conversion doesn't fail. */

        copiedString.set(static_cast<UChar*>(g_memdup(aText, aLength * sizeof(aText[0]))));
        UChar* p = copiedString.get();

        /* don't need to reset i */
        for (int i = 0; i < aLength; i++) {
            if (!p[i] || IS_LOW_SURROGATE(p[i]))
                p[i] = 0xFFFD;
            else if (IS_HIGH_SURROGATE(p[i])) {
                if (i < aLength - 1 && IS_LOW_SURROGATE(aText[i+1]))
                    i++;
                else
                    p[i] = 0xFFFD;
            }
        }

        aText = p;
    }

    gchar* utf8Text;
    glong itemsWritten;
    utf8Text = g_utf16_to_utf8(static_cast<const gunichar2*>(aText), aLength, 0, &itemsWritten, 0);
    length = itemsWritten;

    return utf8Text;
}

static gchar* convertUniCharToUTF8(const UChar* characters, gint length, int from, int to)
{
    gint newLength = 0;
    GOwnPtr<gchar> utf8Text(utf16ToUtf8(characters, length, newLength));
    if (!utf8Text)
        return 0;

    gchar* pos = utf8Text.get();
    if (from > 0) {
        // discard the first 'from' characters
        // FIXME: we should do this before the conversion probably
        pos = g_utf8_offset_to_pointer(utf8Text.get(), from);
    }

    gint len = strlen(pos);
    GString* ret = g_string_new_len(NULL, len);

    // replace line break by space
    while (len > 0) {
        gint index, start;
        pango_find_paragraph_boundary(pos, len, &index, &start);
        g_string_append_len(ret, pos, index);
        if (index == start)
            break;
        g_string_append_c(ret, ' ');
        pos += start;
        len -= start;
    }
    return g_string_free(ret, FALSE);
}

static void setPangoAttributes(const Font* font, const TextRun& run, PangoLayout* layout)
{
#if USE(FREETYPE)
    if (font->primaryFont()->platformData().m_pattern) {
        PangoFontDescription* desc = pango_fc_font_description_from_pattern(font->primaryFont()->platformData().m_pattern.get(), FALSE);
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);
    }
#elif USE(PANGO)
    if (font->primaryFont()->platformData().m_font) {
        PangoFontDescription* desc = pango_font_describe(font->primaryFont()->platformData().m_font);
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);
    }
#endif

    pango_layout_set_auto_dir(layout, FALSE);

    PangoContext* pangoContext = pango_layout_get_context(layout);
    PangoDirection direction = run.rtl() ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
    pango_context_set_base_dir(pangoContext, direction);
    PangoAttrList* list = pango_attr_list_new();
    PangoAttribute* attr;

    attr = pango_attr_size_new_absolute(font->pixelSize() * PANGO_SCALE);
    attr->end_index = G_MAXUINT;
    pango_attr_list_insert_before(list, attr);

    if (!run.spacingDisabled()) {
        attr = pango_attr_letter_spacing_new(font->letterSpacing() * PANGO_SCALE);
        attr->end_index = G_MAXUINT;
        pango_attr_list_insert_before(list, attr);
    }

    // Pango does not yet support synthesising small caps
    // See http://bugs.webkit.org/show_bug.cgi?id=15610

    pango_layout_set_attributes(layout, list);
    pango_attr_list_unref(list);
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

static void drawGlyphsShadow(GraphicsContext* graphicsContext, const FloatPoint& point, PangoLayoutLine* layoutLine, PangoRegionType renderRegion)
{
    ShadowBlur& shadow = graphicsContext->platformContext()->shadowBlur();

    if (!(graphicsContext->textDrawingMode() & TextModeFill) || shadow.type() == ShadowBlur::NoShadow)
        return;

    FloatPoint totalOffset(point + graphicsContext->state().shadowOffset);

    // Optimize non-blurry shadows, by just drawing text without the ShadowBlur.
    if (!shadow.mustUseShadowBlur(graphicsContext)) {
        cairo_t* context = graphicsContext->platformContext()->cr();
        cairo_save(context);
        cairo_translate(context, totalOffset.x(), totalOffset.y());

        setSourceRGBAFromColor(context, graphicsContext->state().shadowColor);
        gdk_cairo_region(context, renderRegion);
        cairo_clip(context);
        pango_cairo_show_layout_line(context, layoutLine);

        cairo_restore(context);
        return;
    }

    FloatRect extents(getPangoRegionExtents(renderRegion));
    extents.setLocation(FloatPoint(point.x(), point.y() - extents.height()));
    if (GraphicsContext* shadowContext = shadow.beginShadowLayer(graphicsContext, extents)) {
        cairo_t* cairoShadowContext = shadowContext->platformContext()->cr();
        cairo_translate(cairoShadowContext, point.x(), point.y());
        pango_cairo_show_layout_line(cairoShadowContext, layoutLine);

        // We need the clipping region to be active when we blit the blurred shadow back,
        // because we don't want any bits and pieces of characters out of range to be
        // drawn. Since ShadowBlur expects a consistent transform, we have to undo the
        // translation before calling endShadowLayer as well.
        cairo_t* context = graphicsContext->platformContext()->cr();
        cairo_save(context);
        cairo_translate(context, totalOffset.x(), totalOffset.y());
        gdk_cairo_region(context, renderRegion);
        cairo_clip(context);
        cairo_translate(context, -totalOffset.x(), -totalOffset.y());

        shadow.endShadowLayer(graphicsContext);
        cairo_restore(context);
    }
}

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
#if USE(FREETYPE)
    if (!primaryFont()->platformData().m_pattern) {
        drawSimpleText(context, run, point, from, to);
        return;
    }
#endif

    cairo_t* cr = context->platformContext()->cr();
    PangoLayout* layout = pango_cairo_create_layout(cr);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    // Our layouts are single line
    PangoLayoutLine* layoutLine = pango_layout_get_line_readonly(layout, 0);

    // Get the region where this text will be laid out. We will use it to clip
    // the Cairo context, for when we are only painting part of the text run and
    // to calculate the size of the shadow buffer.
    PangoRegionType partialRegion = 0;
    char* start = g_utf8_offset_to_pointer(utf8, from);
    char* end = g_utf8_offset_to_pointer(start, to - from);
    int ranges[] = {start - utf8, end - utf8};
    partialRegion = gdk_pango_layout_line_get_clip_region(layoutLine, 0, 0, ranges, 1);

    drawGlyphsShadow(context, point, layoutLine, partialRegion);

    cairo_save(cr);
    cairo_translate(cr, point.x(), point.y());

    float red, green, blue, alpha;
    context->fillColor().getRGBA(red, green, blue, alpha);
    cairo_set_source_rgba(cr, red, green, blue, alpha);
    gdk_cairo_region(cr, partialRegion);
    cairo_clip(cr);

    pango_cairo_show_layout_line(cr, layoutLine);

    if (context->textDrawingMode() & TextModeStroke) {
        Color strokeColor = context->strokeColor();
        strokeColor.getRGBA(red, green, blue, alpha);
        cairo_set_source_rgba(cr, red, green, blue, alpha);
        pango_cairo_layout_line_path(cr, layoutLine);
        cairo_set_line_width(cr, context->strokeThickness());
        cairo_stroke(cr);
    }

    // Pango sometimes leaves behind paths we don't want
    cairo_new_path(cr);

    destroyPangoRegion(partialRegion);
    g_free(utf8);
    g_object_unref(layout);

    cairo_restore(cr);
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    notImplemented();
}

// We should create the layout with our actual context but we can't access it from here.
static PangoLayout* getDefaultPangoLayout(const TextRun& run)
{
    static PangoFontMap* map = pango_cairo_font_map_get_default();
#if PANGO_VERSION_CHECK(1,21,5)
    static PangoContext* pangoContext = pango_font_map_create_context(map);
#else
    // Deprecated in Pango 1.21.
    static PangoContext* pangoContext = pango_cairo_font_map_create_context(PANGO_CAIRO_FONT_MAP(map));
#endif
    PangoLayout* layout = pango_layout_new(pangoContext);

    return layout;
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* overflow) const
{
#if USE(FREETYPE)
    if (!primaryFont()->platformData().m_pattern)
        return floatWidthForSimpleText(run, 0, fallbackFonts, overflow);
#endif

    if (run.length() == 0)
        return 0.0f;

    PangoLayout* layout = getDefaultPangoLayout(run);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    int width;
    pango_layout_get_pixel_size(layout, &width, 0);

    g_free(utf8);
    g_object_unref(layout);

    return width;
}

int Font::offsetForPositionForComplexText(const TextRun& run, float xFloat, bool includePartialGlyphs) const
{
#if USE(FREETYPE)
    if (!primaryFont()->platformData().m_pattern)
        return offsetForPositionForSimpleText(run, xFloat, includePartialGlyphs);
#endif
    // FIXME: This truncation is not a problem for HTML, but only affects SVG, which passes floating-point numbers
    // to Font::offsetForPosition(). Bug http://webkit.org/b/40673 tracks fixing this problem.
    int x = static_cast<int>(xFloat);

    PangoLayout* layout = getDefaultPangoLayout(run);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    int index, trailing;
    pango_layout_xy_to_index(layout, x * PANGO_SCALE, 1, &index, &trailing);
    glong offset = g_utf8_pointer_to_offset(utf8, utf8 + index);
    if (includePartialGlyphs)
        offset += trailing;

    g_free(utf8);
    g_object_unref(layout);

    return offset;
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& point, int h, int from, int to) const
{
#if USE(FREETYPE)
    if (!primaryFont()->platformData().m_pattern)
        return selectionRectForSimpleText(run, point, h, from, to);
#endif

    PangoLayout* layout = getDefaultPangoLayout(run);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    char* start = g_utf8_offset_to_pointer(utf8, from);
    char* end = g_utf8_offset_to_pointer(start, to - from);

    if (run.ltr()) {
        from = start - utf8;
        to = end - utf8;
    } else {
        from = end - utf8;
        to = start - utf8;
    }

    PangoLayoutLine* layoutLine = pango_layout_get_line_readonly(layout, 0);
    int x_pos;

    x_pos = 0;
    if (from < layoutLine->length)
        pango_layout_line_index_to_x(layoutLine, from, FALSE, &x_pos);
    float beforeWidth = PANGO_PIXELS_FLOOR(x_pos);

    x_pos = 0;
    if (run.ltr() || to < layoutLine->length)
        pango_layout_line_index_to_x(layoutLine, to, FALSE, &x_pos);
    float afterWidth = PANGO_PIXELS(x_pos);

    g_free(utf8);
    g_object_unref(layout);

    return FloatRect(point.x() + beforeWidth, point.y(), afterWidth - beforeWidth, h);
}

}
