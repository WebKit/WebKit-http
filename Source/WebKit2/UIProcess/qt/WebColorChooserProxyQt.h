/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef WebColorChooserProxyQt_h
#define WebColorChooserProxyQt_h

#include "IntRect.h"
#include "WebColorChooserProxy.h"
#include <QtCore/QObject>
#include <wtf/OwnPtr.h>

QT_BEGIN_NAMESPACE
class QQmlComponent;
class QQmlContext;
class QQuickItem;
QT_END_NAMESPACE

class QQuickWebView;

namespace WebCore {
class Color;
}

namespace WebKit {

class WebColorChooserProxyQt : public QObject, public WebColorChooserProxy {
    Q_OBJECT

public:
    static PassRefPtr<WebColorChooserProxy> create(WebColorChooserProxy::Client* client, QQuickWebView* webView, const WebCore::Color& initialColor, const WebCore::IntRect& elementRect)
    {
        return adoptRef(new WebColorChooserProxyQt(client, webView, initialColor, elementRect));
    }
    ~WebColorChooserProxyQt();

    virtual void setSelectedColor(const WebCore::Color&);

public Q_SLOTS:
    virtual void endChooser();

private Q_SLOTS:
    void notifyColorSelected(const QColor&);

private:
    WebColorChooserProxyQt(WebColorChooserProxy::Client*, QQuickWebView*, const WebCore::Color&, const WebCore::IntRect&);

    void createItem(QObject*);
    void createContext(QQmlComponent*, QObject*);

    OwnPtr<QQmlContext> m_context;
    OwnPtr<QQuickItem> m_colorChooser;

    QQuickWebView* m_webView;
};

} // namespace WebKit

#endif // WebColorChooserProxyQt_h
