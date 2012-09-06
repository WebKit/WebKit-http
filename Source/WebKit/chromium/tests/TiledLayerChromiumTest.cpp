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

#include "TiledLayerChromium.h"

#include "BitmapCanvasLayerTextureUpdater.h"
#include "CCAnimationTestCommon.h"
#include "CCGeometryTestUtils.h"
#include "CCOverdrawMetrics.h"
#include "CCRenderingStats.h"
#include "CCSingleThreadProxy.h" // For DebugScopedSetImplThread
#include "CCTextureUpdateController.h"
#include "CCTiledLayerTestCommon.h"
#include "FakeCCGraphicsContext.h"
#include "FakeCCLayerTreeHostClient.h"
#include "LayerPainterChromium.h"
#include "WebCompositorInitializer.h"
#include <gtest/gtest.h>
#include <public/WebTransformationMatrix.h>

using namespace WebCore;
using namespace WebKitTests;
using namespace WTF;
using WebKit::WebTransformationMatrix;

namespace {

class TestCCOcclusionTracker : public CCOcclusionTracker {
public:
    TestCCOcclusionTracker()
        : CCOcclusionTracker(IntRect(0, 0, 1000, 1000), true)
        , m_layerClipRectInTarget(IntRect(0, 0, 1000, 1000))
    {
        // Pretend we have visited a render surface.
        m_stack.append(StackObject());
    }

    void setOcclusion(const Region& occlusion) { m_stack.last().occlusionInScreen = occlusion; }

protected:
    virtual IntRect layerClipRectInTarget(const LayerChromium* layer) const OVERRIDE { return m_layerClipRectInTarget; }

private:
    IntRect m_layerClipRectInTarget;
};

class TiledLayerChromiumTest : public testing::Test {
public:
    TiledLayerChromiumTest()
        : m_compositorInitializer(0)
        , m_context(WebKit::createFakeCCGraphicsContext())
        , m_textureManager(CCPrioritizedTextureManager::create(60*1024*1024, 1024, CCRenderer::ContentPool))
        , m_occlusion(0)
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        m_resourceProvider = CCResourceProvider::create(m_context.get());
    }

    virtual ~TiledLayerChromiumTest()
    {
        textureManagerClearAllMemory(m_textureManager.get(), m_resourceProvider.get());
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        m_resourceProvider.clear();
    }

    // Helper classes and functions that set the current thread to be the impl thread
    // before doing the action that they wrap.
    class ScopedFakeCCTiledLayerImpl {
    public:
        ScopedFakeCCTiledLayerImpl(int id)
        {
            DebugScopedSetImplThread implThread;
            m_layerImpl = new FakeCCTiledLayerImpl(id);
        }
        ~ScopedFakeCCTiledLayerImpl()
        {
            DebugScopedSetImplThread implThread;
            delete m_layerImpl;
        }
        FakeCCTiledLayerImpl* get()
        {
            return m_layerImpl;
        }
        FakeCCTiledLayerImpl* operator->()
        {
            return m_layerImpl;
        }
    private:
        FakeCCTiledLayerImpl* m_layerImpl;
    };
    void textureManagerClearAllMemory(CCPrioritizedTextureManager* textureManager, CCResourceProvider* resourceProvider)
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManager->clearAllMemory(resourceProvider);
    }
    void updateTextures(int count = 500)
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        CCTextureUpdateController::updateTextures(m_resourceProvider.get(), &m_copier, &m_uploader, &m_queue, count);
    }
    void layerPushPropertiesTo(FakeTiledLayerChromium* layer, FakeCCTiledLayerImpl* layerImpl)
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        layer->pushPropertiesTo(layerImpl);
    }
    void layerUpdate(FakeTiledLayerChromium* layer, TestCCOcclusionTracker* occluded)
    {
        DebugScopedSetMainThread mainThread;
        layer->update(m_queue, occluded, m_stats);
    }

    bool updateAndPush(FakeTiledLayerChromium* layer1,
                       FakeCCTiledLayerImpl* layerImpl1,
                       FakeTiledLayerChromium* layer2 = 0,
                       FakeCCTiledLayerImpl* layerImpl2 = 0)
    {
        // Get textures
        m_textureManager->clearPriorities();
        if (layer1)
            layer1->setTexturePriorities(m_priorityCalculator);
        if (layer2)
            layer2->setTexturePriorities(m_priorityCalculator);     
        m_textureManager->prioritizeTextures();

        // Update content
        if (layer1)
            layer1->update(m_queue, m_occlusion, m_stats);
        if (layer2)
            layer2->update(m_queue, m_occlusion, m_stats);

        bool needsUpdate = false;
        if (layer1)
            needsUpdate |= layer1->needsIdlePaint();
        if (layer2)
            needsUpdate |= layer2->needsIdlePaint();

        // Update textures and push.
        updateTextures();
        if (layer1)
            layerPushPropertiesTo(layer1, layerImpl1);
        if (layer2)
            layerPushPropertiesTo(layer2, layerImpl2);

        return needsUpdate;
    }

public:
    WebKitTests::WebCompositorInitializer m_compositorInitializer;
    OwnPtr<CCGraphicsContext> m_context;
    OwnPtr<CCResourceProvider> m_resourceProvider;
    CCTextureUpdateQueue m_queue;
    CCRenderingStats m_stats;
    FakeTextureCopier m_copier;
    FakeTextureUploader m_uploader;
    CCPriorityCalculator m_priorityCalculator;
    OwnPtr<CCPrioritizedTextureManager> m_textureManager;
    TestCCOcclusionTracker* m_occlusion;
};

TEST_F(TiledLayerChromiumTest, pushDirtyTiles)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // The tile size is 100x100, so this invalidates and then paints two tiles.
    layer->setBounds(IntSize(100, 200));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 200));
    layer->invalidateContentRect(IntRect(0, 0, 100, 200));
    updateAndPush(layer.get(), layerImpl.get());

    // We should have both tiles on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(0, 1));

    // Invalidates both tiles, but then only update one of them.
    layer->setBounds(IntSize(100, 200));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 100));
    layer->invalidateContentRect(IntRect(0, 0, 100, 200));
    updateAndPush(layer.get(), layerImpl.get());

    // We should only have the first tile since the other tile was invalidated but not painted.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_FALSE(layerImpl->hasTileAt(0, 1));
}

TEST_F(TiledLayerChromiumTest, pushOccludedDirtyTiles)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);
    TestCCOcclusionTracker occluded;
    m_occlusion = &occluded;

    // The tile size is 100x100, so this invalidates and then paints two tiles.
    layer->setBounds(IntSize(100, 200));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 200));
    layer->invalidateContentRect(IntRect(0, 0, 100, 200));
    updateAndPush(layer.get(), layerImpl.get());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 20000, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    // We should have both tiles on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(0, 1));

    // Invalidates part of the top tile...
    layer->invalidateContentRect(IntRect(0, 0, 50, 50));
    // ....but the area is occluded.
    occluded.setOcclusion(IntRect(0, 0, 50, 50));
    updateAndPush(layer.get(), layerImpl.get());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 20000 + 2500, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    // We should still have both tiles, as part of the top tile is still unoccluded.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(0, 1));
}

