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

#include "CCPrioritizedTexture.h"

#include "CCPrioritizedTextureManager.h"
#include "CCSingleThreadProxy.h" // For DebugScopedSetImplThread
#include "CCTexture.h"
#include "CCTiledLayerTestCommon.h"
#include "FakeCCGraphicsContext.h"
#include "WebCompositorInitializer.h"
#include <gtest/gtest.h>

using namespace WebCore;
using namespace WebKitTests;
using namespace WTF;

namespace WebCore {

class CCPrioritizedTextureTest : public testing::Test {
public:
    CCPrioritizedTextureTest()
        : m_textureSize(256, 256)
        , m_textureFormat(GraphicsContext3D::RGBA)
        , m_compositorInitializer(0)
        , m_context(WebKit::createFakeCCGraphicsContext())
    {
        DebugScopedSetImplThread implThread;
        m_resourceProvider = CCResourceProvider::create(m_context.get());
    }

    virtual ~CCPrioritizedTextureTest()
    {
        DebugScopedSetImplThread implThread;
        m_resourceProvider.clear();
    }

    size_t texturesMemorySize(size_t textureCount)
    {
        return CCTexture::memorySizeBytes(m_textureSize, m_textureFormat) * textureCount;
    }

    PassOwnPtr<CCPrioritizedTextureManager> createManager(size_t maxTextures)
    {
        return CCPrioritizedTextureManager::create(texturesMemorySize(maxTextures), 1024, 0);
    }

    bool validateTexture(OwnPtr<CCPrioritizedTexture>& texture, bool requestLate)
    {
        textureManagerAssertInvariants(texture->textureManager());
        if (requestLate)
            texture->requestLate();
        textureManagerAssertInvariants(texture->textureManager());
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        bool success = texture->canAcquireBackingTexture();
        if (success)
            texture->acquireBackingTexture(resourceProvider());
        return success;
    }

    void prioritizeTexturesAndBackings(CCPrioritizedTextureManager* textureManager)
    {
        textureManager->prioritizeTextures();
        textureManagerUpdateBackingsPriorities(textureManager);
    }

    void textureManagerUpdateBackingsPriorities(CCPrioritizedTextureManager* textureManager)
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManager->updateBackingsPriorities();
    }

    CCResourceProvider* resourceProvider()
    {
       return m_resourceProvider.get();
    }

    void textureManagerAssertInvariants(CCPrioritizedTextureManager* textureManager)
    {
#if !ASSERT_DISABLED
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManager->assertInvariants();
#endif
    }

    bool textureBackingIsAbovePriorityCutoff(CCPrioritizedTexture* texture)
    {
        return texture->m_backing->wasAbovePriorityCutoffAtLastPriorityUpdate();
    }

protected:
    const IntSize m_textureSize;
    const GC3Denum m_textureFormat;
    WebCompositorInitializer m_compositorInitializer;
    OwnPtr<CCGraphicsContext> m_context;
    OwnPtr<CCResourceProvider> m_resourceProvider;
};

}

namespace {

TEST_F(CCPrioritizedTextureTest, requestTextureExceedingMaxLimit)
{
    const size_t maxTextures = 8;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);

    // Create textures for double our memory limit.
    OwnPtr<CCPrioritizedTexture> textures[maxTextures*2];

