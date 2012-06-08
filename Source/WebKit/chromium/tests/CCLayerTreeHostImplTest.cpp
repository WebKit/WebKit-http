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

#include "cc/CCLayerTreeHostImpl.h"

#include "CCAnimationTestCommon.h"
#include "CCLayerTestCommon.h"
#include "FakeWebGraphicsContext3D.h"
#include "GraphicsContext3DPrivate.h"
#include "LayerRendererChromium.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCLayerTilingData.h"
#include "cc/CCQuadCuller.h"
#include "cc/CCScrollbarLayerImpl.h"
#include "cc/CCSingleThreadProxy.h"
#include "cc/CCSolidColorDrawQuad.h"
#include "cc/CCTextureLayerImpl.h"
#include "cc/CCTileDrawQuad.h"
#include "cc/CCTiledLayerImpl.h"
#include "cc/CCVideoLayerImpl.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <public/WebVideoFrame.h>
#include <public/WebVideoFrameProvider.h>

using namespace CCLayerTestCommon;
using namespace WebCore;
using namespace WebKit;
using namespace WebKitTests;

using ::testing::Mock;
using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::_;

namespace {

class CCLayerTreeHostImplTest : public testing::Test, public CCLayerTreeHostImplClient {
public:
    CCLayerTreeHostImplTest()
        : m_didRequestCommit(false)
        , m_didRequestRedraw(false)
    {
        CCSettings settings;
        m_hostImpl = CCLayerTreeHostImpl::create(settings, this);
        m_hostImpl->initializeLayerRenderer(createContext(), UnthrottledUploader);
        m_hostImpl->setViewportSize(IntSize(10, 10));
    }

    virtual void didLoseContextOnImplThread() OVERRIDE { }
    virtual void onSwapBuffersCompleteOnImplThread() OVERRIDE { }
    virtual void setNeedsRedrawOnImplThread() OVERRIDE { m_didRequestRedraw = true; }
    virtual void setNeedsCommitOnImplThread() OVERRIDE { m_didRequestCommit = true; }
    virtual void postAnimationEventsToMainThreadOnImplThread(PassOwnPtr<CCAnimationEventsVector>, double wallClockTime) OVERRIDE { }
    virtual void postSetContentsMemoryAllocationLimitBytesToMainThreadOnImplThread(size_t) OVERRIDE { }

    PassOwnPtr<CCLayerTreeHostImpl> createLayerTreeHost(bool partialSwap, PassRefPtr<CCGraphicsContext> graphicsContext, PassOwnPtr<CCLayerImpl> rootPtr)
    {
        CCSettings settings;

        settings.partialSwapEnabled = partialSwap;

        OwnPtr<CCLayerTreeHostImpl> myHostImpl = CCLayerTreeHostImpl::create(settings, this);

        myHostImpl->initializeLayerRenderer(graphicsContext, UnthrottledUploader);
        myHostImpl->setViewportSize(IntSize(10, 10));

        OwnPtr<CCLayerImpl> root = rootPtr;

        root->setAnchorPoint(FloatPoint(0, 0));
        root->setPosition(FloatPoint(0, 0));
        root->setBounds(IntSize(10, 10));
        root->setVisibleLayerRect(IntRect(0, 0, 10, 10));
        root->setDrawsContent(true);
        myHostImpl->setRootLayer(root.release());
        return myHostImpl.release();
    }

    static void expectClearedScrollDeltasRecursive(CCLayerImpl* layer)
    {
        ASSERT_EQ(layer->scrollDelta(), IntSize());
        for (size_t i = 0; i < layer->children().size(); ++i)
            expectClearedScrollDeltasRecursive(layer->children()[i].get());
    }

    static void expectContains(const CCScrollAndScaleSet& scrollInfo, int id, const IntSize& scrollDelta)
    {
        int timesEncountered = 0;

        for (size_t i = 0; i < scrollInfo.scrolls.size(); ++i) {
            if (scrollInfo.scrolls[i].layerId != id)
                continue;
            EXPECT_EQ(scrollDelta.width(), scrollInfo.scrolls[i].scrollDelta.width());
            EXPECT_EQ(scrollDelta.height(), scrollInfo.scrolls[i].scrollDelta.height());
            timesEncountered++;
        }

        ASSERT_EQ(timesEncountered, 1);
    }

    void setupScrollAndContentsLayers(const IntSize& contentSize)
    {
        OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
        root->setScrollable(true);
        root->setScrollPosition(IntPoint(0, 0));
        root->setMaxScrollPosition(contentSize);
        OwnPtr<CCLayerImpl> contents = CCLayerImpl::create(1);
        contents->setDrawsContent(true);
        contents->setBounds(contentSize);
        contents->setContentBounds(contentSize);
        root->addChild(contents.release());
        m_hostImpl->setRootLayer(root.release());
    }

protected:
    PassRefPtr<CCGraphicsContext> createContext()
    {
        return CCGraphicsContext::create3D(GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(new FakeWebGraphicsContext3D()), GraphicsContext3D::RenderDirectlyToHostWindow));
    }

    DebugScopedSetImplThread m_alwaysImplThread;
    DebugScopedSetMainThreadBlocked m_alwaysMainThreadBlocked;

    OwnPtr<CCLayerTreeHostImpl> m_hostImpl;
    bool m_didRequestCommit;
    bool m_didRequestRedraw;
};

TEST_F(CCLayerTreeHostImplTest, scrollDeltaNoLayers)
{
    ASSERT_FALSE(m_hostImpl->rootLayer());

    OwnPtr<CCScrollAndScaleSet> scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(scrollInfo->scrolls.size(), 0u);
}

TEST_F(CCLayerTreeHostImplTest, scrollDeltaTreeButNoChanges)
{
    {
        OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
        root->addChild(CCLayerImpl::create(1));
        root->addChild(CCLayerImpl::create(2));
        root->children()[1]->addChild(CCLayerImpl::create(3));
        root->children()[1]->addChild(CCLayerImpl::create(4));
        root->children()[1]->children()[0]->addChild(CCLayerImpl::create(5));
        m_hostImpl->setRootLayer(root.release());
    }
    CCLayerImpl* root = m_hostImpl->rootLayer();

    expectClearedScrollDeltasRecursive(root);

    OwnPtr<CCScrollAndScaleSet> scrollInfo;

    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(scrollInfo->scrolls.size(), 0u);
    expectClearedScrollDeltasRecursive(root);

    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(scrollInfo->scrolls.size(), 0u);
    expectClearedScrollDeltasRecursive(root);
}

TEST_F(CCLayerTreeHostImplTest, scrollDeltaRepeatedScrolls)
{
    IntPoint scrollPosition(20, 30);
    IntSize scrollDelta(11, -15);
    {
        OwnPtr<CCLayerImpl> root = CCLayerImpl::create(10);
        root->setScrollPosition(scrollPosition);
        root->setScrollable(true);
        root->setMaxScrollPosition(IntSize(100, 100));
        root->scrollBy(scrollDelta);
        m_hostImpl->setRootLayer(root.release());
    }
    CCLayerImpl* root = m_hostImpl->rootLayer();

    OwnPtr<CCScrollAndScaleSet> scrollInfo;

    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(scrollInfo->scrolls.size(), 1u);
    EXPECT_EQ(root->sentScrollDelta(), scrollDelta);
    expectContains(*scrollInfo, root->id(), scrollDelta);

    IntSize scrollDelta2(-5, 27);
    root->scrollBy(scrollDelta2);
    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(scrollInfo->scrolls.size(), 1u);
    EXPECT_EQ(root->sentScrollDelta(), scrollDelta + scrollDelta2);
    expectContains(*scrollInfo, root->id(), scrollDelta + scrollDelta2);

    root->scrollBy(IntSize());
    scrollInfo = m_hostImpl->processScrollDeltas();
    EXPECT_EQ(root->sentScrollDelta(), scrollDelta + scrollDelta2);
}

TEST_F(CCLayerTreeHostImplTest, scrollRootCallsCommitAndRedraw)
{
    {
        OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
        root->setScrollable(true);
        root->setScrollPosition(IntPoint(0, 0));
        root->setMaxScrollPosition(IntSize(100, 100));
        m_hostImpl->setRootLayer(root.release());
    }

    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(0, 0), CCInputHandlerClient::Wheel), CCInputHandlerClient::ScrollStarted);
    m_hostImpl->scrollBy(IntSize(0, 10));
    m_hostImpl->scrollEnd();
    EXPECT_TRUE(m_didRequestRedraw);
    EXPECT_TRUE(m_didRequestCommit);
}

TEST_F(CCLayerTreeHostImplTest, wheelEventHandlers)
{
    {
        OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
        root->setScrollable(true);
        root->setScrollPosition(IntPoint(0, 0));
        root->setMaxScrollPosition(IntSize(100, 100));
        m_hostImpl->setRootLayer(root.release());
    }
    CCLayerImpl* root = m_hostImpl->rootLayer();

    root->setHaveWheelEventHandlers(true);
    // With registered event handlers, wheel scrolls have to go to the main thread.
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(0, 0), CCInputHandlerClient::Wheel), CCInputHandlerClient::ScrollFailed);

    // But gesture scrolls can still be handled.
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(0, 0), CCInputHandlerClient::Gesture), CCInputHandlerClient::ScrollStarted);
}

