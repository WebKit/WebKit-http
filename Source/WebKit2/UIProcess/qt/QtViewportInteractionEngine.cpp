/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "QtViewportInteractionEngine.h"

#include "qquickwebpage_p.h"
#include "qquickwebview_p.h"
#include <QPointF>
#include <QTransform>
#include <QWheelEvent>
#include <QtQuick/qquickitem.h>
#include <wtf/PassOwnPtr.h>

namespace WebKit {

static const int kScaleAnimationDurationMillis = 250;

// UPDATE DEFERRING (SUSPEND/RESUME)
// =================================
//
// When interaction with the content, either by animating or by the hand of the user,
// it is important to ensure smooth animations of at least 60fps in order to give a
// good user experience.
//
// In order to do this we need to get rid of unknown factors. These include device
// sensors (geolocation, orientation updates etc), CSS3 animations, JavaScript
// exectution, sub resource loads etc. We do this by emitting suspend and resume
// signals, which are then handled by the viewport and propagates to the right place.
//
// For this reason the ViewportUpdateDeferrer guard must be used when we interact
// or animate the content.
//
// It should be noted that when we update content properties, we might receive notify
// signals send my the content item itself, and care should be taken to not act on
// these unconditionally. An example of this is the pinch zoom, which changes the
// position and will thus result in a QQuickWebPage::geometryChanged() signal getting
// emitted.
//
// If something should only be executed during update deferring, it is possible to
// check for that using ASSERT(m_suspendCount).

class ViewportUpdateDeferrer {
public:
    enum SuspendContentFlag { DeferUpdate, DeferUpdateAndSuspendContent };
    ViewportUpdateDeferrer(QtViewportInteractionEngine* engine, SuspendContentFlag suspendContentFlag = DeferUpdate)
        : engine(engine)
    {
        engine->m_suspendCount++;

        // There is no need to suspend content for immediate updates
        // only during animations or longer gestures.
        if (suspendContentFlag == DeferUpdateAndSuspendContent && !engine->m_hasSuspendedContent) {
            engine->m_hasSuspendedContent = true;
            emit engine->contentSuspendRequested();
        }
    }

