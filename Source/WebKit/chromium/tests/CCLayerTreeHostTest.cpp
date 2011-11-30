/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "cc/CCLayerTreeHost.h"

#include "ContentLayerChromium.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCLayerTreeHostImpl.h"
#include "cc/CCScopedThreadProxy.h"
#include "cc/CCTextureUpdater.h"
#include "cc/CCThreadTask.h"
#include "GraphicsContext3DPrivate.h"
#include <gtest/gtest.h>
#include "LayerChromium.h"
#include "MockWebGraphicsContext3D.h"
#include "TextureManager.h"
#include "WebCompositor.h"
#include "WebKit.h"
#include "WebKitPlatformSupport.h"
#include "WebThread.h"
#include <webkit/support/webkit_support.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

using namespace WebCore;
using namespace WebKit;
using namespace WTF;

namespace {

// Used by test stubs to notify the test when something interesting happens.
class TestHooks {
public:
    virtual void beginCommitOnCCThread(CCLayerTreeHostImpl*) { }
    virtual void commitCompleteOnCCThread(CCLayerTreeHostImpl*) { }
    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl*) { }
    virtual void applyScrollAndScale(const IntSize&, float) { }
    virtual void animateAndLayout(double frameBeginTime) { }
};

// Adapts CCLayerTreeHostImpl for test. Runs real code, then invokes test hooks.
class MockLayerTreeHostImpl : public CCLayerTreeHostImpl {
public:
    static PassOwnPtr<MockLayerTreeHostImpl> create(TestHooks* testHooks, const CCSettings& settings, CCLayerTreeHostImplClient* client)
    {
        return adoptPtr(new MockLayerTreeHostImpl(testHooks, settings, client));
    }

    virtual void beginCommit()
    {
        CCLayerTreeHostImpl::beginCommit();
        m_testHooks->beginCommitOnCCThread(this);
    }

    virtual void commitComplete()
    {
        CCLayerTreeHostImpl::commitComplete();
        m_testHooks->commitCompleteOnCCThread(this);
    }

    virtual void drawLayers()
    {
        CCLayerTreeHostImpl::drawLayers();
        m_testHooks->drawLayersOnCCThread(this);
    }

private:
    MockLayerTreeHostImpl(TestHooks* testHooks, const CCSettings& settings, CCLayerTreeHostImplClient* client)
        : CCLayerTreeHostImpl(settings, client)
        , m_testHooks(testHooks)
    {
    }

    TestHooks* m_testHooks;
};

// Adapts CCLayerTreeHost for test. Injects MockLayerTreeHostImpl.
class MockLayerTreeHost : public CCLayerTreeHost {
public:
    static PassRefPtr<MockLayerTreeHost> create(TestHooks* testHooks, CCLayerTreeHostClient* client, PassRefPtr<LayerChromium> rootLayer, const CCSettings& settings)
    {
        return adoptRef(new MockLayerTreeHost(testHooks, client, rootLayer, settings));
    }

    virtual PassOwnPtr<CCLayerTreeHostImpl> createLayerTreeHostImpl(CCLayerTreeHostImplClient* client)
    {
        return MockLayerTreeHostImpl::create(m_testHooks, settings(), client);
    }

private:
    MockLayerTreeHost(TestHooks* testHooks, CCLayerTreeHostClient* client, PassRefPtr<LayerChromium> rootLayer, const CCSettings& settings)
        : CCLayerTreeHost(client, settings)
        , m_testHooks(testHooks)
    {
        setRootLayer(rootLayer);
        bool success = initialize();
        EXPECT_TRUE(success);
    }

    TestHooks* m_testHooks;
};

// Test stub for WebGraphicsContext3D. Returns canned values needed for compositor initialization.
class CompositorMockWebGraphicsContext3D : public MockWebGraphicsContext3D {
public:
    static PassOwnPtr<CompositorMockWebGraphicsContext3D> create()
    {
        return adoptPtr(new CompositorMockWebGraphicsContext3D());
    }