TEST_F(CCLayerTreeHostImplTest, shouldScrollOnMainThread)
{
    OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
    root->setScrollable(true);
    root->setScrollPosition(IntPoint(0, 0));
    root->setMaxScrollPosition(IntSize(100, 100));
    root->setShouldScrollOnMainThread(true);
    m_hostImpl->setRootLayer(root.release());
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(0, 0), CCInputHandlerClient::Wheel), CCInputHandlerClient::ScrollFailed);
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(0, 0), CCInputHandlerClient::Gesture), CCInputHandlerClient::ScrollFailed);
}

TEST_F(CCLayerTreeHostImplTest, nonFastScrollableRegionBasic)
{
    OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
    root->setScrollable(true);
    root->setScrollPosition(IntPoint(0, 0));
    root->setMaxScrollPosition(IntSize(100, 100));
    root->setNonFastScrollableRegion(IntRect(0, 0, 50, 50));
    m_hostImpl->setRootLayer(root.release());
    // All scroll types inside the non-fast scrollable region should fail.
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(25, 25), CCInputHandlerClient::Wheel), CCInputHandlerClient::ScrollFailed);
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(25, 25), CCInputHandlerClient::Gesture), CCInputHandlerClient::ScrollFailed);

    // All scroll types outside this region should succeed.
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(75, 75), CCInputHandlerClient::Wheel), CCInputHandlerClient::ScrollStarted);
    m_hostImpl->scrollBy(IntSize(0, 10));
    m_hostImpl->scrollEnd();
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(75, 75), CCInputHandlerClient::Gesture), CCInputHandlerClient::ScrollStarted);
    m_hostImpl->scrollBy(IntSize(0, 10));
    m_hostImpl->scrollEnd();
}

TEST_F(CCLayerTreeHostImplTest, nonFastScrollableRegionWithOffset)
{
    OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
    root->setScrollable(true);
    root->setScrollPosition(IntPoint(0, 0));
    root->setMaxScrollPosition(IntSize(100, 100));
    root->setNonFastScrollableRegion(IntRect(0, 0, 50, 50));
    root->setPosition(FloatPoint(-25, 0));
    m_hostImpl->setRootLayer(root.release());
    CCLayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame); // Update draw transforms so we can correctly map points into layer space.

    // This point would fall into the non-fast scrollable region except that we've moved the layer down by 25 pixels.
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(40, 10), CCInputHandlerClient::Wheel), CCInputHandlerClient::ScrollStarted);
    m_hostImpl->scrollBy(IntSize(0, 1));
    m_hostImpl->scrollEnd();

    // This point is still inside the non-fast region.
    EXPECT_EQ(m_hostImpl->scrollBegin(IntPoint(10, 10), CCInputHandlerClient::Wheel), CCInputHandlerClient::ScrollFailed);
}

TEST_F(CCLayerTreeHostImplTest, pinchGesture)
{
    setupScrollAndContentsLayers(IntSize(100, 100));
    m_hostImpl->setViewportSize(IntSize(50, 50));

    CCLayerImpl* scrollLayer = m_hostImpl->scrollLayer();
    ASSERT(scrollLayer);

    const float minPageScale = 0.5, maxPageScale = 4;

    // Basic pinch zoom in gesture
    {
        m_hostImpl->setPageScaleFactorAndLimits(1, minPageScale, maxPageScale);
        scrollLayer->setPageScaleDelta(1);
        scrollLayer->setScrollDelta(IntSize());

        float pageScaleDelta = 2;
        m_hostImpl->pinchGestureBegin();
        m_hostImpl->pinchGestureUpdate(pageScaleDelta, IntPoint(50, 50));
        m_hostImpl->pinchGestureEnd();
        EXPECT_TRUE(m_didRequestRedraw);
        EXPECT_TRUE(m_didRequestCommit);

        OwnPtr<CCScrollAndScaleSet> scrollInfo = m_hostImpl->processScrollDeltas();
        EXPECT_EQ(scrollInfo->pageScaleDelta, pageScaleDelta);
    }

    // Zoom-in clamping
    {
        m_hostImpl->setPageScaleFactorAndLimits(1, minPageScale, maxPageScale);
        scrollLayer->setPageScaleDelta(1);
        scrollLayer->setScrollDelta(IntSize());
        float pageScaleDelta = 10;

        m_hostImpl->pinchGestureBegin();
        m_hostImpl->pinchGestureUpdate(pageScaleDelta, IntPoint(50, 50));
        m_hostImpl->pinchGestureEnd();

        OwnPtr<CCScrollAndScaleSet> scrollInfo = m_hostImpl->processScrollDeltas();
        EXPECT_EQ(scrollInfo->pageScaleDelta, maxPageScale);
    }

    // Zoom-out clamping
    {
        m_hostImpl->setPageScaleFactorAndLimits(1, minPageScale, maxPageScale);
        scrollLayer->setPageScaleDelta(1);
        scrollLayer->setScrollDelta(IntSize());
        scrollLayer->setScrollPosition(IntPoint(50, 50));

        float pageScaleDelta = 0.1f;
        m_hostImpl->pinchGestureBegin();
        m_hostImpl->pinchGestureUpdate(pageScaleDelta, IntPoint(0, 0));
        m_hostImpl->pinchGestureEnd();

        OwnPtr<CCScrollAndScaleSet> scrollInfo = m_hostImpl->processScrollDeltas();
        EXPECT_EQ(scrollInfo->pageScaleDelta, minPageScale);

        // Pushed to (0,0) via clamping against contents layer size.
        expectContains(*scrollInfo, scrollLayer->id(), IntSize(-50, -50));
    }

    // Two-finger panning
    {
        m_hostImpl->setPageScaleFactorAndLimits(1, minPageScale, maxPageScale);
        scrollLayer->setPageScaleDelta(1);
        scrollLayer->setScrollDelta(IntSize());
        scrollLayer->setScrollPosition(IntPoint(20, 20));

        float pageScaleDelta = 1;
        m_hostImpl->pinchGestureBegin();
        m_hostImpl->pinchGestureUpdate(pageScaleDelta, IntPoint(10, 10));
        m_hostImpl->pinchGestureUpdate(pageScaleDelta, IntPoint(20, 20));
        m_hostImpl->pinchGestureEnd();

        OwnPtr<CCScrollAndScaleSet> scrollInfo = m_hostImpl->processScrollDeltas();
        EXPECT_EQ(scrollInfo->pageScaleDelta, pageScaleDelta);
        expectContains(*scrollInfo, scrollLayer->id(), IntSize(-10, -10));
    }
}

TEST_F(CCLayerTreeHostImplTest, pageScaleAnimation)
{
    setupScrollAndContentsLayers(IntSize(100, 100));
    m_hostImpl->setViewportSize(IntSize(50, 50));

    CCLayerImpl* scrollLayer = m_hostImpl->scrollLayer();
    ASSERT(scrollLayer);

    const float minPageScale = 0.5, maxPageScale = 4;
    const double startTime = 1;
    const double duration = 0.1;
    const double halfwayThroughAnimation = startTime + duration / 2;
    const double endTime = startTime + duration;

    // Non-anchor zoom-in
    {
        m_hostImpl->setPageScaleFactorAndLimits(1, minPageScale, maxPageScale);
        scrollLayer->setPageScaleDelta(1);
        scrollLayer->setScrollPosition(IntPoint(50, 50));

        m_hostImpl->startPageScaleAnimation(IntSize(0, 0), false, 2, startTime, duration);
        m_hostImpl->animate(halfwayThroughAnimation, halfwayThroughAnimation);
        EXPECT_TRUE(m_didRequestRedraw);
        m_hostImpl->animate(endTime, endTime);
        EXPECT_TRUE(m_didRequestCommit);

        OwnPtr<CCScrollAndScaleSet> scrollInfo = m_hostImpl->processScrollDeltas();
        EXPECT_EQ(scrollInfo->pageScaleDelta, 2);
        expectContains(*scrollInfo, scrollLayer->id(), IntSize(-50, -50));
    }

    // Anchor zoom-out
    {
        m_hostImpl->setPageScaleFactorAndLimits(1, minPageScale, maxPageScale);
        scrollLayer->setPageScaleDelta(1);
        scrollLayer->setScrollPosition(IntPoint(50, 50));

        m_hostImpl->startPageScaleAnimation(IntSize(25, 25), true, minPageScale, startTime, duration);
        m_hostImpl->animate(endTime, endTime);
        EXPECT_TRUE(m_didRequestRedraw);
        EXPECT_TRUE(m_didRequestCommit);

        OwnPtr<CCScrollAndScaleSet> scrollInfo = m_hostImpl->processScrollDeltas();
        EXPECT_EQ(scrollInfo->pageScaleDelta, minPageScale);
        // Pushed to (0,0) via clamping against contents layer size.
        expectContains(*scrollInfo, scrollLayer->id(), IntSize(-50, -50));
    }
}

