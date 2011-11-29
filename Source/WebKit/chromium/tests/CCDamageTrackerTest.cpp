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

#include "cc/CCDamageTracker.h"

#include "CCLayerTreeTestCommon.h"
#include "cc/CCLayerImpl.h"
#include "cc/CCLayerSorter.h"
#include "cc/CCLayerTreeHostCommon.h"
#include "cc/CCSingleThreadProxy.h"
#include <gtest/gtest.h>

using namespace WebCore;
using namespace WTF;
using namespace WebKitTests;

namespace {

void executeCalculateDrawTransformsAndVisibility(CCLayerImpl* root, Vector<RefPtr<CCLayerImpl> >& renderSurfaceLayerList)
{
    CCLayerSorter layerSorter;
    TransformationMatrix identityMatrix;
    Vector<RefPtr<CCLayerImpl> > dummyLayerList;
    int dummyMaxTextureSize = 512;

    // Sanity check: The test itself should create the root layer's render surface, so
    //               that the surface (and its damage tracker) can persist across multiple
    //               calls to this function.
    ASSERT_TRUE(root->renderSurface());
    ASSERT_FALSE(renderSurfaceLayerList.size());

    root->renderSurface()->clearLayerList();
    renderSurfaceLayerList.append(root);
    CCLayerTreeHostCommon::calculateDrawTransformsAndVisibility(root, root, identityMatrix, identityMatrix, renderSurfaceLayerList, dummyLayerList, &layerSorter, dummyMaxTextureSize);
}

void emulateDrawingOneFrame(CCLayerImpl* root)
{
    // This emulates only the steps that are relevant to testing the damage tracker:
    //   1. computing the render passes and layerlists
    //   2. updating all damage trackers in the correct order
    //   3. resetting all updateRects and propertyChanged flags for all layers and surfaces.

    Vector<RefPtr<CCLayerImpl> > renderSurfaceLayerList;
    executeCalculateDrawTransformsAndVisibility(root, renderSurfaceLayerList);

    // Iterate back-to-front, so that damage correctly propagates from descendant surfaces to ancestors.
    for (int i = renderSurfaceLayerList.size() - 1; i >= 0; --i) {
        CCRenderSurface* targetSurface = renderSurfaceLayerList[i]->renderSurface();
        targetSurface->damageTracker()->updateDamageRectForNextFrame(targetSurface->layerList(), targetSurface->owningLayerId(), renderSurfaceLayerList[i]->maskLayer());
    }

    root->resetAllChangeTrackingForSubtree();
}

PassRefPtr<CCLayerImpl> createTestTreeWithOneSurface()
{
    RefPtr<CCLayerImpl> root = CCLayerImpl::create(1);
    RefPtr<CCLayerImpl> child = CCLayerImpl::create(2);

    root->setPosition(FloatPoint::zero());
    root->setAnchorPoint(FloatPoint::zero());
    root->setBounds(IntSize(500, 500));
    root->setDrawsContent(true);
    root->createRenderSurface();
    root->renderSurface()->setContentRect(IntRect(IntPoint(), IntSize(500, 500)));

    child->setPosition(FloatPoint(100, 100));
    child->setAnchorPoint(FloatPoint::zero());
    child->setBounds(IntSize(30, 30));
    child->setDrawsContent(true);
    root->addChild(child);

    return root.release();
}

PassRefPtr<CCLayerImpl> createTestTreeWithTwoSurfaces()
{
    // This test tree has two render surfaces: one for the root, and one for
    // child1. Additionally, the root has a second child layer, and child1 has two
    // children of its own.

    RefPtr<CCLayerImpl> root = CCLayerImpl::create(1);
    RefPtr<CCLayerImpl> child1 = CCLayerImpl::create(2);
    RefPtr<CCLayerImpl> child2 = CCLayerImpl::create(3);
    RefPtr<CCLayerImpl> grandChild1 = CCLayerImpl::create(4);
    RefPtr<CCLayerImpl> grandChild2 = CCLayerImpl::create(5);

    root->setPosition(FloatPoint::zero());
    root->setAnchorPoint(FloatPoint::zero());
    root->setBounds(IntSize(500, 500));
    root->setDrawsContent(true);
    root->createRenderSurface();
    root->renderSurface()->setContentRect(IntRect(IntPoint(), IntSize(500, 500)));

    child1->setPosition(FloatPoint(100, 100));
    child1->setAnchorPoint(FloatPoint::zero());
    child1->setBounds(IntSize(30, 30));
    child1->setOpacity(0.5); // with a child that drawsContent, this will cause the layer to create its own renderSurface.
    child1->setDrawsContent(false); // this layer does not draw, but is intended to create its own renderSurface.

    child2->setPosition(FloatPoint(11, 11));
    child2->setAnchorPoint(FloatPoint::zero());
    child2->setBounds(IntSize(18, 18));
    child2->setDrawsContent(true);

    grandChild1->setPosition(FloatPoint(200, 200));
    grandChild1->setAnchorPoint(FloatPoint::zero());
    grandChild1->setBounds(IntSize(6, 8));
    grandChild1->setDrawsContent(true);

    grandChild2->setPosition(FloatPoint(190, 190));
    grandChild2->setAnchorPoint(FloatPoint::zero());
    grandChild2->setBounds(IntSize(6, 8));
    grandChild2->setDrawsContent(true);

    child1->addChild(grandChild1);
    child1->addChild(grandChild2);
    root->addChild(child1);
    root->addChild(child2);

    return root.release();
}

PassRefPtr<CCLayerImpl> createAndSetUpTestTreeWithOneSurface()
{
    RefPtr<CCLayerImpl> root = createTestTreeWithOneSurface();

    // Setup includes going past the first frame which always damages everything, so
    // that we can actually perform specific tests.
    emulateDrawingOneFrame(root.get());

    return root.release();
}

PassRefPtr<CCLayerImpl> createAndSetUpTestTreeWithTwoSurfaces()
{
    RefPtr<CCLayerImpl> root = createTestTreeWithTwoSurfaces();

    // Setup includes going past the first frame which always damages everything, so
    // that we can actually perform specific tests.
    emulateDrawingOneFrame(root.get());

    return root.release();
}

class CCDamageTrackerTest : public testing::Test {
private:
    // For testing purposes, fake that we are on the impl thread.
    DebugScopedSetImplThread setImplThread;
};

TEST_F(CCDamageTrackerTest, sanityCheckTestTreeWithOneSurface)
{
    // Sanity check that the simple test tree will actually produce the expected render
    // surfaces and layer lists.

    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithOneSurface();

    EXPECT_EQ(static_cast<size_t>(2), root->renderSurface()->layerList().size());
    EXPECT_EQ(1, root->renderSurface()->layerList()[0]->id());
    EXPECT_EQ(2, root->renderSurface()->layerList()[1]->id());

    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 500, 500), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, sanityCheckTestTreeWithTwoSurfaces)
{
    // Sanity check that the complex test tree will actually produce the expected render
    // surfaces and layer lists.

    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();

    RefPtr<CCLayerImpl> child1 = root->children()[0];
    RefPtr<CCLayerImpl> child2 = root->children()[1];
    FloatRect childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();

    ASSERT_TRUE(child1->renderSurface());
    EXPECT_FALSE(child2->renderSurface());
    EXPECT_EQ(static_cast<size_t>(3), root->renderSurface()->layerList().size());
    EXPECT_EQ(static_cast<size_t>(2), child1->renderSurface()->layerList().size());

    // The render surface for child1 only has a contentRect that encloses grandChild1 and grandChild2, because child1 does not draw content.
    EXPECT_FLOAT_RECT_EQ(FloatRect(190, 190, 16, 18), childDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 500, 500), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForUpdateRects)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithOneSurface();
    RefPtr<CCLayerImpl> child = root->children()[0];

    // CASE 1: Setting the update rect should cause the corresponding damage to the surface.
    //
    child->setUpdateRect(FloatRect(10, 11, 12, 13));
    emulateDrawingOneFrame(root.get());

    // Damage position on the surface should be: position of updateRect (10, 11) relative to the child (100, 100).
    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(110, 111, 12, 13), rootDamageRect);

    // CASE 2: The same update rect twice in a row still produces the same damage.
    //
    child->setUpdateRect(FloatRect(10, 11, 12, 13));
    emulateDrawingOneFrame(root.get());
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(110, 111, 12, 13), rootDamageRect);

    // CASE 3: Setting a different update rect should cause damage on the new update region, but no additional exposed old region.
    //
    child->setUpdateRect(FloatRect(20, 25, 1, 2));
    emulateDrawingOneFrame(root.get());

    // Damage position on the surface should be: position of updateRect (20, 25) relative to the child (100, 100).
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(120, 125, 1, 2), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForPropertyChanges)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithOneSurface();
    RefPtr<CCLayerImpl> child = root->children()[0];

    // CASE 1: The layer's property changed flag takes priority over update rect.
    //
    child->setUpdateRect(FloatRect(10, 11, 12, 13));
    child->setOpacity(0.5);
    emulateDrawingOneFrame(root.get());

    // Sanity check - we should not have accidentally created a separate render surface for the translucent layer.
    ASSERT_FALSE(child->renderSurface());
    ASSERT_EQ(static_cast<size_t>(2), root->renderSurface()->layerList().size());

    // Damage should be the entire child layer in targetSurface space.
    FloatRect expectedRect = FloatRect(100, 100, 30, 30);
    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(expectedRect, rootDamageRect);

    // CASE 2: If a layer moves due to property change, it damages both the new location
    //         and the old (exposed) location. The old location is the entire old layer,
    //         not just the updateRect.

    // Cycle one frame of no change, just to sanity check that the next rect is not because of the old damage state.
    emulateDrawingOneFrame(root.get());
    EXPECT_TRUE(root->renderSurface()->damageTracker()->currentDamageRect().isEmpty());

    // Then, test the actual layer movement.
    child->setPosition(FloatPoint(200, 230));
    emulateDrawingOneFrame(root.get());

    // Expect damage to be the combination of the previous one and the new one.
    expectedRect.uniteIfNonZero(FloatRect(200, 230, 30, 30));
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(expectedRect, rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForTransformedLayer)
{
    // If a layer is transformed, the damage rect should still enclose the entire
    // transformed layer.

    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithOneSurface();
    RefPtr<CCLayerImpl> child = root->children()[0];

    TransformationMatrix rotation;
    rotation.rotate(45);

    // Note carefully, the anchor is actually part of layer->position(). By setting anchor
    // to (0.5, 0.5), the layer's position (100, 100) now refers to the center of the
    // layer, not the corner. This means the layer has actually changed position.
    child->setAnchorPoint(FloatPoint(0.5, 0.5));
    emulateDrawingOneFrame(root.get());

    // Sanity check that the layer actually moved to (85, 85), damaging its old location and new location.
    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(85, 85, 45, 45), rootDamageRect);

    // With the anchor on the layer's center, now we can test the rotation more
    // intuitively, since it applies about the layer's anchor.
    child->setTransform(rotation);
    emulateDrawingOneFrame(root.get());

    // Since the child layer is square, rotation by 45 degrees about the center should
    // increase the size of the expected rect by sqrt(2), centered around (100, 100). The
    // old exposed region should be fully contained in the new region.
    double expectedWidth = 30.0 * sqrt(2.0);
    double expectedPosition = 100.0 - 0.5 * expectedWidth;
    FloatRect expectedRect(expectedPosition, expectedPosition, expectedWidth, expectedWidth);
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(expectedRect, rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForAddingAndRemovingLayer)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithOneSurface();
    RefPtr<CCLayerImpl> child1 = root->children()[0];

    RefPtr<CCLayerImpl> child2 = CCLayerImpl::create(3);
    child2->setPosition(FloatPoint(400, 380));
    child2->setAnchorPoint(FloatPoint::zero());
    child2->setBounds(IntSize(6, 8));
    child2->setDrawsContent(true);

    // CASE 1: Adding a new layer should cause the appropriate damage.
    //
    root->addChild(child2);
    emulateDrawingOneFrame(root.get());

    // Sanity check - all 3 layers should be on the same render surface; render surfaces are tested elsewhere.
    ASSERT_EQ(static_cast<size_t>(3), root->renderSurface()->layerList().size());

    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(400, 380, 6, 8), rootDamageRect);

    // CASE 2: If the layer is removed, its entire old layer becomes exposed, not just the
    //         last update rect.

    // Advance one frame without damage so that we know the damage rect is not leftover from the previous case.
    emulateDrawingOneFrame(root.get());
    EXPECT_TRUE(root->renderSurface()->damageTracker()->currentDamageRect().isEmpty());

    // Then, test removing child1.
    child1->removeFromParent();
    emulateDrawingOneFrame(root.get());
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(100, 100, 30, 30), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForMultipleLayers)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithOneSurface();
    RefPtr<CCLayerImpl> child1 = root->children()[0];

    RefPtr<CCLayerImpl> child2 = CCLayerImpl::create(3);
    child2->setPosition(FloatPoint(400, 380));
    child2->setAnchorPoint(FloatPoint::zero());
    child2->setBounds(IntSize(6, 8));
    child2->setDrawsContent(true);
    root->addChild(child2);

    // In this test we don't want the above tree manipulation to be considered part of the same frame.
    emulateDrawingOneFrame(root.get());

    // Damaging two layers simultaneously should cause combined damage.
    // - child1 update rect in surface space: FloatRect(100, 100, 1, 2);
    // - child2 update rect in surface space: FloatRect(400, 380, 3, 4);
    child1->setUpdateRect(FloatRect(0, 0, 1, 2));
    child2->setUpdateRect(FloatRect(0, 0, 3, 4));
    emulateDrawingOneFrame(root.get());
    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(100, 100, 303, 284), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForNestedSurfaces)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    RefPtr<CCLayerImpl> child2 = root->children()[1];
    RefPtr<CCLayerImpl> grandChild1 = root->children()[0]->children()[0];
    FloatRect childDamageRect;
    FloatRect rootDamageRect;

    // CASE 1: Damage to a descendant surface should propagate properly to ancestor surface.
    //
    grandChild1->setOpacity(0.5);
    emulateDrawingOneFrame(root.get());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(200, 200, 6, 8), childDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(300, 300, 6, 8), rootDamageRect);

    // CASE 2: Same as previous case, but with additional damage elsewhere that should be properly unioned.
    // - child1 surface damage in root surface space: FloatRect(300, 300, 6, 8);
    // - child2 damage in root surface space: FloatRect(11, 11, 18, 18);
    grandChild1->setOpacity(0.7);
    child2->setOpacity(0.7);
    emulateDrawingOneFrame(root.get());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(200, 200, 6, 8), childDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(11, 11, 295, 297), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForSurfaceChangeFromDescendantLayer)
{
    // If descendant layer changes and affects the content bounds of the render surface,
    // then the entire descendant surface should be damaged, and it should damage its
    // ancestor surface with the old and new surface regions.

    // This is a tricky case, since only the first grandChild changes, but the entire
    // surface should be marked dirty.

    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    RefPtr<CCLayerImpl> grandChild1 = root->children()[0]->children()[0];
    FloatRect childDamageRect;
    FloatRect rootDamageRect;

    grandChild1->setPosition(FloatPoint(195, 205));
    emulateDrawingOneFrame(root.get());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();

    // The new surface bounds should be damaged entirely, even though only one of the layers changed.
    EXPECT_FLOAT_RECT_EQ(FloatRect(190, 190, 11, 23), childDamageRect);

    // Damage to the root surface should be the union of child1's *entire* render surface
    // (in target space), and its old exposed area (also in target space).
    EXPECT_FLOAT_RECT_EQ(FloatRect(290, 290, 16, 23), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForSurfaceChangeFromAncestorLayer)
{
    // An ancestor/owning layer changes that affects the position/transform of the render
    // surface. Note that in this case, the layerPropertyChanged flag already propagates
    // to the subtree (tested in CCLayerImpltest), which damages the entire child1
    // surface, but the damage tracker still needs the correct logic to compute the
    // exposed region on the root surface.

    // FIXME: the expectations of this test case should change when we add support for a
    //        unique scissorRect per renderSurface. In that case, the child1 surface
    //        should be completely unchanged, since we are only transforming it, while the
    //        root surface would be damaged appropriately.

    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    RefPtr<CCLayerImpl> grandChild1 = root->children()[0]->children()[0];
    FloatRect childDamageRect;
    FloatRect rootDamageRect;

    child1->setPosition(FloatPoint(50, 50));
    emulateDrawingOneFrame(root.get());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();

    // The new surface bounds should be damaged entirely.
    EXPECT_FLOAT_RECT_EQ(FloatRect(190, 190, 16, 18), childDamageRect);

    // The entire child1 surface and the old exposed child1 surface should damage the root surface.
    //  - old child1 surface in target space: FloatRect(290, 290, 16, 18)
    //  - new child1 surface in target space: FloatRect(240, 240, 16, 18)
    EXPECT_FLOAT_RECT_EQ(FloatRect(240, 240, 66, 68), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForAddingAndRemovingRenderSurfaces)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    FloatRect childDamageRect;
    FloatRect rootDamageRect;

    // CASE 1: If a descendant surface disappears, its entire old area becomes exposed.
    //
    child1->setOpacity(1);
    emulateDrawingOneFrame(root.get());

    // Sanity check that there is only one surface now.
    ASSERT_FALSE(child1->renderSurface());
    ASSERT_EQ(static_cast<size_t>(4), root->renderSurface()->layerList().size());

    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(290, 290, 16, 18), rootDamageRect);

    // CASE 2: If a descendant surface appears, its entire old area becomes exposed.

    // Cycle one frame of no change, just to sanity check that the next rect is not because of the old damage state.
    emulateDrawingOneFrame(root.get());
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_TRUE(rootDamageRect.isEmpty());

    // Then change the tree so that the render surface is added back.
    child1->setOpacity(0.5);
    emulateDrawingOneFrame(root.get());

    // Sanity check that there is a new surface now.
    ASSERT_TRUE(child1->renderSurface());
    EXPECT_EQ(static_cast<size_t>(3), root->renderSurface()->layerList().size());
    EXPECT_EQ(static_cast<size_t>(2), child1->renderSurface()->layerList().size());

    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(190, 190, 16, 18), childDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(290, 290, 16, 18), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyNoDamageWhenNothingChanged)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    FloatRect childDamageRect;
    FloatRect rootDamageRect;

    // CASE 1: If nothing changes, the damage rect should be empty.
    //
    emulateDrawingOneFrame(root.get());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_TRUE(childDamageRect.isEmpty());
    EXPECT_TRUE(rootDamageRect.isEmpty());

    // CASE 2: If nothing changes twice in a row, the damage rect should still be empty.
    //
    emulateDrawingOneFrame(root.get());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_TRUE(childDamageRect.isEmpty());
    EXPECT_TRUE(rootDamageRect.isEmpty());
}

