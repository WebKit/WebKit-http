/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef QGraphicsWebView_h
#define QGraphicsWebView_h

#include <QtWebKit/qwebkitglobal.h>
#include <QtWebKitWidgets/qwebpage.h>
#include <QtCore/qurl.h>
#include <QtGui/qevent.h>
#include <QtGui/qicon.h>
#include <QtGui/qpainter.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtWidgets/qgraphicswidget.h>

#if !defined(QT_NO_GRAPHICSVIEW)

class QWebPage;
class QWebHistory;
class QWebSettings;

class QGraphicsWebViewPrivate;

class QWEBKITWIDGETS_EXPORT QGraphicsWebView : public QGraphicsWidget {
    Q_OBJECT

    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QIcon icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor)

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)

    Q_PROPERTY(bool modified READ isModified)
    Q_PROPERTY(bool resizesToContents READ resizesToContents WRITE setResizesToContents)
    Q_PROPERTY(bool tiledBackingStoreFrozen READ isTiledBackingStoreFrozen WRITE setTiledBackingStoreFrozen)

    Q_PROPERTY(QPainter::RenderHints renderHints READ renderHints WRITE setRenderHints)
    Q_FLAGS(QPainter::RenderHints)

public:
    explicit QGraphicsWebView(QGraphicsItem* parent = Q_NULLPTR);
    ~QGraphicsWebView();

    QWebPage* page() const;
    void setPage(QWebPage*);

    QUrl url() const;
    void setUrl(const QUrl&);

    QString title() const;
    QIcon icon() const;

    qreal zoomFactor() const;
    void setZoomFactor(qreal);

    bool isModified() const;

    void load(const QUrl& url);
    void load(const QNetworkRequest& request, QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation, const QByteArray& body = QByteArray());

    void setHtml(const QString& html, const QUrl& baseUrl = QUrl());
    // FIXME: Consider rename to setHtml?
    void setContent(const QByteArray& data, const QString& mimeType = QString(), const QUrl& baseUrl = QUrl());

    QWebHistory* history() const;
    QWebSettings* settings() const;

    QAction* pageAction(QWebPage::WebAction action) const;
    void triggerPageAction(QWebPage::WebAction action, bool checked = false);

    bool findText(const QString& subString, QWebPage::FindFlags options = QWebPage::FindFlags());

    bool resizesToContents() const;
    void setResizesToContents(bool enabled);
    
    bool isTiledBackingStoreFrozen() const;
    void setTiledBackingStoreFrozen(bool frozen);

    void setGeometry(const QRectF& rect) Q_DECL_OVERRIDE;
    void updateGeometry() Q_DECL_OVERRIDE;
    void paint(QPainter*, const QStyleOptionGraphicsItem* options, QWidget* widget = Q_NULLPTR) Q_DECL_OVERRIDE;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) Q_DECL_OVERRIDE;
    bool event(QEvent*) Q_DECL_OVERRIDE;

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint) const Q_DECL_OVERRIDE;

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const Q_DECL_OVERRIDE;

    QPainter::RenderHints renderHints() const;
    void setRenderHints(QPainter::RenderHints);
    void setRenderHint(QPainter::RenderHint, bool enabled = true);

public Q_SLOTS:
    void stop();
    void back();
    void forward();
    void reload();

Q_SIGNALS:
    void loadStarted();
    void loadFinished(bool);

    void loadProgress(int progress);
    void urlChanged(const QUrl&);
    void titleChanged(const QString&);
    void iconChanged();
    void statusBarMessage(const QString& message);
    void linkClicked(const QUrl&);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) Q_DECL_OVERRIDE;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) Q_DECL_OVERRIDE;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) Q_DECL_OVERRIDE;
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QGraphicsSceneWheelEvent*) Q_DECL_OVERRIDE;
#endif
    void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QGraphicsSceneContextMenuEvent*) Q_DECL_OVERRIDE;
#endif
    void dragEnterEvent(QGraphicsSceneDragDropEvent*) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent*) Q_DECL_OVERRIDE;
    void dragMoveEvent(QGraphicsSceneDragDropEvent*) Q_DECL_OVERRIDE;
    void dropEvent(QGraphicsSceneDragDropEvent*) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent*) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent*) Q_DECL_OVERRIDE;
    void inputMethodEvent(QInputMethodEvent*) Q_DECL_OVERRIDE;
    bool focusNextPrevChild(bool next) Q_DECL_OVERRIDE;

    bool sceneEvent(QEvent*) Q_DECL_OVERRIDE;

private:
    Q_PRIVATE_SLOT(d, void _q_doLoadFinished(bool success))
    Q_PRIVATE_SLOT(d, void _q_pageDestroyed())
    Q_PRIVATE_SLOT(d, void _q_contentsSizeChanged(const QSize&))
    Q_PRIVATE_SLOT(d, void _q_scaleChanged())

    QGraphicsWebViewPrivate* const d;
    friend class QGraphicsWebViewPrivate;
};

#endif // QT_NO_GRAPHICSVIEW

#endif // QGraphicsWebView_h
