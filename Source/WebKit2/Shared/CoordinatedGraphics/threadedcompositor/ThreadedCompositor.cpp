/*
 * Copyright (C) 2014 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ThreadedCompositor.h"

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "CompositingRunLoop.h"
#include "TCDisplayRefreshMonitor.h"
#include <WebCore/PlatformDisplay.h>
#include <WebCore/TransformationMatrix.h>
#include <cstdio>
#include <cstdlib>

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif

#if PLATFORM(WPE)
#include <WebCore/GLContextWPE.h>
#endif

using namespace WebCore;

namespace WebKit {

Ref<ThreadedCompositor> ThreadedCompositor::create(Client& client, WebPage& webPage, const IntSize& viewportSize, float scaleFactor, uint64_t nativeSurfaceHandle, ShouldDoFrameSync doFrameSync, TextureMapper::PaintFlags paintFlags)
{
    return adoptRef(*new ThreadedCompositor(client, webPage, viewportSize, scaleFactor, nativeSurfaceHandle, doFrameSync, paintFlags));
}

ThreadedCompositor::ThreadedCompositor(Client& client, WebPage& webPage, const IntSize& viewportSize, float scaleFactor, uint64_t nativeSurfaceHandle, ShouldDoFrameSync doFrameSync, TextureMapper::PaintFlags paintFlags)
    : m_client(client)
    , m_viewportSize(viewportSize)
    , m_scaleFactor(scaleFactor)
    , m_nativeSurfaceHandle(nativeSurfaceHandle)
#if PLATFORM(GTK)
    , m_doFrameSync(doFrameSync)
#endif
    , m_paintFlags(paintFlags)
    , m_needsResize(!viewportSize.isEmpty())
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    , m_displayRefreshMonitor(adoptRef(new WebKit::DisplayRefreshMonitor(*this)))
#endif
    , m_compositingRunLoop(std::make_unique<CompositingRunLoop>([this] { renderLayerTree(); }))
{
    m_clientRendersNextFrame.store(false);
    m_coordinateUpdateCompletionWithClient.store(false);

    m_compositingManager.establishConnection(webPage);

    m_compositingRunLoop->performTaskSync([this, protectedThis = makeRef(*this)] {
        m_scene = adoptRef(new CoordinatedGraphicsScene(this));
#if PLATFORM(GTK)
        if (m_nativeSurfaceHandle) {
            createGLContext();
            m_scene->setActive(true);
        } else
            m_scene->setActive(false);
#endif
    });
}

ThreadedCompositor::~ThreadedCompositor()
{
}

void ThreadedCompositor::createGLContext()
{
    ASSERT(!isMainThread());

#if PLATFORM(GTK)
    ASSERT(m_nativeSurfaceHandle);

    m_context = GLContext::createContextForWindow(reinterpret_cast<GLNativeWindowType>(m_nativeSurfaceHandle), &PlatformDisplay::sharedDisplayForCompositing());
    if (!m_context)
        return;
#endif

#if PLATFORM(WPE)
    RELEASE_ASSERT(is<PlatformDisplayWPE>(PlatformDisplay::sharedDisplay()));
    m_target = downcast<PlatformDisplayWPE>(PlatformDisplay::sharedDisplay()).createEGLTarget(*this, m_compositingManager.releaseConnectionFd());
    ASSERT(m_target);
    m_target->initialize(m_viewportSize);

    m_context = m_target->createGLContext();
    if (!m_context)
        return;

    if (!m_context->makeContextCurrent())
        return;
#endif

#if PLATFORM(GTK)
    if (m_doFrameSync == ShouldDoFrameSync::No)
        m_context->swapInterval(0);
#endif
}

void ThreadedCompositor::invalidate()
{
    m_scene->detach();
    m_compositingRunLoop->stopUpdates();
    m_compositingRunLoop->performTaskSync([this, protectedThis = makeRef(*this)] {
        m_scene->purgeGLResources();
        m_context = nullptr;
#if PLATFORM(WPE)
        m_target = nullptr;
#endif
        m_scene = nullptr;
    });
    m_compositingRunLoop = nullptr;
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    m_displayRefreshMonitor->invalidate();
#endif
}

void ThreadedCompositor::setNativeSurfaceHandleForCompositing(uint64_t handle)
{
#if PLATFORM(GTK)
    m_compositingRunLoop->stopUpdates();
    m_compositingRunLoop->performTaskSync([this, protectedThis = makeRef(*this), handle] {
        // A new native handle can't be set without destroying the previous one first if any.
        ASSERT(!!handle ^ !!m_nativeSurfaceHandle);
        m_nativeSurfaceHandle = handle;
        if (m_nativeSurfaceHandle) {
            createGLContext();
            m_scene->setActive(true);
        } else {
            m_scene->setActive(false);
            m_context = nullptr;
        }
        m_nativeSurfaceHandle = 0;
    });
#endif
}

void ThreadedCompositor::setScaleFactor(float scale)
{
    m_compositingRunLoop->performTask([this, protectedThis = makeRef(*this), scale] {
        m_scaleFactor = scale;
        scheduleDisplayImmediately();
    });
}

void ThreadedCompositor::setScrollPosition(const IntPoint& scrollPosition, float scale)
{
    m_compositingRunLoop->performTask([this, protectedThis = makeRef(*this), scrollPosition, scale] {
        m_scrollPosition = scrollPosition;
        m_scaleFactor = scale;
        scheduleDisplayImmediately();
    });
}

void ThreadedCompositor::setViewportSize(const IntSize& viewportSize, float scale)
{
    m_compositingRunLoop->performTaskSync([this, protectedThis = makeRef(*this), viewportSize, scale] {
        m_viewportSize = viewportSize;
#if PLATFORM(WPE)
        if (m_target)
            m_target->resize(viewportSize);
#endif
        m_scaleFactor = scale;
        m_needsResize = true;
        scheduleDisplayImmediately();
    });
}

void ThreadedCompositor::setDrawsBackground(bool drawsBackground)
{
    m_compositingRunLoop->performTask([this, protectedThis = Ref<ThreadedCompositor>(*this), drawsBackground] {
        m_drawsBackground = drawsBackground;
        scheduleDisplayImmediately();
    });
}

void ThreadedCompositor::renderNextFrame()
{
    ASSERT(isMainThread());
    m_client.renderNextFrame();
}

void ThreadedCompositor::commitScrollOffset(uint32_t layerID, const IntSize& offset)
{
    ASSERT(isMainThread());
    m_client.commitScrollOffset(layerID, offset);
}

void ThreadedCompositor::updateViewport()
{
    m_compositingRunLoop->scheduleUpdate();
}

void ThreadedCompositor::scheduleDisplayImmediately()
{
    m_compositingRunLoop->scheduleUpdate();
}

void ThreadedCompositor::forceRepaint()
{
    m_compositingRunLoop->performTaskSync([this, protectedThis = makeRef(*this)] {
        renderLayerTree();
    });
}

void ThreadedCompositor::renderLayerTree()
{
    if (!m_scene)
        return;
#if PLATFORM(GTK)
    if (!m_scene->isActive())
        return;
#endif

#if PLATFORM(WPE)
    if (!m_context)
        createGLContext();
#endif

    if (!m_context || !m_context->makeContextCurrent())
        return;

#if PLATFORM(WPE)
    m_target->frameWillRender();
#endif

    if (m_needsResize) {
        glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
        m_needsResize = false;
    }
    FloatRect clipRect(0, 0, m_viewportSize.width(), m_viewportSize.height());

    TransformationMatrix viewportTransform;
    viewportTransform.scale(m_scaleFactor);
    viewportTransform.translate(-m_scrollPosition.x(), -m_scrollPosition.y());

    if (!m_drawsBackground) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    m_scene->paintToCurrentGLContext(viewportTransform, 1, clipRect, Color::transparent, !m_drawsBackground, m_scrollPosition, m_paintFlags);

    m_context->swapBuffers();

#if PLATFORM(WPE)
    m_target->frameRendered();
#endif
}

void ThreadedCompositor::updateSceneState(const CoordinatedGraphicsState& state)
{
    ASSERT(isMainThread());
    RefPtr<ThreadedCompositor> protector(this);
    RefPtr<CoordinatedGraphicsScene> scene = m_scene;
    m_scene->appendUpdate([protector, scene, state] {
        scene->commitSceneState(state);

        protector->m_clientRendersNextFrame.store(true);
        bool coordinateUpdate = std::any_of(state.layersToUpdate.begin(), state.layersToUpdate.end(),
            [](const std::pair<CoordinatedLayerID, CoordinatedGraphicsLayerState>& it) {
                return it.second.platformLayerChanged || it.second.platformLayerUpdated;
            });
        protector->m_coordinateUpdateCompletionWithClient.store(coordinateUpdate);
    });

    scheduleDisplayImmediately();
}

#if PLATFORM(WPE)
static void debugThreadedCompositorFPS()
{
    static double lastTime = currentTime();
    static unsigned frameCount = 0;

    double ct = currentTime();
    frameCount++;

    if (ct - lastTime >= 5.0) {
        fprintf(stderr, "ThreadedCompositor: frame callbacks %.2f FPS\n", frameCount / (ct - lastTime));
        lastTime = ct;
        frameCount = 0;
    }
}

void ThreadedCompositor::frameComplete()
{
    ASSERT(m_compositingRunLoop->isCurrent());
    static bool reportFPS = !!std::getenv("WPE_THREADED_COMPOSITOR_FPS");
    if (reportFPS)
        debugThreadedCompositorFPS();

    bool shouldDispatchDisplayRefreshCallback = m_clientRendersNextFrame.load()
        || m_displayRefreshMonitor->requiresDisplayRefreshCallback();
    bool shouldCoordinateUpdateCompletionWithClient = m_coordinateUpdateCompletionWithClient.load();

    if (shouldDispatchDisplayRefreshCallback)
        m_displayRefreshMonitor->dispatchDisplayRefreshCallback();
    if (!shouldCoordinateUpdateCompletionWithClient)
        m_compositingRunLoop->updateCompleted();
}
#endif

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
RefPtr<WebCore::DisplayRefreshMonitor> ThreadedCompositor::createDisplayRefreshMonitor(PlatformDisplayID)
{
    return m_displayRefreshMonitor;
}
#endif

}
#endif // USE(COORDINATED_GRAPHICS_THREADED)
