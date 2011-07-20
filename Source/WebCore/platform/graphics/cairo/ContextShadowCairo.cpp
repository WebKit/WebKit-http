/*
 * Copyright (C) 2010 Sencha, Inc.
 * Copyright (C) 2010 Igalia S.L.
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
#include "ContextShadow.h"

#include "AffineTransform.h"
#include "CairoUtilities.h"
#include "GraphicsContext.h"
#include "OwnPtrCairo.h"
#include "Path.h"
#include "PlatformContextCairo.h"
#include "Timer.h"
#include <cairo.h>
#include <wtf/StdLibExtras.h>

using WTF::max;

namespace WebCore {

static RefPtr<cairo_surface_t> gScratchBuffer;
static void purgeScratchBuffer()
{
    gScratchBuffer.clear();
}

// ContextShadow needs a scratch image as the buffer for the blur filter.
// Instead of creating and destroying the buffer for every operation,
// we create a buffer which will be automatically purged via a timer.
class PurgeScratchBufferTimer : public TimerBase {
private:
    virtual void fired() { purgeScratchBuffer(); }
};

static void scheduleScratchBufferPurge()
{
    DEFINE_STATIC_LOCAL(PurgeScratchBufferTimer, purgeScratchBufferTimer, ());
    if (purgeScratchBufferTimer.isActive())
        purgeScratchBufferTimer.stop();
    purgeScratchBufferTimer.startOneShot(2);
}

static cairo_surface_t* getScratchBuffer(const IntSize& size)
{
    int width = size.width();
    int height = size.height();
    int scratchWidth = gScratchBuffer.get() ? cairo_image_surface_get_width(gScratchBuffer.get()) : 0;
    int scratchHeight = gScratchBuffer.get() ? cairo_image_surface_get_height(gScratchBuffer.get()) : 0;

    // We do not need to recreate the buffer if the current buffer is large enough.
    if (gScratchBuffer.get() && scratchWidth >= width && scratchHeight >= height)
        return gScratchBuffer.get();

    purgeScratchBuffer();

    // Round to the nearest 32 pixels so we do not grow the buffer for similar sized requests.
    width = (1 + (width >> 5)) << 5;
    height = (1 + (height >> 5)) << 5;
    gScratchBuffer = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height));
    return gScratchBuffer.get();
}

PlatformContext ContextShadow::beginShadowLayer(GraphicsContext* context, const FloatRect& layerArea)
{
    adjustBlurDistance(context);

    double x1, x2, y1, y2;
    cairo_clip_extents(context->platformContext()->cr(), &x1, &y1, &x2, &y2);
    IntRect layerRect = calculateLayerBoundingRect(context, layerArea, IntRect(x1, y1, x2 - x1, y2 - y1));

    // Don't paint if we are totally outside the clip region.
    if (layerRect.isEmpty())
        return 0;

    m_layerImage = getScratchBuffer(layerRect.size());
    m_layerContext = cairo_create(m_layerImage);

    // Always clear the surface first.
    cairo_set_operator(m_layerContext, CAIRO_OPERATOR_CLEAR);
    cairo_paint(m_layerContext);
    cairo_set_operator(m_layerContext, CAIRO_OPERATOR_OVER);

    cairo_translate(m_layerContext, m_layerContextTranslation.x(), m_layerContextTranslation.y());
    return m_layerContext;
}

void ContextShadow::endShadowLayer(GraphicsContext* context)
{
    cairo_destroy(m_layerContext);
    m_layerContext = 0;

    if (m_type == BlurShadow) {
        cairo_surface_flush(m_layerImage);
        blurLayerImage(cairo_image_surface_get_data(m_layerImage),
                       IntSize(cairo_image_surface_get_width(m_layerImage), cairo_image_surface_get_height(m_layerImage)),
                       cairo_image_surface_get_stride(m_layerImage));
        cairo_surface_mark_dirty(m_layerImage);
    }

    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);
    setSourceRGBAFromColor(cr, m_color);
    cairo_mask_surface(cr, m_layerImage, m_layerOrigin.x(), m_layerOrigin.y());
    cairo_restore(cr);

    // Schedule a purge of the scratch buffer. We do not need to destroy the surface.
    scheduleScratchBufferPurge();
}

void ContextShadow::drawRectShadowWithoutTiling(GraphicsContext* context, const IntRect& shadowRect, const IntSize& topLeftRadius, const IntSize& topRightRadius, const IntSize& bottomLeftRadius, const IntSize& bottomRightRadius, float alpha)
{
    beginShadowLayer(context, shadowRect);

    if (!m_layerContext)
        return;

    Path path;
    path.addRoundedRect(shadowRect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius);

    appendWebCorePathToCairoContext(m_layerContext, path);
    cairo_set_source_rgba(m_layerContext, 0, 0, 0, alpha);
    cairo_fill(m_layerContext);

    endShadowLayer(context);
}

static inline FloatPoint getPhase(const FloatRect& dest, const FloatRect& tile)
{
    FloatPoint phase = dest.location();
    phase.move(-tile.x(), -tile.y());

    return phase;
}

/*
  This function uses tiling to improve the performance of the shadow
  drawing of rounded rectangles. The code basically does the following
  steps:

     1. Calculate the size of the shadow template, a rectangle that
     contains all the necessary tiles to draw the complete shadow.

     2. If that size is smaller than the real rectangle render the new
     template rectangle and its shadow in a new surface, in other case
     render the shadow of the real rectangle in the destination
     surface.

     3. Calculate the sizes and positions of the tiles and their
     destinations and use drawPattern to render the final shadow. The
     code divides the rendering in 8 tiles:

        1 | 2 | 3
       -----------
        4 |   | 5
       -----------
        6 | 7 | 8

     The corners are directly copied from the template rectangle to the
     real one and the side tiles are 1 pixel width, we use them as

     tiles to cover the destination side. The corner tiles are bigger
     than just the side of the rounded corner, we need to increase it
     because the modifications caused by the corner over the blur
     effect. We fill the central part with solid color to complete the
     shadow.
 */
