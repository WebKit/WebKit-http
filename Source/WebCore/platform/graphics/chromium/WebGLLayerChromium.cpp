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

#include "WebGLLayerChromium.h"

#include "Extensions3DChromium.h"
#include "GraphicsContext3D.h"
#include "LayerRendererChromium.h"
#include "TraceEvent.h"

namespace WebCore {

PassRefPtr<WebGLLayerChromium> WebGLLayerChromium::create(CCLayerDelegate* delegate)
{
    return adoptRef(new WebGLLayerChromium(delegate));
}

WebGLLayerChromium::WebGLLayerChromium(CCLayerDelegate* delegate)
    : CanvasLayerChromium(delegate)
    , m_context(0)
    , m_textureChanged(true)
    , m_contextSupportsRateLimitingExtension(false)
    , m_rateLimitingTimer(this, &WebGLLayerChromium::rateLimitContext)
    , m_textureUpdated(false)
{
}

WebGLLayerChromium::~WebGLLayerChromium()
{
}

bool WebGLLayerChromium::drawsContent() const
{
    return (m_context && m_context->getExtensions()->getGraphicsResetStatusARB() == GraphicsContext3D::NO_ERROR);
}

void WebGLLayerChromium::updateCompositorResources(GraphicsContext3D* rendererContext, TextureAllocator*)
{
    if (!drawsContent())
        return;

    if (m_dirtyRect.isEmpty())
        return;

    if (m_textureChanged) {
        rendererContext->bindTexture(GraphicsContext3D::TEXTURE_2D, m_textureId);
        // Set the min-mag filters to linear and wrap modes to GL_CLAMP_TO_EDGE
        // to get around NPOT texture limitations of GLES.
        rendererContext->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
        rendererContext->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
        rendererContext->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
        rendererContext->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
        m_textureChanged = false;
    }
    // Update the contents of the texture used by the compositor.
    if (!m_dirtyRect.isEmpty() && m_textureUpdated) {
        // prepareTexture copies the contents of the off-screen render target into the texture
        // used by the compositor.
        //
        m_context->prepareTexture();
        m_context->markLayerComposited();
        resetNeedsDisplay();
        m_textureUpdated = false;
    }
}

bool WebGLLayerChromium::paintRenderedResultsToCanvas(ImageBuffer* imageBuffer)
{
    if (m_textureUpdated || !layerRendererContext() || !drawsContent())
        return false;

    IntSize framebufferSize = m_context->getInternalFramebufferSize();
    ASSERT(layerRendererContext());

    // This would ideally be done in the webgl context, but that isn't possible yet.
    Platform3DObject framebuffer = layerRendererContext()->createFramebuffer();
    layerRendererContext()->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, framebuffer);
    layerRendererContext()->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, m_textureId, 0);

    Extensions3DChromium* extensions = static_cast<Extensions3DChromium*>(layerRendererContext()->getExtensions());
    extensions->paintFramebufferToCanvas(framebuffer, framebufferSize.width(), framebufferSize.height(), !m_context->getContextAttributes().premultipliedAlpha, imageBuffer);
    layerRendererContext()->deleteFramebuffer(framebuffer);
    return true;
}

void WebGLLayerChromium::setTextureUpdated()
{
    m_textureUpdated = true;
    // If WebGL commands are issued outside of a the animation callbacks, then use
    // call rateLimitOffscreenContextCHROMIUM() to keep the context from getting too far ahead.
    if (layerTreeHost() && !layerTreeHost()->animating() && m_contextSupportsRateLimitingExtension && !m_rateLimitingTimer.isActive())
        m_rateLimitingTimer.startOneShot(0);
}

void WebGLLayerChromium::setContext(const GraphicsContext3D* context)
{
    bool contextChanged = (m_context != context);

    m_context = const_cast<GraphicsContext3D*>(context);

    if (!m_context)
        return;

    unsigned int textureId = m_context->platformTexture();
    if (textureId != m_textureId || contextChanged) {
        m_textureChanged = true;
        m_textureUpdated = true;
    }
    m_textureId = textureId;
    GraphicsContext3D::Attributes attributes = m_context->getContextAttributes();
    m_hasAlpha = attributes.alpha;
    m_premultipliedAlpha = attributes.premultipliedAlpha;
    m_contextSupportsRateLimitingExtension = m_context->getExtensions()->supports("GL_CHROMIUM_rate_limit_offscreen_context");
}

GraphicsContext3D* WebGLLayerChromium::layerRendererContext()
{
    // FIXME: In the threaded case, paintRenderedResultsToCanvas must be
    // refactored to be asynchronous. Currently this is unimplemented.
    if (!layerTreeHost() || layerTreeHost()->settings().enableCompositorThread)
        return 0;
    return layerTreeHost()->context();
}

void WebGLLayerChromium::rateLimitContext(Timer<WebGLLayerChromium>*)
{
    TRACE_EVENT("WebGLLayerChromium::rateLimitContext", this, 0);

    if (!m_context)
        return;

    Extensions3DChromium* extensions = static_cast<Extensions3DChromium*>(m_context->getExtensions());
    extensions->rateLimitOffscreenContextCHROMIUM();
}

}
#endif // USE(ACCELERATED_COMPOSITING)
