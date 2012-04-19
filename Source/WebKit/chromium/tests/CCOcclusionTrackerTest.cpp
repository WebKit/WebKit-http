/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "cc/CCOcclusionTracker.h"

#include "CCAnimationTestCommon.h"
#include "CCOcclusionTrackerTestCommon.h"
#include "FilterOperations.h"
#include "LayerChromium.h"
#include "Region.h"
#include "TransformationMatrix.h"
#include "TranslateTransformOperation.h"
#include "cc/CCLayerAnimationController.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCLayerTreeHostCommon.h"
#include "cc/CCSingleThreadProxy.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace WebCore;
using namespace WebKitTests;

#define EXPECT_EQ_RECT(a, b) \
    EXPECT_EQ(a.x(), b.x()); \
    EXPECT_EQ(a.y(), b.y()); \
    EXPECT_EQ(a.width(), b.width()); \
    EXPECT_EQ(a.height(), b.height());

namespace {

class TestContentLayerChromium : public LayerChromium {
public:
    TestContentLayerChromium()
        : LayerChromium()
        , m_overrideOpaqueContentsRect(false)
    {
    }

    virtual bool drawsContent() const OVERRIDE { return true; }
    virtual Region visibleContentOpaqueRegion() const OVERRIDE
    {
        if (m_overrideOpaqueContentsRect)
            return intersection(m_opaqueContentsRect, visibleLayerRect());
        return LayerChromium::visibleContentOpaqueRegion();
    }
    void setOpaqueContentsRect(const IntRect& opaqueContentsRect)
    {
        m_overrideOpaqueContentsRect = true;
        m_opaqueContentsRect = opaqueContentsRect;
    }

private:
    bool m_overrideOpaqueContentsRect;
    IntRect m_opaqueContentsRect;
};

class TestContentLayerImpl : public CCLayerImpl {
public:
    TestContentLayerImpl(int id)
        : CCLayerImpl(id)
        , m_overrideOpaqueContentsRect(false)
    {
        setDrawsContent(true);
    }

    virtual Region visibleContentOpaqueRegion() const OVERRIDE
    {
        if (m_overrideOpaqueContentsRect)
            return intersection(m_opaqueContentsRect, visibleLayerRect());
        return CCLayerImpl::visibleContentOpaqueRegion();
    }
    void setOpaqueContentsRect(const IntRect& opaqueContentsRect)
    {
        m_overrideOpaqueContentsRect = true;
        m_opaqueContentsRect = opaqueContentsRect;
    }

private:
    bool m_overrideOpaqueContentsRect;
    IntRect m_opaqueContentsRect;
};

template<typename LayerType, typename RenderSurfaceType>
class TestCCOcclusionTrackerWithScissor : public TestCCOcclusionTrackerBase<LayerType, RenderSurfaceType> {
public:
    TestCCOcclusionTrackerWithScissor(IntRect screenScissorRect, bool recordMetricsForFrame = false)
        : TestCCOcclusionTrackerBase<LayerType, RenderSurfaceType>(screenScissorRect, recordMetricsForFrame)
        , m_overrideLayerScissorRect(false)
    {
    }

    void setLayerScissorRect(const IntRect& rect) { m_overrideLayerScissorRect = true; m_layerScissorRect = rect;}
    void useDefaultLayerScissorRect() { m_overrideLayerScissorRect = false; }

protected:
    virtual IntRect layerScissorRectInTargetSurface(const LayerType* layer) const { return m_overrideLayerScissorRect ? m_layerScissorRect : CCOcclusionTrackerBase<LayerType, RenderSurfaceType>::layerScissorRectInTargetSurface(layer); }

private:
    bool m_overrideLayerScissorRect;
    IntRect m_layerScissorRect;
};

struct CCOcclusionTrackerTestMainThreadTypes {
    typedef LayerChromium LayerType;
    typedef RenderSurfaceChromium RenderSurfaceType;
    typedef TestContentLayerChromium ContentLayerType;
    typedef RefPtr<LayerChromium> LayerPtrType;
    typedef PassRefPtr<LayerChromium> PassLayerPtrType;
    typedef RefPtr<ContentLayerType> ContentLayerPtrType;
    typedef PassRefPtr<ContentLayerType> PassContentLayerPtrType;
    typedef CCLayerIterator<LayerChromium, Vector<RefPtr<LayerChromium> >, RenderSurfaceChromium, CCLayerIteratorActions::FrontToBack> LayerIterator;
    typedef CCOcclusionTracker OcclusionTrackerType;

    static PassLayerPtrType createLayer() { return LayerChromium::create(); }
    static PassContentLayerPtrType createContentLayer() { return adoptRef(new ContentLayerType()); }
};

struct CCOcclusionTrackerTestImplThreadTypes {
    typedef CCLayerImpl LayerType;
    typedef CCRenderSurface RenderSurfaceType;
    typedef TestContentLayerImpl ContentLayerType;
    typedef OwnPtr<CCLayerImpl> LayerPtrType;
    typedef PassOwnPtr<CCLayerImpl> PassLayerPtrType;
    typedef OwnPtr<ContentLayerType> ContentLayerPtrType;
    typedef PassOwnPtr<ContentLayerType> PassContentLayerPtrType;
    typedef CCLayerIterator<CCLayerImpl, Vector<CCLayerImpl*>, CCRenderSurface, CCLayerIteratorActions::FrontToBack> LayerIterator;
    typedef CCOcclusionTrackerImpl OcclusionTrackerType;

    static PassLayerPtrType createLayer() { return CCLayerImpl::create(nextCCLayerImplId++); }
    static PassContentLayerPtrType createContentLayer() { return adoptPtr(new ContentLayerType(nextCCLayerImplId++)); }
    static int nextCCLayerImplId;
};

int CCOcclusionTrackerTestImplThreadTypes::nextCCLayerImplId = 0;

template<typename Types, bool opaqueLayers>
class CCOcclusionTrackerTest : public testing::Test {
protected:
    CCOcclusionTrackerTest()
        : testing::Test()
    { }

    virtual void runMyTest() = 0;

    virtual void TearDown()
    {
        m_root.clear();
        m_renderSurfaceLayerListChromium.clear();
        m_renderSurfaceLayerListImpl.clear();
        m_replicaLayers.clear();
        CCLayerTreeHost::setNeedsFilterContext(false);
    }

    typename Types::ContentLayerType* createRoot(const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds)
    {
        typename Types::ContentLayerPtrType layer(Types::createContentLayer());
        typename Types::ContentLayerType* layerPtr = layer.get();
        setProperties(layerPtr, transform, position, bounds);

        ASSERT(!m_root);
        m_root = layer.release();
        return layerPtr;
    }

    typename Types::LayerType* createLayer(typename Types::LayerType* parent, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds)
    {
        typename Types::LayerPtrType layer(Types::createLayer());
        typename Types::LayerType* layerPtr = layer.get();
        setProperties(layerPtr, transform, position, bounds);
        parent->addChild(layer.release());
        return layerPtr;
    }

    typename Types::LayerType* createSurface(typename Types::LayerType* parent, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds)
    {
        typename Types::LayerType* layer = createLayer(parent, transform, position, bounds);
        FilterOperations filters;
        filters.operations().append(BasicComponentTransferFilterOperation::create(0.5, FilterOperation::GRAYSCALE));
        layer->setFilters(filters);
        return layer;
    }

    typename Types::ContentLayerType* createDrawingLayer(typename Types::LayerType* parent, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds, bool opaque)
    {
        typename Types::ContentLayerPtrType layer(Types::createContentLayer());
        typename Types::ContentLayerType* layerPtr = layer.get();
        setProperties(layerPtr, transform, position, bounds);

        if (opaqueLayers)
            layerPtr->setOpaque(opaque);
        else {
            layerPtr->setOpaque(false);
            if (opaque)
                layerPtr->setOpaqueContentsRect(IntRect(IntPoint(), bounds));
            else
                layerPtr->setOpaqueContentsRect(IntRect());
        }

        parent->addChild(layer.release());
        return layerPtr;
    }

    typename Types::LayerType* createReplicaLayer(typename Types::LayerType* owningLayer, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds)
    {
        typename Types::ContentLayerPtrType layer(Types::createContentLayer());
        typename Types::ContentLayerType* layerPtr = layer.get();
        setProperties(layerPtr, transform, position, bounds);
        setReplica(owningLayer, layer.release());
        return layerPtr;
    }

    typename Types::ContentLayerType* createDrawingSurface(typename Types::LayerType* parent, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds, bool opaque)
    {
        typename Types::ContentLayerType* layer = createDrawingLayer(parent, transform, position, bounds, opaque);
        FilterOperations filters;
        filters.operations().append(BasicComponentTransferFilterOperation::create(0.5, FilterOperation::GRAYSCALE));
        layer->setFilters(filters);
        return layer;
    }

    void calcDrawEtc(TestContentLayerImpl* root)
    {
        ASSERT(root == m_root.get());
        Vector<CCLayerImpl*> dummyLayerList;
        int dummyMaxTextureSize = 512;

        ASSERT(!root->renderSurface());
        root->createRenderSurface();
        root->renderSurface()->setContentRect(IntRect(IntPoint::zero(), root->bounds()));
        root->setClipRect(IntRect(IntPoint::zero(), root->bounds()));
        m_renderSurfaceLayerListImpl.append(m_root.get());

        CCLayerTreeHostCommon::calculateDrawTransformsAndVisibility(root, root, identityMatrix, identityMatrix, m_renderSurfaceLayerListImpl, dummyLayerList, 0, dummyMaxTextureSize);
        m_layerIterator = m_layerIteratorBegin = Types::LayerIterator::begin(&m_renderSurfaceLayerListImpl);
    }