    virtual bool makeContextCurrent() { return true; }
    virtual WebGLId createProgram() { return 1; }
    virtual WebGLId createShader(WGC3Denum) { return 1; }
    virtual void getShaderiv(WebGLId, WGC3Denum, WGC3Dint* value) { *value = 1; }
    virtual void getProgramiv(WebGLId, WGC3Denum, WGC3Dint* value) { *value = 1; }

private:
    CompositorMockWebGraphicsContext3D() { }
};

// Implementation of CCLayerTreeHost callback interface.
class MockLayerTreeHostClient : public CCLayerTreeHostClient {
public:
    static PassOwnPtr<MockLayerTreeHostClient> create(TestHooks* testHooks)
    {
        return adoptPtr(new MockLayerTreeHostClient(testHooks));
    }

    virtual void animateAndLayout(double frameBeginTime)
    {
        m_testHooks->animateAndLayout(frameBeginTime);
    }

    virtual void applyScrollAndScale(const IntSize& scrollDelta, float scale)
    {
        m_testHooks->applyScrollAndScale(scrollDelta, scale);
    }

    virtual PassRefPtr<GraphicsContext3D> createLayerTreeHostContext3D()
    {
        OwnPtr<WebGraphicsContext3D> mock = CompositorMockWebGraphicsContext3D::create();
        GraphicsContext3D::Attributes attrs;
        RefPtr<GraphicsContext3D> context = GraphicsContext3DPrivate::createGraphicsContextFromWebContext(mock.release(), attrs, 0, GraphicsContext3D::RenderDirectlyToHostWindow, GraphicsContext3DPrivate::ForUseOnAnotherThread);
        return context;
    }

    virtual void didCommitAndDrawFrame()
    {
    }

    virtual void didCompleteSwapBuffers()
    {
    }

    virtual void didRecreateGraphicsContext(bool)
    {
    }

    virtual void scheduleComposite() { }

private:
    explicit MockLayerTreeHostClient(TestHooks* testHooks) : m_testHooks(testHooks) { }

    TestHooks* m_testHooks;
};

// The CCLayerTreeHostTest runs with the main loop running. It instantiates a single MockLayerTreeHost and associated
// MockLayerTreeHostImpl/MockLayerTreeHostClient.
//
// beginTest() is called once the main message loop is running and the layer tree host is initialized.
//
// Key stages of the drawing loop, e.g. drawing or commiting, redirect to CCLayerTreeHostTest methods of similar names.
// To track the commit process, override these functions.
//
// The test continues until someone calls endTest. endTest can be called on any thread, but be aware that
// ending the test is an asynchronous process.
class CCLayerTreeHostTest : public testing::Test, TestHooks {
public:
    virtual void afterTest() = 0;
    virtual void beginTest() = 0;

    void endTest();

    void postSetNeedsAnimateToMainThread()
    {
        callOnMainThread(CCLayerTreeHostTest::dispatchSetNeedsAnimate, this);
    }

    void postSetNeedsCommitToMainThread()
    {
        callOnMainThread(CCLayerTreeHostTest::dispatchSetNeedsCommit, this);
    }

    void postSetNeedsRedrawToMainThread()
    {
        callOnMainThread(CCLayerTreeHostTest::dispatchSetNeedsRedraw, this);
    }

    void postSetNeedsAnimateAndCommitToMainThread()
    {
        callOnMainThread(CCLayerTreeHostTest::dispatchSetNeedsAnimateAndCommit, this);
    }


    void postSetVisibleToMainThread(bool visible)
    {
        callOnMainThread(visible ? CCLayerTreeHostTest::dispatchSetVisible : CCLayerTreeHostTest::dispatchSetInvisible, this);
    }

    void timeout()
    {
        m_timedOut = true;
        endTest();
    }

    void clearTimeout()
    {
        m_timeoutTask = 0;
    }

