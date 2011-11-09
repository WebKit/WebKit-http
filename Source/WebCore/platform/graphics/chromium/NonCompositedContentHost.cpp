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

#include "NonCompositedContentHost.h"

#include "FloatRect.h"
#include "GraphicsLayer.h"
#include "LayerChromium.h"
#include "LayerPainterChromium.h"

namespace WebCore {

NonCompositedContentHost::NonCompositedContentHost(PassOwnPtr<LayerPainterChromium> contentPaint)
    : m_contentPaint(contentPaint)
{
    m_graphicsLayer = GraphicsLayer::create(this);
#ifndef NDEBUG
    m_graphicsLayer->setName("non-composited content");
#endif
    m_graphicsLayer->setDrawsContent(true);
    m_graphicsLayer->platformLayer()->setIsNonCompositedContent(true);
}

NonCompositedContentHost::~NonCompositedContentHost()
{
}

void NonCompositedContentHost::setBackgroundColor(const Color& color)
{
    if (color.isValid())
        m_graphicsLayer->platformLayer()->setBackgroundColor(color);
    else
        m_graphicsLayer->platformLayer()->setBackgroundColor(Color::white);
}

void NonCompositedContentHost::setScrollLayer(GraphicsLayer* layer)
{
    m_graphicsLayer->setNeedsDisplay();

    if (!layer) {
        m_graphicsLayer->removeFromParent();
        m_graphicsLayer->platformLayer()->setLayerTreeHost(0);
        return;
    }

    if (layer->platformLayer() == scrollLayer())
        return;

    layer->addChildAtIndex(m_graphicsLayer.get(), 0);
    ASSERT(scrollLayer());
}

void NonCompositedContentHost::setViewport(const IntSize& viewportSize, const IntSize& contentsSize, const IntPoint& scrollPosition, float pageScale)
{
    if (!scrollLayer())
        return;

    bool visibleRectChanged = m_viewportSize != viewportSize;

    m_viewportSize = viewportSize;
    scrollLayer()->setScrollPosition(scrollPosition);
    // Due to the possibility of pinch zoom, the noncomposited layer is always
    // assumed to be scrollable.
    scrollLayer()->setScrollable(true);
    m_graphicsLayer->setSize(contentsSize);

    if (visibleRectChanged)
        m_graphicsLayer->setNeedsDisplay();

    if (m_graphicsLayer->pageScaleFactor() != pageScale)
        m_graphicsLayer->deviceOrPageScaleFactorChanged();
}

LayerChromium* NonCompositedContentHost::scrollLayer()
{
    if (!m_graphicsLayer->parent())
        return 0;
    return m_graphicsLayer->parent()->platformLayer();
}

void NonCompositedContentHost::protectVisibleTileTextures()
{
    m_graphicsLayer->platformLayer()->protectVisibleTileTextures();
}

void NonCompositedContentHost::invalidateRect(const IntRect& rect)
{
    m_graphicsLayer->setNeedsDisplayInRect(FloatRect(rect));
}

void NonCompositedContentHost::notifyAnimationStarted(const GraphicsLayer*, double /* time */)
{
    // Intentionally left empty since we don't support animations on the non-composited content.
}

void NonCompositedContentHost::notifySyncRequired(const GraphicsLayer*)
{
}

void NonCompositedContentHost::paintContents(const GraphicsLayer*, GraphicsContext& context, GraphicsLayerPaintingPhase, const IntRect& clipRect)
{
    m_contentPaint->paint(context, clipRect);
}

bool NonCompositedContentHost::showDebugBorders() const
{
    return false;
}

bool NonCompositedContentHost::showRepaintCounter() const
{
    return false;
}

} // namespace WebCore
