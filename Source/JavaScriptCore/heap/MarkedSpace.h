/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef MarkedSpace_h
#define MarkedSpace_h

#include "MachineStackMarker.h"
#include "MarkedAllocator.h"
#include "MarkedBlock.h"
#include "MarkedBlockSet.h"
#include <wtf/PageAllocationAligned.h>
#include <wtf/Bitmap.h>
#include <wtf/DoublyLinkedList.h>
#include <wtf/FixedArray.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

#define ASSERT_CLASS_FITS_IN_CELL(class) COMPILE_ASSERT(sizeof(class) <= MarkedSpace::maxCellSize, class_fits_in_cell)

namespace JSC {

class Heap;
class JSCell;
class LiveObjectIterator;
class LLIntOffsetsExtractor;
class WeakGCHandle;
class SlotVisitor;

struct ClearMarks : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->clearMarks(); }
};

struct Sweep : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->sweep(); }
};

struct MarkCount : MarkedBlock::CountFunctor {
    void operator()(MarkedBlock* block) { count(block->markCount()); }
};

struct Size : MarkedBlock::CountFunctor {
    void operator()(MarkedBlock* block) { count(block->markCount() * block->cellSize()); }
};

struct Capacity : MarkedBlock::CountFunctor {
    void operator()(MarkedBlock* block) { count(block->capacity()); }
};

class MarkedSpace {
    WTF_MAKE_NONCOPYABLE(MarkedSpace);
public:
    static const size_t maxCellSize = 2048;

    MarkedSpace(Heap*);
    ~MarkedSpace();
    void lastChanceToFinalize();

    MarkedAllocator& firstAllocator();
    MarkedAllocator& allocatorFor(size_t);
    MarkedAllocator& allocatorFor(MarkedBlock*);
    MarkedAllocator& destructorAllocatorFor(size_t);
    void* allocateWithDestructor(size_t);
    void* allocateWithoutDestructor(size_t);
    void* allocateStructure(size_t);
 
    void resetAllocators();

    void visitWeakSets(HeapRootVisitor&);
    void reapWeakSets();

    MarkedBlockSet& blocks() { return m_blocks; }
    
    void canonicalizeCellLivenessData();

    typedef HashSet<MarkedBlock*>::iterator BlockIterator;
    
    template<typename Functor> typename Functor::ReturnType forEachLiveCell(Functor&);
    template<typename Functor> typename Functor::ReturnType forEachLiveCell();
    template<typename Functor> typename Functor::ReturnType forEachBlock(Functor&);
    template<typename Functor> typename Functor::ReturnType forEachBlock();
    
    void shrink();
    void freeBlock(MarkedBlock*);
    void freeOrShrinkBlock(MarkedBlock*);

    void didAddBlock(MarkedBlock*);
    void didConsumeFreeList(MarkedBlock*);

    void clearMarks();
    void sweep();
    size_t objectCount();
    size_t size();
    size_t capacity();

    bool isPagedOut(double deadline);

private:
    friend class LLIntOffsetsExtractor;

    // [ 32... 512 ]
    static const size_t preciseStep = MarkedBlock::atomSize;
    static const size_t preciseCutoff = 512;
    static const size_t preciseCount = preciseCutoff / preciseStep;

    // [ 1024... blockSize ]
    static const size_t impreciseStep = 2 * preciseCutoff;
    static const size_t impreciseCutoff = MarkedBlock::blockSize / 2;
    static const size_t impreciseCount = impreciseCutoff / impreciseStep;

    struct Subspace {
        FixedArray<MarkedAllocator, preciseCount> preciseAllocators;
        FixedArray<MarkedAllocator, impreciseCount> impreciseAllocators;
    };

    Subspace m_destructorSpace;
    Subspace m_normalSpace;
    MarkedAllocator m_largeAllocator;
    MarkedAllocator m_structureAllocator;

    Heap* m_heap;
    MarkedBlockSet m_blocks;
};

template<typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachLiveCell(Functor& functor)
{
    canonicalizeCellLivenessData();

    BlockIterator end = m_blocks.set().end();
    for (BlockIterator it = m_blocks.set().begin(); it != end; ++it)
        (*it)->forEachLiveCell(functor);
    return functor.returnValue();
}

template<typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachLiveCell()
{
    Functor functor;
    return forEachLiveCell(functor);
}

inline MarkedAllocator& MarkedSpace::firstAllocator()
{
    return m_normalSpace.preciseAllocators[0];
}

inline MarkedAllocator& MarkedSpace::allocatorFor(size_t bytes)
{
    ASSERT(bytes);
    if (bytes <= preciseCutoff)
        return m_normalSpace.preciseAllocators[(bytes - 1) / preciseStep];
    if (bytes <= impreciseCutoff)
        return m_normalSpace.impreciseAllocators[(bytes - 1) / impreciseStep];
    return m_largeAllocator;
}

inline MarkedAllocator& MarkedSpace::allocatorFor(MarkedBlock* block)
{
    if (block->onlyContainsStructures())
        return m_structureAllocator;

    if (block->cellsNeedDestruction())
        return destructorAllocatorFor(block->cellSize());

    return allocatorFor(block->cellSize());
}

inline MarkedAllocator& MarkedSpace::destructorAllocatorFor(size_t bytes)
{
    ASSERT(bytes);
    if (bytes <= preciseCutoff)
        return m_destructorSpace.preciseAllocators[(bytes - 1) / preciseStep];
    if (bytes <= impreciseCutoff)
        return m_normalSpace.impreciseAllocators[(bytes - 1) / impreciseStep];
    return m_largeAllocator;
}

inline void* MarkedSpace::allocateWithoutDestructor(size_t bytes)
{
    return allocatorFor(bytes).allocate(bytes);
}

inline void* MarkedSpace::allocateWithDestructor(size_t bytes)
{
    return destructorAllocatorFor(bytes).allocate(bytes);
}

inline void* MarkedSpace::allocateStructure(size_t bytes)
{
    return m_structureAllocator.allocate(bytes);
}

template <typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachBlock(Functor& functor)
{
    for (size_t i = 0; i < preciseCount; ++i) {
        m_normalSpace.preciseAllocators[i].forEachBlock(functor);
        m_destructorSpace.preciseAllocators[i].forEachBlock(functor);
    }

    for (size_t i = 0; i < impreciseCount; ++i) {
        m_normalSpace.impreciseAllocators[i].forEachBlock(functor);
        m_destructorSpace.impreciseAllocators[i].forEachBlock(functor);
    }

    m_largeAllocator.forEachBlock(functor);
    m_structureAllocator.forEachBlock(functor);

    return functor.returnValue();
}

template <typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachBlock()
{
    Functor functor;
    return forEachBlock(functor);
}

inline void MarkedSpace::didAddBlock(MarkedBlock* block)
{
    m_blocks.add(block);
}

inline void MarkedSpace::clearMarks()
{
    forEachBlock<ClearMarks>();
}

inline size_t MarkedSpace::objectCount()
{
    return forEachBlock<MarkCount>();
}

inline size_t MarkedSpace::size()
{
    return forEachBlock<Size>();
}

inline size_t MarkedSpace::capacity()
{
    return forEachBlock<Capacity>();
}

} // namespace JSC

#endif // MarkedSpace_h
