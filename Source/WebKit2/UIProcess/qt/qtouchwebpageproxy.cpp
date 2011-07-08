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
#include "qtouchwebpageproxy.h"

#include <TiledDrawingAreaProxy.h>
#include <IntRect.h>
#include <NativeWebTouchEvent.h>
#include <WebEventFactoryQt.h>

using namespace WebCore;

QTouchWebPageProxy::QTouchWebPageProxy(TouchViewInterface* viewInterface, QWKContext* context, WKPageGroupRef pageGroupRef)
    : QtWebPageProxy(viewInterface, context, pageGroupRef)
    , m_panGestureRecognizer(viewInterface)
{
    init();
    // FIXME: add proper handling of viewport.
    setResizesToContentsUsingLayoutSize(QSize(980, 980));
}

PassOwnPtr<DrawingAreaProxy> QTouchWebPageProxy::createDrawingAreaProxy()
{
    return TiledDrawingAreaProxy::create(pageView(), m_webPageProxy.get());
}

void QTouchWebPageProxy::processDidCrash()
{
    QtWebPageProxy::processDidCrash();
    m_panGestureRecognizer.reset();
}

void QTouchWebPageProxy::paintContent(QPainter* painter, const QRect& area)
{
    m_webPageProxy->drawingArea()->paint(IntRect(area), painter);
}

void QTouchWebPageProxy::setViewportArguments(const WebCore::ViewportArguments& args)
{
    m_viewportArguments = args;
    // FIXME: we must tell our Views to react to the new viewport parameters.
}

#if ENABLE(TOUCH_EVENTS)
void QTouchWebPageProxy::doneWithTouchEvent(const NativeWebTouchEvent& event, bool wasEventHandled)
{
    if (wasEventHandled)
        m_panGestureRecognizer.reset();
    else
        m_panGestureRecognizer.recognize(event.nativeEvent());
}
#endif

bool QTouchWebPageProxy::handleEvent(QEvent* ev)
{
    switch (ev->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
        touchEvent(static_cast<QTouchEvent*>(ev));
        return true;
    }
    return QtWebPageProxy::handleEvent(ev);
}

void QTouchWebPageProxy::touchEvent(QTouchEvent* event)
{
#if ENABLE(TOUCH_EVENTS)
    m_webPageProxy->handleTouchEvent(NativeWebTouchEvent(event));
    event->accept();
#else
    ASSERT_NOT_REACHED();
    ev->ignore();
#endif
}