    for (size_t i = 0; i < maxTextures*2; ++i)
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);

    // Set decreasing priorities
    for (size_t i = 0; i < maxTextures*2; ++i)
        textures[i]->setRequestPriority(100 + i);

    // Only lower half should be available.
    prioritizeTexturesAndBackings(textureManager.get());
    EXPECT_TRUE(validateTexture(textures[0], false));
    EXPECT_TRUE(validateTexture(textures[7], false));
    EXPECT_FALSE(validateTexture(textures[8], false));
    EXPECT_FALSE(validateTexture(textures[15], false));

    // Set increasing priorities
    for (size_t i = 0; i < maxTextures*2; ++i)
        textures[i]->setRequestPriority(100 - i);

    // Only upper half should be available.
    prioritizeTexturesAndBackings(textureManager.get());
    EXPECT_FALSE(validateTexture(textures[0], false));
    EXPECT_FALSE(validateTexture(textures[7], false));
    EXPECT_TRUE(validateTexture(textures[8], false));
    EXPECT_TRUE(validateTexture(textures[15], false));

    EXPECT_EQ(texturesMemorySize(maxTextures), textureManager->memoryAboveCutoffBytes());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, changeMemoryLimits)
{
    const size_t maxTextures = 8;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);
    OwnPtr<CCPrioritizedTexture> textures[maxTextures];

    for (size_t i = 0; i < maxTextures; ++i)
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);
    for (size_t i = 0; i < maxTextures; ++i)
        textures[i]->setRequestPriority(100 + i);

    // Set max limit to 8 textures
    textureManager->setMaxMemoryLimitBytes(texturesMemorySize(8));
    prioritizeTexturesAndBackings(textureManager.get());
    for (size_t i = 0; i < maxTextures; ++i)
        validateTexture(textures[i], false);
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManager->reduceMemory(resourceProvider());
    }

    EXPECT_EQ(texturesMemorySize(8), textureManager->memoryAboveCutoffBytes());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    // Set max limit to 5 textures
    textureManager->setMaxMemoryLimitBytes(texturesMemorySize(5));
    prioritizeTexturesAndBackings(textureManager.get());
    for (size_t i = 0; i < maxTextures; ++i)
        EXPECT_EQ(validateTexture(textures[i], false), i < 5);
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManager->reduceMemory(resourceProvider());
    }

    EXPECT_EQ(texturesMemorySize(5), textureManager->memoryAboveCutoffBytes());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    // Set max limit to 4 textures
    textureManager->setMaxMemoryLimitBytes(texturesMemorySize(4));
    prioritizeTexturesAndBackings(textureManager.get());
    for (size_t i = 0; i < maxTextures; ++i)
        EXPECT_EQ(validateTexture(textures[i], false), i < 4);
    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManager->reduceMemory(resourceProvider());
    }

    EXPECT_EQ(texturesMemorySize(4), textureManager->memoryAboveCutoffBytes());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, textureManagerPartialUpdateTextures)
{
    const size_t maxTextures = 4;
    const size_t numTextures = 4;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);
    OwnPtr<CCPrioritizedTexture> textures[numTextures];
    OwnPtr<CCPrioritizedTexture> moreTextures[numTextures];

    for (size_t i = 0; i < numTextures; ++i) {
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);
        moreTextures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);
    }

    for (size_t i = 0; i < numTextures; ++i)
        textures[i]->setRequestPriority(200 + i);
    prioritizeTexturesAndBackings(textureManager.get());

    // Allocate textures which are currently high priority.
    EXPECT_TRUE(validateTexture(textures[0], false));
    EXPECT_TRUE(validateTexture(textures[1], false));
    EXPECT_TRUE(validateTexture(textures[2], false));
    EXPECT_TRUE(validateTexture(textures[3], false));

    EXPECT_TRUE(textures[0]->haveBackingTexture());
    EXPECT_TRUE(textures[1]->haveBackingTexture());
    EXPECT_TRUE(textures[2]->haveBackingTexture());
    EXPECT_TRUE(textures[3]->haveBackingTexture());

    for (size_t i = 0; i < numTextures; ++i)
        moreTextures[i]->setRequestPriority(100 + i);
    prioritizeTexturesAndBackings(textureManager.get());

    // Textures are now below cutoff.
    EXPECT_FALSE(validateTexture(textures[0], false));
    EXPECT_FALSE(validateTexture(textures[1], false));
    EXPECT_FALSE(validateTexture(textures[2], false));
    EXPECT_FALSE(validateTexture(textures[3], false));

    // But they are still valid to use.
    EXPECT_TRUE(textures[0]->haveBackingTexture());
    EXPECT_TRUE(textures[1]->haveBackingTexture());
    EXPECT_TRUE(textures[2]->haveBackingTexture());
    EXPECT_TRUE(textures[3]->haveBackingTexture());

    // Higher priority textures are finally needed.
    EXPECT_TRUE(validateTexture(moreTextures[0], false));
    EXPECT_TRUE(validateTexture(moreTextures[1], false));
    EXPECT_TRUE(validateTexture(moreTextures[2], false));
    EXPECT_TRUE(validateTexture(moreTextures[3], false));

    // Lower priority have been fully evicted.
    EXPECT_FALSE(textures[0]->haveBackingTexture());
    EXPECT_FALSE(textures[1]->haveBackingTexture());
    EXPECT_FALSE(textures[2]->haveBackingTexture());
    EXPECT_FALSE(textures[3]->haveBackingTexture());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, textureManagerPrioritiesAreEqual)
{
    const size_t maxTextures = 16;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);
    OwnPtr<CCPrioritizedTexture> textures[maxTextures];

    for (size_t i = 0; i < maxTextures; ++i)
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);

    // All 16 textures have the same priority except 2 higher priority.
    for (size_t i = 0; i < maxTextures; ++i)
        textures[i]->setRequestPriority(100);
    textures[0]->setRequestPriority(99);
    textures[1]->setRequestPriority(99);

    // Set max limit to 8 textures
    textureManager->setMaxMemoryLimitBytes(texturesMemorySize(8));
    prioritizeTexturesAndBackings(textureManager.get());

    // The two high priority textures should be available, others should not.
    for (size_t i = 0; i < 2; ++i)
        EXPECT_TRUE(validateTexture(textures[i], false));
    for (size_t i = 2; i < maxTextures; ++i)
        EXPECT_FALSE(validateTexture(textures[i], false));
    EXPECT_EQ(texturesMemorySize(2), textureManager->memoryAboveCutoffBytes());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    // Manually reserving textures should only succeed on the higher priority textures,
    // and on remaining textures up to the memory limit.
    for (size_t i = 0; i < 8; i++)
        EXPECT_TRUE(validateTexture(textures[i], true));
    for (size_t i = 9; i < maxTextures; i++)
        EXPECT_FALSE(validateTexture(textures[i], true));
    EXPECT_EQ(texturesMemorySize(8), textureManager->memoryAboveCutoffBytes());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, textureManagerDestroyedFirst)
{
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(1);
    OwnPtr<CCPrioritizedTexture> texture = textureManager->createTexture(m_textureSize, m_textureFormat);

    // Texture is initially invalid, but it will become available.
    EXPECT_FALSE(texture->haveBackingTexture());

    texture->setRequestPriority(100);
    prioritizeTexturesAndBackings(textureManager.get());

    EXPECT_TRUE(validateTexture(texture, false));
    EXPECT_TRUE(texture->canAcquireBackingTexture());
    EXPECT_TRUE(texture->haveBackingTexture());

    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManager->clearAllMemory(resourceProvider());
    }
    textureManager.clear();

    EXPECT_FALSE(texture->canAcquireBackingTexture());
    EXPECT_FALSE(texture->haveBackingTexture());
}