    void calcDrawEtc(TestContentLayerChromium* root)
    {
        ASSERT(root == m_root.get());
        Vector<RefPtr<LayerChromium> > dummyLayerList;
        int dummyMaxTextureSize = 512;

        ASSERT(!root->renderSurface());
        root->createRenderSurface();
        root->renderSurface()->setContentRect(IntRect(IntPoint::zero(), root->bounds()));
        root->setClipRect(IntRect(IntPoint::zero(), root->bounds()));
        m_renderSurfaceLayerListChromium.append(m_root);

        CCLayerTreeHostCommon::calculateDrawTransformsAndVisibility(root, root, identityMatrix, identityMatrix, m_renderSurfaceLayerListChromium, dummyLayerList, dummyMaxTextureSize);
        m_layerIterator = m_layerIteratorBegin = Types::LayerIterator::begin(&m_renderSurfaceLayerListChromium);
    }

    void enterLayer(typename Types::LayerType* layer, typename Types::OcclusionTrackerType& occlusion)
    {
        ASSERT_EQ(layer, *m_layerIterator);
        ASSERT_TRUE(m_layerIterator.representsItself());
        occlusion.enterLayer(m_layerIterator);
    }

    void leaveLayer(typename Types::LayerType* layer, typename Types::OcclusionTrackerType& occlusion)
    {
        ASSERT_EQ(layer, *m_layerIterator);
        ASSERT_TRUE(m_layerIterator.representsItself());
        occlusion.leaveLayer(m_layerIterator);
        ++m_layerIterator;
    }

    void visitLayer(typename Types::LayerType* layer, typename Types::OcclusionTrackerType& occlusion)
    {
        enterLayer(layer, occlusion);
        leaveLayer(layer, occlusion);
    }

    void enterContributingSurface(typename Types::LayerType* layer, typename Types::OcclusionTrackerType& occlusion)
    {
        ASSERT_EQ(layer, *m_layerIterator);
        ASSERT_TRUE(m_layerIterator.representsTargetRenderSurface());
        occlusion.enterLayer(m_layerIterator);
        occlusion.leaveLayer(m_layerIterator);
        ++m_layerIterator;
        ASSERT_TRUE(m_layerIterator.representsContributingRenderSurface());
        occlusion.enterLayer(m_layerIterator);
    }

    void leaveContributingSurface(typename Types::LayerType* layer, typename Types::OcclusionTrackerType& occlusion)
    {
        ASSERT_EQ(layer, *m_layerIterator);
        ASSERT_TRUE(m_layerIterator.representsContributingRenderSurface());
        occlusion.leaveLayer(m_layerIterator);
        ++m_layerIterator;
    }

    void visitContributingSurface(typename Types::LayerType* layer, typename Types::OcclusionTrackerType& occlusion)
    {
        enterContributingSurface(layer, occlusion);
        leaveContributingSurface(layer, occlusion);
    }

    void resetLayerIterator()
    {
        m_layerIterator = m_layerIteratorBegin;
    }

    const TransformationMatrix identityMatrix;

private:
    void setBaseProperties(typename Types::LayerType* layer, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds)
    {
        layer->setTransform(transform);
        layer->setSublayerTransform(TransformationMatrix());
        layer->setAnchorPoint(FloatPoint(0, 0));
        layer->setPosition(position);
        layer->setBounds(bounds);
    }

    void setProperties(LayerChromium* layer, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds)
    {
        setBaseProperties(layer, transform, position, bounds);
    }

    void setProperties(CCLayerImpl* layer, const TransformationMatrix& transform, const FloatPoint& position, const IntSize& bounds)
    {
        setBaseProperties(layer, transform, position, bounds);

        layer->setContentBounds(layer->bounds());
    }

    void setReplica(LayerChromium* owningLayer, PassRefPtr<LayerChromium> layer)
    {
        owningLayer->setReplicaLayer(layer.get());
        m_replicaLayers.append(layer);
    }

    void setReplica(CCLayerImpl* owningLayer, PassOwnPtr<CCLayerImpl> layer)
    {
        owningLayer->setReplicaLayer(layer);
    }