class DidDrawCheckLayer : public CCTiledLayerImpl {
public:
    static PassOwnPtr<DidDrawCheckLayer> create(int id) { return adoptPtr(new DidDrawCheckLayer(id)); }

    virtual void didDraw()
    {
        m_didDrawCalled = true;
    }

    virtual void willDraw(CCRenderer*, CCGraphicsContext*)
    {
        m_willDrawCalled = true;
    }

    bool didDrawCalled() const { return m_didDrawCalled; }
    bool willDrawCalled() const { return m_willDrawCalled; }

    void clearDidDrawCheck()
    {
        m_didDrawCalled = false;
        m_willDrawCalled = false;
    }

protected:
    explicit DidDrawCheckLayer(int id)
        : CCTiledLayerImpl(id)
        , m_didDrawCalled(false)
        , m_willDrawCalled(false)
    {
        setAnchorPoint(FloatPoint(0, 0));
        setBounds(IntSize(10, 10));
        setContentBounds(IntSize(10, 10));
        setDrawsContent(true);
        setSkipsDraw(false);

        OwnPtr<CCLayerTilingData> tiler = CCLayerTilingData::create(IntSize(100, 100), CCLayerTilingData::HasBorderTexels);
        setTilingData(*tiler.get());
    }

private:
    bool m_didDrawCalled;
    bool m_willDrawCalled;
};

TEST_F(CCLayerTreeHostImplTest, didDrawNotCalledOnHiddenLayer)
{
    // The root layer is always drawn, so run this test on a child layer that
    // will be masked out by the root layer's bounds.
    m_hostImpl->setRootLayer(DidDrawCheckLayer::create(0));
    DidDrawCheckLayer* root = static_cast<DidDrawCheckLayer*>(m_hostImpl->rootLayer());
    root->setMasksToBounds(true);

    root->addChild(DidDrawCheckLayer::create(1));
    DidDrawCheckLayer* layer = static_cast<DidDrawCheckLayer*>(root->children()[0].get());
    // Ensure visibleLayerRect for layer is empty
    layer->setPosition(FloatPoint(100, 100));
    layer->setBounds(IntSize(10, 10));
    layer->setContentBounds(IntSize(10, 10));

    CCLayerTreeHostImpl::FrameData frame;

    EXPECT_FALSE(layer->willDrawCalled());
    EXPECT_FALSE(layer->didDrawCalled());

    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);

    EXPECT_FALSE(layer->willDrawCalled());
    EXPECT_FALSE(layer->didDrawCalled());

    EXPECT_TRUE(layer->visibleLayerRect().isEmpty());

    // Ensure visibleLayerRect for layer layer is not empty
    layer->setPosition(FloatPoint(0, 0));

    EXPECT_FALSE(layer->willDrawCalled());
    EXPECT_FALSE(layer->didDrawCalled());

    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);

    EXPECT_TRUE(layer->willDrawCalled());
    EXPECT_TRUE(layer->didDrawCalled());

    EXPECT_FALSE(layer->visibleLayerRect().isEmpty());
}

TEST_F(CCLayerTreeHostImplTest, willDrawNotCalledOnOccludedLayer)
{
    // Make the viewport large so that we can have large layers that get considered for occlusion (small layers do not).
    IntSize bigSize(1000, 1000);
    m_hostImpl->setViewportSize(bigSize);

    m_hostImpl->setRootLayer(DidDrawCheckLayer::create(0));
    DidDrawCheckLayer* root = static_cast<DidDrawCheckLayer*>(m_hostImpl->rootLayer());

    root->addChild(DidDrawCheckLayer::create(1));
    DidDrawCheckLayer* occludedLayer = static_cast<DidDrawCheckLayer*>(root->children()[0].get());

    root->addChild(DidDrawCheckLayer::create(2));
    DidDrawCheckLayer* topLayer = static_cast<DidDrawCheckLayer*>(root->children()[1].get());
    // This layer covers the occludedLayer above. Make this layer large so it can occlude.
    topLayer->setBounds(bigSize);
    topLayer->setContentBounds(bigSize);
    topLayer->setOpaque(true);

    CCLayerTreeHostImpl::FrameData frame;

    EXPECT_FALSE(occludedLayer->willDrawCalled());
    EXPECT_FALSE(occludedLayer->didDrawCalled());
    EXPECT_FALSE(topLayer->willDrawCalled());
    EXPECT_FALSE(topLayer->didDrawCalled());

    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);

    EXPECT_FALSE(occludedLayer->willDrawCalled());
    EXPECT_FALSE(occludedLayer->didDrawCalled());
    EXPECT_TRUE(topLayer->willDrawCalled());
    EXPECT_TRUE(topLayer->didDrawCalled());
}

TEST_F(CCLayerTreeHostImplTest, didDrawCalledOnAllLayers)
{
    m_hostImpl->setRootLayer(DidDrawCheckLayer::create(0));
    DidDrawCheckLayer* root = static_cast<DidDrawCheckLayer*>(m_hostImpl->rootLayer());

    root->addChild(DidDrawCheckLayer::create(1));
    DidDrawCheckLayer* layer1 = static_cast<DidDrawCheckLayer*>(root->children()[0].get());

    layer1->addChild(DidDrawCheckLayer::create(2));
    DidDrawCheckLayer* layer2 = static_cast<DidDrawCheckLayer*>(layer1->children()[0].get());

    layer1->setOpacity(0.3f);
    layer1->setPreserves3D(false);

    EXPECT_FALSE(root->didDrawCalled());
    EXPECT_FALSE(layer1->didDrawCalled());
    EXPECT_FALSE(layer2->didDrawCalled());

    CCLayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);

    EXPECT_TRUE(root->didDrawCalled());
    EXPECT_TRUE(layer1->didDrawCalled());
    EXPECT_TRUE(layer2->didDrawCalled());

    EXPECT_NE(root->renderSurface(), layer1->renderSurface());
    EXPECT_TRUE(!!layer1->renderSurface());
}

class MissingTextureAnimatingLayer : public DidDrawCheckLayer {
public:
    static PassOwnPtr<MissingTextureAnimatingLayer> create(int id, bool tileMissing, bool skipsDraw, bool animating) { return adoptPtr(new MissingTextureAnimatingLayer(id, tileMissing, skipsDraw, animating)); }

private:
    explicit MissingTextureAnimatingLayer(int id, bool tileMissing, bool skipsDraw, bool animating)
        : DidDrawCheckLayer(id)
    {
        OwnPtr<CCLayerTilingData> tilingData = CCLayerTilingData::create(IntSize(10, 10), CCLayerTilingData::NoBorderTexels);
        tilingData->setBounds(bounds());
        setTilingData(*tilingData.get());
        setSkipsDraw(skipsDraw);
        if (!tileMissing)
            pushTileProperties(0, 0, 1, IntRect());
        if (animating)
            addAnimatedTransformToLayer(*this, 10, 3, 0);
    }
};

TEST_F(CCLayerTreeHostImplTest, prepareToDrawFailsWhenAnimationUsesCheckerboard)
{
    // When the texture is not missing, we draw as usual.
    m_hostImpl->setRootLayer(DidDrawCheckLayer::create(0));
    DidDrawCheckLayer* root = static_cast<DidDrawCheckLayer*>(m_hostImpl->rootLayer());
    root->addChild(MissingTextureAnimatingLayer::create(1, false, false, true));

    CCLayerTreeHostImpl::FrameData frame;

    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);

    // When a texture is missing and we're not animating, we draw as usual with checkerboarding.
    m_hostImpl->setRootLayer(DidDrawCheckLayer::create(0));
    root = static_cast<DidDrawCheckLayer*>(m_hostImpl->rootLayer());
    root->addChild(MissingTextureAnimatingLayer::create(1, true, false, false));

    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);

    // When a texture is missing and we're animating, we don't want to draw anything.
    m_hostImpl->setRootLayer(DidDrawCheckLayer::create(0));
    root = static_cast<DidDrawCheckLayer*>(m_hostImpl->rootLayer());
    root->addChild(MissingTextureAnimatingLayer::create(1, true, false, true));

    EXPECT_FALSE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);

    // When the layer skips draw and we're animating, we still draw the frame.
    m_hostImpl->setRootLayer(DidDrawCheckLayer::create(0));
    root = static_cast<DidDrawCheckLayer*>(m_hostImpl->rootLayer());
    root->addChild(MissingTextureAnimatingLayer::create(1, false, true, true));

    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

class BlendStateTrackerContext: public FakeWebGraphicsContext3D {
public:
    BlendStateTrackerContext() : m_blend(false) { }

    virtual void enable(WGC3Denum cap)
    {
        if (cap == GraphicsContext3D::BLEND)
            m_blend = true;
    }

