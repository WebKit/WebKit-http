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

#ifndef MarkedBlock_h
#define MarkedBlock_h

#include "CardSet.h"
#include "HeapBlock.h"

#include "WeakSet.h"
#include <wtf/Bitmap.h>
#include <wtf/DataLog.h>
#include <wtf/DoublyLinkedList.h>
#include <wtf/HashFunctions.h>
#include <wtf/PageAllocationAligned.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

// Set to log state transitions of blocks.
#define HEAP_LOG_BLOCK_STATE_TRANSITIONS 0

#if HEAP_LOG_BLOCK_STATE_TRANSITIONS
#define HEAP_LOG_BLOCK_STATE_TRANSITION(block) do {                     \
        dataLog(                                                    \
            "%s:%d %s: block %s = %p, %d\n",                            \
            __FILE__, __LINE__, __FUNCTION__,                           \
            #block, (block), (block)->m_state);                         \
    } while (false)
#else
#define HEAP_LOG_BLOCK_STATE_TRANSITION(block) ((void)0)
#endif

namespace JSC {
    
    class Heap;
    class JSCell;

    typedef uintptr_t Bits;

    static const size_t MB = 1024 * 1024;
    
    bool isZapped(const JSCell*);
    
    // A marked block is a page-aligned container for heap-allocated objects.
    // Objects are allocated within cells of the marked block. For a given
    // marked block, all cells have the same size. Objects smaller than the
    // cell size may be allocated in the marked block, in which case the
    // allocation suffers from internal fragmentation: wasted space whose
    // size is equal to the difference between the cell size and the object
    // size.

    class MarkedBlock : public HeapBlock<MarkedBlock> {
    public:
        // Ensure natural alignment for native types whilst recognizing that the smallest
        // object the heap will commonly allocate is four words.
        static const size_t atomSize = 4 * sizeof(void*);
        static const size_t atomShift = 5;
        static const size_t blockSize = 64 * KB;
        static const size_t blockMask = ~(blockSize - 1); // blockSize must be a power of two.

        static const size_t atomsPerBlock = blockSize / atomSize; // ~0.4% overhead
        static const size_t atomMask = atomsPerBlock - 1;
        static const int cardShift = 8; // This is log2 of bytes per card.
        static const size_t bytesPerCard = 1 << cardShift;
        static const int cardCount = blockSize / bytesPerCard;
        static const int cardMask = cardCount - 1;

        struct FreeCell {
            FreeCell* next;
        };
        
        struct FreeList {
            FreeCell* head;
            size_t bytes;

            FreeList();
            FreeList(FreeCell*, size_t);
        };

        struct VoidFunctor {
            typedef void ReturnType;
            void returnValue() { }
        };

        class CountFunctor {
        public:
            typedef size_t ReturnType;

            CountFunctor() : m_count(0) { }
            void count(size_t count) { m_count += count; }
            ReturnType returnValue() { return m_count; }

        private:
            ReturnType m_count;
        };

        static MarkedBlock* create(const PageAllocationAligned&, Heap*, size_t cellSize, bool cellsNeedDestruction, bool onlyContainsStructures);

        static bool isAtomAligned(const void*);
        static MarkedBlock* blockFor(const void*);
        static size_t firstAtom();
        
        void lastChanceToFinalize();

        Heap* heap() const;
        JSGlobalData* globalData() const;
        WeakSet& weakSet();
        
        enum SweepMode { SweepOnly, SweepToFreeList };
        FreeList sweep(SweepMode = SweepOnly);

        void shrink();

        void visitWeakSet(HeapRootVisitor&);
        void reapWeakSet();

        // While allocating from a free list, MarkedBlock temporarily has bogus
        // cell liveness data. To restore accurate cell liveness data, call one
        // of these functions:
        void didConsumeFreeList(); // Call this once you've allocated all the items in the free list.
        void canonicalizeCellLivenessData(const FreeList&);

        void clearMarks();
        size_t markCount();
        bool isEmpty();

        size_t cellSize();
        bool cellsNeedDestruction();
        bool onlyContainsStructures();

