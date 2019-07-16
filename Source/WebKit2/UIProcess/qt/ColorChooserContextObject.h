/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#pragma once

#include <QColor>
#include <QObject>
#include <QRectF>

namespace WebKit {

class ColorChooserContextObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QColor currentColor READ currentColor CONSTANT FINAL)
    Q_PROPERTY(QRectF elementRect READ elementRect CONSTANT FINAL)

public:
    ColorChooserContextObject(const QColor& color, const QRectF& rect)
        : m_currentColor(color)
        , m_rect(rect)
    {
    }

    QColor currentColor() const { return m_currentColor; }
    QRectF elementRect() const { return m_rect; }

    Q_INVOKABLE void accept(const QColor& color) { emit accepted(color); }
    Q_INVOKABLE void reject() { emit rejected(); }

Q_SIGNALS:
    void accepted(const QColor&);
    void rejected();

private:
    QColor m_currentColor;
    QRectF m_rect;
};

} // namespace WebKit
