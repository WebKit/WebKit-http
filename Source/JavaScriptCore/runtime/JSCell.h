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
#include "SlotVisitorInlineMethods.h"
#include "WriteBarrier.h"
#include <wtf/Noncopyable.h>
#include <wtf/TypeTraits.h>

namespace JSC {

    class JSGlobalObject;
    class LLIntOffsetsExtractor;
    class PropertyDescriptor;
    class PropertyNameArray;
    class Structure;

    enum EnumerationMode {
        ExcludeDontEnumProperties,
        IncludeDontEnumProperties
    };

    enum TypedArrayType {
        TypedArrayNone,
        TypedArrayInt8,
        TypedArrayInt16,
        TypedArrayInt32,
        TypedArrayUint8,
        TypedArrayUint8Clamped,
        TypedArrayUint16,
        TypedArrayUint32,
        TypedArrayFloat32,
        TypedArrayFloat64
    };

    class JSCell {
        friend class JSValue;
        friend class MarkedBlock;
        template<typename T> friend void* allocateCell(Heap&);
        template<typename T> friend void* allocateCell(Heap&, size_t);

    public:
        static const unsigned StructureFlags = 0;

        enum CreatingEarlyCellTag { CreatingEarlyCell };
        JSCell(CreatingEarlyCellTag);

    protected:
        JSCell(JSGlobalData&, Structure*);
        JS_EXPORT_PRIVATE static void destroy(JSCell*);

    public:
        // Querying the type.
        bool isString() const;
        bool isObject() const;
        bool isGetterSetter() const;
        bool inherits(const ClassInfo*) const;
        bool isAPIValueWrapper() const;

        Structure* structure() const;
        void setStructure(JSGlobalData&, Structure*);
        void clearStructure() { m_structure.clear(); }

        const char* className();

        // Extracting the value.
        JS_EXPORT_PRIVATE bool getString(ExecState*, String&) const;
        JS_EXPORT_PRIVATE String getString(ExecState*) const; // null string if not a string
        JS_EXPORT_PRIVATE JSObject* getObject(); // NULL if not an object
        const JSObject* getObject() const; // NULL if not an object
        
        JS_EXPORT_PRIVATE static CallType getCallData(JSCell*, CallData&);
        JS_EXPORT_PRIVATE static ConstructType getConstructData(JSCell*, ConstructData&);

        // Basic conversions.
        JS_EXPORT_PRIVATE JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        bool getPrimitiveNumber(ExecState*, double& number, JSValue&) const;
        bool toBoolean(ExecState*) const;
        JS_EXPORT_PRIVATE double toNumber(ExecState*) const;
        JS_EXPORT_PRIVATE JSObject* toObject(ExecState*, JSGlobalObject*) const;

        static void visitChildren(JSCell*, SlotVisitor&);

        // Object operations, with the toObject operation included.
        const ClassInfo* classInfo() const;
        const MethodTable* methodTable() const;
        static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
        static void putByIndex(JSCell*, ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
        
        static bool deleteProperty(JSCell*, ExecState*, PropertyName);
        static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned propertyName);

        static JSObject* toThisObject(JSCell*, ExecState*);

        void zap() { *reinterpret_cast<uintptr_t**>(this) = 0; }
        bool isZapped() const { return !*reinterpret_cast<uintptr_t* const*>(this); }

        // FIXME: Rename getOwnPropertySlot to virtualGetOwnPropertySlot, and
        // fastGetOwnPropertySlot to getOwnPropertySlot. Callers should always
        // call this function, not its slower virtual counterpart. (For integer
        // property names, we want a similar interface with appropriate optimizations.)
        bool fastGetOwnPropertySlot(ExecState*, PropertyName, PropertySlot&);
        JSValue fastGetOwnProperty(ExecState*, const String&);

        static ptrdiff_t structureOffset()
        {
            return OBJECT_OFFSETOF(JSCell, m_structure);
        }

        void* structureAddress()
        {
            return &m_structure;
        }
        
#if ENABLE(GC_VALIDATION)
        Structure* unvalidatedStructure() { return m_structure.unvalidatedGet(); }
#endif
        
        static const TypedArrayType TypedArrayStorageType = TypedArrayNone;
    protected:

        void finishCreation(JSGlobalData&);
        void finishCreation(JSGlobalData&, Structure*, CreatingEarlyCellTag);

        // Base implementation; for non-object classes implements getPropertySlot.
        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static bool getOwnPropertySlotByIndex(JSCell*, ExecState*, unsigned propertyName, PropertySlot&);

