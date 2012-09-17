/*
 * Copyright (C) 2011, 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Benjamin Poulain <benjamin@webkit.org>
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

#ifndef PageViewportControllerClientQt_h
#define PageViewportControllerClientQt_h

#include "PageViewportController.h"
#include "PageViewportControllerClient.h"
#include <QObject>
#include <QPointF>
#include <QScopedPointer>
#include <QVariant>
#include <QVariantAnimation>

QT_BEGIN_NAMESPACE
class QRectF;
QT_END_NAMESPACE

class QQuickWebPage;
class QQuickWebView;
class QWebKitTest;

namespace WebKit {

class PageViewportControllerClientQt : public QObject, public PageViewportControllerClient {
    Q_OBJECT

public:
    PageViewportControllerClientQt(QQuickWebView*, QQuickWebPage*);
    ~PageViewportControllerClientQt();

    virtual void setContentsPosition(const WebCore::FloatPoint& localPoint);
    virtual void setContentsScale(float localScale, bool treatAsInitialValue);

    virtual void didResumeContent();
    virtual void didChangeContentsSize();
    virtual void didChangeVisibleContents();
    virtual void didChangeViewportAttributes();

    virtual void setController(PageViewportController* controller) { m_controller = controller; }

    // Additional methods currently only relevant in the QQuick context.
    void touchBegin();
    void touchEnd();

    bool scrollAnimationActive() const;
    void cancelScrollAnimation();

    bool panGestureActive() const;
    void panGestureStarted(const QPointF& position, qint64 eventTimestampMillis);
    void panGestureRequestUpdate(const QPointF& position, qint64 eventTimestampMillis);
    void panGestureEnded(const QPointF& position, qint64 eventTimestampMillis);
    void panGestureCancelled();

    bool scaleAnimationActive() const;
    void interruptScaleAnimation();

    bool pinchGestureActive() const;
    void pinchGestureStarted(const QPointF& pinchCenterInViewportCoordinates);
    void pinchGestureRequestUpdate(const QPointF& pinchCenterInViewportCoordinates, qreal totalScaleFactor);
    void pinchGestureEnded();
    void pinchGestureCancelled();

    void zoomToAreaGestureEnded(const QPointF& touchPoint, const QRectF& targetArea);
    void focusEditableArea(const QRectF& caretArea, const QRectF& targetArea);

private Q_SLOTS:
    // Respond to changes of position that are not driven by us.
    void pageItemPositionChanged();

    void scaleAnimationStateChanged(QAbstractAnimation::State, QAbstractAnimation::State);

    void flickMoveStarted(); // Called when panning starts.
    void flickMoveEnded(); //   Called when panning (+ kinetic animation) ends.

private:
    class ScaleAnimation : public QVariantAnimation {
        PageViewportControllerClientQt* m_controllerClient;
    public:
        ScaleAnimation(PageViewportControllerClientQt* parent)
            : QVariantAnimation(parent)
            , m_controllerClient(parent)
        { }

        virtual void updateCurrentValue(const QVariant&);
    };

    struct ScaleStackItem {
        ScaleStackItem(qreal scale, qreal xPosition)
            : scale(scale)
            , xPosition(xPosition)
        { }

        qreal scale;
        qreal xPosition;
    };

    friend class ScaleAnimation;
    friend class ::QWebKitTest;

    PageViewportController* m_controller;
    QQuickWebView* const m_viewportItem;
    QQuickWebPage* const m_pageItem;

    float viewportScaleForRect(const QRectF&) const;
    QRectF visibleContentsRect() const;
    QRectF nearestValidVisibleContentsRect() const;

    void setContentsRectToNearestValidBounds();
    void updateViewportController(const QPointF& trajectory = QPointF(), qreal scale = -1);
    void setContentRectVisiblePositionAtScale(const QPointF& location, qreal itemScale);
    void animateContentRectVisible(const QRectF& contentRect);
    void scaleContent(qreal itemScale, const QPointF& centerInCSSCoordinates = QPointF());

    QScopedPointer<ViewportUpdateDeferrer> m_scaleUpdateDeferrer;
    QScopedPointer<ViewportUpdateDeferrer> m_scrollUpdateDeferrer;
    QScopedPointer<ViewportUpdateDeferrer> m_touchUpdateDeferrer;
    QScopedPointer<ViewportUpdateDeferrer> m_animationUpdateDeferrer;

    ScaleAnimation* m_scaleAnimation;
    QPointF m_lastPinchCenterInViewportCoordinates;
    QPointF m_lastScrollPosition;
    qreal m_pinchStartScale;
    qreal m_lastCommittedScale;
    qreal m_zoomOutScale;
    QList<ScaleStackItem> m_scaleStack;
    bool m_ignoreViewportChanges;
};

} // namespace WebKit

#endif // PageViewportControllerClientQt_h