    virtual void disable(WGC3Denum cap)
    {
        if (cap == GraphicsContext3D::BLEND)
            m_blend = false;
    }

    bool blend() const { return m_blend; }

private:
    bool m_blend;
};

class BlendStateCheckLayer : public CCLayerImpl {
public:
    static PassOwnPtr<BlendStateCheckLayer> create(int id) { return adoptPtr(new BlendStateCheckLayer(id)); }

    virtual void appendQuads(CCQuadCuller& quadList, const CCSharedQuadState* sharedQuadState, bool&) OVERRIDE
    {
        m_quadsAppended = true;

        IntRect opaqueRect;
        if (opaque() || m_opaqueContents)
            opaqueRect = m_quadRect;
        else
            opaqueRect = m_opaqueContentRect;
        OwnPtr<CCDrawQuad> testBlendingDrawQuad = CCTileDrawQuad::create(sharedQuadState, m_quadRect, opaqueRect, 0, IntPoint(), IntSize(1, 1), 0, false, false, false, false, false);
        testBlendingDrawQuad->setQuadVisibleRect(m_quadVisibleRect);
        EXPECT_EQ(m_blend, testBlendingDrawQuad->needsBlending());
        EXPECT_EQ(m_hasRenderSurface, !!renderSurface());
        quadList.append(testBlendingDrawQuad.release());
    }

    void setExpectation(bool blend, bool hasRenderSurface)
    {
        m_blend = blend;
        m_hasRenderSurface = hasRenderSurface;
        m_quadsAppended = false;
    }

    bool quadsAppended() const { return m_quadsAppended; }

    void setQuadRect(const IntRect& rect) { m_quadRect = rect; }
    void setQuadVisibleRect(const IntRect& rect) { m_quadVisibleRect = rect; }
    void setOpaqueContents(bool opaque) { m_opaqueContents = opaque; }
    void setOpaqueContentRect(const IntRect& rect) { m_opaqueContentRect = rect; }

private:
    explicit BlendStateCheckLayer(int id)
        : CCLayerImpl(id)
        , m_blend(false)
        , m_hasRenderSurface(false)
        , m_quadsAppended(false)
        , m_opaqueContents(false)
        , m_quadRect(5, 5, 5, 5)
        , m_quadVisibleRect(5, 5, 5, 5)
    {
        setAnchorPoint(FloatPoint(0, 0));
        setBounds(IntSize(10, 10));
        setDrawsContent(true);
    }

    bool m_blend;
    bool m_hasRenderSurface;
    bool m_quadsAppended;
    bool m_opaqueContents;
    IntRect m_quadRect;
    IntRect m_opaqueContentRect;
    IntRect m_quadVisibleRect;
};