TEST_F(TiledLayerChromiumTest, pushDeletedTiles)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // The tile size is 100x100, so this invalidates and then paints two tiles.
    layer->setBounds(IntSize(100, 200));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 200));
    layer->invalidateContentRect(IntRect(0, 0, 100, 200));
    updateAndPush(layer.get(), layerImpl.get());

    // We should have both tiles on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(0, 1));

    m_textureManager->clearPriorities();
    textureManagerClearAllMemory(m_textureManager.get(), m_resourceProvider.get());
    m_textureManager->setMaxMemoryLimitBytes(4*1024*1024);

    // This should drop the tiles on the impl thread.
    layerPushPropertiesTo(layer.get(), layerImpl.get());

    // We should now have no textures on the impl thread.
    EXPECT_FALSE(layerImpl->hasTileAt(0, 0));
    EXPECT_FALSE(layerImpl->hasTileAt(0, 1));

    // This should recreate and update one of the deleted textures.
    layer->setVisibleContentRect(IntRect(0, 0, 100, 100));
    updateAndPush(layer.get(), layerImpl.get());

    // We should have one tiles on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_FALSE(layerImpl->hasTileAt(0, 1));
}

TEST_F(TiledLayerChromiumTest, pushIdlePaintTiles)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // The tile size is 100x100. Setup 5x5 tiles with one visible tile in the center.
    // This paints 1 visible of the 25 invalid tiles.
    layer->setBounds(IntSize(500, 500));
    layer->setVisibleContentRect(IntRect(200, 200, 100, 100));
    layer->invalidateContentRect(IntRect(0, 0, 500, 500));
    bool needsUpdate = updateAndPush(layer.get(), layerImpl.get());
    // We should need idle-painting for surrounding tiles.
    EXPECT_TRUE(needsUpdate);

    // We should have one tile on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(2, 2));

    // For the next four updates, we should detect we still need idle painting.
    for (int i = 0; i < 4; i++) {
        needsUpdate = updateAndPush(layer.get(), layerImpl.get());
        EXPECT_TRUE(needsUpdate);
    }

    // We should have one tile surrounding the visible tile on all sides, but no other tiles.
    IntRect idlePaintTiles(1, 1, 3, 3);
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++)
            EXPECT_EQ(layerImpl->hasTileAt(i, j), idlePaintTiles.contains(i, j));
    }

    // We should always finish painting eventually.
    for (int i = 0; i < 20; i++)
        needsUpdate = updateAndPush(layer.get(), layerImpl.get());
    EXPECT_FALSE(needsUpdate);
}

TEST_F(TiledLayerChromiumTest, pushTilesAfterIdlePaintFailed)
{
    // Start with 2mb of memory, but the test is going to try to use just more than 1mb, so we reduce to 1mb later.
    m_textureManager->setMaxMemoryLimitBytes(2 * 1024 * 1024);    
    RefPtr<FakeTiledLayerChromium> layer1 = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl1(1);
    RefPtr<FakeTiledLayerChromium> layer2 = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl2(2);

    // For this test we have two layers. layer1 exhausts most texture memory, leaving room for 2 more tiles from
    // layer2, but not all three tiles. First we paint layer1, and one tile from layer2. Then when we idle paint
    // layer2, we will fail on the third tile of layer2, and this should not leave the second tile in a bad state.

    // This uses 960000 bytes, leaving 88576 bytes of memory left, which is enough for 2 tiles only in the other layer.
    IntRect layer1Rect(0, 0, 100, 2400);
    
    // This requires 4*30000 bytes of memory.
    IntRect layer2Rect(0, 0, 100, 300);

    // Paint a single tile in layer2 so that it will idle paint.
    layer1->setBounds(layer1Rect.size());
    layer1->setVisibleContentRect(layer1Rect);
    layer2->setBounds(layer2Rect.size());
    layer2->setVisibleContentRect(IntRect(0, 0, 100, 100));
    bool needsUpdate = updateAndPush(layer1.get(), layerImpl1.get(),
                                     layer2.get(), layerImpl2.get());
    // We should need idle-painting for both remaining tiles in layer2.
    EXPECT_TRUE(needsUpdate);

    // Reduce our memory limits to 1mb.
    m_textureManager->setMaxMemoryLimitBytes(1024 * 1024);

    // Now idle paint layer2. We are going to run out of memory though!
    // Oh well, commit the frame and push.
    for (int i = 0; i < 4; i++) {
        needsUpdate = updateAndPush(layer1.get(), layerImpl1.get(),
                                    layer2.get(), layerImpl2.get());
    }

    // Sanity check, we should have textures for the big layer.
    EXPECT_TRUE(layerImpl1->hasTextureIdForTileAt(0, 0));
    EXPECT_TRUE(layerImpl1->hasTextureIdForTileAt(0, 23));

    // We should only have the first two tiles from layer2 since
    // it failed to idle update the last tile.
    EXPECT_TRUE(layerImpl2->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl2->hasTextureIdForTileAt(0, 0));
    EXPECT_TRUE(layerImpl2->hasTileAt(0, 1));
    EXPECT_TRUE(layerImpl2->hasTextureIdForTileAt(0, 1));
    
    EXPECT_FALSE(needsUpdate);
    EXPECT_FALSE(layerImpl2->hasTileAt(0, 2));
}

TEST_F(TiledLayerChromiumTest, pushIdlePaintedOccludedTiles)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);
    TestCCOcclusionTracker occluded;
    m_occlusion = &occluded;
    
    // The tile size is 100x100, so this invalidates one occluded tile, culls it during paint, but prepaints it.
    occluded.setOcclusion(IntRect(0, 0, 100, 100));

    layer->setBounds(IntSize(100, 100));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 100));
    updateAndPush(layer.get(), layerImpl.get());

    // We should have the prepainted tile on the impl side, but culled it during paint.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_EQ(1, occluded.overdrawMetrics().tilesCulledForUpload());
}

TEST_F(TiledLayerChromiumTest, pushTilesMarkedDirtyDuringPaint)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // The tile size is 100x100, so this invalidates and then paints two tiles.
    // However, during the paint, we invalidate one of the tiles. This should
    // not prevent the tile from being pushed.
    layer->fakeLayerTextureUpdater()->setRectToInvalidate(IntRect(0, 50, 100, 50), layer.get());
    layer->setBounds(IntSize(100, 200));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 200));    
    updateAndPush(layer.get(), layerImpl.get());

    // We should have both tiles on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(0, 1));
}

TEST_F(TiledLayerChromiumTest, pushTilesLayerMarkedDirtyDuringPaintOnNextLayer)
{
    RefPtr<FakeTiledLayerChromium> layer1 = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    RefPtr<FakeTiledLayerChromium> layer2 = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layer1Impl(1);
    ScopedFakeCCTiledLayerImpl layer2Impl(2);

    // Invalidate a tile on layer1, during update of layer 2.
    layer2->fakeLayerTextureUpdater()->setRectToInvalidate(IntRect(0, 50, 100, 50), layer1.get());
    layer1->setBounds(IntSize(100, 200));
    layer1->setVisibleContentRect(IntRect(0, 0, 100, 200));    
    layer2->setBounds(IntSize(100, 200));
    layer2->setVisibleContentRect(IntRect(0, 0, 100, 200));    
    updateAndPush(layer1.get(), layer1Impl.get(),
                  layer2.get(), layer2Impl.get());

    // We should have both tiles on the impl side for all layers.
    EXPECT_TRUE(layer1Impl->hasTileAt(0, 0));
    EXPECT_TRUE(layer1Impl->hasTileAt(0, 1));
    EXPECT_TRUE(layer2Impl->hasTileAt(0, 0));
    EXPECT_TRUE(layer2Impl->hasTileAt(0, 1));
}

