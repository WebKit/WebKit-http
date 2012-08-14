/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "WebPageCompositor.h"

#if USE(ACCELERATED_COMPOSITING)
#include "WebPageCompositorClient.h"
#include "WebPageCompositor_p.h"

#include "BackingStore_p.h"
#include "LayerWebKitThread.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformDebugMacros.h>
#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformMessage.h>
#include <BlackBerryPlatformMessageClient.h>
#include <GenericTimerClient.h>
#include <ThreadTimerClient.h>
#include <wtf/CurrentTime.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

WebPageCompositorPrivate::WebPageCompositorPrivate(WebPagePrivate* page, WebPageCompositorClient* client)
    : m_client(client)
    , m_webPage(page)
    , m_drawsRootLayer(false)
{
    ASSERT(m_webPage);
    setOneShot(true); // one-shot animation client
}

WebPageCompositorPrivate::~WebPageCompositorPrivate()
{
    detach();
}

void WebPageCompositorPrivate::detach()
{
    if (m_webPage)
        Platform::AnimationFrameRateController::instance()->removeClient(this);
    m_webPage = 0;
}

void WebPageCompositorPrivate::setPage(WebPagePrivate *p)
{
    if (p == m_webPage)
        return;
    ASSERT(p);
    ASSERT(m_webPage); // if this is null, we have a bug and we need to re-add.
    m_webPage = p;
}

void WebPageCompositorPrivate::setContext(Platform::Graphics::GLES2Context* context)
{
    if (m_context == context)
        return;

    m_context = context;
    if (!m_context)
        m_layerRenderer.clear();
}

void WebPageCompositorPrivate::setRootLayer(LayerCompositingThread* rootLayer)
{
    m_rootLayer = rootLayer;
}

void WebPageCompositorPrivate::setOverlayLayer(LayerCompositingThread* overlayLayer)
{
    m_overlayLayer = overlayLayer;
}

void WebPageCompositorPrivate::prepareFrame(double animationTime)
{
    if (!m_context)
        return;

    // The LayerRenderer is involved in rendering the BackingStore when
    // WebPageCompositor is used to render the web page. The presence of a
    // WebPageCompositorClient (m_client) indicates this is the case, so
    // create a LayerRenderer if there are layers or if there's a client.
    if (!m_rootLayer && !m_overlayLayer && !m_compositingThreadOverlayLayer && !m_client)
        return;

    if (!m_layerRenderer) {
        m_layerRenderer = LayerRenderer::create(m_context);
        if (!m_layerRenderer->hardwareCompositing()) {
            m_layerRenderer.clear();
            return;
        }
    }

    // Unfortunately, we have to use currentTime() because the animations are
    // started in that time coordinate system.
    animationTime = currentTime();
    if (m_rootLayer)
        m_layerRenderer->prepareFrame(animationTime, m_rootLayer.get());
    if (m_overlayLayer)
        m_layerRenderer->prepareFrame(animationTime, m_overlayLayer.get());
    if (m_compositingThreadOverlayLayer)
        m_layerRenderer->prepareFrame(animationTime, m_compositingThreadOverlayLayer.get());
}

void WebPageCompositorPrivate::render(const IntRect& targetRect, const IntRect& clipRect, const TransformationMatrix& transformIn, const FloatRect& transformedContents, const FloatRect& /*viewport*/)
{
    // m_layerRenderer should have been created in prepareFrame
    if (!m_layerRenderer)
        return;

    // It's not safe to call into the BackingStore if the compositor hasn't been set yet.
    // For thread safety, we have to do it using a round-trip to the WebKit thread, so the
    // embedder might call this before the round-trip to WebPagePrivate::setCompositor() is
    // done.
    if (!m_webPage || m_webPage->compositor() != this)
        return;

    m_layerRenderer->setClearSurfaceOnDrawLayers(false);

    FloatRect contents = m_webPage->mapFromTransformedFloatRect(transformedContents);

    m_layerRenderer->setViewport(targetRect, clipRect, contents, m_layoutRectForCompositing, m_contentsSizeForCompositing);

    TransformationMatrix transform(transformIn);
    transform *= *m_webPage->m_transformationMatrix;

    if (!drawsRootLayer())
        m_webPage->m_backingStore->d->compositeContents(m_layerRenderer.get(), transform, contents, !m_backgroundColor.hasAlpha());

    compositeLayers(transform);
}

