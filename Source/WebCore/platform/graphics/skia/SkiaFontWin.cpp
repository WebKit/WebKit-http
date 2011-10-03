/*
 * Copyright (c) 2008, Google Inc. All rights reserved.
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
#include "SkiaFontWin.h"

#include "AffineTransform.h"
#include "PlatformContextSkia.h"
#include "Gradient.h"
#include "Pattern.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkShader.h"
#include "SkTemplates.h"
#include "SkTypeface_win.h"

namespace WebCore {

static void skiaDrawText(SkCanvas* canvas,
                         const SkPoint& point,
                         SkPaint* paint,
                         const WORD* glyphs,
                         const int* advances,
                         const GOFFSET* offsets,
                         int numGlyphs)
{
    // Reserve space for 64 SkPoints on the stack. If numGlyphs is larger, the array
    // will dynamically allocate it space for numGlyph glyphs. This is used to store
    // the computed x,y locations. In the case where offsets==null, then we use it
    // to store (twice as many) SkScalars for x[]
    static const size_t kLocalGlyphMax = 64;

    SkScalar x = point.fX;
    SkScalar y = point.fY;
    if (offsets) {
        SkAutoSTArray<kLocalGlyphMax, SkPoint> storage(numGlyphs);
        SkPoint* pos = storage.get();
        for (int i = 0; i < numGlyphs; i++) {
            // GDI has dv go up, so we negate it
            pos[i].set(x + SkIntToScalar(offsets[i].du),
                       y + -SkIntToScalar(offsets[i].dv));
            x += SkIntToScalar(advances[i]);
        }
        canvas->drawPosText(glyphs, numGlyphs * sizeof(uint16_t), pos, *paint);
    } else {
        SkAutoSTArray<kLocalGlyphMax * 2, SkScalar> storage(numGlyphs);
        SkScalar* xpos = storage.get();
        for (int i = 0; i < numGlyphs; i++) {
            xpos[i] = x;
            x += SkIntToScalar(advances[i]);
        }
        canvas->drawPosTextH(glyphs, numGlyphs * sizeof(uint16_t),
                             xpos, y, *paint);
    }
}

static bool isCanvasMultiLayered(SkCanvas* canvas)
{
    SkCanvas::LayerIter layerIterator(canvas, false);
    layerIterator.next();
    return !layerIterator.done();
}

// lifted from FontSkia.cpp
static bool disableTextLCD(PlatformContextSkia* skiaContext)
{
    // Our layers only have a single alpha channel. This means that subpixel
    // rendered text cannot be compositied correctly when the layer is
    // collapsed. Therefore, subpixel text is disabled when we are drawing
    // onto a layer or when the compositor is being used.
    return isCanvasMultiLayered(skiaContext->canvas())
           || skiaContext->isDrawingToImageBuffer();
}

// Lookup the current system settings for font smoothing.
// We cache these values for performance, but if the browser has away to be
// notified when these change, we could re-query them at that time.
static uint32_t getDefaultGDITextFlags()
{
    static bool gInited;
    static uint32_t gFlags;
    if (!gInited) {
        BOOL enabled;
        gFlags = 0;
        if (SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &enabled, 0) && enabled) {
            gFlags |= SkPaint::kAntiAlias_Flag;

            UINT smoothType;
            if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &smoothType, 0)) {
                if (FE_FONTSMOOTHINGCLEARTYPE == smoothType)
                    gFlags |= SkPaint::kLCDRenderText_Flag;
            }
        }
        gInited = true;
    }
    return gFlags;
}

static void setupPaintForFont(HFONT hfont, SkPaint* paint, PlatformContextSkia* pcs)
{
    //  FIXME:
    //  Much of this logic could also happen in
    //  FontCustomPlatformData::fontPlatformData and be cached,
    //  allowing us to avoid talking to GDI at this point.
    //
    LOGFONT info;
    GetObject(hfont, sizeof(info), &info);
    int size = info.lfHeight;
    if (size < 0)
        size = -size; // We don't let GDI dpi-scale us (see SkFontHost_win.cpp).
    paint->setTextSize(SkIntToScalar(size));

    SkTypeface* face = SkCreateTypefaceFromLOGFONT(info);
    paint->setTypeface(face);
    SkSafeUnref(face);

    // turn lfQuality into text flags
    uint32_t textFlags;
    switch (info.lfQuality) {
    case NONANTIALIASED_QUALITY:
        textFlags = 0;
        break;
    case ANTIALIASED_QUALITY:
        textFlags = SkPaint::kAntiAlias_Flag;
        break;
    case CLEARTYPE_QUALITY:
        textFlags = (SkPaint::kAntiAlias_Flag | SkPaint::kLCDRenderText_Flag);
        break;
    default:
        textFlags = getDefaultGDITextFlags();
        break;
    }
    // only allow features that SystemParametersInfo allows
    textFlags &= getDefaultGDITextFlags();

    // do this check after our switch on lfQuality
    if (disableTextLCD(pcs))
        textFlags &= ~SkPaint::kLCDRenderText_Flag;

    // now copy in just the text flags
    uint32_t flags = paint->getFlags();
    flags &= ~(SkPaint::kAntiAlias_Flag | SkPaint::kLCDRenderText_Flag);
    flags |= textFlags;
    paint->setFlags(flags);
}

void paintSkiaText(GraphicsContext* context,
                   HFONT hfont,
                   int numGlyphs,
                   const WORD* glyphs,
                   const int* advances,
                   const GOFFSET* offsets,
                   const SkPoint* origin)
{
    PlatformContextSkia* platformContext = context->platformContext();
    SkCanvas* canvas = platformContext->canvas();
    TextDrawingModeFlags textMode = platformContext->getTextDrawingMode();

    // Filling (if necessary). This is the common case.
    SkPaint paint;
    platformContext->setupPaintForFilling(&paint);
    paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    setupPaintForFont(hfont, &paint, platformContext);

    bool didFill = false;

    if ((textMode & TextModeFill) && (SkColorGetA(paint.getColor()) || paint.getLooper())) {
        skiaDrawText(canvas, *origin, &paint, &glyphs[0], &advances[0], &offsets[0], numGlyphs);
        didFill = true;
    }

    // Stroking on top (if necessary).
    if ((textMode & TextModeStroke)
        && platformContext->getStrokeStyle() != NoStroke
        && platformContext->getStrokeThickness() > 0) {

        paint.reset();
        platformContext->setupPaintForStroking(&paint, 0, 0);
        paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
        setupPaintForFont(hfont, &paint, platformContext);

        if (didFill) {
            // If there is a shadow and we filled above, there will already be
            // a shadow. We don't want to draw it again or it will be too dark
            // and it will go on top of the fill.
            //
            // Note that this isn't strictly correct, since the stroke could be
            // very thick and the shadow wouldn't account for this. The "right"
            // thing would be to draw to a new layer and then draw that layer
            // with a shadow. But this is a lot of extra work for something
            // that isn't normally an issue.
            paint.setLooper(0);
        }

        skiaDrawText(canvas, *origin, &paint, &glyphs[0], &advances[0], &offsets[0], numGlyphs);
    }
}

}  // namespace WebCore
