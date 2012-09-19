/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "BackingStore.h"

#include "BackingStoreClient.h"
#include "BackingStoreCompositingSurface.h"
#include "BackingStoreTile.h"
#include "BackingStore_p.h"
#include "FatFingers.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "InspectorController.h"
#include "InspectorInstrumentation.h"
#include "Page.h"
#include "SurfacePool.h"
#include "WebPage.h"
#include "WebPageClient.h"
#include "WebPageCompositorClient.h"
#include "WebPageCompositor_p.h"
#include "WebPage_p.h"
#include "WebSettings.h"

#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformIntRectRegion.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformMessage.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformScreen.h>
#include <BlackBerryPlatformSettings.h>
#include <BlackBerryPlatformWindow.h>

#include <SkImageDecoder.h>

#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/NotFound.h>

#define SUPPRESS_NON_VISIBLE_REGULAR_RENDER_JOBS 0
#define ENABLE_SCROLLBARS 1
#define ENABLE_REPAINTONSCROLL 1
#define DEBUG_BACKINGSTORE 0
#define DEBUG_CHECKERBOARD 0
#define DEBUG_WEBCORE_REQUESTS 0
#define DEBUG_VISUALIZE 0
#define DEBUG_TILEMATRIX 0
#define DEBUG_COMPOSITING_DIRTY_REGION 0

using namespace WebCore;
using namespace std;

using BlackBerry::Platform::Graphics::Window;
using BlackBerry::Platform::IntRect;
using BlackBerry::Platform::IntPoint;
using BlackBerry::Platform::IntSize;

namespace BlackBerry {
namespace WebKit {

WebPage* BackingStorePrivate::s_currentBackingStoreOwner = 0;

typedef std::pair<int, int> Divisor;
typedef Vector<Divisor> DivisorList;
// FIXME: Cache this and/or use a smarter algorithm.
static DivisorList divisors(unsigned n)
{
    DivisorList divisors;
    for (unsigned i = 1; i <= n; ++i)
        if (!(n % i))
            divisors.append(std::make_pair(i, n / i));
    return divisors;
}

static bool divisorIsPerfectWidth(Divisor divisor, Platform::IntSize size, int tileWidth)
{
    return size.width() <= divisor.first * tileWidth && abs(size.width() - divisor.first * tileWidth) < tileWidth;
}

static bool divisorIsPerfectHeight(Divisor divisor, Platform::IntSize size, int tileHeight)
{
    return size.height() <= divisor.second * tileHeight && abs(size.height() - divisor.second * tileHeight) < tileHeight;
}

static bool divisorIsPreferredDirection(Divisor divisor, BackingStorePrivate::TileMatrixDirection direction)
{
    if (direction == BackingStorePrivate::Vertical)
        return divisor.second > divisor.first;
    return divisor.first > divisor.second;
}

// Compute best divisor given the ratio determined by size.
static Divisor bestDivisor(Platform::IntSize size, int tileWidth, int tileHeight,
                           int minimumNumberOfTilesWide, int minimumNumberOfTilesHigh,
                           BackingStorePrivate::TileMatrixDirection direction)
{
    // The point of this function is to determine the number of tiles in each
    // dimension. We do this by looking to match the tile matrix width/height
    // ratio as closely as possible with the width/height ratio of the contents.
    // We also look at the direction passed to give preference to one dimension
    // over another. This method could probably be made faster, but it gets the
    // job done.
    SurfacePool* surfacePool = SurfacePool::globalSurfacePool();
    ASSERT(!surfacePool->isEmpty());

    // Store a static list of possible divisors.
    static DivisorList divisorList = divisors(surfacePool->size());

    // The ratio we're looking to best imitate.
    float ratio = static_cast<float>(size.width()) / static_cast<float>(size.height());

    Divisor bestDivisor;
    for (size_t i = 0; i < divisorList.size(); ++i) {
        Divisor divisor = divisorList[i];

        const bool isPerfectWidth = divisorIsPerfectWidth(divisor, size, tileWidth);
        const bool isPerfectHeight = divisorIsPerfectHeight(divisor, size, tileHeight);
        const bool isValidWidth = divisor.first >= minimumNumberOfTilesWide || isPerfectWidth;
        const bool isValidHeight = divisor.second >= minimumNumberOfTilesHigh || isPerfectHeight;
        if (!isValidWidth || !isValidHeight)
            continue;

        if (isPerfectWidth || isPerfectHeight) {
            bestDivisor = divisor; // Found a perfect fit!
#if DEBUG_TILEMATRIX
            BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "bestDivisor found perfect size isPerfectWidth=%s isPerfectHeight=%s",
                                   isPerfectWidth ? "true" : "false",
                                   isPerfectHeight ? "true" : "false");
#endif
            break;
        }

        // Store basis of comparison.
        if (!bestDivisor.first || !bestDivisor.second) {
            bestDivisor = divisor;
            continue;
        }

        // If the current best divisor agrees with the preferred tile matrix direction,
        // then continue if the current candidate does not.
        if (divisorIsPreferredDirection(bestDivisor, direction) && !divisorIsPreferredDirection(divisor, direction))
            continue;

        // Compare ratios.
        float diff1 = fabs((static_cast<float>(divisor.first) / static_cast<float>(divisor.second)) - ratio);
        float diff2 = fabs((static_cast<float>(bestDivisor.first) / static_cast<float>(bestDivisor.second)) - ratio);
        if (diff1 < diff2)
            bestDivisor = divisor;
    }

    return bestDivisor;
}

struct BackingStoreMutexLocker {
    BackingStoreMutexLocker(BackingStorePrivate* backingStorePrivate)
        : m_backingStorePrivate(backingStorePrivate)
    {
        m_backingStorePrivate->lockBackingStore();
    }

    ~BackingStoreMutexLocker()
    {
        m_backingStorePrivate->unlockBackingStore();
    }

private:
    BackingStorePrivate* m_backingStorePrivate;
};

Platform::IntRect BackingStoreGeometry::backingStoreRect() const
{
    return Platform::IntRect(backingStoreOffset(), backingStoreSize());
}

Platform::IntSize BackingStoreGeometry::backingStoreSize() const
{
    return Platform::IntSize(numberOfTilesWide() * BackingStorePrivate::tileWidth(), numberOfTilesHigh() * BackingStorePrivate::tileHeight());
}

BackingStorePrivate::BackingStorePrivate()
    : m_suspendScreenUpdates(0)
    , m_suspendBackingStoreUpdates(0)
    , m_resumeOperation(BackingStore::None)
    , m_suspendRenderJobs(false)
    , m_suspendRegularRenderJobs(false)
    , m_isScrollingOrZooming(false)
    , m_webPage(0)
    , m_client(0)
    , m_renderQueue(adoptPtr(new RenderQueue(this)))
    , m_defersBlit(true)
    , m_hasBlitJobs(false)
    , m_currentWindowBackBuffer(0)
    , m_preferredTileMatrixDimension(Vertical)
#if USE(ACCELERATED_COMPOSITING)
    , m_isDirectRenderingAnimationMessageScheduled(false)
#endif
{
    m_frontState = reinterpret_cast<unsigned>(new BackingStoreGeometry);
    m_backState = reinterpret_cast<unsigned>(new BackingStoreGeometry);

    // Need a recursive mutex to achieve a global lock.
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

BackingStorePrivate::~BackingStorePrivate()
{
    BackingStoreGeometry* front = reinterpret_cast<BackingStoreGeometry*>(m_frontState);
    delete front;
    m_frontState = 0;

    BackingStoreGeometry* back = reinterpret_cast<BackingStoreGeometry*>(m_backState);
    delete back;
    m_backState = 0;

    pthread_mutex_destroy(&m_mutex);
}

void BackingStorePrivate::instrumentBeginFrame()
{
    WebCore::InspectorInstrumentation::didBeginFrame(WebPagePrivate::core(m_webPage));
}

void BackingStorePrivate::instrumentCancelFrame()
{
    WebCore::InspectorInstrumentation::didCancelFrame(WebPagePrivate::core(m_webPage));
}

bool BackingStorePrivate::shouldDirectRenderingToWindow() const
{
    // Direct rendering doesn't work with OpenGL compositing code paths due to
    // a race condition on which thread's EGL context gets to make the surface
    // current, see PR 105750.
    // As a workaround, we will be using compositor to draw the root layer.
    if (isOpenGLCompositing())
        return false;

    if (m_webPage->settings()->isDirectRenderingToWindowEnabled())
        return true;

    // If the BackingStore is inactive, see if there's a compositor to do the
    // work of rendering the root layer.
    if (!isActive())
        return !m_webPage->d->compositorDrawsRootLayer();

    const BackingStoreGeometry* currentState = frontState();
    const unsigned tilesNecessary = minimumNumberOfTilesWide() * minimumNumberOfTilesHigh();
    const unsigned tilesAvailable = currentState->numberOfTilesWide() * currentState->numberOfTilesHigh();
    return tilesAvailable < tilesNecessary;
}

bool BackingStorePrivate::isOpenGLCompositing() const
{
    if (Window* window = m_webPage->client()->window())
        return window->windowUsage() == Window::GLES2Usage;

    // If there's no window, OpenGL rendering is currently the only option.
    return true;
}

void BackingStorePrivate::suspendScreenAndBackingStoreUpdates()
{
    if (m_suspendScreenUpdates) {
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelInfo,
            "Screen and backingstore already suspended, increasing suspend counter.");
    }

    ++m_suspendBackingStoreUpdates;

    // Make sure the user interface thread gets the message before we proceed
    // because blitContents can be called from this thread and it must honor
    // this flag.
    ++m_suspendScreenUpdates;

    BlackBerry::Platform::userInterfaceThreadMessageClient()->syncToCurrentMessage();

#if USE(ACCELERATED_COMPOSITING)
    m_webPage->d->resetCompositingSurface();
#endif
}

void BackingStorePrivate::resumeScreenAndBackingStoreUpdates(BackingStore::ResumeUpdateOperation op)
{
    ASSERT(m_suspendScreenUpdates);
    ASSERT(m_suspendBackingStoreUpdates);

    // Both variables are similar except for the timing of setting them.
    ASSERT(m_suspendScreenUpdates == m_suspendBackingStoreUpdates);

    if (!m_suspendScreenUpdates || !m_suspendBackingStoreUpdates) {
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelCritical,
            "Call mismatch: Screen and backingstore haven't been suspended, therefore won't resume!");
        return;
    }

    // Out of all nested resume calls, resume with the maximum-impact operation.
    if (op == BackingStore::RenderAndBlit
        || (m_resumeOperation == BackingStore::None && op == BackingStore::Blit))
        m_resumeOperation = op;

    if (m_suspendScreenUpdates >= 2 && m_suspendBackingStoreUpdates >= 2) { // we're still suspended
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelInfo,
            "Screen and backingstore still suspended, decreasing suspend counter.");
        --m_suspendBackingStoreUpdates;
        --m_suspendScreenUpdates;
        return;
    }

    --m_suspendBackingStoreUpdates;

    op = m_resumeOperation;
    m_resumeOperation = BackingStore::None;

#if USE(ACCELERATED_COMPOSITING)
    if (op != BackingStore::None) {
        if (isOpenGLCompositing() && !isActive()) {
            m_webPage->d->setCompositorDrawsRootLayer(true);
            m_webPage->d->setNeedsOneShotDrawingSynchronization();
            --m_suspendScreenUpdates;
            BlackBerry::Platform::userInterfaceThreadMessageClient()->syncToCurrentMessage();
            return;
        }

        m_webPage->d->setNeedsOneShotDrawingSynchronization();
    }
#endif

    // For the direct rendering case, there is no such operation as blit,
    // we have to render to get anything to the screen.
    if (shouldDirectRenderingToWindow() && op == BackingStore::Blit)
        op = BackingStore::RenderAndBlit;

    // Do some rendering if necessary.
    if (op == BackingStore::RenderAndBlit)
        renderVisibleContents();

    // Make sure the user interface thread gets the message before we proceed
    // because blitContents can be called from the user interface thread and
    // it must honor this flag.
    --m_suspendScreenUpdates;
    BlackBerry::Platform::userInterfaceThreadMessageClient()->syncToCurrentMessage();

    if (op == BackingStore::None)
        return;
#if USE(ACCELERATED_COMPOSITING)
    // This will also blit since we set the OSDS flag above.
    m_webPage->d->commitRootLayerIfNeeded();
#else
    // Do some blitting if necessary.
    if ((op == BackingStore::Blit || op == BackingStore::RenderAndBlit) && !shouldDirectRenderingToWindow())
        blitVisibleContents();
#endif
}

void BackingStorePrivate::repaint(const Platform::IntRect& windowRect,
                                  bool contentChanged, bool immediate)
{
    if (m_suspendBackingStoreUpdates)
        return;

     // If immediate is true, then we're being asked to perform synchronously.
     // NOTE: WebCore::ScrollView will call this method with immediate:true and contentChanged:false.
     // This is a special case introduced specifically for the Apple's windows port and can be safely ignored I believe.
     // Now this method will be called from WebPagePrivate::repaint().

    if (contentChanged && !windowRect.isEmpty()) {
        // This windowRect is in untransformed coordinates relative to the viewport, but
        // it needs to be transformed coordinates relative to the transformed contents.
        Platform::IntRect rect = m_webPage->d->mapToTransformed(m_client->mapFromViewportToContents(windowRect));
        rect.inflate(1 /*dx*/, 1 /*dy*/); // Account for anti-aliasing of previous rendering runs.

        // FIXME: This should not explicitely depend on WebCore::.
        WebCore::IntRect tmpRect = rect;
        m_client->clipToTransformedContentsRect(tmpRect);

        rect = tmpRect;
        if (rect.isEmpty())
            return;

#if DEBUG_WEBCORE_REQUESTS
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
                                  "BackingStorePrivate::repaint rect=%d,%d %dx%d contentChanged=%s immediate=%s",
                                  rect.x(), rect.y(), rect.width(), rect.height(),
                                  (contentChanged ? "true" : "false"),
                                  (immediate ? "true" : "false"));
#endif

        if (immediate) {
            if (render(rect)) {
                if (!shouldDirectRenderingToWindow() && !m_webPage->d->commitRootLayerIfNeeded())
                    blitVisibleContents();
                m_webPage->d->m_client->notifyContentRendered(rect);
            }
        } else
            m_renderQueue->addToQueue(RenderQueue::RegularRender, rect);
    }
}