        size_t size();
        size_t capacity();

        bool isMarked(const void*);
        bool testAndSetMarked(const void*);
        bool isLive(const JSCell*);
        bool isLiveCell(const void*);
        void setMarked(const void*);
        void clearMarked(const void*);

        bool needsSweeping();

#if ENABLE(GGC)
        void setDirtyObject(const void* atom)
        {
            ASSERT(MarkedBlock::blockFor(atom) == this);
            m_cards.markCardForAtom(atom);
        }

        uint8_t* addressOfCardFor(const void* atom)
        {
            ASSERT(MarkedBlock::blockFor(atom) == this);
            return &m_cards.cardForAtom(atom);
        }

        static inline size_t offsetOfCards()
        {
            return OBJECT_OFFSETOF(MarkedBlock, m_cards);
        }

        static inline size_t offsetOfMarks()
        {
            return OBJECT_OFFSETOF(MarkedBlock, m_marks);
        }

        typedef Vector<JSCell*, 32> DirtyCellVector;
        inline void gatherDirtyCells(DirtyCellVector&);
        template <int size> inline void gatherDirtyCellsWithSize(DirtyCellVector&);
#endif

        template <typename Functor> void forEachCell(Functor&);
        template <typename Functor> void forEachLiveCell(Functor&);
        template <typename Functor> void forEachDeadCell(Functor&);

    private:
        static const size_t atomAlignmentMask = atomSize - 1; // atomSize must be a power of two.

        enum BlockState { New, FreeListed, Allocated, Marked };
        template<bool destructorCallNeeded> FreeList sweepHelper(SweepMode = SweepOnly);

        typedef char Atom[atomSize];

        MarkedBlock(const PageAllocationAligned&, Heap*, size_t cellSize, bool cellsNeedDestruction, bool onlyContainsStructures);
        Atom* atoms();
        size_t atomNumber(const void*);
        void callDestructor(JSCell*);
        template<BlockState, SweepMode, bool destructorCallNeeded> FreeList specializedSweep();
        
#if ENABLE(GGC)
        CardSet<bytesPerCard, blockSize> m_cards;
#endif

        size_t m_atomsPerCell;
        size_t m_endAtom; // This is a fuzzy end. Always test for < m_endAtom.
#if ENABLE(PARALLEL_GC)
        WTF::Bitmap<atomsPerBlock, WTF::BitmapAtomic> m_marks;
#else
        WTF::Bitmap<atomsPerBlock, WTF::BitmapNotAtomic> m_marks;
#endif
        bool m_cellsNeedDestruction;
        bool m_onlyContainsStructures;
        BlockState m_state;
        WeakSet m_weakSet;
    };

    inline MarkedBlock::FreeList::FreeList()
        : head(0)
        , bytes(0)
    {
    }

    inline MarkedBlock::FreeList::FreeList(FreeCell* head, size_t bytes)
        : head(head)
        , bytes(bytes)
    {
    }

    inline size_t MarkedBlock::firstAtom()
    {
        return WTF::roundUpToMultipleOf<atomSize>(sizeof(MarkedBlock)) / atomSize;
    }

    inline MarkedBlock::Atom* MarkedBlock::atoms()
    {
        return reinterpret_cast<Atom*>(this);
    }

    inline bool MarkedBlock::isAtomAligned(const void* p)
    {
        return !(reinterpret_cast<Bits>(p) & atomAlignmentMask);
    }

    inline MarkedBlock* MarkedBlock::blockFor(const void* p)
    {
        return reinterpret_cast<MarkedBlock*>(reinterpret_cast<Bits>(p) & blockMask);
    }

    inline void MarkedBlock::lastChanceToFinalize()
    {
        m_weakSet.lastChanceToFinalize();

        clearMarks();
        sweep();
    }

    inline Heap* MarkedBlock::heap() const
    {
        return m_weakSet.heap();
    }

    inline JSGlobalData* MarkedBlock::globalData() const
    {
        return m_weakSet.globalData();
    }

    inline WeakSet& MarkedBlock::weakSet()
    {
        return m_weakSet;
    }

