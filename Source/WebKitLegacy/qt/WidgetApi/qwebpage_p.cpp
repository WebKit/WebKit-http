/*
    Copyright (C) 2008, 2009, 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.
    Copyright (C) 2007 Apple Inc.

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

#include "qwebpage_p.h"

#include "qwebframe_p.h"
#include "qwebfullscreenrequest.h"
#include "qwebinspector.h"
#include <QMenu>
#include <QUndoStack>

QWebPagePrivate::~QWebPagePrivate()
{
#ifndef QT_NO_CONTEXTMENU
    delete currentContextMenu.data();
#endif
#ifndef QT_NO_UNDOSTACK
    delete undoStack;
    undoStack = 0;
#endif

    if (inspector) {
        // If the inspector is ours, delete it, otherwise just detach from it.
        if (inspectorIsInternalOnly)
            delete inspector;
        else
            inspector->setPage(0);
    }
    // Explicitly destruct the WebCore page at this point when the
    // QWebPagePrivate / QWebPageAdapater vtables are still intact,
    // in order for various destruction callbacks out of WebCore to
    // work.
    deletePage();

    clearCustomActions();
}

QWebFramePrivate::~QWebFramePrivate() = default;

void QWebPagePrivate::fullScreenRequested(QWebFullScreenRequest request)
{
    m_fullScreenRequested.invoke(q, Qt::QueuedConnection, Q_ARG(QWebFullScreenRequest, request));
}

void QWebPagePrivate::recentlyAudibleChanged(bool recentlyAudible)
{
    emit q->recentlyAudibleChanged(recentlyAudible);
}

void QWebPagePrivate::focusedElementChanged(const QWebElement& element)
{
    emit q->focusedElementChanged(element);
}
