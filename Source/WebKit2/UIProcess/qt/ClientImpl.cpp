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

#include "config.h"
#include "ClientImpl.h"

#include "WebFrameProxy.h"
#include "WKAPICast.h"
#include "WKStringQt.h"
#include "WKURLQt.h"
#include "qweberror.h"
#include "qweberror_p.h"
#include <qwkcontext.h>
#include <QtWebPageProxy.h>
#include <ViewInterface.h>
#include <WKFrame.h>
#include <WKType.h>

using namespace WebKit;

static QWKContext* toQWKContext(const void* clientInfo)
{
    if (clientInfo)
        return reinterpret_cast<QWKContext*>(const_cast<void*>(clientInfo));
    return 0;
}

static QtWebPageProxy* toQtWebPageProxy(const void* clientInfo)
{
    if (clientInfo)
        return reinterpret_cast<QtWebPageProxy*>(const_cast<void*>(clientInfo));
    return 0;
}

static inline ViewInterface* toViewInterface(const void* clientInfo)
{
    ASSERT(clientInfo);
    return reinterpret_cast<ViewInterface*>(const_cast<void*>(clientInfo));
}

static void dispatchLoadSucceeded(WKFrameRef frame, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->loadDidSucceed();
}

static void dispatchLoadFailed(WKFrameRef frame, const void* clientInfo, WKErrorRef error)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->loadDidFail(QWebErrorPrivate::createQWebError(error));
}

void qt_wk_didStartProvisionalLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->loadDidBegin();
}

void qt_wk_didFailProvisionalLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef userData, const void* clientInfo)
{
    dispatchLoadFailed(frame, clientInfo, error);
}

void qt_wk_didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;
    WebFrameProxy* wkframe = toImpl(frame);
    QString urlStr(wkframe->url());
    QUrl qUrl = urlStr;
    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->didChangeUrl(qUrl);
}

void qt_wk_didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    dispatchLoadSucceeded(frame, clientInfo);
}

void qt_wk_didFailLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef userData, const void* clientInfo)
{
    dispatchLoadFailed(frame, clientInfo, error);
}

void qt_wk_didSameDocumentNavigationForFrame(WKPageRef page, WKFrameRef frame, WKSameDocumentNavigationType type, WKTypeRef userData, const void* clientInfo)
{
    WebFrameProxy* wkframe = toImpl(frame);
    QString urlStr(wkframe->url());
    QUrl qUrl = urlStr;
    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->didChangeUrl(qUrl);
}

void qt_wk_didReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;
    QString qTitle = WKStringCopyQString(title);
    toQtWebPageProxy(clientInfo)->didChangeTitle(qTitle);
}

void qt_wk_didStartProgress(WKPageRef page, const void* clientInfo)
{
    toQtWebPageProxy(clientInfo)->didChangeLoadProgress(0);
}

void qt_wk_didChangeProgress(WKPageRef page, const void* clientInfo)
{
    toQtWebPageProxy(clientInfo)->didChangeLoadProgress(WKPageGetEstimatedProgress(page) * 100);
}

void qt_wk_didFinishProgress(WKPageRef page, const void* clientInfo)
{
    toQtWebPageProxy(clientInfo)->didChangeLoadProgress(100);
}

void qt_wk_setStatusText(WKPageRef, WKStringRef text, const void *clientInfo)
{
    QString qText = WKStringCopyQString(text);
    toViewInterface(clientInfo)->didChangeStatusText(qText);
}

void qt_wk_didChangeIconForPageURL(WKIconDatabaseRef iconDatabase, WKURLRef pageURL, const void* clientInfo)
{
    QUrl qUrl = WKURLCopyQUrl(pageURL);
    emit toQWKContext(clientInfo)->iconChangedForPageURL(qUrl);
}

void qt_wk_didRemoveAllIcons(WKIconDatabaseRef iconDatabase, const void* clientInfo)
{
}
