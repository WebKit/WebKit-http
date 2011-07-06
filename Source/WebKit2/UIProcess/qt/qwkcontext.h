/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef qwkcontext_h
#define qwkcontext_h

#include "qwebkitglobal.h"
#include <QObject>
#include <WebKit2/WKContext.h>

class QIcon;
class QUrl;
class QWKContextPrivate;

class QWEBKIT_EXPORT QWKContext : public QObject {
    Q_OBJECT
public:
    QWKContext(QObject* parent = 0);
    virtual ~QWKContext();

    // Bridge from the C API
    QWKContext(WKContextRef contextRef, QObject* parent = 0);

    void setIconDatabasePath(const QString&);
    QIcon iconForPageURL(const QUrl&) const;

public:
    Q_SIGNAL void iconChangedForPageURL(const QUrl&);

private:
    QWKContextPrivate* d;

    friend class QtWebPageProxy;
};

#endif /* qwkcontext_h */
