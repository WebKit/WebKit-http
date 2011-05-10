/*
 * Copyright (C) 2011 Igalia S.L.
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
#include "PlatformContextCairo.h"

#include "GraphicsContext.h"
#include <cairo.h>

namespace WebCore {

PlatformContextCairo::PlatformContextCairo(cairo_t* cr)
    : m_cr(cr)
{
}

void PlatformContextCairo::restore()
{
    const ImageMaskInformation& maskInformation = m_maskImageStack.last();
    if (maskInformation.isValid()) {
        const FloatRect& maskRect = maskInformation.maskRect();
        cairo_pop_group_to_source(m_cr.get());
        cairo_mask_surface(m_cr.get(), maskInformation.maskSurface(), maskRect.x(), maskRect.y());
    }
    m_maskImageStack.removeLast();

    cairo_restore(m_cr.get());
}

void PlatformContextCairo::save()
{
    m_maskImageStack.append(ImageMaskInformation());

    cairo_save(m_cr.get());
}

void PlatformContextCairo::pushImageMask(cairo_surface_t* surface, const FloatRect& rect)
{
    // We must call savePlatformState at least once before we can use image masking,
    // since we actually apply the mask in restorePlatformState.
    ASSERT(!m_maskImageStack.isEmpty());
    m_maskImageStack.last().update(surface, rect);

    // Cairo doesn't support the notion of an image clip, so we push a group here
    // and then paint it to the surface with an image mask (which is an immediate
    // operation) during restorePlatformState.

    // We want to allow the clipped elements to composite with the surface as it
    // is now, but they are isolated in another group. To make this work, we're
    // going to blit the current surface contents onto the new group once we push it.
    cairo_surface_t* currentTarget = cairo_get_target(m_cr.get());
    cairo_surface_flush(currentTarget);

    // Pushing a new group ensures that only things painted after this point are clipped.
    cairo_push_group(m_cr.get());
    cairo_set_operator(m_cr.get(), CAIRO_OPERATOR_SOURCE);

    cairo_set_source_surface(m_cr.get(), currentTarget, 0, 0);
    cairo_rectangle(m_cr.get(), rect.x(), rect.y(), rect.width(), rect.height());
    cairo_fill(m_cr.get());
}

static void drawPatternToCairoContext(cairo_t* cr, cairo_pattern_t* pattern, const FloatRect& destRect, float alpha)
{
    cairo_translate(cr, destRect.x(), destRect.y());
    cairo_set_source(cr, pattern);
    cairo_rectangle(cr, 0, 0, destRect.width(), destRect.height());
    cairo_clip(cr);
    cairo_paint_with_alpha(cr, alpha);
}

void PlatformContextCairo::drawSurfaceToContext(cairo_surface_t* surface, const FloatRect& destRect, const FloatRect& srcRect, GraphicsContext* context)
{
    // If we're drawing a sub portion of the image or scaling then create
    // a pattern transformation on the image and draw the transformed pattern.
    // Test using example site at http://www.meyerweb.com/eric/css/edge/complexspiral/demo.html
    RefPtr<cairo_pattern_t> pattern = adoptRef(cairo_pattern_create_for_surface(surface));
    cairo_pattern_set_extend(pattern.get(), CAIRO_EXTEND_PAD);

    float scaleX = srcRect.width() / destRect.width();
    float scaleY = srcRect.height() / destRect.height();
    cairo_matrix_t matrix = { scaleX, 0, 0, scaleY, srcRect.x(), srcRect.y() };
    cairo_pattern_set_matrix(pattern.get(), &matrix);

    ContextShadow* shadow = context->contextShadow();
    if (shadow && shadow->m_type != ContextShadow::NoShadow) {
        if (cairo_t* shadowContext = shadow->beginShadowLayer(context, destRect)) {
            drawPatternToCairoContext(shadowContext, pattern.get(), destRect, 1);
            shadow->endShadowLayer(context);
        }
    }

    cairo_save(m_cr.get());
    drawPatternToCairoContext(m_cr.get(), pattern.get(), destRect, context->getAlpha());
    cairo_restore(m_cr.get());
}


} // namespace WebCore