TEST_F(TiledLayerChromiumTest, pushTilesLayerMarkedDirtyDuringPaintOnPreviousLayer)
{
    RefPtr<FakeTiledLayerChromium> layer1 = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    RefPtr<FakeTiledLayerChromium> layer2 = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layer1Impl(1);
    ScopedFakeCCTiledLayerImpl layer2Impl(2);

    layer1->fakeLayerTextureUpdater()->setRectToInvalidate(IntRect(0, 50, 100, 50), layer2.get());
    layer1->setBounds(IntSize(100, 200));
    layer1->setVisibleContentRect(IntRect(0, 0, 100, 200));    
    layer2->setBounds(IntSize(100, 200));
    layer2->setVisibleContentRect(IntRect(0, 0, 100, 200));    
    updateAndPush(layer1.get(), layer1Impl.get(),
                  layer2.get(), layer2Impl.get());

    // We should have both tiles on the impl side for all layers.
    EXPECT_TRUE(layer1Impl->hasTileAt(0, 0));
    EXPECT_TRUE(layer1Impl->hasTileAt(0, 1));
    EXPECT_TRUE(layer2Impl->hasTileAt(0, 0));
    EXPECT_TRUE(layer2Impl->hasTileAt(0, 1));
}

TEST_F(TiledLayerChromiumTest, paintSmallAnimatedLayersImmediately)
{
    // Create a CCLayerTreeHost that has the right viewportsize,
    // so the layer is considered small enough.
    FakeCCLayerTreeHostClient fakeCCLayerTreeHostClient;
    OwnPtr<CCLayerTreeHost> ccLayerTreeHost = CCLayerTreeHost::create(&fakeCCLayerTreeHostClient, CCLayerTreeSettings());

    bool runOutOfMemory[2] = {false, true};
    for (int i = 0; i < 2; i++) {
        // Create a layer with 4x4 tiles.
        int layerWidth  = 4 * FakeTiledLayerChromium::tileSize().width();
        int layerHeight = 4 * FakeTiledLayerChromium::tileSize().height();
        int memoryForLayer = layerWidth * layerHeight * 4;
        IntSize viewportSize = IntSize(layerWidth, layerHeight);
        ccLayerTreeHost->setViewportSize(viewportSize, viewportSize);

        // Use 8x4 tiles to run out of memory.
        if (runOutOfMemory[i])
            layerWidth *= 2;

        m_textureManager->setMaxMemoryLimitBytes(memoryForLayer);

        RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
        ScopedFakeCCTiledLayerImpl layerImpl(1);

        // Full size layer with half being visible.
        IntSize contentBounds(layerWidth, layerHeight);
        IntRect contentRect(IntPoint::zero(), contentBounds);
        IntRect visibleRect(IntPoint::zero(), IntSize(layerWidth / 2, layerHeight));

        // Pretend the layer is animating.
        layer->setDrawTransformIsAnimating(true);
        layer->setBounds(contentBounds);
        layer->setVisibleContentRect(visibleRect);
        layer->invalidateContentRect(contentRect);
        layer->setLayerTreeHost(ccLayerTreeHost.get());

        // The layer should paint it's entire contents on the first paint
        // if it is close to the viewport size and has the available memory.
        layer->setTexturePriorities(m_priorityCalculator);
        m_textureManager->prioritizeTextures();
        layer->update(m_queue, 0, m_stats);
        updateTextures();
        layerPushPropertiesTo(layer.get(), layerImpl.get());

        // We should have all the tiles for the small animated layer.
        // We should still have the visible tiles when we didn't
        // have enough memory for all the tiles.
        if (!runOutOfMemory[i]) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j)
                    EXPECT_TRUE(layerImpl->hasTileAt(i, j));
            }
        } else {
            for (int i = 0; i < 8; ++i) {
                for (int j = 0; j < 4; ++j)
                    EXPECT_EQ(layerImpl->hasTileAt(i, j), i < 4);
            }
        }
    }
    ccLayerTreeHost.clear();
}

TEST_F(TiledLayerChromiumTest, idlePaintOutOfMemory)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // We have enough memory for only the visible rect, so we will run out of memory in first idle paint.
    int memoryLimit = 4 * 100 * 100; // 1 tiles, 4 bytes per pixel.
    m_textureManager->setMaxMemoryLimitBytes(memoryLimit);

    // The tile size is 100x100, so this invalidates and then paints two tiles.
    bool needsUpdate = false;
    layer->setBounds(IntSize(300, 300));
    layer->setVisibleContentRect(IntRect(100, 100, 100, 100));
    for (int i = 0; i < 2; i++)
        needsUpdate = updateAndPush(layer.get(), layerImpl.get());

    // Idle-painting should see no more priority tiles for painting.
    EXPECT_FALSE(needsUpdate);

    // We should have one tile on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(1, 1));
}

TEST_F(TiledLayerChromiumTest, idlePaintZeroSizedLayer)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    bool animating[2] = {false, true};
    for (int i = 0; i < 2; i++) {
        // Pretend the layer is animating.
        layer->setDrawTransformIsAnimating(animating[i]);

        // The layer's bounds are empty.
        // Empty layers don't paint or idle-paint.
        layer->setBounds(IntSize());
        layer->setVisibleContentRect(IntRect());
        bool needsUpdate = updateAndPush(layer.get(), layerImpl.get());
        
        // Empty layers don't have tiles.
        EXPECT_EQ(0u, layer->numPaintedTiles());

        // Empty layers don't need prepaint.
        EXPECT_FALSE(needsUpdate);

        // Empty layers don't have tiles.
        EXPECT_FALSE(layerImpl->hasTileAt(0, 0));
    }
}

TEST_F(TiledLayerChromiumTest, idlePaintNonVisibleLayers)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // Alternate between not visible and visible.
    IntRect v(0, 0, 100, 100);
    IntRect nv(0, 0, 0, 0);
    IntRect visibleRect[10] = {nv, nv, v, v, nv, nv, v, v, nv, nv};
    bool invalidate[10] =  {true, true, true, true, true, true, true, true, false, false };

    // We should not have any tiles except for when the layer was visible
    // or after the layer was visible and we didn't invalidate.
    bool haveTile[10] = { false, false, true, true, false, false, true, true, true, true };
    
    for (int i = 0; i < 10; i++) {
        layer->setBounds(IntSize(100, 100));
        layer->setVisibleContentRect(visibleRect[i]);

        if (invalidate[i])
            layer->invalidateContentRect(IntRect(0, 0, 100, 100));
        bool needsUpdate = updateAndPush(layer.get(), layerImpl.get());
        
        // We should never signal idle paint, as we painted the entire layer
        // or the layer was not visible.
        EXPECT_FALSE(needsUpdate);
        EXPECT_EQ(layerImpl->hasTileAt(0, 0), haveTile[i]);
    }
}