    CCLayerTreeHost* layerTreeHost() { return m_layerTreeHost.get(); }


protected:
    CCLayerTreeHostTest()
        : m_beginning(false)
        , m_endWhenBeginReturns(false)
        , m_timedOut(false)
    {
        m_webThread = adoptPtr(webKitPlatformSupport()->createThread("CCLayerTreeHostTest"));
        ASSERT(CCProxy::mainThread());

        WebCompositor::setThread(m_webThread.get());
        ASSERT(CCProxy::isMainThread());
        m_mainThreadProxy = CCScopedThreadProxy::create(CCProxy::mainThread());
    }

    void doBeginTest();

    static void onBeginTest(void* self)
    {
        static_cast<CCLayerTreeHostTest*>(self)->doBeginTest();
    }

    static void onEndTest(void* self)
    {
        ASSERT(isMainThread());
        webkit_support::QuitMessageLoop();
        webkit_support::RunAllPendingMessages();
        CCLayerTreeHostTest* test = static_cast<CCLayerTreeHostTest*>(self);
        ASSERT_TRUE(test);
        test->m_layerTreeHost.clear();
    }

    static void dispatchSetNeedsAnimate(void* self)
    {
      ASSERT(isMainThread());
      CCLayerTreeHostTest* test = static_cast<CCLayerTreeHostTest*>(self);
      ASSERT(test);
      if (test->m_layerTreeHost)
          test->m_layerTreeHost->setNeedsAnimate();
    }

    static void dispatchSetNeedsAnimateAndCommit(void* self)
    {
      ASSERT(isMainThread());
      CCLayerTreeHostTest* test = static_cast<CCLayerTreeHostTest*>(self);
      ASSERT(test);
      if (test->m_layerTreeHost) {
          test->m_layerTreeHost->setNeedsAnimate();
          test->m_layerTreeHost->setNeedsCommit();
      }
    }

    static void dispatchSetNeedsCommit(void* self)
    {
      ASSERT(isMainThread());
      CCLayerTreeHostTest* test = static_cast<CCLayerTreeHostTest*>(self);
      ASSERT_TRUE(test);
      if (test->m_layerTreeHost)
          test->m_layerTreeHost->setNeedsCommit();
    }

    static void dispatchSetNeedsRedraw(void* self)
    {
      ASSERT(isMainThread());
      CCLayerTreeHostTest* test = static_cast<CCLayerTreeHostTest*>(self);
      ASSERT_TRUE(test);
      if (test->m_layerTreeHost)
          test->m_layerTreeHost->setNeedsRedraw();
    }

    static void dispatchSetVisible(void* self)
    {
      ASSERT(isMainThread());
      CCLayerTreeHostTest* test = static_cast<CCLayerTreeHostTest*>(self);
      ASSERT(test);
      if (test->m_layerTreeHost)
          test->m_layerTreeHost->setVisible(true);
    }

    static void dispatchSetInvisible(void* self)
    {
      ASSERT(isMainThread());
      CCLayerTreeHostTest* test = static_cast<CCLayerTreeHostTest*>(self);
      ASSERT(test);
      if (test->m_layerTreeHost)
          test->m_layerTreeHost->setVisible(false);
    }

    class TimeoutTask : public webkit_support::TaskAdaptor {
    public:
        explicit TimeoutTask(CCLayerTreeHostTest* test)
            : m_test(test)
        {
        }

        void clearTest()
        {
            m_test = 0;
        }

        virtual ~TimeoutTask()
        {
            if (m_test)
                m_test->clearTimeout();
        }

        virtual void Run()
        {
            if (m_test)
                m_test->timeout();
        }

    private:
        CCLayerTreeHostTest* m_test;
    };