    // These hold ownership of the layers for the duration of the test.
    typename Types::LayerPtrType m_root;
    Vector<RefPtr<LayerChromium> > m_renderSurfaceLayerListChromium;
    Vector<CCLayerImpl*> m_renderSurfaceLayerListImpl;
    typename Types::LayerIterator m_layerIteratorBegin;
    typename Types::LayerIterator m_layerIterator;
    typename Types::LayerType* m_lastLayerVisited;
    Vector<RefPtr<LayerChromium> > m_replicaLayers;
};

#define RUN_TEST_MAIN_THREAD_OPAQUE_LAYERS(ClassName) \
    class ClassName##MainThreadOpaqueLayers : public ClassName<CCOcclusionTrackerTestMainThreadTypes, true> { \
    public: \
        ClassName##MainThreadOpaqueLayers() : ClassName<CCOcclusionTrackerTestMainThreadTypes, true>() { } \
    }; \
    TEST_F(ClassName##MainThreadOpaqueLayers, runTest) { runMyTest(); }
#define RUN_TEST_MAIN_THREAD_OPAQUE_PAINTS(ClassName) \
    class ClassName##MainThreadOpaquePaints : public ClassName<CCOcclusionTrackerTestMainThreadTypes, false> { \
    public: \
        ClassName##MainThreadOpaquePaints() : ClassName<CCOcclusionTrackerTestMainThreadTypes, false>() { } \
    }; \
    TEST_F(ClassName##MainThreadOpaquePaints, runTest) { runMyTest(); }

#define RUN_TEST_IMPL_THREAD_OPAQUE_LAYERS(ClassName) \
    class ClassName##ImplThreadOpaqueLayers : public ClassName<CCOcclusionTrackerTestImplThreadTypes, true> { \
        DebugScopedSetImplThread impl; \
    public: \
        ClassName##ImplThreadOpaqueLayers() : ClassName<CCOcclusionTrackerTestImplThreadTypes, true>() { } \
    }; \
    TEST_F(ClassName##ImplThreadOpaqueLayers, runTest) { runMyTest(); }
#define RUN_TEST_IMPL_THREAD_OPAQUE_PAINTS(ClassName) \
    class ClassName##ImplThreadOpaquePaints : public ClassName<CCOcclusionTrackerTestImplThreadTypes, false> { \
        DebugScopedSetImplThread impl; \
    public: \
        ClassName##ImplThreadOpaquePaints() : ClassName<CCOcclusionTrackerTestImplThreadTypes, false>() { } \
    }; \
    TEST_F(ClassName##ImplThreadOpaquePaints, runTest) { runMyTest(); }

#define ALL_CCOCCLUSIONTRACKER_TEST(ClassName) \
    RUN_TEST_MAIN_THREAD_OPAQUE_LAYERS(ClassName) \
    RUN_TEST_MAIN_THREAD_OPAQUE_PAINTS(ClassName) \
    RUN_TEST_IMPL_THREAD_OPAQUE_LAYERS(ClassName) \
    RUN_TEST_IMPL_THREAD_OPAQUE_PAINTS(ClassName)

#define MAIN_THREAD_TEST(ClassName) \
    RUN_TEST_MAIN_THREAD_OPAQUE_LAYERS(ClassName)

#define MAIN_AND_IMPL_THREAD_TEST(ClassName) \
    RUN_TEST_MAIN_THREAD_OPAQUE_LAYERS(ClassName) \
    RUN_TEST_IMPL_THREAD_OPAQUE_LAYERS(ClassName)

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestIdentityTransforms : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(30, 30), IntSize(500, 500), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->visitLayer(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 29, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(31, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 31, 70, 70)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 29, 70, 70)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(31, 30, 70, 70)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 31, 70, 70)));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(30, 30, 70, 70)).isEmpty());
        EXPECT_EQ_RECT(IntRect(29, 30, 1, 70), occlusion.unoccludedContentRect(parent, IntRect(29, 30, 70, 70)));
        EXPECT_EQ_RECT(IntRect(29, 29, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(29, 29, 70, 70)));
        EXPECT_EQ_RECT(IntRect(30, 29, 70, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 29, 70, 70)));
        EXPECT_EQ_RECT(IntRect(31, 29, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(31, 29, 70, 70)));
        EXPECT_EQ_RECT(IntRect(100, 30, 1, 70), occlusion.unoccludedContentRect(parent, IntRect(31, 30, 70, 70)));
        EXPECT_EQ_RECT(IntRect(31, 31, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(31, 31, 70, 70)));
        EXPECT_EQ_RECT(IntRect(30, 100, 70, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 31, 70, 70)));
        EXPECT_EQ_RECT(IntRect(29, 31, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(29, 31, 70, 70)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestIdentityTransforms);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestRotatedChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix layerTransform;
        layerTransform.translate(250, 250);
        layerTransform.rotate(90);
        layerTransform.translate(-250, -250);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, layerTransform, FloatPoint(30, 30), IntSize(500, 500), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->visitLayer(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 29, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(31, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 31, 70, 70)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 30, 70, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 29, 70, 70)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(31, 30, 70, 70)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 31, 70, 70)));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(30, 30, 70, 70)).isEmpty());
        EXPECT_EQ_RECT(IntRect(29, 30, 1, 70), occlusion.unoccludedContentRect(parent, IntRect(29, 30, 70, 70)));
        EXPECT_EQ_RECT(IntRect(29, 29, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(29, 29, 70, 70)));
        EXPECT_EQ_RECT(IntRect(30, 29, 70, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 29, 70, 70)));
        EXPECT_EQ_RECT(IntRect(31, 29, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(31, 29, 70, 70)));
        EXPECT_EQ_RECT(IntRect(100, 30, 1, 70), occlusion.unoccludedContentRect(parent, IntRect(31, 30, 70, 70)));
        EXPECT_EQ_RECT(IntRect(31, 31, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(31, 31, 70, 70)));
        EXPECT_EQ_RECT(IntRect(30, 100, 70, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 31, 70, 70)));
        EXPECT_EQ_RECT(IntRect(29, 31, 70, 70), occlusion.unoccludedContentRect(parent, IntRect(29, 31, 70, 70)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestRotatedChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestTranslatedChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix layerTransform;
        layerTransform.translate(20, 20);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, layerTransform, FloatPoint(30, 30), IntSize(500, 500), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->visitLayer(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(50, 50, 50, 50), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(50, 50, 50, 50), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(50, 50, 50, 50)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(49, 50, 50, 50)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(50, 49, 50, 50)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(51, 50, 50, 50)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(50, 51, 50, 50)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(50, 50, 50, 50)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(49, 50, 50, 50)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(50, 49, 50, 50)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(51, 50, 50, 50)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(50, 51, 50, 50)));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(50, 50, 50, 50)).isEmpty());
        EXPECT_EQ_RECT(IntRect(49, 50, 1, 50), occlusion.unoccludedContentRect(parent, IntRect(49, 50, 50, 50)));
        EXPECT_EQ_RECT(IntRect(49, 49, 50, 50), occlusion.unoccludedContentRect(parent, IntRect(49, 49, 50, 50)));
        EXPECT_EQ_RECT(IntRect(50, 49, 50, 1), occlusion.unoccludedContentRect(parent, IntRect(50, 49, 50, 50)));
        EXPECT_EQ_RECT(IntRect(51, 49, 50, 50), occlusion.unoccludedContentRect(parent, IntRect(51, 49, 50, 50)));
        EXPECT_EQ_RECT(IntRect(100, 50, 1, 50), occlusion.unoccludedContentRect(parent, IntRect(51, 50, 50, 50)));
        EXPECT_EQ_RECT(IntRect(51, 51, 50, 50), occlusion.unoccludedContentRect(parent, IntRect(51, 51, 50, 50)));
        EXPECT_EQ_RECT(IntRect(50, 100, 50, 1), occlusion.unoccludedContentRect(parent, IntRect(50, 51, 50, 50)));
        EXPECT_EQ_RECT(IntRect(49, 51, 50, 50), occlusion.unoccludedContentRect(parent, IntRect(49, 51, 50, 50)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(50, 50, 50, 50)).isEmpty());
        EXPECT_EQ_RECT(IntRect(49, 50, 1, 50), occlusion.unoccludedContentRect(parent, IntRect(49, 50, 50, 50)));
        EXPECT_EQ_RECT(IntRect(49, 49, 50, 50), occlusion.unoccludedContentRect(parent, IntRect(49, 49, 50, 50)));
        EXPECT_EQ_RECT(IntRect(50, 49, 50, 1), occlusion.unoccludedContentRect(parent, IntRect(50, 49, 50, 50)));
        EXPECT_EQ_RECT(IntRect(51, 49, 49, 1), occlusion.unoccludedContentRect(parent, IntRect(51, 49, 50, 50)));
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(51, 50, 50, 50)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(51, 51, 50, 50)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(50, 51, 50, 50)).isEmpty());
        EXPECT_EQ_RECT(IntRect(49, 51, 1, 49), occlusion.unoccludedContentRect(parent, IntRect(49, 51, 50, 50)));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestTranslatedChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestChildInRotatedChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix childTransform;
        childTransform.translate(250, 250);
        childTransform.rotate(90);
        childTransform.translate(-250, -250);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::LayerType* child = this->createLayer(parent, childTransform, FloatPoint(30, 30), IntSize(500, 500));
        child->setMasksToBounds(true);
        typename Types::ContentLayerType* layer = this->createDrawingLayer(child, this->identityMatrix, FloatPoint(10, 10), IntSize(500, 500), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->visitLayer(layer, occlusion);
        this->enterContributingSurface(child, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 40, 70, 60), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(10, 430, 60, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(9, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(10, 429, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(10, 430, 61, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(10, 430, 60, 71)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 430, 60, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(9, 430, 60, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 429, 60, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 430, 61, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 430, 60, 71)));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->leaveContributingSurface(child, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 40, 70, 60), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 40, 70, 60), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 39, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(31, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 41, 70, 60)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 39, 70, 60)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(31, 40, 70, 60)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 41, 70, 60)));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));


        /* Justification for the above occlusion from |layer|:
                   100
          +---------------------+                                      +---------------------+
          |                     |                                      |                     |30  Visible region of |layer|: /////
          |    30               |           rotate(90)                 |                     |
          | 30 + ---------------------------------+                    |     +---------------------------------+
      100 |    |  10            |                 |            ==>     |     |               |10               |
          |    |10+---------------------------------+                  |  +---------------------------------+  |
          |    |  |             |                 | |                  |  |  |///////////////|     420      |  |
          |    |  |             |                 | |                  |  |  |///////////////|60            |  |
          |    |  |             |                 | |                  |  |  |///////////////|              |  |
          +----|--|-------------+                 | |                  +--|--|---------------+              |  |
               |  |                               | |                   20|10|     70                       |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |500                  |  |                              |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |                     |  |                              |10|
               +--|-------------------------------+ |                     |  +------------------------------|--+
                  |                                 |                     |                 490             |
                  +---------------------------------+                     +---------------------------------+
                                 500                                                     500
         */
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestChildInRotatedChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestVisitTargetTwoTimes : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix childTransform;
        childTransform.translate(250, 250);
        childTransform.rotate(90);
        childTransform.translate(-250, -250);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::LayerType* child = this->createLayer(parent, childTransform, FloatPoint(30, 30), IntSize(500, 500));
        child->setMasksToBounds(true);
        typename Types::ContentLayerType* layer = this->createDrawingLayer(child, this->identityMatrix, FloatPoint(10, 10), IntSize(500, 500), true);
        // |child2| makes |parent|'s surface get considered by CCOcclusionTracker first, instead of |child|'s. This exercises different code in
        // leaveToTargetRenderSurface, as the target surface has already been seen.
        typename Types::ContentLayerType* child2 = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(30, 30), IntSize(60, 20), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(-10, -10, 1000, 1000));

        this->visitLayer(child2, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 30, 60, 20), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 30, 60, 20), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->visitLayer(layer, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(10, 430, 60, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->enterContributingSurface(child, occlusion);

        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(9, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(10, 429, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(11, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(10, 431, 60, 70)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 430, 60, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(9, 430, 60, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 429, 60, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(11, 430, 60, 70)));
        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 431, 60, 70)));
        occlusion.setLayerScissorRect(IntRect(-10, -10, 1000, 1000));

        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(10, 430, 60, 70)).isEmpty());
        // This is the little piece not occluded by child2
        EXPECT_EQ_RECT(IntRect(9, 430, 1, 10), occlusion.unoccludedContentRect(child, IntRect(9, 430, 60, 70)));
        // This extends past both sides of child2, so it will be the original rect.
        EXPECT_EQ_RECT(IntRect(9, 430, 60, 80), occlusion.unoccludedContentRect(child, IntRect(9, 430, 60, 80)));
        // This extends past two adjacent sides of child2, and should included the unoccluded parts of each side.
        // This also demonstrates that the rect can be arbitrary and does not get clipped to the layer's visibleLayerRect().
        EXPECT_EQ_RECT(IntRect(-10, 430, 20, 70), occlusion.unoccludedContentRect(child, IntRect(-10, 430, 60, 70)));
        // This extends past three adjacent sides of child2, so it should contain the unoccluded parts of each side. The left
        // and bottom edges are completely unoccluded for some row/column so we get back the original query rect.
        EXPECT_EQ_RECT(IntRect(-10, 430, 60, 80), occlusion.unoccludedContentRect(child, IntRect(-10, 430, 60, 80)));
        EXPECT_EQ_RECT(IntRect(10, 429, 60, 1), occlusion.unoccludedContentRect(child, IntRect(10, 429, 60, 70)));
        EXPECT_EQ_RECT(IntRect(70, 430, 1, 70), occlusion.unoccludedContentRect(child, IntRect(11, 430, 60, 70)));
        EXPECT_EQ_RECT(IntRect(10, 500, 60, 1), occlusion.unoccludedContentRect(child, IntRect(10, 431, 60, 70)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(10, 430, 60, 70)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(9, 430, 60, 70)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(9, 430, 60, 80)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(-10, 430, 60, 70)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(-10, 430, 60, 80)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(10, 429, 60, 70)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(11, 430, 60, 70)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(10, 431, 60, 70)).isEmpty());
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        // Occlusion in |child2| should get merged with the |child| surface we are leaving now.
        this->leaveContributingSurface(child, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 30, 70, 70)));
        EXPECT_EQ_RECT(IntRect(90, 30, 10, 10), occlusion.unoccludedContentRect(parent, IntRect(30, 30, 70, 70)));

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 30, 60, 10)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 30, 60, 10)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 29, 60, 10)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(31, 30, 60, 10)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 31, 60, 10)));

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 39, 70, 60)));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(30, 30, 60, 10)).isEmpty());
        EXPECT_EQ_RECT(IntRect(29, 30, 1, 10), occlusion.unoccludedContentRect(parent, IntRect(29, 30, 60, 10)));
        EXPECT_EQ_RECT(IntRect(30, 29, 60, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 29, 60, 10)));
        EXPECT_EQ_RECT(IntRect(90, 30, 1, 10), occlusion.unoccludedContentRect(parent, IntRect(31, 30, 60, 10)));
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(30, 31, 60, 10)).isEmpty());

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(30, 40, 70, 60)).isEmpty());
        EXPECT_EQ_RECT(IntRect(29, 40, 1, 60), occlusion.unoccludedContentRect(parent, IntRect(29, 40, 70, 60)));
        // This rect is mostly occluded by |child2|.
        EXPECT_EQ_RECT(IntRect(90, 39, 10, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 39, 70, 60)));
        // This rect extends past top/right ends of |child2|.
        EXPECT_EQ_RECT(IntRect(30, 29, 70, 11), occlusion.unoccludedContentRect(parent, IntRect(30, 29, 70, 70)));
        // This rect extends past left/right ends of |child2|.
        EXPECT_EQ_RECT(IntRect(20, 39, 80, 60), occlusion.unoccludedContentRect(parent, IntRect(20, 39, 80, 60)));
        EXPECT_EQ_RECT(IntRect(100, 40, 1, 60), occlusion.unoccludedContentRect(parent, IntRect(31, 40, 70, 60)));
        EXPECT_EQ_RECT(IntRect(30, 100, 70, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 41, 70, 60)));

        /* Justification for the above occlusion from |layer|:
                   100
          +---------------------+                                      +---------------------+
          |                     |                                      |                     |30  Visible region of |layer|: /////
          |    30               |           rotate(90)                 |     30   60         |    |child2|: \\\\\
          | 30 + ------------+--------------------+                    |  30 +------------+--------------------+
      100 |    |  10         |  |                 |            ==>     |     |\\\\\\\\\\\\|  |10               |
          |    |10+----------|----------------------+                  |  +--|\\\\\\\\\\\\|-----------------+  |
          |    + ------------+  |                 | |                  |  |  +------------+//|     420      |  |
          |    |  |             |                 | |                  |  |  |///////////////|60            |  |
          |    |  |             |                 | |                  |  |  |///////////////|              |  |
          +----|--|-------------+                 | |                  +--|--|---------------+              |  |
               |  |                               | |                   20|10|     70                       |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |500                  |  |                              |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |                     |  |                              |  |
               |  |                               | |                     |  |                              |10|
               +--|-------------------------------+ |                     |  +------------------------------|--+
                  |                                 |                     |                 490             |
                  +---------------------------------+                     +---------------------------------+
                                 500                                                     500
         */
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestVisitTargetTwoTimes);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceRotatedOffAxis : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix childTransform;
        childTransform.translate(250, 250);
        childTransform.rotate(95);
        childTransform.translate(-250, -250);

        TransformationMatrix layerTransform;
        layerTransform.translate(10, 10);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::LayerType* child = this->createLayer(parent, childTransform, FloatPoint(30, 30), IntSize(500, 500));
        child->setMasksToBounds(true);
        typename Types::ContentLayerType* layer = this->createDrawingLayer(child, layerTransform, FloatPoint(0, 0), IntSize(500, 500), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        IntRect clippedLayerInChild = layerTransform.mapRect(layer->visibleLayerRect());

        this->visitLayer(layer, occlusion);
        this->enterContributingSurface(child, occlusion);

        EXPECT_EQ_RECT(IntRect(), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(0u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(clippedLayerInChild, occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(child, clippedLayerInChild));
        EXPECT_TRUE(occlusion.unoccludedContentRect(child, clippedLayerInChild).isEmpty());
        clippedLayerInChild.move(-1, 0);
        EXPECT_FALSE(occlusion.occluded(child, clippedLayerInChild));
        EXPECT_FALSE(occlusion.unoccludedContentRect(child, clippedLayerInChild).isEmpty());
        clippedLayerInChild.move(1, 0);
        clippedLayerInChild.move(1, 0);
        EXPECT_FALSE(occlusion.occluded(child, clippedLayerInChild));
        EXPECT_FALSE(occlusion.unoccludedContentRect(child, clippedLayerInChild).isEmpty());
        clippedLayerInChild.move(-1, 0);
        clippedLayerInChild.move(0, -1);
        EXPECT_FALSE(occlusion.occluded(child, clippedLayerInChild));
        EXPECT_FALSE(occlusion.unoccludedContentRect(child, clippedLayerInChild).isEmpty());
        clippedLayerInChild.move(0, 1);
        clippedLayerInChild.move(0, 1);
        EXPECT_FALSE(occlusion.occluded(child, clippedLayerInChild));
        EXPECT_FALSE(occlusion.unoccludedContentRect(child, clippedLayerInChild).isEmpty());
        clippedLayerInChild.move(0, -1);

        this->leaveContributingSurface(child, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(0u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(0u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_FALSE(occlusion.occluded(parent, IntRect(75, 55, 1, 1)));
        EXPECT_EQ_RECT(IntRect(75, 55, 1, 1), occlusion.unoccludedContentRect(parent, IntRect(75, 55, 1, 1)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestSurfaceRotatedOffAxis);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceWithTwoOpaqueChildren : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix childTransform;
        childTransform.translate(250, 250);
        childTransform.rotate(90);
        childTransform.translate(-250, -250);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::LayerType* child = this->createLayer(parent, childTransform, FloatPoint(30, 30), IntSize(500, 500));
        child->setMasksToBounds(true);
        typename Types::ContentLayerType* layer1 = this->createDrawingLayer(child, this->identityMatrix, FloatPoint(10, 10), IntSize(500, 500), true);
        typename Types::ContentLayerType* layer2 = this->createDrawingLayer(child, this->identityMatrix, FloatPoint(10, 450), IntSize(500, 60), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->visitLayer(layer2, occlusion);
        this->visitLayer(layer1, occlusion);
        this->enterContributingSurface(child, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 40, 70, 60), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(10, 430, 60, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(child, IntRect(10, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(9, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(10, 429, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(11, 430, 60, 70)));
        EXPECT_FALSE(occlusion.occluded(child, IntRect(10, 431, 60, 70)));

        EXPECT_TRUE(occlusion.unoccludedContentRect(child, IntRect(10, 430, 60, 70)).isEmpty());
        EXPECT_EQ_RECT(IntRect(9, 430, 1, 70), occlusion.unoccludedContentRect(child, IntRect(9, 430, 60, 70)));
        EXPECT_EQ_RECT(IntRect(10, 429, 60, 1), occlusion.unoccludedContentRect(child, IntRect(10, 429, 60, 70)));
        EXPECT_EQ_RECT(IntRect(70, 430, 1, 70), occlusion.unoccludedContentRect(child, IntRect(11, 430, 60, 70)));
        EXPECT_EQ_RECT(IntRect(10, 500, 60, 1), occlusion.unoccludedContentRect(child, IntRect(10, 431, 60, 70)));

        this->leaveContributingSurface(child, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 40, 70, 60), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 40, 70, 60), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 40, 70, 60)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 39, 70, 60)));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(30, 40, 70, 60)).isEmpty());
        EXPECT_EQ_RECT(IntRect(29, 40, 1, 60), occlusion.unoccludedContentRect(parent, IntRect(29, 40, 70, 60)));
        EXPECT_EQ_RECT(IntRect(30, 39, 70, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 39, 70, 60)));
        EXPECT_EQ_RECT(IntRect(100, 40, 1, 60), occlusion.unoccludedContentRect(parent, IntRect(31, 40, 70, 60)));
        EXPECT_EQ_RECT(IntRect(30, 100, 70, 1), occlusion.unoccludedContentRect(parent, IntRect(30, 41, 70, 60)));

        /* Justification for the above occlusion from |layer1| and |layer2|:

           +---------------------+
           |                     |30  Visible region of |layer1|: /////
           |                     |    Visible region of |layer2|: \\\\\
           |     +---------------------------------+
           |     |               |10               |
           |  +---------------+-----------------+  |
           |  |  |\\\\\\\\\\\\|//|     420      |  |
           |  |  |\\\\\\\\\\\\|//|60            |  |
           |  |  |\\\\\\\\\\\\|//|              |  |
           +--|--|------------|--+              |  |
            20|10|     70     |                 |  |
              |  |            |                 |  |
              |  |            |                 |  |
              |  |            |                 |  |
              |  |            |                 |  |
              |  |            |                 |  |
              |  |            |                 |10|
              |  +------------|-----------------|--+
              |               | 490             |
              +---------------+-----------------+
                     60               440
         */
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestSurfaceWithTwoOpaqueChildren);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestOverlappingSurfaceSiblings : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix childTransform;
        childTransform.translate(250, 250);
        childTransform.rotate(90);
        childTransform.translate(-250, -250);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::LayerType* child1 = this->createSurface(parent, childTransform, FloatPoint(30, 30), IntSize(10, 10));
        typename Types::LayerType* child2 = this->createSurface(parent, childTransform, FloatPoint(20, 40), IntSize(10, 10));
        typename Types::ContentLayerType* layer1 = this->createDrawingLayer(child1, this->identityMatrix, FloatPoint(-10, -10), IntSize(510, 510), true);
        typename Types::ContentLayerType* layer2 = this->createDrawingLayer(child2, this->identityMatrix, FloatPoint(-10, -10), IntSize(510, 510), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(-20, -20, 1000, 1000));

        this->visitLayer(layer2, occlusion);
        this->enterContributingSurface(child2, occlusion);

        EXPECT_EQ_RECT(IntRect(20, 30, 80, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(-10, 420, 70, 80), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(child2, IntRect(-10, 420, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-11, 420, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-10, 419, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-10, 420, 71, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-10, 420, 70, 81)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(child2, IntRect(-10, 420, 70, 80)));
        EXPECT_TRUE(occlusion.occluded(child2, IntRect(-11, 420, 70, 80)));
        EXPECT_TRUE(occlusion.occluded(child2, IntRect(-10, 419, 70, 80)));
        EXPECT_TRUE(occlusion.occluded(child2, IntRect(-10, 420, 71, 80)));
        EXPECT_TRUE(occlusion.occluded(child2, IntRect(-10, 420, 70, 81)));
        occlusion.setLayerScissorRect(IntRect(-20, -20, 1000, 1000));

        // There is nothing above child2's surface in the z-order.
        EXPECT_EQ_RECT(IntRect(-10, 420, 70, 80), occlusion.unoccludedContributingSurfaceContentRect(child2, false, IntRect(-10, 420, 70, 80)));

        this->leaveContributingSurface(child2, occlusion);
        this->visitLayer(layer1, occlusion);
        this->enterContributingSurface(child1, occlusion);

        EXPECT_EQ_RECT(IntRect(20, 20, 80, 80), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(-10, 430, 80, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(child1, IntRect(-10, 430, 80, 70)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(-11, 430, 80, 70)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(-10, 429, 80, 70)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(-10, 430, 81, 70)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(-10, 430, 80, 71)));

        // child2's contents will occlude child1 below it.
        EXPECT_EQ_RECT(IntRect(-10, 430, 10, 70), occlusion.unoccludedContributingSurfaceContentRect(child1, false, IntRect(-10, 430, 80, 70)));

        this->leaveContributingSurface(child1, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(20, 20, 80, 80), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(20, 20, 80, 80), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_FALSE(occlusion.occluded(parent, IntRect(20, 20, 80, 80)));

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(30, 20, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(29, 20, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(30, 19, 70, 80)));

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(20, 30, 80, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(19, 30, 80, 70)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(20, 29, 80, 70)));

        /* Justification for the above occlusion:
                   100
          +---------------------+
          |    20               |       layer1
          |  30+ ---------------------------------+
      100 |  30|                |     layer2      |
          |20+----------------------------------+ |
          |  | |                |               | |
          |  | |                |               | |
          |  | |                |               | |
          +--|-|----------------+               | |
             | |                                | | 510
             | |                                | |
             | |                                | |
             | |                                | |
             | |                                | |
             | |                                | |
             | |                                | |
             | +--------------------------------|-+
             |                                  |
             +----------------------------------+
                             510
         */
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestOverlappingSurfaceSiblings);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestOverlappingSurfaceSiblingsWithTwoTransforms : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix child1Transform;
        child1Transform.translate(250, 250);
        child1Transform.rotate(-90);
        child1Transform.translate(-250, -250);

        TransformationMatrix child2Transform;
        child2Transform.translate(250, 250);
        child2Transform.rotate(90);
        child2Transform.translate(-250, -250);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::LayerType* child1 = this->createSurface(parent, child1Transform, FloatPoint(30, 20), IntSize(10, 10));
        typename Types::LayerType* child2 = this->createDrawingSurface(parent, child2Transform, FloatPoint(20, 40), IntSize(10, 10), false);
        typename Types::ContentLayerType* layer1 = this->createDrawingLayer(child1, this->identityMatrix, FloatPoint(-10, -20), IntSize(510, 510), true);
        typename Types::ContentLayerType* layer2 = this->createDrawingLayer(child2, this->identityMatrix, FloatPoint(-10, -10), IntSize(510, 510), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(-30, -30, 1000, 1000));

        this->visitLayer(layer2, occlusion);
        this->enterLayer(child2, occlusion);

        EXPECT_EQ_RECT(IntRect(20, 30, 80, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(-10, 420, 70, 80), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(child2, IntRect(-10, 420, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-11, 420, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-10, 419, 70, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-10, 420, 71, 80)));
        EXPECT_FALSE(occlusion.occluded(child2, IntRect(-10, 420, 70, 81)));

        this->leaveLayer(child2, occlusion);
        this->enterContributingSurface(child2, occlusion);

        // There is nothing above child2's surface in the z-order.
        EXPECT_EQ_RECT(IntRect(-10, 420, 70, 80), occlusion.unoccludedContributingSurfaceContentRect(child2, false, IntRect(-10, 420, 70, 80)));

        this->leaveContributingSurface(child2, occlusion);
        this->visitLayer(layer1, occlusion);
        this->enterContributingSurface(child1, occlusion);

        EXPECT_EQ_RECT(IntRect(10, 20, 90, 80), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(420, -20, 80, 90), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(child1, IntRect(420, -20, 80, 90)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(419, -20, 80, 90)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(420, -21, 80, 90)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(420, -19, 80, 90)));
        EXPECT_FALSE(occlusion.occluded(child1, IntRect(421, -20, 80, 90)));

        // child2's contents will occlude child1 below it.
        EXPECT_EQ_RECT(IntRect(420, -20, 80, 90), occlusion.unoccludedContributingSurfaceContentRect(child1, false, IntRect(420, -20, 80, 90)));
        EXPECT_EQ_RECT(IntRect(490, -10, 10, 80), occlusion.unoccludedContributingSurfaceContentRect(child1, false, IntRect(420, -10, 80, 90)));
        EXPECT_EQ_RECT(IntRect(420, -20, 70, 10), occlusion.unoccludedContributingSurfaceContentRect(child1, false, IntRect(420, -20, 70, 90)));

        this->leaveContributingSurface(child1, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_EQ_RECT(IntRect(10, 20, 90, 80), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(10, 20, 90, 80), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(10, 20, 90, 80)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(9, 20, 90, 80)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(10, 19, 90, 80)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(11, 20, 90, 80)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(10, 21, 90, 80)));

        /* Justification for the above occlusion:
                      100
            +---------------------+
            |20                   |       layer1
           10+----------------------------------+
        100 || 30                 |     layer2  |
            |20+----------------------------------+
            || |                  |             | |
            || |                  |             | |
            || |                  |             | |
            +|-|------------------+             | |
             | |                                | | 510
             | |                            510 | |
             | |                                | |
             | |                                | |
             | |                                | |
             | |                                | |
             | |                520             | |
             +----------------------------------+ |
               |                                  |
               +----------------------------------+
                               510
         */
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestOverlappingSurfaceSiblingsWithTwoTransforms);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestFilters : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix layerTransform;
        layerTransform.translate(250, 250);
        layerTransform.rotate(90);
        layerTransform.translate(-250, -250);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::ContentLayerType* blurLayer = this->createDrawingLayer(parent, layerTransform, FloatPoint(30, 30), IntSize(500, 500), true);
        typename Types::ContentLayerType* opaqueLayer = this->createDrawingLayer(parent, layerTransform, FloatPoint(30, 30), IntSize(500, 500), true);
        typename Types::ContentLayerType* opacityLayer = this->createDrawingLayer(parent, layerTransform, FloatPoint(30, 30), IntSize(500, 500), true);

        FilterOperations filters;
        filters.operations().append(BlurFilterOperation::create(Length(10, WebCore::Percent), FilterOperation::BLUR));
        blurLayer->setFilters(filters);

        filters.operations().clear();
        filters.operations().append(BasicComponentTransferFilterOperation::create(0.5, FilterOperation::GRAYSCALE));
        opaqueLayer->setFilters(filters);

        filters.operations().clear();
        filters.operations().append(BasicComponentTransferFilterOperation::create(0.5, FilterOperation::OPACITY));
        opacityLayer->setFilters(filters);

        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        // Opacity layer won't contribute to occlusion.
        this->visitLayer(opacityLayer, occlusion);
        this->enterContributingSurface(opacityLayer, occlusion);

        EXPECT_TRUE(occlusion.occlusionInScreenSpace().isEmpty());
        EXPECT_TRUE(occlusion.occlusionInTargetSurface().isEmpty());

        // And has nothing to contribute to its parent surface.
        this->leaveContributingSurface(opacityLayer, occlusion);
        EXPECT_TRUE(occlusion.occlusionInScreenSpace().isEmpty());
        EXPECT_TRUE(occlusion.occlusionInTargetSurface().isEmpty());

        // Opaque layer will contribute to occlusion.
        this->visitLayer(opaqueLayer, occlusion);
        this->enterContributingSurface(opaqueLayer, occlusion);

        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 430, 70, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        // And it gets translated to the parent surface.
        this->leaveContributingSurface(opaqueLayer, occlusion);
        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        // The blur layer needs to throw away any occlusion from outside its subtree.
        this->enterLayer(blurLayer, occlusion);
        EXPECT_TRUE(occlusion.occlusionInScreenSpace().isEmpty());
        EXPECT_TRUE(occlusion.occlusionInTargetSurface().isEmpty());

        // And it won't contribute to occlusion.
        this->leaveLayer(blurLayer, occlusion);
        this->enterContributingSurface(blurLayer, occlusion);
        EXPECT_TRUE(occlusion.occlusionInScreenSpace().isEmpty());
        EXPECT_TRUE(occlusion.occlusionInTargetSurface().isEmpty());

        // But the opaque layer's occlusion is preserved on the parent. 
        this->leaveContributingSurface(blurLayer, occlusion);
        this->enterLayer(parent, occlusion);
        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(30, 30, 70, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestFilters);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestReplicaDoesOcclude : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 200));
        typename Types::LayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 100), IntSize(50, 50), true);
        this->createReplicaLayer(surface, this->identityMatrix, FloatPoint(50, 50), IntSize());
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->visitLayer(surface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 100, 50, 50), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 50, 50), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->visitContributingSurface(surface, occlusion);
        this->enterLayer(parent, occlusion);

        // The surface and replica should both be occluding the parent.
        EXPECT_EQ_RECT(IntRect(0, 100, 100, 100), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInTargetSurface().rects().size());
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestReplicaDoesOcclude);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestReplicaWithClipping : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 170));
        typename Types::LayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 100), IntSize(50, 50), true);
        this->createReplicaLayer(surface, this->identityMatrix, FloatPoint(50, 50), IntSize());
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->visitLayer(surface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 100, 50, 50), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 50, 50), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->visitContributingSurface(surface, occlusion);
        this->enterLayer(parent, occlusion);

        // The surface and replica should both be occluding the parent.
        EXPECT_EQ_RECT(IntRect(0, 100, 100, 70), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInTargetSurface().rects().size());
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestReplicaWithClipping);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestLayerScissorRectOutsideChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(200, 100, 100, 100));

        this->enterLayer(layer, occlusion);

        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(200, 100, 100, 100)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(200, 100, 100, 100)));
        occlusion.setLayerScissorRect(IntRect(200, 100, 100, 100));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_EQ_RECT(IntRect(200, 100, 100, 100), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestLayerScissorRectOutsideChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestScreenScissorRectOutsideChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(200, 100, 100, 100));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->enterLayer(layer, occlusion);

        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(200, 100, 100, 100)));

        occlusion.useDefaultLayerScissorRect();
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(200, 100, 100, 100)));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_EQ_RECT(IntRect(200, 100, 100, 100), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestScreenScissorRectOutsideChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestLayerScissorRectOverChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(100, 100, 100, 100));

        this->enterLayer(layer, occlusion);

        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)).isEmpty());
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestLayerScissorRectOverChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestScreenScissorRectOverChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(100, 100, 100, 100));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->enterLayer(layer, occlusion);

        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)).isEmpty());
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestScreenScissorRectOverChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestLayerScissorRectPartlyOverChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(50, 50, 200, 200));

        this->enterLayer(layer, occlusion);

        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_EQ_RECT(IntRect(50, 50, 200, 200), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
        EXPECT_EQ_RECT(IntRect(200, 50, 50, 50), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 100)));
        EXPECT_EQ_RECT(IntRect(200, 100, 50, 100), occlusion.unoccludedContentRect(parent, IntRect(0, 100, 300, 100)));
        EXPECT_EQ_RECT(IntRect(200, 100, 50, 100), occlusion.unoccludedContentRect(parent, IntRect(200, 100, 100, 100)));
        EXPECT_EQ_RECT(IntRect(100, 200, 100, 50), occlusion.unoccludedContentRect(parent, IntRect(100, 200, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestLayerScissorRectPartlyOverChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestScreenScissorRectPartlyOverChild : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(50, 50, 200, 200));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->enterLayer(layer, occlusion);

        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_EQ_RECT(IntRect(50, 50, 200, 200), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
        EXPECT_EQ_RECT(IntRect(200, 50, 50, 50), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 100)));
        EXPECT_EQ_RECT(IntRect(200, 100, 50, 100), occlusion.unoccludedContentRect(parent, IntRect(0, 100, 300, 100)));
        EXPECT_EQ_RECT(IntRect(200, 100, 50, 100), occlusion.unoccludedContentRect(parent, IntRect(200, 100, 100, 100)));
        EXPECT_EQ_RECT(IntRect(100, 200, 100, 50), occlusion.unoccludedContentRect(parent, IntRect(100, 200, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestScreenScissorRectPartlyOverChild);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestLayerScissorRectOverNothing : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(500, 500, 100, 100));

        this->enterLayer(layer, occlusion);

        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 100)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 100, 300, 100)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(200, 100, 100, 100)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(100, 200, 100, 100)).isEmpty());
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestLayerScissorRectOverNothing);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestScreenScissorRectOverNothing : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(500, 500, 100, 100));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        this->enterLayer(layer, occlusion);

        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 100, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 0, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(0, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 200, 100, 100)));
        EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));

        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 100)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(0, 100, 300, 100)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(200, 100, 100, 100)).isEmpty());
        EXPECT_TRUE(occlusion.unoccludedContentRect(parent, IntRect(100, 200, 100, 100)).isEmpty());
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestScreenScissorRectOverNothing);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestLayerScissorRectForLayerOffOrigin : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        this->enterLayer(layer, occlusion);

        // This layer is translated when drawn into its target. So if the scissor rect given from the target surface
        // is not in that target space, then after translating these query rects into the target, they will fall outside
        // the scissor and be considered occluded.
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestLayerScissorRectForLayerOffOrigin);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestOpaqueContentsRegionEmpty : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 200), false);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        this->enterLayer(layer, occlusion);

        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 0, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(0, 100, 100, 100)));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(100, 100, 100, 100)));

        // Occluded since its outside the surface bounds.
        EXPECT_TRUE(occlusion.occluded(layer, IntRect(200, 100, 100, 100)));

        // Test without any scissors.
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));
        EXPECT_FALSE(occlusion.occluded(layer, IntRect(200, 100, 100, 100)));
        occlusion.useDefaultLayerScissorRect();

        this->leaveLayer(layer, occlusion);
        this->visitContributingSurface(layer, occlusion);
        this->enterLayer(parent, occlusion);

        EXPECT_TRUE(occlusion.occlusionInScreenSpace().bounds().isEmpty());
        EXPECT_EQ(0u, occlusion.occlusionInScreenSpace().rects().size());
    }
};

