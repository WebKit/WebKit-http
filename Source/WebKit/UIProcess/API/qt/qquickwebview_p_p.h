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

#ifndef qquickwebview_p_p_h
#define qquickwebview_p_p_h

#include "DefaultUndoController.h"
#include "PageViewportController.h"
#include "QtPageClient.h"
#include "QtWebPageUIClient.h"

#include "qquickwebpage_p.h"
#include "qquickwebview_p.h"
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <WebCore/ViewportArguments.h>
#include <WebKit/WKRetainPtr.h>
#include <wtf/RefPtr.h>

namespace WebKit {
class CoordinatedGraphicsScene;
class DownloadProxy;
class DrawingAreaProxy;
class QtDialogRunner;
class PageViewportControllerClientQt;
class QtWebContext;
class QtWebError;
class QtWebPageEventHandler;
class QtWebPagePolicyClient;
class WebPageProxy;
}

class QWebNavigationHistory;
class QWebKitTest;
class QWebChannelWebKitTransport;

QT_BEGIN_NAMESPACE
class QQmlComponent;
QT_END_NAMESPACE

class QQuickWebViewPrivate {
    Q_DECLARE_PUBLIC(QQuickWebView)
    friend class WebKit::QtDialogRunner;
    friend class QQuickWebViewExperimental;
    friend class QQuickWebPage;
    friend class QWebPreferencesPrivate;
    friend class QWebKitTest;

public:
    static QQuickWebViewPrivate* get(QQuickWebView* q) { return q->d_ptr.data(); }
    static QQuickWebViewPrivate* get(WKPageRef);

    virtual ~QQuickWebViewPrivate();

    virtual void initialize(WKPageConfigurationRef configurationRef = 0);

    virtual void onComponentComplete() { }

    virtual void loadProgressDidChange(int loadProgress);
    virtual void handleMouseEvent(QMouseEvent*);

    static void didFindString(WKPageRef page, WKStringRef string, unsigned matchCount, const void* clientInfo);
    static void didFailToFindString(WKPageRef page, WKStringRef string, const void* clientInfo);

    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes& attr) { }

    int loadProgress() const { return m_loadProgress; }
    void setNeedsDisplay();
    void didRenderFrame();

    virtual WebKit::PageViewportController* viewportController() const { return 0; }
    virtual void updateViewportSize() { }
    void updateTouchViewportSize();

    virtual qreal zoomFactor() const { return 1; }
    virtual void setZoomFactor(qreal) { }

    void _q_onVisibleChanged();
    void _q_onUrlChanged();
    void _q_onReceivedResponseFromDownload(QWebDownloadItem*);
    void _q_onIconChangedForPageURL(const QString&);

    void chooseFiles(WKOpenPanelResultListenerRef, const QStringList& selectedFileNames, WebKit::QtWebPageUIClient::FileChooserType);
    quint64 exceededDatabaseQuota(const QString& databaseName, const QString& displayName, WKSecurityOriginRef securityOrigin, quint64 currentQuota, quint64 currentOriginUsage, quint64 currentDatabaseUsage, quint64 expectedUsage);
    void runJavaScriptAlert(const QString&);
    bool runJavaScriptConfirm(const QString&);
    QString runJavaScriptPrompt(const QString&, const QString& defaultValue, bool& ok);

    void handleAuthenticationRequiredRequest(const QString& hostname, const QString& realm, const QString& prefilledUsername, QString& username, QString& password);
    bool handleCertificateVerificationRequest(const QString& hostname);
    void handleProxyAuthenticationRequiredRequest(const QString& hostname, uint16_t port, const QString& prefilledUsername, QString& username, QString& password);

    void setRenderToOffscreenBuffer(bool enable) { m_renderToOffscreenBuffer = enable; }
    void setTransparentBackground(bool);
    void addAttachedPropertyTo(QObject*);

    bool navigatorQtObjectEnabled() const;
    bool renderToOffscreenBuffer() const { return m_renderToOffscreenBuffer; }
    bool transparentBackground() const;
    void setNavigatorQtObjectEnabled(bool);
    void updateUserScripts();
    void updateUserStyleSheets();
    void updateSchemeDelegates();

    QPointF contentPos() const;
    void setContentPos(const QPointF&);

    void updateIcon();

    // PageClient.
    WebCore::IntSize viewSize() const;
    virtual void pageDidRequestScroll(const QPoint& pos) { }
    void didRelaunchProcess();
    std::unique_ptr<WebKit::DrawingAreaProxy> createDrawingAreaProxy();
    void handleDownloadRequest(WebKit::DownloadProxy*);

    void didReceiveMessageFromNavigatorQtObject(WKStringRef message);
#if ENABLE(QT_WEBCHANNEL)
    void didReceiveMessageFromNavigatorQtWebChannelTransportObject(WKDataRef);
#endif

    WebKit::CoordinatedGraphicsScene* coordinatedGraphicsScene();
    float deviceScaleFactor();
    void setIntrinsicDeviceScaleFactor(float);