void BackingStorePrivate::slowScroll(const Platform::IntSize& delta, const Platform::IntRect& windowRect, bool immediate)
{
#if DEBUG_BACKINGSTORE
    // Start the time measurement...
    double time = WTF::currentTime();
#endif

    scrollingStartedHelper(delta);

    // This windowRect is in untransformed coordinates relative to the viewport, but
    // it needs to be transformed coordinates relative to the transformed contents.
    Platform::IntRect rect = m_webPage->d->mapToTransformed(m_client->mapFromViewportToContents(windowRect));

    if (immediate) {
        if (render(rect) && !isSuspended() && !shouldDirectRenderingToWindow() && !m_webPage->d->commitRootLayerIfNeeded())
            blitVisibleContents();
    } else {
        m_renderQueue->addToQueue(RenderQueue::VisibleScroll, rect);
        // We only blit here if the client did not generate the scroll as the client
        // now supports blitting asynchronously during scroll operations.
        if (!m_client->isClientGeneratedScroll() && !shouldDirectRenderingToWindow())
            blitVisibleContents();
    }

#if DEBUG_BACKINGSTORE
    // Stop the time measurement.
    double elapsed = WTF::currentTime() - time;
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::slowScroll elapsed=%f", elapsed);
#endif
}

void BackingStorePrivate::scroll(const Platform::IntSize& delta,
                                 const Platform::IntRect& scrollViewRect,
                                 const Platform::IntRect& clipRect)
{
    // If we are direct rendering then we are forced to go down the slow path
    // to scrolling.
    if (shouldDirectRenderingToWindow()) {
        Platform::IntRect viewportRect(Platform::IntPoint(0, 0), m_webPage->d->transformedViewportSize());
        slowScroll(delta, m_webPage->d->mapFromTransformed(viewportRect), true /*immediate*/);
        return;
    }

#if DEBUG_BACKINGSTORE
    // Start the time measurement...
    double time = WTF::currentTime();
#endif

    scrollingStartedHelper(delta);

    // We only blit here if the client did not generate the scroll as the client
    // now supports blitting asynchronously during scroll operations.
    if (!m_client->isClientGeneratedScroll())
        blitVisibleContents();

#if DEBUG_BACKINGSTORE
    // Stop the time measurement.
    double elapsed = WTF::currentTime() - time;
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::scroll dx=%d, dy=%d elapsed=%f", delta.width(), delta.height(), elapsed);
#endif
}

void BackingStorePrivate::scrollingStartedHelper(const Platform::IntSize& delta)
{
    // Notify the render queue so that it can shuffle accordingly.
    m_renderQueue->updateSortDirection(delta.width(), delta.height());
    m_renderQueue->visibleContentChanged(visibleContentsRect());

    // Scroll the actual backingstore.
    scrollBackingStore(delta.width(), delta.height());

    // Add any newly visible tiles that have not been previously rendered to the queue
    // and check if the tile was previously rendered by regular render job.
    updateTilesForScrollOrNotRenderedRegion();
}

bool BackingStorePrivate::shouldSuppressNonVisibleRegularRenderJobs() const
{
#if SUPPRESS_NON_VISIBLE_REGULAR_RENDER_JOBS
    return true;
#else
    // Always suppress when loading as this drastically decreases page loading
    // time...
    return m_client->isLoading();
#endif
}

bool BackingStorePrivate::shouldPerformRenderJobs() const
{
    return (m_webPage->isVisible() || shouldDirectRenderingToWindow()) && !m_suspendRenderJobs && !m_suspendBackingStoreUpdates && !m_renderQueue->isEmpty(!m_suspendRegularRenderJobs);
}

bool BackingStorePrivate::shouldPerformRegularRenderJobs() const
{
    return shouldPerformRenderJobs() && !m_suspendRegularRenderJobs;
}

static const BlackBerry::Platform::Message::Type RenderJobMessageType = BlackBerry::Platform::Message::generateUniqueMessageType();
class RenderJobMessage : public BlackBerry::Platform::ExecutableMessage
{
public:
    RenderJobMessage(BlackBerry::Platform::MessageDelegate* delegate)
        : BlackBerry::Platform::ExecutableMessage(delegate, BlackBerry::Platform::ExecutableMessage::UniqueCoalescing, RenderJobMessageType)
    { }
};

void BackingStorePrivate::dispatchRenderJob()
{
    BlackBerry::Platform::MessageDelegate* messageDelegate = BlackBerry::Platform::createMethodDelegate(&BackingStorePrivate::renderJob, this);
    BlackBerry::Platform::webKitThreadMessageClient()->dispatchMessage(new RenderJobMessage(messageDelegate));
}

void BackingStorePrivate::renderJob()
{
    if (!shouldPerformRenderJobs())
        return;

#if DEBUG_BACKINGSTORE
    BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::renderJob");
#endif

    m_renderQueue->render(!m_suspendRegularRenderJobs);

#if USE(ACCELERATED_COMPOSITING)
    m_webPage->d->commitRootLayerIfNeeded();
#endif

    if (shouldPerformRenderJobs())
        dispatchRenderJob();
}

Platform::IntRect BackingStorePrivate::expandedContentsRect() const
{
    return Platform::IntRect(Platform::IntPoint(0, 0), expandedContentsSize());
}

Platform::IntRect BackingStorePrivate::visibleContentsRect() const
{
    return intersection(m_client->transformedVisibleContentsRect(),
                        Platform::IntRect(Platform::IntPoint(0, 0), m_client->transformedContentsSize()));
}

Platform::IntRect BackingStorePrivate::unclippedVisibleContentsRect() const
{
    return m_client->transformedVisibleContentsRect();
}

bool BackingStorePrivate::shouldMoveLeft(const Platform::IntRect& backingStoreRect) const
{
    return canMoveX(backingStoreRect)
            && backingStoreRect.x() > visibleContentsRect().x()
            && backingStoreRect.x() > expandedContentsRect().x();
}

bool BackingStorePrivate::shouldMoveRight(const Platform::IntRect& backingStoreRect) const
{
    return canMoveX(backingStoreRect)
            && backingStoreRect.right() < visibleContentsRect().right()
            && backingStoreRect.right() < expandedContentsRect().right();
}

bool BackingStorePrivate::shouldMoveUp(const Platform::IntRect& backingStoreRect) const
{
    return canMoveY(backingStoreRect)
            && backingStoreRect.y() > visibleContentsRect().y()
            && backingStoreRect.y() > expandedContentsRect().y();
}

bool BackingStorePrivate::shouldMoveDown(const Platform::IntRect& backingStoreRect) const
{
    return canMoveY(backingStoreRect)
            && backingStoreRect.bottom() < visibleContentsRect().bottom()
            && backingStoreRect.bottom() < expandedContentsRect().bottom();
}

bool BackingStorePrivate::canMoveX(const Platform::IntRect& backingStoreRect) const
{
    return backingStoreRect.width() > visibleContentsRect().width();
}

bool BackingStorePrivate::canMoveY(const Platform::IntRect& backingStoreRect) const
{
    return backingStoreRect.height() > visibleContentsRect().height();
}

bool BackingStorePrivate::canMoveLeft(const Platform::IntRect& rect) const
{
    Platform::IntRect backingStoreRect = rect;
    Platform::IntRect visibleContentsRect = this->visibleContentsRect();
    Platform::IntRect contentsRect = this->expandedContentsRect();
    backingStoreRect.move(-tileWidth(), 0);
    return backingStoreRect.right() >= visibleContentsRect.right()
            && backingStoreRect.x() >= contentsRect.x();
}

bool BackingStorePrivate::canMoveRight(const Platform::IntRect& rect) const
{
    Platform::IntRect backingStoreRect = rect;
    Platform::IntRect visibleContentsRect = this->visibleContentsRect();
    Platform::IntRect contentsRect = this->expandedContentsRect();
    backingStoreRect.move(tileWidth(), 0);
    return backingStoreRect.x() <= visibleContentsRect.x()
            && (backingStoreRect.right() <= contentsRect.right()
            || (backingStoreRect.right() - contentsRect.right()) < tileWidth());
}

bool BackingStorePrivate::canMoveUp(const Platform::IntRect& rect) const
{
    Platform::IntRect backingStoreRect = rect;
    Platform::IntRect visibleContentsRect = this->visibleContentsRect();
    Platform::IntRect contentsRect = this->expandedContentsRect();
    backingStoreRect.move(0, -tileHeight());
    return backingStoreRect.bottom() >= visibleContentsRect.bottom()
            && backingStoreRect.y() >= contentsRect.y();
}

bool BackingStorePrivate::canMoveDown(const Platform::IntRect& rect) const
{
    Platform::IntRect backingStoreRect = rect;
    Platform::IntRect visibleContentsRect = this->visibleContentsRect();
    Platform::IntRect contentsRect = this->expandedContentsRect();
    backingStoreRect.move(0, tileHeight());
    return backingStoreRect.y() <= visibleContentsRect.y()
            && (backingStoreRect.bottom() <= contentsRect.bottom()
            || (backingStoreRect.bottom() - contentsRect.bottom()) < tileHeight());
}

Platform::IntRect BackingStorePrivate::backingStoreRectForScroll(int deltaX, int deltaY, const Platform::IntRect& rect) const
{
    // The current rect.
    Platform::IntRect backingStoreRect = rect;

    // This method uses the delta values to describe the backingstore rect
    // given the current scroll direction and the viewport position. However,
    // this method can be called with no deltas whatsoever for instance when
    // the contents size changes or the orientation changes. In this case, we
    // want to use the previous scroll direction to describe the backingstore
    // rect. This will result in less checkerboard.
    if (!deltaX && !deltaY) {
        deltaX = m_previousDelta.width();
        deltaY = m_previousDelta.height();
    }
    m_previousDelta = Platform::IntSize(deltaX, deltaY);

    // Return to origin if need be.
    if (!canMoveX(backingStoreRect) && backingStoreRect.x())
        backingStoreRect.setX(0);

    if (!canMoveY(backingStoreRect) && backingStoreRect.y())
        backingStoreRect.setY(0);

    // Move the rect left.
    while (shouldMoveLeft(backingStoreRect) || (deltaX > 0 && canMoveLeft(backingStoreRect)))
        backingStoreRect.move(-tileWidth(), 0);

    // Move the rect right.
    while (shouldMoveRight(backingStoreRect) || (deltaX < 0 && canMoveRight(backingStoreRect)))
        backingStoreRect.move(tileWidth(), 0);

    // Move the rect up.
    while (shouldMoveUp(backingStoreRect) || (deltaY > 0 && canMoveUp(backingStoreRect)))
        backingStoreRect.move(0, -tileHeight());

    // Move the rect down.
    while (shouldMoveDown(backingStoreRect) || (deltaY < 0 && canMoveDown(backingStoreRect)))
        backingStoreRect.move(0, tileHeight());

    return backingStoreRect;
}

void BackingStorePrivate::setBackingStoreRect(const Platform::IntRect& backingStoreRect)
{
    if (!m_webPage->isVisible())
        return;

    if (!isActive()) {
        m_webPage->d->setShouldResetTilesWhenShown(true);
        return;
    }

    Platform::IntRect currentBackingStoreRect = frontState()->backingStoreRect();

    if (backingStoreRect == currentBackingStoreRect)
        return;

#if DEBUG_TILEMATRIX
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::setBackingStoreRect changed from (%d,%d %dx%d) to (%d,%d %dx%d)",
                           currentBackingStoreRect.x(),
                           currentBackingStoreRect.y(),
                           currentBackingStoreRect.width(),
                           currentBackingStoreRect.height(),
                           backingStoreRect.x(),
                           backingStoreRect.y(),
                           backingStoreRect.width(),
                           backingStoreRect.height());