// https://bugs.webkit.org/show_bug.cgi?id=75783
TEST_F(CCLayerTreeHostImplTest, blendingOffWhenDrawingOpaqueLayers)
{

    {
        OwnPtr<CCLayerImpl> root = CCLayerImpl::create(0);
        root->setAnchorPoint(FloatPoint(0, 0));
        root->setBounds(IntSize(10, 10));
        root->setDrawsContent(false);
        m_hostImpl->setRootLayer(root.release());
    }
    CCLayerImpl* root = m_hostImpl->rootLayer();

    root->addChild(BlendStateCheckLayer::create(1));
    BlendStateCheckLayer* layer1 = static_cast<BlendStateCheckLayer*>(root->children()[0].get());

    CCLayerTreeHostImpl::FrameData frame;

    // Opaque layer, drawn without blending.
    layer1->setOpaque(true);
    layer1->setOpaqueContents(true);
    layer1->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with translucent content, but opaque content, so drawn without blending.
    layer1->setOpaque(false);
    layer1->setOpaqueContents(true);
    layer1->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with translucent content and painting, so drawn with blending.
    layer1->setOpaque(false);
    layer1->setOpaqueContents(false);
    layer1->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with translucent opacity, drawn with blending.
    layer1->setOpaque(true);
    layer1->setOpaqueContents(true);
    layer1->setOpacity(0.5);
    layer1->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with translucent opacity and painting, drawn with blending.
    layer1->setOpaque(true);
    layer1->setOpaqueContents(false);
    layer1->setOpacity(0.5);
    layer1->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    layer1->addChild(BlendStateCheckLayer::create(2));
    BlendStateCheckLayer* layer2 = static_cast<BlendStateCheckLayer*>(layer1->children()[0].get());

    // 2 opaque layers, drawn without blending.
    layer1->setOpaque(true);
    layer1->setOpaqueContents(true);
    layer1->setOpacity(1);
    layer1->setExpectation(false, false);
    layer2->setOpaque(true);
    layer2->setOpaqueContents(true);
    layer2->setOpacity(1);
    layer2->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    EXPECT_TRUE(layer2->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Parent layer with translucent content, drawn with blending.
    // Child layer with opaque content, drawn without blending.
    layer1->setOpaque(false);
    layer1->setOpaqueContents(false);
    layer1->setExpectation(true, false);
    layer2->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    EXPECT_TRUE(layer2->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Parent layer with translucent content but opaque painting, drawn without blending.
    // Child layer with opaque content, drawn without blending.
    layer1->setOpaque(false);
    layer1->setOpaqueContents(true);
    layer1->setExpectation(false, false);
    layer2->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    EXPECT_TRUE(layer2->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Parent layer with translucent opacity and opaque content. Since it has a
    // drawing child, it's drawn to a render surface which carries the opacity,
    // so it's itself drawn without blending.
    // Child layer with opaque content, drawn without blending (parent surface
    // carries the inherited opacity).
    layer1->setOpaque(true);
    layer1->setOpaqueContents(true);
    layer1->setOpacity(0.5);
    layer1->setExpectation(false, true);
    layer2->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    EXPECT_TRUE(layer2->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Draw again, but with child non-opaque, to make sure
    // layer1 not culled.
    layer1->setOpaque(true);
    layer1->setOpaqueContents(true);
    layer1->setOpacity(1);
    layer1->setExpectation(false, false);
    layer2->setOpaque(true);
    layer2->setOpaqueContents(true);
    layer2->setOpacity(0.5);
    layer2->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    EXPECT_TRUE(layer2->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // A second way of making the child non-opaque.
    layer1->setOpaque(true);
    layer1->setOpacity(1);
    layer1->setExpectation(false, false);
    layer2->setOpaque(false);
    layer2->setOpaqueContents(false);
    layer2->setOpacity(1);
    layer2->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    EXPECT_TRUE(layer2->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // And when the layer says its not opaque but is painted opaque, it is not blended.
    layer1->setOpaque(true);
    layer1->setOpacity(1);
    layer1->setExpectation(false, false);
    layer2->setOpaque(false);
    layer2->setOpaqueContents(true);
    layer2->setOpacity(1);
    layer2->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    EXPECT_TRUE(layer2->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with partially opaque contents, drawn with blending.
    layer1->setOpaque(false);
    layer1->setQuadRect(IntRect(5, 5, 5, 5));
    layer1->setQuadVisibleRect(IntRect(5, 5, 5, 5));
    layer1->setOpaqueContents(false);
    layer1->setOpaqueContentRect(IntRect(5, 5, 2, 5));
    layer1->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with partially opaque contents partially culled, drawn with blending.
    layer1->setOpaque(false);
    layer1->setQuadRect(IntRect(5, 5, 5, 5));
    layer1->setQuadVisibleRect(IntRect(5, 5, 5, 2));
    layer1->setOpaqueContents(false);
    layer1->setOpaqueContentRect(IntRect(5, 5, 2, 5));
    layer1->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with partially opaque contents culled, drawn with blending.
    layer1->setOpaque(false);
    layer1->setQuadRect(IntRect(5, 5, 5, 5));
    layer1->setQuadVisibleRect(IntRect(7, 5, 3, 5));
    layer1->setOpaqueContents(false);
    layer1->setOpaqueContentRect(IntRect(5, 5, 2, 5));
    layer1->setExpectation(true, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

    // Layer with partially opaque contents and translucent contents culled, drawn without blending.
    layer1->setOpaque(false);
    layer1->setQuadRect(IntRect(5, 5, 5, 5));
    layer1->setQuadVisibleRect(IntRect(5, 5, 2, 5));
    layer1->setOpaqueContents(false);
    layer1->setOpaqueContentRect(IntRect(5, 5, 2, 5));
    layer1->setExpectation(false, false);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(layer1->quadsAppended());
    m_hostImpl->didDrawAllLayers(frame);

}

TEST_F(CCLayerTreeHostImplTest, viewportCovered)
{
    m_hostImpl->initializeLayerRenderer(createContext(), UnthrottledUploader);
    m_hostImpl->setBackgroundColor(Color::gray);

    IntSize viewportSize(1000, 1000);
    m_hostImpl->setViewportSize(viewportSize);

    m_hostImpl->setRootLayer(BlendStateCheckLayer::create(0));
    BlendStateCheckLayer* root = static_cast<BlendStateCheckLayer*>(m_hostImpl->rootLayer());
    root->setExpectation(false, true);
    root->setOpaque(true);

    // No gutter rects
    {
        IntRect layerRect(0, 0, 1000, 1000);
        root->setPosition(layerRect.location());
        root->setBounds(layerRect.size());
        root->setContentBounds(layerRect.size());
        root->setQuadRect(IntRect(IntPoint(), layerRect.size()));
        root->setQuadVisibleRect(IntRect(IntPoint(), layerRect.size()));

        CCLayerTreeHostImpl::FrameData frame;
        EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
        ASSERT_EQ(1u, frame.renderPasses.size());

        size_t numGutterQuads = 0;
        for (size_t i = 0; i < frame.renderPasses[0]->quadList().size(); ++i)
            numGutterQuads += (frame.renderPasses[0]->quadList()[i]->material() == CCDrawQuad::SolidColor) ? 1 : 0;
        EXPECT_EQ(0u, numGutterQuads);
        EXPECT_EQ(1u, frame.renderPasses[0]->quadList().size());

        verifyQuadsExactlyCoverRect(frame.renderPasses[0]->quadList(), IntRect(-layerRect.location(), viewportSize));
        m_hostImpl->didDrawAllLayers(frame);
    }

    // Empty visible content area (fullscreen gutter rect)
    {
        IntRect layerRect(0, 0, 0, 0);
        root->setPosition(layerRect.location());
        root->setBounds(layerRect.size());
        root->setContentBounds(layerRect.size());
        root->setQuadRect(IntRect(IntPoint(), layerRect.size()));
        root->setQuadVisibleRect(IntRect(IntPoint(), layerRect.size()));

        CCLayerTreeHostImpl::FrameData frame;
        EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
        ASSERT_EQ(1u, frame.renderPasses.size());
        m_hostImpl->didDrawAllLayers(frame);

        size_t numGutterQuads = 0;
        for (size_t i = 0; i < frame.renderPasses[0]->quadList().size(); ++i)
            numGutterQuads += (frame.renderPasses[0]->quadList()[i]->material() == CCDrawQuad::SolidColor) ? 1 : 0;
        EXPECT_EQ(1u, numGutterQuads);
        EXPECT_EQ(1u, frame.renderPasses[0]->quadList().size());

        verifyQuadsExactlyCoverRect(frame.renderPasses[0]->quadList(), IntRect(-layerRect.location(), viewportSize));
        m_hostImpl->didDrawAllLayers(frame);
    }

    // Content area in middle of clip rect (four surrounding gutter rects)
    {
        IntRect layerRect(500, 500, 200, 200);
        root->setPosition(layerRect.location());
        root->setBounds(layerRect.size());
        root->setContentBounds(layerRect.size());
        root->setQuadRect(IntRect(IntPoint(), layerRect.size()));
        root->setQuadVisibleRect(IntRect(IntPoint(), layerRect.size()));

        CCLayerTreeHostImpl::FrameData frame;
        EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
        ASSERT_EQ(1u, frame.renderPasses.size());

        size_t numGutterQuads = 0;
        for (size_t i = 0; i < frame.renderPasses[0]->quadList().size(); ++i)
            numGutterQuads += (frame.renderPasses[0]->quadList()[i]->material() == CCDrawQuad::SolidColor) ? 1 : 0;
        EXPECT_EQ(4u, numGutterQuads);
        EXPECT_EQ(5u, frame.renderPasses[0]->quadList().size());

        verifyQuadsExactlyCoverRect(frame.renderPasses[0]->quadList(), IntRect(-layerRect.location(), viewportSize));
        m_hostImpl->didDrawAllLayers(frame);
    }

}


class ReshapeTrackerContext: public FakeWebGraphicsContext3D {
public:
    ReshapeTrackerContext() : m_reshapeCalled(false) { }

    virtual void reshape(int width, int height)
    {
        m_reshapeCalled = true;
    }

    bool reshapeCalled() const { return m_reshapeCalled; }

private:
    bool m_reshapeCalled;
};

class FakeDrawableCCLayerImpl: public CCLayerImpl {
public:
    explicit FakeDrawableCCLayerImpl(int id) : CCLayerImpl(id) { }
};

// Only reshape when we know we are going to draw. Otherwise, the reshape
// can leave the window at the wrong size if we never draw and the proper
// viewport size is never set.
TEST_F(CCLayerTreeHostImplTest, reshapeNotCalledUntilDraw)
{
    ReshapeTrackerContext* reshapeTracker = new ReshapeTrackerContext();
    RefPtr<GraphicsContext3D> context = GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(reshapeTracker), GraphicsContext3D::RenderDirectlyToHostWindow);
    RefPtr<CCGraphicsContext> ccContext = CCGraphicsContext::create3D(context);
    m_hostImpl->initializeLayerRenderer(ccContext, UnthrottledUploader);

    CCLayerImpl* root = new FakeDrawableCCLayerImpl(1);
    root->setAnchorPoint(FloatPoint(0, 0));
    root->setBounds(IntSize(10, 10));
    root->setDrawsContent(true);
    m_hostImpl->setRootLayer(adoptPtr(root));
    EXPECT_FALSE(reshapeTracker->reshapeCalled());

    CCLayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    EXPECT_TRUE(reshapeTracker->reshapeCalled());
    m_hostImpl->didDrawAllLayers(frame);
}

class PartialSwapTrackerContext : public FakeWebGraphicsContext3D {
public:
    virtual void postSubBufferCHROMIUM(int x, int y, int width, int height)
    {
        m_partialSwapRect = IntRect(x, y, width, height);
    }

    virtual WebString getString(WGC3Denum name)
    {
        if (name == GraphicsContext3D::EXTENSIONS)
            return WebString("GL_CHROMIUM_post_sub_buffer GL_CHROMIUM_set_visibility");

        return WebString();
    }

    IntRect partialSwapRect() const { return m_partialSwapRect; }

private:
    IntRect m_partialSwapRect;
};

// Make sure damage tracking propagates all the way to the graphics context,
// where it should request to swap only the subBuffer that is damaged.
TEST_F(CCLayerTreeHostImplTest, partialSwapReceivesDamageRect)
{
    PartialSwapTrackerContext* partialSwapTracker = new PartialSwapTrackerContext();
    RefPtr<GraphicsContext3D> context = GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(partialSwapTracker), GraphicsContext3D::RenderDirectlyToHostWindow);
    RefPtr<CCGraphicsContext> ccContext = CCGraphicsContext::create3D(context);

    // This test creates its own CCLayerTreeHostImpl, so
    // that we can force partial swap enabled.
    CCSettings settings;
    settings.partialSwapEnabled = true;
    OwnPtr<CCLayerTreeHostImpl> layerTreeHostImpl = CCLayerTreeHostImpl::create(settings, this);
    layerTreeHostImpl->initializeLayerRenderer(ccContext, UnthrottledUploader);
    layerTreeHostImpl->setViewportSize(IntSize(500, 500));

    CCLayerImpl* root = new FakeDrawableCCLayerImpl(1);
    CCLayerImpl* child = new FakeDrawableCCLayerImpl(2);
    child->setPosition(FloatPoint(12, 13));
    child->setAnchorPoint(FloatPoint(0, 0));
    child->setBounds(IntSize(14, 15));
    child->setDrawsContent(true);
    root->setAnchorPoint(FloatPoint(0, 0));
    root->setBounds(IntSize(500, 500));
    root->setDrawsContent(true);
    root->addChild(adoptPtr(child));
    layerTreeHostImpl->setRootLayer(adoptPtr(root));

    CCLayerTreeHostImpl::FrameData frame;

    // First frame, the entire screen should get swapped.
    EXPECT_TRUE(layerTreeHostImpl->prepareToDraw(frame));
    layerTreeHostImpl->drawLayers(frame);
    layerTreeHostImpl->didDrawAllLayers(frame);
    layerTreeHostImpl->swapBuffers();
    IntRect actualSwapRect = partialSwapTracker->partialSwapRect();
    IntRect expectedSwapRect = IntRect(IntPoint::zero(), IntSize(500, 500));
    EXPECT_EQ(expectedSwapRect.x(), actualSwapRect.x());
    EXPECT_EQ(expectedSwapRect.y(), actualSwapRect.y());
    EXPECT_EQ(expectedSwapRect.width(), actualSwapRect.width());
    EXPECT_EQ(expectedSwapRect.height(), actualSwapRect.height());

    // Second frame, only the damaged area should get swapped. Damage should be the union
    // of old and new child rects.
    // expected damage rect: IntRect(IntPoint::zero(), IntSize(26, 28));
    // expected swap rect: vertically flipped, with origin at bottom left corner.
    child->setPosition(FloatPoint(0, 0));
    EXPECT_TRUE(layerTreeHostImpl->prepareToDraw(frame));
    layerTreeHostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
    layerTreeHostImpl->swapBuffers();
    actualSwapRect = partialSwapTracker->partialSwapRect();
    expectedSwapRect = IntRect(IntPoint(0, 500-28), IntSize(26, 28));
    EXPECT_EQ(expectedSwapRect.x(), actualSwapRect.x());
    EXPECT_EQ(expectedSwapRect.y(), actualSwapRect.y());
    EXPECT_EQ(expectedSwapRect.width(), actualSwapRect.width());
    EXPECT_EQ(expectedSwapRect.height(), actualSwapRect.height());

    // Make sure that partial swap is constrained to the viewport dimensions
    // expected damage rect: IntRect(IntPoint::zero(), IntSize(500, 500));
    // expected swap rect: flipped damage rect, but also clamped to viewport
    layerTreeHostImpl->setViewportSize(IntSize(10, 10));
    root->setOpacity(0.7f); // this will damage everything
    EXPECT_TRUE(layerTreeHostImpl->prepareToDraw(frame));
    layerTreeHostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
    layerTreeHostImpl->swapBuffers();
    actualSwapRect = partialSwapTracker->partialSwapRect();
    expectedSwapRect = IntRect(IntPoint::zero(), IntSize(10, 10));
    EXPECT_EQ(expectedSwapRect.x(), actualSwapRect.x());
    EXPECT_EQ(expectedSwapRect.y(), actualSwapRect.y());
    EXPECT_EQ(expectedSwapRect.width(), actualSwapRect.width());
    EXPECT_EQ(expectedSwapRect.height(), actualSwapRect.height());
}

class FakeLayerWithQuads : public CCLayerImpl {
public:
    static PassOwnPtr<FakeLayerWithQuads> create(int id) { return adoptPtr(new FakeLayerWithQuads(id)); }

    virtual void appendQuads(CCQuadCuller& quadList, const CCSharedQuadState* sharedQuadState, bool&) OVERRIDE
    {
        const Color gray(100, 100, 100);
        IntRect quadRect(0, 0, 5, 5);
        OwnPtr<CCDrawQuad> myQuad = CCSolidColorDrawQuad::create(sharedQuadState, quadRect, gray);
        quadList.append(myQuad.release());
    }

private:
    FakeLayerWithQuads(int id)
        : CCLayerImpl(id)
    {
    }
};

class MockContext : public FakeWebGraphicsContext3D {
public:
    MOCK_METHOD1(useProgram, void(WebGLId program));
    MOCK_METHOD5(uniform4f, void(WGC3Dint location, WGC3Dfloat x, WGC3Dfloat y, WGC3Dfloat z, WGC3Dfloat w));
    MOCK_METHOD4(uniformMatrix4fv, void(WGC3Dint location, WGC3Dsizei count, WGC3Dboolean transpose, const WGC3Dfloat* value));
    MOCK_METHOD4(drawElements, void(WGC3Denum mode, WGC3Dsizei count, WGC3Denum type, WGC3Dintptr offset));
    MOCK_METHOD1(getString, WebString(WGC3Denum name));
    MOCK_METHOD0(getRequestableExtensionsCHROMIUM, WebString());
    MOCK_METHOD1(enable, void(WGC3Denum cap));
    MOCK_METHOD4(scissor, void(WGC3Dint x, WGC3Dint y, WGC3Dsizei width, WGC3Dsizei height));
};

class MockContextHarness {
private:
    MockContext* m_context;
public:
    MockContextHarness(MockContext* context)
        : m_context(context)
    {
        // Catch "uninteresting" calls
        EXPECT_CALL(*m_context, useProgram(_))
            .Times(0);

        EXPECT_CALL(*m_context, drawElements(_, _, _, _))
            .Times(0);

        // These are not asserted
        EXPECT_CALL(*m_context, uniformMatrix4fv(_, _, _, _))
            .WillRepeatedly(Return());

        EXPECT_CALL(*m_context, uniform4f(_, _, _, _, _))
            .WillRepeatedly(Return());

        // Any other strings are empty
        EXPECT_CALL(*m_context, getString(_))
            .WillRepeatedly(Return(WebString()));

        // Support for partial swap, if needed
        EXPECT_CALL(*m_context, getString(GraphicsContext3D::EXTENSIONS))
            .WillRepeatedly(Return(WebString("GL_CHROMIUM_post_sub_buffer")));

        EXPECT_CALL(*m_context, getRequestableExtensionsCHROMIUM())
            .WillRepeatedly(Return(WebString("GL_CHROMIUM_post_sub_buffer")));

        // Any un-sanctioned calls to enable() are OK
        EXPECT_CALL(*m_context, enable(_))
            .WillRepeatedly(Return());
    }

    void mustDrawSolidQuad()
    {
        EXPECT_CALL(*m_context, drawElements(GraphicsContext3D::TRIANGLES, 6, GraphicsContext3D::UNSIGNED_SHORT, 0))
            .WillOnce(Return())
            .RetiresOnSaturation();

        // 1 is hardcoded return value of fake createProgram()
        EXPECT_CALL(*m_context, useProgram(1))
            .WillOnce(Return())
            .RetiresOnSaturation();

    }

    void mustSetScissor(int x, int y, int width, int height)
    {
        EXPECT_CALL(*m_context, enable(GraphicsContext3D::SCISSOR_TEST))
            .WillRepeatedly(Return());

        EXPECT_CALL(*m_context, scissor(x, y, width, height))
            .Times(AtLeast(1))
            .WillRepeatedly(Return());
    }

};

TEST_F(CCLayerTreeHostImplTest, noPartialSwap)
{
    MockContext* mockContext = new MockContext();
    RefPtr<CCGraphicsContext> context = CCGraphicsContext::create3D(GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(mockContext), GraphicsContext3D::RenderDirectlyToHostWindow));
    MockContextHarness harness(mockContext);

    harness.mustDrawSolidQuad();
    harness.mustSetScissor(0, 0, 10, 10);

    // Run test case
    OwnPtr<CCLayerTreeHostImpl> myHostImpl = createLayerTreeHost(false, context, FakeLayerWithQuads::create(1));

    CCLayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(myHostImpl->prepareToDraw(frame));
    myHostImpl->drawLayers(frame);
    myHostImpl->didDrawAllLayers(frame);
    Mock::VerifyAndClearExpectations(&mockContext);
}

TEST_F(CCLayerTreeHostImplTest, partialSwap)
{
    MockContext* mockContext = new MockContext();
    RefPtr<CCGraphicsContext> context = CCGraphicsContext::create3D(GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(mockContext), GraphicsContext3D::RenderDirectlyToHostWindow));
    MockContextHarness harness(mockContext);

    harness.mustDrawSolidQuad();
    harness.mustSetScissor(0, 0, 10, 10);

    OwnPtr<CCLayerTreeHostImpl> myHostImpl = createLayerTreeHost(true, context, FakeLayerWithQuads::create(1));

    CCLayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(myHostImpl->prepareToDraw(frame));
    myHostImpl->drawLayers(frame);
    myHostImpl->didDrawAllLayers(frame);
    Mock::VerifyAndClearExpectations(&mockContext);
}

TEST_F(CCLayerTreeHostImplTest, partialSwapNoUpdate)
{
    MockContext* mockContext = new MockContext();
    RefPtr<CCGraphicsContext> context = CCGraphicsContext::create3D(GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(mockContext), GraphicsContext3D::RenderDirectlyToHostWindow));
    MockContextHarness harness(mockContext);

    harness.mustDrawSolidQuad();
    harness.mustSetScissor(0, 8, 2, 2);
    harness.mustDrawSolidQuad();
    harness.mustSetScissor(0, 0, 10, 10);

    OwnPtr<CCLayerTreeHostImpl> myHostImpl = createLayerTreeHost(true, context, FakeLayerWithQuads::create(1));
    
    // Draw once to make sure layer is not new
    CCLayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(myHostImpl->prepareToDraw(frame));
    myHostImpl->drawLayers(frame);
    myHostImpl->didDrawAllLayers(frame);

    // Generate update in layer
    CCLayerImpl* root = myHostImpl->rootLayer();
    root->setUpdateRect(FloatRect(0, 0, 2, 2));

    // This draw should generate no new udpates
    EXPECT_TRUE(myHostImpl->prepareToDraw(frame));
    myHostImpl->drawLayers(frame);
    myHostImpl->didDrawAllLayers(frame);

    Mock::VerifyAndClearExpectations(&mockContext);
}

class PartialSwapContext: public FakeWebGraphicsContext3D {
public:
    WebString getString(WGC3Denum name)
    {
        if (name == GraphicsContext3D::EXTENSIONS)
            return WebString("GL_CHROMIUM_post_sub_buffer");
        return WebString();
    }
    
    WebString getRequestableExtensionsCHROMIUM()
    {
        return WebString("GL_CHROMIUM_post_sub_buffer");
    }
};

static PassOwnPtr<CCLayerTreeHostImpl> setupLayersForOpacity(bool partialSwap, CCLayerTreeHostImplClient* client)
{
    CCSettings settings;
    settings.partialSwapEnabled = partialSwap;

    RefPtr<CCGraphicsContext> context = CCGraphicsContext::create3D(GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(new PartialSwapContext()), GraphicsContext3D::RenderDirectlyToHostWindow));
    OwnPtr<CCLayerTreeHostImpl> myHostImpl = CCLayerTreeHostImpl::create(settings, client);
    myHostImpl->initializeLayerRenderer(context.release(), UnthrottledUploader);
    myHostImpl->setViewportSize(IntSize(100, 100));

    /*
      Layers are created as follows:

         +--------------------+
         |                  1 |
         |  +-----------+     |
         |  |         2 |     |
         |  | +-------------------+
         |  | |   3               |
         |  | +-------------------+
         |  |           |     |
         |  +-----------+     |
         |                    |
         |                    |
         +--------------------+

         Layers 1, 2 have render surfaces
     */
    OwnPtr<CCLayerImpl> root = CCLayerImpl::create(1);
    OwnPtr<CCLayerImpl> child = CCLayerImpl::create(2);
    OwnPtr<CCLayerImpl> grandChild = FakeLayerWithQuads::create(3);

    IntRect rootRect(0, 0, 100, 100);
    IntRect childRect(10, 10, 50, 50);
    IntRect grandChildRect(5, 5, 150, 150);

    root->createRenderSurface();
    root->setAnchorPoint(FloatPoint(0, 0));
    root->setPosition(FloatPoint(rootRect.x(), rootRect.y()));
    root->setBounds(IntSize(rootRect.width(), rootRect.height()));
    root->setVisibleLayerRect(rootRect);
    root->setDrawsContent(false);
    root->renderSurface()->setContentRect(IntRect(IntPoint(), IntSize(rootRect.width(), rootRect.height())));

    child->setAnchorPoint(FloatPoint(0, 0));
    child->setPosition(FloatPoint(childRect.x(), childRect.y()));
    child->setOpacity(0.5f);
    child->setBounds(IntSize(childRect.width(), childRect.height()));
    child->setVisibleLayerRect(childRect);
    child->setDrawsContent(false);

    grandChild->setAnchorPoint(FloatPoint(0, 0));
    grandChild->setPosition(IntPoint(grandChildRect.x(), grandChildRect.y()));
    grandChild->setBounds(IntSize(grandChildRect.width(), grandChildRect.height()));
    grandChild->setVisibleLayerRect(grandChildRect);
    grandChild->setDrawsContent(true);

    child->addChild(grandChild.release());
    root->addChild(child.release());

    myHostImpl->setRootLayer(root.release());
    return myHostImpl.release();
}

TEST_F(CCLayerTreeHostImplTest, contributingLayerEmptyScissorPartialSwap)
{
    OwnPtr<CCLayerTreeHostImpl> myHostImpl = setupLayersForOpacity(true, this);

    {
        CCLayerTreeHostImpl::FrameData frame;
        EXPECT_TRUE(myHostImpl->prepareToDraw(frame));

        // Just for consistency, the most interesting stuff already happened
        myHostImpl->drawLayers(frame);
        myHostImpl->didDrawAllLayers(frame);

        // Verify all quads have been computed
        ASSERT_EQ(2U, frame.renderPasses.size());
        ASSERT_EQ(1U, frame.renderPasses[0]->quadList().size());
        ASSERT_EQ(1U, frame.renderPasses[1]->quadList().size());
        EXPECT_EQ(CCDrawQuad::SolidColor, frame.renderPasses[0]->quadList()[0]->material());
        EXPECT_EQ(CCDrawQuad::RenderPass, frame.renderPasses[1]->quadList()[0]->material());
    }
}

TEST_F(CCLayerTreeHostImplTest, contributingLayerEmptyScissorNoPartialSwap)
{
    OwnPtr<CCLayerTreeHostImpl> myHostImpl = setupLayersForOpacity(false, this);

    {
        CCLayerTreeHostImpl::FrameData frame;
        EXPECT_TRUE(myHostImpl->prepareToDraw(frame));

        // Just for consistency, the most interesting stuff already happened
        myHostImpl->drawLayers(frame);
        myHostImpl->didDrawAllLayers(frame);

        // Verify all quads have been computed
        ASSERT_EQ(2U, frame.renderPasses.size());
        ASSERT_EQ(1U, frame.renderPasses[0]->quadList().size());
        ASSERT_EQ(1U, frame.renderPasses[1]->quadList().size());
        EXPECT_EQ(CCDrawQuad::SolidColor, frame.renderPasses[0]->quadList()[0]->material());
        EXPECT_EQ(CCDrawQuad::RenderPass, frame.renderPasses[1]->quadList()[0]->material());
    }
}

TEST_F(CCLayerTreeHostImplTest, didDrawNotCalledOnScissoredLayer)
{
    CCSettings settings;
    settings.partialSwapEnabled = true;

    RefPtr<CCGraphicsContext> context = CCGraphicsContext::create3D(GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(new PartialSwapContext()), GraphicsContext3D::RenderDirectlyToHostWindow));
    OwnPtr<CCLayerTreeHostImpl> myHostImpl = CCLayerTreeHostImpl::create(settings, this);
    myHostImpl->initializeLayerRenderer(context.release(), UnthrottledUploader);
    myHostImpl->setViewportSize(IntSize(10, 10));

    myHostImpl->setRootLayer(DidDrawCheckLayer::create(1));
    DidDrawCheckLayer* root = static_cast<DidDrawCheckLayer*>(myHostImpl->rootLayer());
    root->setMasksToBounds(true);

    root->addChild(DidDrawCheckLayer::create(2));
    DidDrawCheckLayer* layer = static_cast<DidDrawCheckLayer*>(root->children()[0].get());

    CCLayerTreeHostImpl::FrameData frame;

    EXPECT_FALSE(root->willDrawCalled());
    EXPECT_FALSE(root->didDrawCalled());
    EXPECT_FALSE(layer->willDrawCalled());
    EXPECT_FALSE(layer->didDrawCalled());

    // We should draw everything the first frame.
    EXPECT_TRUE(myHostImpl->prepareToDraw(frame));
    myHostImpl->drawLayers(frame);
    myHostImpl->didDrawAllLayers(frame);

    EXPECT_TRUE(root->willDrawCalled());
    EXPECT_TRUE(root->didDrawCalled());
    EXPECT_TRUE(layer->willDrawCalled());
    EXPECT_TRUE(layer->didDrawCalled());

    root->clearDidDrawCheck();
    layer->clearDidDrawCheck();

    EXPECT_FALSE(root->willDrawCalled());
    EXPECT_FALSE(root->didDrawCalled());
    EXPECT_FALSE(layer->willDrawCalled());
    EXPECT_FALSE(layer->didDrawCalled());

    // Drawing again, we should scissor out everything since there is no damage.
    EXPECT_TRUE(myHostImpl->prepareToDraw(frame));
    myHostImpl->drawLayers(frame);
    myHostImpl->didDrawAllLayers(frame);

    EXPECT_FALSE(root->willDrawCalled());
    EXPECT_FALSE(root->didDrawCalled());
    EXPECT_FALSE(layer->willDrawCalled());
    EXPECT_FALSE(layer->didDrawCalled());
}

// Make sure that context lost notifications are propagated through the tree.
class ContextLostNotificationCheckLayer : public CCLayerImpl {
public:
    static PassOwnPtr<ContextLostNotificationCheckLayer> create(int id) { return adoptPtr(new ContextLostNotificationCheckLayer(id)); }

    virtual void didLoseContext() OVERRIDE
    {
        m_didLoseContextCalled = true;
    }

    bool didLoseContextCalled() const { return m_didLoseContextCalled; }

private:
    explicit ContextLostNotificationCheckLayer(int id)
        : CCLayerImpl(id)
        , m_didLoseContextCalled(false)
    {
    }

    bool m_didLoseContextCalled;
};

TEST_F(CCLayerTreeHostImplTest, contextLostAndRestoredNotificationSentToAllLayers)
{
    m_hostImpl->setRootLayer(ContextLostNotificationCheckLayer::create(0));
    ContextLostNotificationCheckLayer* root = static_cast<ContextLostNotificationCheckLayer*>(m_hostImpl->rootLayer());

    root->addChild(ContextLostNotificationCheckLayer::create(1));
    ContextLostNotificationCheckLayer* layer1 = static_cast<ContextLostNotificationCheckLayer*>(root->children()[0].get());

    layer1->addChild(ContextLostNotificationCheckLayer::create(2));
    ContextLostNotificationCheckLayer* layer2 = static_cast<ContextLostNotificationCheckLayer*>(layer1->children()[0].get());

    EXPECT_FALSE(root->didLoseContextCalled());
    EXPECT_FALSE(layer1->didLoseContextCalled());
    EXPECT_FALSE(layer2->didLoseContextCalled());

    m_hostImpl->initializeLayerRenderer(createContext(), UnthrottledUploader);

    EXPECT_TRUE(root->didLoseContextCalled());
    EXPECT_TRUE(layer1->didLoseContextCalled());
    EXPECT_TRUE(layer2->didLoseContextCalled());
}

class FakeWebGraphicsContext3DMakeCurrentFails : public FakeWebGraphicsContext3D {
public:
    virtual bool makeContextCurrent() { return false; }
};

TEST_F(CCLayerTreeHostImplTest, finishAllRenderingAfterContextLost)
{
    // The context initialization will fail, but we should still be able to call finishAllRendering() without any ill effects.
    m_hostImpl->initializeLayerRenderer(CCGraphicsContext::create3D(GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(new FakeWebGraphicsContext3DMakeCurrentFails), GraphicsContext3D::RenderDirectlyToHostWindow)), UnthrottledUploader);
    m_hostImpl->finishAllRendering();
}

