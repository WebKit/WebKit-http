/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING)

#include "ContentLayerChromium.h"

#include "LayerPainterChromium.h"
#include "LayerRendererChromium.h"
#include "LayerTextureUpdaterCanvas.h"
#include "PlatformSupport.h"
#include "cc/CCLayerTreeHost.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

class ContentLayerPainter : public LayerPainterChromium {
    WTF_MAKE_NONCOPYABLE(ContentLayerPainter);
public:
    static PassOwnPtr<ContentLayerPainter> create(CCLayerDelegate* delegate)
    {
        return adoptPtr(new ContentLayerPainter(delegate));
    }

    virtual void paint(GraphicsContext& context, const IntRect& contentRect)
    {
        double paintStart = currentTime();
        context.clearRect(contentRect);
        context.clip(contentRect);
        m_delegate->paintContents(context, contentRect);
        double paintEnd = currentTime();
        double pixelsPerSec = (contentRect.width() * contentRect.height()) / (paintEnd - paintStart);
        PlatformSupport::histogramCustomCounts("Renderer4.AccelContentPaintDurationMS", (paintEnd - paintStart) * 1000, 0, 120, 30);
        PlatformSupport::histogramCustomCounts("Renderer4.AccelContentPaintMegapixPerSecond", pixelsPerSec / 1000000, 10, 210, 30);
    }
private:
    explicit ContentLayerPainter(CCLayerDelegate* delegate)
        : m_delegate(delegate)
    {
    }

    CCLayerDelegate* m_delegate;
};

PassRefPtr<ContentLayerChromium> ContentLayerChromium::create(CCLayerDelegate* delegate)
{
    return adoptRef(new ContentLayerChromium(delegate));
}

ContentLayerChromium::ContentLayerChromium(CCLayerDelegate* delegate)
    : TiledLayerChromium(delegate)
{
}

ContentLayerChromium::~ContentLayerChromium()
{
    cleanupResources();
}

void ContentLayerChromium::cleanupResources()
{
    m_textureUpdater.clear();
    TiledLayerChromium::cleanupResources();
}

void ContentLayerChromium::paintContentsIfDirty()
{
    ASSERT(drawsContent());

    updateTileSizeAndTilingOption();

    const IntRect& layerRect = visibleLayerRect();
    if (layerRect.isEmpty())
        return;

    IntRect dirty = enclosingIntRect(m_dirtyRect);
    dirty.intersect(IntRect(IntPoint(), contentBounds()));
    invalidateRect(dirty);

    if (!drawsContent())
        return;

    prepareToUpdate(layerRect);
    resetNeedsDisplay();
}

bool ContentLayerChromium::drawsContent() const
{
    return m_delegate && m_delegate->drawsContent() && TiledLayerChromium::drawsContent();
}

void ContentLayerChromium::createTextureUpdater(const CCLayerTreeHost* host)
{
#if USE(SKIA) && USE(ACCELERATED_DRAWING)
    // Note that host->skiaContext() will crash if called while in threaded
    // mode. This thus depends on CCLayerTreeHost::initialize turning off
    // acceleratePainting to prevent this from crashing.
    if (host->settings().acceleratePainting) {
        m_textureUpdater = LayerTextureUpdaterSkPicture::create(ContentLayerPainter::create(m_delegate));
        return;
    }
#endif // USE(SKIA) && USE(ACCELERATED_DRAWING)

    m_textureUpdater = LayerTextureUpdaterBitmap::create(ContentLayerPainter::create(m_delegate), host->layerRendererCapabilities().usingMapSub);
}

}
#endif // USE(ACCELERATED_COMPOSITING)
