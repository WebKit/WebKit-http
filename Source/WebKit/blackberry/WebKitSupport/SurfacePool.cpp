/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "SurfacePool.h"

#include "PlatformContextSkia.h"

#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformMisc.h>
#include <BlackBerryPlatformScreen.h>
#include <BlackBerryPlatformSettings.h>
#include <BlackBerryPlatformThreading.h>
#include <errno.h>

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
#include <EGL/eglext.h>
#endif

#define SHARED_PIXMAP_GROUP "webkit_backingstore_group"

namespace BlackBerry {
namespace WebKit {

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
#endif

SurfacePool* SurfacePool::globalSurfacePool()
{
    static SurfacePool* s_instance = 0;
    if (!s_instance)
        s_instance = new SurfacePool;
    return s_instance;
}

SurfacePool::SurfacePool()
    : m_visibleTileBuffer(0)
    , m_tileRenderingSurface(0)
    , m_backBuffer(0)
    , m_initialized(false)
    , m_buffersSuspended(false)
    , m_hasFenceExtension(false)
{
}

void SurfacePool::initialize(const Platform::IntSize& tileSize)
{
    if (m_initialized)
        return;
    m_initialized = true;

    const unsigned numberOfTiles = Platform::Settings::instance()->numberOfBackingStoreTiles();
    const unsigned maxNumberOfTiles = Platform::Settings::instance()->maximumNumberOfBackingStoreTilesAcrossProcesses();

    if (numberOfTiles) { // Only allocate if we actually use a backingstore.
        unsigned byteLimit = (maxNumberOfTiles /*pool*/ + 2 /*visible tile buffer, backbuffer*/) * tileSize.width() * tileSize.height() * 4;
        bool success = Platform::Graphics::createPixmapGroup(SHARED_PIXMAP_GROUP, byteLimit);
        if (!success) {
            BBLOG(Platform::LogLevelWarn,
                "Shared buffer pool could not be set up, using regular memory allocation instead.");
        }
    }

    m_tileRenderingSurface = Platform::Graphics::drawingSurface();

    if (!numberOfTiles)
        return; // we only use direct rendering when 0 tiles are specified.

    // Create the shared backbuffer.
    m_backBuffer = reinterpret_cast<unsigned>(new TileBuffer(tileSize));

    for (size_t i = 0; i < numberOfTiles; ++i)
        m_tilePool.append(BackingStoreTile::create(tileSize, BackingStoreTile::DoubleBuffered));

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
    const char* extensions = eglQueryString(Platform::Graphics::eglDisplay(), EGL_EXTENSIONS);
    if (strstr(extensions, "EGL_KHR_fence_sync")) {
        // We assume GL_OES_EGL_sync is present, but we don't check for it because
        // no GL context is current at this point.
        // TODO: check for it
        eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC) eglGetProcAddress("eglCreateSyncKHR");
        eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC) eglGetProcAddress("eglDestroySyncKHR");
        eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC) eglGetProcAddress("eglClientWaitSyncKHR");
        m_hasFenceExtension = eglCreateSyncKHR && eglDestroySyncKHR && eglClientWaitSyncKHR;
    }
#endif

    // m_mutex must be recursive because destroyPlatformSync may be called indirectly
    // from notifyBuffersComposited
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

PlatformGraphicsContext* SurfacePool::createPlatformGraphicsContext(Platform::Graphics::Drawable* drawable) const
{
    return new WebCore::PlatformContextSkia(drawable);
}

PlatformGraphicsContext* SurfacePool::lockTileRenderingSurface() const
{
    if (!m_tileRenderingSurface)
        return 0;

    return createPlatformGraphicsContext(Platform::Graphics::lockBufferDrawable(m_tileRenderingSurface));
}

void SurfacePool::releaseTileRenderingSurface(PlatformGraphicsContext* context) const
{
    if (!m_tileRenderingSurface)
        return;

    delete context;
    Platform::Graphics::releaseBufferDrawable(m_tileRenderingSurface);
}

void SurfacePool::initializeVisibleTileBuffer(const Platform::IntSize& visibleSize)
{
    if (!m_visibleTileBuffer || m_visibleTileBuffer->size() != visibleSize) {
        delete m_visibleTileBuffer;
        m_visibleTileBuffer = BackingStoreTile::create(visibleSize, BackingStoreTile::SingleBuffered);
    }
}

TileBuffer* SurfacePool::backBuffer() const
{
    ASSERT(m_backBuffer);
    return reinterpret_cast<TileBuffer*>(m_backBuffer);
}

const char *SurfacePool::sharedPixmapGroup() const
{
    return SHARED_PIXMAP_GROUP;
}