    virtual void runTest(bool threaded)
    {
        m_settings.enableCompositorThread = threaded;
        m_settings.refreshRate = 100.0;
        webkit_support::PostDelayedTask(CCLayerTreeHostTest::onBeginTest, static_cast<void*>(this), 0);
        m_timeoutTask = new TimeoutTask(this);
        webkit_support::PostDelayedTask(m_timeoutTask, 5000); // webkit_support takes ownership of the task
        webkit_support::RunMessageLoop();
        webkit_support::RunAllPendingMessages();

        if (m_timeoutTask)
            m_timeoutTask->clearTest();

        ASSERT_FALSE(m_layerTreeHost.get());
        m_client.clear();
        if (m_timedOut) {
            FAIL() << "Test timed out";
            return;
        }
        m_rootLayer->setLayerTreeHost(0);
        afterTest();
    }

    CCSettings m_settings;
    OwnPtr<MockLayerTreeHostClient> m_client;
    RefPtr<CCLayerTreeHost> m_layerTreeHost;

private:
    bool m_beginning;
    bool m_endWhenBeginReturns;
    bool m_timedOut;

    RefPtr<LayerChromium> m_rootLayer;
    OwnPtr<WebThread> m_webThread;
    RefPtr<CCScopedThreadProxy> m_mainThreadProxy;
    TimeoutTask* m_timeoutTask;
};

void CCLayerTreeHostTest::doBeginTest()
{
    ASSERT(isMainThread());
    m_client = MockLayerTreeHostClient::create(this);

    m_rootLayer = LayerChromium::create(0);
    m_layerTreeHost = MockLayerTreeHost::create(this, m_client.get(), m_rootLayer, m_settings);
    ASSERT_TRUE(m_layerTreeHost);
    m_rootLayer->setLayerTreeHost(m_layerTreeHost.get());

    m_beginning = true;
    beginTest();
    m_beginning = false;
    if (m_endWhenBeginReturns)
        onEndTest(static_cast<void*>(this));
}

void CCLayerTreeHostTest::endTest()
{
    // If we are called from the CCThread, re-call endTest on the main thread.
    if (!isMainThread())
        m_mainThreadProxy->postTask(createCCThreadTask(this, &CCLayerTreeHostTest::endTest));
    else {
        // For the case where we endTest during beginTest(), set a flag to indicate that
        // the test should end the second beginTest regains control.
        if (m_beginning)
            m_endWhenBeginReturns = true;
        else
            onEndTest(static_cast<void*>(this));
    }
}

class CCLayerTreeHostTestThreadOnly : public CCLayerTreeHostTest {
public:
    void runTestThreaded()
    {
        CCLayerTreeHostTest::runTest(true);
    }
};

// Shortlived layerTreeHosts shouldn't die.
class CCLayerTreeHostTestShortlived1 : public CCLayerTreeHostTest {
public:
    CCLayerTreeHostTestShortlived1() { }

    virtual void beginTest()
    {
        endTest();
    }

    virtual void afterTest()
    {
    }
};

#define SINGLE_AND_MULTI_THREAD_TEST_F(TEST_FIXTURE_NAME) \
    TEST_F(TEST_FIXTURE_NAME, runSingleThread)            \
    {                                                     \
        runTest(false);                                   \
    }                                                     \
    TEST_F(TEST_FIXTURE_NAME, runMultiThread)             \
    {                                                     \
        runTest(true);                                    \
    }

SINGLE_AND_MULTI_THREAD_TEST_F(CCLayerTreeHostTestShortlived1)

// Shortlived layerTreeHosts shouldn't die with a commit in flight.
class CCLayerTreeHostTestShortlived2 : public CCLayerTreeHostTest {
public:
    CCLayerTreeHostTestShortlived2() { }

    virtual void beginTest()
    {
        postSetNeedsCommitToMainThread();
        endTest();
    }

    virtual void afterTest()
    {
    }
};

SINGLE_AND_MULTI_THREAD_TEST_F(CCLayerTreeHostTestShortlived2)

// Shortlived layerTreeHosts shouldn't die with a redraw in flight.
class CCLayerTreeHostTestShortlived3 : public CCLayerTreeHostTest {
public:
    CCLayerTreeHostTestShortlived3() { }

