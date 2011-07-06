/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
#include "qtouchwebview.h"
#include "qtouchwebview_p.h"

#include "TouchViewInterface.h"
#include "qtouchwebpage_p.h"
#include "QtWebPageProxy.h"
#include <qgraphicssceneevent.h>

QTouchWebViewPrivate::QTouchWebViewPrivate(QTouchWebView* q)
    : q(q)
    , pageView(new QTouchWebPage(q))
    , viewInterface(q, pageView.data())
    , page(&viewInterface, defaultWKContext())
{
    QTouchWebPagePrivate* const pageViewPrivate = QTouchWebPagePrivate::getPageViewPrivate(pageView.data());
    pageViewPrivate->setPage(&page);
}

QTouchWebView::QTouchWebView()
    : d(new QTouchWebViewPrivate(this))
{
    setFlags(QGraphicsItem::ItemClipsChildrenToShape);
}

QTouchWebView::~QTouchWebView()
{
    delete d;
}

QTouchWebPage* QTouchWebView::page()
{
    return d->pageView.data();
}