#endif

    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();

    TileIndexList indexesToFill = indexesForBackingStoreRect(backingStoreRect);

    ASSERT(static_cast<int>(indexesToFill.size()) == currentMap.size());

    TileMap newTileMap;
    TileMap leftOverTiles;

    // Iterate through our current tile map and add tiles that are rendered with
    // our new backing store rect.
    TileMap::const_iterator tileMapEnd = currentMap.end();
    for (TileMap::const_iterator it = currentMap.begin(); it != tileMapEnd; ++it) {
        TileIndex oldIndex = it->first;
        BackingStoreTile* tile = it->second;

        // Reset the old index.
        resetTile(oldIndex, tile, false /*resetBackground*/);

        // Origin of last committed render for tile in transformed content coordinates.
        Platform::IntPoint origin = originOfLastRenderForTile(oldIndex, tile, currentBackingStoreRect);

        // If the new backing store rect contains this origin, then insert the tile there
        // and mark it as no longer shifted. Note: Platform::IntRect::contains checks for a 1x1 rect
        // below and to the right of the origin so it is correct usage here.
        if (backingStoreRect.contains(origin)) {
            TileIndex newIndex = indexOfTile(origin, backingStoreRect);
            Platform::IntRect rect(origin, tileSize());
            if (m_renderQueue->regularRenderJobsPreviouslyAttemptedButNotRendered(rect)) {
                // If the render queue previously tried to render this tile, but the
                // backingstore wasn't in the correct place or the tile wasn't visible
                // at the time then we can't simply restore the tile since the content
                // is now invalid as far as WebKit is concerned. Instead, we clear
                // the tile here of the region and then put the tile in the render
                // queue again.

                // Intersect the tile with the not rendered region to get the areas
                // of the tile that we need to clear.
                Platform::IntRectRegion tileNotRenderedRegion = Platform::IntRectRegion::intersectRegions(m_renderQueue->regularRenderJobsNotRenderedRegion(), rect);
                clearAndUpdateTileOfNotRenderedRegion(newIndex, tile, tileNotRenderedRegion, backingStoreRect);
#if DEBUG_BACKINGSTORE
                Platform::IntRect extents = tileNotRenderedRegion.extents();
                BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::setBackingStoreRect did clear tile %d,%d %dx%d",
                                       extents.x(), extents.y(), extents.width(), extents.height());
#endif
            } else {
                // Mark as needing update.
                if (!tile->frontBuffer()->isRendered()
                    && !isCurrentVisibleJob(newIndex, tile, backingStoreRect))
                    updateTile(origin, false /*immediate*/);
            }

            // Do some bookkeeping with shifting tiles...
            tile->clearShift();
            tile->setCommitted(true);

            size_t i = indexesToFill.find(newIndex);
            ASSERT(i != WTF::notFound);
            indexesToFill.remove(i);
            newTileMap.add(newIndex, tile);
        } else {
            // Store this tile and index so we can add it to the remaining left over spots...
            leftOverTiles.add(oldIndex, tile);
        }
    }

    ASSERT(static_cast<int>(indexesToFill.size()) == leftOverTiles.size());
    size_t i = 0;
    TileMap::const_iterator leftOverEnd = leftOverTiles.end();
    for (TileMap::const_iterator it = leftOverTiles.begin(); it != leftOverEnd; ++it) {
        TileIndex oldIndex = it->first;
        BackingStoreTile* tile = it->second;
        if (i >= indexesToFill.size()) {
            ASSERT_NOT_REACHED();
            break;
        }

        TileIndex newIndex = indexesToFill.at(i);

        // Origin of last committed render for tile in transformed content coordinates.
        Platform::IntPoint originOfOld = originOfLastRenderForTile(oldIndex, tile, currentBackingStoreRect);
        // Origin of the new index for the new backing store rect.
        Platform::IntPoint originOfNew = originOfTile(newIndex, backingStoreRect);

        // Mark as needing update.
        updateTile(originOfNew, false /*immediate*/);

        tile->clearShift();
        tile->setCommitted(false);
        tile->setHorizontalShift((originOfOld.x() - originOfNew.x()) / tileWidth());
        tile->setVerticalShift((originOfOld.y() - originOfNew.y()) / tileHeight());

        newTileMap.add(newIndex, tile);

        ++i;
    }

    // Checks to make sure we haven't lost any tiles.
    ASSERT(currentMap.size() == newTileMap.size());

    backState()->setNumberOfTilesWide(backingStoreRect.width() / tileWidth());
    backState()->setNumberOfTilesHigh(backingStoreRect.height() / tileHeight());
    backState()->setBackingStoreOffset(backingStoreRect.location());
    backState()->setTileMap(newTileMap);

    swapState();
}

BackingStorePrivate::TileIndexList BackingStorePrivate::indexesForBackingStoreRect(const Platform::IntRect& backingStoreRect) const
{
    TileIndexList indexes;
    int numberOfTilesWide = backingStoreRect.width() / tileWidth();
    int numberOfTilesHigh = backingStoreRect.height() / tileHeight();
    for (int y = 0; y < numberOfTilesHigh; ++y) {
        for (int x = 0; x < numberOfTilesWide; ++x) {
            TileIndex index(x, y);
            indexes.append(index);
        }
    }
    return indexes;
}

Platform::IntPoint BackingStorePrivate::originOfLastRenderForTile(const TileIndex& index,
                                                                 BackingStoreTile* tile,
                                                                 const Platform::IntRect& backingStoreRect) const
{
    return originOfTile(indexOfLastRenderForTile(index, tile), backingStoreRect);
}

TileIndex BackingStorePrivate::indexOfLastRenderForTile(const TileIndex& index, BackingStoreTile* tile) const
{
    return TileIndex(index.i() + tile->horizontalShift(), index.j() + tile->verticalShift());
}

TileIndex BackingStorePrivate::indexOfTile(const Platform::IntPoint& origin,
                                           const Platform::IntRect& backingStoreRect) const
{
    int offsetX = origin.x() - backingStoreRect.x();
    int offsetY = origin.y() - backingStoreRect.y();
    if (offsetX)
        offsetX = offsetX / tileWidth();
    if (offsetY)
        offsetY = offsetY / tileHeight();
    return TileIndex(offsetX, offsetY);
}

void BackingStorePrivate::clearAndUpdateTileOfNotRenderedRegion(const TileIndex& index, BackingStoreTile* tile,
                                                                const Platform::IntRectRegion& tileNotRenderedRegion,
                                                                const Platform::IntRect& backingStoreRect,
                                                                bool update)
{
    if (tileNotRenderedRegion.isEmpty())
        return;

    // Intersect the tile with the not rendered region to get the areas
    // of the tile that we need to clear.
    IntRectList tileNotRenderedRegionRects = tileNotRenderedRegion.rects();
    for (size_t i = 0; i < tileNotRenderedRegionRects.size(); ++i) {
        Platform::IntRect tileNotRenderedRegionRect = tileNotRenderedRegionRects.at(i);
        // Clear the render queue of this rect.
        m_renderQueue->clear(tileNotRenderedRegionRect, true /*clearRegularRenderJobs*/);

        if (update) {
            // Add it again as a regular render job.
            m_renderQueue->addToQueue(RenderQueue::RegularRender, tileNotRenderedRegionRect);
        }
    }

    // Find the origin of this tile.
    Platform::IntPoint origin = originOfTile(index, backingStoreRect);

    // Map to tile coordinates.
    Platform::IntRectRegion translatedRegion(tileNotRenderedRegion);
    translatedRegion.move(-origin.x(), -origin.y());

    // If the region in question is already marked as not rendered, return early
    if (Platform::IntRectRegion::intersectRegions(tile->frontBuffer()->renderedRegion(), translatedRegion).isEmpty())
        return;

    // Clear the tile of this region. The back buffer region is invalid anyway, but the front
    // buffer must not be manipulated without synchronization with the compositing thread, or
    // we have a race.
    // Instead of using the customary sequence of copy-back, modify and swap, we send a synchronous
    // message to the compositing thread to avoid the copy-back step and save memory bandwidth.
    // The trade-off is that the WebKit thread might wait a little longer for the compositing thread
    // than it would from a waitForCurrentMessage() call.

    ASSERT(Platform::webKitThreadMessageClient()->isCurrentThread());
    if (!Platform::webKitThreadMessageClient()->isCurrentThread())
        return;

    Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
        Platform::createMethodCallMessage(&BackingStorePrivate::clearRenderedRegion,
            this, tile, translatedRegion));
}

void BackingStorePrivate::clearRenderedRegion(BackingStoreTile* tile, const Platform::IntRectRegion& region)
{
    ASSERT(Platform::userInterfaceThreadMessageClient()->isCurrentThread());
    if (!Platform::userInterfaceThreadMessageClient()->isCurrentThread())
        return;

    tile->frontBuffer()->clearRenderedRegion(region);
}

bool BackingStorePrivate::isCurrentVisibleJob(const TileIndex& index, BackingStoreTile* tile, const Platform::IntRect& backingStoreRect) const
{
    // First check if the whole rect is in the queue.
    Platform::IntRect wholeRect = Platform::IntRect(originOfTile(index, backingStoreRect), tileSize());
    if (m_renderQueue->isCurrentVisibleScrollJob(wholeRect) || m_renderQueue->isCurrentVisibleScrollJobCompleted(wholeRect))
        return true;

    // Second check if the individual parts of the non-rendered region are in the regular queue.
    IntRectList tileNotRenderedRegionRects = tile->frontBuffer()->notRenderedRegion().rects();
    for (size_t i = 0; i < tileNotRenderedRegionRects.size(); ++i) {
        Platform::IntRect tileNotRenderedRegionRect = tileNotRenderedRegionRects.at(i);
        Platform::IntPoint origin = originOfTile(index, backingStoreRect);

        // Map to transformed contents coordinates.
        tileNotRenderedRegionRect.move(origin.x(), origin.y());

        if (!m_renderQueue->isCurrentRegularRenderJob(tileNotRenderedRegionRect))
            return false;
    }

    return true;
}

void BackingStorePrivate::scrollBackingStore(int deltaX, int deltaY)
{
    if (!m_webPage->isVisible())
        return;

    if (!isActive()) {
        m_webPage->d->setShouldResetTilesWhenShown(true);
        return;
    }

    // Calculate our new preferred matrix dimension.
    if (deltaX || deltaY)
        m_preferredTileMatrixDimension = abs(deltaX) > abs(deltaY) ? Horizontal : Vertical;

    // Calculate our preferred matrix geometry.
    Divisor divisor = bestDivisor(expandedContentsSize(),
                                  tileWidth(), tileHeight(),
                                  minimumNumberOfTilesWide(), minimumNumberOfTilesHigh(),
                                  m_preferredTileMatrixDimension);

#if DEBUG_TILEMATRIX
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::scrollBackingStore divisor %dx%d",
                           divisor.first,
                           divisor.second);
#endif

    // Initialize a rect with that new geometry.
    Platform::IntRect backingStoreRect(0, 0, divisor.first * tileWidth(), divisor.second * tileHeight());

    // Scroll that rect so that it fits our contents and viewport and scroll delta.
    backingStoreRect = backingStoreRectForScroll(deltaX, deltaY, backingStoreRect);

    ASSERT(!backingStoreRect.isEmpty());

    setBackingStoreRect(backingStoreRect);
}

bool BackingStorePrivate::renderDirectToWindow(const Platform::IntRect& rect)
{
    requestLayoutIfNeeded();

    Platform::IntRect dirtyRect = rect;
    dirtyRect.intersect(unclippedVisibleContentsRect());

    if (dirtyRect.isEmpty())
        return false;

    Platform::IntRect screenRect = m_client->mapFromTransformedContentsToTransformedViewport(dirtyRect);
    windowFrontBufferState()->clearBlittedRegion(screenRect);

    paintDefaultBackground(dirtyRect, TransformationMatrix(), true /*flush*/);

    const Platform::IntPoint origin = unclippedVisibleContentsRect().location();
    // We don't need a buffer since we're direct rendering to window.
    renderContents(0, origin, dirtyRect);
    windowBackBufferState()->addBlittedRegion(screenRect);

#if USE(ACCELERATED_COMPOSITING)
    m_isDirectRenderingAnimationMessageScheduled = false;

    if (m_webPage->d->isAcceleratedCompositingActive()) {
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
            BlackBerry::Platform::createMethodCallMessage(
                &BackingStorePrivate::drawAndBlendLayersForDirectRendering,
                this, dirtyRect));
    }
#endif

    invalidateWindow(screenRect);
    return true;
}

bool BackingStorePrivate::render(const Platform::IntRect& rect)
{
    if (!m_webPage->isVisible())
        return false;

    requestLayoutIfNeeded();

    if (shouldDirectRenderingToWindow())
        return renderDirectToWindow(rect);

    // If direct rendering is off, even though we're not active, someone else
    // has to render the root layer. There are no tiles available for us to
    // draw to.
    if (!isActive())
        return false;

    TileRectList tileRectList = mapFromTransformedContentsToTiles(rect);
    if (tileRectList.isEmpty())
        return false;

#if DEBUG_BACKINGSTORE
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
                           "BackingStorePrivate::render rect=(%d,%d %dx%d), m_suspendBackingStoreUpdates = %s",
                           rect.x(), rect.y(), rect.width(), rect.height(),
                           m_suspendBackingStoreUpdates ? "true" : "false");