TEST_F(TiledLayerChromiumTest, invalidateFromPrepare)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // The tile size is 100x100, so this invalidates and then paints two tiles.
    layer->setBounds(IntSize(100, 200));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 200));
    updateAndPush(layer.get(), layerImpl.get());

    // We should have both tiles on the impl side.
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(0, 1));

    layer->fakeLayerTextureUpdater()->clearPrepareCount();
    // Invoke update again. As the layer is valid update shouldn't be invoked on
    // the LayerTextureUpdater.
    updateAndPush(layer.get(), layerImpl.get());
    EXPECT_EQ(0, layer->fakeLayerTextureUpdater()->prepareCount());

    // setRectToInvalidate triggers invalidateContentRect() being invoked from update.
    layer->fakeLayerTextureUpdater()->setRectToInvalidate(IntRect(25, 25, 50, 50), layer.get());
    layer->fakeLayerTextureUpdater()->clearPrepareCount();
    layer->invalidateContentRect(IntRect(0, 0, 50, 50));
    updateAndPush(layer.get(), layerImpl.get());
    EXPECT_EQ(1, layer->fakeLayerTextureUpdater()->prepareCount());
    layer->fakeLayerTextureUpdater()->clearPrepareCount();

    // The layer should still be invalid as update invoked invalidate.
    updateAndPush(layer.get(), layerImpl.get()); // visible
    EXPECT_EQ(1, layer->fakeLayerTextureUpdater()->prepareCount());
}

TEST_F(TiledLayerChromiumTest, verifyUpdateRectWhenContentBoundsAreScaled)
{
    // The updateRect (that indicates what was actually painted) should be in
    // layer space, not the content space.
    RefPtr<FakeTiledLayerWithScaledBounds> layer = adoptRef(new FakeTiledLayerWithScaledBounds(m_textureManager.get()));

    IntRect layerBounds(0, 0, 300, 200);
    IntRect contentBounds(0, 0, 200, 250);

    layer->setBounds(layerBounds.size());
    layer->setContentBounds(contentBounds.size());
    layer->setVisibleContentRect(contentBounds);

    // On first update, the updateRect includes all tiles, even beyond the boundaries of the layer.
    // However, it should still be in layer space, not content space.
    layer->invalidateContentRect(contentBounds);

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, 0, m_stats);
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 300, 300 * 0.8), layer->updateRect());
    updateTextures();

    // After the tiles are updated once, another invalidate only needs to update the bounds of the layer.
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->invalidateContentRect(contentBounds);
    layer->update(m_queue, 0, m_stats);
    EXPECT_FLOAT_RECT_EQ(FloatRect(layerBounds), layer->updateRect());
    updateTextures();

    // Partial re-paint should also be represented by the updateRect in layer space, not content space.
    IntRect partialDamage(30, 100, 10, 10);
    layer->invalidateContentRect(partialDamage);
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, 0, m_stats);
    EXPECT_FLOAT_RECT_EQ(FloatRect(45, 80, 15, 8), layer->updateRect());
}

TEST_F(TiledLayerChromiumTest, verifyInvalidationWhenContentsScaleChanges)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    ScopedFakeCCTiledLayerImpl layerImpl(1);

    // Create a layer with one tile.
    layer->setBounds(IntSize(100, 100));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 100));

    // Invalidate the entire layer.
    layer->setNeedsDisplay();
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 100, 100), layer->lastNeedsDisplayRect());

    // Push the tiles to the impl side and check that there is exactly one.
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, 0, m_stats);
    updateTextures();
    layerPushPropertiesTo(layer.get(), layerImpl.get());
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_FALSE(layerImpl->hasTileAt(0, 1));
    EXPECT_FALSE(layerImpl->hasTileAt(1, 0));
    EXPECT_FALSE(layerImpl->hasTileAt(1, 1));

    // Change the contents scale and verify that the content rectangle requiring painting
    // is not scaled.
    layer->setContentsScale(2);
    layer->setVisibleContentRect(IntRect(0, 0, 200, 200));
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 100, 100), layer->lastNeedsDisplayRect());

    // The impl side should get 2x2 tiles now.
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, 0, m_stats);
    updateTextures();
    layerPushPropertiesTo(layer.get(), layerImpl.get());
    EXPECT_TRUE(layerImpl->hasTileAt(0, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(0, 1));
    EXPECT_TRUE(layerImpl->hasTileAt(1, 0));
    EXPECT_TRUE(layerImpl->hasTileAt(1, 1));

    // Invalidate the entire layer again, but do not paint. All tiles should be gone now from the
    // impl side.
    layer->setNeedsDisplay();
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    layerPushPropertiesTo(layer.get(), layerImpl.get());
    EXPECT_FALSE(layerImpl->hasTileAt(0, 0));
    EXPECT_FALSE(layerImpl->hasTileAt(0, 1));
    EXPECT_FALSE(layerImpl->hasTileAt(1, 0));
    EXPECT_FALSE(layerImpl->hasTileAt(1, 1));
}

TEST_F(TiledLayerChromiumTest, skipsDrawGetsReset)
{
    FakeCCLayerTreeHostClient fakeCCLayerTreeHostClient;
    OwnPtr<CCLayerTreeHost> ccLayerTreeHost = CCLayerTreeHost::create(&fakeCCLayerTreeHostClient, CCLayerTreeSettings());
    ASSERT_TRUE(ccLayerTreeHost->initializeRendererIfNeeded());

    // Create two 300 x 300 tiled layers.
    IntSize contentBounds(300, 300);
    IntRect contentRect(IntPoint::zero(), contentBounds);

    // We have enough memory for only one of the two layers.
    int memoryLimit = 4 * 300 * 300; // 4 bytes per pixel.

    RefPtr<FakeTiledLayerChromium> rootLayer = adoptRef(new FakeTiledLayerChromium(ccLayerTreeHost->contentsTextureManager()));
    RefPtr<FakeTiledLayerChromium> childLayer = adoptRef(new FakeTiledLayerChromium(ccLayerTreeHost->contentsTextureManager()));
    rootLayer->addChild(childLayer);

    rootLayer->setBounds(contentBounds);
    rootLayer->setVisibleContentRect(contentRect);
    rootLayer->setPosition(FloatPoint(0, 0));
    childLayer->setBounds(contentBounds);
    childLayer->setVisibleContentRect(contentRect);
    childLayer->setPosition(FloatPoint(0, 0));
    rootLayer->invalidateContentRect(contentRect);
    childLayer->invalidateContentRect(contentRect);

    ccLayerTreeHost->setRootLayer(rootLayer);
    ccLayerTreeHost->setViewportSize(IntSize(300, 300), IntSize(300, 300));

    ccLayerTreeHost->updateLayers(m_queue, memoryLimit);

    // We'll skip the root layer.
    EXPECT_TRUE(rootLayer->skipsDraw());
    EXPECT_FALSE(childLayer->skipsDraw());

    ccLayerTreeHost->commitComplete();

    // Remove the child layer.
    rootLayer->removeAllChildren();

    ccLayerTreeHost->updateLayers(m_queue, memoryLimit);
    EXPECT_FALSE(rootLayer->skipsDraw());

    textureManagerClearAllMemory(ccLayerTreeHost->contentsTextureManager(), m_resourceProvider.get());
    ccLayerTreeHost->setRootLayer(0);
    ccLayerTreeHost.clear();
}

TEST_F(TiledLayerChromiumTest, resizeToSmaller)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));

    layer->setBounds(IntSize(700, 700));
    layer->setVisibleContentRect(IntRect(0, 0, 700, 700));
    layer->invalidateContentRect(IntRect(0, 0, 700, 700));

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, 0, m_stats);

    layer->setBounds(IntSize(200, 200));
    layer->invalidateContentRect(IntRect(0, 0, 200, 200));
}

TEST_F(TiledLayerChromiumTest, hugeLayerUpdateCrash)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));

    int size = 1 << 30;
    layer->setBounds(IntSize(size, size));
    layer->setVisibleContentRect(IntRect(0, 0, 700, 700));
    layer->invalidateContentRect(IntRect(0, 0, size, size));

    // Ensure no crash for bounds where size * size would overflow an int.
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, 0, m_stats);
}