    inline void MarkedBlock::shrink()
    {
        m_weakSet.shrink();
    }

    inline void MarkedBlock::visitWeakSet(HeapRootVisitor& heapRootVisitor)
    {
        m_weakSet.visit(heapRootVisitor);
    }

    inline void MarkedBlock::reapWeakSet()
    {
        m_weakSet.reap();
    }

    inline void MarkedBlock::didConsumeFreeList()
    {
        HEAP_LOG_BLOCK_STATE_TRANSITION(this);

        ASSERT(m_state == FreeListed);
        m_state = Allocated;
    }

    inline void MarkedBlock::clearMarks()
    {
        HEAP_LOG_BLOCK_STATE_TRANSITION(this);

        ASSERT(m_state != New && m_state != FreeListed);
        m_marks.clearAll();

        // This will become true at the end of the mark phase. We set it now to
        // avoid an extra pass to do so later.
        m_state = Marked;
    }

    inline size_t MarkedBlock::markCount()
    {
        return m_marks.count();
    }

    inline bool MarkedBlock::isEmpty()
    {
        return m_marks.isEmpty() && m_weakSet.isEmpty();
    }

    inline size_t MarkedBlock::cellSize()
    {
        return m_atomsPerCell * atomSize;
    }

    inline bool MarkedBlock::cellsNeedDestruction()
    {
        return m_cellsNeedDestruction; 
    }

    inline bool MarkedBlock::onlyContainsStructures()
    {
        return m_onlyContainsStructures;
    }

    inline size_t MarkedBlock::size()
    {
        return markCount() * cellSize();
    }

    inline size_t MarkedBlock::capacity()
    {
        return allocation().size();
    }

    inline size_t MarkedBlock::atomNumber(const void* p)
    {
        return (reinterpret_cast<Bits>(p) - reinterpret_cast<Bits>(this)) / atomSize;
    }

    inline bool MarkedBlock::isMarked(const void* p)
    {
        return m_marks.get(atomNumber(p));
    }

    inline bool MarkedBlock::testAndSetMarked(const void* p)
    {
        return m_marks.concurrentTestAndSet(atomNumber(p));
    }

    inline void MarkedBlock::setMarked(const void* p)
    {
        m_marks.set(atomNumber(p));
    }

    inline void MarkedBlock::clearMarked(const void* p)
    {
        ASSERT(m_marks.get(atomNumber(p)));
        m_marks.clear(atomNumber(p));
    }

    inline bool MarkedBlock::isLive(const JSCell* cell)
    {
        switch (m_state) {
        case Allocated:
            return true;

        case Marked:
            return m_marks.get(atomNumber(cell));

        case New:
        case FreeListed:
            ASSERT_NOT_REACHED();
            return false;
        }

        ASSERT_NOT_REACHED();
        return false;
    }

    inline bool MarkedBlock::isLiveCell(const void* p)
    {
        ASSERT(MarkedBlock::isAtomAligned(p));
        size_t atomNumber = this->atomNumber(p);
        size_t firstAtom = this->firstAtom();
        if (atomNumber < firstAtom) // Filters pointers into MarkedBlock metadata.
            return false;
        if ((atomNumber - firstAtom) % m_atomsPerCell) // Filters pointers into cell middles.
            return false;
        if (atomNumber >= m_endAtom) // Filters pointers into invalid cells out of the range.
            return false;

        return isLive(static_cast<const JSCell*>(p));
    }

    template <typename Functor> inline void MarkedBlock::forEachCell(Functor& functor)
    {
        for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell) {
            JSCell* cell = reinterpret_cast_ptr<JSCell*>(&atoms()[i]);
            functor(cell);
        }
    }

    template <typename Functor> inline void MarkedBlock::forEachLiveCell(Functor& functor)
    {
        for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell) {
            JSCell* cell = reinterpret_cast_ptr<JSCell*>(&atoms()[i]);
            if (!isLive(cell))
                continue;

            functor(cell);
        }
    }

    template <typename Functor> inline void MarkedBlock::forEachDeadCell(Functor& functor)
    {
        for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell) {
            JSCell* cell = reinterpret_cast_ptr<JSCell*>(&atoms()[i]);
            if (isLive(cell))
                continue;

            functor(cell);
        }
    }

    inline bool MarkedBlock::needsSweeping()
    {
        return m_state == Marked;
    }