#endif

    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();

    for (size_t i = 0; i < tileRectList.size(); ++i) {
        TileRect tileRect = tileRectList[i];
        TileIndex index = tileRect.first;
        Platform::IntRect dirtyTileRect = tileRect.second;
        BackingStoreTile* tile = currentMap.get(index);

        // This dirty tile rect is in tile coordinates, but it needs to be in
        // transformed contents coordinates.
        Platform::IntRect dirtyRect = mapFromTilesToTransformedContents(tileRect);

        // If the tile has been created, but this is the first time we are painting on it
        // then it hasn't been given a default background yet so that we can save time during
        // startup. That's why we are doing it here instead...
        if (!tile->backgroundPainted())
            tile->paintBackground();

        // Paint default background if contents rect is empty.
        if (!expandedContentsRect().isEmpty()) {
            // Otherwise we should clip the contents size and render the content.
            dirtyRect.intersect(expandedContentsRect());

            dirtyTileRect.intersect(tileContentsRect(index, expandedContentsRect(), currentState));

            // We probably have extra tiles since the contents size is so small.
            // Save some cycles here...
            if (dirtyRect.isEmpty())
                continue;
        }

        BlackBerry::Platform::Graphics::Buffer* nativeBuffer
            = tile->backBuffer()->nativeBuffer();

        // TODO: This code is only needed for EGLImage code path, but preferrably BackingStore
        // should not know that, and the synchronization should be in BlackBerry::Platform::Graphics
        // if possible.
        if (isOpenGLCompositing())
            SurfacePool::globalSurfacePool()->waitForBuffer(tile->backBuffer());

        // Modify the buffer only after we've waited for the buffer to become available above.

        // If we're not yet committed, then commit only after the tile has back buffer has been
        // swapped in so it has some valid content.
        // Otherwise the compositing thread could pick up the tile while its front buffer is still invalid.
        bool wasCommitted = tile->isCommitted();
        if (wasCommitted)
            copyPreviousContentsToBackSurfaceOfTile(dirtyTileRect, tile);
        else
            tile->backBuffer()->clearRenderedRegion();

        // FIXME: modify render to take a Vector<IntRect> parameter so we're not recreating
        // GraphicsContext on the stack each time.
        renderContents(nativeBuffer, originOfTile(index), dirtyRect);

        // Add the newly rendered region to the tile so it can keep track for blits.
        tile->backBuffer()->addRenderedRegion(dirtyTileRect);

        // Thanks to the copyPreviousContentsToBackSurfaceOfTile() call above, we know that
        // the rendered region of the back buffer contains the rendered region of the front buffer.
        // Assert this just to make sure.
        // For previously uncommitted tiles, the front buffer's rendered region is not relevant.
        ASSERT(!wasCommitted || tile->backBuffer()->isRendered(tile->frontBuffer()->renderedRegion()));

        // We will need a swap here because of the shared back buffer.
        tile->swapBuffers();

        if (!wasCommitted) {
            // Commit the tile only after it has valid front buffer contents. Now, the compositing thread
            // can finally start blitting this tile.
            tile->clearShift();
            tile->setCommitted(true);
        }

        // Before clearing the render region, wait for the compositing thread to stop using the
        // buffer, in order to avoid a race on its rendered region.
        BlackBerry::Platform::userInterfaceThreadMessageClient()->syncToCurrentMessage();
        tile->backBuffer()->clearRenderedRegion();
    }

    return true;
}

void BackingStorePrivate::requestLayoutIfNeeded() const
{
    m_webPage->d->requestLayoutIfNeeded();
}

bool BackingStorePrivate::renderVisibleContents()
{
    Platform::IntRect renderRect = shouldDirectRenderingToWindow() ? visibleContentsRect() : visibleTilesRect();
    if (render(renderRect)) {
        m_renderQueue->clear(renderRect, true /*clearRegularRenderJobs*/);
        return true;
    }
    return false;
}

bool BackingStorePrivate::renderBackingStore()
{
    return render(frontState()->backingStoreRect());
}

void BackingStorePrivate::blitVisibleContents(bool force)
{
    // Blitting must never happen for direct rendering case.
    ASSERT(!shouldDirectRenderingToWindow());
    if (shouldDirectRenderingToWindow()) {
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelCritical,
            "BackingStore::blitVisibleContents operation not supported in direct rendering mode");
        return;
    }

    if (m_suspendScreenUpdates) {
        // Avoid client going into busy loop while updates suspended.
        if (force)
            m_hasBlitJobs = false;
        return;
    }

    if (!BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread()) {
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchMessage(
            BlackBerry::Platform::createMethodCallMessage(
                &BackingStorePrivate::blitVisibleContents, this, force));
        return;
    }

    blitContents(m_webPage->client()->userInterfaceBlittedDestinationRect(),
                 m_webPage->client()->userInterfaceBlittedVisibleContentsRect(),
                 force);
}

void BackingStorePrivate::copyPreviousContentsToBackSurfaceOfWindow()
{
    Platform::IntRectRegion previousContentsRegion
        = Platform::IntRectRegion::subtractRegions(windowFrontBufferState()->blittedRegion(), windowBackBufferState()->blittedRegion());

    if (previousContentsRegion.isEmpty())
        return;

    if (Window* window = m_webPage->client()->window())
        window->copyFromFrontToBack(previousContentsRegion);
    windowBackBufferState()->addBlittedRegion(previousContentsRegion);
}

void BackingStorePrivate::copyPreviousContentsToBackSurfaceOfTile(const Platform::IntRect& rect,
                                                                  BackingStoreTile* tile)
{
    Platform::IntRectRegion previousContentsRegion
        = Platform::IntRectRegion::subtractRegions(tile->frontBuffer()->renderedRegion(), rect);

    IntRectList previousContentsRects = previousContentsRegion.rects();
    for (size_t i = 0; i < previousContentsRects.size(); ++i) {
        Platform::IntRect previousContentsRect = previousContentsRects.at(i);
        tile->backBuffer()->addRenderedRegion(previousContentsRect);

        BlackBerry::Platform::Graphics::blitToBuffer(
            tile->backBuffer()->nativeBuffer(), previousContentsRect,
            tile->frontBuffer()->nativeBuffer(), previousContentsRect);
    }
}

void BackingStorePrivate::paintDefaultBackground(const Platform::IntRect& contents,
                                                 const WebCore::TransformationMatrix& transformation,
                                                 bool flush)
{
    const Platform::IntRect contentsRect = Platform::IntRect(Platform::IntPoint(0, 0), m_webPage->d->transformedContentsSize());
    Platform::IntPoint origin = contents.location();
    Platform::IntRect contentsClipped = contents;

    // We have to paint the default background in the case of overzoom and
    // make sure it is invalidated.
    Platform::IntRectRegion overScrollRegion
            = Platform::IntRectRegion::subtractRegions(Platform::IntRect(contentsClipped), contentsRect);

    IntRectList overScrollRects = overScrollRegion.rects();
    for (size_t i = 0; i < overScrollRects.size(); ++i) {
        Platform::IntRect overScrollRect = overScrollRects.at(i);
        overScrollRect.move(-origin.x(), -origin.y());
        overScrollRect = transformation.mapRect(overScrollRect);

        if (!transformation.isIdentity()) {
            // Because of rounding it is possible that overScrollRect could be off-by-one larger
            // than the surface size of the window. We prevent this here, by clamping
            // it to ensure that can't happen.
            overScrollRect.intersect(Platform::IntRect(Platform::IntPoint(0, 0), surfaceSize()));
        }

        if (m_webPage->settings()->isEnableDefaultOverScrollBackground()) {
            fillWindow(BlackBerry::Platform::Graphics::DefaultBackgroundPattern,
                overScrollRect, overScrollRect.location(), 1.0 /*contentsScale*/);
        } else {
            Color color(m_webPage->settings()->overScrollColor());
            clearWindow(overScrollRect, color.red(), color.green(), color.blue(), color.alpha());
        }
    }
}

void BackingStorePrivate::blitContents(const Platform::IntRect& dstRect,
                                       const Platform::IntRect& srcRect,
                                       bool force)
{
    // Blitting must never happen for direct rendering case.
    // Use invalidateWindow() instead.
    ASSERT(!shouldDirectRenderingToWindow());
    if (shouldDirectRenderingToWindow())
        return;

    if (!m_webPage->isVisible() || m_suspendScreenUpdates) {
        // Avoid client going into busy loop while blit is impossible.
        if (force)
            m_hasBlitJobs = false;
        return;
    }

    if (!BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread()) {
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchMessage(
            BlackBerry::Platform::createMethodCallMessage(
                &BackingStorePrivate::blitContents, this, dstRect, srcRect, force));
        return;
    }

    if (m_defersBlit && !force) {
#if USE(ACCELERATED_COMPOSITING)
        // If there's a WebPageCompositorClient, let it schedule the blit.
        if (WebPageCompositorPrivate* compositor = m_webPage->d->compositor()) {
            if (WebPageCompositorClient* client = compositor->client()) {
                client->invalidate(0);
                return;
            }
        }
#endif

        m_hasBlitJobs = true;
        return;
    }

    m_hasBlitJobs = false;

    const Platform::IntRect contentsRect = Platform::IntRect(Platform::IntPoint(0, 0), m_client->transformedContentsSize());

#if DEBUG_VISUALIZE
    // Substitute a debugRect that consists of the union of the backingstore rect
    // and the ui thread viewport rect instead of the normal source rect so we
    // can visualize the entire backingstore and what it is doing when we
    // scroll and zoom!
    // FIXME: This should not explicitely depend on WebCore::.
    WebCore::IntRect debugRect = frontState()->backingStoreRect();
    debugRect.unite(m_webPage->client()->userInterfaceBlittedVisibleContentsRect());
    if (debugRect.width() < debugRect.height())
        debugRect.setWidth(ceil(double(srcRect.width()) * (double(debugRect.height()) / srcRect.height())));
    if (debugRect.height() < debugRect.width())
        debugRect.setHeight(ceil(double(srcRect.height()) * (double(debugRect.width()) / srcRect.width())));
    Platform::IntRect contents = debugRect;
#else
    Platform::IntRect contents = srcRect;
#endif

    // FIXME: This should not explicitely depend on WebCore::.
    TransformationMatrix transformation;
    if (!contents.isEmpty())
        transformation = TransformationMatrix::rectToRect(FloatRect(FloatPoint(0.0, 0.0), WebCore::IntSize(contents.size())), WebCore::IntRect(dstRect));

#if DEBUG_BACKINGSTORE
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
                           "BackingStorePrivate::blitContents dstRect=(%d,%d %dx%d) srcRect=(%d,%d %dx%d)",
                           dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height(),
                           srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height());
#endif

    Platform::IntPoint origin = contents.location();
    Platform::IntRect contentsClipped = contents;

    Vector<TileBuffer*> blittedTiles;

    if (isActive() && !m_webPage->d->compositorDrawsRootLayer()) {
        paintDefaultBackground(contents, transformation, false /*flush*/);

        BackingStoreGeometry* currentState = frontState();
        TileMap currentMap = currentState->tileMap();

#if DEBUG_CHECKERBOARD
        bool blitCheckered = false;
#endif

        // Don't clip to contents if it is empty so we can still paint default background.
        if (!contentsRect.isEmpty()) {
            contentsClipped.intersect(contentsRect);
            if (contentsClipped.isEmpty()) {
                invalidateWindow(dstRect);
                return;
            }

            Platform::IntRectRegion contentsRegion = contentsClipped;
            Platform::IntRectRegion backingStoreRegion = currentState->backingStoreRect();
            Platform::IntRectRegion checkeredRegion
                = Platform::IntRectRegion::subtractRegions(contentsRegion, backingStoreRegion);

            // Blit checkered to those parts that are not covered by the backingStoreRect.
            IntRectList checkeredRects = checkeredRegion.rects();
            for (size_t i = 0; i < checkeredRects.size(); ++i) {
                Platform::IntRect clippedDstRect = transformation.mapRect(Platform::IntRect(
                    Platform::IntPoint(checkeredRects.at(i).x() - origin.x(), checkeredRects.at(i).y() - origin.y()),
                                       checkeredRects.at(i).size()));
                // To eliminate 1 pixel inflation due to transformation rounding.
                clippedDstRect.intersect(dstRect);
#if DEBUG_CHECKERBOARD
                blitCheckered = true;
#endif

                fillWindow(BlackBerry::Platform::Graphics::CheckerboardPattern,
                    clippedDstRect, checkeredRects.at(i).location(), transformation.a());
            }
        }

        // Get the list of tile rects that makeup the content.
        TileRectList tileRectList = mapFromTransformedContentsToTiles(contentsClipped, currentState);
        for (size_t i = 0; i < tileRectList.size(); ++i) {
            TileRect tileRect = tileRectList[i];
            TileIndex index = tileRect.first;
            Platform::IntRect dirtyTileRect = tileRect.second;
            BackingStoreTile* tile = currentMap.get(index);
            TileBuffer* tileBuffer = tile->frontBuffer();

            // This dirty rect is in tile coordinates, but it needs to be in
            // transformed contents coordinates.
            Platform::IntRect dirtyRect
                = mapFromTilesToTransformedContents(tileRect, currentState->backingStoreRect());

            // Don't clip to contents if it is empty so we can still paint default background.
            if (!contentsRect.isEmpty()) {
                // Otherwise we should clip the contents size and blit.
                dirtyRect.intersect(contentsRect);

                // We probably have extra tiles since the contents size is so small.
                // Save some cycles here...
                if (dirtyRect.isEmpty())
                    continue;
            }

            // Now, this dirty rect is in transformed coordinates relative to the
            // transformed contents, but ultimately it needs to be transformed
            // coordinates relative to the viewport.
            dirtyRect.move(-origin.x(), -origin.y());

            // Save some cycles here...
            if (dirtyRect.isEmpty() || dirtyTileRect.isEmpty())
                continue;

            TileRect wholeTileRect;
            wholeTileRect.first = index;
            wholeTileRect.second = this->tileRect();

            bool committed = tile->isCommitted();
            bool rendered = tileBuffer->isRendered(dirtyTileRect);
            bool paintCheckered = !committed || !rendered;

            if (paintCheckered) {
                Platform::IntRect dirtyRectT = transformation.mapRect(dirtyRect);

                if (!transformation.isIdentity()) {
                    // Because of rounding it is possible that dirtyRect could be off-by-one larger
                    // than the surface size of the dst buffer. We prevent this here, by clamping
                    // it to ensure that can't happen.
                    dirtyRectT.intersect(Platform::IntRect(Platform::IntPoint(0, 0), surfaceSize()));
                }
                const Platform::IntPoint contentsOrigin(dirtyRect.x() + origin.x(), dirtyRect.y() + origin.y());
#if DEBUG_CHECKERBOARD
                blitCheckered = true;
#endif
                fillWindow(BlackBerry::Platform::Graphics::CheckerboardPattern,
                    dirtyRectT, contentsOrigin, transformation.a());
            }

            // Blit the visible buffer here if we have visible zoom jobs.
            if (m_renderQueue->hasCurrentVisibleZoomJob()) {

                // Needs to be in same coordinate system as dirtyRect.
                Platform::IntRect visibleTileBufferRect = m_visibleTileBufferRect;
                visibleTileBufferRect.move(-origin.x(), -origin.y());

                // Clip to the visibleTileBufferRect.
                dirtyRect.intersect(visibleTileBufferRect);

                // Clip to the dirtyRect.
                visibleTileBufferRect.intersect(dirtyRect);

                if (!dirtyRect.isEmpty() && !visibleTileBufferRect.isEmpty()) {
                    BackingStoreTile* visibleTileBuffer
                        = SurfacePool::globalSurfacePool()->visibleTileBuffer();
                    ASSERT(visibleTileBuffer->size() == visibleContentsRect().size());

                    // The offset of the current viewport with the visble tile buffer.
                    Platform::IntPoint difference = origin - m_visibleTileBufferRect.location();
                    Platform::IntSize offset = Platform::IntSize(difference.x(), difference.y());

                    // Map to the visibleTileBuffer coordinates.
                    Platform::IntRect dirtyTileRect = visibleTileBufferRect;
                    dirtyTileRect.move(offset.width(), offset.height());

                    Platform::IntRect dirtyRectT = transformation.mapRect(dirtyRect);

                    if (!transformation.isIdentity()) {
                        // Because of rounding it is possible that dirtyRect could be off-by-one larger
                        // than the surface size of the dst buffer. We prevent this here, by clamping
                        // it to ensure that can't happen.
                        dirtyRectT.intersect(Platform::IntRect(Platform::IntPoint(0, 0), surfaceSize()));
                    }

                    blitToWindow(dirtyRectT,
                                 visibleTileBuffer->frontBuffer()->nativeBuffer(),
                                 dirtyTileRect,
                                 false /*blend*/, 255);
                }
            } else if (committed) {
                // Intersect the rendered region.
                Platform::IntRectRegion renderedRegion = tileBuffer->renderedRegion();
                IntRectList dirtyRenderedRects = renderedRegion.rects();
                for (size_t i = 0; i < dirtyRenderedRects.size(); ++i) {
                    TileRect tileRect;
                    tileRect.first = index;
                    tileRect.second = intersection(dirtyTileRect, dirtyRenderedRects.at(i));
                    if (tileRect.second.isEmpty())
                        continue;
                    // Blit the rendered parts.
                    blitTileRect(tileBuffer, tileRect, origin, transformation, currentState);
                }
                blittedTiles.append(tileBuffer);
            }
        }
    }

    // TODO: This code is only needed for EGLImage code path, but preferrably BackingStore
    // should not know that, and the synchronization should be in BlackBerry::Platform::Graphics
    // if possible.
    if (isOpenGLCompositing())
        SurfacePool::globalSurfacePool()->notifyBuffersComposited(blittedTiles);