TEST_F(TiledLayerChromiumTest, partialUpdates)
{
    CCLayerTreeSettings settings;
    settings.maxPartialTextureUpdates = 4;

    FakeCCLayerTreeHostClient fakeCCLayerTreeHostClient;
    OwnPtr<CCLayerTreeHost> ccLayerTreeHost = CCLayerTreeHost::create(&fakeCCLayerTreeHostClient, settings);
    ASSERT_TRUE(ccLayerTreeHost->initializeRendererIfNeeded());

    // Create one 300 x 200 tiled layer with 3 x 2 tiles.
    IntSize contentBounds(300, 200);
    IntRect contentRect(IntPoint::zero(), contentBounds);

    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(ccLayerTreeHost->contentsTextureManager()));
    layer->setBounds(contentBounds);
    layer->setPosition(FloatPoint(0, 0));
    layer->setVisibleContentRect(contentRect);
    layer->invalidateContentRect(contentRect);

    ccLayerTreeHost->setRootLayer(layer);
    ccLayerTreeHost->setViewportSize(IntSize(300, 200), IntSize(300, 200));

    // Full update of all 6 tiles.
    ccLayerTreeHost->updateLayers(m_queue, std::numeric_limits<size_t>::max());
    {
        ScopedFakeCCTiledLayerImpl layerImpl(1);
        updateTextures(4);
        EXPECT_EQ(4, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_TRUE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        updateTextures(4);
        EXPECT_EQ(2, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        layerPushPropertiesTo(layer.get(), layerImpl.get());
    }
    ccLayerTreeHost->commitComplete();

    // Full update of 3 tiles and partial update of 3 tiles.
    layer->invalidateContentRect(IntRect(0, 0, 300, 150));
    ccLayerTreeHost->updateLayers(m_queue, std::numeric_limits<size_t>::max());
    {
        ScopedFakeCCTiledLayerImpl layerImpl(1);
        updateTextures(4);
        EXPECT_EQ(3, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_TRUE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        updateTextures(4);
        EXPECT_EQ(3, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        layerPushPropertiesTo(layer.get(), layerImpl.get());
    }
    ccLayerTreeHost->commitComplete();

    // Partial update of 6 tiles.
    layer->invalidateContentRect(IntRect(50, 50, 200, 100));
    {
        ScopedFakeCCTiledLayerImpl layerImpl(1);
        ccLayerTreeHost->updateLayers(m_queue, std::numeric_limits<size_t>::max());
        updateTextures(4);
        EXPECT_EQ(2, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_TRUE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        updateTextures(4);
        EXPECT_EQ(4, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        layerPushPropertiesTo(layer.get(), layerImpl.get());
    }
    ccLayerTreeHost->commitComplete();

    // Checkerboard all tiles.
    layer->invalidateContentRect(IntRect(0, 0, 300, 200));
    {
        ScopedFakeCCTiledLayerImpl layerImpl(1);
        layerPushPropertiesTo(layer.get(), layerImpl.get());
    }
    ccLayerTreeHost->commitComplete();

    // Partial update of 6 checkerboard tiles.
    layer->invalidateContentRect(IntRect(50, 50, 200, 100));
    {
        ScopedFakeCCTiledLayerImpl layerImpl(1);
        ccLayerTreeHost->updateLayers(m_queue, std::numeric_limits<size_t>::max());
        updateTextures(4);
        EXPECT_EQ(4, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_TRUE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        updateTextures(4);
        EXPECT_EQ(2, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        layerPushPropertiesTo(layer.get(), layerImpl.get());
    }
    ccLayerTreeHost->commitComplete();

    // Partial update of 4 tiles.
    layer->invalidateContentRect(IntRect(50, 50, 100, 100));
    {
        ScopedFakeCCTiledLayerImpl layerImpl(1);
        ccLayerTreeHost->updateLayers(m_queue, std::numeric_limits<size_t>::max());
        updateTextures(4);
        EXPECT_EQ(4, layer->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());
        layer->fakeLayerTextureUpdater()->clearUpdateCount();
        layerPushPropertiesTo(layer.get(), layerImpl.get());
    }
    ccLayerTreeHost->commitComplete();

    textureManagerClearAllMemory(ccLayerTreeHost->contentsTextureManager(), m_resourceProvider.get());
    ccLayerTreeHost->setRootLayer(0);
    ccLayerTreeHost.clear();
}

TEST_F(TiledLayerChromiumTest, tilesPaintedWithoutOcclusion)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));

    // The tile size is 100x100, so this invalidates and then paints two tiles.
    layer->setBounds(IntSize(100, 200));
    layer->setDrawableContentRect(IntRect(0, 0, 100, 200));
    layer->setVisibleContentRect(IntRect(0, 0, 100, 200));
    layer->invalidateContentRect(IntRect(0, 0, 100, 200));

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, 0, m_stats);
    EXPECT_EQ(2, layer->fakeLayerTextureUpdater()->prepareRectCount());
}

TEST_F(TiledLayerChromiumTest, tilesPaintedWithOcclusion)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    TestCCOcclusionTracker occluded;

    // The tile size is 100x100.

    layer->setBounds(IntSize(600, 600));

    occluded.setOcclusion(IntRect(200, 200, 300, 100));
    layer->setDrawableContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->setVisibleContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(36-3, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 330000, 1);
    EXPECT_EQ(3, occluded.overdrawMetrics().tilesCulledForUpload());

    layer->fakeLayerTextureUpdater()->clearPrepareRectCount();
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    occluded.setOcclusion(IntRect(250, 200, 300, 100));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(36-2, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 330000 + 340000, 1);
    EXPECT_EQ(3 + 2, occluded.overdrawMetrics().tilesCulledForUpload());

    layer->fakeLayerTextureUpdater()->clearPrepareRectCount();
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    occluded.setOcclusion(IntRect(250, 250, 300, 100));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(36, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 330000 + 340000 + 360000, 1);
    EXPECT_EQ(3 + 2, occluded.overdrawMetrics().tilesCulledForUpload());
}

TEST_F(TiledLayerChromiumTest, tilesPaintedWithOcclusionAndVisiblityConstraints)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    TestCCOcclusionTracker occluded;

    // The tile size is 100x100.

    layer->setBounds(IntSize(600, 600));

    // The partially occluded tiles (by the 150 occlusion height) are visible beyond the occlusion, so not culled.
    occluded.setOcclusion(IntRect(200, 200, 300, 150));
    layer->setDrawableContentRect(IntRect(0, 0, 600, 360));
    layer->setVisibleContentRect(IntRect(0, 0, 600, 360));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(24-3, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 210000, 1);
    EXPECT_EQ(3, occluded.overdrawMetrics().tilesCulledForUpload());

    layer->fakeLayerTextureUpdater()->clearPrepareRectCount();

    // Now the visible region stops at the edge of the occlusion so the partly visible tiles become fully occluded.
    occluded.setOcclusion(IntRect(200, 200, 300, 150));
    layer->setDrawableContentRect(IntRect(0, 0, 600, 350));
    layer->setVisibleContentRect(IntRect(0, 0, 600, 350));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(24-6, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 210000 + 180000, 1);
    EXPECT_EQ(3 + 6, occluded.overdrawMetrics().tilesCulledForUpload());

    layer->fakeLayerTextureUpdater()->clearPrepareRectCount();

    // Now the visible region is even smaller than the occlusion, it should have the same result.
    occluded.setOcclusion(IntRect(200, 200, 300, 150));
    layer->setDrawableContentRect(IntRect(0, 0, 600, 340));
    layer->setVisibleContentRect(IntRect(0, 0, 600, 340));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(24-6, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 210000 + 180000 + 180000, 1);
    EXPECT_EQ(3 + 6 + 6, occluded.overdrawMetrics().tilesCulledForUpload());

}

TEST_F(TiledLayerChromiumTest, tilesNotPaintedWithoutInvalidation)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    TestCCOcclusionTracker occluded;

    // The tile size is 100x100.

    layer->setBounds(IntSize(600, 600));

    occluded.setOcclusion(IntRect(200, 200, 300, 100));
    layer->setDrawableContentRect(IntRect(0, 0, 600, 600));
    layer->setVisibleContentRect(IntRect(0, 0, 600, 600));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(36-3, layer->fakeLayerTextureUpdater()->prepareRectCount());
    {
        updateTextures();
    }

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 330000, 1);
    EXPECT_EQ(3, occluded.overdrawMetrics().tilesCulledForUpload());

    layer->fakeLayerTextureUpdater()->clearPrepareRectCount();
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    // Repaint without marking it dirty. The 3 culled tiles will be pre-painted now.
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(3, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 330000, 1);
    EXPECT_EQ(6, occluded.overdrawMetrics().tilesCulledForUpload());
}

