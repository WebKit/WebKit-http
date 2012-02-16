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

#if USE(ACCELERATED_COMPOSITING)
#include "WebPageCompositor.h"

#include "LayerWebKitThread.h"
#include "WebPage_p.h"

#include <GenericTimerClient.h>
#include <ThreadTimerClient.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

WebPageCompositor::WebPageCompositor(WebPagePrivate* page)
    : m_webPage(page)
    , m_layerRenderer(LayerRenderer::create(page->m_page))
    , m_generation(0)
    , m_compositedGeneration(-1)
    , m_compositingOntoMainWindow(false)
    , m_blitTimer(this, &BlackBerry::WebKit::WebPageCompositor::blitTimerFired)
    , m_timerClient(new BlackBerry::Platform::GenericTimerClient(BlackBerry::Platform::userInterfaceThreadTimerClient()))
{
    m_blitTimer.setClient(m_timerClient);
}

WebPageCompositor::~WebPageCompositor()
{
    m_blitTimer.stop();
    delete m_timerClient;
}

bool WebPageCompositor::hardwareCompositing() const
{
    return m_layerRenderer->hardwareCompositing();
}

void WebPageCompositor::setRootLayer(LayerCompositingThread* rootLayer)
{
    m_rootLayer = rootLayer;
    m_layerRenderer->setRootLayer(m_rootLayer.get());
}

void WebPageCompositor::setCompositingOntoMainWindow(bool compositingOntoMainWindow)
{
    m_compositingOntoMainWindow = compositingOntoMainWindow;
    m_layerRenderer->setClearSurfaceOnDrawLayers(!compositingOntoMainWindow);
}

void WebPageCompositor::commit(LayerWebKitThread* rootLayer)
{
    if (!rootLayer)
        return;

    rootLayer->commitOnCompositingThread();
    ++m_generation;
}

bool WebPageCompositor::drawLayers(const IntRect& dstRect, const FloatRect& contents)
{
    // Save a draw if we already drew this generation, for example due to a concurrent scroll operation.
    if (m_compositedGeneration == m_generation && dstRect == m_compositedDstRect
        && contents == m_compositedContentsRect && !m_compositingOntoMainWindow)
        return false;

    m_layerRenderer->drawLayers(contents, m_layoutRectForCompositing, m_contentsSizeForCompositing, dstRect);
    m_lastCompositingResults = m_layerRenderer->lastRenderingResults();

    m_compositedDstRect = dstRect;
    m_compositedContentsRect = contents;
    m_compositedGeneration = m_generation;

    if (m_lastCompositingResults.needsAnimationFrame) {
        ++m_generation; // The animation update moves us along one generation.
        // Using a timeout of 0 actually won't start a timer, it will send a message.
        m_blitTimer.start(1.0 / 60.0);
        m_webPage->updateDelegatedOverlays();
    }

    return true;
}

void WebPageCompositor::releaseLayerResources()
{
    m_layerRenderer->releaseLayerResources();
}

void WebPageCompositor::blitTimerFired()
{
    m_webPage->blitVisibleContents();
}

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)