#if USE(ACCELERATED_COMPOSITING)
    if (WebPageCompositorPrivate* compositor = m_webPage->d->compositor()) {
        WebCore::FloatRect contentsRect = m_webPage->d->mapFromTransformedFloatRect(WebCore::FloatRect(WebCore::IntRect(contents)));
        compositor->drawLayers(dstRect, contentsRect);
        if (compositor->drawsRootLayer())
            paintDefaultBackground(contents, transformation, false /*flush*/);
    }

    if (!isOpenGLCompositing())
        blendCompositingSurface(dstRect);
#endif

#if ENABLE_SCROLLBARS
    if (isScrollingOrZooming() && m_client->isMainFrame()) {
        blitHorizontalScrollbar(origin);
        blitVerticalScrollbar(origin);
    }
#endif

#if DEBUG_VISUALIZE
    // FIXME: This should not explicitely depend on WebCore::.
    BlackBerry::Platform::Graphics::Buffer* windowBuffer = buffer();
    BlackBerry::Platform::Graphics::Drawable* bufferDrawable =
        BlackBerry::Platform::Graphics::lockBufferDrawable(windowBuffer);
    PlatformGraphicsContext* bufferPlatformGraphicsContext =
        SurfacePool::globalSurfacePool()->createPlatformGraphicsContext(bufferDrawable);
    GraphicsContext graphicsContext(bufferPlatformGraphicsContext);
    FloatRect wkViewport = FloatRect(visibleContentsRect());
    FloatRect uiViewport = FloatRect(m_webPage->client()->userInterfaceBlittedVisibleContentsRect());
    wkViewport.move(-contents.x(), -contents.y());
    uiViewport.move(-contents.x(), -contents.y());

    graphicsContext.save();

    // Draw a blue rect for the webkit thread viewport.
    graphicsContext.setStrokeColor(WebCore::Color(0, 0, 255), WebCore::ColorSpaceDeviceRGB);
    graphicsContext.strokeRect(transformation.mapRect(wkViewport), 1.0);

    // Draw a red rect for the ui thread viewport.
    graphicsContext.setStrokeColor(WebCore::Color(255, 0, 0), WebCore::ColorSpaceDeviceRGB);
    graphicsContext.strokeRect(transformation.mapRect(uiViewport), 1.0);

    graphicsContext.restore();

    delete bufferPlatformGraphicsContext;
    releaseBufferDrawable(windowBuffer);
#endif

#if DEBUG_CHECKERBOARD
    static double lastCheckeredTime = 0;

    if (blitCheckered && !lastCheckeredTime) {
        lastCheckeredTime = WTF::currentTime();
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
            "Blitting checkered pattern at %f\n", lastCheckeredTime);
    } else if (blitCheckered && lastCheckeredTime) {
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
            "Blitting checkered pattern at %f\n", WTF::currentTime());
    } else if (!blitCheckered && lastCheckeredTime) {
        double time = WTF::currentTime();
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
            "Blitting over checkered pattern at %f took %f\n", time, time - lastCheckeredTime);
        lastCheckeredTime = 0;
    }
#endif

    invalidateWindow(dstRect);
}

#if USE(ACCELERATED_COMPOSITING)
void BackingStorePrivate::compositeContents(WebCore::LayerRenderer* layerRenderer, const WebCore::TransformationMatrix& transform, const WebCore::FloatRect& contents, bool contentsOpaque)
{
    const Platform::IntRect transformedContentsRect = Platform::IntRect(Platform::IntPoint(0, 0), m_client->transformedContentsSize());
    Platform::IntRect transformedContents = enclosingIntRect(m_webPage->d->m_transformationMatrix->mapRect(contents));
    transformedContents.intersect(transformedContentsRect);
    if (transformedContents.isEmpty())
        return;

    if (!isActive())
        return;

    if (m_webPage->d->compositorDrawsRootLayer())
        return;

    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();
    Vector<TileBuffer*> compositedTiles;

    Platform::IntRectRegion transformedContentsRegion = transformedContents;
    Platform::IntRectRegion backingStoreRegion = currentState->backingStoreRect();
    Platform::IntRectRegion checkeredRegion
        = Platform::IntRectRegion::subtractRegions(transformedContentsRegion, backingStoreRegion);

    // Blit checkered to those parts that are not covered by the backingStoreRect.
    IntRectList checkeredRects = checkeredRegion.rects();
    for (size_t i = 0; i < checkeredRects.size(); ++i)
        layerRenderer->drawCheckerboardPattern(transform, m_webPage->d->mapFromTransformedFloatRect(WebCore::IntRect(checkeredRects.at(i))));

    // Get the list of tile rects that makeup the content.
    TileRectList tileRectList = mapFromTransformedContentsToTiles(transformedContents, currentState);
    for (size_t i = 0; i < tileRectList.size(); ++i) {
        TileRect tileRect = tileRectList[i];
        TileIndex index = tileRect.first;
        Platform::IntRect dirtyTileRect = tileRect.second;
        BackingStoreTile* tile = currentMap.get(index);
        TileBuffer* tileBuffer = tile->frontBuffer();

        // This dirty rect is in tile coordinates, but it needs to be in
        // transformed contents coordinates.
        Platform::IntRect dirtyRect = mapFromTilesToTransformedContents(tileRect, currentState->backingStoreRect());

        if (!dirtyRect.intersects(transformedContents))
            continue;

        TileRect wholeTileRect;
        wholeTileRect.first = index;
        wholeTileRect.second = this->tileRect();

        Platform::IntRect wholeRect = mapFromTilesToTransformedContents(wholeTileRect, currentState->backingStoreRect());

        bool committed = tile->isCommitted();

        if (!committed)
            layerRenderer->drawCheckerboardPattern(transform, m_webPage->d->mapFromTransformedFloatRect(Platform::FloatRect(dirtyRect)));
        else {
            layerRenderer->compositeBuffer(transform, m_webPage->d->mapFromTransformedFloatRect(Platform::FloatRect(wholeRect)), tileBuffer->nativeBuffer(), contentsOpaque, 1.0f);
            compositedTiles.append(tileBuffer);
            // Intersect the rendered region.
            Platform::IntRectRegion notRenderedRegion = Platform::IntRectRegion::subtractRegions(dirtyTileRect, tileBuffer->renderedRegion());
            IntRectList notRenderedRects = notRenderedRegion.rects();
            for (size_t i = 0; i < notRenderedRects.size(); ++i)
                layerRenderer->drawCheckerboardPattern(transform, m_webPage->d->mapFromTransformedFloatRect(Platform::FloatRect(notRenderedRects.at(i))));
        }
    }

    SurfacePool::globalSurfacePool()->notifyBuffersComposited(compositedTiles);
}
#endif

Platform::IntRect BackingStorePrivate::blitTileRect(TileBuffer* tileBuffer,
                                                   const TileRect& tileRect,
                                                   const Platform::IntPoint& origin,
                                                   const WebCore::TransformationMatrix& matrix,
                                                   BackingStoreGeometry* state)
{
    if (!m_webPage->isVisible() || !isActive())
        return Platform::IntRect();

    Platform::IntRect dirtyTileRect = tileRect.second;

    // This dirty rect is in tile coordinates, but it needs to be in
    // transformed contents coordinates.
    Platform::IntRect dirtyRect = mapFromTilesToTransformedContents(tileRect, state->backingStoreRect());

    // Now, this dirty rect is in transformed coordinates relative to the
    // transformed contents, but ultimately it needs to be transformed
    // coordinates relative to the viewport.
    dirtyRect.move(-origin.x(), -origin.y());
    dirtyRect = matrix.mapRect(dirtyRect);

    if (!matrix.isIdentity()) {
        // Because of rounding it is possible that dirtyRect could be off-by-one larger
        // than the surface size of the dst buffer. We prevent this here, by clamping
        // it to ensure that can't happen.
        dirtyRect.intersect(Platform::IntRect(Platform::IntPoint(0, 0), surfaceSize()));
    }

    ASSERT(!dirtyRect.isEmpty());
    ASSERT(!dirtyTileRect.isEmpty());
    if (dirtyRect.isEmpty() || dirtyTileRect.isEmpty())
        return Platform::IntRect();

    blitToWindow(dirtyRect, tileBuffer->nativeBuffer(), dirtyTileRect,
                 false /*blend*/, 255);
    return dirtyRect;
}

#if USE(ACCELERATED_COMPOSITING)
void BackingStorePrivate::blendCompositingSurface(const Platform::IntRect& dstRect)
{
    if (!BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread()) {
        typedef void (BlackBerry::WebKit::BackingStorePrivate::*FunctionType)(const Platform::IntRect&);
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchMessage(
            BlackBerry::Platform::createMethodCallMessage<FunctionType, BackingStorePrivate, Platform::IntRect>(
                &BackingStorePrivate::blendCompositingSurface, this, dstRect));
        return;
    }

    BackingStoreCompositingSurface* compositingSurface =
        SurfacePool::globalSurfacePool()->compositingSurface();

    if (!compositingSurface || !m_webPage->isVisible())
        return;

    WebCore::LayerRenderingResults lastCompositingResults = m_webPage->d->lastCompositingResults();
    for (size_t i = 0; i < lastCompositingResults.holePunchRectSize(); i++) {
        Platform::IntRect holePunchRect = lastCompositingResults.holePunchRect(i);

        holePunchRect.intersect(dstRect);
        holePunchRect.intersect(Platform::IntRect(
            Platform::IntPoint(0, 0), surfaceSize()));

        if (!holePunchRect.isEmpty())
            clearWindow(holePunchRect, 0, 0, 0, 0);
    }

    CompositingSurfaceBuffer* frontBuffer = compositingSurface->frontBuffer();

    IntRectList rects = lastCompositingResults.dirtyRegion.rects();
    for (size_t i = 0; i < rects.size(); ++i) {
        rects[i].intersect(dstRect);
#if DEBUG_COMPOSITING_DIRTY_REGION
        clearBuffer(buffer(), rects[i], 255, 0, 0, 128);
#endif
        blitToWindow(rects[i], frontBuffer->nativeBuffer(), rects[i], true /*blend*/, 255);
    }
}

void BackingStorePrivate::clearCompositingSurface()
{
    BackingStoreCompositingSurface* compositingSurface =
        SurfacePool::globalSurfacePool()->compositingSurface();

    if (!compositingSurface)
        return;

    CompositingSurfaceBuffer* frontBuffer = compositingSurface->frontBuffer();
    BlackBerry::Platform::Graphics::clearBuffer(frontBuffer->nativeBuffer(), Platform::IntRect(Platform::IntPoint(), frontBuffer->surfaceSize()), 0, 0, 0, 0);
}
#endif // USE(ACCELERATED_COMPOSITING)

void BackingStorePrivate::blitHorizontalScrollbar(const Platform::IntPoint& scrollPosition)
{
    if (!m_webPage->isVisible())
        return;

    m_webPage->client()->drawHorizontalScrollbar();
}