    ~ViewportUpdateDeferrer()
    {
        if (--(engine->m_suspendCount))
            return;

        if (engine->m_hasSuspendedContent) {
            engine->m_hasSuspendedContent = false;
            emit engine->contentResumeRequested();
        }

        // Make sure that tiles all around the viewport will be requested.
        emit engine->informVisibleContentChange(QPointF());
    }

private:
    QtViewportInteractionEngine* const engine;
};

// A floating point compare with absolute error.
static inline bool fuzzyCompare(qreal a, qreal b, qreal epsilon)
{
    return qAbs(a - b) < epsilon;
}

inline qreal QtViewportInteractionEngine::cssScaleFromItem(qreal itemScale) const
{
    return itemScale / m_devicePixelRatio;
}

inline qreal QtViewportInteractionEngine::itemScaleFromCSS(qreal cssScale) const
{
    return cssScale * m_devicePixelRatio;
}

inline qreal QtViewportInteractionEngine::itemCoordFromCSS(qreal value) const
{
    return value * m_devicePixelRatio;
}

inline QRectF QtViewportInteractionEngine::itemRectFromCSS(const QRectF& cssRect) const
{
    QRectF itemRect;

    itemRect.setX(itemCoordFromCSS(cssRect.x()));
    itemRect.setY(itemCoordFromCSS(cssRect.y()));
    itemRect.setWidth(itemCoordFromCSS(cssRect.width()));
    itemRect.setHeight(itemCoordFromCSS(cssRect.height()));

    return itemRect;
}

QtViewportInteractionEngine::QtViewportInteractionEngine(QQuickWebView* viewportItem, QQuickWebPage* pageItem)
    : m_viewportItem(viewportItem)
    , m_pageItem(pageItem)
    , m_suspendCount(0)
    , m_hasSuspendedContent(false)
    , m_hadUserInteraction(false)
    , m_scaleAnimation(new ScaleAnimation(this))
    , m_pinchStartScale(-1)
    , m_zoomOutScale(0.0)
{
    reset();

    connect(m_pageItem, SIGNAL(widthChanged()), SLOT(itemSizeChanged()), Qt::DirectConnection);
    connect(m_pageItem, SIGNAL(heightChanged()), SLOT(itemSizeChanged()), Qt::DirectConnection);
    connect(m_viewportItem, SIGNAL(movementStarted()), SLOT(flickableMoveStarted()), Qt::DirectConnection);
    connect(m_viewportItem, SIGNAL(movementEnded()), SLOT(flickableMoveEnded()), Qt::DirectConnection);

    connect(m_scaleAnimation, SIGNAL(valueChanged(QVariant)),
            SLOT(scaleAnimationValueChanged(QVariant)), Qt::DirectConnection);
    connect(m_scaleAnimation, SIGNAL(stateChanged(QAbstractAnimation::State, QAbstractAnimation::State)),
            SLOT(scaleAnimationStateChanged(QAbstractAnimation::State, QAbstractAnimation::State)), Qt::DirectConnection);
}

QtViewportInteractionEngine::~QtViewportInteractionEngine()
{
}

qreal QtViewportInteractionEngine::innerBoundedCSSScale(qreal cssScale) const
{
    return qBound(m_minimumScale, cssScale, m_maximumScale);
}

qreal QtViewportInteractionEngine::outerBoundedCSSScale(qreal cssScale) const
{
    if (m_allowsUserScaling) {
        // Bounded by [0.1, 10.0] like the viewport meta code in WebCore.
        qreal hardMin = qMax<qreal>(0.1, qreal(0.5) * m_minimumScale);
        qreal hardMax = qMin<qreal>(10, qreal(2.0) * m_maximumScale);
        return qBound(hardMin, cssScale, hardMax);
    }
    return innerBoundedCSSScale(cssScale);
}

void QtViewportInteractionEngine::setItemRectVisible(const QRectF& itemRect)
{
    ASSERT_WITH_MESSAGE(m_suspendCount,
        "setItemRectVisible has to be guarded using a ViewportUpdateDeferrer.");

    if (itemRect.isEmpty())
        return;

    qreal itemScale = m_viewportItem->width() / itemRect.width();

    m_pageItem->setContentsScale(itemScale);

    // To animate the position together with the scale we multiply the position with the current scale
    // and add it to the page position (displacement on the flickable contentItem because of additional items).
    QPointF newPosition(m_pageItem->pos() + (itemRect.topLeft() * itemScale));

    m_viewportItem->setContentPos(newPosition);
}

// Ease out overshoot of 1.25 combined with ease in correction of 0.25. Both quadratic to have physical motion.
static qreal physicalOvershoot(qreal t)
{
    return (-t * (t - 2)) * 1.25 - (t * t) * 0.25;
}

void QtViewportInteractionEngine::animateItemRectVisible(const QRectF& itemRect)
{
    ASSERT(m_scaleAnimation->state() == QAbstractAnimation::Stopped);

    ASSERT(!scrollAnimationActive());
    if (scrollAnimationActive())
        return;

    QRectF currentItemRectVisible = m_viewportItem->mapRectToWebContent(m_viewportItem->boundingRect());
    if (itemRect == currentItemRectVisible)
        return;

    QEasingCurve easingCurve;
    easingCurve.setCustomType(physicalOvershoot);

    m_scaleAnimation->setDuration(kScaleAnimationDurationMillis);
    m_scaleAnimation->setEasingCurve(easingCurve);

    m_scaleAnimation->setStartValue(currentItemRectVisible);
    m_scaleAnimation->setEndValue(itemRect);

    m_scaleAnimation->start();
}

void QtViewportInteractionEngine::flickableMoveStarted()
{
    Q_ASSERT(m_viewportItem->isMoving());
    m_scrollUpdateDeferrer = adoptPtr(new ViewportUpdateDeferrer(this, ViewportUpdateDeferrer::DeferUpdateAndSuspendContent));

    m_lastScrollPosition = m_viewportItem->contentPos();
    connect(m_viewportItem, SIGNAL(contentXChanged()), SLOT(flickableMovingPositionUpdate()));
    connect(m_viewportItem, SIGNAL(contentYChanged()), SLOT(flickableMovingPositionUpdate()));
}

void QtViewportInteractionEngine::flickableMoveEnded()
{
    Q_ASSERT(!m_viewportItem->isMoving());
    // This method is called on the end of the pan or pan kinetic animation.
    m_scrollUpdateDeferrer.clear();

    m_lastScrollPosition = QPointF();
    disconnect(m_viewportItem, SIGNAL(contentXChanged()), this, SLOT(flickableMovingPositionUpdate()));
    disconnect(m_viewportItem, SIGNAL(contentYChanged()), this, SLOT(flickableMovingPositionUpdate()));
}

void QtViewportInteractionEngine::flickableMovingPositionUpdate()
{
    QPointF newPosition = m_viewportItem->contentPos();

    emit informVisibleContentChange(m_lastScrollPosition - newPosition);

    m_lastScrollPosition = newPosition;
}

void QtViewportInteractionEngine::scaleAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State /*oldState*/)
{
    switch (newState) {
    case QAbstractAnimation::Running:
        m_viewportItem->cancelFlick();
        ASSERT(!m_animationUpdateDeferrer);
        m_animationUpdateDeferrer = adoptPtr(new ViewportUpdateDeferrer(this, ViewportUpdateDeferrer::DeferUpdateAndSuspendContent));
        break;
    case QAbstractAnimation::Stopped:
        m_animationUpdateDeferrer.clear();
        break;
    default:
        break;
    }
}

