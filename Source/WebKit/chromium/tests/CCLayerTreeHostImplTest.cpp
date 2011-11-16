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

#include "GraphicsContext3DPrivate.h"
#include "LayerRendererChromium.h"
#include "MockWebGraphicsContext3D.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCSingleThreadProxy.h"
#include <gtest/gtest.h>

using namespace WebCore;
using namespace WebKit;

namespace {

class CCLayerTreeHostImplTest : public testing::Test, CCLayerTreeHostImplClient {
public:
    CCLayerTreeHostImplTest()
        : m_didRequestCommit(false)
        , m_didRequestRedraw(false)
    {
        CCSettings settings;
        m_hostImpl = CCLayerTreeHostImpl::create(settings, this);
    }

    virtual void onSwapBuffersCompleteOnImplThread() { }
    virtual void setNeedsRedrawOnImplThread() { m_didRequestRedraw = true; }
    virtual void setNeedsCommitOnImplThread() { m_didRequestCommit = true; }

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
            ASSERT_EQ(scrollInfo.scrolls[i].scrollDelta, scrollDelta);
            timesEncountered++;
        }

        ASSERT_EQ(timesEncountered, 1);
    }

protected:
    DebugScopedSetImplThread m_alwaysImplThread;
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
    RefPtr<CCLayerImpl> root = CCLayerImpl::create(0);
    root->addChild(CCLayerImpl::create(1));
    root->addChild(CCLayerImpl::create(2));
    root->children()[1]->addChild(CCLayerImpl::create(3));
    root->children()[1]->addChild(CCLayerImpl::create(4));
    root->children()[1]->children()[0]->addChild(CCLayerImpl::create(5));
    m_hostImpl->setRootLayer(root);

    expectClearedScrollDeltasRecursive(root.get());

    OwnPtr<CCScrollAndScaleSet> scrollInfo;

    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(scrollInfo->scrolls.size(), 0u);
    expectClearedScrollDeltasRecursive(root.get());

    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(scrollInfo->scrolls.size(), 0u);
    expectClearedScrollDeltasRecursive(root.get());
}

TEST_F(CCLayerTreeHostImplTest, scrollDeltaRepeatedScrolls)
{
    IntPoint scrollPosition(20, 30);
    IntSize scrollDelta(11, -15);
    RefPtr<CCLayerImpl> root = CCLayerImpl::create(10);
    root->setScrollPosition(scrollPosition);
    root->setScrollable(true);
    root->setMaxScrollPosition(IntSize(100, 100));
    root->scrollBy(scrollDelta);
    m_hostImpl->setRootLayer(root);

    OwnPtr<CCScrollAndScaleSet> scrollInfo;

    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(root->scrollPosition(), scrollPosition + scrollDelta);
    ASSERT_EQ(scrollInfo->scrolls.size(), 1u);
    EXPECT_EQ(root->sentScrollDelta(), scrollDelta);
    expectContains(*scrollInfo.get(), root->id(), scrollDelta);
    expectClearedScrollDeltasRecursive(root.get());

    IntSize scrollDelta2(-5, 27);
    root->scrollBy(scrollDelta2);
    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(root->scrollPosition(), scrollPosition + scrollDelta + scrollDelta2);
    ASSERT_EQ(scrollInfo->scrolls.size(), 1u);
    expectContains(*scrollInfo.get(), root->id(), scrollDelta2);
    expectClearedScrollDeltasRecursive(root.get());

    root->scrollBy(IntSize());
    scrollInfo = m_hostImpl->processScrollDeltas();
    ASSERT_EQ(root->scrollPosition(), scrollPosition + scrollDelta + scrollDelta2);
    ASSERT_EQ(scrollInfo->scrolls.size(), 0u);
    expectClearedScrollDeltasRecursive(root.get());
}

TEST_F(CCLayerTreeHostImplTest, scrollRootCallsCommitAndRedraw)
{
    RefPtr<CCLayerImpl> root = CCLayerImpl::create(0);
    root->setScrollable(true);
    root->setScrollPosition(IntPoint(0, 0));
    root->setMaxScrollPosition(IntSize(100, 100));
    m_hostImpl->setRootLayer(root);
    m_hostImpl->scrollRootLayer(IntSize(0, 10));
    EXPECT_TRUE(m_didRequestRedraw);
    EXPECT_TRUE(m_didRequestCommit);
}

class BlendStateTrackerContext: public MockWebGraphicsContext3D {
public:
    BlendStateTrackerContext() : m_blend(false) { }

    virtual bool initialize(Attributes, WebView*, bool renderDirectlyToWebView) { return true; }

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
    static PassRefPtr<BlendStateCheckLayer> create(int id) { return adoptRef(new BlendStateCheckLayer(id)); }

    virtual void draw(LayerRendererChromium* renderer)
    {
        m_drawn = true;
        BlendStateTrackerContext* context = static_cast<BlendStateTrackerContext*>(GraphicsContext3DPrivate::extractWebGraphicsContext3D(renderer->context()));
        EXPECT_EQ(m_blend, context->blend());
        EXPECT_EQ(m_hasRenderSurface, !!renderSurface());
    }

    void setExpectation(bool blend, bool hasRenderSurface)
    {
        m_blend = blend;
        m_hasRenderSurface = hasRenderSurface;
        m_drawn = false;
    }

