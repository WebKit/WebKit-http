/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include "qquickwebpage_p.h"
#include "qquickwebview_p.h"
#include "qwebdownloaditem_p.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeextensionplugin.h>

QT_BEGIN_NAMESPACE

class QQuickWebViewExperimentalExtension : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickWebViewExperimental* experimental READ experimental CONSTANT FINAL)
public:
    QQuickWebViewExperimentalExtension(QObject *parent = 0) : QObject(parent) { }
    QQuickWebViewExperimental* experimental() { return static_cast<QQuickWebView*>(parent())->experimental(); }
};

class WebKitQmlExperimentalExtensionPlugin: public QDeclarativeExtensionPlugin {
    Q_OBJECT
public:
    virtual void registerTypes(const char* uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtWebKit.experimental"));

        qmlRegisterUncreatableType<QWebDownloadItem>(uri, 3, 0, "DownloadItem", QObject::tr("Cannot create separate instance of DownloadItem"));
        qmlRegisterExtendedType<QQuickWebView, QQuickWebViewExperimentalExtension>(uri, 3, 0, "WebView");
        qmlRegisterUncreatableType<QQuickWebViewExperimental>(uri, 3, 0, "QQuickWebViewExperimental",
            QObject::tr("Cannot create separate instance of QQuickWebViewExperimental"));
    }
};

QT_END_NAMESPACE

#include "plugin.moc"

Q_EXPORT_PLUGIN2(qmlwebkitpluginexperimental, QT_PREPEND_NAMESPACE(WebKitQmlExperimentalExtensionPlugin));
