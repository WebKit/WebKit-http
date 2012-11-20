/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if USE(COORDINATED_GRAPHICS)
#include "LayerTreeCoordinatorProxy.h"

#include "LayerTreeCoordinatorMessages.h"
#include "LayerTreeRenderer.h"
#include "UpdateInfo.h"
#include "WebCoreArgumentCoders.h"
#include "WebLayerTreeInfo.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <WebCore/GraphicsSurface.h>

#if ENABLE(CSS_SHADERS)
#include "CustomFilterProgramInfo.h"
#endif

namespace WebKit {

using namespace WebCore;

LayerTreeCoordinatorProxy::LayerTreeCoordinatorProxy(DrawingAreaProxy* drawingAreaProxy)
    : m_drawingAreaProxy(drawingAreaProxy)
    , m_renderer(adoptRef(new LayerTreeRenderer(this)))
    , m_lastSentScale(0)
{
}

LayerTreeCoordinatorProxy::~LayerTreeCoordinatorProxy()
{
    m_renderer->detach();
}

void LayerTreeCoordinatorProxy::updateViewport()
{
    m_drawingAreaProxy->updateViewport();
}

void LayerTreeCoordinatorProxy::dispatchUpdate(const Function<void()>& function)
{
    m_renderer->appendUpdate(function);
}

void LayerTreeCoordinatorProxy::createTileForLayer(int layerID, int tileID, const WebCore::IntRect& tileRect, const WebKit::SurfaceUpdateInfo& updateInfo)
{
    dispatchUpdate(bind(&LayerTreeRenderer::createTile, m_renderer.get(), layerID, tileID, updateInfo.scaleFactor));
    updateTileForLayer(layerID, tileID, tileRect, updateInfo);
}

void LayerTreeCoordinatorProxy::updateTileForLayer(int layerID, int tileID, const IntRect& tileRect, const WebKit::SurfaceUpdateInfo& updateInfo)
{
    SurfaceMap::iterator it = m_surfaces.find(updateInfo.atlasID);
    ASSERT(it != m_surfaces.end());
    dispatchUpdate(bind(&LayerTreeRenderer::updateTile, m_renderer.get(), layerID, tileID, LayerTreeRenderer::TileUpdate(updateInfo.updateRect, tileRect, it->value, updateInfo.surfaceOffset)));
}

void LayerTreeCoordinatorProxy::removeTileForLayer(int layerID, int tileID)
{
    dispatchUpdate(bind(&LayerTreeRenderer::removeTile, m_renderer.get(), layerID, tileID));
}

void LayerTreeCoordinatorProxy::createUpdateAtlas(int atlasID, const ShareableSurface::Handle& handle)
{
    ASSERT(!m_surfaces.contains(atlasID));
    m_surfaces.add(atlasID, ShareableSurface::create(handle));
}

void LayerTreeCoordinatorProxy::removeUpdateAtlas(int atlasID)
{
    ASSERT(m_surfaces.contains(atlasID));
    m_surfaces.remove(atlasID);
}

void LayerTreeCoordinatorProxy::deleteCompositingLayer(WebLayerID id)
{
    dispatchUpdate(bind(&LayerTreeRenderer::deleteLayer, m_renderer.get(), id));
    updateViewport();
}

void LayerTreeCoordinatorProxy::setRootCompositingLayer(WebLayerID id)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setRootLayerID, m_renderer.get(), id));
    updateViewport();
}

void LayerTreeCoordinatorProxy::setCompositingLayerState(WebLayerID id, const WebLayerInfo& info)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setLayerState, m_renderer.get(), id, info));
}

void LayerTreeCoordinatorProxy::setCompositingLayerChildren(WebLayerID id, const Vector<WebLayerID>& children)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setLayerChildren, m_renderer.get(), id, children));
}

#if ENABLE(CSS_FILTERS)
void LayerTreeCoordinatorProxy::setCompositingLayerFilters(WebLayerID id, const FilterOperations& filters)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setLayerFilters, m_renderer.get(), id, filters));
}
#endif

#if ENABLE(CSS_SHADERS)
void LayerTreeCoordinatorProxy::removeCustomFilterProgram(int id)
{
    dispatchUpdate(bind(&LayerTreeRenderer::removeCustomFilterProgram, m_renderer.get(), id));
}
void LayerTreeCoordinatorProxy::createCustomFilterProgram(int id, const WebCore::CustomFilterProgramInfo& programInfo)
{
    dispatchUpdate(bind(&LayerTreeRenderer::createCustomFilterProgram, m_renderer.get(), id, programInfo));
}
#endif

void LayerTreeCoordinatorProxy::didRenderFrame(const WebCore::IntSize& contentsSize, const WebCore::IntRect& coveredRect)
{
    dispatchUpdate(bind(&LayerTreeRenderer::flushLayerChanges, m_renderer.get()));
    updateViewport();
#if USE(TILED_BACKING_STORE)
    m_drawingAreaProxy->page()->didRenderFrame(contentsSize, coveredRect);
#else
    UNUSED_PARAM(contentsSize);
    UNUSED_PARAM(coveredRect);
#endif
}

