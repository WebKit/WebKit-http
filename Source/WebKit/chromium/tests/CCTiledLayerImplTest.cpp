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

#include "cc/CCTiledLayerImpl.h"

#include "Region.h"
#include "cc/CCSingleThreadProxy.h"
#include "cc/CCTileDrawQuad.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace WebCore;

namespace {

// Create a default tiled layer with textures for all tiles and a default
// visibility of the entire layer size.
static PassRefPtr<CCTiledLayerImpl> createLayer(const IntSize& tileSize, const IntSize& layerSize, CCLayerTilingData::BorderTexelOption borderTexels)
{
    RefPtr<CCTiledLayerImpl> layer = CCTiledLayerImpl::create(0);
    OwnPtr<CCLayerTilingData> tiler = CCLayerTilingData::create(tileSize, borderTexels);
    tiler->setBounds(layerSize);
    layer->setTilingData(*tiler);
    layer->setSkipsDraw(false);
    layer->setVisibleLayerRect(IntRect(IntPoint(), layerSize));

    int textureId = 1;
    for (int i = 0; i < tiler->numTilesX(); ++i)
        for (int j = 0; j < tiler->numTilesY(); ++j)
            layer->syncTextureId(i, j, static_cast<Platform3DObject>(textureId++));

    return layer.release();
}

TEST(CCTiledLayerImplTest, emptyQuadList)
{
    DebugScopedSetImplThread scopedImplThread;

    const IntSize tileSize(90, 90);
    const int numTilesX = 8;
    const int numTilesY = 4;
    const IntSize layerSize(tileSize.width() * numTilesX, tileSize.height() * numTilesY);

    // Verify default layer does creates quads
    {
        RefPtr<CCTiledLayerImpl> layer = createLayer(tileSize, layerSize, CCLayerTilingData::NoBorderTexels);
        CCQuadList quads;
        OwnPtr<CCSharedQuadState> sharedQuadState = layer->createSharedQuadState();
        layer->appendQuads(quads, sharedQuadState.get());
        const unsigned numTiles = numTilesX * numTilesY;
        EXPECT_EQ(quads.size(), numTiles);
    }

    // Layer with empty visible layer rect produces no quads
    {
        RefPtr<CCTiledLayerImpl> layer = createLayer(tileSize, layerSize, CCLayerTilingData::NoBorderTexels);
        layer->setVisibleLayerRect(IntRect());

        CCQuadList quads;
        OwnPtr<CCSharedQuadState> sharedQuadState = layer->createSharedQuadState();
        layer->appendQuads(quads, sharedQuadState.get());
        EXPECT_EQ(quads.size(), 0u);
    }

    // Layer with non-intersecting visible layer rect produces no quads
    {
        RefPtr<CCTiledLayerImpl> layer = createLayer(tileSize, layerSize, CCLayerTilingData::NoBorderTexels);

        IntRect outsideBounds(IntPoint(-100, -100), IntSize(50, 50));
        layer->setVisibleLayerRect(outsideBounds);

        CCQuadList quads;
        OwnPtr<CCSharedQuadState> sharedQuadState = layer->createSharedQuadState();
        layer->appendQuads(quads, sharedQuadState.get());
        EXPECT_EQ(quads.size(), 0u);
    }

    // Layer with skips draw produces no quads
    {
        RefPtr<CCTiledLayerImpl> layer = createLayer(tileSize, layerSize, CCLayerTilingData::NoBorderTexels);
        layer->setSkipsDraw(true);

        CCQuadList quads;
        OwnPtr<CCSharedQuadState> sharedQuadState = layer->createSharedQuadState();
        layer->appendQuads(quads, sharedQuadState.get());
        EXPECT_EQ(quads.size(), 0u);
    }
}

TEST(CCTiledLayerImplTest, checkerboarding)
{
    DebugScopedSetImplThread scopedImplThread;

    const IntSize tileSize(10, 10);
    const int numTilesX = 2;
    const int numTilesY = 2;
    const IntSize layerSize(tileSize.width() * numTilesX, tileSize.height() * numTilesY);

    RefPtr<CCTiledLayerImpl> layer = createLayer(tileSize, layerSize, CCLayerTilingData::NoBorderTexels);
    OwnPtr<CCSharedQuadState> sharedQuadState = layer->createSharedQuadState();

    // No checkerboarding
    {
        CCQuadList quads;
        layer->appendQuads(quads, sharedQuadState.get());
        EXPECT_EQ(quads.size(), 4u);

        for (size_t i = 0; i < quads.size(); ++i)
            EXPECT_EQ(quads[i]->material(), CCDrawQuad::TiledContent);
    }

    for (int i = 0; i < numTilesX; ++i)
        for (int j = 0; j < numTilesY; ++j)
            layer->syncTextureId(i, j, static_cast<Platform3DObject>(0));

    // All checkerboarding
    {
        CCQuadList quads;
        layer->appendQuads(quads, sharedQuadState.get());
        EXPECT_EQ(quads.size(), 4u);
        for (size_t i = 0; i < quads.size(); ++i)
            EXPECT_EQ(quads[i]->material(), CCDrawQuad::SolidColor);
    }
}

static bool completelyContains(const Region& container, const IntRect& rect)
{
    Region tester(rect);
    Vector<IntRect> rects = container.rects();
    for (size_t i = 0; i < rects.size(); ++i)
        tester.subtract(rects[i]);
    return tester.isEmpty();
}

static CCQuadList getQuads(IntSize tileSize, const IntSize& layerSize, CCLayerTilingData::BorderTexelOption borderTexelOption, const IntRect& visibleLayerRect)
{
    CCQuadList quads;

    RefPtr<CCTiledLayerImpl> layer = createLayer(tileSize, layerSize, borderTexelOption);
    layer->setVisibleLayerRect(visibleLayerRect);
    layer->setBounds(layerSize);

    OwnPtr<CCSharedQuadState> sharedQuadState = layer->createSharedQuadState();
    layer->appendQuads(quads, sharedQuadState.get());

    return quads;
}

// Align with expected and actual output
static const char* quadString = "    Quad: ";

static void verifyQuadsExactlyCoverRect(const CCQuadList& quads, const IntRect& rect)
{
    Region remaining(rect);

    for (size_t i = 0; i < quads.size(); ++i) {
        CCDrawQuad* quad = quads[i].get();

        EXPECT_TRUE(rect.contains(quad->quadRect())) << quadString << i;
        EXPECT_TRUE(completelyContains(remaining, quad->quadRect())) << quadString << i;
        remaining.subtract(Region(quad->quadRect()));
    }

    EXPECT_TRUE(remaining.isEmpty());
}

// Test with both border texels and without.
#define WITH_AND_WITHOUT_BORDER_TEST(testFixtureName)       \
    TEST(CCTiledLayerImplTest, testFixtureName##NoBorders)  \
    {                                                       \
        testFixtureName(CCLayerTilingData::NoBorderTexels); \
    }                                                       \
    TEST(CCTiledLayerImplTest, testFixtureName##HasBorders) \
    {                                                       \
        testFixtureName(CCLayerTilingData::HasBorderTexels);\
    }

static void coverageVisibleRectOnTileBoundaries(CCLayerTilingData::BorderTexelOption borders)
{
    DebugScopedSetImplThread scopedImplThread;

    IntSize layerSize(1000, 1000);
    CCQuadList quads = getQuads(IntSize(100, 100), layerSize, borders, IntRect(IntPoint(), layerSize));
    verifyQuadsExactlyCoverRect(quads, IntRect(IntPoint(), layerSize));
}
WITH_AND_WITHOUT_BORDER_TEST(coverageVisibleRectOnTileBoundaries);

static void coverageVisibleRectIntersectsTiles(CCLayerTilingData::BorderTexelOption borders)
{
    DebugScopedSetImplThread scopedImplThread;

    // This rect intersects the middle 3x3 of the 5x5 tiles.
    IntPoint topLeft(65, 73);
    IntPoint bottomRight(182, 198);
    IntRect visibleLayerRect(topLeft, bottomRight - topLeft);

    IntSize layerSize(250, 250);
    CCQuadList quads = getQuads(IntSize(50, 50), IntSize(250, 250), CCLayerTilingData::NoBorderTexels, visibleLayerRect);
    verifyQuadsExactlyCoverRect(quads, visibleLayerRect);
}
WITH_AND_WITHOUT_BORDER_TEST(coverageVisibleRectIntersectsTiles);

static void coverageVisibleRectIntersectsBounds(CCLayerTilingData::BorderTexelOption borders)
{
    DebugScopedSetImplThread scopedImplThread;

    IntSize layerSize(220, 210);
    IntRect visibleLayerRect(IntPoint(), layerSize);
    CCQuadList quads = getQuads(IntSize(100, 100), layerSize, CCLayerTilingData::NoBorderTexels, visibleLayerRect);
    verifyQuadsExactlyCoverRect(quads, visibleLayerRect);
}
WITH_AND_WITHOUT_BORDER_TEST(coverageVisibleRectIntersectsBounds);

TEST(CCTiledLayerImplTest, textureInfoForLayerNoBorders)
{
    DebugScopedSetImplThread scopedImplThread;

    IntSize tileSize(50, 50);
    IntSize layerSize(250, 250);
    CCQuadList quads = getQuads(tileSize, layerSize, CCLayerTilingData::NoBorderTexels, IntRect(IntPoint(), layerSize));

    for (size_t i = 0; i < quads.size(); ++i) {
        ASSERT_EQ(quads[i]->material(), CCDrawQuad::TiledContent) << quadString << i;
        CCTileDrawQuad* quad = static_cast<CCTileDrawQuad*>(quads[i].get());

        EXPECT_NE(quad->textureId(), 0u) << quadString << i;
        EXPECT_EQ(quad->textureOffset(), IntPoint()) << quadString << i;
        EXPECT_EQ(quad->textureSize(), tileSize) << quadString << i;
    }
}


} // namespace