    bool drawn() const { return m_drawn; }

private:
    explicit BlendStateCheckLayer(int id)
        : CCLayerImpl(id)
        , m_blend(false)
        , m_hasRenderSurface(false)
        , m_drawn(false)
    {
        setAnchorPoint(FloatPoint(0, 0));
        setBounds(IntSize(10, 10));
        setDrawsContent(true);
    }

    bool m_blend;
    bool m_hasRenderSurface;
    bool m_drawn;
};

TEST_F(CCLayerTreeHostImplTest, blendingOffWhenDrawingOpaqueLayers)
{
    GraphicsContext3D::Attributes attrs;
    RefPtr<GraphicsContext3D> context = GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(new BlendStateTrackerContext()), attrs, 0, GraphicsContext3D::RenderDirectlyToHostWindow, GraphicsContext3DPrivate::ForUseOnThisThread);
    m_hostImpl->initializeLayerRenderer(context);
    m_hostImpl->setViewport(IntSize(10, 10));

    RefPtr<CCLayerImpl> root = CCLayerImpl::create(0);
    root->setAnchorPoint(FloatPoint(0, 0));
    root->setBounds(IntSize(10, 10));
    root->setDrawsContent(false);
    m_hostImpl->setRootLayer(root);

    RefPtr<BlendStateCheckLayer> layer1 = BlendStateCheckLayer::create(1);
    root->addChild(layer1);

    // Opaque layer, drawn without blending.
    layer1->setOpaque(true);
    layer1->setExpectation(false, false);
    m_hostImpl->drawLayers();
    EXPECT_TRUE(layer1->drawn());

    // Layer with translucent content, drawn with blending.
    layer1->setOpaque(false);
    layer1->setExpectation(true, false);
    m_hostImpl->drawLayers();
    EXPECT_TRUE(layer1->drawn());

    // Layer with translucent opacity, drawn with blending.
    layer1->setOpaque(true);
    layer1->setOpacity(0.5);
    layer1->setExpectation(true, false);
    m_hostImpl->drawLayers();
    EXPECT_TRUE(layer1->drawn());

    RefPtr<BlendStateCheckLayer> layer2 = BlendStateCheckLayer::create(2);
    layer1->addChild(layer2);

    // 2 opaque layers, drawn without blending.
    layer1->setOpaque(true);
    layer1->setOpacity(1);
    layer1->setExpectation(false, false);
    layer2->setOpaque(true);
    layer2->setOpacity(1);
    layer2->setExpectation(false, false);
    m_hostImpl->drawLayers();
    EXPECT_TRUE(layer1->drawn());
    EXPECT_TRUE(layer2->drawn());

    // Parent layer with translucent content, drawn with blending.
    // Child layer with opaque content, drawn without blending.
    layer1->setOpaque(false);
    layer1->setExpectation(true, false);
    layer2->setExpectation(false, false);
    m_hostImpl->drawLayers();
    EXPECT_TRUE(layer1->drawn());
    EXPECT_TRUE(layer2->drawn());

    // Parent layer with translucent opacity and opaque content. Since it has a
    // drawing child, it's drawn to a render surface which carries the opacity,
    // so it's itself drawn without blending.
    // Child layer with opaque content, drawn without blending (parent surface
    // carries the inherited opacity).
    layer1->setOpaque(true);
    layer1->setOpacity(0.5);
    layer1->setExpectation(false, true);
    layer2->setExpectation(false, false);
    m_hostImpl->drawLayers();
    EXPECT_TRUE(layer1->drawn());
    EXPECT_TRUE(layer2->drawn());
}

class ReshapeTrackerContext: public MockWebGraphicsContext3D {
public:
    ReshapeTrackerContext() : m_reshapeCalled(false) { }

    virtual bool initialize(Attributes, WebView*, bool renderDirectlyToWebView) { return true; }

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
    FakeDrawableCCLayerImpl() : CCLayerImpl(0) { }
    virtual void draw(LayerRendererChromium* renderer) { }
};

// Only reshape when we know we are going to draw. Otherwise, the reshape
// can leave the window at the wrong size if we never draw and the proper
// viewport size is never set.
TEST_F(CCLayerTreeHostImplTest, reshapeNotCalledUntilDraw)
{
    GraphicsContext3D::Attributes attrs;
    ReshapeTrackerContext* reshapeTracker = new ReshapeTrackerContext();
    RefPtr<GraphicsContext3D> context = GraphicsContext3DPrivate::createGraphicsContextFromWebContext(adoptPtr(reshapeTracker), attrs, 0, GraphicsContext3D::RenderDirectlyToHostWindow, GraphicsContext3DPrivate::ForUseOnThisThread);
    m_hostImpl->initializeLayerRenderer(context);
    m_hostImpl->setViewport(IntSize(10, 10));

    RefPtr<CCLayerImpl> root = adoptRef(new FakeDrawableCCLayerImpl());
    root->setAnchorPoint(FloatPoint(0, 0));
    root->setBounds(IntSize(10, 10));
    root->setDrawsContent(true);
    m_hostImpl->setRootLayer(root);
    EXPECT_FALSE(reshapeTracker->reshapeCalled());

    m_hostImpl->drawLayers();
    EXPECT_TRUE(reshapeTracker->reshapeCalled());
}


} // namespace
