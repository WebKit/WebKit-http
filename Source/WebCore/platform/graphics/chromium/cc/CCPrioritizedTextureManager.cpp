/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#include "CCPrioritizedTextureManager.h"

#include "CCPrioritizedTexture.h"
#include "CCPriorityCalculator.h"
#include "CCProxy.h"
#include "TraceEvent.h"
#include <algorithm>

using namespace std;

namespace WebCore {

CCPrioritizedTextureManager::CCPrioritizedTextureManager(size_t maxMemoryLimitBytes, int, int pool)
    : m_maxMemoryLimitBytes(maxMemoryLimitBytes)
    , m_memoryUseBytes(0)
    , m_memoryAboveCutoffBytes(0)
    , m_memoryAvailableBytes(0)
    , m_pool(pool)
    , m_needsUpdateBackingsPrioritites(false)
{
}

CCPrioritizedTextureManager::~CCPrioritizedTextureManager()
{
    while (m_textures.size() > 0)
        unregisterTexture(*m_textures.begin());

    deleteEvictedBackings();

    // Each remaining backing is a leaked opengl texture. There should be none.
    ASSERT(m_backings.isEmpty());
}

void CCPrioritizedTextureManager::prioritizeTextures()
{
    TRACE_EVENT0("cc", "CCPrioritizedTextureManager::prioritizeTextures");
    ASSERT(CCProxy::isMainThread());

    // Sorting textures in this function could be replaced by a slightly
    // modified O(n) quick-select to partition textures rather than
    // sort them (if performance of the sort becomes an issue).

    TextureVector& sortedTextures = m_tempTextureVector;
    sortedTextures.clear();

    // Copy all textures into a vector and sort them.
    for (TextureSet::iterator it = m_textures.begin(); it != m_textures.end(); ++it)
        sortedTextures.append(*it);
    std::sort(sortedTextures.begin(), sortedTextures.end(), compareTextures);

    m_memoryAvailableBytes = m_maxMemoryLimitBytes;
    m_priorityCutoff = CCPriorityCalculator::lowestPriority();
    size_t memoryBytes = 0;
    for (TextureVector::iterator it = sortedTextures.begin(); it != sortedTextures.end(); ++it) {
        if ((*it)->requestPriority() == CCPriorityCalculator::lowestPriority())
            break;

        if ((*it)->isSelfManaged()) {
            // Account for self-managed memory immediately by reducing the memory
            // available (since it never gets acquired).
            size_t newMemoryBytes = memoryBytes + (*it)->bytes();
            if (newMemoryBytes > m_memoryAvailableBytes) {
                m_priorityCutoff = (*it)->requestPriority();
                m_memoryAvailableBytes = memoryBytes;
                break;
            }
            m_memoryAvailableBytes -= (*it)->bytes();
        } else {
            size_t newMemoryBytes = memoryBytes + (*it)->bytes();
            if (newMemoryBytes > m_memoryAvailableBytes) {
                m_priorityCutoff = (*it)->requestPriority();
                break;
            }
            memoryBytes = newMemoryBytes;
        }
    }

    // Only allow textures if they are higher than the cutoff. All textures
    // of the same priority are accepted or rejected together, rather than
    // being partially allowed randomly.
    m_memoryAboveCutoffBytes = 0;
    for (TextureVector::iterator it = sortedTextures.begin(); it != sortedTextures.end(); ++it) {
        bool isAbovePriorityCutoff = CCPriorityCalculator::priorityIsHigher((*it)->requestPriority(), m_priorityCutoff);
        (*it)->setAbovePriorityCutoff(isAbovePriorityCutoff);
        if (isAbovePriorityCutoff && !(*it)->isSelfManaged())
            m_memoryAboveCutoffBytes += (*it)->bytes();
    }
    sortedTextures.clear();

    m_needsUpdateBackingsPrioritites = true;

    ASSERT(m_memoryAboveCutoffBytes <= m_memoryAvailableBytes);
    ASSERT(memoryAboveCutoffBytes() <= maxMemoryLimitBytes());
}

void CCPrioritizedTextureManager::updateBackingsPriorities()
{
    TRACE_EVENT0("cc", "CCPrioritizedTextureManager::updateBackingsPriorities");
    ASSERT(CCProxy::isImplThread() && CCProxy::isMainThreadBlocked());

    if (!m_needsUpdateBackingsPrioritites)
        return;

#if !ASSERT_DISABLED
    assertInvariants();
#endif

    // Update backings' priorities and put backings in eviction/recycling order.
    BackingVector& sortedBackings = m_tempBackingVector;
    sortedBackings.clear();
    for (BackingSet::iterator it = m_backings.begin(); it != m_backings.end(); ++it) {
        (*it)->updatePriority();
        sortedBackings.append(*it);
    }
    std::sort(sortedBackings.begin(), sortedBackings.end(), compareBackings);

    for (BackingVector::iterator it = sortedBackings.begin(); it != sortedBackings.end(); ++it) {
        m_backings.remove(*it);
        m_backings.add(*it);
    }
    sortedBackings.clear();
    m_needsUpdateBackingsPrioritites = false;

#if !ASSERT_DISABLED
    assertInvariants();
#endif
}

void CCPrioritizedTextureManager::clearPriorities()
{
    ASSERT(CCProxy::isMainThread());
    for (TextureSet::iterator it = m_textures.begin(); it != m_textures.end(); ++it) {
        // FIXME: We should remove this and just set all priorities to
        //        CCPriorityCalculator::lowestPriority() once we have priorities
        //        for all textures (we can't currently calculate distances for
        //        off-screen textures).
        (*it)->setRequestPriority(CCPriorityCalculator::lingeringPriority((*it)->requestPriority()));
    }
}

bool CCPrioritizedTextureManager::requestLate(CCPrioritizedTexture* texture)
{
    ASSERT(CCProxy::isMainThread());

    // This is already above cutoff, so don't double count it's memory below.
    if (texture->isAbovePriorityCutoff())
        return true;

    if (CCPriorityCalculator::priorityIsLower(texture->requestPriority(), m_priorityCutoff))
        return false;

    size_t newMemoryBytes = m_memoryAboveCutoffBytes + texture->bytes();
    if (newMemoryBytes > m_memoryAvailableBytes)
        return false;

    m_memoryAboveCutoffBytes = newMemoryBytes;
    texture->setAbovePriorityCutoff(true);
    m_needsUpdateBackingsPrioritites = true;
    return true;
}

void CCPrioritizedTextureManager::acquireBackingTextureIfNeeded(CCPrioritizedTexture* texture, CCResourceProvider* resourceProvider)
{
    ASSERT(CCProxy::isImplThread() && CCProxy::isMainThreadBlocked());
    ASSERT(!texture->isSelfManaged());
    ASSERT(texture->isAbovePriorityCutoff());
    if (texture->backing() || !texture->isAbovePriorityCutoff())
        return;

    // Make sure that the backings list is up to date and sorted before traversing it.
    updateBackingsPriorities();

    // Find a backing below, by either recycling or allocating.
    CCPrioritizedTexture::Backing* backing = 0;

    // First try to recycle
    for (BackingSet::iterator it = m_backings.begin(); it != m_backings.end(); ++it) {
        if ((*it)->hadOwnerAtLastPriorityUpdate() && (*it)->wasAbovePriorityCutoffAtLastPriorityUpdate())
            break;
        if ((*it)->size() == texture->size() && (*it)->format() == texture->format()) {
            backing = (*it);
            break;
        }
    }

    // Otherwise reduce memory and just allocate a new backing texures.
    if (!backing) {
        evictBackingsToReduceMemory(m_memoryAvailableBytes - texture->bytes(), RespectManagerPriorityCutoff, resourceProvider);
        backing = createBacking(texture->size(), texture->format(), resourceProvider);
    }

    // Move the used backing texture to the end of the eviction list.
    if (backing->owner())
        backing->owner()->unlink();
    texture->link(backing);
    m_backings.remove(backing);
    m_backings.add(backing);

    // Update the backing's priority from its new owner.
    backing->updatePriority();
}

void CCPrioritizedTextureManager::evictBackingsToReduceMemory(size_t limitBytes, EvictionPriorityPolicy evictionPolicy, CCResourceProvider* resourceProvider)
{
    ASSERT(CCProxy::isImplThread());
    if (memoryUseBytes() <= limitBytes)
        return;

    // Destroy backings until we are below the limit,
    // or until all backings remaining are above the cutoff.
    while (memoryUseBytes() > limitBytes && m_backings.size() > 0) {
        CCPrioritizedTexture::Backing* backing = *m_backings.begin();
        if (evictionPolicy == RespectManagerPriorityCutoff)
            if (backing->hadOwnerAtLastPriorityUpdate() && backing->wasAbovePriorityCutoffAtLastPriorityUpdate())
                break;
        evictBackingResource(backing, resourceProvider);
    }
}

void CCPrioritizedTextureManager::reduceMemory(CCResourceProvider* resourceProvider)
{
    ASSERT(CCProxy::isImplThread() && CCProxy::isMainThreadBlocked());

    // Make sure that the backings list is up to date and sorted before traversing it.
    updateBackingsPriorities();

    evictBackingsToReduceMemory(m_memoryAvailableBytes, RespectManagerPriorityCutoff, resourceProvider);
    ASSERT(memoryUseBytes() <= maxMemoryLimitBytes());

    // We currently collect backings from deleted textures for later recycling.
    // However, if we do that forever we will always use the max limit even if
    // we really need very little memory. This should probably be solved by reducing the
    // limit externally, but until then this just does some "clean up" of unused
    // backing textures (any more than 10%).
    size_t wastedMemory = 0;
    for (BackingSet::iterator it = m_backings.begin(); it != m_backings.end(); ++it) {
        if ((*it)->owner())
            break;
        wastedMemory += (*it)->bytes();
    }
    size_t tenPercentOfMemory = m_memoryAvailableBytes / 10;
    if (wastedMemory > tenPercentOfMemory)
        evictBackingsToReduceMemory(memoryUseBytes() - (wastedMemory - tenPercentOfMemory), RespectManagerPriorityCutoff, resourceProvider);

    deleteEvictedBackings();
}

void CCPrioritizedTextureManager::clearAllMemory(CCResourceProvider* resourceProvider)
{
    ASSERT(CCProxy::isImplThread() && CCProxy::isMainThreadBlocked());
    ASSERT(resourceProvider);
    evictBackingsToReduceMemory(0, DoNotRespectManagerPriorityCutoff, resourceProvider);
    deleteEvictedBackings();
}

void CCPrioritizedTextureManager::reduceMemoryOnImplThread(size_t limitBytes, CCResourceProvider* resourceProvider)
{
    ASSERT(CCProxy::isImplThread());
    ASSERT(resourceProvider);

    evictBackingsToReduceMemory(limitBytes, DoNotRespectManagerPriorityCutoff, resourceProvider);

    // Deleting just some (not all) resources is not supported yet because we do not clear
    // only the deleted resources from the texture upload queues (rather, we clear all uploads).
    // Make sure that if we evict all resources.
    ASSERT(m_backings.isEmpty());
}

void CCPrioritizedTextureManager::getEvictedBackings(BackingVector& evictedBackings)
{
    ASSERT(CCProxy::isImplThread());
    evictedBackings.clear();
    evictedBackings.append(m_evictedBackings);
}

void CCPrioritizedTextureManager::unlinkEvictedBackings(const BackingVector& evictedBackings)
{
    ASSERT(CCProxy::isMainThread());
    for (BackingVector::const_iterator it = evictedBackings.begin(); it != evictedBackings.end(); ++it) {
        CCPrioritizedTexture::Backing* backing = (*it);
        if (backing->owner())
            backing->owner()->unlink();
    }
}

bool CCPrioritizedTextureManager::deleteEvictedBackings()
{
    ASSERT(CCProxy::isMainThread() || (CCProxy::isImplThread() && CCProxy::isMainThreadBlocked()));
    bool linkedEvictedBackingsExisted = false;
    for (BackingVector::const_iterator it = m_evictedBackings.begin(); it != m_evictedBackings.end(); ++it) {
        CCPrioritizedTexture::Backing* backing = (*it);
        if (backing->owner()) {
            linkedEvictedBackingsExisted = true;
            backing->owner()->unlink();
        }
        delete backing;
    }
    m_evictedBackings.clear();
    return linkedEvictedBackingsExisted;
}

void CCPrioritizedTextureManager::registerTexture(CCPrioritizedTexture* texture)
{
    ASSERT(CCProxy::isMainThread());
    ASSERT(texture);
    ASSERT(!texture->textureManager());
    ASSERT(!texture->backing());
    ASSERT(m_textures.find(texture) == m_textures.end());

    texture->setManagerInternal(this);
    m_textures.add(texture);

}

void CCPrioritizedTextureManager::unregisterTexture(CCPrioritizedTexture* texture)
{
    ASSERT(CCProxy::isMainThread() || (CCProxy::isImplThread() && CCProxy::isMainThreadBlocked()));
    ASSERT(texture);
    ASSERT(m_textures.find(texture) != m_textures.end());

    returnBackingTexture(texture);
    texture->setManagerInternal(0);
    m_textures.remove(texture);
    texture->setAbovePriorityCutoff(false);
}

void CCPrioritizedTextureManager::returnBackingTexture(CCPrioritizedTexture* texture)
{
    ASSERT(CCProxy::isMainThread() || (CCProxy::isImplThread() && CCProxy::isMainThreadBlocked()));
    if (texture->backing()) {
        texture->unlink();
        m_needsUpdateBackingsPrioritites = true;
    }
}

CCPrioritizedTexture::Backing* CCPrioritizedTextureManager::createBacking(IntSize size, GC3Denum format, CCResourceProvider* resourceProvider)
{
    ASSERT(CCProxy::isImplThread() && CCProxy::isMainThreadBlocked());
    ASSERT(resourceProvider);
    CCResourceProvider::ResourceId resourceId = resourceProvider->createResource(m_pool, size, format, CCResourceProvider::TextureUsageAny);
    CCPrioritizedTexture::Backing* backing = new CCPrioritizedTexture::Backing(resourceId, size, format);
    m_memoryUseBytes += backing->bytes();
    // Put backing texture at the front for eviction, since it isn't in use yet.
    m_backings.insertBefore(m_backings.begin(), backing);
    return backing;
}

void CCPrioritizedTextureManager::evictBackingResource(CCPrioritizedTexture::Backing* backing, CCResourceProvider* resourceProvider)
{
    ASSERT(CCProxy::isImplThread());
    ASSERT(backing);
    ASSERT(resourceProvider);
    ASSERT(m_backings.find(backing) != m_backings.end());

    resourceProvider->deleteResource(backing->id());
    backing->setId(0);
    m_memoryUseBytes -= backing->bytes();
    m_backings.remove(backing);
    m_evictedBackings.append(backing);
}

#if !ASSERT_DISABLED
void CCPrioritizedTextureManager::assertInvariants()
{
    ASSERT(CCProxy::isImplThread() && CCProxy::isMainThreadBlocked());

    // If we hit any of these asserts, there is a bug in this class. To see
    // where the bug is, call this function at the beginning and end of
    // every public function.

    // Backings/textures must be doubly-linked and only to other backings/textures in this manager.
    for (BackingSet::iterator it = m_backings.begin(); it != m_backings.end(); ++it) {
        if ((*it)->owner()) {
            ASSERT(m_textures.find((*it)->owner()) != m_textures.end());
            ASSERT((*it)->owner()->backing() == (*it));
        }
    }
    for (TextureSet::iterator it = m_textures.begin(); it != m_textures.end(); ++it) {
        if ((*it)->backing()) {
            ASSERT(m_backings.find((*it)->backing()) != m_backings.end());
            ASSERT((*it)->backing()->owner() == (*it));
        }
    }

    // At all times, backings that can be evicted must always come before
    // backings that can't be evicted in the backing texture list (otherwise
    // reduceMemory will not find all textures available for eviction/recycling).
    bool reachedOwned = false;
    bool reachedAboveCutoff = false;
    for (BackingSet::iterator it = m_backings.begin(); it != m_backings.end(); ++it) {
        if ((*it)->hadOwnerAtLastPriorityUpdate())
            reachedOwned = true;
        if ((*it)->wasAbovePriorityCutoffAtLastPriorityUpdate())
            reachedAboveCutoff = true;
        if (reachedOwned)
            ASSERT((*it)->hadOwnerAtLastPriorityUpdate());
        if (reachedAboveCutoff) {
            ASSERT((*it)->hadOwnerAtLastPriorityUpdate() && (*it)->wasAbovePriorityCutoffAtLastPriorityUpdate());
            ASSERT(reachedOwned);
        }
    }
}
#endif

} // namespace WebCore
