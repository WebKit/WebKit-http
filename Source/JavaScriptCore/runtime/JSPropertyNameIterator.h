/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#ifndef JSPropertyNameIterator_h
#define JSPropertyNameIterator_h

#include "JSObject.h"
#include "JSString.h"
#include "Operations.h"
#include "PropertyNameArray.h"

namespace JSC {

    class Identifier;
    class JSObject;
    class LLIntOffsetsExtractor;

    class JSPropertyNameIterator : public JSCell {
        friend class JIT;

    public:
        typedef JSCell Base;

        static JSPropertyNameIterator* create(ExecState*, JSObject*);

        static void destroy(JSCell*);
       
        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(globalData, globalObject, prototype, TypeInfo(CompoundType, OverridesVisitChildren), &s_info);
        }

        static void visitChildren(JSCell*, SlotVisitor&);

        bool getOffset(size_t i, PropertyOffset& offset)
        {
            if (i >= m_numCacheableSlots)
                return false;
            offset = i + m_offsetBase;
            return true;
        }

        JSValue get(ExecState*, JSObject*, size_t i);
        size_t size() { return m_jsStringsSize; }

        void setCachedStructure(JSGlobalData& globalData, Structure* structure)
        {
            ASSERT(!m_cachedStructure);
            ASSERT(structure);
            m_cachedStructure.set(globalData, this, structure);
        }
        Structure* cachedStructure() { return m_cachedStructure.get(); }

        void setCachedPrototypeChain(JSGlobalData& globalData, StructureChain* cachedPrototypeChain) { m_cachedPrototypeChain.set(globalData, this, cachedPrototypeChain); }
        StructureChain* cachedPrototypeChain() { return m_cachedPrototypeChain.get(); }
        
        static const ClassInfo s_info;

    protected:
        void finishCreation(ExecState* exec, PropertyNameArrayData* propertyNameArrayData, JSObject* object)
        {
            Base::finishCreation(exec->globalData());
            PropertyNameArrayData::PropertyNameVector& propertyNameVector = propertyNameArrayData->propertyNameVector();
            for (size_t i = 0; i < m_jsStringsSize; ++i)
                m_jsStrings[i].set(exec->globalData(), this, jsOwnedString(exec, propertyNameVector[i].string()));
            m_offsetBase = object->structure()->firstValidOffset();
        }

    private:
        friend class LLIntOffsetsExtractor;
        
        JSPropertyNameIterator(ExecState*, PropertyNameArrayData* propertyNameArrayData, size_t numCacheableSlot);

        WriteBarrier<Structure> m_cachedStructure;
        WriteBarrier<StructureChain> m_cachedPrototypeChain;
        uint32_t m_numCacheableSlots;
        uint32_t m_jsStringsSize;
        PropertyOffset m_offsetBase;
        OwnArrayPtr<WriteBarrier<Unknown> > m_jsStrings;
    };

    inline void Structure::setEnumerationCache(JSGlobalData& globalData, JSPropertyNameIterator* enumerationCache)
    {
        ASSERT(!isDictionary());
        m_enumerationCache.set(globalData, this, enumerationCache);
    }

    inline JSPropertyNameIterator* Structure::enumerationCache()
    {
        return m_enumerationCache.get();
    }

    ALWAYS_INLINE JSPropertyNameIterator* Register::propertyNameIterator() const
    {
        return jsCast<JSPropertyNameIterator*>(jsValue().asCell());
    }

} // namespace JSC

#endif // JSPropertyNameIterator_h