TEST_F(CCDamageTrackerTest, verifyNoDamageForUpdateRectThatDoesNotDrawContent)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    FloatRect childDamageRect;
    FloatRect rootDamageRect;

    // In our specific tree, the update rect of child1 should not cause any damage to any
    // surface because it does not actually draw content.
    child1->setUpdateRect(FloatRect(0, 0, 1, 2));
    emulateDrawingOneFrame(root.get());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_TRUE(childDamageRect.isEmpty());
    EXPECT_TRUE(rootDamageRect.isEmpty());
}

TEST_F(CCDamageTrackerTest, verifyDamageForReplica)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    RefPtr<CCLayerImpl> grandChild1 = child1->children()[0];
    RefPtr<CCLayerImpl> grandChild2 = child1->children()[1];

    // Damage on a surface that has a reflection should cause the target surface to
    // receive the surface's damage and the surface's reflected damage.

    // For this test case, we modify grandChild2, and add grandChild3 to extend the bounds
    // of child1's surface. This way, we can test reflection changes without changing
    // contentBounds of the surface.
    grandChild2->setPosition(FloatPoint(180, 180));
    RefPtr<CCLayerImpl> grandChild3 = CCLayerImpl::create(6);
    grandChild3->setPosition(FloatPoint(240, 240));
    grandChild3->setAnchorPoint(FloatPoint::zero());
    grandChild3->setBounds(IntSize(10, 10));
    grandChild3->setDrawsContent(true);
    child1->setOpacity(0.5);
    child1->addChild(grandChild3);
    emulateDrawingOneFrame(root.get());

    // CASE 1: adding a reflection about the left edge of grandChild1.
    //
    RefPtr<CCLayerImpl> grandChild1Replica = CCLayerImpl::create(7);
    grandChild1Replica->setPosition(FloatPoint::zero());
    grandChild1Replica->setAnchorPoint(FloatPoint::zero());
    TransformationMatrix reflection;
    reflection.scale3d(-1.0, 1.0, 1.0);
    grandChild1Replica->setTransform(reflection);
    grandChild1->setReplicaLayer(grandChild1Replica);
    emulateDrawingOneFrame(root.get());

    FloatRect grandChildDamageRect = grandChild1->renderSurface()->damageTracker()->currentDamageRect();
    FloatRect childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    FloatRect rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();

    // The grandChild surface damage should not include its own replica. The child
    // surface damage should include the normal and replica surfaces.
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 6, 8), grandChildDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(194, 200, 12, 8), childDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(294, 300, 12, 8), rootDamageRect);

    // CASE 2: moving the descendant surface should cause both the original and reflected
    //         areas to be damaged on the target.
    IntRect oldContentRect = child1->renderSurface()->contentRect();
    grandChild1->setPosition(FloatPoint(195.0, 205.0));
    emulateDrawingOneFrame(root.get());
    ASSERT_EQ(oldContentRect.width(), child1->renderSurface()->contentRect().width());
    ASSERT_EQ(oldContentRect.height(), child1->renderSurface()->contentRect().height());

    grandChildDamageRect = grandChild1->renderSurface()->damageTracker()->currentDamageRect();
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();

    // The child surface damage should include normal and replica surfaces for both old and new locations.
    //  - old location in target space: FloatRect(194, 200, 12, 8)
    //  - new location in target space: FloatRect(189, 205, 12, 8)
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 6, 8), grandChildDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(189, 200, 17, 13), childDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(289, 300, 17, 13), rootDamageRect);

    // CASE 3: removing the reflection should cause the entire region including reflection
    //         to damage the target surface.
    grandChild1->setReplicaLayer(0);
    emulateDrawingOneFrame(root.get());
    ASSERT_EQ(oldContentRect.width(), child1->renderSurface()->contentRect().width());
    ASSERT_EQ(oldContentRect.height(), child1->renderSurface()->contentRect().height());

    EXPECT_FALSE(grandChild1->renderSurface());
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();
    rootDamageRect = root->renderSurface()->damageTracker()->currentDamageRect();

    EXPECT_FLOAT_RECT_EQ(FloatRect(189, 205, 12, 8), childDamageRect);
    EXPECT_FLOAT_RECT_EQ(FloatRect(289, 305, 12, 8), rootDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForMask)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithOneSurface();
    RefPtr<CCLayerImpl> child = root->children()[0];

    // In the current implementation of the damage tracker, changes to mask layers should
    // damage the entire corresponding surface.

    // Set up the mask layer.
    RefPtr<CCLayerImpl> maskLayer = CCLayerImpl::create(3);
    maskLayer->setPosition(child->position());
    maskLayer->setAnchorPoint(FloatPoint::zero());
    maskLayer->setBounds(child->bounds());
    child->setMaskLayer(maskLayer);

    // Add opacity and a grandChild so that the render surface persists even after we remove the mask.
    child->setOpacity(0.5);
    RefPtr<CCLayerImpl> grandChild = CCLayerImpl::create(4);
    grandChild->setPosition(FloatPoint(2.0, 2.0));
    grandChild->setAnchorPoint(FloatPoint::zero());
    grandChild->setBounds(IntSize(2, 2));
    grandChild->setDrawsContent(true);
    child->addChild(grandChild);
    emulateDrawingOneFrame(root.get());

    // Sanity check that a new surface was created for the child.
    ASSERT_TRUE(child->renderSurface());

    // CASE 1: the updateRect on a mask layer should damage the entire target surface.
    //
    maskLayer->setUpdateRect(FloatRect(1, 2, 3, 4));
    emulateDrawingOneFrame(root.get());
    FloatRect childDamageRect = child->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 30, 30), childDamageRect);

    // CASE 2: a property change on the mask layer should damage the entire target surface.
    //

    // Advance one frame without damage so that we know the damage rect is not leftover from the previous case.
    emulateDrawingOneFrame(root.get());
    childDamageRect = child->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_TRUE(childDamageRect.isEmpty());

    // Then test the property change.
    maskLayer->setOpacity(0.5);
    emulateDrawingOneFrame(root.get());
    childDamageRect = child->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 30, 30), childDamageRect);

    // CASE 3: removing the mask also damages the entire target surface.
    //

    // Advance one frame without damage so that we know the damage rect is not leftover from the previous case.
    emulateDrawingOneFrame(root.get());
    childDamageRect = child->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_TRUE(childDamageRect.isEmpty());

    // Then test mask removal.
    child->setMaskLayer(0);
    ASSERT_TRUE(child->layerPropertyChanged());
    emulateDrawingOneFrame(root.get());

    // Sanity check that a render surface still exists.
    ASSERT_TRUE(child->renderSurface());

    childDamageRect = child->renderSurface()->damageTracker()->currentDamageRect();
    EXPECT_FLOAT_RECT_EQ(FloatRect(0, 0, 30, 30), childDamageRect);
}

