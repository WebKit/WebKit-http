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

#include "config.h"
#include "WebColorPickerQt.h"

#include "ColorChooserContextObject.h"
#include "qquickwebview_p.h"
#include "qquickwebview_p_p.h"
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

using namespace WebCore;

namespace WebKit {

WebColorPickerQt::WebColorPickerQt(WebColorPicker::Client* client, QQuickWebView* webView, const Color& initialColor, const IntRect& elementRect)
    : WebColorPicker(client)
    , m_webView(webView)
{
    const QRectF mappedRect= m_webView->mapRectFromWebContent(QRect(elementRect));
    ColorChooserContextObject* contextObject = new ColorChooserContextObject(initialColor, mappedRect);
    createItem(contextObject);
}

WebColorPickerQt::~WebColorPickerQt()
{
}

void WebColorPickerQt::createItem(QObject* contextObject)
{
    QQmlComponent* component = m_webView->experimental()->colorChooser();
    if (!component) {
        delete contextObject;
        return;
    }

    createContext(component, contextObject);
    QObject* object = component->beginCreate(m_context.get());
    if (!object) {
        m_context = nullptr;
        return;
    }

    m_colorChooser.reset(qobject_cast<QQuickItem*>(object));
    if (!m_colorChooser) {
        m_context = nullptr;
        return;
    }

    // Needs to be enqueue because it might trigger deletion.
    connect(contextObject, SIGNAL(accepted(QColor)), SLOT(notifyColorSelected(QColor)), Qt::QueuedConnection);
    connect(contextObject, SIGNAL(rejected()), SLOT(endPicker()), Qt::QueuedConnection);

    QQuickWebViewPrivate::get(m_webView)->addAttachedPropertyTo(m_colorChooser.get());
    m_colorChooser->setParentItem(m_webView);

    component->completeCreate();
}

void WebColorPickerQt::createContext(QQmlComponent* component, QObject* contextObject)
{
    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = QQmlEngine::contextForObject(m_webView);
    m_context.reset(new QQmlContext(baseContext));

    contextObject->setParent(m_context.get());
    m_context->setContextProperty(QStringLiteral("model"), contextObject);
    m_context->setContextObject(contextObject);
}

void WebColorPickerQt::setSelectedColor(const Color&)
{
    // This is suppose to be used to react to DOM changes. When
    // a user script changes the input value, the method gives the
    // option to update the color chooser UI if we were showing the
    // current value. Since we don't, it is irrelevant right now.
    // And yes, the name sounds misleading but comes from WebCore.
}

void WebColorPickerQt::showColorPicker(const Color&)
{
    // We use ENABLE(INPUT_TYPE_COLOR_POPOVER), so new color picker is created
    // each time
}

void WebColorPickerQt::notifyColorSelected(const QColor& color)
{
    if (!m_client)
        return;

    // Alpha is always ignored by the color input
    Color coreColor = makeRGB(color.red(), color.green(), color.blue());
    m_client->didChooseColor(coreColor);

    endPicker();
}

void WebColorPickerQt::endPicker()
{
    m_colorChooser = nullptr;
    m_context = nullptr;
}

} // namespace WebKit

#include "moc_WebColorPickerQt.cpp"