static inline QPointF boundPosition(const QPointF minPosition, const QPointF& position, const QPointF& maxPosition)
{
    return QPointF(qBound(minPosition.x(), position.x(), maxPosition.x()),
                   qBound(minPosition.y(), position.y(), maxPosition.y()));
}

void QtViewportInteractionEngine::pagePositionRequest(const QPoint& pagePosition)
{
    // Ignore the request if suspended. Can only happen due to delay in event delivery.
    if (m_suspendCount)
        return;

    qreal endItemScale = m_pageItem->contentsScale(); // Stay at same scale.

    QRectF endPosRange = computePosRangeForItemAtScale(endItemScale);
    QPointF endPosition = boundPosition(endPosRange.topLeft(), pagePosition * endItemScale, endPosRange.bottomRight());

    QRectF endVisibleContentRect(endPosition / endItemScale, m_viewportItem->boundingRect().size() / endItemScale);

    ViewportUpdateDeferrer guard(this);
    setItemRectVisible(endVisibleContentRect);
}

void QtViewportInteractionEngine::touchBegin()
{
    m_hadUserInteraction = true;

    // Prevents resuming the page between the user's flicks of the page while the animation is running.
    if (scrollAnimationActive())
        m_touchUpdateDeferrer = adoptPtr(new ViewportUpdateDeferrer(this, ViewportUpdateDeferrer::DeferUpdateAndSuspendContent));
}

void QtViewportInteractionEngine::touchEnd()
{
    m_touchUpdateDeferrer.clear();
}

QRectF QtViewportInteractionEngine::computePosRangeForItemAtScale(qreal itemScale) const
{
    const QSizeF contentItemSize = m_pageItem->contentsSize() * itemScale;
    const QSizeF viewportItemSize = m_viewportItem->boundingRect().size();

    const qreal horizontalRange = contentItemSize.width() - viewportItemSize.width();
    const qreal verticalRange = contentItemSize.height() - viewportItemSize.height();

    return QRectF(QPointF(0, 0), QSizeF(horizontalRange, verticalRange));
}

void QtViewportInteractionEngine::focusEditableArea(const QRectF& caretArea, const QRectF& targetArea)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_hadUserInteraction);

    QRectF endArea = itemRectFromCSS(targetArea);

    qreal endItemScale = itemScaleFromCSS(innerBoundedCSSScale(2.0));
    const QRectF viewportRect = m_viewportItem->boundingRect();

    qreal x;
    const qreal borderOffset = 10;
    if ((endArea.width() + borderOffset) * endItemScale <= viewportRect.width()) {
        // Center the input field in the middle of the view, if it is smaller than
        // the view at the scale target.
        x = viewportRect.center().x() - endArea.width() * endItemScale / 2.0;
    } else {
        // Ensure that the caret always has borderOffset contents pixels to the right
        // of it, and secondarily (if possible), that the area has borderOffset
        // contents pixels to the left of it.
        qreal caretOffset = itemCoordFromCSS(caretArea.x()) - endArea.x();
        x = qMin(viewportRect.width() - (caretOffset + borderOffset) * endItemScale, borderOffset * endItemScale);
    }

    const QPointF hotspot = QPointF(endArea.x(), endArea.center().y());
    const QPointF viewportHotspot = QPointF(x, /* FIXME: visibleCenter */ viewportRect.center().y());

    QPointF endPosition = hotspot * endItemScale - viewportHotspot;
    QRectF endPosRange = computePosRangeForItemAtScale(endItemScale);

    endPosition = boundPosition(endPosRange.topLeft(), endPosition, endPosRange.bottomRight());

    QRectF endVisibleContentRect(endPosition / endItemScale, viewportRect.size() / endItemScale);

    animateItemRectVisible(endVisibleContentRect);
}

