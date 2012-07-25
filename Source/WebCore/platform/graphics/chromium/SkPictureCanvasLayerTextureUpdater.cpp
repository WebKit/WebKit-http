/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING)

#include "SkPictureCanvasLayerTextureUpdater.h"

#include "LayerPainterChromium.h"
#include "SkCanvas.h"
#include "TraceEvent.h"

namespace WebCore {

SkPictureCanvasLayerTextureUpdater::SkPictureCanvasLayerTextureUpdater(PassOwnPtr<LayerPainterChromium> painter)
    : CanvasLayerTextureUpdater(painter)
    , m_layerIsOpaque(false)
{
}

SkPictureCanvasLayerTextureUpdater::~SkPictureCanvasLayerTextureUpdater()
{
}

void SkPictureCanvasLayerTextureUpdater::prepareToUpdate(const IntRect& contentRect, const IntSize&, float contentsWidthScale, float contentsHeightScale, IntRect& resultingOpaqueRect, CCRenderingStats& stats)
{
    SkCanvas* canvas = m_picture.beginRecording(contentRect.width(), contentRect.height());
    paintContents(canvas, contentRect, contentsWidthScale, contentsHeightScale, resultingOpaqueRect, stats);
    m_picture.endRecording();
}

void SkPictureCanvasLayerTextureUpdater::drawPicture(SkCanvas* canvas)
{
    TRACE_EVENT0("cc", "SkPictureCanvasLayerTextureUpdater::drawPicture");
    canvas->drawPicture(m_picture);
}

void SkPictureCanvasLayerTextureUpdater::setOpaque(bool opaque)
{
    m_layerIsOpaque = opaque;
}

} // namespace WebCore
#endif // USE(ACCELERATED_COMPOSITING)