MAIN_AND_IMPL_THREAD_TEST(CCOcclusionTrackerTestOpaqueContentsRegionEmpty);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestOpaqueContentsRegionNonEmpty : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(100, 100), IntSize(200, 200), false);
        this->calcDrawEtc(parent);

        {
            TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
            layer->setOpaqueContentsRect(IntRect(0, 0, 100, 100));

            this->resetLayerIterator();
            this->visitLayer(layer, occlusion);
            this->enterLayer(parent, occlusion);

            EXPECT_EQ_RECT(IntRect(100, 100, 100, 100), occlusion.occlusionInScreenSpace().bounds());
            EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());

            EXPECT_FALSE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
            EXPECT_TRUE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
            EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));
        }

        {
            TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
            layer->setOpaqueContentsRect(IntRect(20, 20, 180, 180));

            this->resetLayerIterator();
            this->visitLayer(layer, occlusion);
            this->enterLayer(parent, occlusion);

            EXPECT_EQ_RECT(IntRect(120, 120, 180, 180), occlusion.occlusionInScreenSpace().bounds());
            EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());

            EXPECT_FALSE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
            EXPECT_FALSE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
            EXPECT_TRUE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));
        }

        {
            TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
            layer->setOpaqueContentsRect(IntRect(150, 150, 100, 100));

            this->resetLayerIterator();
            this->visitLayer(layer, occlusion);
            this->enterLayer(parent, occlusion);

            EXPECT_EQ_RECT(IntRect(250, 250, 50, 50), occlusion.occlusionInScreenSpace().bounds());
            EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());

            EXPECT_FALSE(occlusion.occluded(parent, IntRect(0, 100, 100, 100)));
            EXPECT_FALSE(occlusion.occluded(parent, IntRect(100, 100, 100, 100)));
            EXPECT_FALSE(occlusion.occluded(parent, IntRect(200, 200, 100, 100)));
        }
    }
};