void QtViewportInteractionEngine::zoomToAreaGestureEnded(const QPointF& touchPoint, const QRectF& targetArea)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_hadUserInteraction);

    if (!targetArea.isValid())
        return;

    if (m_suspendCount)
        return;

    const int margin = 10; // We want at least a little bit of margin.
    QRectF endArea = itemRectFromCSS(targetArea.adjusted(-margin, -margin, margin, margin));

    const QRectF viewportRect = m_viewportItem->boundingRect();

    qreal targetCSSScale = viewportRect.size().width() / endArea.size().width();
    qreal endCSSScale = innerBoundedCSSScale(qMin(targetCSSScale, qreal(2.5)));
    qreal endItemScale = itemScaleFromCSS(endCSSScale);
    qreal currentScale = m_pageItem->contentsScale();

    // We want to end up with the target area filling the whole width of the viewport (if possible),
    // and centralized vertically where the user requested zoom. Thus our hotspot is the center of
    // the targetArea x-wise and the requested zoom position, y-wise.
    const QPointF hotspot = QPointF(endArea.center().x(), itemCoordFromCSS(touchPoint.y()));
    const QPointF viewportHotspot = viewportRect.center();

    QPointF endPosition = hotspot * endCSSScale - viewportHotspot;

    QRectF endPosRange = computePosRangeForItemAtScale(endItemScale);
    endPosition = boundPosition(endPosRange.topLeft(), endPosition, endPosRange.bottomRight());

    QRectF endVisibleContentRect(endPosition / endItemScale, viewportRect.size() / endItemScale);

    enum { ZoomIn, ZoomBack, ZoomOut, NoZoom } zoomAction = ZoomIn;

    if (!m_scaleStack.isEmpty()) {
        // Zoom back out if attempting to scale to the same current scale, or
        // attempting to continue scaling out from the inner most level.
        // Use fuzzy compare with a fixed error to be able to deal with largish differences due to pixel rounding.
        if (fuzzyCompare(endItemScale, currentScale, 0.01)) {
            // If moving the viewport would expose more of the targetRect and move at least 40 pixels, update position but do not scale out.
            QRectF currentContentRect(m_viewportItem->contentPos() / currentScale, viewportRect.size() / currentScale);
            QRectF targetIntersection = endVisibleContentRect.intersected(targetArea);
            if (!currentContentRect.contains(targetIntersection) && (qAbs(endVisibleContentRect.top() - currentContentRect.top()) >= 40 || qAbs(endVisibleContentRect.left() - currentContentRect.left()) >= 40))
                zoomAction = NoZoom;
            else
                zoomAction = ZoomBack;
        } else if (fuzzyCompare(endItemScale, m_zoomOutScale, 0.01))
            zoomAction = ZoomBack;
        else if (endItemScale < currentScale)
            zoomAction = ZoomOut;
    }

    switch (zoomAction) {
    case ZoomIn:
        m_scaleStack.append(ScaleStackItem(currentScale, m_viewportItem->contentPos().x()));
        m_zoomOutScale = endItemScale;
        break;
    case ZoomBack: {
        ScaleStackItem lastScale = m_scaleStack.takeLast();
        endItemScale = lastScale.scale;
        endCSSScale = cssScaleFromItem(lastScale.scale);
        // Recalculate endPosition and bound it according to new scale.
        endPosition.setY(hotspot.y() * endCSSScale - viewportHotspot.y());
        endPosition.setX(lastScale.xPosition);
        endPosRange = computePosRangeForItemAtScale(endItemScale);
        endPosition = boundPosition(endPosRange.topLeft(), endPosition, endPosRange.bottomRight());
        endVisibleContentRect = QRectF(endPosition / endItemScale, viewportRect.size() / endItemScale);
        break;
    }
    case ZoomOut:
        // Unstack all scale-levels deeper than the new level, so a zoom-back won't end up zooming in.
        while (!m_scaleStack.isEmpty() && m_scaleStack.last().scale >= endItemScale)
            m_scaleStack.removeLast();
        m_zoomOutScale = endItemScale;
        break;
    case NoZoom:
        break;
    }

    animateItemRectVisible(endVisibleContentRect);
}