#if ENABLE(GGC)
template <int _cellSize> void MarkedBlock::gatherDirtyCellsWithSize(DirtyCellVector& dirtyCells)
{
    if (m_cards.testAndClear(0)) {
        char* ptr = reinterpret_cast<char*>(&atoms()[firstAtom()]);
        const char* end = reinterpret_cast<char*>(this) + bytesPerCard;
        while (ptr < end) {
            JSCell* cell = reinterpret_cast<JSCell*>(ptr);
            if (isMarked(cell))
                dirtyCells.append(cell);
            ptr += _cellSize;
        }
    }
    
    const size_t cellOffset = firstAtom() * atomSize % _cellSize;
    for (size_t i = 1; i < m_cards.cardCount; i++) {
        if (!m_cards.testAndClear(i))
            continue;
        char* ptr = reinterpret_cast<char*>(this) + i * bytesPerCard + cellOffset;
        char* end = reinterpret_cast<char*>(this) + (i + 1) * bytesPerCard;
        
        while (ptr < end) {
            JSCell* cell = reinterpret_cast<JSCell*>(ptr);
            if (isMarked(cell))
                dirtyCells.append(cell);
            ptr += _cellSize;
        }
    }
}

void MarkedBlock::gatherDirtyCells(DirtyCellVector& dirtyCells)
{
    COMPILE_ASSERT((int)m_cards.cardCount == (int)cardCount, MarkedBlockCardCountsMatch);

    ASSERT(m_state != New && m_state != FreeListed);
    
    // This is an optimisation to avoid having to walk the set of marked
    // blocks twice during GC.
    m_state = Marked;
    
    if (isEmpty())
        return;
    
    size_t cellSize = this->cellSize();
    if (cellSize == 32) {
        gatherDirtyCellsWithSize<32>(dirtyCells);
        return;
    }
    if (cellSize == 64) {
        gatherDirtyCellsWithSize<64>(dirtyCells);
        return;
    }

    const size_t firstCellOffset = firstAtom() * atomSize % cellSize;
    
    if (m_cards.testAndClear(0)) {
        char* ptr = reinterpret_cast<char*>(this) + firstAtom() * atomSize;
        char* end = reinterpret_cast<char*>(this) + bytesPerCard;
        while (ptr < end) {
            JSCell* cell = reinterpret_cast<JSCell*>(ptr);
            if (isMarked(cell))
                dirtyCells.append(cell);
            ptr += cellSize;
        }
    }
    for (size_t i = 1; i < m_cards.cardCount; i++) {
        if (!m_cards.testAndClear(i))
            continue;
        char* ptr = reinterpret_cast<char*>(this) + firstCellOffset + cellSize * ((i * bytesPerCard + cellSize - 1 - firstCellOffset) / cellSize);
        char* end = reinterpret_cast<char*>(this) + std::min((i + 1) * bytesPerCard, m_endAtom * atomSize);
        
        while (ptr < end) {
            JSCell* cell = reinterpret_cast<JSCell*>(ptr);
            if (isMarked(cell))
                dirtyCells.append(cell);
            ptr += cellSize;
        }
    }
}
#endif

} // namespace JSC

namespace WTF {

    struct MarkedBlockHash : PtrHash<JSC::MarkedBlock*> {
        static unsigned hash(JSC::MarkedBlock* const& key)
        {
            // Aligned VM regions tend to be monotonically increasing integers,
            // which is a great hash function, but we have to remove the low bits,
            // since they're always zero, which is a terrible hash function!
            return reinterpret_cast<JSC::Bits>(key) / JSC::MarkedBlock::blockSize;
        }
    };

    template<> struct DefaultHash<JSC::MarkedBlock*> {
        typedef MarkedBlockHash Hash;
    };

} // namespace WTF

#endif // MarkedBlock_h