class ScrollbarLayerFakePaint : public CCScrollbarLayerImpl {
public:
    static PassOwnPtr<ScrollbarLayerFakePaint> create(int id) { return adoptPtr(new ScrollbarLayerFakePaint(id)); }

    virtual void paint(GraphicsContext*) { }

private:
    ScrollbarLayerFakePaint(int id) : CCScrollbarLayerImpl(id) { }
};

TEST_F(CCLayerTreeHostImplTest, scrollbarLayerLostContext)
{
    m_hostImpl->setRootLayer(ScrollbarLayerFakePaint::create(0));
    ScrollbarLayerFakePaint* scrollbar = static_cast<ScrollbarLayerFakePaint*>(m_hostImpl->rootLayer());
    scrollbar->setBounds(IntSize(1, 1));
    scrollbar->setContentBounds(IntSize(1, 1));
    scrollbar->setDrawsContent(true);

    for (int i = 0; i < 2; ++i) {
        CCLayerTreeHostImpl::FrameData frame;
        EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
        ASSERT(frame.renderPasses.size() == 1);
        CCRenderPass* renderPass = frame.renderPasses[0].get();
        // Scrollbar layer should always generate quads, even after lost context
        EXPECT_GT(renderPass->quadList().size(), 0u);
        m_hostImpl->didDrawAllLayers(frame);
        m_hostImpl->initializeLayerRenderer(createContext(), UnthrottledUploader);
    }
}