QRectF QtViewportInteractionEngine::nearestValidBounds() const
{
    qreal endItemScale = itemScaleFromCSS(innerBoundedCSSScale(currentCSSScale()));

    const QRectF viewportRect = m_viewportItem->boundingRect();
    QPointF viewportHotspot = viewportRect.center();

    QPointF endPosition = m_viewportItem->mapToWebContent(viewportHotspot) * endItemScale - viewportHotspot;

    QRectF endPosRange = computePosRangeForItemAtScale(endItemScale);
    endPosition = boundPosition(endPosRange.topLeft(), endPosition, endPosRange.bottomRight());

    QRectF endVisibleContentRect(endPosition / endItemScale, viewportRect.size() / endItemScale);

    return endVisibleContentRect;
}

void QtViewportInteractionEngine::reset()
{
    ASSERT(!m_suspendCount);

    m_hadUserInteraction = false;

    m_allowsUserScaling = false;
    m_minimumScale = 1;
    m_maximumScale = 1;
    m_devicePixelRatio = 1;
    m_pinchStartScale = -1;
    m_zoomOutScale = 0.0;

    m_viewportItem->cancelFlick();
    m_scaleAnimation->stop();
    m_scaleUpdateDeferrer.clear();
    m_scrollUpdateDeferrer.clear();
    m_scaleStack.clear();
}

void QtViewportInteractionEngine::setCSSScaleBounds(qreal minimum, qreal maximum)
{
    m_minimumScale = minimum;
    m_maximumScale = maximum;
}

void QtViewportInteractionEngine::setCSSScale(qreal scale)
{
    ViewportUpdateDeferrer guard(this);

    qreal newScale = innerBoundedCSSScale(scale);
    m_pageItem->setContentsScale(itemScaleFromCSS(newScale));
}

qreal QtViewportInteractionEngine::currentCSSScale() const
{
    return cssScaleFromItem(m_pageItem->contentsScale());
}

bool QtViewportInteractionEngine::scrollAnimationActive() const
{
    return m_viewportItem->isFlicking();
}

bool QtViewportInteractionEngine::panGestureActive() const
{
    return m_viewportItem->isDragging();
}

void QtViewportInteractionEngine::panGestureStarted(const QPointF& position, qint64 eventTimestampMillis)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_hadUserInteraction);

    m_viewportItem->handleFlickableMousePress(position, eventTimestampMillis);
    m_lastPinchCenterInViewportCoordinates = position;
}

void QtViewportInteractionEngine::panGestureRequestUpdate(const QPointF& position, qint64 eventTimestampMillis)
{
    m_viewportItem->handleFlickableMouseMove(position, eventTimestampMillis);
    m_lastPinchCenterInViewportCoordinates = position;
}

void QtViewportInteractionEngine::panGestureEnded(const QPointF& position, qint64 eventTimestampMillis)
{
    m_viewportItem->handleFlickableMouseRelease(position, eventTimestampMillis);
    m_lastPinchCenterInViewportCoordinates = position;
}

void QtViewportInteractionEngine::panGestureCancelled()
{
    // Reset the velocity samples of the flickable.
    // This should only be called by the recognizer if we have a recognized
    // pan gesture and receive a touch event with multiple touch points
    // (ie. transition to a pinch gesture) as it does not move the content
    // back inside valid bounds.
    // When the pinch gesture ends, the content is positioned and scaled
    // back to valid boundaries.
    m_viewportItem->cancelFlick();
}

