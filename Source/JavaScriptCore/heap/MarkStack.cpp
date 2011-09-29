/*
 * Copyright (C) 2009, 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "MarkStack.h"

#include "ConservativeRoots.h"
#include "Heap.h"
#include "JSArray.h"
#include "JSCell.h"
#include "JSObject.h"
#include "ScopeChain.h"
#include "Structure.h"

namespace JSC {

void MarkStack::reset()
{
    m_values.shrinkAllocation(pageSize());
    m_markSets.shrinkAllocation(pageSize());
    m_opaqueRoots.clear();
}

void MarkStack::append(ConservativeRoots& conservativeRoots)
{
    JSCell** roots = conservativeRoots.roots();
    size_t size = conservativeRoots.size();
    for (size_t i = 0; i < size; ++i)
        internalAppend(roots[i]);
}

void SlotVisitor::visitChildren(JSCell* cell)
{
#if ENABLE(SIMPLE_HEAP_PROFILING)
    m_visitedTypeCounts.count(cell);
#endif

    ASSERT(Heap::isMarked(cell));
    if (cell->structure()->typeInfo().type() < CompoundType) {
        JSCell::visitChildren(cell, *this);
        return;
    }

    if (!cell->structure()->typeInfo().overridesVisitChildren()) {
        ASSERT(cell->isObject());
#ifdef NDEBUG
        asObject(cell)->visitChildrenDirect(*this);
#else
        ASSERT(!m_isCheckingForDefaultMarkViolation);
        m_isCheckingForDefaultMarkViolation = true;
        cell->methodTable()->visitChildren(cell, *this);
        ASSERT(m_isCheckingForDefaultMarkViolation);
        m_isCheckingForDefaultMarkViolation = false;
#endif
        return;
    }
    if (cell->vptr() == m_jsArrayVPtr) {
        asArray(cell)->visitChildrenDirect(*this);
        return;
    }
    cell->methodTable()->visitChildren(cell, *this);
}

void SlotVisitor::drain()
{
#if !ASSERT_DISABLED
    ASSERT(!m_isDraining);
    m_isDraining = true;
#endif
    while (!m_markSets.isEmpty() || !m_values.isEmpty()) {
        while (!m_markSets.isEmpty() && m_values.size() < 50) {
            ASSERT(!m_markSets.isEmpty());
            MarkSet& current = m_markSets.last();
            ASSERT(current.m_values);
            JSValue* end = current.m_end;
            ASSERT(current.m_values);
            ASSERT(current.m_values != end);
        findNextUnmarkedNullValue:
            ASSERT(current.m_values != end);
            JSValue value = *current.m_values;
            current.m_values++;

            JSCell* cell;
            if (!value || !value.isCell() || Heap::testAndSetMarked(cell = value.asCell())) {
                if (current.m_values == end) {
                    m_markSets.removeLast();
                    continue;
                }
                goto findNextUnmarkedNullValue;
            }

            if (cell->structure()->typeInfo().type() < CompoundType) {
#if ENABLE(SIMPLE_HEAP_PROFILING)
                m_visitedTypeCounts.count(cell);
#endif
                JSCell::visitChildren(cell, *this);
                if (current.m_values == end) {
                    m_markSets.removeLast();
                    continue;
                }
                goto findNextUnmarkedNullValue;
            }

            if (current.m_values == end)
                m_markSets.removeLast();

            visitChildren(cell);
        }
        while (!m_values.isEmpty())
            visitChildren(m_values.removeLast());
    }
#if !ASSERT_DISABLED
    m_isDraining = false;
#endif
}

void SlotVisitor::harvestWeakReferences()
{
    while (m_firstWeakReferenceHarvester) {
        WeakReferenceHarvester* current = m_firstWeakReferenceHarvester;
        WeakReferenceHarvester* next = reinterpret_cast<WeakReferenceHarvester*>(current->m_nextAndFlag & ~1);
        current->m_nextAndFlag = 0;
        m_firstWeakReferenceHarvester = next;
        current->visitWeakReferences(*this);
    }
}

#if ENABLE(GC_VALIDATION)
void MarkStack::validateSet(JSValue* values, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (values[i])
            validateValue(values[i]);
    }
}

void MarkStack::validateValue(JSValue value)
{
    if (!value)
        CRASH();
    if (!value.isCell())
        return;
    JSCell* cell = value.asCell();
    if (!cell)
        CRASH();

    if (!cell->structure())
        CRASH();

    // Both the cell's structure, and the cell's structure's structure should be the Structure Structure.
    // I hate this sentence.
    if (cell->structure()->structure()->JSCell::classInfo() != cell->structure()->JSCell::classInfo())
        CRASH();
}
#else
void MarkStack::validateValue(JSValue)
{
} 
#endif

} // namespace JSC