void WebPageCompositorPrivate::compositeLayers(const TransformationMatrix& transform)
{
    if (m_rootLayer)
        m_layerRenderer->compositeLayers(transform, m_rootLayer.get());
    if (m_overlayLayer)
        m_layerRenderer->compositeLayers(transform, m_overlayLayer.get());
    if (m_compositingThreadOverlayLayer)
        m_layerRenderer->compositeLayers(transform, m_compositingThreadOverlayLayer.get());

    m_lastCompositingResults = m_layerRenderer->lastRenderingResults();

    if (m_lastCompositingResults.needsAnimationFrame) {
        Platform::AnimationFrameRateController::instance()->addClient(this);
        m_webPage->updateDelegatedOverlays();
    }
}

bool WebPageCompositorPrivate::drawsRootLayer() const
{
    return m_rootLayer && m_drawsRootLayer;
}

bool WebPageCompositorPrivate::drawLayers(const IntRect& dstRect, const FloatRect& contents)
{
    // Is there anything to draw?
    if (!m_rootLayer && !m_overlayLayer && !m_compositingThreadOverlayLayer)
        return false;

    // prepareFrame creates the LayerRenderer if needed
    prepareFrame(currentTime());
    if (!m_layerRenderer)
        return false;

    bool shouldClear = drawsRootLayer();
    if (BackingStore* backingStore = m_webPage->m_backingStore)
        shouldClear = shouldClear || !backingStore->d->isOpenGLCompositing();
    m_layerRenderer->setClearSurfaceOnDrawLayers(shouldClear);

    // OpenGL window coordinates origin is at the lower left corner of the surface while
    // WebKit uses upper left as the origin of the window coordinate system. The passed in 'dstRect'
    // is in WebKit window coordinate system. Here we setup the viewport to the corresponding value
    // in OpenGL window coordinates.
    int viewportY = std::max(0, m_context->surfaceSize().height() - dstRect.maxY());
    IntRect viewport = IntRect(dstRect.x(), viewportY, dstRect.width(), dstRect.height());

    m_layerRenderer->setViewport(viewport, viewport, contents, m_layoutRectForCompositing, m_contentsSizeForCompositing);

    // WebKit uses row vectors which are multiplied by the matrix on the left (i.e. v*M)
    // Transformations are composed on the left so that M1.xform(M2) means M2*M1
    // We therefore start with our (othogonal) projection matrix, which will be applied
    // as the last transformation.
    TransformationMatrix transform = LayerRenderer::orthoMatrix(0, contents.width(), contents.height(), 0, -1000, 1000);
    transform.translate3d(-contents.x(), -contents.y(), 0);
    compositeLayers(transform);

    return true;
}

void WebPageCompositorPrivate::setBackgroundColor(const Color& color)
{
    m_backgroundColor = color;
}

void WebPageCompositorPrivate::releaseLayerResources()
{
    if (m_layerRenderer)
        m_layerRenderer->releaseLayerResources();
}

void WebPageCompositorPrivate::animationFrameChanged()
{
    BackingStore* backingStore = m_webPage->m_backingStore;
    if (!backingStore) {
        drawLayers(m_webPage->client()->userInterfaceBlittedDestinationRect(),
                   IntRect(m_webPage->client()->userInterfaceBlittedVisibleContentsRect()));
        return;
    }

    if (backingStore->d->shouldDirectRenderingToWindow()) {
        if (backingStore->d->isDirectRenderingAnimationMessageScheduled())
            return; // don't send new messages as long as we haven't rerendered

        using namespace BlackBerry::Platform;

        backingStore->d->setDirectRenderingAnimationMessageScheduled();
        webKitThreadMessageClient()->dispatchMessage(createMethodCallMessage(&BackingStorePrivate::renderVisibleContents, backingStore->d));
        return;
    }
    m_webPage->blitVisibleContents();
}