    virtual void beginTest()
    {
        postSetNeedsRedrawToMainThread();
        endTest();
    }

    virtual void afterTest()
    {
    }
};

SINGLE_AND_MULTI_THREAD_TEST_F(CCLayerTreeHostTestShortlived3)

// Constantly redrawing layerTreeHosts shouldn't die when they commit
class CCLayerTreeHostTestCommitingWithContinuousRedraw : public CCLayerTreeHostTest {
public:
    CCLayerTreeHostTestCommitingWithContinuousRedraw()
        : m_numCompleteCommits(0)
        , m_numDraws(0)
    {
    }

    virtual void beginTest()
    {
        postSetNeedsCommitToMainThread();
        endTest();
    }

    virtual void commitCompleteOnCCThread(CCLayerTreeHostImpl*)
    {
        m_numCompleteCommits++;
        if (m_numCompleteCommits == 2)
            endTest();
    }

    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl*)
    {
        if (m_numDraws == 1)
          postSetNeedsCommitToMainThread();
        m_numDraws++;
        postSetNeedsRedrawToMainThread();
    }

    virtual void afterTest()
    {
    }

private:
    int m_numCompleteCommits;
    int m_numDraws;
};

SINGLE_AND_MULTI_THREAD_TEST_F(CCLayerTreeHostTestCommitingWithContinuousRedraw)

// Two setNeedsCommits in a row should lead to at least 1 commit and at least 1
// draw with frame 0.
class CCLayerTreeHostTestSetNeedsCommit1 : public CCLayerTreeHostTestThreadOnly {
public:
    CCLayerTreeHostTestSetNeedsCommit1()
        : m_numCommits(0)
        , m_numDraws(0)
    {
    }

    virtual void beginTest()
    {
        postSetNeedsCommitToMainThread();
        postSetNeedsCommitToMainThread();
    }

    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl* impl)
    {
        m_numDraws++;
        if (!impl->sourceFrameNumber())
            endTest();
    }

    virtual void commitCompleteOnCCThread(CCLayerTreeHostImpl*)
    {
        m_numCommits++;
    }

    virtual void afterTest()
    {
        EXPECT_GE(1, m_numCommits);
        EXPECT_GE(1, m_numDraws);
    }

private:
    int m_numCommits;
    int m_numDraws;
};

TEST_F(CCLayerTreeHostTestSetNeedsCommit1, runMultiThread)
{
    runTestThreaded();
}

// A setNeedsCommit should lead to 1 commit. Issuing a second commit after that
// first committed frame draws should lead to another commit.
class CCLayerTreeHostTestSetNeedsCommit2 : public CCLayerTreeHostTestThreadOnly {
public:
    CCLayerTreeHostTestSetNeedsCommit2()
        : m_numCommits(0)
        , m_numDraws(0)
    {
    }

    virtual void beginTest()
    {
        postSetNeedsCommitToMainThread();
    }

    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl* impl)
    {
        if (!impl->sourceFrameNumber())
            postSetNeedsCommitToMainThread();
        else if (impl->sourceFrameNumber() == 1)
            endTest();
    }

    virtual void commitCompleteOnCCThread(CCLayerTreeHostImpl*)
    {
        m_numCommits++;
    }

    virtual void afterTest()
    {
        EXPECT_EQ(2, m_numCommits);
        EXPECT_GE(2, m_numDraws);
    }

private:
    int m_numCommits;
    int m_numDraws;
};

TEST_F(CCLayerTreeHostTestSetNeedsCommit2, runMultiThread)
{
    runTestThreaded();
}

// 1 setNeedsRedraw after the first commit has completed should lead to 1
// additional draw.
class CCLayerTreeHostTestSetNeedsRedraw : public CCLayerTreeHostTestThreadOnly {
public:
    CCLayerTreeHostTestSetNeedsRedraw()
        : m_numCommits(0)
        , m_numDraws(0)
    {
    }