void BackingStorePrivate::blitVerticalScrollbar(const Platform::IntPoint& scrollPosition)
{
    if (!m_webPage->isVisible())
        return;

    m_webPage->client()->drawVerticalScrollbar();
}

bool BackingStorePrivate::isTileVisible(const TileIndex& index) const
{
    TileRect tileRect;
    tileRect.first = index;
    tileRect.second = this->tileRect();
    return mapFromTilesToTransformedContents(tileRect).intersects(visibleContentsRect());
}

bool BackingStorePrivate::isTileVisible(const Platform::IntPoint& origin) const
{
    return Platform::IntRect(origin, tileSize()).intersects(visibleContentsRect());
}

Platform::IntRect BackingStorePrivate::visibleTilesRect() const
{
    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();

    Platform::IntRect rect;
    TileMap::const_iterator end = currentMap.end();
    for (TileMap::const_iterator it = currentMap.begin(); it != end; ++it) {
        TileRect tileRect;
        tileRect.first = it->first;
        tileRect.second = this->tileRect();
        Platform::IntRect tile = mapFromTilesToTransformedContents(tileRect);
        if (tile.intersects(visibleContentsRect()))
            rect = Platform::unionOfRects(rect, tile);
    }
    return rect;
}

Platform::IntRect BackingStorePrivate::tileVisibleContentsRect(const TileIndex& index) const
{
    if (!isTileVisible(index))
        return Platform::IntRect();

    return tileContentsRect(index, visibleContentsRect());
}

Platform::IntRect BackingStorePrivate::tileUnclippedVisibleContentsRect(const TileIndex& index) const
{
    if (!isTileVisible(index))
        return Platform::IntRect();

    return tileContentsRect(index, unclippedVisibleContentsRect());
}

Platform::IntRect BackingStorePrivate::tileContentsRect(const TileIndex& index,
                                                       const Platform::IntRect& contents) const
{
    return tileContentsRect(index, contents, frontState());
}

Platform::IntRect BackingStorePrivate::tileContentsRect(const TileIndex& index,
                                                       const Platform::IntRect& contents,
                                                       BackingStoreGeometry* state) const
{
    TileRectList tileRectList = mapFromTransformedContentsToTiles(contents, state);
    for (size_t i = 0; i < tileRectList.size(); ++i) {
        TileRect tileRect = tileRectList[i];
        if (index == tileRect.first)
            return tileRect.second;
    }
    return Platform::IntRect();
}

void BackingStorePrivate::resetRenderQueue()
{
    m_renderQueue->reset();
}

void BackingStorePrivate::clearVisibleZoom()
{
    m_renderQueue->clearVisibleZoom();
}

void BackingStorePrivate::resetTiles(bool resetBackground)
{
    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();

    TileMap::const_iterator end = currentMap.end();
    for (TileMap::const_iterator it = currentMap.begin(); it != end; ++it)
        resetTile(it->first, it->second, resetBackground);
}

void BackingStorePrivate::updateTiles(bool updateVisible, bool immediate)
{
    if (!isActive())
        return;

    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();

    TileMap::const_iterator end = currentMap.end();
    for (TileMap::const_iterator it = currentMap.begin(); it != end; ++it) {
        bool isVisible = isTileVisible(it->first);
        if (!updateVisible && isVisible)
            continue;
        updateTile(it->first, immediate);
    }
}

void BackingStorePrivate::updateTilesForScrollOrNotRenderedRegion(bool checkLoading)
{
    // This method looks at all the tiles and if they are visible, but not completely
    // rendered or we are loading, then it updates them. For all tiles, visible and
    // non-visible, if a previous attempt was made to render them during a regular
    // render job, but they were not visible at the time, then update them and if
    // they are currently visible, reset them.

    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();
    Platform::IntRect backingStoreRect = currentState->backingStoreRect();

    bool isLoading = m_client->loadState() == WebPagePrivate::Committed;
    bool forceVisible = checkLoading && isLoading;

    TileMap::const_iterator end = currentMap.end();
    for (TileMap::const_iterator it = currentMap.begin(); it != end; ++it) {
        TileIndex index = it->first;
        BackingStoreTile* tile = it->second;
        bool isVisible = isTileVisible(index);
        // The rect in transformed contents coordinates.
        Platform::IntRect rect(originOfTile(index), tileSize());
        if (tile->isCommitted()
            && m_renderQueue->regularRenderJobsPreviouslyAttemptedButNotRendered(rect)) {
            // If the render queue previously tried to render this tile, but the
            // tile wasn't visible at the time we can't simply restore the tile
            // since the content is now invalid as far as WebKit is concerned.
            // Instead, we clear that part of the tile if it is visible and then
            // put the tile in the render queue again.
            if (isVisible) {
                // Intersect the tile with the not rendered region to get the areas
                // of the tile that we need to clear.
                Platform::IntRectRegion tileNotRenderedRegion
                    = Platform::IntRectRegion::intersectRegions(
                        m_renderQueue->regularRenderJobsNotRenderedRegion(),
                        rect);
                clearAndUpdateTileOfNotRenderedRegion(index,
                                                      tile,
                                                      tileNotRenderedRegion,
                                                      backingStoreRect,
                                                      false /*update*/);
#if DEBUG_BACKINGSTORE
                Platform::IntRect extents = tileNotRenderedRegion.extents();
                BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
                    "BackingStorePrivate::updateTilesForScroll did clear tile %d,%d %dx%d",
                    extents.x(), extents.y(), extents.width(), extents.height());
#endif
            }
            updateTile(index, false /*immediate*/);
        } else if (isVisible
            && (forceVisible || !tile->frontBuffer()->isRendered(tileVisibleContentsRect(index)))
            && !isCurrentVisibleJob(index, tile, backingStoreRect))
            updateTile(index, false /*immediate*/);
    }
}

void BackingStorePrivate::resetTile(const TileIndex& index, BackingStoreTile* tile, bool resetBackground)
{
    if (!m_webPage->isVisible())
        return;

    if (!isActive()) {
        m_webPage->d->setShouldResetTilesWhenShown(true);
        return;
    }

    TileRect tileRect;
    tileRect.first = index;
    tileRect.second = this->tileRect();
    // Only clear regular render jobs if we're clearing the background too.
    m_renderQueue->clear(mapFromTilesToTransformedContents(tileRect), resetBackground /*clearRegularRenderJobs*/);
    if (resetBackground)
        tile->reset();
}

void BackingStorePrivate::updateTile(const TileIndex& index, bool immediate)
{
    if (!isActive())
        return;

    TileRect tileRect;
    tileRect.first = index;
    tileRect.second = this->tileRect();
    Platform::IntRect updateRect = mapFromTilesToTransformedContents(tileRect);
    RenderQueue::JobType jobType = isTileVisible(index) ? RenderQueue::VisibleScroll : RenderQueue::NonVisibleScroll;
    if (immediate)
        render(updateRect);
    else
        m_renderQueue->addToQueue(jobType, updateRect);
}

void BackingStorePrivate::updateTile(const Platform::IntPoint& origin, bool immediate)
{
    if (!isActive())
        return;

    Platform::IntRect updateRect = Platform::IntRect(origin, tileSize());
    RenderQueue::JobType jobType = isTileVisible(origin) ? RenderQueue::VisibleScroll : RenderQueue::NonVisibleScroll;
    if (immediate)
        render(updateRect);
    else
        m_renderQueue->addToQueue(jobType, updateRect);
}

Platform::IntRect BackingStorePrivate::mapFromTilesToTransformedContents(const BackingStorePrivate::TileRect& tileRect) const
{
    return mapFromTilesToTransformedContents(tileRect, frontState()->backingStoreRect());
}

Platform::IntRect BackingStorePrivate::mapFromTilesToTransformedContents(const BackingStorePrivate::TileRect& tileRect, const Platform::IntRect& backingStoreRect) const
{
    TileIndex index = tileRect.first;
    Platform::IntRect rect = tileRect.second;
    // The origin of the tile including the backing store offset.
    const Platform::IntPoint originOfTile = this->originOfTile(index, backingStoreRect);
    rect.move(originOfTile.x(), originOfTile.y());
    return rect;
}

BackingStorePrivate::TileRectList BackingStorePrivate::mapFromTransformedContentsToAbsoluteTileBoundaries(const Platform::IntRect& rect) const
{
    if (!m_webPage->isVisible() || !isActive()) {
        ASSERT_NOT_REACHED();
        return TileRectList();
    }

    TileRectList tileRectList;
    int firstXOffset = rect.x() / tileWidth();
    int firstYOffset = rect.y() / tileHeight();
    int lastXOffset = (rect.right() - 1) / tileWidth();
    int lastYOffset = (rect.bottom() - 1) / tileHeight();
    for (int i = firstXOffset; i <= lastXOffset; ++i) {
        for (int j = firstYOffset; j <= lastYOffset; ++j) {
            const int dstX = (i == firstXOffset) ? rect.x() : i * tileWidth();
            const int dstY = (j == firstYOffset) ? rect.y() : j * tileHeight();
            const int dstRight = (i == lastXOffset) ? rect.right() : (i + 1) * tileWidth();
            const int dstBottom = (j == lastYOffset) ? rect.bottom() : (j + 1) * tileHeight();
            const int srcX = dstX % tileWidth();
            const int srcY = dstY % tileHeight();
            TileRect tileRect;
            tileRect.first = TileIndex(i, j);
            tileRect.second = Platform::IntRect(srcX, srcY, dstRight - dstX, dstBottom - dstY);
            tileRectList.append(tileRect);
        }
    }
    return tileRectList;
}


BackingStorePrivate::TileRectList BackingStorePrivate::mapFromTransformedContentsToTiles(const Platform::IntRect& rect) const
{
    return mapFromTransformedContentsToTiles(rect, frontState());
}

BackingStorePrivate::TileRectList BackingStorePrivate::mapFromTransformedContentsToTiles(const Platform::IntRect& rect, BackingStoreGeometry* state) const
{
    TileMap tileMap = state->tileMap();

    TileRectList tileRectList;
    TileMap::const_iterator end = tileMap.end();
    for (TileMap::const_iterator it = tileMap.begin(); it != end; ++it) {
        TileIndex index = it->first;
        BackingStoreTile* tile = it->second;

        // Need to map the rect to tile coordinates.
        Platform::IntRect r = rect;

        // The origin of the tile including the backing store offset.
        const Platform::IntPoint originOfTile = this->originOfTile(index, state->backingStoreRect());

        r.move(-(originOfTile.x()), -(originOfTile.y()));

        // Do we intersect the current tile or no?
        r.intersect(tile->rect());
        if (r.isEmpty())
            continue;

        // If we do append to list and Voila!
        TileRect tileRect;
        tileRect.first = index;
        tileRect.second = r;
        tileRectList.append(tileRect);
    }
    return tileRectList;
}

void BackingStorePrivate::updateTileMatrixIfNeeded()
{
    // This will update the tile matrix.
    scrollBackingStore(0, 0);
}

void BackingStorePrivate::contentsSizeChanged(const Platform::IntSize&)
{
    updateTileMatrixIfNeeded();
}

void BackingStorePrivate::scrollChanged(const Platform::IntPoint&)
{
    // FIXME: Need to do anything here?
}

void BackingStorePrivate::transformChanged()
{
    if (!m_webPage->isVisible())
        return;

    if (!isActive()) {
        m_renderQueue->reset();
        m_renderQueue->addToQueue(RenderQueue::VisibleZoom, visibleContentsRect());
        m_webPage->d->setShouldResetTilesWhenShown(true);
        return;
    }

    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();

    bool hasCurrentVisibleZoomJob = m_renderQueue->hasCurrentVisibleZoomJob();
    bool isLoading = m_client->isLoading();
    if (isLoading) {
        if (!hasCurrentVisibleZoomJob)
            m_visibleTileBufferRect = visibleContentsRect(); // Cache this for blitVisibleContents.

        // Add the currently visible tiles to the render queue as visible zoom jobs.
        TileRectList tileRectList = mapFromTransformedContentsToTiles(visibleContentsRect());
        for (size_t i = 0; i < tileRectList.size(); ++i) {
            TileRect tileRect = tileRectList[i];
            TileIndex index = tileRect.first;
            Platform::IntRect dirtyTileRect = tileRect.second;
            BackingStoreTile* tile = currentMap.get(index);

            // Invalidate the whole rect.
            tileRect.second = this->tileRect();
            Platform::IntRect wholeRect = mapFromTilesToTransformedContents(tileRect);
            m_renderQueue->addToQueue(RenderQueue::VisibleZoom, wholeRect);

            // Copy the visible contents into the visibleTileBuffer if we don't have
            // any current visible zoom jobs.
            if (!hasCurrentVisibleZoomJob) {
                // Map to the destination's coordinate system.
                Platform::IntPoint difference = this->originOfTile(index) - m_visibleTileBufferRect.location();
                Platform::IntSize offset = Platform::IntSize(difference.x(), difference.y());
                Platform::IntRect dirtyRect = dirtyTileRect;
                dirtyRect.move(offset.width(), offset.height());

                BackingStoreTile* visibleTileBuffer
                    = SurfacePool::globalSurfacePool()->visibleTileBuffer();
                ASSERT(visibleTileBuffer->size() == Platform::IntSize(m_webPage->d->transformedViewportSize()));
                BlackBerry::Platform::Graphics::blitToBuffer(
                    visibleTileBuffer->frontBuffer()->nativeBuffer(), dirtyRect,
                    tile->frontBuffer()->nativeBuffer(), dirtyTileRect);
            }
        }
    }

    m_renderQueue->reset();
    resetTiles(true /*resetBackground*/);
}