void LayerTreeCoordinatorProxy::createImageBacking(CoordinatedImageBackingID imageID)
{
    dispatchUpdate(bind(&LayerTreeRenderer::createImageBacking, m_renderer.get(), imageID));
}

void LayerTreeCoordinatorProxy::updateImageBacking(CoordinatedImageBackingID imageID, const ShareableSurface::Handle& handle)
{
    dispatchUpdate(bind(&LayerTreeRenderer::updateImageBacking, m_renderer.get(), imageID, ShareableSurface::create(handle)));
}

void LayerTreeCoordinatorProxy::clearImageBackingContents(CoordinatedImageBackingID imageID)
{
    dispatchUpdate(bind(&LayerTreeRenderer::clearImageBackingContents, m_renderer.get(), imageID));
}

void LayerTreeCoordinatorProxy::removeImageBacking(CoordinatedImageBackingID imageID)
{
    dispatchUpdate(bind(&LayerTreeRenderer::removeImageBacking, m_renderer.get(), imageID));
}

void LayerTreeCoordinatorProxy::setContentsSize(const FloatSize& contentsSize)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setContentsSize, m_renderer.get(), contentsSize));
}

void LayerTreeCoordinatorProxy::setLayerAnimations(WebLayerID id, const GraphicsLayerAnimations& animations)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setLayerAnimations, m_renderer.get(), id, animations));
}

void LayerTreeCoordinatorProxy::setAnimationsLocked(bool locked)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setAnimationsLocked, m_renderer.get(), locked));
}

void LayerTreeCoordinatorProxy::setVisibleContentsRect(const FloatRect& rect, float scale, const FloatPoint& trajectoryVector)
{
    // Inform the renderer to adjust viewport-fixed layers.
    dispatchUpdate(bind(&LayerTreeRenderer::setVisibleContentsRect, m_renderer.get(), rect));

    // Round the rect instead of enclosing it to make sure that its size stays the same while panning. This can have nasty effects on layout.
    IntRect roundedRect = roundedIntRect(rect);
    if (roundedRect == m_lastSentVisibleRect && scale == m_lastSentScale && trajectoryVector == m_lastSentTrajectoryVector)
        return;

    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::SetVisibleContentsRect(roundedRect, scale, trajectoryVector), m_drawingAreaProxy->page()->pageID());
    m_lastSentVisibleRect = roundedRect;
    m_lastSentScale = scale;
    m_lastSentTrajectoryVector = trajectoryVector;
}

void LayerTreeCoordinatorProxy::renderNextFrame()
{
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::RenderNextFrame(), m_drawingAreaProxy->page()->pageID());
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void LayerTreeCoordinatorProxy::requestAnimationFrame()
{
    dispatchUpdate(bind(&LayerTreeRenderer::requestAnimationFrame, m_renderer.get()));
    updateViewport();
}

void LayerTreeCoordinatorProxy::animationFrameReady()
{
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::AnimationFrameReady(), m_drawingAreaProxy->page()->pageID());
}
#endif

void LayerTreeCoordinatorProxy::didChangeScrollPosition(const IntPoint& position)
{
    dispatchUpdate(bind(&LayerTreeRenderer::didChangeScrollPosition, m_renderer.get(), position));
}

#if USE(GRAPHICS_SURFACE)
void LayerTreeCoordinatorProxy::createCanvas(WebLayerID id, const IntSize& canvasSize, const GraphicsSurfaceToken& token)
{
    GraphicsSurface::Flags surfaceFlags = GraphicsSurface::SupportsTextureTarget | GraphicsSurface::SupportsSharing;
    dispatchUpdate(bind(&LayerTreeRenderer::createCanvas, m_renderer.get(), id, canvasSize, GraphicsSurface::create(canvasSize, surfaceFlags, token)));
}

void LayerTreeCoordinatorProxy::syncCanvas(WebLayerID id, uint32_t frontBuffer)
{
    dispatchUpdate(bind(&LayerTreeRenderer::syncCanvas, m_renderer.get(), id, frontBuffer));
}

void LayerTreeCoordinatorProxy::destroyCanvas(WebLayerID id)
{
    dispatchUpdate(bind(&LayerTreeRenderer::destroyCanvas, m_renderer.get(), id));
}
#endif

void LayerTreeCoordinatorProxy::purgeBackingStores()
{
    m_surfaces.clear();
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::PurgeBackingStores(), m_drawingAreaProxy->page()->pageID());
}

void LayerTreeCoordinatorProxy::setBackgroundColor(const WebCore::Color& color)
{
    dispatchUpdate(bind(&LayerTreeRenderer::setBackgroundColor, m_renderer.get(), color));
}

}
#endif // USE(COORDINATED_GRAPHICS)