TEST_F(TiledLayerChromiumTest, tilesPaintedWithOcclusionAndTransforms)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    TestCCOcclusionTracker occluded;

    // The tile size is 100x100.

    // This makes sure the painting works when the occluded region (in screen space)
    // is transformed differently than the layer.
    layer->setBounds(IntSize(600, 600));
    WebTransformationMatrix screenTransform;
    screenTransform.scale(0.5);
    layer->setScreenSpaceTransform(screenTransform);
    layer->setDrawTransform(screenTransform);

    occluded.setOcclusion(IntRect(100, 100, 150, 50));
    layer->setDrawableContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->setVisibleContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(36-3, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 330000, 1);
    EXPECT_EQ(3, occluded.overdrawMetrics().tilesCulledForUpload());
}

TEST_F(TiledLayerChromiumTest, tilesPaintedWithOcclusionAndScaling)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    TestCCOcclusionTracker occluded;

    // The tile size is 100x100.

    // This makes sure the painting works when the content space is scaled to
    // a different layer space. In this case tiles are scaled to be 200x200
    // pixels, which means none should be occluded.
    layer->setContentsScale(0.5);
    layer->setBounds(IntSize(600, 600));
    WebTransformationMatrix drawTransform;
    drawTransform.scale(1 / layer->contentsScale());
    layer->setDrawTransform(drawTransform);
    layer->setScreenSpaceTransform(drawTransform);

    occluded.setOcclusion(IntRect(200, 200, 300, 100));
    layer->setDrawableContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->setVisibleContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    // The content is half the size of the layer (so the number of tiles is fewer).
    // In this case, the content is 300x300, and since the tile size is 100, the
    // number of tiles 3x3.
    EXPECT_EQ(9, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 90000, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    layer->fakeLayerTextureUpdater()->clearPrepareRectCount();

    // This makes sure the painting works when the content space is scaled to
    // a different layer space. In this case the occluded region catches the
    // blown up tiles.
    occluded.setOcclusion(IntRect(200, 200, 300, 200));
    layer->setDrawableContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->setVisibleContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(9-1, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 90000 + 80000, 1);
    EXPECT_EQ(1, occluded.overdrawMetrics().tilesCulledForUpload());

    layer->fakeLayerTextureUpdater()->clearPrepareRectCount();

    // This makes sure content scaling and transforms work together.
    WebTransformationMatrix screenTransform;
    screenTransform.scale(0.5);
    layer->setScreenSpaceTransform(screenTransform);
    layer->setDrawTransform(screenTransform);

    occluded.setOcclusion(IntRect(100, 100, 150, 100));
    layer->setDrawableContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->setVisibleContentRect(IntRect(IntPoint(), layer->contentBounds()));
    layer->invalidateContentRect(IntRect(0, 0, 600, 600));
    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();
    layer->update(m_queue, &occluded, m_stats);
    EXPECT_EQ(9-1, layer->fakeLayerTextureUpdater()->prepareRectCount());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 90000 + 80000 + 80000, 1);
    EXPECT_EQ(1 + 1, occluded.overdrawMetrics().tilesCulledForUpload());
}

TEST_F(TiledLayerChromiumTest, visibleContentOpaqueRegion)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    TestCCOcclusionTracker occluded;

    // The tile size is 100x100, so this invalidates and then paints two tiles in various ways.

    IntRect opaquePaintRect;
    Region opaqueContents;

    IntRect contentBounds = IntRect(0, 0, 100, 200);
    IntRect visibleBounds = IntRect(0, 0, 100, 150);

    layer->setBounds(contentBounds.size());
    layer->setDrawableContentRect(visibleBounds);
    layer->setVisibleContentRect(visibleBounds);
    layer->setDrawOpacity(1);

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    // If the layer doesn't paint opaque content, then the visibleContentOpaqueRegion should be empty.
    layer->fakeLayerTextureUpdater()->setOpaquePaintRect(IntRect());
    layer->invalidateContentRect(contentBounds);
    layer->update(m_queue, &occluded, m_stats);
    opaqueContents = layer->visibleContentOpaqueRegion();
    EXPECT_TRUE(opaqueContents.isEmpty());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsPainted(), 20000, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 20000, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    // visibleContentOpaqueRegion should match the visible part of what is painted opaque.
    opaquePaintRect = IntRect(10, 10, 90, 190);
    layer->fakeLayerTextureUpdater()->setOpaquePaintRect(opaquePaintRect);
    layer->invalidateContentRect(contentBounds);
    layer->update(m_queue, &occluded, m_stats);
    updateTextures();
    opaqueContents = layer->visibleContentOpaqueRegion();
    EXPECT_RECT_EQ(intersection(opaquePaintRect, visibleBounds), opaqueContents.bounds());
    EXPECT_EQ(1u, opaqueContents.rects().size());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsPainted(), 20000 * 2, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 17100, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 20000 + 20000 - 17100, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    // If we paint again without invalidating, the same stuff should be opaque.
    layer->fakeLayerTextureUpdater()->setOpaquePaintRect(IntRect());
    layer->update(m_queue, &occluded, m_stats);
    updateTextures();
    opaqueContents = layer->visibleContentOpaqueRegion();
    EXPECT_RECT_EQ(intersection(opaquePaintRect, visibleBounds), opaqueContents.bounds());
    EXPECT_EQ(1u, opaqueContents.rects().size());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsPainted(), 20000 * 2, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 17100, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 20000 + 20000 - 17100, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    // If we repaint a non-opaque part of the tile, then it shouldn't lose its opaque-ness. And other tiles should
    // not be affected.
    layer->fakeLayerTextureUpdater()->setOpaquePaintRect(IntRect());
    layer->invalidateContentRect(IntRect(0, 0, 1, 1));
    layer->update(m_queue, &occluded, m_stats);
    updateTextures();
    opaqueContents = layer->visibleContentOpaqueRegion();
    EXPECT_RECT_EQ(intersection(opaquePaintRect, visibleBounds), opaqueContents.bounds());
    EXPECT_EQ(1u, opaqueContents.rects().size());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsPainted(), 20000 * 2 + 1, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 17100, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 20000 + 20000 - 17100 + 1, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    // If we repaint an opaque part of the tile, then it should lose its opaque-ness. But other tiles should still
    // not be affected.
    layer->fakeLayerTextureUpdater()->setOpaquePaintRect(IntRect());
    layer->invalidateContentRect(IntRect(10, 10, 1, 1));
    layer->update(m_queue, &occluded, m_stats);
    updateTextures();
    opaqueContents = layer->visibleContentOpaqueRegion();
    EXPECT_RECT_EQ(intersection(IntRect(10, 100, 90, 100), visibleBounds), opaqueContents.bounds());
    EXPECT_EQ(1u, opaqueContents.rects().size());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsPainted(), 20000 * 2 + 1  + 1, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 17100, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 20000 + 20000 - 17100 + 1 + 1, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());
}