    virtual void beginTest()
    {
        postSetNeedsCommitToMainThread();
    }

    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl* impl)
    {
        EXPECT_EQ(0, impl->sourceFrameNumber());
        if (!m_numDraws)
            postSetNeedsRedrawToMainThread(); // Redraw again to verify that the second redraw doesn't commit.
        else
            endTest();
        m_numDraws++;
    }

    virtual void commitCompleteOnCCThread(CCLayerTreeHostImpl*)
    {
        EXPECT_EQ(0, m_numDraws);
        m_numCommits++;
    }

    virtual void afterTest()
    {
        EXPECT_GE(2, m_numDraws);
        EXPECT_EQ(1, m_numCommits);
    }

private:
    int m_numCommits;
    int m_numDraws;
};

TEST_F(CCLayerTreeHostTestSetNeedsRedraw, runMultiThread)
{
    runTestThreaded();
}

// Trigger a frame with setNeedsCommit. Then, inside the resulting animate
// callback, requet another frame using setNeedsAnimate. End the test when
// animate gets called yet-again, indicating that the proxy is correctly
// handling the case where setNeedsAnimate() is called inside the begin frame
// flow.
class CCLayerTreeHostTestSetNeedsAnimateInsideAnimationCallback : public CCLayerTreeHostTestThreadOnly {
public:
    CCLayerTreeHostTestSetNeedsAnimateInsideAnimationCallback()
        : m_numAnimates(0)
    {
    }

    virtual void beginTest()
    {
        postSetNeedsAnimateToMainThread();
    }

    virtual void animateAndLayout(double)
    {
        if (!m_numAnimates) {
            m_layerTreeHost->setNeedsAnimate();
            m_numAnimates++;
            return;
        }
        endTest();
    }

    virtual void afterTest()
    {
    }

private:
    int m_numAnimates;
};

TEST_F(CCLayerTreeHostTestSetNeedsAnimateInsideAnimationCallback, runMultiThread)
{
    runTestThreaded();
}

class CCLayerTreeHostTestScrollSimple : public CCLayerTreeHostTestThreadOnly {
public:
    CCLayerTreeHostTestScrollSimple()
        : m_initialScroll(IntPoint(10, 20))
        , m_secondScroll(IntPoint(40, 5))
        , m_scrollAmount(2, -1)
        , m_scrolls(0)
    {
    }

    virtual void beginTest()
    {
        m_layerTreeHost->rootLayer()->setScrollable(true);
        m_layerTreeHost->rootLayer()->setScrollPosition(m_initialScroll);
        postSetNeedsCommitToMainThread();
    }

    virtual void beginCommitOnCCThread(CCLayerTreeHostImpl* impl)
    {
        LayerChromium* root = m_layerTreeHost->rootLayer();
        if (!m_layerTreeHost->frameNumber())
            EXPECT_EQ(root->scrollPosition(), m_initialScroll);
        else {
            EXPECT_EQ(root->scrollPosition(), m_initialScroll + m_scrollAmount);

            // Pretend like Javascript updated the scroll position itself.
            root->setScrollPosition(m_secondScroll);
        }
    }

    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl* impl)
    {
        CCLayerImpl* root = impl->rootLayer();
        EXPECT_EQ(root->scrollDelta(), IntSize());

        root->setScrollable(true);
        root->setMaxScrollPosition(IntSize(100, 100));
        root->scrollBy(m_scrollAmount);

        if (impl->frameNumber() == 1) {
            EXPECT_EQ(root->scrollPosition(), m_initialScroll);
            EXPECT_EQ(root->scrollDelta(), m_scrollAmount);
            postSetNeedsCommitToMainThread();
        } else if (impl->frameNumber() == 2) {
            EXPECT_EQ(root->scrollPosition(), m_secondScroll);
            EXPECT_EQ(root->scrollDelta(), m_scrollAmount);
            endTest();
        }
    }

    virtual void applyScrollAndScale(const IntSize& scrollDelta, float scale)
    {
        IntPoint position = m_layerTreeHost->rootLayer()->scrollPosition();
        m_layerTreeHost->rootLayer()->setScrollPosition(position + scrollDelta);
        m_scrolls++;
    }

    virtual void afterTest()
    {
        EXPECT_EQ(1, m_scrolls);
    }
