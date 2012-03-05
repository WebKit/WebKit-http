/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "qquickwebpage_p.h"

#include "LayerTreeHostProxy.h"
#include "QtWebPageEventHandler.h"
#include "QtWebPageSGNode.h"
#include "TransformationMatrix.h"
#include "qquickwebpage_p_p.h"
#include "qquickwebview_p.h"

QQuickWebPage::QQuickWebPage(QQuickWebView* viewportItem)
    : QQuickItem(viewportItem)
    , d(new QQuickWebPagePrivate(this, viewportItem))
{
    setFlag(ItemHasContents);

    // We do the transform from the top left so the viewport can assume the position 0, 0
    // is always where rendering starts.
    setTransformOrigin(TopLeft);
}

QQuickWebPage::~QQuickWebPage()
{
    delete d;
}

QQuickWebPagePrivate::QQuickWebPagePrivate(QQuickWebPage* q, QQuickWebView* viewportItem)
    : q(q)
    , viewportItem(viewportItem)
    , webPageProxy(0)
    , paintingIsInitialized(false)
    , m_paintNode(0)
    , contentsScale(1)
{
}

void QQuickWebPagePrivate::initialize(WebKit::WebPageProxy* webPageProxy)
{
    this->webPageProxy = webPageProxy;
    eventHandler.reset(new QtWebPageEventHandler(toAPI(webPageProxy), q, viewportItem));
}

void QQuickWebPagePrivate::setDrawingAreaSize(const QSize& size)
{
    DrawingAreaProxy* drawingArea = webPageProxy->drawingArea();
    if (!drawingArea)
        return;
    drawingArea->setSize(WebCore::IntSize(size), WebCore::IntSize());
}

void QQuickWebPagePrivate::paint(QPainter* painter)
{
    if (webPageProxy->drawingArea())
        webPageProxy->drawingArea()->paintLayerTree(painter);
}


QSGNode* QQuickWebPage::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    if (!(flags() & ItemHasContents)) {
        if (oldNode)
            delete oldNode;
        return 0;
    }

    QtWebPageSGNode* sceneNode = static_cast<QtWebPageSGNode*>(oldNode);
    if (sceneNode)
        return sceneNode;
    sceneNode = new QtWebPageSGNode(d->webPageProxy->drawingArea()->layerTreeHostProxy(), d);
    {
        MutexLocker lock(d->m_paintNodeMutex);
        d->m_paintNode = sceneNode;
    }
    d->updateSize();
    return sceneNode;
}

QtWebPageEventHandler* QQuickWebPage::eventHandler() const
{
    return d->eventHandler.data();
}

void QQuickWebPage::setContentsSize(const QSizeF& size)
{
    if (size.isEmpty() || d->contentsSize == size)
        return;

    d->contentsSize = size;
    d->updateSize();
    d->setDrawingAreaSize(d->contentsSize.toSize());
}

const QSizeF& QQuickWebPage::contentsSize() const
{
    return d->contentsSize;
}

void QQuickWebPage::setContentsScale(qreal scale)
{
    ASSERT(scale > 0);
    d->contentsScale = scale;
    d->updateSize();
}

qreal QQuickWebPage::contentsScale() const
{
    ASSERT(d->contentsScale > 0);
    return d->contentsScale;
}

QTransform QQuickWebPage::transformFromItem() const
{
    return transformToItem().inverted();
}

QTransform QQuickWebPage::transformToItem() const
{
    QPointF pos = d->viewportItem->pageItemPos();
    return QTransform(d->contentsScale, 0, 0, 0, d->contentsScale, 0, pos.x(), pos.y(), 1);
}

void QQuickWebPagePrivate::updateSize()
{
    QSizeF scaledSize = contentsSize * contentsScale;
    q->setSize(scaledSize);
    viewportItem->updateContentsSize(scaledSize);
    QRectF clipRect = viewportItem->mapRectToScene(viewportItem->boundingRect());

    MutexLocker lock(m_paintNodeMutex);
    if (!m_paintNode)
        return;
    m_paintNode->setClipRect(clipRect);
    m_paintNode->setContentsScale(contentsScale);
}

void QQuickWebPagePrivate::willDeleteScenegraphNode()
{
    MutexLocker lock(m_paintNodeMutex);
    m_paintNode = 0;
}

QQuickWebPagePrivate::~QQuickWebPagePrivate()
{
}

#include "moc_qquickwebpage_p.cpp"
