/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

/*
 *  Warning: The contents of this file is not part of the public QtWebKit API
 *  and may be changed from version to version or even be completely removed.
 */

#include <QMediaPlayer>
#include <QObject>

class QWebFullScreenVideoHandler : public QObject {
    Q_OBJECT
public:
    QWebFullScreenVideoHandler() {}
    virtual ~QWebFullScreenVideoHandler() {}
    virtual bool requiresFullScreenForVideoPlayback() const = 0;

Q_SIGNALS:
    void fullScreenClosed();

public Q_SLOTS:
    virtual void enterFullScreen(QMediaPlayer*) = 0;
    virtual void exitFullScreen() = 0;
};