private:
    IntPoint m_initialScroll;
    IntPoint m_secondScroll;
    IntSize m_scrollAmount;
    int m_scrolls;
};

TEST_F(CCLayerTreeHostTestScrollSimple, runMultiThread)
{
    runTestThreaded();
}

class CCLayerTreeHostTestScrollMultipleRedraw : public CCLayerTreeHostTestThreadOnly {
public:
    CCLayerTreeHostTestScrollMultipleRedraw()
        : m_initialScroll(IntPoint(40, 10))
        , m_scrollAmount(-3, 17)
        , m_scrolls(0)
    {
    }

    virtual void beginTest()
    {
        m_layerTreeHost->rootLayer()->setScrollable(true);
        m_layerTreeHost->rootLayer()->setScrollPosition(m_initialScroll);
        postSetNeedsCommitToMainThread();
    }

    virtual void beginCommitOnCCThread(CCLayerTreeHostImpl* impl)
    {
        LayerChromium* root = m_layerTreeHost->rootLayer();
        if (!m_layerTreeHost->frameNumber())
            EXPECT_EQ(root->scrollPosition(), m_initialScroll);
        else if (m_layerTreeHost->frameNumber() == 1)
            EXPECT_EQ(root->scrollPosition(), m_initialScroll + m_scrollAmount + m_scrollAmount);
        else if (m_layerTreeHost->frameNumber() == 2)
            EXPECT_EQ(root->scrollPosition(), m_initialScroll + m_scrollAmount + m_scrollAmount);
    }

    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl* impl)
    {
        CCLayerImpl* root = impl->rootLayer();
        root->setScrollable(true);
        root->setMaxScrollPosition(IntSize(100, 100));

        if (impl->frameNumber() == 1) {
            EXPECT_EQ(root->scrollDelta(), IntSize());
            root->scrollBy(m_scrollAmount);
            EXPECT_EQ(root->scrollDelta(), m_scrollAmount);

            EXPECT_EQ(root->scrollPosition(), m_initialScroll);
            postSetNeedsRedrawToMainThread();
        } else if (impl->frameNumber() == 2) {
            EXPECT_EQ(root->scrollDelta(), m_scrollAmount);
            root->scrollBy(m_scrollAmount);
            EXPECT_EQ(root->scrollDelta(), m_scrollAmount + m_scrollAmount);

            EXPECT_EQ(root->scrollPosition(), m_initialScroll);
            postSetNeedsCommitToMainThread();
        } else if (impl->frameNumber() == 3) {
            EXPECT_EQ(root->scrollDelta(), IntSize());
            EXPECT_EQ(root->scrollPosition(), m_initialScroll + m_scrollAmount + m_scrollAmount);
            endTest();
        }
    }

    virtual void applyScrollAndScale(const IntSize& scrollDelta, float scale)
    {
        IntPoint position = m_layerTreeHost->rootLayer()->scrollPosition();
        m_layerTreeHost->rootLayer()->setScrollPosition(position + scrollDelta);
        m_scrolls++;
    }

    virtual void afterTest()
    {
        EXPECT_EQ(1, m_scrolls);
    }
private:
    IntPoint m_initialScroll;
    IntSize m_scrollAmount;
    int m_scrolls;
};

TEST_F(CCLayerTreeHostTestScrollMultipleRedraw, runMultiThread)
{
    runTestThreaded();
}

class CCLayerTreeHostTestSetVisible : public CCLayerTreeHostTest {
public:

    CCLayerTreeHostTestSetVisible()
        : m_numCommits(0)
        , m_numDraws(0)
    {
    }

