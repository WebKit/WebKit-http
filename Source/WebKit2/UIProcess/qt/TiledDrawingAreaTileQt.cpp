/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TiledDrawingAreaTile.h"

#include "GraphicsContext.h"
#include "SGAgent.h"
#include "ShareableBitmap.h"
#include "TiledDrawingAreaProxy.h"
#include "TouchViewInterface.h"
#include "UpdateInfo.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <QApplication>
#include <QObject>
#include <QPainter>

using namespace WebCore;

namespace WebKit {

TiledDrawingAreaTile::TiledDrawingAreaTile(TiledDrawingAreaProxy* proxy, PlatformWebView* webView, const Coordinate& tileCoordinate)
    : m_proxy(proxy)
    , m_coordinate(tileCoordinate)
    , m_rect(proxy->tileRectForCoordinate(tileCoordinate))
    , m_hasUpdatePending(false)
    , m_sgAgent(webView->sceneGraphAgent())
    , m_nodeID(0)
    , m_parentNodeID(0)
    , m_isBackBufferValid(false)
    , m_dirtyRegion(m_rect)
{
    static int id = 0;
    m_ID = ++id;
    m_proxy->registerTile(m_ID, this);
#ifdef TILE_DEBUG_LOG
    qDebug() << "deleting tile id=" << m_ID;
#endif
}

TiledDrawingAreaTile::~TiledDrawingAreaTile()
{
#ifdef TILE_DEBUG_LOG
    qDebug() << "deleting tile id=" << m_ID;
#endif
    disableUpdates();
    if (m_nodeID)
        m_sgAgent->removeNode(m_nodeID);
}

bool TiledDrawingAreaTile::isDirty() const
{
    return !m_dirtyRegion.isEmpty();
}

bool TiledDrawingAreaTile::isReadyToPaint() const
{
    return !!m_nodeID;
}

bool TiledDrawingAreaTile::hasReadyBackBuffer() const
{
    return m_isBackBufferValid && !m_hasUpdatePending;
}

void TiledDrawingAreaTile::invalidate(const IntRect& dirtyRect)
{
    IntRect tileDirtyRect = intersection(dirtyRect, m_rect);
    if (tileDirtyRect.isEmpty())
        return;

    m_dirtyRegion += tileDirtyRect;
}

void TiledDrawingAreaTile::resize(const IntSize& newSize)
{
    IntRect oldRect = m_rect;
    m_rect = IntRect(m_rect.location(), newSize);
    if (m_rect.maxX() > oldRect.maxX())
        invalidate(IntRect(oldRect.maxX(), oldRect.y(), m_rect.maxX() - oldRect.maxX(), m_rect.height()));
    if (m_rect.maxY() > oldRect.maxY())
        invalidate(IntRect(oldRect.x(), oldRect.maxY(), m_rect.width(), m_rect.maxY() - oldRect.maxY()));
}

void TiledDrawingAreaTile::swapBackBufferToFront()
{
    // FIXME: Implement once we can tight invalidated tile updates together.
}

void TiledDrawingAreaTile::paint(GraphicsContext* context, const IntRect& rect)
{
    ASSERT_NOT_REACHED();
}

void TiledDrawingAreaTile::incorporateUpdate(const UpdateInfo& updateInfo, float)
{
    ASSERT(m_proxy);
    m_isBackBufferValid = true;
    m_hasUpdatePending = false;

    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::create(updateInfo.bitmapHandle);
    QImage image(bitmap->createQImage());
    const IntRect& updateChunkRect = updateInfo.updateRectBounds;

    const QSize tileSize = m_proxy->tileSize();

    if (m_backBuffer.size() != tileSize) {
        m_backBuffer = QImage(tileSize, image.format());
        m_backBuffer.fill(Qt::transparent);
    }

    {
        QPainter painter(&m_backBuffer);
        IntSize drawPoint = updateChunkRect.location() - m_rect.location();
        painter.drawImage(QPoint(drawPoint.width(), drawPoint.height()), image);
    }

    if (!m_nodeID)
        m_nodeID = m_sgAgent->createTileNode(m_parentNodeID);
    ASSERT(m_nodeID);
    QRect sourceRect(0, 0, m_rect.width(), m_rect.height());
    m_sgAgent->setNodeTexture(m_nodeID, m_backBuffer, sourceRect, m_rect);
}

void TiledDrawingAreaTile::disableUpdates()
{
    if (!m_proxy)
        return;

    if (m_hasUpdatePending) {
        m_proxy->cancelTileUpdate(m_ID);
        m_hasUpdatePending = false;
    }

    m_proxy->unregisterTile(m_ID);
    m_backBuffer = QImage();
    m_isBackBufferValid = false;
    m_proxy = 0;
}

void TiledDrawingAreaTile::setParentNodeID(int nodeID)
{
    ASSERT(!m_nodeID);
    m_parentNodeID = nodeID;
}

void TiledDrawingAreaTile::updateBackBuffer()
{
    ASSERT(m_proxy);
    if (isReadyToPaint() && !isDirty())
        return;

    // FIXME: individual rects
    IntRect dirtyRect = m_dirtyRegion.boundingRect();
    m_dirtyRegion = QRegion();

#ifdef TILE_DEBUG_LOG
    qDebug() << "requesting tile update id=" << m_ID << " rect=" << QRect(dirtyRect) << " scale=" << m_proxy->contentsScale();
#endif
    if (!m_proxy->page()->process()->isValid())
        return;
    m_proxy->requestTileUpdate(m_ID, dirtyRect);

    m_hasUpdatePending = true;
}

}