bool QtViewportInteractionEngine::scaleAnimationActive() const
{
    return m_scaleAnimation->state() == QAbstractAnimation::Running;
}

void QtViewportInteractionEngine::cancelScrollAnimation()
{
    if (!scrollAnimationActive())
        return;

    // If the pan gesture recognizer receives a touch begin event
    // during an ongoing kinetic scroll animation of a previous
    // pan gesture, the animation is stopped and the content is
    // immediately positioned back to valid boundaries.

    m_viewportItem->cancelFlick();
    ViewportUpdateDeferrer guard(this);
    setItemRectVisible(nearestValidBounds());
}

void QtViewportInteractionEngine::interruptScaleAnimation()
{
    // This interrupts the scale animation exactly where it is, even if it is out of bounds.
    m_scaleAnimation->stop();
}

bool QtViewportInteractionEngine::pinchGestureActive() const
{
    return m_pinchStartScale > 0;
}

void QtViewportInteractionEngine::pinchGestureStarted(const QPointF& pinchCenterInViewportCoordinates)
{
    // This can only happen as a result of a user interaction.
    ASSERT(m_hadUserInteraction);

    if (!m_allowsUserScaling)
        return;

    m_scaleStack.clear();
    m_zoomOutScale = 0.0;

    m_scaleUpdateDeferrer = adoptPtr(new ViewportUpdateDeferrer(this, ViewportUpdateDeferrer::DeferUpdateAndSuspendContent));

    m_lastPinchCenterInViewportCoordinates = pinchCenterInViewportCoordinates;
    m_pinchStartScale = m_pageItem->contentsScale();
}

void QtViewportInteractionEngine::pinchGestureRequestUpdate(const QPointF& pinchCenterInViewportCoordinates, qreal totalScaleFactor)
{
    ASSERT(m_suspendCount);

    if (!m_allowsUserScaling)
        return;

    //  Changes of the center position should move the page even if the zoom factor
    //  does not change.
    const qreal cssScale = cssScaleFromItem(m_pinchStartScale * totalScaleFactor);

    // Allow zooming out beyond mimimum scale on pages that do not explicitly disallow it.
    const qreal targetCSSScale = outerBoundedCSSScale(cssScale);

    scaleContent(m_viewportItem->mapToWebContent(pinchCenterInViewportCoordinates), targetCSSScale);

    const QPointF positionDiff = pinchCenterInViewportCoordinates - m_lastPinchCenterInViewportCoordinates;
    m_lastPinchCenterInViewportCoordinates = pinchCenterInViewportCoordinates;

    m_viewportItem->setContentPos(m_viewportItem->contentPos() - positionDiff);
}

void QtViewportInteractionEngine::pinchGestureEnded()
{
    ASSERT(m_suspendCount);

    if (!m_allowsUserScaling)
        return;

    m_pinchStartScale = -1;

    animateItemRectVisible(nearestValidBounds());
    m_scaleUpdateDeferrer.clear(); // Clear after starting potential animation, which takes over deferring.
}

void QtViewportInteractionEngine::pinchGestureCancelled()
{
    m_pinchStartScale = -1;
    m_scaleUpdateDeferrer.clear();
}

/*
 * This is called for all changes of item scale, width or height.
 * This is called when interacting, ie. during for instance pinch-zooming.
 *
 * FIXME: This is currently called twice if you concurrently change width and height.
 */
void QtViewportInteractionEngine::itemSizeChanged()
{
    if (m_suspendCount)
        return;

    ViewportUpdateDeferrer guard(this);
    setItemRectVisible(nearestValidBounds());
}

void QtViewportInteractionEngine::scaleContent(const QPointF& centerInCSSCoordinates, qreal cssScale)
{
    QPointF oldPinchCenterOnViewport = m_viewportItem->mapFromWebContent(centerInCSSCoordinates);
    m_pageItem->setContentsScale(itemScaleFromCSS(cssScale));
    QPointF newPinchCenterOnViewport = m_viewportItem->mapFromWebContent(centerInCSSCoordinates);

    m_viewportItem->setContentPos(m_viewportItem->contentPos() + (newPinchCenterOnViewport - oldPinchCenterOnViewport));
}

} // namespace WebKit

#include "moc_QtViewportInteractionEngine.cpp"


