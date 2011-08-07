/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef JSCell_h
#define JSCell_h

#include "CallData.h"
#include "CallFrame.h"
#include "ConstructData.h"
#include "Heap.h"
#include "JSLock.h"
#include "JSValueInlineMethods.h"
#include "SlotVisitor.h"
#include "WriteBarrier.h"
#include <wtf/Noncopyable.h>

namespace JSC {

    class JSGlobalObject;
    class Structure;

#if COMPILER(MSVC)
    // If WTF_MAKE_NONCOPYABLE is applied to JSCell we end up with a bunch of
    // undefined references to the JSCell copy constructor and assignment operator
    // when linking JavaScriptCore.
    class MSVCBugWorkaround {
        WTF_MAKE_NONCOPYABLE(MSVCBugWorkaround);

    protected:
        MSVCBugWorkaround() { }
        ~MSVCBugWorkaround() { }
    };

    class JSCell : MSVCBugWorkaround {
#else
    class JSCell {
        WTF_MAKE_NONCOPYABLE(JSCell);
#endif

        friend class ExecutableBase;
        friend class GetterSetter;
        friend class Heap;
        friend class JSObject;
        friend class JSPropertyNameIterator;
        friend class JSString;
        friend class JSValue;
        friend class JSAPIValueWrapper;
        friend class JSGlobalData;
        friend class NewSpace;
        friend class MarkedBlock;
        friend class ScopeChainNode;
        friend class Structure;
        friend class StructureChain;
        friend class RegExp;

        enum CreatingEarlyCellTag { CreatingEarlyCell };

    protected:
        enum VPtrStealingHackType { VPtrStealingHack };

    private:
        explicit JSCell(VPtrStealingHackType) { }
        JSCell(JSGlobalData&, Structure*);
        JSCell(JSGlobalData&, Structure*, CreatingEarlyCellTag);
        virtual ~JSCell();

    public:
        // Querying the type.
        bool isString() const;
        bool isObject() const;
        virtual bool isGetterSetter() const;
        bool inherits(const ClassInfo*) const;
        virtual bool isAPIValueWrapper() const { return false; }
        virtual bool isPropertyNameIterator() const { return false; }

        Structure* structure() const;

        // Extracting the value.
        bool getString(ExecState* exec, UString&) const;
        UString getString(ExecState* exec) const; // null string if not a string
        JSObject* getObject(); // NULL if not an object
        const JSObject* getObject() const; // NULL if not an object
        
        virtual CallType getCallData(CallData&);
        virtual ConstructType getConstructData(ConstructData&);

        // Extracting integer values.
        // FIXME: remove these methods, can check isNumberCell in JSValue && then call asNumberCell::*.
        virtual bool getUInt32(uint32_t&) const;

        // Basic conversions.
        virtual JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(ExecState*, double& number, JSValue&);
        virtual bool toBoolean(ExecState*) const;
        virtual double toNumber(ExecState*) const;
        virtual UString toString(ExecState*) const;
        virtual JSObject* toObject(ExecState*, JSGlobalObject*) const;

        // Garbage collection.
        void* operator new(size_t, void* placementNewDestination) { return placementNewDestination; }

        virtual void visitChildren(SlotVisitor&);

        // Object operations, with the toObject operation included.
        const ClassInfo* classInfo() const;
        virtual void put(ExecState*, const Identifier& propertyName, JSValue, PutPropertySlot&);
        virtual void put(ExecState*, unsigned propertyName, JSValue);
        virtual bool deleteProperty(ExecState*, const Identifier& propertyName);
        virtual bool deleteProperty(ExecState*, unsigned propertyName);

        virtual JSObject* toThisObject(ExecState*) const;
        virtual JSValue getJSNumber();
        void* vptr() { return *reinterpret_cast<void**>(this); }
        void setVPtr(void* vptr) { *reinterpret_cast<void**>(this) = vptr; }

        // FIXME: Rename getOwnPropertySlot to virtualGetOwnPropertySlot, and
        // fastGetOwnPropertySlot to getOwnPropertySlot. Callers should always
        // call this function, not its slower virtual counterpart. (For integer
        // property names, we want a similar interface with appropriate optimizations.)
        bool fastGetOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        JSValue fastGetOwnProperty(ExecState*, const UString&);

        static ptrdiff_t structureOffset()
        {
            return OBJECT_OFFSETOF(JSCell, m_structure);
        }

#if ENABLE(GC_VALIDATION)
        Structure* unvalidatedStructure() { return m_structure.unvalidatedGet(); }
#endif
        
    protected:
        static const unsigned AnonymousSlotCount = 0;

    private:
        // Base implementation; for non-object classes implements getPropertySlot.
        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
        
        // Note that the first two declarations of operator new have no corresponding implementation and 
        // will cause link errors if you use them.
        void* operator new(size_t, ExecState*);
        void* operator new(size_t, JSGlobalData*);
        
        WriteBarrier<Structure> m_structure;
    };

    inline JSCell::JSCell(JSGlobalData& globalData, Structure* structure)
        : m_structure(globalData, this, structure)
    {
        ASSERT(m_structure);
    }