// Fake WebGraphicsContext3D that will cause a failure if trying to use a
// resource that wasn't created by it (resources created by
// FakeWebGraphicsContext3D have an id of 1).
class StrictWebGraphicsContext3D : public FakeWebGraphicsContext3D {
public:
    virtual WebGLId createBuffer() { return 2; }
    virtual WebGLId createFramebuffer() { return 3; }
    virtual WebGLId createProgram() { return 4; }
    virtual WebGLId createRenderbuffer() { return 5; }
    virtual WebGLId createShader(WGC3Denum) { return 6; }
    virtual WebGLId createTexture() { return 7; }

    virtual void deleteBuffer(WebGLId id)
    {
        if (id != 2)
            ADD_FAILURE() << "Trying to delete buffer id " << id;
    }

    virtual void deleteFramebuffer(WebGLId id)
    {
        if (id != 3)
            ADD_FAILURE() << "Trying to delete framebuffer id " << id;
    }

    virtual void deleteProgram(WebGLId id)
    {
        if (id != 4)
            ADD_FAILURE() << "Trying to delete program id " << id;
    }

    virtual void deleteRenderbuffer(WebGLId id)
    {
        if (id != 5)
            ADD_FAILURE() << "Trying to delete renderbuffer id " << id;
    }

    virtual void deleteShader(WebGLId id)
    {
        if (id != 6)
            ADD_FAILURE() << "Trying to delete shader id " << id;
    }

