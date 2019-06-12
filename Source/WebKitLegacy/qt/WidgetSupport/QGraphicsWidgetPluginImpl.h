/*
 * Copyright (C) 2015 The Qt Company Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef QGraphicsWidgetPluginImpl_h
#define QGraphicsWidgetPluginImpl_h

#ifndef QT_NO_GRAPHICSVIEW

#include "QtPluginWidgetAdapter.h"

QT_BEGIN_NAMESPACE
class QGraphicsWidget;
QT_END_NAMESPACE

class QGraphicsWidgetPluginImpl final : public QtPluginWidgetAdapter {
    Q_OBJECT
public:
    QGraphicsWidgetPluginImpl(QGraphicsWidget *w) : m_graphicsWidget(w) { }
    ~QGraphicsWidgetPluginImpl();
    void update(const QRect &) final;
    void setGeometryAndClip(const QRect&, const QRect&, bool) final;
    void setVisible(bool) final;
    void setStyleSheet(const QString&) final { }
    void setWidgetParent(QObject*) final;
    QObject* handle() const final;
private:
    QGraphicsWidget *m_graphicsWidget;
};
#endif // !defined(QT_NO_GRAPHICSVIEW)

#endif // QGraphicsWidgetPluginImpl_h