protected:
    class FlickableAxisLocker {
        QQuickFlickable::FlickableDirection m_allowedDirection;

        ulong m_time;
        QPointF m_initialPosition;
        QPointF m_lockReferencePosition;
        int m_sampleCount;

        QVector2D touchVelocity(const QTouchEvent* event);

    public:
        FlickableAxisLocker();

        void update(const QTouchEvent* event);
        void setReferencePosition(const QPointF&);
        void reset();
        QPointF adjust(const QPointF&);
    };

    // WKPageLoadClient callbacks.
    static void didStartProvisionalLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didReceiveServerRedirectForProvisionalLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didFailLoad(WKPageRef, WKFrameRef, WKErrorRef, WKTypeRef userData, const void* clientInfo);
    static void didCommitLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didSameDocumentNavigationForFrame(WKPageRef, WKFrameRef, WKSameDocumentNavigationType, WKTypeRef userData, const void* clientInfo);
    static void didReceiveTitleForFrame(WKPageRef, WKStringRef, WKFrameRef, WKTypeRef userData, const void* clientInfo);
    static void didStartProgress(WKPageRef, const void* clientInfo);
    static void didChangeProgress(WKPageRef, const void* clientInfo);
    static void didFinishProgress(WKPageRef, const void* clientInfo);
    static void didChangeBackForwardList(WKPageRef, WKBackForwardListItemRef, WKArrayRef, const void *clientInfo);
    static void processDidBecomeUnresponsive(WKPageRef, const void* clientInfo);
    static void processDidBecomeResponsive(WKPageRef, const void* clientInfo);
    static void processDidCrash(WKPageRef, const void* clientInfo);

    QQuickWebViewPrivate(QQuickWebView* viewport);
    RefPtr<WebKit::WebPageProxy> webPageProxy;
    WKRetainPtr<WKPageRef> webPage;

    WebKit::QtPageClient pageClient;
    WebKit::DefaultUndoController undoController;
    std::unique_ptr<QWebNavigationHistory> navigationHistory;
    std::unique_ptr<QWebPreferences> preferences;

    QScopedPointer<WebKit::QtWebPagePolicyClient> pagePolicyClient;
    QScopedPointer<WebKit::QtWebPageUIClient> pageUIClient;

    QScopedPointer<QQuickWebPage> pageView;
    QScopedPointer<WebKit::QtWebPageEventHandler> pageEventHandler;
    QQuickWebView* q_ptr;
    QQuickWebViewExperimental* experimental;
    WebKit::QtWebContext* context;

    FlickableAxisLocker axisLocker;

    QQmlComponent* alertDialog;
    QQmlComponent* confirmDialog;
    QQmlComponent* promptDialog;
    QQmlComponent* authenticationDialog;
    QQmlComponent* certificateVerificationDialog;
    QQmlComponent* itemSelector;
    QQmlComponent* proxyAuthenticationDialog;
    QQmlComponent* filePicker;
    QQmlComponent* databaseQuotaDialog;
    QQmlComponent* colorChooser;

    QList<QUrl> userScripts;
    QList<QUrl> userStyleSheets;

    bool m_betweenLoadCommitAndFirstFrame;
    bool m_useDefaultContentItemSize;
    bool m_navigatorQtObjectEnabled;
    bool m_renderToOffscreenBuffer;
    bool m_allowAnyHTTPSCertificateForLocalHost;
    QUrl m_iconUrl;
    int m_loadProgress;
    QString m_currentUrl;
};

class QQuickWebViewLegacyPrivate : public QQuickWebViewPrivate {
    Q_DECLARE_PUBLIC(QQuickWebView)
public:
    QQuickWebViewLegacyPrivate(QQuickWebView* viewport);
    void initialize(WKPageConfigurationRef configurationRef = 0) Q_DECL_OVERRIDE;

    void updateViewportSize() Q_DECL_OVERRIDE;

    qreal zoomFactor() const Q_DECL_OVERRIDE;
    void setZoomFactor(qreal) Q_DECL_OVERRIDE;
};

class QQuickWebViewFlickablePrivate : public QQuickWebViewPrivate {
    Q_DECLARE_PUBLIC(QQuickWebView)
public:
    QQuickWebViewFlickablePrivate(QQuickWebView* viewport);
    void initialize(WKPageConfigurationRef configurationRef = 0) Q_DECL_OVERRIDE;

    void onComponentComplete() Q_DECL_OVERRIDE;

    void didChangeViewportProperties(const WebCore::ViewportAttributes&) Q_DECL_OVERRIDE;
    WebKit::PageViewportController* viewportController() const Q_DECL_OVERRIDE { return m_pageViewportController.data(); }
    void updateViewportSize() Q_DECL_OVERRIDE;

    void pageDidRequestScroll(const QPoint& pos) Q_DECL_OVERRIDE;
    void handleMouseEvent(QMouseEvent*) Q_DECL_OVERRIDE;

private:
    QScopedPointer<WebKit::PageViewportController> m_pageViewportController;
    QScopedPointer<WebKit::PageViewportControllerClientQt> m_pageViewportControllerClient;
};

#endif // qquickwebview_p_p_h
