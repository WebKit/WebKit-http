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

#ifndef QtWebPageLoadClient_h
#define QtWebPageLoadClient_h

#include <QtGlobal>
#include <WebFrameProxy.h>
#include <WKPage.h>
#include <wtf/text/WTFString.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

class QQuickWebView;

namespace WebKit {

class QtWebError;

class QtWebPageLoadClient {
public:
    QtWebPageLoadClient(WKPageRef, QQuickWebView*);

private:
    void didStartProvisionalLoad(const WTF::String&);
    void didReceiveServerRedirectForProvisionalLoad(const WTF::String&);
    void didCommitLoad();
    void didSameDocumentNavigation();
    void didReceiveTitle();
    void didChangeProgress(int);
    void didChangeBackForwardList();

    void dispatchLoadSucceeded();
    void dispatchLoadStopped();
    void dispatchLoadFailed(WebFrameProxy*, const QtWebError&);


    // WKPageLoadClient callbacks.
    static void didStartProvisionalLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didReceiveServerRedirectForProvisionalLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didFailProvisionalLoadWithErrorForFrame(WKPageRef, WKFrameRef, WKErrorRef, WKTypeRef userData, const void* clientInfo);
    static void didCommitLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didFailLoadWithErrorForFrame(WKPageRef, WKFrameRef, WKErrorRef, WKTypeRef userData, const void* clientInfo);
    static void didSameDocumentNavigationForFrame(WKPageRef, WKFrameRef, WKSameDocumentNavigationType, WKTypeRef userData, const void* clientInfo);
    static void didReceiveTitleForFrame(WKPageRef, WKStringRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didStartProgress(WKPageRef, const void* clientInfo);
    static void didChangeProgress(WKPageRef, const void* clientInfo);
    static void didFinishProgress(WKPageRef, const void* clientInfo);
    static void didChangeBackForwardList(WKPageRef, WKBackForwardListItemRef, WKArrayRef, const void *clientInfo);

    QQuickWebView* m_webView;
};

} // namespace Webkit

#endif // QtWebPageLoadClient_h