void SurfacePool::createBuffers()
{
    if (!m_initialized || m_tilePool.isEmpty() || !m_buffersSuspended)
        return;

    // Create the tile pool.
    for (size_t i = 0; i < m_tilePool.size(); ++i) {
        if (m_tilePool[i]->frontBuffer()->wasNativeBufferCreated())
            Platform::Graphics::createPixmapBuffer(m_tilePool[i]->frontBuffer()->nativeBuffer());
    }

    if (m_visibleTileBuffer && m_visibleTileBuffer->frontBuffer()->wasNativeBufferCreated())
        Platform::Graphics::createPixmapBuffer(m_visibleTileBuffer->frontBuffer()->nativeBuffer());

    if (backBuffer() && backBuffer()->wasNativeBufferCreated())
        Platform::Graphics::createPixmapBuffer(backBuffer()->nativeBuffer());

    m_buffersSuspended = false;
}

void SurfacePool::releaseBuffers()
{
    if (!m_initialized || m_tilePool.isEmpty() || m_buffersSuspended)
        return;

    m_buffersSuspended = true;

    // Release the tile pool.
    for (size_t i = 0; i < m_tilePool.size(); ++i) {
        if (!m_tilePool[i]->frontBuffer()->wasNativeBufferCreated())
            continue;
        m_tilePool[i]->frontBuffer()->clearRenderedRegion();
        // Clear the buffer to prevent accidental leakage of (possibly sensitive) pixel data.
        Platform::Graphics::clearBuffer(m_tilePool[i]->frontBuffer()->nativeBuffer(), 0, 0, 0, 0);
        Platform::Graphics::destroyPixmapBuffer(m_tilePool[i]->frontBuffer()->nativeBuffer());
    }

    if (m_visibleTileBuffer && m_visibleTileBuffer->frontBuffer()->wasNativeBufferCreated()) {
        m_visibleTileBuffer->frontBuffer()->clearRenderedRegion();
        Platform::Graphics::clearBuffer(m_visibleTileBuffer->frontBuffer()->nativeBuffer(), 0, 0, 0, 0);
        Platform::Graphics::destroyPixmapBuffer(m_visibleTileBuffer->frontBuffer()->nativeBuffer());
    }

    if (backBuffer() && backBuffer()->wasNativeBufferCreated()) {
        backBuffer()->clearRenderedRegion();
        Platform::Graphics::clearBuffer(backBuffer()->nativeBuffer(), 0, 0, 0, 0);
        Platform::Graphics::destroyPixmapBuffer(backBuffer()->nativeBuffer());
    }

    Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
        Platform::createFunctionCallMessage(&Platform::Graphics::collectThreadSpecificGarbage));
}

void SurfacePool::waitForBuffer(TileBuffer* tileBuffer)
{
    if (!m_hasFenceExtension)
        return;

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
    EGLSyncKHR platformSync;

    {
        Platform::MutexLocker locker(&m_mutex);
        platformSync = tileBuffer->fence()->takePlatformSync();
    }

    if (!platformSync)
        return;

    if (!eglClientWaitSyncKHR(Platform::Graphics::eglDisplay(), platformSync, 0, 100000000LL))
        Platform::logAlways(Platform::LogLevelWarn, "Failed to wait for EGLSyncKHR object!\n");

    // Instead of assuming eglDestroySyncKHR is thread safe, we add it to
    // a garbage list for later collection on the thread that created it.
    {
        Platform::MutexLocker locker(&m_mutex);
        m_garbage.insert(platformSync);
    }
#endif
}

void SurfacePool::notifyBuffersComposited(const Vector<TileBuffer*>& tileBuffers)
{
    if (!m_hasFenceExtension)
        return;

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
    Platform::MutexLocker locker(&m_mutex);

    EGLDisplay display = Platform::Graphics::eglDisplay();

    // The EGL_KHR_fence_sync spec is nice enough to specify that the sync object
    // is not actually deleted until everyone has stopped using it.
    for (std::set<void*>::const_iterator it = m_garbage.begin(); it != m_garbage.end(); ++it)
        eglDestroySyncKHR(display, *it);
    m_garbage.clear();

    // If we didn't blit anything, we don't need to create a new fence.
    if (tileBuffers.isEmpty())
        return;

    // Create a new fence and assign to the tiles that were blit. Invalidate any previous
    // fence that may be active among these tiles and add its sync object to the garbage set
    // for later destruction to make sure it doesn't leak.
    RefPtr<Fence> fence = Fence::create(eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, 0));
    for (unsigned int i = 0; i < tileBuffers.size(); ++i)
        tileBuffers[i]->setFence(fence);
#endif
}

void SurfacePool::destroyPlatformSync(void* platformSync)
{
#if BLACKBERRY_PLATFORM_GRAPHICS_EGL && USE(SKIA)
    Platform::MutexLocker locker(&m_mutex);
    m_garbage.insert(platformSync);
#endif
}

}
}
