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

#ifndef ThreadedCompositor_h
#define ThreadedCompositor_h

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "CoordinatedGraphicsScene.h"
#include "SimpleViewportController.h"
#include <WebCore/IntSize.h>
#include <WebCore/TransformationMatrix.h>
#include <wtf/Atomics.h>
#include <wtf/Condition.h>
#include <wtf/FastMalloc.h>
#include <wtf/Lock.h>
#include <wtf/Noncopyable.h>
#include <wtf/ThreadSafeRefCounted.h>

#if PLATFORM(WPE)
#include "CompositingManager.h"
#endif

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
#include <WebCore/DisplayRefreshMonitor.h>
#endif

#if PLATFORM(WPE)
#include "CompositingManager.h"
#include <WebCore/PlatformDisplayWPE.h>
#endif

namespace WebCore {
struct CoordinatedGraphicsState;
class GLContext;
}

namespace WebKit {

class CompositingRunLoop;
class CoordinatedGraphicsScene;
class CoordinatedGraphicsSceneClient;
class DisplayRefreshMonitor;
class WebPage;

class ThreadedCompositor : public ThreadSafeRefCounted<ThreadedCompositor>, public SimpleViewportController::Client, public CoordinatedGraphicsSceneClient
#if PLATFORM(WPE)
    , public WebCore::PlatformDisplayWPE::EGLTarget::Client
#endif
    {
    WTF_MAKE_NONCOPYABLE(ThreadedCompositor);
    WTF_MAKE_FAST_ALLOCATED;
public:
    class Client {
    public:
        virtual void setVisibleContentsRect(const WebCore::FloatRect&, const WebCore::FloatPoint&, float) = 0;
        virtual void purgeBackingStores() = 0;
        virtual void renderNextFrame() = 0;
        virtual void commitScrollOffset(uint32_t layerID, const WebCore::IntSize& offset) = 0;
    };

    static Ref<ThreadedCompositor> create(Client*, WebPage&);
    virtual ~ThreadedCompositor();

    void setNeedsDisplay();

    void setNativeSurfaceHandleForCompositing(uint64_t);
    void setDeviceScaleFactor(float);

    void updateSceneState(const WebCore::CoordinatedGraphicsState&);

    void didChangeViewportSize(const WebCore::IntSize&);
    void didChangeViewportAttribute(const WebCore::ViewportAttributes&);
    void didChangeContentsSize(const WebCore::IntSize&);
    void scrollTo(const WebCore::IntPoint&);
    void scrollBy(const WebCore::IntSize&);

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    RefPtr<WebCore::DisplayRefreshMonitor> createDisplayRefreshMonitor(WebCore::PlatformDisplayID);
#endif

private:
    ThreadedCompositor(Client*, WebPage&);

    // CoordinatedGraphicsSceneClient
    void purgeBackingStores() override;
    void renderNextFrame() override;
    void updateViewport() override;
    void commitScrollOffset(uint32_t layerID, const WebCore::IntSize& offset) override;

#if PLATFORM(WPE)
    // WebCore::PlatformDisplayWPE::Surface::Client
    virtual void frameComplete() override;
#endif

    void renderLayerTree();
    void scheduleDisplayImmediately();
    void didChangeVisibleRect() override;

    bool ensureGLContext();
    WebCore::GLContext* glContext();
    SimpleViewportController* viewportController() { return m_viewportController.get(); }

    void callOnCompositingThread(std::function<void()>&&);
    void createCompositingThread();
    void runCompositingThread();
    void terminateCompositingThread();
    static void compositingThreadEntry(void*);

    Client* m_client;
    RefPtr<CoordinatedGraphicsScene> m_scene;
    std::unique_ptr<SimpleViewportController> m_viewportController;

#if PLATFORM(WPE)
    CompositingManager m_compositingManager;
    std::unique_ptr<WebCore::PlatformDisplayWPE::EGLTarget> m_target;
#endif
    std::unique_ptr<WebCore::GLContext> m_context;

    WebCore::IntSize m_viewportSize;
    float m_deviceScaleFactor;
    uint64_t m_nativeSurfaceHandle;

    ThreadIdentifier m_threadIdentifier;
    Condition m_initializeRunLoopCondition;
    Lock m_initializeRunLoopConditionLock;
    Condition m_terminateRunLoopCondition;
    Lock m_terminateRunLoopConditionLock;

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    friend class DisplayRefreshMonitor;
    RefPtr<DisplayRefreshMonitor> m_displayRefreshMonitor;
#endif

    std::unique_ptr<CompositingRunLoop> m_compositingRunLoop;

    Atomic<bool> m_clientRendersNextFrame;
    Atomic<bool> m_coordinateUpdateCompletionWithClient;
};

} // namespace WebKit

#endif

#endif // ThreadedCompositor_h
