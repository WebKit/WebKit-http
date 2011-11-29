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

#ifndef qquickwebview_p_h
#define qquickwebview_p_h

#include "qwebkitglobal.h"
#include <QtDeclarative/qquickitem.h>

class QDeclarativeComponent;
class QQuickWebPage;
class QQuickWebViewAttached;
class QQuickWebViewPrivate;
class QQuickWebViewExperimental;
class QWebDownloadItem;
class QWebPreferences;

namespace WTR {
class PlatformWebView;
}

typedef const struct OpaqueWKContext* WKContextRef;
typedef const struct OpaqueWKPageGroup* WKPageGroupRef;
typedef const struct OpaqueWKPage* WKPageRef;

QT_BEGIN_NAMESPACE
class QPainter;
class QUrl;
QT_END_NAMESPACE

class QWEBKIT_EXPORT QQuickWebView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationStateChanged FINAL)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationStateChanged FINAL)
    Q_PROPERTY(bool canStop READ canStop NOTIFY navigationStateChanged FINAL)
    Q_PROPERTY(bool canReload READ canReload NOTIFY navigationStateChanged FINAL)
    Q_PROPERTY(QWebPreferences* preferences READ preferences CONSTANT FINAL)
    Q_PROPERTY(QQuickWebPage* page READ page CONSTANT FINAL)
    Q_ENUMS(NavigationRequestAction)
    Q_ENUMS(ErrorType)

public:
    enum NavigationRequestAction {
        AcceptRequest,
        IgnoreRequest,
        DownloadRequest
    };

    enum ErrorType {
        EngineError,
        NetworkError,
        HttpError,
        DownloadError
    };
    QQuickWebView(QQuickItem* parent = 0);
    virtual ~QQuickWebView();

    QUrl url() const;
    QString title() const;
    int loadProgress() const;

    bool canGoBack() const;
    bool canGoForward() const;
    bool canStop() const;
    bool canReload() const;

    QWebPreferences* preferences() const;
    QQuickWebPage* page();

    QQuickWebViewExperimental* experimental() const;
    static QQuickWebViewAttached* qmlAttachedProperties(QObject*);

public Q_SLOTS:
    void load(const QUrl&);
    void postMessage(const QString&);
    void loadHtml(const QString& html, const QUrl& baseUrl = QUrl());

    void goBack();
    void goForward();
    void stop();
    void reload();

Q_SIGNALS:
    void titleChanged(const QString& title);
    void statusBarMessageChanged(const QString& message);
    void loadStarted();
    void loadSucceeded();
    void loadFailed(QQuickWebView::ErrorType errorType, int errorCode, const QUrl& url);
    void loadProgressChanged(int progress);
    void urlChanged(const QUrl& url);
    void messageReceived(const QVariantMap& message);
    void linkHovered(const QUrl& url, const QString& title);
    void viewModeChanged();
    void navigationStateChanged();
    void navigationRequested(QObject* request);

protected:
    virtual void geometryChanged(const QRectF&, const QRectF&);
    virtual void focusInEvent(QFocusEvent*);
    virtual void focusOutEvent(QFocusEvent*);
    virtual void touchEvent(QTouchEvent* event);

private:
    Q_DECLARE_PRIVATE(QQuickWebView)

    QQuickWebView(WKContextRef, WKPageGroupRef, QQuickItem* parent = 0);
    WKPageRef pageRef() const;

    Q_PRIVATE_SLOT(d_func(), void _q_suspend());
    Q_PRIVATE_SLOT(d_func(), void _q_resume());
    Q_PRIVATE_SLOT(d_func(), void _q_viewportTrajectoryVectorChanged(const QPointF&));
    Q_PRIVATE_SLOT(d_func(), void _q_onOpenPanelFilesSelected());
    Q_PRIVATE_SLOT(d_func(), void _q_onOpenPanelFinished(int result));
    Q_PRIVATE_SLOT(d_func(), void _q_onVisibleChanged());
    // Hides QObject::d_ptr allowing us to use the convenience macros.
    QScopedPointer<QQuickWebViewPrivate> d_ptr;
    QQuickWebViewExperimental* m_experimental;

    friend class QtWebPageLoadClient;
    friend class QtWebPagePolicyClient;
    friend class QtWebPageProxy;
    friend class QtWebPageUIClient;
    friend class WTR::PlatformWebView;
    friend class QQuickWebViewExperimental;
};

QML_DECLARE_TYPE(QQuickWebView)

class QWEBKIT_EXPORT QQuickWebViewAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickWebView* view READ view NOTIFY viewChanged FINAL)

public:
    QQuickWebViewAttached(QObject* object);
    QQuickWebView* view() const { return m_view; }
    void setView(QQuickWebView*);

Q_SIGNALS:
    void viewChanged();

private:
    QQuickWebView* m_view;
};

QML_DECLARE_TYPEINFO(QQuickWebView, QML_HAS_ATTACHED_PROPERTIES)

class QWEBKIT_EXPORT QQuickWebViewExperimental : public QObject {
    Q_OBJECT
    Q_PROPERTY(QDeclarativeComponent* alertDialog READ alertDialog WRITE setAlertDialog NOTIFY alertDialogChanged)
    Q_PROPERTY(QDeclarativeComponent* confirmDialog READ confirmDialog WRITE setConfirmDialog NOTIFY confirmDialogChanged)
    Q_PROPERTY(QDeclarativeComponent* promptDialog READ promptDialog WRITE setPromptDialog NOTIFY promptDialogChanged)
    Q_PROPERTY(bool useTraditionalDesktopBehaviour READ useTraditionalDesktopBehaviour WRITE setUseTraditionalDesktopBehaviour)

public:
    QQuickWebViewExperimental(QQuickWebView* webView);
    virtual ~QQuickWebViewExperimental();

    QDeclarativeComponent* alertDialog() const;
    void setAlertDialog(QDeclarativeComponent*);
    QDeclarativeComponent* confirmDialog() const;
    void setConfirmDialog(QDeclarativeComponent*);
    QDeclarativeComponent* promptDialog() const;
    void setPromptDialog(QDeclarativeComponent*);

    bool useTraditionalDesktopBehaviour() const;

public Q_SLOTS:
    void setUseTraditionalDesktopBehaviour(bool enable);

Q_SIGNALS:
    void alertDialogChanged();
    void confirmDialogChanged();
    void promptDialogChanged();
    void downloadRequested(QWebDownloadItem* downloadItem);

private:
    QQuickWebView* q_ptr;
    QQuickWebViewPrivate* d_ptr;

    friend class QtWebPageProxy;

    Q_DECLARE_PRIVATE(QQuickWebView)
    Q_DECLARE_PUBLIC(QQuickWebView)
};

#endif // qquickwebview_p_h