void ContextShadow::drawRectShadow(GraphicsContext* context, const IntRect& rect, const IntSize& topLeftRadius, const IntSize& topRightRadius, const IntSize& bottomLeftRadius, const IntSize& bottomRightRadius)
{

    float radiusTwice = m_blurDistance * 2;

    // Find the space the corners need inside the rect for its shadows.
    int internalShadowWidth = radiusTwice + max(topLeftRadius.width(), bottomLeftRadius.width()) +
        max(topRightRadius.width(), bottomRightRadius.width());
    int internalShadowHeight = radiusTwice + max(topLeftRadius.height(), topRightRadius.height()) +
        max(bottomLeftRadius.height(), bottomRightRadius.height());

    cairo_t* cr = context->platformContext()->cr();
    float globalAlpha = context->platformContext()->globalAlpha();

    // drawShadowedRect still does not work with rotations.
    // https://bugs.webkit.org/show_bug.cgi?id=45042
    if ((!context->getCTM().isIdentityOrTranslationOrFlipped()) || (internalShadowWidth > rect.width())
        || (internalShadowHeight > rect.height()) || (m_type != BlurShadow)) {
        drawRectShadowWithoutTiling(context, rect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius, globalAlpha);
        return;
    }

    // Calculate size of the template shadow buffer.
    IntSize shadowBufferSize = IntSize(rect.width() + radiusTwice, rect.height() + radiusTwice);

    // Determine dimensions of shadow rect.
    FloatRect shadowRect = FloatRect(rect.location(), shadowBufferSize);
    shadowRect.move(- m_blurDistance, - m_blurDistance);

    // Size of the tiling side.
    int sideTileWidth = 1;

    // The length of a side of the buffer is the enough space for four blur radii,
    // the radii of the corners, and then 1 pixel to draw the side tiles.
    IntSize shadowTemplateSize = IntSize(sideTileWidth + radiusTwice + internalShadowWidth,
                                         sideTileWidth + radiusTwice + internalShadowHeight);

    // Reduce the size of what we have to draw with the clip area.
    double x1, x2, y1, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    calculateLayerBoundingRect(context, shadowRect, IntRect(x1, y1, x2 - x1, y2 - y1));

    if ((shadowTemplateSize.width() * shadowTemplateSize.height() > m_sourceRect.width() * m_sourceRect.height())) {
        drawRectShadowWithoutTiling(context, rect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius, globalAlpha);
        return;
    }

    shadowRect.move(m_offset.width(), m_offset.height());

    m_layerImage = getScratchBuffer(shadowTemplateSize);

    // Draw shadow into a new ImageBuffer.
    m_layerContext = cairo_create(m_layerImage);

    // Clear the surface first.
    cairo_set_operator(m_layerContext, CAIRO_OPERATOR_CLEAR);
    cairo_paint(m_layerContext);
    cairo_set_operator(m_layerContext, CAIRO_OPERATOR_OVER);

    // Draw the rectangle.
    IntRect templateRect = IntRect(m_blurDistance, m_blurDistance, shadowTemplateSize.width() - radiusTwice, shadowTemplateSize.height() - radiusTwice);
    Path path;
    path.addRoundedRect(templateRect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius);
    appendWebCorePathToCairoContext(m_layerContext, path);

    cairo_set_source_rgba(m_layerContext, 0, 0, 0, globalAlpha);
    cairo_fill(m_layerContext);

    // Blur the image.
    cairo_surface_flush(m_layerImage);
    blurLayerImage(cairo_image_surface_get_data(m_layerImage), shadowTemplateSize, cairo_image_surface_get_stride(m_layerImage));
    cairo_surface_mark_dirty(m_layerImage);

    // Mask the image with the shadow color.
    cairo_set_operator(m_layerContext, CAIRO_OPERATOR_IN);
    setSourceRGBAFromColor(m_layerContext, m_color);
    cairo_paint(m_layerContext);

    cairo_destroy(m_layerContext);
    m_layerContext = 0;

    // Fill the internal part of the shadow.
    shadowRect.inflate(-radiusTwice);
    if (!shadowRect.isEmpty()) {
        cairo_save(cr);
        path.clear();
        path.addRoundedRect(shadowRect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius);
        appendWebCorePathToCairoContext(cr, path);
        setSourceRGBAFromColor(cr, m_color);
        cairo_fill(cr);
        cairo_restore(cr);
    }
    shadowRect.inflate(radiusTwice);

    // Draw top side.
    FloatRect tileRect = FloatRect(radiusTwice + topLeftRadius.width(), 0, sideTileWidth, radiusTwice);
    FloatRect destRect = tileRect;
    destRect.move(shadowRect.x(), shadowRect.y());
    destRect.setWidth(shadowRect.width() - topLeftRadius.width() - topRightRadius.width() - m_blurDistance * 4);
    FloatPoint phase = getPhase(destRect, tileRect);
    AffineTransform patternTransform;
    patternTransform.makeIdentity();
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Draw the bottom side.
    tileRect = FloatRect(radiusTwice + bottomLeftRadius.width(), shadowTemplateSize.height() - radiusTwice, sideTileWidth, radiusTwice);
    destRect = tileRect;
    destRect.move(shadowRect.x(), shadowRect.y() + radiusTwice + rect.height() - shadowTemplateSize.height());
    destRect.setWidth(shadowRect.width() - bottomLeftRadius.width() - bottomRightRadius.width() - m_blurDistance * 4);
    phase = getPhase(destRect, tileRect);
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Draw the right side.
    tileRect = FloatRect(shadowTemplateSize.width() - radiusTwice, radiusTwice + topRightRadius.height(), radiusTwice, sideTileWidth);
    destRect = tileRect;
    destRect.move(shadowRect.x() + radiusTwice + rect.width() - shadowTemplateSize.width(), shadowRect.y());
    destRect.setHeight(shadowRect.height() - topRightRadius.height() - bottomRightRadius.height() - m_blurDistance * 4);
    phase = getPhase(destRect, tileRect);
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Draw the left side.
    tileRect = FloatRect(0, radiusTwice + topLeftRadius.height(), radiusTwice, sideTileWidth);
    destRect = tileRect;
    destRect.move(shadowRect.x(), shadowRect.y());
    destRect.setHeight(shadowRect.height() - topLeftRadius.height() - bottomLeftRadius.height() - m_blurDistance * 4);
    phase = FloatPoint(destRect.x() - tileRect.x(), destRect.y() - tileRect.y());
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Draw the top left corner.
    tileRect = FloatRect(0, 0, radiusTwice + topLeftRadius.width(), radiusTwice + topLeftRadius.height());
    destRect = tileRect;
    destRect.move(shadowRect.x(), shadowRect.y());
    phase = getPhase(destRect, tileRect);
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Draw the top right corner.
    tileRect = FloatRect(shadowTemplateSize.width() - radiusTwice - topRightRadius.width(), 0, radiusTwice + topRightRadius.width(),
                         radiusTwice + topRightRadius.height());
    destRect = tileRect;
    destRect.move(shadowRect.x() + rect.width() - shadowTemplateSize.width() + radiusTwice, shadowRect.y());
    phase = getPhase(destRect, tileRect);
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Draw the bottom right corner.
    tileRect = FloatRect(shadowTemplateSize.width() - radiusTwice - bottomRightRadius.width(),
                         shadowTemplateSize.height() - radiusTwice - bottomRightRadius.height(),
                         radiusTwice + bottomRightRadius.width(), radiusTwice + bottomRightRadius.height());
    destRect = tileRect;
    destRect.move(shadowRect.x() + rect.width() - shadowTemplateSize.width() + radiusTwice,
                  shadowRect.y() + rect.height() - shadowTemplateSize.height() + radiusTwice);
    phase = getPhase(destRect, tileRect);
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Draw the bottom left corner.
    tileRect = FloatRect(0, shadowTemplateSize.height() - radiusTwice - bottomLeftRadius.height(),
                         radiusTwice + bottomLeftRadius.width(), radiusTwice + bottomLeftRadius.height());
    destRect = tileRect;
    destRect.move(shadowRect.x(), shadowRect.y() + rect.height() - shadowTemplateSize.height() + radiusTwice);
    phase = getPhase(destRect, tileRect);
    drawPatternToCairoContext(cr, m_layerImage, shadowTemplateSize, tileRect, patternTransform, phase, CAIRO_OPERATOR_OVER, destRect);

    // Schedule a purge of the scratch buffer.
    scheduleScratchBufferPurge();
}

}
