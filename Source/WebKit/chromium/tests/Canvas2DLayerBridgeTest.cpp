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

#include "Canvas2DLayerBridge.h"

#include "FakeWebCompositorOutputSurface.h"
#include "FakeWebGraphicsContext3D.h"
#include "GraphicsContext3DPrivate.h"
#include "ImageBuffer.h"
#include "WebCompositorInitializer.h"
#include <public/Platform.h>
#include <public/WebThread.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wtf/RefPtr.h>

using namespace WebCore;
using namespace WebKit;
using testing::InSequence;
using testing::Return;
using testing::Test;

namespace {

class MockCanvasContext : public FakeWebGraphicsContext3D {
public:
    MOCK_METHOD0(flush, void(void));
    MOCK_METHOD0(createTexture, unsigned(void));
    MOCK_METHOD1(deleteTexture, void(unsigned));

    virtual GrGLInterface* onCreateGrGLInterface() OVERRIDE { return 0; }
};

class MockWebTextureUpdater : public WebTextureUpdater {
public:
    MOCK_METHOD3(appendCopy, void(unsigned, unsigned, WebSize));
};

} // namespace

enum ThreadMode {
    SingleThreaded, Threaded
};

class Canvas2DLayerBridgeTest : public Test {
protected:
    void fullLifecycleTest(ThreadMode threadMode, DeferralMode deferralMode)
    {
        RefPtr<GraphicsContext3D> mainContext = GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(new MockCanvasContext));

        MockCanvasContext& mainMock = *static_cast<MockCanvasContext*>(GraphicsContext3DPrivate::extractWebGraphicsContext3D(mainContext.get()));

        MockWebTextureUpdater updater;

        const IntSize size(300, 150);

        OwnPtr<WebThread> thread;
        if (threadMode == Threaded)
            thread = adoptPtr(WebKit::Platform::current()->createThread("Canvas2DLayerBridgeTest"));
        WebKitTests::WebCompositorInitializer initializer(thread.get());

        WebGLId backTextureId = 1;
        WebGLId frontTextureId = 1;

        // Threaded and non deferred canvases are double buffered.
        if (threadMode == Threaded && deferralMode == NonDeferred) {
            frontTextureId = 2;
            // Create texture (on the main thread) and do the copy (on the impl thread).
            EXPECT_CALL(mainMock, createTexture()).WillOnce(Return(frontTextureId));
        }

        OwnPtr<Canvas2DLayerBridge> bridge = Canvas2DLayerBridge::create(mainContext.get(), size, deferralMode, backTextureId);

        ::testing::Mock::VerifyAndClearExpectations(&mainMock);

        EXPECT_CALL(mainMock, flush());
        if (threadMode == Threaded && deferralMode == NonDeferred)
            EXPECT_CALL(updater, appendCopy(backTextureId, frontTextureId, WebSize(300, 150)));
        EXPECT_EQ(frontTextureId, bridge->prepareTexture(updater));
        ::testing::Mock::VerifyAndClearExpectations(&mainMock);
        ::testing::Mock::VerifyAndClearExpectations(&updater);

        if (threadMode == Threaded && deferralMode == NonDeferred) {
            EXPECT_CALL(mainMock, deleteTexture(frontTextureId));
            EXPECT_CALL(mainMock, flush());
        }
        bridge.clear();
    }
};

namespace {

TEST_F(Canvas2DLayerBridgeTest, testFullLifecycleSingleThreadedDeferred)
{
    fullLifecycleTest(SingleThreaded, NonDeferred);
}

TEST_F(Canvas2DLayerBridgeTest, testFullLifecycleSingleThreadedNonDeferred)
{
    fullLifecycleTest(SingleThreaded, Deferred);
}

TEST_F(Canvas2DLayerBridgeTest, testFullLifecycleThreadedNonDeferred)
{
    fullLifecycleTest(Threaded, NonDeferred);
}

TEST_F(Canvas2DLayerBridgeTest, testFullLifecycleThreadedDeferred)
{
    fullLifecycleTest(Threaded, Deferred);
}

} // namespace
