/*
 * Copyright (C) 2012, 2014 Apple Inc. All rights reserved.
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

#ifndef PolymorphicPutByIdList_h
#define PolymorphicPutByIdList_h

#if ENABLE(JIT)

#include "CodeOrigin.h"
#include "MacroAssembler.h"
#include "Opcode.h"
#include "PutKind.h"
#include "PutPropertySlot.h"
#include "Structure.h"
#include <wtf/Vector.h>

namespace JSC {

class CodeBlock;
struct StructureStubInfo;

class PutByIdAccess {
public:
    enum AccessType {
        Invalid,
        Transition,
        Replace,
        Setter,
        CustomSetter
    };
    
    PutByIdAccess()
        : m_type(Invalid)
    {
    }
    
    static PutByIdAccess transition(
        VM& vm,
        JSCell* owner,
        Structure* oldStructure,
        Structure* newStructure,
        StructureChain* chain,
        PassRefPtr<JITStubRoutine> stubRoutine)
    {
        PutByIdAccess result;
        result.m_type = Transition;
        result.m_oldStructure.set(vm, owner, oldStructure);
        result.m_newStructure.set(vm, owner, newStructure);
        result.m_chain.set(vm, owner, chain);
        result.m_customSetter = 0;
        result.m_stubRoutine = stubRoutine;
        return result;
    }

    static PutByIdAccess replace(
        VM& vm,
        JSCell* owner,
        Structure* structure,
        PassRefPtr<JITStubRoutine> stubRoutine)
    {
        PutByIdAccess result;
        result.m_type = Replace;
        result.m_oldStructure.set(vm, owner, structure);
        result.m_customSetter = 0;
        result.m_stubRoutine = stubRoutine;
        return result;
    }


    static PutByIdAccess setter(
        VM& vm,
        JSCell* owner,
        AccessType accessType,
        Structure* structure,
        StructureChain* chain,
        PutPropertySlot::PutValueFunc customSetter,
        PassRefPtr<JITStubRoutine> stubRoutine)
    {
        RELEASE_ASSERT(accessType == Setter || accessType == CustomSetter);
        PutByIdAccess result;
        result.m_oldStructure.set(vm, owner, structure);
        result.m_type = accessType;
        if (chain)
            result.m_chain.set(vm, owner, chain);
        result.m_customSetter = customSetter;
        result.m_stubRoutine = stubRoutine;
        return result;
    }
    
    static PutByIdAccess fromStructureStubInfo(StructureStubInfo&);
    
    bool isSet() const { return m_type != Invalid; }
    bool operator!() const { return !isSet(); }
    
    AccessType type() const { return m_type; }
    
    bool isTransition() const { return m_type == Transition; }
    bool isReplace() const { return m_type == Replace; }
    bool isSetter() const { return m_type == Setter; }
    bool isCustom() const { return m_type == CustomSetter; }
    
    Structure* oldStructure() const
    {
        // Using this instead of isSet() to make this assertion robust against the possibility
        // of additional access types being added.
        ASSERT(isTransition() || isReplace() || isSetter() || isCustom());
        
        return m_oldStructure.get();
    }
    
    Structure* structure() const
    {
        ASSERT(isReplace());
        return m_oldStructure.get();
    }
    
    Structure* newStructure() const
    {
        ASSERT(isTransition());
        return m_newStructure.get();
    }
    
    StructureChain* chain() const
    {
        ASSERT(isTransition() || isSetter() || isCustom());
        return m_chain.get();
    }
    
    JITStubRoutine* stubRoutine() const
    {
        ASSERT(isTransition() || isReplace() || isSetter() || isCustom());
        return m_stubRoutine.get();
    }

    PutPropertySlot::PutValueFunc customSetter() const
    {
        ASSERT(isCustom());
        return m_customSetter;
    }

    bool visitWeak(RepatchBuffer&) const;
    
private:
    friend class CodeBlock;
    
    AccessType m_type;
    WriteBarrier<Structure> m_oldStructure;
    WriteBarrier<Structure> m_newStructure;
    WriteBarrier<StructureChain> m_chain;
    PutPropertySlot::PutValueFunc m_customSetter;
    RefPtr<JITStubRoutine> m_stubRoutine;
};

class PolymorphicPutByIdList {
    WTF_MAKE_FAST_ALLOCATED;
public:
    // Either creates a new polymorphic put list, or returns the one that is already
    // in place.
    static PolymorphicPutByIdList* from(PutKind, StructureStubInfo&);
    
    ~PolymorphicPutByIdList();
    
    MacroAssemblerCodePtr currentSlowPathTarget() const
    {
        return m_list.last().stubRoutine()->code().code();
    }
    
    void addAccess(const PutByIdAccess&);
    
    bool isEmpty() const { return m_list.isEmpty(); }
    unsigned size() const { return m_list.size(); }
    bool isFull() const;
    bool isAlmostFull() const; // True if adding an element would make isFull() true.
    const PutByIdAccess& at(unsigned i) const { return m_list[i]; }
    const PutByIdAccess& operator[](unsigned i) const { return m_list[i]; }
    
    PutKind kind() const { return m_kind; }
    
    bool visitWeak(RepatchBuffer&) const;
    
private:
    friend class CodeBlock;
    
    // Initialize from a stub info; this will place one element in the list and it will
    // be created by converting the stub info's put by id access information into our
    // PutByIdAccess.
    PolymorphicPutByIdList(PutKind, StructureStubInfo&);

    Vector<PutByIdAccess, 2> m_list;
    PutKind m_kind;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // PolymorphicPutByIdList_h

