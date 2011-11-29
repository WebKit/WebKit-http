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

#include "cc/CCLayerImpl.h"

#include "cc/CCSingleThreadProxy.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace WebCore;

namespace {

#define EXECUTE_AND_VERIFY_SUBTREE_CHANGED(codeToTest)                  \
    root->resetAllChangeTrackingForSubtree();                           \
    codeToTest;                                                         \
    EXPECT_TRUE(root->layerPropertyChanged());                          \
    EXPECT_TRUE(child->layerPropertyChanged());                         \
    EXPECT_TRUE(grandChild->layerPropertyChanged())

#define EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(codeToTest)           \
    root->resetAllChangeTrackingForSubtree();                           \
    codeToTest;                                                         \
    EXPECT_FALSE(root->layerPropertyChanged());                         \
    EXPECT_FALSE(child->layerPropertyChanged());                        \
    EXPECT_FALSE(grandChild->layerPropertyChanged())

#define EXECUTE_AND_VERIFY_ONLY_LAYER_CHANGED(codeToTest)               \
    root->resetAllChangeTrackingForSubtree();                           \
    codeToTest;                                                         \
    EXPECT_TRUE(root->layerPropertyChanged());                          \
    EXPECT_FALSE(child->layerPropertyChanged());                        \
    EXPECT_FALSE(grandChild->layerPropertyChanged())

TEST(CCLayerImplTest, verifyLayerChangesAreTrackedProperly)
{
    //
    // This test checks that layerPropertyChanged() has the correct behavior.
    //

    // The constructor on this will fake that we are on the correct thread.
    DebugScopedSetImplThread setImplThread;

    // Create a simple CCLayerImpl tree:
    RefPtr<CCLayerImpl> root = CCLayerImpl::create(1);
    RefPtr<CCLayerImpl> child = CCLayerImpl::create(2);
    RefPtr<CCLayerImpl> grandChild = CCLayerImpl::create(3);
    root->addChild(child);
    child->addChild(grandChild);

    RefPtr<CCLayerImpl> dummyMask = CCLayerImpl::create(4);
    RefPtr<CCLayerImpl> dummyReplica = CCLayerImpl::create(5);

    // Adding children is an internal operation and should not mark layers as changed.
    EXPECT_FALSE(root->layerPropertyChanged());
    EXPECT_FALSE(child->layerPropertyChanged());
    EXPECT_FALSE(grandChild->layerPropertyChanged());

    FloatPoint arbitraryFloatPoint = FloatPoint(0.125f, 0.25f);
    float arbitraryNumber = 0.352f;
    IntSize arbitraryIntSize = IntSize(111, 222);
    IntPoint arbitraryIntPoint = IntPoint(333, 444);
    IntRect arbitraryIntRect = IntRect(arbitraryIntPoint, arbitraryIntSize);
    FloatRect arbitraryFloatRect = FloatRect(arbitraryFloatPoint, FloatSize(1.234, 5.678));
    Color arbitraryColor = Color(10, 20, 30);
    TransformationMatrix arbitraryTransform;
    arbitraryTransform.scale3d(0.1, 0.2, 0.3);

    // Changing these properties affects the entire subtree of layers.
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setAnchorPoint(arbitraryFloatPoint));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setAnchorPointZ(arbitraryNumber));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setMaskLayer(dummyMask));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setMasksToBounds(true));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setOpaque(true));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setOpacity(arbitraryNumber));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setReplicaLayer(dummyReplica));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setPosition(arbitraryFloatPoint));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setPreserves3D(true));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setTransform(arbitraryTransform));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setZoomAnimatorTransform(arbitraryTransform));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setDoubleSided(false)); // constructor initializes it to "true".
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->scrollBy(arbitraryIntSize));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setScrollDelta(arbitraryIntSize));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setScrollPosition(arbitraryIntPoint));
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setPageScaleDelta(arbitraryNumber));

    // Changing these properties only affects the layer itself.
    EXECUTE_AND_VERIFY_ONLY_LAYER_CHANGED(root->setContentBounds(arbitraryIntSize));
    EXECUTE_AND_VERIFY_ONLY_LAYER_CHANGED(root->setDebugBorderColor(arbitraryColor));
    EXECUTE_AND_VERIFY_ONLY_LAYER_CHANGED(root->setDebugBorderWidth(arbitraryNumber));
    EXECUTE_AND_VERIFY_ONLY_LAYER_CHANGED(root->setDrawsContent(true));
    EXECUTE_AND_VERIFY_ONLY_LAYER_CHANGED(root->setBackgroundColor(Color::gray));

    // Special case: check that sublayer transform changes all layer's descendants, but not the layer itself.
    root->resetAllChangeTrackingForSubtree();
    root->setSublayerTransform(arbitraryTransform);
    EXPECT_FALSE(root->layerPropertyChanged());
    EXPECT_TRUE(child->layerPropertyChanged());
    EXPECT_TRUE(grandChild->layerPropertyChanged());

    // Special case: check that setBounds changes behavior depending on masksToBounds.
    root->setMasksToBounds(false);
    EXECUTE_AND_VERIFY_ONLY_LAYER_CHANGED(root->setBounds(IntSize(135, 246)));
    root->setMasksToBounds(true);
    EXECUTE_AND_VERIFY_SUBTREE_CHANGED(root->setBounds(arbitraryIntSize)); // should be a different size than previous call, to ensure it marks tree changed.

    // After setting all these properties already, setting to the exact same values again should
    // not cause any change.
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setAnchorPoint(arbitraryFloatPoint));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setAnchorPointZ(arbitraryNumber));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setMaskLayer(dummyMask));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setMasksToBounds(true));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setReplicaLayer(dummyReplica));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setPosition(arbitraryFloatPoint));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setPreserves3D(true));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setZoomAnimatorTransform(arbitraryTransform));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setTransform(arbitraryTransform));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setDoubleSided(false)); // constructor initializes it to "true".
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setScrollDelta(arbitraryIntSize));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setScrollPosition(arbitraryIntPoint));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setPageScaleDelta(arbitraryNumber));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setContentBounds(arbitraryIntSize));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setOpaque(true));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setOpacity(arbitraryNumber));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setDebugBorderColor(arbitraryColor));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setDebugBorderWidth(arbitraryNumber));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setDrawsContent(true));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setSublayerTransform(arbitraryTransform));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setBounds(arbitraryIntSize));

    // These properties are internal, and should not be considered "change" when they are used.
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setIsNonCompositedContent(true));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setClipRect(arbitraryIntRect));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setUsesLayerClipping(true));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setDrawOpacity(arbitraryNumber));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setTargetRenderSurface(0));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setDrawTransform(arbitraryTransform));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setScreenSpaceTransform(arbitraryTransform));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setDrawableContentRect(arbitraryIntRect));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setUpdateRect(arbitraryFloatRect));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setVisibleLayerRect(arbitraryIntRect));
    EXECUTE_AND_VERIFY_SUBTREE_DID_NOT_CHANGE(root->setMaxScrollPosition(arbitraryIntSize));
}

} // namespace
