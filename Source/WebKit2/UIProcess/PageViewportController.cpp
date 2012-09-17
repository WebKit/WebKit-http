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

#include "config.h"
#include "PageViewportController.h"

#include "PageViewportControllerClient.h"
#include "WebPageProxy.h"
#include <WebCore/FloatRect.h>
#include <WebCore/FloatSize.h>
#include <wtf/MathExtras.h>

using namespace WebCore;

namespace WebKit {

static inline float bound(float min, float value, float max)
{
    return clampTo<float>(value, min, max);
}

bool fuzzyCompare(float a, float b, float epsilon)
{
    return std::abs(a - b) < epsilon;
}

FloatPoint boundPosition(const FloatPoint minPosition, const FloatPoint& position, const FloatPoint& maxPosition)
{
    return FloatPoint(bound(minPosition.x(), position.x(), maxPosition.x()), bound(minPosition.y(), position.y(), maxPosition.y()));
}

ViewportUpdateDeferrer::ViewportUpdateDeferrer(PageViewportController* PageViewportController, SuspendContentFlag suspendContentFlag)
    : m_controller(PageViewportController)
{
    m_controller->m_activeDeferrerCount++;

    // There is no need to suspend content for immediate updates
    // only during animations or longer gestures.
    if (suspendContentFlag == DeferUpdateAndSuspendContent)
        m_controller->suspendContent();
}

ViewportUpdateDeferrer::~ViewportUpdateDeferrer()
{
    if (--(m_controller->m_activeDeferrerCount))
        return;

    m_controller->resumeContent();

    // Make sure that tiles all around the viewport will be requested.
    m_controller->syncVisibleContents();
}

PageViewportController::PageViewportController(WebKit::WebPageProxy* proxy, PageViewportControllerClient* client)
    : m_webPageProxy(proxy)
    , m_client(client)
    , m_allowsUserScaling(false)
    , m_minimumScale(1)
    , m_maximumScale(1)
    , m_devicePixelRatio(1)
    , m_activeDeferrerCount(0)
    , m_hasSuspendedContent(false)
    , m_hadUserInteraction(false)
    , m_effectiveScale(1)
{
    // Initializing Viewport Raw Attributes to avoid random negative scale factors
    // if there is a race condition between the first layout and setting the viewport attributes for the first time.
    m_rawAttributes.initialScale = 1;
    m_rawAttributes.minimumScale = m_minimumScale;
    m_rawAttributes.maximumScale = m_maximumScale;
    m_rawAttributes.userScalable = m_allowsUserScaling;

    ASSERT(m_client);
    m_client->setController(this);
}

FloatRect PageViewportController::convertToViewport(const FloatRect& cssRect) const
{
    return FloatRect(
        convertToViewport(cssRect.x()),
        convertToViewport(cssRect.y()),
        convertToViewport(cssRect.width()),
        convertToViewport(cssRect.height())
    );
}

float PageViewportController::innerBoundedContentsScale(float cssScale) const
{
    return bound(m_minimumScale, cssScale, m_maximumScale);
}

float PageViewportController::outerBoundedContentsScale(float cssScale) const
{
    if (m_allowsUserScaling) {
        // Bounded by [0.1, 10.0] like the viewport meta code in WebCore.
        float hardMin = std::max<float>(0.1, 0.5 * m_minimumScale);
        float hardMax = std::min<float>(10, 2 * m_maximumScale);
        return bound(hardMin, cssScale, hardMax);
    }
    return innerBoundedContentsScale(cssScale);
}

void PageViewportController::didChangeContentsSize(const IntSize& newSize)
{
    if (m_viewportSize.isEmpty() || newSize.isEmpty())
        return;

    m_contentsSize = newSize;

    float minimumScale = WebCore::computeMinimumScaleFactorForContentContained(m_rawAttributes, WebCore::roundedIntSize(m_viewportSize), newSize);

    if (!fuzzyCompare(minimumScale, m_rawAttributes.minimumScale, 0.001)) {
        m_minimumScale = minimumScale;

        if (!m_hadUserInteraction && !hasSuspendedContent())
            m_client->setContentsScale(convertToViewport(minimumScale), true /* isInitialScale */);

        m_client->didChangeViewportAttributes();
    }

    m_client->didChangeContentsSize();
}

void PageViewportController::pageDidRequestScroll(const IntPoint& cssPosition)
{
    // Ignore the request if suspended. Can only happen due to delay in event delivery.
    if (m_activeDeferrerCount)
        return;

    FloatRect endPosRange = positionRangeForContentAtScale(m_effectiveScale);
    FloatPoint endPosition(cssPosition);
    endPosition.scale(m_effectiveScale, m_effectiveScale);
    endPosition = boundPosition(endPosRange.minXMinYCorner(), endPosition, endPosRange.maxXMaxYCorner());

    m_client->setContentsPosition(endPosition);
}

void PageViewportController::setViewportSize(const FloatSize& newSize)
{
    if (newSize.isEmpty())
        return;

    m_viewportSize = newSize;

    // Let the WebProcess know about the new viewport size, so that
    // it can resize the content accordingly.
    m_webPageProxy->setViewportSize(roundedIntSize(newSize));

    syncVisibleContents();
}

void PageViewportController::setVisibleContentsRect(const FloatRect& visibleContentsRect, float viewportScale, const FloatPoint& trajectoryVector)
{
    m_visibleContentsRect = visibleContentsRect;
    m_effectiveScale = viewportScale;
    syncVisibleContents(trajectoryVector);
}

void PageViewportController::syncVisibleContents(const FloatPoint& trajectoryVector)
{
    DrawingAreaProxy* const drawingArea = m_webPageProxy->drawingArea();
    if (!drawingArea || m_viewportSize.isEmpty() || m_contentsSize.isEmpty() || m_visibleContentsRect.isEmpty())
        return;

    drawingArea->setVisibleContentsRect(m_visibleContentsRect, m_effectiveScale, trajectoryVector);

    m_client->didChangeVisibleContents();
}

void PageViewportController::didChangeViewportAttributes(const WebCore::ViewportAttributes& newAttributes)
{
    if (newAttributes.layoutSize.isEmpty())
        return;

    m_rawAttributes = newAttributes;
    WebCore::restrictScaleFactorToInitialScaleIfNotUserScalable(m_rawAttributes);

    m_devicePixelRatio = m_webPageProxy->deviceScaleFactor();
    m_allowsUserScaling = !!m_rawAttributes.userScalable;
    m_minimumScale = m_rawAttributes.minimumScale;
    m_maximumScale = m_rawAttributes.maximumScale;

    m_client->didChangeViewportAttributes();
}

void PageViewportController::suspendContent()
{
    if (m_hasSuspendedContent)
        return;

    m_hasSuspendedContent = true;
    m_webPageProxy->suspendActiveDOMObjectsAndAnimations();
}

void PageViewportController::resumeContent()
{
    if (!m_rawAttributes.layoutSize.isEmpty() && m_rawAttributes.initialScale > 0) {
        m_hadUserInteraction = false;
        m_client->setContentsScale(convertToViewport(innerBoundedContentsScale(m_rawAttributes.initialScale)), /* isInitialScale */ true);
        m_rawAttributes.initialScale = -1; // Mark used.
    }

    m_client->didResumeContent();

    if (!m_hasSuspendedContent)
        return;

    m_hasSuspendedContent = false;
    m_webPageProxy->resumeActiveDOMObjectsAndAnimations();
}

FloatRect PageViewportController::positionRangeForContentAtScale(float viewportScale) const
{
    const FloatSize contentSize = m_contentsSize * viewportScale;

    const float horizontalRange = contentSize.width() - m_viewportSize.width();
    const float verticalRange = contentSize.height() - m_viewportSize.height();

    return FloatRect(0, 0, horizontalRange, verticalRange);
}

} // namespace WebKit