MAIN_AND_IMPL_THREAD_TEST(CCOcclusionTrackerTestOpaqueContentsRegionNonEmpty);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTest3dTransform : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix transform;
        transform.rotate3d(0, 30, 0);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::LayerType* container = this->createLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(container, transform, FloatPoint(100, 100), IntSize(200, 200), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        this->enterLayer(layer, occlusion);

        // The layer is rotated in 3d but without preserving 3d, so it only gets resized.
        EXPECT_EQ_RECT(IntRect(0, 0, 200, 200), occlusion.unoccludedContentRect(layer, IntRect(0, 0, 200, 200)));
    }
};

MAIN_AND_IMPL_THREAD_TEST(CCOcclusionTrackerTest3dTransform);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestPerspectiveTransform : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix transform;
        transform.translate(150, 150);
        transform.applyPerspective(400);
        transform.rotate3d(1, 0, 0, -30);
        transform.translate(-150, -150);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::LayerType* container = this->createLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(container, transform, FloatPoint(100, 100), IntSize(200, 200), true);
        container->setPreserves3D(true);
        layer->setPreserves3D(true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        this->enterLayer(layer, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 200, 200), occlusion.unoccludedContentRect(layer, IntRect(0, 0, 200, 200)));
    }
};

MAIN_THREAD_TEST(CCOcclusionTrackerTestPerspectiveTransform);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestPerspectiveTransformBehindCamera : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        // This test is based on the platform/chromium/compositing/3d-corners.html layout test.
        TransformationMatrix transform;
        transform.translate(250, 50);
        transform.applyPerspective(10);
        transform.translate(-250, -50);
        transform.translate(250, 50);
        transform.rotate3d(1, 0, 0, -167);
        transform.translate(-250, -50);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(500, 100));
        typename Types::LayerType* container = this->createLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(500, 500));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(container, transform, FloatPoint(0, 0), IntSize(500, 500), true);
        container->setPreserves3D(true);
        layer->setPreserves3D(true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        this->enterLayer(layer, occlusion);

        // The bottom 11 pixel rows of this layer remain visible inside the container, after translation to the target surface. When translated back,
        // this will include many more pixels but must include at least the bottom 11 rows.
        EXPECT_TRUE(occlusion.unoccludedContentRect(layer, IntRect(0, 0, 500, 500)).contains(IntRect(0, 489, 500, 11)));
    }
};

