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
#include "QtWebPagePolicyClient.h"

#include "WKURLQt.h"
#include "qquickwebview_p.h"
#include "qquickwebview_p_p.h"
#include <QtCore/QObject>
#include <WKFramePolicyListener.h>
#include <WKURLRequest.h>

class NavigationRequest : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url CONSTANT FINAL)
    Q_PROPERTY(int button READ button CONSTANT FINAL)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT FINAL)
    Q_PROPERTY(int action READ action WRITE setAction NOTIFY actionChanged FINAL)

public:
    NavigationRequest(const QUrl& url, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
        : m_url(url)
        , m_button(button)
        , m_modifiers(modifiers)
        , m_action(QQuickWebView::AcceptRequest)
    {
    }

    QUrl url() const { return m_url; }
    int button() const { return int(m_button); }
    int modifiers() const { return int(m_modifiers); }

    int action() const { return int(m_action); }
    void setAction(int action)
    {
        if (m_action == action)
            return;
        m_action = action;
        emit actionChanged();
    }

Q_SIGNALS:
    void actionChanged();

private:
    QUrl m_url;
    Qt::MouseButton m_button;
    Qt::KeyboardModifiers m_modifiers;
    int m_action;
};

QtWebPagePolicyClient::QtWebPagePolicyClient(WKPageRef pageRef, QQuickWebView* webView)
    : m_webView(webView)
{
    WKPagePolicyClient policyClient;
    memset(&policyClient, 0, sizeof(WKPagePolicyClient));
    policyClient.version = kWKPagePolicyClientCurrentVersion;
    policyClient.clientInfo = this;
    policyClient.decidePolicyForNavigationAction = decidePolicyForNavigationAction;
    policyClient.decidePolicyForResponse = decidePolicyForResponse;
    WKPageSetPagePolicyClient(pageRef, &policyClient);
}

void QtWebPagePolicyClient::decidePolicyForNavigationAction(const QUrl& url, Qt::MouseButton mouseButton, Qt::KeyboardModifiers keyboardModifiers, WKFramePolicyListenerRef listener)
{
    // NOTE: even though the C API (and the WebKit2 IPC) supports an asynchronous answer, this is not currently working.
    // We are expected to call the listener immediately. See the patch for https://bugs.webkit.org/show_bug.cgi?id=53785.
    NavigationRequest navigationRequest(url, mouseButton, keyboardModifiers);
    emit m_webView->navigationRequested(&navigationRequest);

    switch (QQuickWebView::NavigationRequestAction(navigationRequest.action())) {
    case QQuickWebView::IgnoreRequest:
        WKFramePolicyListenerIgnore(listener);
        return;
    case QQuickWebView::DownloadRequest:
        WKFramePolicyListenerDownload(listener);
        return;
    case QQuickWebView::AcceptRequest:
        WKFramePolicyListenerUse(listener);
        return;
    }
    ASSERT_NOT_REACHED();
}

static inline QtWebPagePolicyClient* toQtWebPagePolicyClient(const void* clientInfo)
{
    ASSERT(clientInfo);
    return reinterpret_cast<QtWebPagePolicyClient*>(const_cast<void*>(clientInfo));
}

static Qt::MouseButton toQtMouseButton(WKEventMouseButton button)
{
    switch (button) {
    case kWKEventMouseButtonLeftButton:
        return Qt::LeftButton;
    case kWKEventMouseButtonMiddleButton:
        return Qt::MiddleButton;
    case kWKEventMouseButtonRightButton:
        return Qt::RightButton;
    case kWKEventMouseButtonNoButton:
        return Qt::NoButton;
    }
    ASSERT_NOT_REACHED();
    return Qt::NoButton;
}

static Qt::KeyboardModifiers toQtKeyboardModifiers(WKEventModifiers modifiers)
{
    Qt::KeyboardModifiers qtModifiers = Qt::NoModifier;
    if (modifiers & kWKEventModifiersShiftKey)
        qtModifiers |= Qt::ShiftModifier;
    if (modifiers & kWKEventModifiersControlKey)
        qtModifiers |= Qt::ControlModifier;
    if (modifiers & kWKEventModifiersAltKey)
        qtModifiers |= Qt::AltModifier;
    if (modifiers & kWKEventModifiersMetaKey)
        qtModifiers |= Qt::MetaModifier;
    return qtModifiers;
}

void QtWebPagePolicyClient::decidePolicyForNavigationAction(WKPageRef, WKFrameRef, WKFrameNavigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef, const void* clientInfo)
{
    WKURLRef requestURL = WKURLRequestCopyURL(request);
    QUrl qUrl = WKURLCopyQUrl(requestURL);
    WKRelease(requestURL);
    toQtWebPagePolicyClient(clientInfo)->decidePolicyForNavigationAction(qUrl, toQtMouseButton(mouseButton), toQtKeyboardModifiers(modifiers), listener);
}

void QtWebPagePolicyClient::decidePolicyForResponse(WKPageRef page, WKFrameRef frame, WKURLResponseRef response, WKURLRequestRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*)
{
    String type = toImpl(response)->resourceResponse().mimeType();
    type.makeLower();
    bool canShowMIMEType = toImpl(frame)->canShowMIMEType(type);

    if (WKPageGetMainFrame(page) == frame) {
        if (canShowMIMEType) {
            WKFramePolicyListenerUse(listener);
            return;
        }

        // If we can't use (show) it then we should download it.
        WKFramePolicyListenerDownload(listener);
        return;
    }

    // We should ignore downloadable top-level content for subframes, with an exception for text/xml and application/xml so we can still support Acid3 test.
    // It makes the browser intentionally behave differently when it comes to text(application)/xml content in subframes vs. mainframe.
    if (!canShowMIMEType && !(type == "text/xml" || type == "application/xml")) {
        WKFramePolicyListenerIgnore(listener);
        return;
    }

    WKFramePolicyListenerUse(listener);
}

#include "QtWebPagePolicyClient.moc"