    inline JSCell::JSCell(JSGlobalData& globalData, Structure* structure, CreatingEarlyCellTag)
    {
#if ENABLE(GC_VALIDATION)
        if (structure)
#endif
            m_structure.setEarlyValue(globalData, this, structure);
        // Very first set of allocations won't have a real structure.
        ASSERT(m_structure || !globalData.structureStructure);
    }

    inline JSCell::~JSCell()
    {
#if ENABLE(GC_VALIDATION)
        m_structure.clear();
#endif
    }

    inline Structure* JSCell::structure() const
    {
        return m_structure.get();
    }

    inline void JSCell::visitChildren(SlotVisitor& visitor)
    {
        visitor.append(&m_structure);
    }

    // --- JSValue inlines ----------------------------

    inline bool JSValue::isString() const
    {
        return isCell() && asCell()->isString();
    }

    inline bool JSValue::isPrimitive() const
    {
        return !isCell() || asCell()->isString();
    }

    inline bool JSValue::isGetterSetter() const
    {
        return isCell() && asCell()->isGetterSetter();
    }

    inline bool JSValue::isObject() const
    {
        return isCell() && asCell()->isObject();
    }

    inline bool JSValue::getString(ExecState* exec, UString& s) const
    {
        return isCell() && asCell()->getString(exec, s);
    }

    inline UString JSValue::getString(ExecState* exec) const
    {
        return isCell() ? asCell()->getString(exec) : UString();
    }

    template <typename Base> UString HandleConverter<Base, Unknown>::getString(ExecState* exec) const
    {
        return jsValue().getString(exec);
    }

    inline JSObject* JSValue::getObject() const
    {
        return isCell() ? asCell()->getObject() : 0;
    }

    inline CallType getCallData(JSValue value, CallData& callData)
    {
        CallType result = value.isCell() ? value.asCell()->getCallData(callData) : CallTypeNone;
        ASSERT(result == CallTypeNone || value.isValidCallee());
        return result;
    }

    inline ConstructType getConstructData(JSValue value, ConstructData& constructData)
    {
        ConstructType result = value.isCell() ? value.asCell()->getConstructData(constructData) : ConstructTypeNone;
        ASSERT(result == ConstructTypeNone || value.isValidCallee());
        return result;
    }

    ALWAYS_INLINE bool JSValue::getUInt32(uint32_t& v) const
    {
        if (isInt32()) {
            int32_t i = asInt32();
            v = static_cast<uint32_t>(i);
            return i >= 0;
        }
        if (isDouble()) {
            double d = asDouble();
            v = static_cast<uint32_t>(d);
            return v == d;
        }
        return false;
    }

    inline JSValue JSValue::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
    {
        return isCell() ? asCell()->toPrimitive(exec, preferredType) : asValue();
    }

    inline bool JSValue::getPrimitiveNumber(ExecState* exec, double& number, JSValue& value)
    {
        if (isInt32()) {
            number = asInt32();
            value = *this;
            return true;
        }
        if (isDouble()) {
            number = asDouble();
            value = *this;
            return true;
        }
        if (isCell())
            return asCell()->getPrimitiveNumber(exec, number, value);
        if (isTrue()) {
            number = 1.0;
            value = *this;
            return true;
        }
        if (isFalse() || isNull()) {
            number = 0.0;
            value = *this;
            return true;
        }
        ASSERT(isUndefined());
        number = std::numeric_limits<double>::quiet_NaN();
        value = *this;
        return true;
    }

    inline bool JSValue::toBoolean(ExecState* exec) const
    {
        if (isInt32())
            return asInt32() != 0;
        if (isDouble())
            return asDouble() > 0.0 || asDouble() < 0.0; // false for NaN
        if (isCell())
            return asCell()->toBoolean(exec);
        return isTrue(); // false, null, and undefined all convert to false.
    }

    ALWAYS_INLINE double JSValue::toNumber(ExecState* exec) const
    {
        if (isInt32())
            return asInt32();
        if (isDouble())
            return asDouble();
        return toNumberSlowCase(exec);
    }

    inline JSValue JSValue::getJSNumber()
    {
        if (isInt32() || isDouble())
            return *this;
        if (isCell())
            return asCell()->getJSNumber();
        return JSValue();
    }

    inline JSObject* JSValue::toObject(ExecState* exec) const
    {
        return isCell() ? asCell()->toObject(exec, exec->lexicalGlobalObject()) : toObjectSlowCase(exec, exec->lexicalGlobalObject());
    }

    inline JSObject* JSValue::toObject(ExecState* exec, JSGlobalObject* globalObject) const
    {
        return isCell() ? asCell()->toObject(exec, globalObject) : toObjectSlowCase(exec, globalObject);
    }

    inline JSObject* JSValue::toThisObject(ExecState* exec) const
    {
        return isCell() ? asCell()->toThisObject(exec) : toThisObjectSlowCase(exec);
    }

    template <typename T> void* allocateCell(Heap& heap)
    {
        return heap.allocate(sizeof(T));
    }
        
} // namespace JSC

#endif // JSCell_h