MAIN_THREAD_TEST(CCOcclusionTrackerTestPerspectiveTransformBehindCamera);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestLayerBehindCameraDoesNotOcclude : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix transform;
        transform.translate(50, 50);
        transform.applyPerspective(100);
        transform.translate3d(0, 0, 110);
        transform.translate(-50, -50);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, transform, FloatPoint(0, 0), IntSize(100, 100), true);
        parent->setPreserves3D(true);
        layer->setPreserves3D(true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));

        // The |layer| is entirely behind the camera and should not occlude.
        this->visitLayer(layer, occlusion);
        this->enterLayer(parent, occlusion);
        EXPECT_EQ(0u, occlusion.occlusionInTargetSurface().rects().size());
        EXPECT_EQ(0u, occlusion.occlusionInScreenSpace().rects().size());
    }
};

MAIN_THREAD_TEST(CCOcclusionTrackerTestLayerBehindCameraDoesNotOcclude);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestLargePixelsOccludeInsideClipRect : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix transform;
        transform.translate(50, 50);
        transform.applyPerspective(100);
        transform.translate3d(0, 0, 99);
        transform.translate(-50, -50);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, transform, FloatPoint(0, 0), IntSize(100, 100), true);
        parent->setPreserves3D(true);
        layer->setPreserves3D(true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));

        // This is very close to the camera, so pixels in its visibleLayerRect will actually go outside of the layer's clipRect.
        // Ensure that those pixels don't occlude things outside the clipRect.
        this->visitLayer(layer, occlusion);
        this->enterLayer(parent, occlusion);
        EXPECT_EQ(IntRect(0, 0, 100, 100), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
        EXPECT_EQ(IntRect(0, 0, 100, 100), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
    }
};