void BackingStorePrivate::orientationChanged()
{
    updateTileMatrixIfNeeded();
    createVisibleTileBuffer();
}

void BackingStorePrivate::actualVisibleSizeChanged(const Platform::IntSize& size)
{
}

static void createVisibleTileBufferForWebPage(WebPagePrivate* page)
{
    ASSERT(page);
    SurfacePool* surfacePool = SurfacePool::globalSurfacePool();
    surfacePool->initializeVisibleTileBuffer(page->transformedViewportSize());
}

void BackingStorePrivate::createSurfaces()
{
    BackingStoreGeometry* currentState = frontState();
    TileMap currentMap = currentState->tileMap();

    ASSERT(currentMap.isEmpty());

    if (m_webPage->isVisible()) {
        // This method is only to be called as part of setting up a new web page instance and
        // before said instance is made visible so as to ensure a consistent definition of web
        // page visibility. That is, a web page is said to be visible when explicitly made visible.
        ASSERT_NOT_REACHED();
        return;
    }

    SurfacePool* surfacePool = SurfacePool::globalSurfacePool();
    surfacePool->initialize(tileSize());

    if (surfacePool->isEmpty()) // Settings specify 0 tiles / no backing store.
        return;

    const Divisor divisor = bestDivisor(expandedContentsSize(), tileWidth(), tileHeight(), minimumNumberOfTilesWide(), minimumNumberOfTilesHigh(), m_preferredTileMatrixDimension);

    int numberOfTilesWide = divisor.first;
    int numberOfTilesHigh = divisor.second;

    const SurfacePool::TileList tileList = surfacePool->tileList();
    ASSERT(static_cast<int>(tileList.size()) >= (numberOfTilesWide * numberOfTilesHigh));

    TileMap newTileMap;
    for (int y = 0; y < numberOfTilesHigh; ++y) {
        for (int x = 0; x < numberOfTilesWide; ++x) {
            TileIndex index(x, y);
            newTileMap.add(index, tileList.at(x + y * numberOfTilesWide));
        }
    }

    // Set the initial state of the backingstore geometry.
    backState()->setNumberOfTilesWide(divisor.first);
    backState()->setNumberOfTilesHigh(divisor.second);
    backState()->setTileMap(newTileMap);

    // Swap back/front state.
    swapState();

    createVisibleTileBufferForWebPage(m_webPage->d);

    // Don't try to blit to screen unless we have a buffer.
    if (!buffer())
        suspendScreenAndBackingStoreUpdates();
}

void BackingStorePrivate::createVisibleTileBuffer()
{
    if (!m_webPage->isVisible() || !isActive())
        return;

    createVisibleTileBufferForWebPage(m_webPage->d);
}

Platform::IntPoint BackingStorePrivate::originOfTile(const TileIndex& index) const
{
    return originOfTile(index, frontState()->backingStoreRect());
}

Platform::IntPoint BackingStorePrivate::originOfTile(const TileIndex& index, const Platform::IntRect& backingStoreRect) const
{
    return Platform::IntPoint(backingStoreRect.x() + (index.i() * tileWidth()),
                              backingStoreRect.y() + (index.j() * tileHeight()));
}

int BackingStorePrivate::minimumNumberOfTilesWide() const
{
    // The minimum number of tiles wide required to fill the viewport + 1 tile extra to allow scrolling.
    return static_cast<int>(ceilf(m_client->transformedViewportSize().width() / static_cast<float>(tileWidth()))) + 1;
}

int BackingStorePrivate::minimumNumberOfTilesHigh() const
{
    // The minimum number of tiles high required to fill the viewport + 1 tile extra to allow scrolling.
    return static_cast<int>(ceilf(m_client->transformedViewportSize().height() / static_cast<float>(tileHeight()))) + 1;
}

Platform::IntSize BackingStorePrivate::expandedContentsSize() const
{
    return m_client->transformedContentsSize().expandedTo(m_client->transformedViewportSize());
}

int BackingStorePrivate::tileWidth()
{
    static int tileWidth = BlackBerry::Platform::Graphics::Screen::primaryScreen()->landscapeWidth();
    return tileWidth;
}

int BackingStorePrivate::tileHeight()
{
    static int tileHeight = BlackBerry::Platform::Graphics::Screen::primaryScreen()->landscapeHeight();
    return tileHeight;
}

Platform::IntSize BackingStorePrivate::tileSize()
{
    return Platform::IntSize(tileWidth(), tileHeight());
}

Platform::IntRect BackingStorePrivate::tileRect()
{
    return Platform::IntRect(0, 0, tileWidth(), tileHeight());
}

void BackingStorePrivate::renderContents(Platform::Graphics::Drawable* drawable,
                                         const Platform::IntRect& contentsRect,
                                         const Platform::IntSize& destinationSize) const
{
    if (!drawable || contentsRect.isEmpty())
        return;

    requestLayoutIfNeeded();

    PlatformGraphicsContext* platformGraphicsContext = SurfacePool::globalSurfacePool()->createPlatformGraphicsContext(drawable);
    GraphicsContext graphicsContext(platformGraphicsContext);

    graphicsContext.translate(-contentsRect.x(), -contentsRect.y());

    WebCore::IntRect transformedContentsRect(contentsRect.x(), contentsRect.y(), contentsRect.width(), contentsRect.height());

    float widthScale = static_cast<float>(destinationSize.width()) / contentsRect.width();
    float heightScale = static_cast<float>(destinationSize.height()) / contentsRect.height();

    if (widthScale != 1.0 && heightScale != 1.0) {
        TransformationMatrix matrix;
        matrix.scaleNonUniform(1.0 / widthScale, 1.0 / heightScale);
        transformedContentsRect = matrix.mapRect(transformedContentsRect);

        // We extract from the contentsRect but draw a slightly larger region than
        // we were told to, in order to avoid pixels being rendered only partially.
        const int atLeastOneDevicePixel = static_cast<int>(ceilf(std::max(1.0 / widthScale, 1.0 / heightScale)));
        transformedContentsRect.inflate(atLeastOneDevicePixel);
        graphicsContext.scale(FloatSize(widthScale, heightScale));
    }

    graphicsContext.clip(transformedContentsRect);
    m_client->frame()->view()->paintContents(&graphicsContext, transformedContentsRect);

    delete platformGraphicsContext;
}

void BackingStorePrivate::renderContents(BlackBerry::Platform::Graphics::Buffer* tileBuffer,
                                         const Platform::IntPoint& surfaceOffset,
                                         const Platform::IntRect& contentsRect) const
{
    // If tileBuffer == 0, we render directly to the window.
    if (!m_webPage->isVisible() && tileBuffer)
        return;

#if DEBUG_BACKINGSTORE
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
                           "BackingStorePrivate::renderContents tileBuffer=0x%x surfaceOffset=(%d,%d) contentsRect=(%d,%d %dx%d)",
                           tileBuffer, surfaceOffset.x(), surfaceOffset.y(),
                           contentsRect.x(), contentsRect.y(), contentsRect.width(), contentsRect.height());
#endif

    // It is up to callers of this method to perform layout themselves!
    ASSERT(!m_webPage->d->mainFrame()->view()->needsLayout());

    Platform::IntSize contentsSize = m_client->contentsSize();
    Color backgroundColor(m_webPage->settings()->backgroundColor());

    BlackBerry::Platform::Graphics::Buffer* targetBuffer = tileBuffer
        ? tileBuffer
        : buffer();

    if (contentsSize.isEmpty()
        || !Platform::IntRect(Platform::IntPoint(0, 0), m_client->transformedContentsSize()).contains(contentsRect)
        || backgroundColor.hasAlpha()) {
        // Clear the area if it's not fully covered by (opaque) contents.
        BlackBerry::Platform::IntRect clearRect = BlackBerry::Platform::IntRect(
            contentsRect.x() - surfaceOffset.x(), contentsRect.y() - surfaceOffset.y(),
            contentsRect.width(), contentsRect.height());

        BlackBerry::Platform::Graphics::clearBuffer(targetBuffer, clearRect,
            backgroundColor.red(), backgroundColor.green(),
            backgroundColor.blue(), backgroundColor.alpha());
    }

    if (contentsSize.isEmpty())
        return;

    BlackBerry::Platform::Graphics::Drawable* bufferDrawable =
        BlackBerry::Platform::Graphics::lockBufferDrawable(targetBuffer);

    PlatformGraphicsContext* bufferPlatformGraphicsContext = bufferDrawable
        ? SurfacePool::globalSurfacePool()->createPlatformGraphicsContext(bufferDrawable)
        : 0;
    PlatformGraphicsContext* targetPlatformGraphicsContext = bufferPlatformGraphicsContext
        ? bufferPlatformGraphicsContext
        : SurfacePool::globalSurfacePool()->lockTileRenderingSurface();

    ASSERT(targetPlatformGraphicsContext);

    {
        GraphicsContext graphicsContext(targetPlatformGraphicsContext);

        // Believe it or not this is important since the WebKit Skia backend
        // doesn't store the original state unless you call save first :P
        graphicsContext.save();

        // Translate context according to offset.
        graphicsContext.translate(-surfaceOffset.x(), -surfaceOffset.y());

        // Add our transformation matrix as the global transform.
        AffineTransform affineTransform(
            m_webPage->d->transformationMatrix()->a(),
            m_webPage->d->transformationMatrix()->b(),
            m_webPage->d->transformationMatrix()->c(),
            m_webPage->d->transformationMatrix()->d(),
            m_webPage->d->transformationMatrix()->e(),
            m_webPage->d->transformationMatrix()->f());
        graphicsContext.concatCTM(affineTransform);

        // Now that the matrix is applied we need untranformed contents coordinates.
        Platform::IntRect untransformedContentsRect = m_webPage->d->mapFromTransformed(contentsRect);

        // We extract from the contentsRect but draw a slightly larger region than
        // we were told to, in order to avoid pixels being rendered only partially.
        const int atLeastOneDevicePixel =
            static_cast<int>(ceilf(1.0 / m_webPage->d->transformationMatrix()->a()));
        untransformedContentsRect.inflate(atLeastOneDevicePixel, atLeastOneDevicePixel);

        // Make sure the untransformed rectangle for the (slightly larger than
        // initially requested) repainted region is within the bounds of the page.
        untransformedContentsRect.intersect(Platform::IntRect(Platform::IntPoint(0, 0), contentsSize));

        // Some WebKit painting backends *cough* Skia *cough* don't set this automatically
        // to the dirtyRect so do so here explicitly.
        graphicsContext.clip(untransformedContentsRect);

        // Take care of possible left overflow on RTL page.
        if (int leftOverFlow = m_client->frame()->view()->minimumScrollPosition().x()) {
            untransformedContentsRect.move(leftOverFlow, 0);
            graphicsContext.translate(-leftOverFlow, 0);
        }

        // Let WebCore render the page contents into the drawing surface.
        m_client->frame()->view()->paintContents(&graphicsContext, untransformedContentsRect);

        graphicsContext.restore();
    }

    // Grab the requested region from the drawing surface into the tile image.

    delete bufferPlatformGraphicsContext;

    if (bufferDrawable)
        releaseBufferDrawable(targetBuffer);
    else {
        const Platform::IntPoint dstPoint(contentsRect.x() - surfaceOffset.x(),
                                          contentsRect.y() - surfaceOffset.y());
        const Platform::IntRect dstRect(dstPoint, contentsRect.size());
        const Platform::IntRect srcRect = dstRect;

        // If we couldn't directly draw to the buffer, copy from the drawing surface.
        SurfacePool::globalSurfacePool()->releaseTileRenderingSurface(targetPlatformGraphicsContext);
        BlackBerry::Platform::Graphics::blitToBuffer(targetBuffer, dstRect, BlackBerry::Platform::Graphics::drawingSurface(), srcRect);
    }
}

#if DEBUG_FAT_FINGERS
static void drawDebugRect(BlackBerry::Platform::Graphics::Buffer* dstBuffer, const Platform::IntRect& dstRect, const Platform::IntRect& srcRect, unsigned char red, unsigned char green, unsigned char blue)
{
    Platform::IntRect drawRect(srcRect);
    drawRect.intersect(dstRect);
    if (!drawRect.isEmpty())
        BlackBerry::Platform::Graphics::clearBuffer(dstBuffer, drawRect, red, green, blue, 128);
}
#endif

void BackingStorePrivate::blitToWindow(const Platform::IntRect& dstRect,
                                       const BlackBerry::Platform::Graphics::Buffer* srcBuffer,
                                       const Platform::IntRect& srcRect,
                                       bool blend,
                                       unsigned char globalAlpha)
{
    ASSERT(BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread());

    windowFrontBufferState()->clearBlittedRegion(dstRect);
    windowBackBufferState()->addBlittedRegion(dstRect);

    BlackBerry::Platform::Graphics::Buffer* dstBuffer = buffer();
    ASSERT(dstBuffer);
    ASSERT(srcBuffer);
    if (!dstBuffer)
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelWarn, "Empty window buffer, couldn't blitToWindow");

    BlackBerry::Platform::Graphics::BlendMode blendMode = blend
        ? BlackBerry::Platform::Graphics::SourceOver
        : BlackBerry::Platform::Graphics::SourceCopy;

    BlackBerry::Platform::Graphics::blitToBuffer(dstBuffer, dstRect, srcBuffer, srcRect, blendMode, globalAlpha);