TEST_F(TiledLayerChromiumTest, pixelsPaintedMetrics)
{
    RefPtr<FakeTiledLayerChromium> layer = adoptRef(new FakeTiledLayerChromium(m_textureManager.get()));
    TestCCOcclusionTracker occluded;

    // The tile size is 100x100, so this invalidates and then paints two tiles in various ways.

    IntRect opaquePaintRect;
    Region opaqueContents;

    IntRect contentBounds = IntRect(0, 0, 100, 300);
    IntRect visibleBounds = IntRect(0, 0, 100, 300);

    layer->setBounds(contentBounds.size());
    layer->setDrawableContentRect(visibleBounds);
    layer->setVisibleContentRect(visibleBounds);
    layer->setDrawOpacity(1);

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    // Invalidates and paints the whole layer.
    layer->fakeLayerTextureUpdater()->setOpaquePaintRect(IntRect());
    layer->invalidateContentRect(contentBounds);
    layer->update(m_queue, &occluded, m_stats);
    updateTextures();
    opaqueContents = layer->visibleContentOpaqueRegion();
    EXPECT_TRUE(opaqueContents.isEmpty());

    EXPECT_NEAR(occluded.overdrawMetrics().pixelsPainted(), 30000, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 30000, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());

    // Invalidates an area on the top and bottom tile, which will cause us to paint the tile in the middle,
    // even though it is not dirty and will not be uploaded.
    layer->fakeLayerTextureUpdater()->setOpaquePaintRect(IntRect());
    layer->invalidateContentRect(IntRect(0, 0, 1, 1));
    layer->invalidateContentRect(IntRect(50, 200, 10, 10));
    layer->update(m_queue, &occluded, m_stats);
    updateTextures();
    opaqueContents = layer->visibleContentOpaqueRegion();
    EXPECT_TRUE(opaqueContents.isEmpty());

    // The middle tile was painted even though not invalidated.
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsPainted(), 30000 + 60 * 210, 1);
    // The pixels uploaded will not include the non-invalidated tile in the middle.
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedOpaque(), 0, 1);
    EXPECT_NEAR(occluded.overdrawMetrics().pixelsUploadedTranslucent(), 30000 + 1 + 100, 1);
    EXPECT_EQ(0, occluded.overdrawMetrics().tilesCulledForUpload());
}

TEST_F(TiledLayerChromiumTest, dontAllocateContentsWhenTargetSurfaceCantBeAllocated)
{
    // Tile size is 100x100.
    IntRect rootRect(0, 0, 300, 200);
    IntRect childRect(0, 0, 300, 100);
    IntRect child2Rect(0, 100, 300, 100);

    CCLayerTreeSettings settings;
    FakeCCLayerTreeHostClient fakeCCLayerTreeHostClient;
    OwnPtr<CCLayerTreeHost> ccLayerTreeHost = CCLayerTreeHost::create(&fakeCCLayerTreeHostClient, settings);
    ASSERT_TRUE(ccLayerTreeHost->initializeRendererIfNeeded());

    RefPtr<FakeTiledLayerChromium> root = adoptRef(new FakeTiledLayerChromium(ccLayerTreeHost->contentsTextureManager()));
    RefPtr<LayerChromium> surface = LayerChromium::create();
    RefPtr<FakeTiledLayerChromium> child = adoptRef(new FakeTiledLayerChromium(ccLayerTreeHost->contentsTextureManager()));
    RefPtr<FakeTiledLayerChromium> child2 = adoptRef(new FakeTiledLayerChromium(ccLayerTreeHost->contentsTextureManager()));

    root->setBounds(rootRect.size());
    root->setAnchorPoint(FloatPoint());
    root->setDrawableContentRect(rootRect);
    root->setVisibleContentRect(rootRect);
    root->addChild(surface);

    surface->setForceRenderSurface(true);
    surface->setAnchorPoint(FloatPoint());
    surface->setOpacity(0.5);
    surface->addChild(child);
    surface->addChild(child2);

    child->setBounds(childRect.size());
    child->setAnchorPoint(FloatPoint());
    child->setPosition(childRect.location());
    child->setVisibleContentRect(childRect);
    child->setDrawableContentRect(rootRect);

    child2->setBounds(child2Rect.size());
    child2->setAnchorPoint(FloatPoint());
    child2->setPosition(child2Rect.location());
    child2->setVisibleContentRect(child2Rect);
    child2->setDrawableContentRect(rootRect);

    ccLayerTreeHost->setRootLayer(root);
    ccLayerTreeHost->setViewportSize(rootRect.size(), rootRect.size());

    // With a huge memory limit, all layers should update and push their textures.
    root->invalidateContentRect(rootRect);
    child->invalidateContentRect(childRect);
    child2->invalidateContentRect(child2Rect);
    ccLayerTreeHost->updateLayers(m_queue, std::numeric_limits<size_t>::max());
    {
        updateTextures(1000);
        EXPECT_EQ(6, root->fakeLayerTextureUpdater()->updateCount());
        EXPECT_EQ(3, child->fakeLayerTextureUpdater()->updateCount());
        EXPECT_EQ(3, child2->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());

        root->fakeLayerTextureUpdater()->clearUpdateCount();
        child->fakeLayerTextureUpdater()->clearUpdateCount();
        child2->fakeLayerTextureUpdater()->clearUpdateCount();

        ScopedFakeCCTiledLayerImpl rootImpl(root->id());
        ScopedFakeCCTiledLayerImpl childImpl(child->id());
        ScopedFakeCCTiledLayerImpl child2Impl(child2->id());
        layerPushPropertiesTo(root.get(), rootImpl.get());
        layerPushPropertiesTo(child.get(), childImpl.get());
        layerPushPropertiesTo(child2.get(), child2Impl.get());

        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < 2; ++j)
                EXPECT_TRUE(rootImpl->hasTextureIdForTileAt(i, j));
            EXPECT_TRUE(childImpl->hasTextureIdForTileAt(i, 0));
            EXPECT_TRUE(child2Impl->hasTextureIdForTileAt(i, 0));
        }
    }
    ccLayerTreeHost->commitComplete();

    // With a memory limit that includes only the root layer (3x2 tiles) and half the surface that
    // the child layers draw into, the child layers will not be allocated. If the surface isn't
    // accounted for, then one of the children would fit within the memory limit.
    root->invalidateContentRect(rootRect);
    child->invalidateContentRect(childRect);
    child2->invalidateContentRect(child2Rect);
    ccLayerTreeHost->updateLayers(m_queue, (3 * 2 + 3 * 1) * (100 * 100) * 4);
    {
        updateTextures(1000);
        EXPECT_EQ(6, root->fakeLayerTextureUpdater()->updateCount());
        EXPECT_EQ(0, child->fakeLayerTextureUpdater()->updateCount());
        EXPECT_EQ(0, child2->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());

        root->fakeLayerTextureUpdater()->clearUpdateCount();
        child->fakeLayerTextureUpdater()->clearUpdateCount();
        child2->fakeLayerTextureUpdater()->clearUpdateCount();

        ScopedFakeCCTiledLayerImpl rootImpl(root->id());
        ScopedFakeCCTiledLayerImpl childImpl(child->id());
        ScopedFakeCCTiledLayerImpl child2Impl(child2->id());
        layerPushPropertiesTo(root.get(), rootImpl.get());
        layerPushPropertiesTo(child.get(), childImpl.get());
        layerPushPropertiesTo(child2.get(), child2Impl.get());

        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < 2; ++j)
                EXPECT_TRUE(rootImpl->hasTextureIdForTileAt(i, j));
            EXPECT_FALSE(childImpl->hasTextureIdForTileAt(i, 0));
            EXPECT_FALSE(child2Impl->hasTextureIdForTileAt(i, 0));
        }
    }
    ccLayerTreeHost->commitComplete();

    // With a memory limit that includes only half the root layer, no contents will be
    // allocated. If render surface memory wasn't accounted for, there is enough space
    // for one of the children layers, but they draw into a surface that can't be
    // allocated.
    root->invalidateContentRect(rootRect);
    child->invalidateContentRect(childRect);
    child2->invalidateContentRect(child2Rect);
    ccLayerTreeHost->updateLayers(m_queue, (3 * 1) * (100 * 100) * 4);
    {
        updateTextures(1000);
        EXPECT_EQ(0, root->fakeLayerTextureUpdater()->updateCount());
        EXPECT_EQ(0, child->fakeLayerTextureUpdater()->updateCount());
        EXPECT_EQ(0, child2->fakeLayerTextureUpdater()->updateCount());
        EXPECT_FALSE(m_queue.hasMoreUpdates());

        root->fakeLayerTextureUpdater()->clearUpdateCount();
        child->fakeLayerTextureUpdater()->clearUpdateCount();
        child2->fakeLayerTextureUpdater()->clearUpdateCount();

        ScopedFakeCCTiledLayerImpl rootImpl(root->id());
        ScopedFakeCCTiledLayerImpl childImpl(child->id());
        ScopedFakeCCTiledLayerImpl child2Impl(child2->id());
        layerPushPropertiesTo(root.get(), rootImpl.get());
        layerPushPropertiesTo(child.get(), childImpl.get());
        layerPushPropertiesTo(child2.get(), child2Impl.get());

        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < 2; ++j)
                EXPECT_FALSE(rootImpl->hasTextureIdForTileAt(i, j));
            EXPECT_FALSE(childImpl->hasTextureIdForTileAt(i, 0));
            EXPECT_FALSE(child2Impl->hasTextureIdForTileAt(i, 0));
        }
    }
    ccLayerTreeHost->commitComplete();

    textureManagerClearAllMemory(ccLayerTreeHost->contentsTextureManager(), m_resourceProvider.get());
    ccLayerTreeHost->setRootLayer(0);
    ccLayerTreeHost.clear();
}