MAIN_THREAD_TEST(CCOcclusionTrackerTestLargePixelsOccludeInsideClipRect);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestAnimationOpacity1OnMainThread : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), true);
        typename Types::ContentLayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), true);
        typename Types::ContentLayerType* surfaceChild = this->createDrawingLayer(surface, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 300), true);
        typename Types::ContentLayerType* surfaceChild2 = this->createDrawingLayer(surface, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 300), true);
        typename Types::ContentLayerType* parent2 = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), false);
        typename Types::ContentLayerType* topmost = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(250, 0), IntSize(50, 300), true);

        addOpacityTransitionToController(*layer->layerAnimationController(), 10, 0, 1, false);
        addOpacityTransitionToController(*surface->layerAnimationController(), 10, 0, 1, false);
        this->calcDrawEtc(parent);

        EXPECT_TRUE(layer->drawOpacityIsAnimating());
        EXPECT_FALSE(surface->drawOpacityIsAnimating());
        EXPECT_TRUE(surface->renderSurface()->drawOpacityIsAnimating());

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));

        this->visitLayer(topmost, occlusion);
        this->enterLayer(parent2, occlusion);
        // This occlusion will affect all surfaces.
        EXPECT_EQ_RECT(IntRect(0, 0, 250, 300), occlusion.unoccludedContentRect(parent2, IntRect(0, 0, 300, 300)));
        this->leaveLayer(parent2, occlusion);

        this->visitLayer(surfaceChild2, occlusion);
        this->enterLayer(surfaceChild, occlusion);
        EXPECT_EQ_RECT(IntRect(100, 0, 150, 300), occlusion.unoccludedContentRect(surfaceChild, IntRect(0, 0, 300, 300)));
        this->leaveLayer(surfaceChild, occlusion);
        this->enterLayer(surface, occlusion);
        EXPECT_EQ_RECT(IntRect(200, 0, 50, 300), occlusion.unoccludedContentRect(surface, IntRect(0, 0, 300, 300)));
        this->leaveLayer(surface, occlusion);

        this->enterContributingSurface(surface, occlusion);
        // Occlusion within the surface is lost when leaving the animating surface.
        EXPECT_EQ_RECT(IntRect(0, 0, 250, 300), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 300, 300)));
        this->leaveContributingSurface(surface, occlusion);

        this->visitLayer(layer, occlusion);
        this->enterLayer(parent, occlusion);

        // Occlusion is not added for the animating |layer|.
        EXPECT_EQ_RECT(IntRect(0, 0, 250, 300), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
    }
};

MAIN_THREAD_TEST(CCOcclusionTrackerTestAnimationOpacity1OnMainThread);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestAnimationOpacity0OnMainThread : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), true);
        typename Types::ContentLayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), true);
        typename Types::ContentLayerType* surfaceChild = this->createDrawingLayer(surface, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 300), true);
        typename Types::ContentLayerType* surfaceChild2 = this->createDrawingLayer(surface, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 300), true);
        typename Types::ContentLayerType* parent2 = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), false);
        typename Types::ContentLayerType* topmost = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(250, 0), IntSize(50, 300), true);

        addOpacityTransitionToController(*layer->layerAnimationController(), 10, 1, 0, false);
        addOpacityTransitionToController(*surface->layerAnimationController(), 10, 1, 0, false);
        this->calcDrawEtc(parent);

        EXPECT_TRUE(layer->drawOpacityIsAnimating());
        EXPECT_FALSE(surface->drawOpacityIsAnimating());
        EXPECT_TRUE(surface->renderSurface()->drawOpacityIsAnimating());

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));

        this->visitLayer(topmost, occlusion);
        this->enterLayer(parent2, occlusion);
        // This occlusion will affect all surfaces.
        EXPECT_EQ_RECT(IntRect(0, 0, 250, 300), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
        this->leaveLayer(parent2, occlusion);

        this->visitLayer(surfaceChild2, occlusion);
        this->enterLayer(surfaceChild, occlusion);
        EXPECT_EQ_RECT(IntRect(100, 0, 150, 300), occlusion.unoccludedContentRect(surfaceChild, IntRect(0, 0, 300, 300)));
        this->leaveLayer(surfaceChild, occlusion);
        this->enterLayer(surface, occlusion);
        EXPECT_EQ_RECT(IntRect(200, 0, 50, 300), occlusion.unoccludedContentRect(surface, IntRect(0, 0, 300, 300)));
        this->leaveLayer(surface, occlusion);

        this->enterContributingSurface(surface, occlusion);
        // Occlusion within the surface is lost when leaving the animating surface.
        EXPECT_EQ_RECT(IntRect(0, 0, 250, 300), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 300, 300)));
        this->leaveContributingSurface(surface, occlusion);

        this->visitLayer(layer, occlusion);
        this->enterLayer(parent, occlusion);

        // Occlusion is not added for the animating |layer|.
        EXPECT_EQ_RECT(IntRect(0, 0, 250, 300), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
    }
};

MAIN_THREAD_TEST(CCOcclusionTrackerTestAnimationOpacity0OnMainThread);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestAnimationTranslateOnMainThread : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* layer = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), true);
        typename Types::ContentLayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300), true);
        typename Types::ContentLayerType* surfaceChild = this->createDrawingLayer(surface, this->identityMatrix, FloatPoint(0, 0), IntSize(200, 300), true);
        typename Types::ContentLayerType* surfaceChild2 = this->createDrawingLayer(surface, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 300), true);
        typename Types::ContentLayerType* surface2 = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(50, 300), true);

        addAnimatedTransformToController(*layer->layerAnimationController(), 10, 30, 0);
        addAnimatedTransformToController(*surface->layerAnimationController(), 10, 30, 0);
        addAnimatedTransformToController(*surfaceChild->layerAnimationController(), 10, 30, 0);
        this->calcDrawEtc(parent);

        EXPECT_TRUE(layer->drawTransformIsAnimating());
        EXPECT_TRUE(layer->screenSpaceTransformIsAnimating());
        EXPECT_TRUE(surface->renderSurface()->targetSurfaceTransformsAreAnimating());
        EXPECT_TRUE(surface->renderSurface()->screenSpaceTransformsAreAnimating());
        // The surface owning layer doesn't animate against its own surface.
        EXPECT_FALSE(surface->drawTransformIsAnimating());
        EXPECT_TRUE(surface->screenSpaceTransformIsAnimating());
        EXPECT_TRUE(surfaceChild->drawTransformIsAnimating());
        EXPECT_TRUE(surfaceChild->screenSpaceTransformIsAnimating());

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));

        this->visitLayer(surface2, occlusion);
        this->enterContributingSurface(surface2, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 50, 300), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());

        this->leaveContributingSurface(surface2, occlusion);
        this->enterLayer(surfaceChild2, occlusion);

        // surfaceChild2 is moving in screen space but not relative to its target, so occlusion should happen in its target space only.
        // It also means that things occluding in screen space (e.g. surface2) cannot occlude this layer.
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 300), occlusion.unoccludedContentRect(surfaceChild2, IntRect(0, 0, 100, 300)));
        EXPECT_FALSE(occlusion.occluded(surfaceChild, IntRect(0, 0, 50, 300)));

        this->leaveLayer(surfaceChild2, occlusion);
        this->enterLayer(surfaceChild, occlusion);
        EXPECT_FALSE(occlusion.occluded(surfaceChild, IntRect(0, 0, 100, 300)));
        EXPECT_EQ_RECT(IntRect(0, 0, 50, 300), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 300), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
        EXPECT_EQ_RECT(IntRect(100, 0, 200, 300), occlusion.unoccludedContentRect(surface, IntRect(0, 0, 300, 300)));

        // The surfaceChild is occluded by the surfaceChild2, but is moving relative its target and the screen, so it
        // can't be occluded.
        EXPECT_EQ_RECT(IntRect(0, 0, 200, 300), occlusion.unoccludedContentRect(surfaceChild, IntRect(0, 0, 200, 300)));
        EXPECT_FALSE(occlusion.occluded(surfaceChild, IntRect(0, 0, 50, 300)));

        this->leaveLayer(surfaceChild, occlusion);
        this->enterLayer(surface, occlusion);
        // The surfaceChild is moving in screen space but not relative to its target, so occlusion should happen in its target space only.
        EXPECT_EQ_RECT(IntRect(0, 0, 50, 300), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 300), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
        EXPECT_EQ_RECT(IntRect(100, 0, 200, 300), occlusion.unoccludedContentRect(surface, IntRect(0, 0, 300, 300)));

        this->leaveLayer(surface, occlusion);
        // The surface's owning layer is moving in screen space but not relative to its target, so occlusion should happen in its target space only.
        EXPECT_EQ_RECT(IntRect(0, 0, 50, 300), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 300, 300), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 0, 0), occlusion.unoccludedContentRect(surface, IntRect(0, 0, 300, 300)));

        this->enterContributingSurface(surface, occlusion);
        // The contributing |surface| is animating so it can't be occluded.
        EXPECT_EQ_RECT(IntRect(0, 0, 300, 300), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 300, 300)));
        this->leaveContributingSurface(surface, occlusion);

        this->enterLayer(layer, occlusion);
        // The |surface| is moving in the screen and in its target, so all occlusion within the surface is lost when leaving it.
        EXPECT_EQ_RECT(IntRect(50, 0, 250, 300), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
        this->leaveLayer(layer, occlusion);

        this->enterLayer(parent, occlusion);
        // The |layer| is animating in the screen and in its target, so no occlusion is added.
        EXPECT_EQ_RECT(IntRect(50, 0, 250, 300), occlusion.unoccludedContentRect(parent, IntRect(0, 0, 300, 300)));
    }
};