    virtual void beginTest()
    {
        postSetVisibleToMainThread(false);
        postSetNeedsRedrawToMainThread(); // This is suppressed while we're invisible.
        postSetVisibleToMainThread(true); // Triggers the redraw.
    }

    virtual void drawLayersOnCCThread(CCLayerTreeHostImpl* impl)
    {
        EXPECT_TRUE(impl->visible());
        ++m_numDraws;
        endTest();
    }

    virtual void afterTest()
    {
        EXPECT_EQ(1, m_numDraws);
    }

private:
    int m_numCommits;
    int m_numDraws;
};

TEST_F(CCLayerTreeHostTestSetVisible, runMultiThread)
{
    runTest(true);
}

class TestOpacityChangeLayerDelegate : public CCLayerDelegate {
public:
    TestOpacityChangeLayerDelegate(CCLayerTreeHostTest* test)
        : m_test(test)
    {
    }

    virtual void paintContents(GraphicsContext&, const IntRect&)
    {
        // Set layer opacity to 0.
        m_test->layerTreeHost()->rootLayer()->setOpacity(0);
    }

    virtual bool drawsContent() const { return true; }
    virtual bool preserves3D() { return false; }
    virtual void notifySyncRequired() { }

private:
    CCLayerTreeHostTest* m_test;
};

class ContentLayerChromiumWithUpdateTracking : public ContentLayerChromium {
public:
    static PassRefPtr<ContentLayerChromiumWithUpdateTracking> create(CCLayerDelegate *delegate) { return adoptRef(new ContentLayerChromiumWithUpdateTracking(delegate)); }

    int paintContentsCount() { return m_paintContentsCount; }
    void resetPaintContentsCount() { m_paintContentsCount = 0; }

    int updateCount() { return m_updateCount; }
    void resetUpdateCount() { m_updateCount = 0; }

    virtual void paintContentsIfDirty()
    {
        ContentLayerChromium::paintContentsIfDirty();
        m_paintContentsCount++;
    }

    virtual void updateCompositorResources(GraphicsContext3D* context, CCTextureUpdater& updater)
    {
        ContentLayerChromium::updateCompositorResources(context, updater);
        m_updateCount++;
    }

private:
    explicit ContentLayerChromiumWithUpdateTracking(CCLayerDelegate *delegate)
        : ContentLayerChromium(delegate)
        , m_paintContentsCount(0)
        , m_updateCount(0)
    {
        setBounds(IntSize(10, 10));
    }

    int m_paintContentsCount;
    int m_updateCount;
};

// Layer opacity change during paint should not prevent compositor resources from being updated during commit.
class CCLayerTreeHostTestOpacityChange : public CCLayerTreeHostTest {
public:
    CCLayerTreeHostTestOpacityChange()
        : m_testOpacityChangeDelegate(this)
        , m_updateCheckLayer(ContentLayerChromiumWithUpdateTracking::create(&m_testOpacityChangeDelegate))
    {
    }

    virtual void beginTest()
    {
        m_layerTreeHost->setRootLayer(m_updateCheckLayer);
        m_layerTreeHost->setViewport(IntSize(10, 10));

        postSetNeedsCommitToMainThread();
    }

    virtual void commitCompleteOnCCThread(CCLayerTreeHostImpl*)
    {
        endTest();
    }

    virtual void afterTest()
    {
        // paintContentsIfDirty() should have been called once.
        EXPECT_EQ(1, m_updateCheckLayer->paintContentsCount());

        // updateCompositorResources() should have been called the same
        // amout of times as paintContentsIfDirty().
        EXPECT_EQ(m_updateCheckLayer->paintContentsCount(),
                  m_updateCheckLayer->updateCount());
    }

private:
    TestOpacityChangeLayerDelegate m_testOpacityChangeDelegate;
    RefPtr<ContentLayerChromiumWithUpdateTracking> m_updateCheckLayer;
};

TEST_F(CCLayerTreeHostTestOpacityChange, runMultiThread)
{
    runTest(true);
}

} // namespace