        // Dummy implementations of override-able static functions for classes to put in their MethodTable
        static JSValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);
        static NO_RETURN_DUE_TO_ASSERT void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        static NO_RETURN_DUE_TO_ASSERT void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        static NO_RETURN_DUE_TO_ASSERT void getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        static String className(const JSObject*);
        static bool hasInstance(JSObject*, ExecState*, JSValue, JSValue prototypeProperty);
        static NO_RETURN_DUE_TO_ASSERT void putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned attributes);
        static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);
        static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

    private:
        friend class LLIntOffsetsExtractor;
        
        WriteBarrier<Structure> m_structure;
    };

    inline JSCell::JSCell(CreatingEarlyCellTag)
    {
    }

    inline void JSCell::finishCreation(JSGlobalData& globalData)
    {
#if ENABLE(GC_VALIDATION)
        ASSERT(globalData.isInitializingObject());
        globalData.setInitializingObjectClass(0);
#else
        UNUSED_PARAM(globalData);
#endif
        ASSERT(m_structure);
    }

    inline Structure* JSCell::structure() const
    {
        return m_structure.get();
    }

    inline void JSCell::visitChildren(JSCell* cell, SlotVisitor& visitor)
    {
        MARK_LOG_PARENT(visitor, cell);

        visitor.append(&cell->m_structure);
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

    inline bool JSValue::getString(ExecState* exec, String& s) const
    {
        return isCell() && asCell()->getString(exec, s);
    }

    inline String JSValue::getString(ExecState* exec) const
    {
        return isCell() ? asCell()->getString(exec) : String();
    }

    template <typename Base> String HandleConverter<Base, Unknown>::getString(ExecState* exec) const
    {
        return jsValue().getString(exec);
    }

    inline JSObject* JSValue::getObject() const
    {
        return isCell() ? asCell()->getObject() : 0;
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

    ALWAYS_INLINE double JSValue::toNumber(ExecState* exec) const
    {
        if (isInt32())
            return asInt32();
        if (isDouble())
            return asDouble();
        return toNumberSlowCase(exec);
    }

    inline JSObject* JSValue::toObject(ExecState* exec) const
    {
        return isCell() ? asCell()->toObject(exec, exec->lexicalGlobalObject()) : toObjectSlowCase(exec, exec->lexicalGlobalObject());
    }

    inline JSObject* JSValue::toObject(ExecState* exec, JSGlobalObject* globalObject) const
    {
        return isCell() ? asCell()->toObject(exec, globalObject) : toObjectSlowCase(exec, globalObject);
    }

    template<class T>
    struct NeedsDestructor {
        static const bool value = !WTF::HasTrivialDestructor<T>::value;
    };

    template<typename T>
    void* allocateCell(Heap& heap)
    {
#if ENABLE(GC_VALIDATION)
        ASSERT(!heap.globalData()->isInitializingObject());
        heap.globalData()->setInitializingObjectClass(&T::s_info);
#endif
        JSCell* result = 0;
        if (NeedsDestructor<T>::value)
            result = static_cast<JSCell*>(heap.allocateWithDestructor(sizeof(T)));
        else {
            ASSERT(T::s_info.methodTable.destroy == JSCell::destroy);
            result = static_cast<JSCell*>(heap.allocateWithoutDestructor(sizeof(T)));
        }
        result->clearStructure();
        return result;
    }
    
    template<typename T>
    void* allocateCell(Heap& heap, size_t size)
    {
        ASSERT(size >= sizeof(T));
#if ENABLE(GC_VALIDATION)
        ASSERT(!heap.globalData()->isInitializingObject());
        heap.globalData()->setInitializingObjectClass(&T::s_info);
#endif
        JSCell* result = 0;
        if (NeedsDestructor<T>::value)
            result = static_cast<JSCell*>(heap.allocateWithDestructor(size));
        else {
            ASSERT(T::s_info.methodTable.destroy == JSCell::destroy);
            result = static_cast<JSCell*>(heap.allocateWithoutDestructor(size));
        }
        result->clearStructure();
        return result;
    }
    
    inline bool isZapped(const JSCell* cell)
    {
        return cell->isZapped();
    }

    template<typename To, typename From>
    inline To jsCast(From* from)
    {
        ASSERT(!from || from->JSCell::inherits(&WTF::RemovePointer<To>::Type::s_info));
        return static_cast<To>(from);
    }

    template<typename To>
    inline To jsCast(JSValue from)
    {
        ASSERT(from.isCell() && from.asCell()->JSCell::inherits(&WTF::RemovePointer<To>::Type::s_info));
        return static_cast<To>(from.asCell());
    }

    template<typename To, typename From>
    inline To jsDynamicCast(From* from)
    {
        return from->inherits(&WTF::RemovePointer<To>::Type::s_info) ? static_cast<To>(from) : 0;
    }

    template<typename To>
    inline To jsDynamicCast(JSValue from)
    {
        return from.isCell() && from.asCell()->inherits(&WTF::RemovePointer<To>::Type::s_info) ? static_cast<To>(from.asCell()) : 0;
    }

} // namespace JSC

#endif // JSCell_h