TEST_F(CCPrioritizedTextureTest, textureMovedToNewManager)
{
    OwnPtr<CCPrioritizedTextureManager> textureManagerOne = createManager(1);
    OwnPtr<CCPrioritizedTextureManager> textureManagerTwo = createManager(1);
    OwnPtr<CCPrioritizedTexture> texture = textureManagerOne->createTexture(m_textureSize, m_textureFormat);

    // Texture is initially invalid, but it will become available.
    EXPECT_FALSE(texture->haveBackingTexture());

    texture->setRequestPriority(100);
    prioritizeTexturesAndBackings(textureManagerOne.get());

    EXPECT_TRUE(validateTexture(texture, false));
    EXPECT_TRUE(texture->canAcquireBackingTexture());
    EXPECT_TRUE(texture->haveBackingTexture());

    texture->setTextureManager(0);

    {
        DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
        textureManagerOne->clearAllMemory(resourceProvider());
    }
    textureManagerOne.clear();

    EXPECT_FALSE(texture->canAcquireBackingTexture());
    EXPECT_FALSE(texture->haveBackingTexture());

    texture->setTextureManager(textureManagerTwo.get());

    prioritizeTexturesAndBackings(textureManagerTwo.get());

    EXPECT_TRUE(validateTexture(texture, false));
    EXPECT_TRUE(texture->canAcquireBackingTexture());
    EXPECT_TRUE(texture->haveBackingTexture());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManagerTwo->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, renderSurfacesReduceMemoryAvailableOutsideRootSurface)
{
    const size_t maxTextures = 8;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);

    // Half of the memory is taken by surfaces (with high priority place-holder)
    OwnPtr<CCPrioritizedTexture> renderSurfacePlaceHolder = textureManager->createTexture(m_textureSize, m_textureFormat);
    renderSurfacePlaceHolder->setToSelfManagedMemoryPlaceholder(texturesMemorySize(4));
    renderSurfacePlaceHolder->setRequestPriority(CCPriorityCalculator::renderSurfacePriority());

    // Create textures to fill our memory limit.
    OwnPtr<CCPrioritizedTexture> textures[maxTextures];

    for (size_t i = 0; i < maxTextures; ++i)
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);

    // Set decreasing non-visible priorities outside root surface.
    for (size_t i = 0; i < maxTextures; ++i)
        textures[i]->setRequestPriority(100 + i);

    // Only lower half should be available.
    prioritizeTexturesAndBackings(textureManager.get());
    EXPECT_TRUE(validateTexture(textures[0], false));
    EXPECT_TRUE(validateTexture(textures[3], false));
    EXPECT_FALSE(validateTexture(textures[4], false));
    EXPECT_FALSE(validateTexture(textures[7], false));

    // Set increasing non-visible priorities outside root surface.
    for (size_t i = 0; i < maxTextures; ++i)
        textures[i]->setRequestPriority(100 - i);

    // Only upper half should be available.
    prioritizeTexturesAndBackings(textureManager.get());
    EXPECT_FALSE(validateTexture(textures[0], false));
    EXPECT_FALSE(validateTexture(textures[3], false));
    EXPECT_TRUE(validateTexture(textures[4], false));
    EXPECT_TRUE(validateTexture(textures[7], false));

    EXPECT_EQ(texturesMemorySize(4), textureManager->memoryAboveCutoffBytes());
    EXPECT_EQ(texturesMemorySize(4), textureManager->memoryForSelfManagedTextures());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, renderSurfacesReduceMemoryAvailableForRequestLate)
{
    const size_t maxTextures = 8;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);

    // Half of the memory is taken by surfaces (with high priority place-holder)
    OwnPtr<CCPrioritizedTexture> renderSurfacePlaceHolder = textureManager->createTexture(m_textureSize, m_textureFormat);
    renderSurfacePlaceHolder->setToSelfManagedMemoryPlaceholder(texturesMemorySize(4));
    renderSurfacePlaceHolder->setRequestPriority(CCPriorityCalculator::renderSurfacePriority());

    // Create textures to fill our memory limit.
    OwnPtr<CCPrioritizedTexture> textures[maxTextures];

    for (size_t i = 0; i < maxTextures; ++i)
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);

    // Set equal priorities.
    for (size_t i = 0; i < maxTextures; ++i)
        textures[i]->setRequestPriority(100);

    // The first four to be requested late will be available.
    prioritizeTexturesAndBackings(textureManager.get());
    for (unsigned i = 0; i < maxTextures; ++i)
        EXPECT_FALSE(validateTexture(textures[i], false));
    for (unsigned i = 0; i < maxTextures; i += 2)
        EXPECT_TRUE(validateTexture(textures[i], true));
    for (unsigned i = 1; i < maxTextures; i += 2)
        EXPECT_FALSE(validateTexture(textures[i], true));

    EXPECT_EQ(texturesMemorySize(4), textureManager->memoryAboveCutoffBytes());
    EXPECT_EQ(texturesMemorySize(4), textureManager->memoryForSelfManagedTextures());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, whenRenderSurfaceNotAvailableTexturesAlsoNotAvailable)
{
    const size_t maxTextures = 8;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);

    // Half of the memory is taken by surfaces (with high priority place-holder)
    OwnPtr<CCPrioritizedTexture> renderSurfacePlaceHolder = textureManager->createTexture(m_textureSize, m_textureFormat);
    renderSurfacePlaceHolder->setToSelfManagedMemoryPlaceholder(texturesMemorySize(4));
    renderSurfacePlaceHolder->setRequestPriority(CCPriorityCalculator::renderSurfacePriority());

    // Create textures to fill our memory limit.
    OwnPtr<CCPrioritizedTexture> textures[maxTextures];

    for (size_t i = 0; i < maxTextures; ++i)
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);

    // Set 6 visible textures in the root surface, and 2 in a child surface.
    for (size_t i = 0; i < 6; ++i)
        textures[i]->setRequestPriority(CCPriorityCalculator::visiblePriority(true));
    for (size_t i = 6; i < 8; ++i)
        textures[i]->setRequestPriority(CCPriorityCalculator::visiblePriority(false));

    prioritizeTexturesAndBackings(textureManager.get());

    // Unable to requestLate textures in the child surface.
    EXPECT_FALSE(validateTexture(textures[6], true));
    EXPECT_FALSE(validateTexture(textures[7], true));

    // Root surface textures are valid.
    for (size_t i = 0; i < 6; ++i)
        EXPECT_TRUE(validateTexture(textures[i], false));

    EXPECT_EQ(texturesMemorySize(6), textureManager->memoryAboveCutoffBytes());
    EXPECT_EQ(texturesMemorySize(2), textureManager->memoryForSelfManagedTextures());
    EXPECT_LE(textureManager->memoryUseBytes(), textureManager->memoryAboveCutoffBytes());

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}

