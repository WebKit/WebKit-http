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

#include "Canvas2DLayerChromium.h"

#include "DrawingBuffer.h"
#include "Extensions3DChromium.h"
#include "GraphicsContext3D.h"
#include "LayerRendererChromium.h"

namespace WebCore {

PassRefPtr<Canvas2DLayerChromium> Canvas2DLayerChromium::create(DrawingBuffer* drawingBuffer, GraphicsLayerChromium* owner)
{
    return adoptRef(new Canvas2DLayerChromium(drawingBuffer, owner));
}

Canvas2DLayerChromium::Canvas2DLayerChromium(DrawingBuffer* drawingBuffer, GraphicsLayerChromium* owner)
    : CanvasLayerChromium(owner)
    , m_drawingBuffer(drawingBuffer)
{
}

Canvas2DLayerChromium::~Canvas2DLayerChromium()
{
    if (m_drawingBuffer && layerRenderer())
        layerRenderer()->removeChildContext(m_drawingBuffer->graphicsContext3D().get());
}

bool Canvas2DLayerChromium::drawsContent() const
{
    GraphicsContext3D* context;
    return (m_drawingBuffer
            && (context = m_drawingBuffer->graphicsContext3D().get())
            && (context->getExtensions()->getGraphicsResetStatusARB() == GraphicsContext3D::NO_ERROR));
}

void Canvas2DLayerChromium::updateCompositorResources()
{
    if (!m_contentsDirty || !drawsContent())
        return;
    // Update the contents of the texture used by the compositor.
    if (m_contentsDirty) {
        m_drawingBuffer->publishToPlatformLayer();
        m_contentsDirty = false;
    }
}

unsigned Canvas2DLayerChromium::textureId() const
{
    return m_drawingBuffer ? m_drawingBuffer->platformColorBuffer() : 0;
}

void Canvas2DLayerChromium::setDrawingBuffer(DrawingBuffer* drawingBuffer)
{
    if (drawingBuffer != m_drawingBuffer) {
        if (m_drawingBuffer && layerRenderer())
            layerRenderer()->removeChildContext(m_drawingBuffer->graphicsContext3D().get());

        m_drawingBuffer = drawingBuffer;

        if (drawingBuffer && layerRenderer())
            layerRenderer()->addChildContext(m_drawingBuffer->graphicsContext3D().get());
    }
}

void Canvas2DLayerChromium::setLayerRenderer(LayerRendererChromium* newLayerRenderer)
{
    if (layerRenderer() != newLayerRenderer && m_drawingBuffer) {
        if (m_drawingBuffer->graphicsContext3D()) {
            if (layerRenderer())
                layerRenderer()->removeChildContext(m_drawingBuffer->graphicsContext3D().get());
            if (newLayerRenderer)
                newLayerRenderer->addChildContext(m_drawingBuffer->graphicsContext3D().get());
        }

        LayerChromium::setLayerRenderer(newLayerRenderer);
    }
}

}
#endif // USE(ACCELERATED_COMPOSITING)