    virtual void deleteTexture(WebGLId id)
    {
        if (id != 7)
            ADD_FAILURE() << "Trying to delete texture id " << id;
    }

    virtual void bindBuffer(WGC3Denum, WebGLId id)
    {
        if (id != 2 && id)
            ADD_FAILURE() << "Trying to bind buffer id " << id;
    }

    virtual void bindFramebuffer(WGC3Denum, WebGLId id)
    {
        if (id != 3 && id)
            ADD_FAILURE() << "Trying to bind framebuffer id " << id;
    }

    virtual void useProgram(WebGLId id)
    {
        if (id != 4)
            ADD_FAILURE() << "Trying to use program id " << id;
    }

    virtual void bindRenderbuffer(WGC3Denum, WebGLId id)
    {
        if (id != 5 && id)
            ADD_FAILURE() << "Trying to bind renderbuffer id " << id;
    }

    virtual void attachShader(WebGLId program, WebGLId shader)
    {
        if ((program != 4) || (shader != 6))
            ADD_FAILURE() << "Trying to attach shader id " << shader << " to program id " << program;
    }

    virtual void bindTexture(WGC3Denum, WebGLId id)
    {
        if (id != 7 && id)
            ADD_FAILURE() << "Trying to bind texture id " << id;
    }

    static PassRefPtr<GraphicsContext3D> createGraphicsContext()
    {
        return GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(new StrictWebGraphicsContext3D()), GraphicsContext3D::RenderDirectlyToHostWindow);
    }
};

// Fake video frame that represents a 4x4 YUV video frame.
class FakeVideoFrame: public WebVideoFrame {
public:
    FakeVideoFrame() { memset(m_data, 0x80, sizeof(m_data)); }
    virtual ~FakeVideoFrame() { }
    virtual Format format() const { return FormatYV12; }
    virtual unsigned width() const { return 4; }
    virtual unsigned height() const { return 4; }
    virtual unsigned planes() const { return 3; }
    virtual int stride(unsigned plane) const { return 4; }
    virtual const void* data(unsigned plane) const { return m_data; }
    virtual unsigned textureId() const { return 0; }
    virtual unsigned textureTarget() const { return 0; }

private:
    char m_data[16];
};

// Fake video frame provider that always provides the same FakeVideoFrame.
class FakeVideoFrameProvider: public WebVideoFrameProvider {
public:
    FakeVideoFrameProvider() : m_client(0) { }
    virtual ~FakeVideoFrameProvider()
    {
        if (m_client)
            m_client->stopUsingProvider();
    }

    virtual void setVideoFrameProviderClient(Client* client) { m_client = client; }
    virtual WebVideoFrame* getCurrentFrame() { return &m_frame; }
    virtual void putCurrentFrame(WebVideoFrame*) { }

private:
    FakeVideoFrame m_frame;
    Client* m_client;
};

TEST_F(CCLayerTreeHostImplTest, dontUseOldResourcesAfterLostContext)
{
    OwnPtr<CCLayerImpl> rootLayer(CCLayerImpl::create(0));
    rootLayer->setBounds(IntSize(10, 10));
    rootLayer->setAnchorPoint(FloatPoint(0, 0));

    OwnPtr<CCTiledLayerImpl> tileLayer = CCTiledLayerImpl::create(1);
    tileLayer->setBounds(IntSize(10, 10));
    tileLayer->setAnchorPoint(FloatPoint(0, 0));
    tileLayer->setContentBounds(IntSize(10, 10));
    tileLayer->setDrawsContent(true);
    tileLayer->setSkipsDraw(false);
    OwnPtr<CCLayerTilingData> tilingData(CCLayerTilingData::create(IntSize(10, 10), CCLayerTilingData::NoBorderTexels));
    tilingData->setBounds(IntSize(10, 10));
    tileLayer->setTilingData(*tilingData);
    tileLayer->pushTileProperties(0, 0, 1, IntRect(0, 0, 10, 10));
    rootLayer->addChild(tileLayer.release());

    OwnPtr<CCTextureLayerImpl> textureLayer = CCTextureLayerImpl::create(2);
    textureLayer->setBounds(IntSize(10, 10));
    textureLayer->setAnchorPoint(FloatPoint(0, 0));
    textureLayer->setContentBounds(IntSize(10, 10));
    textureLayer->setDrawsContent(true);
    textureLayer->setTextureId(1);
    rootLayer->addChild(textureLayer.release());

    FakeVideoFrameProvider provider;
    OwnPtr<CCVideoLayerImpl> videoLayer = CCVideoLayerImpl::create(3, &provider);
    videoLayer->setBounds(IntSize(10, 10));
    videoLayer->setAnchorPoint(FloatPoint(0, 0));
    videoLayer->setContentBounds(IntSize(10, 10));
    videoLayer->setDrawsContent(true);
    rootLayer->addChild(videoLayer.release());

    m_hostImpl->setRootLayer(rootLayer.release());

    CCLayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
    m_hostImpl->swapBuffers();

    // Lose the context, replacing it with a StrictWebGraphicsContext3D, that
    // will warn if any resource from the previous context gets used.
    m_hostImpl->initializeLayerRenderer(CCGraphicsContext::create3D(StrictWebGraphicsContext3D::createGraphicsContext()), UnthrottledUploader);
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));
    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
    m_hostImpl->swapBuffers();
}

} // namespace