TEST_F(CCPrioritizedTextureTest, requestLateBackingsSorting)
{
    const size_t maxTextures = 8;
    OwnPtr<CCPrioritizedTextureManager> textureManager = createManager(maxTextures);
    textureManager->setMaxMemoryLimitBytes(texturesMemorySize(maxTextures));

    // Create textures to fill our memory limit.
    OwnPtr<CCPrioritizedTexture> textures[maxTextures];
    for (size_t i = 0; i < maxTextures; ++i)
        textures[i] = textureManager->createTexture(m_textureSize, m_textureFormat);

    // Set equal priorities, and allocate backings for all textures.
    for (size_t i = 0; i < maxTextures; ++i)
        textures[i]->setRequestPriority(100);
    prioritizeTexturesAndBackings(textureManager.get());
    for (unsigned i = 0; i < maxTextures; ++i)
        EXPECT_TRUE(validateTexture(textures[i], false));

    // Drop the memory limit and prioritize (none will be above the threshold,
    // but they still have backings because reduceMemory hasn't been called).
    textureManager->setMaxMemoryLimitBytes(texturesMemorySize(maxTextures / 2));
    prioritizeTexturesAndBackings(textureManager.get());

    // Push half of them back over the limit.
    for (size_t i = 0; i < maxTextures; i += 2)
        EXPECT_TRUE(textures[i]->requestLate());

    // Push the priorities to the backings array and sort the backings array
    textureManagerUpdateBackingsPriorities(textureManager.get());

    // Assert that the backings list be sorted with the below-limit backings
    // before the above-limit backings.
    textureManagerAssertInvariants(textureManager.get());

    // Make sure that we have backings for all of the textures.
    for (size_t i = 0; i < maxTextures; ++i)
        EXPECT_TRUE(textures[i]->haveBackingTexture());

    // Make sure that only the requestLate textures are above the priority cutoff
    for (size_t i = 0; i < maxTextures; i += 2)
        EXPECT_TRUE(textureBackingIsAbovePriorityCutoff(textures[i].get()));
    for (size_t i = 1; i < maxTextures; i += 2)
        EXPECT_FALSE(textureBackingIsAbovePriorityCutoff(textures[i].get()));

    DebugScopedSetImplThreadAndMainThreadBlocked implThreadAndMainThreadBlocked;
    textureManager->clearAllMemory(resourceProvider());
}


} // namespace