void WebPageCompositorPrivate::compositorDestroyed()
{
    if (m_client)
        m_client->compositorDestroyed();

    m_client = 0;
}

void WebPageCompositorPrivate::addOverlay(LayerCompositingThread* layer)
{
    if (!m_compositingThreadOverlayLayer)
        m_compositingThreadOverlayLayer = LayerCompositingThread::create(LayerData::CustomLayer, 0);
    m_compositingThreadOverlayLayer->addSublayer(layer);
}

void WebPageCompositorPrivate::removeOverlay(LayerCompositingThread* layer)
{
    if (layer->superlayer() != m_compositingThreadOverlayLayer)
        return;

    layer->removeFromSuperlayer();

    if (m_compositingThreadOverlayLayer && m_compositingThreadOverlayLayer->getSublayers().isEmpty())
        m_compositingThreadOverlayLayer.clear();
}

WebPageCompositor::WebPageCompositor(WebPage* page, WebPageCompositorClient* client)
{
    using namespace BlackBerry::Platform;

    RefPtr<WebPageCompositorPrivate> tmp = WebPageCompositorPrivate::create(page->d, client);

    // Keep one ref ourselves...
    d = tmp.get();
    d->ref();

    // ...And pass one ref to the web page.
    // This ensures that the compositor will be around for as long as it's
    // needed. Unfortunately RefPtr<T> is not public, so we have declare to
    // resort to manual refcounting.
    webKitThreadMessageClient()->dispatchMessage(createMethodCallMessage(&WebPagePrivate::setCompositor, d->page(), tmp));
}

WebPageCompositor::~WebPageCompositor()
{
    using namespace BlackBerry::Platform;

    // If we're being destroyed before the page, send a message to disconnect us
    if (d->page())
        webKitThreadMessageClient()->dispatchMessage(createMethodCallMessage(&WebPagePrivate::setCompositor, d->page(), PassRefPtr<WebPageCompositorPrivate>(0)));
    d->compositorDestroyed();
    d->deref();
}

WebPageCompositorClient* WebPageCompositor::client() const
{
    return d->client();
}

void WebPageCompositor::prepareFrame(Platform::Graphics::GLES2Context* context, double animationTime)
{
    d->setContext(context);
    d->prepareFrame(animationTime);
}

void WebPageCompositor::render(Platform::Graphics::GLES2Context* context,
                               const Platform::IntRect& targetRect,
                               const Platform::IntRect& clipRect,
                               const Platform::TransformationMatrix& transform,
                               const Platform::FloatRect& contents,
                               const Platform::FloatRect& viewport)
{
    d->setContext(context);
    d->render(targetRect, clipRect, TransformationMatrix(reinterpret_cast<const TransformationMatrix&>(transform)), contents, viewport);
}

void WebPageCompositor::cleanup(Platform::Graphics::GLES2Context* context)
{
    d->setContext(0);
}

void WebPageCompositor::contextLost()
{
    // This method needs to delete the compositor in a way that not tries to
    // use any OpenGL calls (the context is gone, so we can't and don't need to
    // free any OpenGL resources.
    notImplemented();
}

} // namespace WebKit
} // namespace BlackBerry

#else // USE(ACCELERATED_COMPOSITING)

namespace BlackBerry {
namespace WebKit {

WebPageCompositor::WebPageCompositor(WebPage*, WebPageCompositorClient*)
    : d(0)
{
}

WebPageCompositor::~WebPageCompositor()
{
}

WebPageCompositorClient* WebPageCompositor::client() const
{
    return 0;
}

void WebPageCompositor::prepareFrame(Platform::Graphics::GLES2Context*, double)
{
}

void WebPageCompositor::render(Platform::Graphics::GLES2Context*,
                               const Platform::IntRect&,
                               const Platform::IntRect&,
                               const Platform::TransformationMatrix&,
                               const Platform::FloatRect&,
                               const Platform::FloatRect&)
{
}

void WebPageCompositor::cleanup(Platform::Graphics::GLES2Context*)
{
}

void WebPageCompositor::contextLost()
{
}

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)