TEST_F(CCDamageTrackerTest, verifyDamageForReplicaMask)
{
    RefPtr<CCLayerImpl> root = createAndSetUpTestTreeWithTwoSurfaces();
    RefPtr<CCLayerImpl> child1 = root->children()[0];
    RefPtr<CCLayerImpl> grandChild1 = child1->children()[0];

    // Changes to a replica's mask should not damage the original surface, because it is
    // not masked. But it does damage the ancestor target surface.

    // Create a reflection about the left edge of grandChild1.
    RefPtr<CCLayerImpl> grandChild1Replica = CCLayerImpl::create(6);
    grandChild1Replica->setPosition(FloatPoint::zero());
    grandChild1Replica->setAnchorPoint(FloatPoint::zero());
    TransformationMatrix reflection;
    reflection.scale3d(-1.0, 1.0, 1.0);
    grandChild1Replica->setTransform(reflection);
    grandChild1->setReplicaLayer(grandChild1Replica);

    // Set up the mask layer on the replica layer
    RefPtr<CCLayerImpl> replicaMaskLayer = CCLayerImpl::create(7);
    replicaMaskLayer->setPosition(FloatPoint::zero());
    replicaMaskLayer->setAnchorPoint(FloatPoint::zero());
    replicaMaskLayer->setBounds(grandChild1->bounds());
    grandChild1Replica->setMaskLayer(replicaMaskLayer);

    emulateDrawingOneFrame(root.get());

    // Sanity check that the appropriate render surfaces were created
    ASSERT_TRUE(grandChild1->renderSurface());

    // CASE 1: a property change on the mask should damage only the reflected region on the target surface.
    replicaMaskLayer->setOpacity(0.6);
    emulateDrawingOneFrame(root.get());

    FloatRect grandChildDamageRect = grandChild1->renderSurface()->damageTracker()->currentDamageRect();
    FloatRect childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();

    EXPECT_TRUE(grandChildDamageRect.isEmpty());
    EXPECT_FLOAT_RECT_EQ(FloatRect(194, 200, 6, 8), childDamageRect);

    // CASE 2: removing the replica mask damages only the reflected region on the target surface.
    //
    grandChild1Replica->setMaskLayer(0);
    emulateDrawingOneFrame(root.get());

    grandChildDamageRect = grandChild1->renderSurface()->damageTracker()->currentDamageRect();
    childDamageRect = child1->renderSurface()->damageTracker()->currentDamageRect();

    EXPECT_TRUE(grandChildDamageRect.isEmpty());
    EXPECT_FLOAT_RECT_EQ(FloatRect(194, 200, 6, 8), childDamageRect);
}

} // namespace
