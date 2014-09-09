/*
 * Copyright (C) 2007, 2008, 2012-2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SymbolTable_h
#define SymbolTable_h

#include "ConcurrentJITLock.h"
#include "JSObject.h"
#include "TypeSet.h"
#include "VariableWatchpointSet.h"
#include <memory>
#include <wtf/HashTraits.h>
#include <wtf/text/StringImpl.h>

namespace JSC {

struct SlowArgument {
public:
    enum Status {
        Normal = 0,
        Captured = 1,
        Deleted = 2
    };

    SlowArgument()
        : status(Normal)
        , index(0)
    {
    }

    Status status;
    int index; // If status is 'Deleted', index is bogus.
};

static ALWAYS_INLINE int missingSymbolMarker() { return std::numeric_limits<int>::max(); }

// The bit twiddling in this class assumes that every register index is a
// reasonably small positive or negative number, and therefore has its high
// four bits all set or all unset.

// In addition to implementing semantics-mandated variable attributes and
// implementation-mandated variable indexing, this class also implements
// watchpoints to be used for JIT optimizations. Because watchpoints are
// meant to be relatively rare, this class optimizes heavily for the case
// that they are not being used. To that end, this class uses the thin-fat
// idiom: either it is thin, in which case it contains an in-place encoded
// word that consists of attributes, the index, and a bit saying that it is
// thin; or it is fat, in which case it contains a pointer to a malloc'd
// data structure and a bit saying that it is fat. The malloc'd data
// structure will be malloced a second time upon copy, to preserve the
// property that in-place edits to SymbolTableEntry do not manifest in any
// copies. However, the malloc'd FatEntry data structure contains a ref-
// counted pointer to a shared WatchpointSet. Thus, in-place edits of the
// WatchpointSet will manifest in all copies. Here's a picture:
//
// SymbolTableEntry --> FatEntry --> VariableWatchpointSet
//
// If you make a copy of a SymbolTableEntry, you will have:
//
// original: SymbolTableEntry --> FatEntry --> VariableWatchpointSet
// copy:     SymbolTableEntry --> FatEntry -----^

struct SymbolTableEntry {
    // Use the SymbolTableEntry::Fast class, either via implicit cast or by calling
    // getFast(), when you (1) only care about isNull(), getIndex(), and isReadOnly(),
    // and (2) you are in a hot path where you need to minimize the number of times
    // that you branch on isFat() when getting the bits().
    class Fast {
    public:
        Fast()
            : m_bits(SlimFlag)
        {
        }
        
        ALWAYS_INLINE Fast(const SymbolTableEntry& entry)
            : m_bits(entry.bits())
        {
        }
    
        bool isNull() const
        {
            return !(m_bits & ~SlimFlag);
        }

        int getIndex() const
        {
            return static_cast<int>(m_bits >> FlagBits);
        }
    
        bool isReadOnly() const
        {
            return m_bits & ReadOnlyFlag;
        }
        
        unsigned getAttributes() const
        {
            unsigned attributes = 0;
            if (m_bits & ReadOnlyFlag)
                attributes |= ReadOnly;
            if (m_bits & DontEnumFlag)
                attributes |= DontEnum;
            return attributes;
        }

        bool isFat() const
        {
            return !(m_bits & SlimFlag);
        }
        
    private:
        friend struct SymbolTableEntry;
        intptr_t m_bits;
    };

    SymbolTableEntry()
        : m_bits(SlimFlag)
    {
    }

    SymbolTableEntry(int index)
        : m_bits(SlimFlag)
    {
        ASSERT(isValidIndex(index));
        pack(index, false, false);
    }

    SymbolTableEntry(int index, unsigned attributes)
        : m_bits(SlimFlag)
    {
        ASSERT(isValidIndex(index));
        pack(index, attributes & ReadOnly, attributes & DontEnum);
    }
    
    ~SymbolTableEntry()
    {
        freeFatEntry();
    }
    
    SymbolTableEntry(const SymbolTableEntry& other)
        : m_bits(SlimFlag)
    {
        *this = other;
    }
    
    SymbolTableEntry& operator=(const SymbolTableEntry& other)
    {
        if (UNLIKELY(other.isFat()))
            return copySlow(other);
        freeFatEntry();
        m_bits = other.m_bits;
        return *this;
    }
    
    bool isNull() const
    {
        return !(bits() & ~SlimFlag);
    }

    int getIndex() const
    {
        return static_cast<int>(bits() >> FlagBits);
    }
    
    ALWAYS_INLINE Fast getFast() const
    {
        return Fast(*this);
    }
    
    ALWAYS_INLINE Fast getFast(bool& wasFat) const
    {
        Fast result;
        wasFat = isFat();
        if (wasFat)
            result.m_bits = fatEntry()->m_bits | SlimFlag;
        else
            result.m_bits = m_bits;
        return result;
    }
    
    unsigned getAttributes() const
    {
        return getFast().getAttributes();
    }

    void setAttributes(unsigned attributes)
    {
        pack(getIndex(), attributes & ReadOnly, attributes & DontEnum);
    }

    bool isReadOnly() const
    {
        return bits() & ReadOnlyFlag;
    }
    
    JSValue inferredValue();
    
    void prepareToWatch(SymbolTable*);
    
    void addWatchpoint(Watchpoint*);
    
    VariableWatchpointSet* watchpointSet()
    {
        if (!isFat())
            return 0;
        return fatEntry()->m_watchpoints.get();
    }
    
    ALWAYS_INLINE void notifyWrite(VM& vm, JSValue value, const FireDetail& detail)
    {
        if (LIKELY(!isFat()))
            return;
        notifyWriteSlow(vm, value, detail);
    }
    
private:
    static const intptr_t SlimFlag = 0x1;
    static const intptr_t ReadOnlyFlag = 0x2;
    static const intptr_t DontEnumFlag = 0x4;
    static const intptr_t NotNullFlag = 0x8;
    static const intptr_t FlagBits = 4;
    
    class FatEntry {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        FatEntry(intptr_t bits)
            : m_bits(bits & ~SlimFlag)
        {
        }
        
        intptr_t m_bits; // always has FatFlag set and exactly matches what the bits would have been if this wasn't fat.
        
        RefPtr<VariableWatchpointSet> m_watchpoints;
    };
    
    SymbolTableEntry& copySlow(const SymbolTableEntry&);
    JS_EXPORT_PRIVATE void notifyWriteSlow(VM&, JSValue, const FireDetail&);
    
    bool isFat() const
    {
        return !(m_bits & SlimFlag);
    }
    
    const FatEntry* fatEntry() const
    {
        ASSERT(isFat());
        return bitwise_cast<const FatEntry*>(m_bits);
    }
    
    FatEntry* fatEntry()
    {
        ASSERT(isFat());
        return bitwise_cast<FatEntry*>(m_bits);
    }
    
    FatEntry* inflate()
    {
        if (LIKELY(isFat()))
            return fatEntry();
        return inflateSlow();
    }
    
    FatEntry* inflateSlow();
    
    ALWAYS_INLINE intptr_t bits() const
    {
        if (isFat())
            return fatEntry()->m_bits;
        return m_bits;
    }
    
    ALWAYS_INLINE intptr_t& bits()
    {
        if (isFat())
            return fatEntry()->m_bits;
        return m_bits;
    }
    
    void freeFatEntry()
    {
        if (LIKELY(!isFat()))
            return;
        freeFatEntrySlow();
    }

    JS_EXPORT_PRIVATE void freeFatEntrySlow();

    void pack(int index, bool readOnly, bool dontEnum)
    {
        ASSERT(!isFat());
        intptr_t& bitsRef = bits();
        bitsRef = (static_cast<intptr_t>(index) << FlagBits) | NotNullFlag | SlimFlag;
        if (readOnly)
            bitsRef |= ReadOnlyFlag;
        if (dontEnum)
            bitsRef |= DontEnumFlag;
    }
    
    bool isValidIndex(int index)
    {
        return ((static_cast<intptr_t>(index) << FlagBits) >> FlagBits) == static_cast<intptr_t>(index);
    }

    intptr_t m_bits;
};

struct SymbolTableIndexHashTraits : HashTraits<SymbolTableEntry> {
    static const bool needsDestruction = true;
};

class SymbolTable : public JSCell {
public:
    typedef JSCell Base;

    typedef HashMap<RefPtr<StringImpl>, SymbolTableEntry, IdentifierRepHash, HashTraits<RefPtr<StringImpl>>, SymbolTableIndexHashTraits> Map;
    typedef HashMap<RefPtr<StringImpl>, GlobalVariableID> UniqueIDMap;
    typedef HashMap<RefPtr<StringImpl>, RefPtr<TypeSet>> UniqueTypeSetMap;
    typedef HashMap<int, RefPtr<StringImpl>, WTF::IntHash<int>, WTF::UnsignedWithZeroKeyHashTraits<int>> RegisterToVariableMap;

    static SymbolTable* create(VM& vm)
    {
        SymbolTable* symbolTable = new (NotNull, allocateCell<SymbolTable>(vm.heap)) SymbolTable(vm);
        symbolTable->finishCreation(vm);
        return symbolTable;
    }
    static const bool needsDestruction = true;
    static const bool hasImmortalStructure = true;
    static void destroy(JSCell*);

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(CellType, StructureFlags), info());
    }

    // You must hold the lock until after you're done with the iterator.
    Map::iterator find(const ConcurrentJITLocker&, StringImpl* key)
    {
        return m_map.find(key);
    }
    
    Map::iterator find(const GCSafeConcurrentJITLocker&, StringImpl* key)
    {
        return m_map.find(key);
    }
    
    SymbolTableEntry get(const ConcurrentJITLocker&, StringImpl* key)
    {
        return m_map.get(key);
    }
    
    SymbolTableEntry get(StringImpl* key)
    {
        ConcurrentJITLocker locker(m_lock);
        return get(locker, key);
    }
    
    SymbolTableEntry inlineGet(const ConcurrentJITLocker&, StringImpl* key)
    {
        return m_map.inlineGet(key);
    }
    
    SymbolTableEntry inlineGet(StringImpl* key)
    {
        ConcurrentJITLocker locker(m_lock);
        return inlineGet(locker, key);
    }
    
    Map::iterator begin(const ConcurrentJITLocker&)
    {
        return m_map.begin();
    }
    
    Map::iterator end(const ConcurrentJITLocker&)
    {
        return m_map.end();
    }
    
    Map::iterator end(const GCSafeConcurrentJITLocker&)
    {
        return m_map.end();
    }
    
    size_t size(const ConcurrentJITLocker&) const
    {
        return m_map.size();
    }
    
    size_t size() const
    {
        ConcurrentJITLocker locker(m_lock);
        return size(locker);
    }
    
    Map::AddResult add(const ConcurrentJITLocker&, StringImpl* key, const SymbolTableEntry& entry)
    {
        return m_map.add(key, entry);
    }
    
    void add(StringImpl* key, const SymbolTableEntry& entry)
    {
        ConcurrentJITLocker locker(m_lock);
        add(locker, key, entry);
    }
    
    Map::AddResult set(const ConcurrentJITLocker&, StringImpl* key, const SymbolTableEntry& entry)
    {
        return m_map.set(key, entry);
    }
    
    void set(StringImpl* key, const SymbolTableEntry& entry)
    {
        ConcurrentJITLocker locker(m_lock);
        set(locker, key, entry);
    }
    
    bool contains(const ConcurrentJITLocker&, StringImpl* key)
    {
        return m_map.contains(key);
    }
    
    bool contains(StringImpl* key)
    {
        ConcurrentJITLocker locker(m_lock);
        return contains(locker, key);
    }
    
    GlobalVariableID uniqueIDForVariable(const ConcurrentJITLocker&, StringImpl* key, VM& vm);
    GlobalVariableID uniqueIDForRegister(const ConcurrentJITLocker& locker, int registerIndex, VM& vm);
    RefPtr<TypeSet> globalTypeSetForRegister(const ConcurrentJITLocker& locker, int registerIndex, VM& vm);
    RefPtr<TypeSet> globalTypeSetForVariable(const ConcurrentJITLocker& locker, StringImpl* key, VM& vm);

    bool usesNonStrictEval() { return m_usesNonStrictEval; }
    void setUsesNonStrictEval(bool usesNonStrictEval) { m_usesNonStrictEval = usesNonStrictEval; }

    int captureStart() const { return m_captureStart; }
    void setCaptureStart(int captureStart) { m_captureStart = captureStart; }

    int captureEnd() const { return m_captureEnd; }
    void setCaptureEnd(int captureEnd) { m_captureEnd = captureEnd; }

    int captureCount() const { return -(m_captureEnd - m_captureStart); }
    
    bool isCaptured(int operand)
    {
        return operand <= captureStart() && operand > captureEnd();
    }

    int parameterCount() { return m_parameterCountIncludingThis - 1; }
    int parameterCountIncludingThis() { return m_parameterCountIncludingThis; }
    void setParameterCountIncludingThis(int parameterCountIncludingThis) { m_parameterCountIncludingThis = parameterCountIncludingThis; }

    // 0 if we don't capture any arguments; parameterCount() in length if we do.
    const SlowArgument* slowArguments() { return m_slowArguments.get(); }
    void setSlowArguments(std::unique_ptr<SlowArgument[]> slowArguments) { m_slowArguments = WTF::move(slowArguments); }
    
    SymbolTable* cloneCapturedNames(VM&);

    void prepareForTypeProfiling(const ConcurrentJITLocker&);

    static void visitChildren(JSCell*, SlotVisitor&);

    DECLARE_EXPORT_INFO;

protected:
    static const unsigned StructureFlags = StructureIsImmortal | Base::StructureFlags;

private:
    class WatchpointCleanup : public UnconditionalFinalizer {
    public:
        WatchpointCleanup(SymbolTable*);
        virtual ~WatchpointCleanup();
        
    protected:
        virtual void finalizeUnconditionally() override;

    private:
        SymbolTable* m_symbolTable;
    };
    
    JS_EXPORT_PRIVATE SymbolTable(VM&);
    ~SymbolTable();

    Map m_map;
    struct TypeProfilingRareData {
        UniqueIDMap m_uniqueIDMap;
        RegisterToVariableMap m_registerToVariableMap;
        UniqueTypeSetMap m_uniqueTypeSetMap;
    };
    std::unique_ptr<TypeProfilingRareData> m_typeProfilingRareData;

    int m_parameterCountIncludingThis;
    bool m_usesNonStrictEval;

    int m_captureStart;
    int m_captureEnd;

    std::unique_ptr<SlowArgument[]> m_slowArguments;
    
    std::unique_ptr<WatchpointCleanup> m_watchpointCleanup;

public:
    InlineWatchpointSet m_functionEnteredOnce;
    
    mutable ConcurrentJITLock m_lock;
};

} // namespace JSC

#endif // SymbolTable_h