MAIN_THREAD_TEST(CCOcclusionTrackerTestAnimationTranslateOnMainThread);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceOcclusionTranslatesToParent : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        TransformationMatrix surfaceTransform;
        surfaceTransform.translate(300, 300);
        surfaceTransform.scale(2);
        surfaceTransform.translate(-150, -150);

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(500, 500));
        typename Types::ContentLayerType* surface = this->createDrawingSurface(parent, surfaceTransform, FloatPoint(0, 0), IntSize(300, 300), false);
        typename Types::ContentLayerType* surface2 = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(50, 50), IntSize(300, 300), false);
        surface->setOpaqueContentsRect(IntRect(0, 0, 200, 200));
        surface2->setOpaqueContentsRect(IntRect(0, 0, 200, 200));
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));

        this->visitLayer(surface2, occlusion);
        this->visitContributingSurface(surface2, occlusion);

        EXPECT_EQ_RECT(IntRect(50, 50, 200, 200), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(50, 50, 200, 200), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        // Clear any stored occlusion.
        occlusion.setOcclusionInScreenSpace(Region());
        occlusion.setOcclusionInTargetSurface(Region());

        this->visitLayer(surface, occlusion);
        this->visitContributingSurface(surface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 400, 400), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 400, 400), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
    }
};

MAIN_AND_IMPL_THREAD_TEST(CCOcclusionTrackerTestSurfaceOcclusionTranslatesToParent);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceOcclusionTranslatesWithClipping : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(300, 300));
        typename Types::ContentLayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(500, 300), false);
        surface->setOpaqueContentsRect(IntRect(0, 0, 400, 200));
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));

        this->visitLayer(surface, occlusion);
        this->visitContributingSurface(surface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 300, 200), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 300, 200), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
    }
};

MAIN_AND_IMPL_THREAD_TEST(CCOcclusionTrackerTestSurfaceOcclusionTranslatesWithClipping);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestReplicaOccluded : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 200));
        typename Types::LayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100), true);
        this->createReplicaLayer(surface, this->identityMatrix, FloatPoint(0, 100), IntSize(100, 100));
        typename Types::LayerType* topmost = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 100), IntSize(100, 100), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        // |topmost| occludes the replica, but not the surface itself.
        this->visitLayer(topmost, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 100, 100, 100), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 100, 100, 100), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->visitLayer(surface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 100, 200), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 100), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->enterContributingSurface(surface, occlusion);

        // Surface is not occluded so it shouldn't think it is.
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 100), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestReplicaOccluded);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceWithReplicaUnoccluded : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 200));
        typename Types::LayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100), true);
        this->createReplicaLayer(surface, this->identityMatrix, FloatPoint(0, 100), IntSize(100, 100));
        typename Types::LayerType* topmost = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 110), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        // |topmost| occludes the surface, but not the entire surface's replica.
        this->visitLayer(topmost, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 100, 110), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 110), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->visitLayer(surface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 100, 110), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 100), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->enterContributingSurface(surface, occlusion);

        // Surface is occluded, but only the top 10px of the replica.
        EXPECT_EQ_RECT(IntRect(0, 0, 0, 0), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 100, 100)));
        EXPECT_EQ_RECT(IntRect(0, 10, 100, 90), occlusion.unoccludedContributingSurfaceContentRect(surface, true, IntRect(0, 0, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestSurfaceWithReplicaUnoccluded);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceAndReplicaOccludedDifferently : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 200));
        typename Types::LayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100), true);
        this->createReplicaLayer(surface, this->identityMatrix, FloatPoint(0, 100), IntSize(100, 100));
        typename Types::LayerType* overSurface = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(40, 100), true);
        typename Types::LayerType* overReplica = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 100), IntSize(50, 100), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        // These occlude the surface and replica differently, so we can test each one.
        this->visitLayer(overReplica, occlusion);
        this->visitLayer(overSurface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 50, 200), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 50, 200), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInTargetSurface().rects().size());

        this->visitLayer(surface, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 100, 200), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(2u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 100), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->enterContributingSurface(surface, occlusion);

        // Surface and replica are occluded different amounts.
        EXPECT_EQ_RECT(IntRect(40, 0, 60, 100), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 100, 100)));
        EXPECT_EQ_RECT(IntRect(50, 0, 50, 100), occlusion.unoccludedContributingSurfaceContentRect(surface, true, IntRect(0, 0, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestSurfaceAndReplicaOccludedDifferently);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceChildOfSurface : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        // This test verifies that the surface cliprect does not end up empty and clip away the entire unoccluded rect.

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(100, 200));
        typename Types::LayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100), true);
        typename Types::LayerType* surfaceChild = this->createDrawingSurface(surface, this->identityMatrix, FloatPoint(0, 10), IntSize(100, 50), true);
        typename Types::LayerType* topmost = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 50), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(-100, -100, 1000, 1000));

        // |topmost| occludes everything partially so we know occlusion is happening at all.
        this->visitLayer(topmost, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 100, 50), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 50), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        this->visitLayer(surfaceChild, occlusion);

        // surfaceChild increases the occlusion in the screen by a narrow sliver.
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 60), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        // In its own surface, surfaceChild is at 0,0 as is its occlusion.
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 50), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        // The root layer always has a clipRect. So the parent of |surface| has a clipRect. However, the owning layer for |surface| does not
        // mask to bounds, so it doesn't have a clipRect of its own. Thus the parent of |surfaceChild| exercises different code paths
        // as its parent does not have a clipRect.

        this->enterContributingSurface(surfaceChild, occlusion);
        // The surfaceChild's parent does not have a clipRect as it owns a render surface. Make sure the unoccluded rect
        // does not get clipped away inappropriately.
        EXPECT_EQ_RECT(IntRect(0, 40, 100, 10), occlusion.unoccludedContributingSurfaceContentRect(surfaceChild, false, IntRect(0, 0, 100, 50)));
        this->leaveContributingSurface(surfaceChild, occlusion);

        // When the surfaceChild's occlusion is transformed up to its parent, make sure it is not clipped away inappropriately also.
        this->enterLayer(surface, occlusion);
        EXPECT_EQ_RECT(IntRect(0, 0, 100, 60), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 10, 100, 50), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());
        this->leaveLayer(surface, occlusion);

        this->enterContributingSurface(surface, occlusion);
        // The surface's parent does have a clipRect as it is the root layer.
        EXPECT_EQ_RECT(IntRect(0, 50, 100, 50), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestSurfaceChildOfSurface);

template<class Types, bool opaqueLayers>
class CCOcclusionTrackerTestSurfaceChildOfClippingSurface : public CCOcclusionTrackerTest<Types, opaqueLayers> {
protected:
    void runMyTest()
    {
        // This test verifies that the surface cliprect does not end up empty and clip away the entire unoccluded rect.

        typename Types::ContentLayerType* parent = this->createRoot(this->identityMatrix, FloatPoint(0, 0), IntSize(80, 200));
        typename Types::LayerType* surface = this->createDrawingSurface(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100), true);
        typename Types::LayerType* surfaceChild = this->createDrawingSurface(surface, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 100), false);
        typename Types::LayerType* topmost = this->createDrawingLayer(parent, this->identityMatrix, FloatPoint(0, 0), IntSize(100, 50), true);
        this->calcDrawEtc(parent);

        TestCCOcclusionTrackerWithScissor<typename Types::LayerType, typename Types::RenderSurfaceType> occlusion(IntRect(0, 0, 1000, 1000));
        occlusion.setLayerScissorRect(IntRect(0, 0, 1000, 1000));

        // |topmost| occludes everything partially so we know occlusion is happening at all.
        this->enterLayer(topmost, occlusion);
        this->leaveLayer(topmost, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 80, 50), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 80, 50), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInTargetSurface().rects().size());

        // surfaceChild is not opaque and does not occlude, so we have a non-empty unoccluded area on surface.
        this->visitLayer(surfaceChild, occlusion);

        EXPECT_EQ_RECT(IntRect(0, 0, 80, 50), occlusion.occlusionInScreenSpace().bounds());
        EXPECT_EQ(1u, occlusion.occlusionInScreenSpace().rects().size());
        EXPECT_EQ_RECT(IntRect(0, 0, 0, 0), occlusion.occlusionInTargetSurface().bounds());
        EXPECT_EQ(0u, occlusion.occlusionInTargetSurface().rects().size());

        // The root layer always has a clipRect. So the parent of |surface| has a clipRect. However, the owning layer for |surface| does not
        // mask to bounds, so it doesn't have a clipRect of its own. Thus the parent of |surfaceChild| exercises different code paths
        // as its parent does not have a clipRect.

        this->enterContributingSurface(surfaceChild, occlusion);
        // The surfaceChild's parent does not have a clipRect as it owns a render surface.
        EXPECT_EQ_RECT(IntRect(0, 50, 80, 50), occlusion.unoccludedContributingSurfaceContentRect(surfaceChild, false, IntRect(0, 0, 100, 100)));
        this->leaveContributingSurface(surfaceChild, occlusion);

        this->visitLayer(surface, occlusion);
        this->enterContributingSurface(surface, occlusion);
        // The surface's parent does have a clipRect as it is the root layer.
        EXPECT_EQ_RECT(IntRect(0, 50, 80, 50), occlusion.unoccludedContributingSurfaceContentRect(surface, false, IntRect(0, 0, 100, 100)));
    }
};

ALL_CCOCCLUSIONTRACKER_TEST(CCOcclusionTrackerTestSurfaceChildOfClippingSurface);

} // namespace
