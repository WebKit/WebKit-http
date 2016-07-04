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
#include "DisplayRefreshMonitor.h"
#include <WebCore/GLContextEGL.h>
#include <WebCore/TransformationMatrix.h>
#include <cstdio>
#include <cstdlib>
#include <wtf/RunLoop.h>
#include <wtf/StdLibExtras.h>

#if PLATFORM(WPE)
#include <WebCore/PlatformDisplayWPE.h>
#endif

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif

using namespace WebCore;

namespace WebKit {

Ref<ThreadedCompositor> ThreadedCompositor::create(Client* client, WebPage& webPage)
{
    return adoptRef(*new ThreadedCompositor(client, webPage));
}

ThreadedCompositor::ThreadedCompositor(Client* client, WebPage& webPage)
    : m_client(client)
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    , m_displayRefreshMonitor(adoptRef(new WebKit::DisplayRefreshMonitor(*this)))
#endif
{
    m_clientRendersNextFrame.store(false);
    m_coordinateUpdateCompletionWithClient.store(false);

    m_compositingManager.establishConnection(webPage);
    createCompositingThread();
}

ThreadedCompositor::~ThreadedCompositor()
{
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    m_displayRefreshMonitor->invalidate();
#endif
    terminateCompositingThread();
}

void ThreadedCompositor::setNativeSurfaceHandleForCompositing(uint64_t handle)
{
    m_compositingRunLoop->stopUpdates();
    m_compositingRunLoop->performTaskSync([this, protectedThis = Ref<ThreadedCompositor>(*this), handle] {
        m_scene->setActive(!!handle);

        // A new native handle can't be set without destroying the previous one first if any.
        ASSERT(!!handle ^ !!m_nativeSurfaceHandle);
        m_nativeSurfaceHandle = handle;
        if (!m_nativeSurfaceHandle)
            m_context = nullptr;
    });
}

void ThreadedCompositor::setDeviceScaleFactor(float scale)
{
    m_compositingRunLoop->performTask([this, protectedThis = Ref<ThreadedCompositor>(*this), scale] {
        m_deviceScaleFactor = scale;
        scheduleDisplayImmediately();
    });
}

void ThreadedCompositor::didChangeViewportSize(const IntSize& size)
{
    m_compositingRunLoop->performTaskSync([this, protectedThis = Ref<ThreadedCompositor>(*this), size] {
        m_viewportController->didChangeViewportSize(size);
    });
}

void ThreadedCompositor::didChangeViewportAttribute(const ViewportAttributes& attr)
{
    m_compositingRunLoop->performTask([this, protectedThis = Ref<ThreadedCompositor>(*this), attr] {
        m_viewportController->didChangeViewportAttribute(attr);
    });
}

void ThreadedCompositor::didChangeContentsSize(const IntSize& size)
{
    m_compositingRunLoop->performTask([this, protectedThis = Ref<ThreadedCompositor>(*this), size] {
#if PLATFORM(WPE)
        if (m_target)
            m_target->resize(size);
#endif
        m_viewportController->didChangeContentsSize(size);
    });
}

void ThreadedCompositor::scrollTo(const IntPoint& position)
{
    m_compositingRunLoop->performTask([this, protectedThis = Ref<ThreadedCompositor>(*this), position] {
        m_viewportController->scrollTo(position);
    });
}

void ThreadedCompositor::scrollBy(const IntSize& delta)
{
    m_compositingRunLoop->performTask([this, protectedThis = Ref<ThreadedCompositor>(*this), delta] {
        m_viewportController->scrollBy(delta);
    });
}

void ThreadedCompositor::purgeBackingStores()
{
    m_client->purgeBackingStores();
}

void ThreadedCompositor::renderNextFrame()
{
    m_client->renderNextFrame();
}

void ThreadedCompositor::updateViewport()
{
    m_compositingRunLoop->scheduleUpdate();
}

void ThreadedCompositor::commitScrollOffset(uint32_t layerID, const IntSize& offset)
{
    m_client->commitScrollOffset(layerID, offset);
}

bool ThreadedCompositor::tryEnsureGLContext()
{
    if (!glContext())
        return false;

    glContext()->makeContextCurrent();
    // The window size may be out of sync with the page size at this point, and getting
    // the viewport parameters incorrect, means that the content will be misplaced. Thus
    // we set the viewport parameters directly from the window size.
    IntSize contextSize = glContext()->defaultFrameBufferSize();
    if (m_viewportSize != contextSize) {
        glViewport(0, 0, contextSize.width(), contextSize.height());
        m_viewportSize = contextSize;
    }

    return true;
}

GLContext* ThreadedCompositor::glContext()
{
    ASSERT(&RunLoop::current() == &m_compositingRunLoop->runLoop());

    if (m_context)
        return m_context.get();

#if PLATFORM(GTK)
    if (!m_nativeSurfaceHandle)
        return nullptr;
#endif
#if PLATFORM(WPE)
    ASSERT(m_target);
    m_target->initialize(IntSize(m_viewportController->visibleContentsRect().size()));
    m_context = m_target->createGLContext();
#endif

    return m_context.get();
}

void ThreadedCompositor::scheduleDisplayImmediately()
{
    m_compositingRunLoop->scheduleUpdate();
}

void ThreadedCompositor::didChangeVisibleRect()
{
    ASSERT(&RunLoop::current() == &m_compositingRunLoop->runLoop());

    RunLoop::main().dispatch([this, protectedThis = Ref<ThreadedCompositor>(*this), visibleRect = m_viewportController->visibleContentsRect(), scale = m_viewportController->pageScaleFactor()] {
        m_client->setVisibleContentsRect(visibleRect, FloatPoint::zero(), scale);
    });

    scheduleDisplayImmediately();
}

void ThreadedCompositor::renderLayerTree()
{
    ASSERT(&RunLoop::current() == &m_compositingRunLoop->runLoop());
    if (!m_scene)
        return;
#if PLATFORM(GTK)
    if (!m_scene->isActive())
        return;
#endif

    if (!tryEnsureGLContext())
        return;

    FloatRect clipRect(0, 0, m_viewportSize.width(), m_viewportSize.height());

    TransformationMatrix viewportTransform;
    FloatPoint scrollPostion = m_viewportController->visibleContentsRect().location();
    viewportTransform.scale(m_viewportController->pageScaleFactor() * m_deviceScaleFactor);
    viewportTransform.translate(-scrollPostion.x(), -scrollPostion.y());

    m_scene->paintToCurrentGLContext(viewportTransform, 1, clipRect, Color::white, false, scrollPostion);

    glContext()->swapBuffers();

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

void ThreadedCompositor::createCompositingThread()
{
    if (m_threadIdentifier)
        return;

    LockHolder locker(m_initializeRunLoopConditionLock);
    m_threadIdentifier = createThread("WebKit: ThreadedCompositor", [this] { runCompositingThread(); });
    m_initializeRunLoopCondition.wait(m_initializeRunLoopConditionLock);
}

void ThreadedCompositor::runCompositingThread()
{
    {
        LockHolder locker(m_initializeRunLoopConditionLock);

        m_compositingRunLoop = std::make_unique<CompositingRunLoop>([&] {
            renderLayerTree();
        });
        m_scene = adoptRef(new CoordinatedGraphicsScene(this));
        m_viewportController = std::make_unique<SimpleViewportController>(this);

#if PLATFORM(WPE)
        RELEASE_ASSERT(is<PlatformDisplayWPE>(PlatformDisplay::sharedDisplay()));
        m_target = downcast<PlatformDisplayWPE>(PlatformDisplay::sharedDisplay()).createEGLTarget(*this, m_compositingManager.releaseConnectionFd());
#endif

        m_initializeRunLoopCondition.notifyOne();
    }

    m_compositingRunLoop->run();

    m_compositingRunLoop->stopUpdates();
    m_scene->purgeGLResources();

    {
        LockHolder locker(m_terminateRunLoopConditionLock);
        m_context = nullptr;
#if PLATFORM(WPE)
        m_target = nullptr;
#endif
        m_scene = nullptr;
        m_viewportController = nullptr;
        m_compositingRunLoop = nullptr;
        m_terminateRunLoopCondition.notifyOne();
    }

    detachThread(m_threadIdentifier);
}

void ThreadedCompositor::terminateCompositingThread()
{
    LockHolder locker(m_terminateRunLoopConditionLock);

    m_scene->detach();
    m_compositingRunLoop->stop();

    m_terminateRunLoopCondition.wait(m_terminateRunLoopConditionLock);
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
    ASSERT(&RunLoop::current() == &m_compositingRunLoop->runLoop());
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
