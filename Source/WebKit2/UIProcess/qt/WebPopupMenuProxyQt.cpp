/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPopupMenuProxyQt.h"

#include "ItemSelectorContextObject.h"
#include "PlatformPopupMenuData.h"
#include "WebPopupItem.h"
#include "qquickwebview_p.h"
#include "qquickwebview_p_p.h"
#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

using namespace WebCore;

namespace WebKit {

WebPopupMenuProxyQt::WebPopupMenuProxyQt(WebPopupMenuProxy::Client& client, QQuickWebView* webView)
    : WebPopupMenuProxy(client)
    , m_webView(webView)
{
}

WebPopupMenuProxyQt::~WebPopupMenuProxyQt()
{
}

void WebPopupMenuProxyQt::showPopupMenu(const IntRect& rect, WebCore::TextDirection, double, const Vector<WebPopupItem>& items, const PlatformPopupMenuData& data, int32_t)
{
    m_selectionType = (data.multipleSelections) ? WebPopupMenuProxyQt::MultipleSelection : WebPopupMenuProxyQt::SingleSelection;

    const QRectF mappedRect= m_webView->mapRectFromWebContent(QRect(rect));
    ItemSelectorContextObject* contextObject = new ItemSelectorContextObject(mappedRect, items, (m_selectionType == WebPopupMenuProxyQt::MultipleSelection));
    createItem(contextObject);
    if (!m_itemSelector) {
        hidePopupMenu();
        return;
    }
}

void WebPopupMenuProxyQt::hidePopupMenu()
{
    m_itemSelector = nullptr;
    m_context = nullptr;

    if (m_client) {
        m_client->closePopupMenu();
        invalidate();
    }
}

void WebPopupMenuProxyQt::selectIndex(int index)
{
    m_client->changeSelectedIndex(index);
}

void WebPopupMenuProxyQt::createItem(QObject* contextObject)
{
    QQmlComponent* component = m_webView->experimental()->itemSelector();
    if (!component) {
        delete contextObject;
        return;
    }

    createContext(component, contextObject);
    QObject* object = component->beginCreate(m_context.get());
    if (!object)
        return;

    m_itemSelector.reset(qobject_cast<QQuickItem*>(object));
    if (!m_itemSelector)
        return;

    connect(contextObject, SIGNAL(acceptedWithOriginalIndex(int)), SLOT(selectIndex(int)));

    // We enqueue these because they are triggered by m_itemSelector and will lead to its destruction.
    connect(contextObject, SIGNAL(done()), SLOT(hidePopupMenu()), Qt::QueuedConnection);
    if (m_selectionType == WebPopupMenuProxyQt::SingleSelection)
        connect(contextObject, SIGNAL(acceptedWithOriginalIndex(int)), SLOT(hidePopupMenu()), Qt::QueuedConnection);

    QQuickWebViewPrivate::get(m_webView)->addAttachedPropertyTo(m_itemSelector.get());
    m_itemSelector->setParentItem(m_webView);

    // Only fully create the component once we've set both a parent
    // and the needed context and attached properties, so that the
    // dialog can do useful stuff in Component.onCompleted().
    component->completeCreate();
}

void WebPopupMenuProxyQt::createContext(QQmlComponent* component, QObject* contextObject)
{
    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = QQmlEngine::contextForObject(m_webView);
    m_context.reset(new QQmlContext(baseContext));

    contextObject->setParent(m_context.get());
    m_context->setContextProperty(QStringLiteral("model"), contextObject);
    m_context->setContextObject(contextObject);
}

} // namespace WebKit

// And we can't compile the moc for WebPopupMenuProxyQt.h by itself, since it doesn't include "config.h"
#include "moc_WebPopupMenuProxyQt.cpp"