class TrackingLayerPainter : public LayerPainterChromium {
public:
    static PassOwnPtr<TrackingLayerPainter> create() { return adoptPtr(new TrackingLayerPainter()); }

    virtual void paint(SkCanvas*, const IntRect& contentRect, FloatRect&) OVERRIDE
    {
        m_paintedRect = contentRect;
    }

    const IntRect& paintedRect() const { return m_paintedRect; }
    void resetPaintedRect() { m_paintedRect = IntRect(); }

private:
    TrackingLayerPainter() { }

    IntRect m_paintedRect;
};

class UpdateTrackingTiledLayerChromium : public FakeTiledLayerChromium {
public:
    explicit UpdateTrackingTiledLayerChromium(WebCore::CCPrioritizedTextureManager* manager)
        : FakeTiledLayerChromium(manager)
    {
        OwnPtr<TrackingLayerPainter> trackingLayerPainter(TrackingLayerPainter::create());
        m_trackingLayerPainter = trackingLayerPainter.get();
        m_layerTextureUpdater = BitmapCanvasLayerTextureUpdater::create(trackingLayerPainter.release());
    }
    virtual ~UpdateTrackingTiledLayerChromium() { }

    TrackingLayerPainter* trackingLayerPainter() const { return m_trackingLayerPainter; }

protected:
    virtual WebCore::LayerTextureUpdater* textureUpdater() const OVERRIDE { return m_layerTextureUpdater.get(); }

private:
    TrackingLayerPainter* m_trackingLayerPainter;
    RefPtr<BitmapCanvasLayerTextureUpdater> m_layerTextureUpdater;
};

TEST_F(TiledLayerChromiumTest, nonIntegerContentsScaleIsNotDistortedDuringPaint)
{
    RefPtr<UpdateTrackingTiledLayerChromium> layer = adoptRef(new UpdateTrackingTiledLayerChromium(m_textureManager.get()));

    IntRect layerRect(0, 0, 30, 31);
    layer->setPosition(layerRect.location());
    layer->setBounds(layerRect.size());
    layer->setContentsScale(1.5);

    IntRect contentRect(0, 0, 45, 47);
    EXPECT_EQ(contentRect.size(), layer->contentBounds());
    layer->setVisibleContentRect(contentRect);
    layer->setDrawableContentRect(contentRect);

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    // Update the whole tile.
    layer->update(m_queue, 0, m_stats);
    layer->trackingLayerPainter()->resetPaintedRect();

    EXPECT_RECT_EQ(IntRect(), layer->trackingLayerPainter()->paintedRect());
    updateTextures();

    // Invalidate the entire layer in content space. When painting, the rect given to webkit should match the layer's bounds.
    layer->invalidateContentRect(contentRect);
    layer->update(m_queue, 0, m_stats);

    EXPECT_RECT_EQ(layerRect, layer->trackingLayerPainter()->paintedRect());
}

TEST_F(TiledLayerChromiumTest, nonIntegerContentsScaleIsNotDistortedDuringInvalidation)
{
    RefPtr<UpdateTrackingTiledLayerChromium> layer = adoptRef(new UpdateTrackingTiledLayerChromium(m_textureManager.get()));

    IntRect layerRect(0, 0, 30, 31);
    layer->setPosition(layerRect.location());
    layer->setBounds(layerRect.size());
    layer->setContentsScale(1.3f);

    IntRect contentRect(IntPoint(), layer->contentBounds());
    layer->setVisibleContentRect(contentRect);
    layer->setDrawableContentRect(contentRect);

    layer->setTexturePriorities(m_priorityCalculator);
    m_textureManager->prioritizeTextures();

    // Update the whole tile.
    layer->update(m_queue, 0, m_stats);
    layer->trackingLayerPainter()->resetPaintedRect();

    EXPECT_RECT_EQ(IntRect(), layer->trackingLayerPainter()->paintedRect());
    updateTextures();

    // Invalidate the entire layer in layer space. When painting, the rect given to webkit should match the layer's bounds.
    layer->setNeedsDisplayRect(layerRect);
    layer->update(m_queue, 0, m_stats);

    EXPECT_RECT_EQ(layerRect, layer->trackingLayerPainter()->paintedRect());
}

} // namespace