#if DEBUG_FAT_FINGERS
    drawDebugRect(dstBuffer, dstRect, FatFingers::m_debugFatFingerRect, 210, 210, 250);
    drawDebugRect(dstBuffer, dstRect, Platform::IntRect(FatFingers::m_debugFatFingerClickPosition, Platform::IntSize(3, 3)), 0, 0, 0);
    drawDebugRect(dstBuffer, dstRect, Platform::IntRect(FatFingers::m_debugFatFingerAdjustedPosition, Platform::IntSize(5, 5)), 100, 100, 100);
#endif

}

void BackingStorePrivate::fillWindow(Platform::Graphics::FillPattern pattern,
                                     const Platform::IntRect& dstRect,
                                     const Platform::IntPoint& contentsOrigin,
                                     double contentsScale)
{
    ASSERT(BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread());

    windowFrontBufferState()->clearBlittedRegion(dstRect);
    windowBackBufferState()->addBlittedRegion(dstRect);

    BlackBerry::Platform::Graphics::Buffer* dstBuffer = buffer();
    ASSERT(dstBuffer);
    if (!dstBuffer)
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelWarn, "Empty window buffer, couldn't fillWindow");

    BlackBerry::Platform::Graphics::fillBuffer(dstBuffer, pattern, dstRect, contentsOrigin, contentsScale);
}

void BackingStorePrivate::invalidateWindow()
{
    // Grab a rect appropriate for the current thread.
    if (BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread())
        invalidateWindow(m_webPage->client()->userInterfaceBlittedDestinationRect());
    else
        invalidateWindow(Platform::IntRect(Platform::IntPoint(0, 0), m_client->transformedViewportSize()));
}

void BackingStorePrivate::invalidateWindow(const Platform::IntRect& dst)
{
    if (dst.isEmpty())
        return;

    if (!BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread() && !shouldDirectRenderingToWindow()) {
        // This needs to be sync in order to swap the recently drawn thing...
        // This will only be called from WebKit thread during direct rendering.
        typedef void (BlackBerry::WebKit::BackingStorePrivate::*FunctionType)(const Platform::IntRect&);
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
            BlackBerry::Platform::createMethodCallMessage<FunctionType, BackingStorePrivate, Platform::IntRect>(
                &BackingStorePrivate::invalidateWindow, this, dst));
        return;
    }

#if DEBUG_BACKINGSTORE
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::invalidateWindow dst = %s", dst.toString().c_str());
#endif

    // Since our window may also be double buffered, we need to also copy the
    // front buffer's contents to the back buffer before we swap them. It is
    // analogous to what we do with our double buffered tiles by calling
    // copyPreviousContentsToBackingSurfaceOfTile(). It only affects partial
    // screen updates since when we are scrolling or zooming, the whole window
    // is invalidated anyways and no copying is needed.
    copyPreviousContentsToBackSurfaceOfWindow();

    Platform::IntRect dstRect = dst;

    Platform::IntRect viewportRect(Platform::IntPoint(0, 0), m_client->transformedViewportSize());
    dstRect.intersect(viewportRect);

    if (dstRect.width() <= 0 || dstRect.height() <= 0)
        return;

#if DEBUG_BACKINGSTORE
    BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "BackingStorePrivate::invalidateWindow posting = %s", dstRect.toString().c_str());
#endif

    m_currentWindowBackBuffer = (m_currentWindowBackBuffer + 1) % 2;
    if (Window* window = m_webPage->client()->window())
        window->post(dstRect);
}

void BackingStorePrivate::clearWindow(const Platform::IntRect& rect,
                                      unsigned char red,
                                      unsigned char green,
                                      unsigned char blue,
                                      unsigned char alpha)
{
    if (!BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread() && !shouldDirectRenderingToWindow()) {
        typedef void (BlackBerry::WebKit::BackingStorePrivate::*FunctionType)(const Platform::IntRect&,
                                                                           unsigned char,
                                                                           unsigned char,
                                                                           unsigned char,
                                                                           unsigned char);
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchMessage(
            BlackBerry::Platform::createMethodCallMessage<FunctionType,
                                                       BackingStorePrivate,
                                                       Platform::IntRect,
                                                       unsigned char,
                                                       unsigned char,
                                                       unsigned char,
                                                       unsigned char>(
                &BackingStorePrivate::clearWindow, this, rect, red, green, blue, alpha));
        return;
    }

    BlackBerry::Platform::Graphics::Buffer* dstBuffer = buffer();
    ASSERT(dstBuffer);
    if (!dstBuffer)
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelWarn, "Empty window buffer, couldn't clearWindow");

    windowFrontBufferState()->clearBlittedRegion(rect);
    windowBackBufferState()->addBlittedRegion(rect);

    BlackBerry::Platform::Graphics::clearBuffer(dstBuffer, rect, red, green, blue, alpha);
}

bool BackingStorePrivate::isScrollingOrZooming() const
{
    BackingStoreMutexLocker locker(const_cast<BackingStorePrivate*>(this));
    return m_isScrollingOrZooming;
}

void BackingStorePrivate::setScrollingOrZooming(bool scrollingOrZooming, bool shouldBlit)
{
    {
        BackingStoreMutexLocker locker(this);
        m_isScrollingOrZooming = scrollingOrZooming;
    }

#if !ENABLE_REPAINTONSCROLL
    m_suspendRenderJobs = scrollingOrZooming; // Suspend the rendering of everything.
#endif

    if (!m_webPage->settings()->shouldRenderAnimationsOnScrollOrZoom())
        m_suspendRegularRenderJobs = scrollingOrZooming; // Suspend the rendering of animations.

    m_webPage->d->m_mainFrame->view()->setConstrainsScrollingToContentEdge(!scrollingOrZooming);

    // Clear this flag since we don't care if the render queue is under pressure
    // or not since we are scrolling and it is more important to not lag than
    // it is to ensure animations achieve better framerates!
    if (scrollingOrZooming)
        m_renderQueue->setCurrentRegularRenderJobBatchUnderPressure(false);
#if ENABLE_SCROLLBARS
    else if (shouldBlit && !shouldDirectRenderingToWindow())
        blitVisibleContents();
#endif
}

void BackingStorePrivate::lockBackingStore()
{
    pthread_mutex_lock(&m_mutex);
}

void BackingStorePrivate::unlockBackingStore()
{
    pthread_mutex_unlock(&m_mutex);
}

BackingStoreGeometry* BackingStorePrivate::frontState() const
{
    return reinterpret_cast<BackingStoreGeometry*>(m_frontState);
}

BackingStoreGeometry* BackingStorePrivate::backState() const
{
    return reinterpret_cast<BackingStoreGeometry*>(m_backState);
}

void BackingStorePrivate::swapState()
{
    unsigned front = reinterpret_cast<unsigned>(frontState());
    unsigned back = reinterpret_cast<unsigned>(backState());

    // Atomic change.
    _smp_xchg(&m_frontState, back);
    _smp_xchg(&m_backState, front);
    BlackBerry::Platform::userInterfaceThreadMessageClient()->syncToCurrentMessage();
}

BackingStoreWindowBufferState* BackingStorePrivate::windowFrontBufferState() const
{
    return &m_windowBufferState[(m_currentWindowBackBuffer + 1) % 2];
}

BackingStoreWindowBufferState* BackingStorePrivate::windowBackBufferState() const
{
    return &m_windowBufferState[m_currentWindowBackBuffer];
}

#if USE(ACCELERATED_COMPOSITING)
void BackingStorePrivate::drawAndBlendLayersForDirectRendering(const Platform::IntRect& dirtyRect)
{
    ASSERT(BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread());
    if (!BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread())
        return;

    // Because we're being called sync from the WebKit thread, we can use
    // regular WebPage size and transformation functions without concerns.
    WebCore::IntRect contentsRect = visibleContentsRect();
    WebCore::FloatRect untransformedContentsRect = m_webPage->d->mapFromTransformedFloatRect(WebCore::FloatRect(contentsRect));
    WebCore::IntRect contentsScreenRect = m_client->mapFromTransformedContentsToTransformedViewport(contentsRect);
    WebCore::IntRect dstRect = intersection(contentsScreenRect,
        WebCore::IntRect(WebCore::IntPoint(0, 0), m_webPage->d->transformedViewportSize()));

    // Check if rendering caused a commit and we need to redraw the layers.
    if (WebPageCompositorPrivate* compositor = m_webPage->d->compositor())
        compositor->drawLayers(dstRect, untransformedContentsRect);

#if ENABLE_COMPOSITING_SURFACE
    // See above comment about sync calling, visibleContentsRect() is safe here.
    Platform::IntRect visibleDirtyRect = dirtyRect;
    visibleDirtyRect.intersect(visibleContentsRect());
    visibleDirtyRect = m_client->mapFromTransformedContentsToTransformedViewport(visibleDirtyRect);

    blendCompositingSurface(visibleDirtyRect);
#endif
}
#endif

bool BackingStorePrivate::isActive() const
{
    return BackingStorePrivate::s_currentBackingStoreOwner == m_webPage && SurfacePool::globalSurfacePool()->isActive();
}

void BackingStorePrivate::didRenderContent(const Platform::IntRect& renderedRect)
{
    if (isScrollingOrZooming())
        return;

    if (!shouldDirectRenderingToWindow()) {
        if (!m_webPage->d->needsOneShotDrawingSynchronization())
            blitVisibleContents();
    } else
        invalidateWindow();

    m_webPage->client()->notifyContentRendered(renderedRect);
}

BackingStore::BackingStore(WebPage* webPage, BackingStoreClient* client)
    : d(new BackingStorePrivate)
{
    d->m_webPage = webPage;
    d->m_client = client;
}

BackingStore::~BackingStore()
{
    deleteGuardedObject(d);
    d = 0;
}

void BackingStore::createSurface()
{
    static bool initialized = false;
    if (!initialized) {
        BlackBerry::Platform::Graphics::initialize();
        initialized = true;
    }

    // Triggers creation of surfaces in backingstore.
    d->createSurfaces();

    // Focusing the WebPage triggers a repaint, so while we want it to be
    // focused initially this has to happen after creation of the surface.
    d->m_webPage->setFocused(true);
}

void BackingStore::suspendScreenAndBackingStoreUpdates()
{
    d->suspendScreenAndBackingStoreUpdates();
}

void BackingStore::resumeScreenAndBackingStoreUpdates(ResumeUpdateOperation op)
{
    d->resumeScreenAndBackingStoreUpdates(op);
}

bool BackingStore::isScrollingOrZooming() const
{
    return d->isScrollingOrZooming();
}

void BackingStore::setScrollingOrZooming(bool scrollingOrZooming)
{
    d->setScrollingOrZooming(scrollingOrZooming);
}

void BackingStore::blitVisibleContents()
{
    d->blitVisibleContents(false /*force*/);
}

void BackingStore::blitContents(const BlackBerry::Platform::IntRect& dstRect, const BlackBerry::Platform::IntRect& contents)
{
    // Blitting during direct rendering is not supported.
    if (isDirectRenderingToWindow()) {
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical,
                               "BackingStore::blitContents operation not supported in direct rendering mode");
        return;
    }

    d->blitContents(dstRect, contents);
}

void BackingStore::repaint(int x, int y, int width, int height,
                           bool contentChanged, bool immediate)
{
    d->repaint(Platform::IntRect(x, y, width, height), contentChanged, immediate);
}

bool BackingStore::isDirectRenderingToWindow() const
{
    BackingStoreMutexLocker locker(d);
    return d->shouldDirectRenderingToWindow();
}

void BackingStore::createBackingStoreMemory()
{
    if (BackingStorePrivate::s_currentBackingStoreOwner == d->m_webPage)
        SurfacePool::globalSurfacePool()->createBuffers();
}

void BackingStore::releaseBackingStoreMemory()
{
    if (BackingStorePrivate::s_currentBackingStoreOwner == d->m_webPage)
        SurfacePool::globalSurfacePool()->releaseBuffers();
}

bool BackingStore::defersBlit() const
{
        return d->m_defersBlit;
}

void BackingStore::setDefersBlit(bool b)
{
        d->m_defersBlit = b;
}

bool BackingStore::hasBlitJobs() const
{
#if USE(ACCELERATED_COMPOSITING)
    // If there's a WebPageCompositorClient, let it schedule the blit.
    WebPageCompositorPrivate* compositor = d->m_webPage->d->compositor();
    if (compositor && compositor->client())
        return false;
#endif

    // Normally, this would be called from the compositing thread,
    // and the flag is set on the compositing thread, so no need for
    // synchronization.
    return d->m_hasBlitJobs;
}

void BackingStore::blitOnIdle()
{
#if USE(ACCELERATED_COMPOSITING)
    // If there's a WebPageCompositorClient, let it schedule the blit.
    WebPageCompositorPrivate* compositor = d->m_webPage->d->compositor();
    if (compositor && compositor->client())
        return;
#endif

    d->blitVisibleContents(true /*force*/);
}

Platform::IntSize BackingStorePrivate::surfaceSize() const
{
    if (Window* window = m_webPage->client()->window())
        return window->surfaceSize();

#if USE(ACCELERATED_COMPOSITING)
    if (WebPageCompositorPrivate* compositor = m_webPage->d->compositor())
        return compositor->context()->surfaceSize();
#endif

    return Platform::IntSize();
}

Platform::Graphics::Buffer* BackingStorePrivate::buffer() const
{
    if (Window* window = m_webPage->client()->window())
        return window->buffer();

#if USE(ACCELERATED_COMPOSITING)
    if (WebPageCompositorPrivate* compositor = m_webPage->d->compositor())
        return compositor->context() ? compositor->context()->buffer() : 0;
#endif

    return 0;
}

void BackingStore::drawContents(Platform::Graphics::Drawable* drawable, const Platform::IntRect& contentsRect, const Platform::IntSize& destinationSize)
{
    d->renderContents(drawable, contentsRect, destinationSize);
}

}
}
