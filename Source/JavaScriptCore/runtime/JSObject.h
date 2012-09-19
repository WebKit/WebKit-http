/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
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

#ifndef JSObject_h
#define JSObject_h

#include "ArgList.h"
#include "ArrayConventions.h"
#include "ArrayStorage.h"
#include "Butterfly.h"
#include "ClassInfo.h"
#include "CommonIdentifiers.h"
#include "CallFrame.h"
#include "JSCell.h"
#include "PropertySlot.h"
#include "PropertyStorage.h"
#include "PutDirectIndexMode.h"
#include "PutPropertySlot.h"

#include "Structure.h"
#include "JSGlobalData.h"
#include "JSString.h"
#include "SparseArrayValueMap.h"
#include <wtf/StdLibExtras.h>

namespace JSC {

    inline JSCell* getJSFunction(JSValue value)
    {
        if (value.isCell() && (value.asCell()->structure()->typeInfo().type() == JSFunctionType))
            return value.asCell();
        return 0;
    }

    JS_EXPORT_PRIVATE JSCell* getCallableObjectSlow(JSCell*);

    inline JSCell* getCallableObject(JSValue value)
    {
        if (!value.isCell())
            return 0;
        return getCallableObjectSlow(value.asCell());
    }

    class GetterSetter;
    class HashEntry;
    class InternalFunction;
    class LLIntOffsetsExtractor;
    class MarkedBlock;
    class PropertyDescriptor;
    class PropertyNameArray;
    class Structure;
    struct HashTable;

    JS_EXPORT_PRIVATE JSObject* throwTypeError(ExecState*, const String&);
    extern JS_EXPORTDATA const char* StrictModeReadonlyPropertyWriteError;

    // ECMA 262-3 8.6.1
    // Property attributes
    enum Attribute {
        None         = 0,
        ReadOnly     = 1 << 1,  // property can be only read, not written
        DontEnum     = 1 << 2,  // property doesn't appear in (for .. in ..)
        DontDelete   = 1 << 3,  // property can't be deleted
        Function     = 1 << 4,  // property is a function - only used by static hashtables
        Accessor     = 1 << 5,  // property is a getter/setter
    };

    COMPILE_ASSERT(None < FirstInternalAttribute, None_is_below_FirstInternalAttribute);
    COMPILE_ASSERT(ReadOnly < FirstInternalAttribute, ReadOnly_is_below_FirstInternalAttribute);
    COMPILE_ASSERT(DontEnum < FirstInternalAttribute, DontEnum_is_below_FirstInternalAttribute);
    COMPILE_ASSERT(DontDelete < FirstInternalAttribute, DontDelete_is_below_FirstInternalAttribute);
    COMPILE_ASSERT(Function < FirstInternalAttribute, Function_is_below_FirstInternalAttribute);
    COMPILE_ASSERT(Accessor < FirstInternalAttribute, Accessor_is_below_FirstInternalAttribute);

    class JSFinalObject;

    class JSObject : public JSCell {
        friend class BatchedTransitionOptimizer;
        friend class JIT;
        friend class JSCell;
        friend class JSFinalObject;
        friend class MarkedBlock;
        JS_EXPORT_PRIVATE friend bool setUpStaticFunctionSlot(ExecState*, const HashEntry*, JSObject*, PropertyName, PropertySlot&);

        enum PutMode {
            PutModePut,
            PutModeDefineOwnProperty,
        };

    public:
        typedef JSCell Base;
        
        JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

        JS_EXPORT_PRIVATE static String className(const JSObject*);

        JSValue prototype() const;
        void setPrototype(JSGlobalData&, JSValue prototype);
        bool setPrototypeWithCycleCheck(JSGlobalData&, JSValue prototype);
        
        Structure* inheritorID(JSGlobalData&);
        void notifyUsedAsPrototype(JSGlobalData&);
        
        bool mayBeUsedAsPrototype(JSGlobalData& globalData)
        {
            return isValidOffset(structure()->get(globalData, globalData.m_inheritorIDKey));
        }
        
        bool mayInterceptIndexedAccesses()
        {
            return structure()->mayInterceptIndexedAccesses();
        }
        
        JSValue get(ExecState*, PropertyName) const;
        JSValue get(ExecState*, unsigned propertyName) const;

        bool getPropertySlot(ExecState*, PropertyName, PropertySlot&);
        bool getPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
        JS_EXPORT_PRIVATE bool getPropertyDescriptor(ExecState*, PropertyName, PropertyDescriptor&);

        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        JS_EXPORT_PRIVATE static bool getOwnPropertySlotByIndex(JSCell*, ExecState*, unsigned propertyName, PropertySlot&);
        JS_EXPORT_PRIVATE static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

        bool allowsAccessFrom(ExecState*);

        unsigned getArrayLength() const
        {
            switch (structure()->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
                return 0;
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                return m_butterfly->arrayStorage()->length();
            default:
                ASSERT_NOT_REACHED();
                return 0;
            }
        }
        
        unsigned getVectorLength()
        {
            switch (structure()->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
                return 0;
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                return m_butterfly->arrayStorage()->vectorLength();
            default:
                ASSERT_NOT_REACHED();
                return 0;
            }
        }
        
        JS_EXPORT_PRIVATE static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
        JS_EXPORT_PRIVATE static void putByIndex(JSCell*, ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
        
        // This is similar to the putDirect* methods:
        //  - the prototype chain is not consulted
        //  - accessors are not called.
        //  - it will ignore extensibility and read-only properties if PutDirectIndexLikePutDirect is passed as the mode (the default).
        // This method creates a property with attributes writable, enumerable and configurable all set to true.
        bool putDirectIndex(ExecState* exec, unsigned propertyName, JSValue value, unsigned attributes, PutDirectIndexMode mode)
        {
            if (!attributes && canSetIndexQuickly(propertyName)) {
                setIndexQuickly(exec->globalData(), propertyName, value);
                return true;
            }
            return putDirectIndexBeyondVectorLength(exec, propertyName, value, attributes, mode);
        }
        bool putDirectIndex(ExecState* exec, unsigned propertyName, JSValue value)
        {
            return putDirectIndex(exec, propertyName, value, 0, PutDirectIndexLikePutDirect);
        }

        // A non-throwing version of putDirect and putDirectIndex.
        void putDirectMayBeIndex(ExecState*, PropertyName, JSValue);
        
        bool canGetIndexQuickly(unsigned i)
        {
            switch (structure()->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
                return false;
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                return i < m_butterfly->arrayStorage()->vectorLength() && m_butterfly->arrayStorage()->m_vector[i];
            default:
                ASSERT_NOT_REACHED();
                return false;
            }
        }
        
        JSValue getIndexQuickly(unsigned i)
        {
            switch (structure()->indexingType()) {
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                return m_butterfly->arrayStorage()->m_vector[i].get();
            default:
                ASSERT_NOT_REACHED();
                return JSValue();
            }
        }
        
        bool canSetIndexQuickly(unsigned i)
        {
            switch (structure()->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
                return false;
            case NonArrayWithArrayStorage:
            case ArrayWithArrayStorage:
                return i < m_butterfly->arrayStorage()->vectorLength();
            case NonArrayWithSlowPutArrayStorage:
            case ArrayWithSlowPutArrayStorage:
                return i < m_butterfly->arrayStorage()->vectorLength()
                    && !!m_butterfly->arrayStorage()->m_vector[i];
            default:
                ASSERT_NOT_REACHED();
                return false;
            }
        }
        
        void setIndexQuickly(JSGlobalData& globalData, unsigned i, JSValue v)
        {
            switch (structure()->indexingType()) {
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                WriteBarrier<Unknown>& x = m_butterfly->arrayStorage()->m_vector[i];
                if (!x) {
                    ArrayStorage* storage = m_butterfly->arrayStorage();
                    ++storage->m_numValuesInVector;
                    if (i >= storage->length())
                        storage->setLength(i + 1);
                }
                x.set(globalData, this, v);
                break;
            }
            default:
                ASSERT_NOT_REACHED();
            }
        }
        
        void initializeIndex(JSGlobalData& globalData, unsigned i, JSValue v)
        {
            switch (structure()->indexingType()) {
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                ArrayStorage* storage = m_butterfly->arrayStorage();
#if CHECK_ARRAY_CONSISTENCY
                ASSERT(storage->m_inCompactInitialization);
                // Check that we are initializing the next index in sequence.
                ASSERT(i == storage->m_initializationIndex);
                // tryCreateUninitialized set m_numValuesInVector to the initialLength,
                // check we do not try to initialize more than this number of properties.
                ASSERT(storage->m_initializationIndex < storage->m_numValuesInVector);
                storage->m_initializationIndex++;
#endif
                ASSERT(i < storage->length());
                ASSERT(i < storage->m_numValuesInVector);
                storage->m_vector[i].set(globalData, this, v);
                break;
            }
            default:
                ASSERT_NOT_REACHED();
            }
        }
        
        void completeInitialization(unsigned newLength)
        {
            switch (structure()->indexingType()) {
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                ArrayStorage* storage = m_butterfly->arrayStorage();
                // Check that we have initialized as meny properties as we think we have.
                UNUSED_PARAM(storage);
                ASSERT_UNUSED(newLength, newLength == storage->length());
#if CHECK_ARRAY_CONSISTENCY
                // Check that the number of propreties initialized matches the initialLength.
                ASSERT(storage->m_initializationIndex == m_storage->m_numValuesInVector);
                ASSERT(storage->m_inCompactInitialization);
                storage->m_inCompactInitialization = false;
#endif
                break;
            }
            default:
                ASSERT_NOT_REACHED();
            }
        }
        
        bool inSparseIndexingMode()
        {
            switch (structure()->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
                return false;
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                return m_butterfly->arrayStorage()->inSparseMode();
            default:
                ASSERT_NOT_REACHED();
                return false;
            }
        }
        
        void enterDictionaryIndexingMode(JSGlobalData&);

        // putDirect is effectively an unchecked vesion of 'defineOwnProperty':
        //  - the prototype chain is not consulted
        //  - accessors are not called.
        //  - attributes will be respected (after the call the property will exist with the given attributes)
        //  - the property name is assumed to not be an index.
        JS_EXPORT_PRIVATE static void putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned attributes);
        void putDirect(JSGlobalData&, PropertyName, JSValue, unsigned attributes = 0);
        void putDirect(JSGlobalData&, PropertyName, JSValue, PutPropertySlot&);
        void putDirectWithoutTransition(JSGlobalData&, PropertyName, JSValue, unsigned attributes = 0);
        void putDirectAccessor(ExecState*, PropertyName, JSValue, unsigned attributes);

        bool propertyIsEnumerable(ExecState*, const Identifier& propertyName) const;

        JS_EXPORT_PRIVATE bool hasProperty(ExecState*, PropertyName) const;
        JS_EXPORT_PRIVATE bool hasProperty(ExecState*, unsigned propertyName) const;
        bool hasOwnProperty(ExecState*, PropertyName) const;

        JS_EXPORT_PRIVATE static bool deleteProperty(JSCell*, ExecState*, PropertyName);
        JS_EXPORT_PRIVATE static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned propertyName);

        JS_EXPORT_PRIVATE static JSValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);

        JS_EXPORT_PRIVATE static bool hasInstance(JSObject*, ExecState*, JSValue, JSValue prototypeProperty);

        JS_EXPORT_PRIVATE static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        JS_EXPORT_PRIVATE static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        JS_EXPORT_PRIVATE static void getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);

        JSValue toPrimitive(ExecState*, PreferredPrimitiveType = NoPreference) const;
        bool getPrimitiveNumber(ExecState*, double& number, JSValue&) const;
        JS_EXPORT_PRIVATE double toNumber(ExecState*) const;
        JS_EXPORT_PRIVATE JSString* toString(ExecState*) const;

        // NOTE: JSObject and its subclasses must be able to gracefully handle ExecState* = 0,
        // because this call may come from inside the compiler.
        JS_EXPORT_PRIVATE static JSObject* toThisObject(JSCell*, ExecState*);
        JSObject* unwrappedObject();

        bool getPropertySpecificValue(ExecState*, PropertyName, JSCell*& specificFunction) const;

        // This get function only looks at the property map.
        JSValue getDirect(JSGlobalData& globalData, PropertyName propertyName) const
        {
            PropertyOffset offset = structure()->get(globalData, propertyName);
            checkOffset(offset, structure()->typeInfo().type());
            return offset != invalidOffset ? getDirectOffset(offset) : JSValue();
        }

        WriteBarrierBase<Unknown>* getDirectLocation(JSGlobalData& globalData, PropertyName propertyName)
        {
            PropertyOffset offset = structure()->get(globalData, propertyName);
            checkOffset(offset, structure()->typeInfo().type());
            return isValidOffset(offset) ? locationForOffset(offset) : 0;
        }

        WriteBarrierBase<Unknown>* getDirectLocation(JSGlobalData& globalData, PropertyName propertyName, unsigned& attributes)
        {
            JSCell* specificFunction;
            PropertyOffset offset = structure()->get(globalData, propertyName, attributes, specificFunction);
            return isValidOffset(offset) ? locationForOffset(offset) : 0;
        }

        bool hasInlineStorage() const { return structure()->hasInlineStorage(); }
        ConstPropertyStorage inlineStorageUnsafe() const
        {
            return bitwise_cast<ConstPropertyStorage>(this + 1);
        }
        PropertyStorage inlineStorageUnsafe()
        {
            return bitwise_cast<PropertyStorage>(this + 1);
        }
        ConstPropertyStorage inlineStorage() const
        {
            ASSERT(hasInlineStorage());
            return inlineStorageUnsafe();
        }
        PropertyStorage inlineStorage()
        {
            ASSERT(hasInlineStorage());
            return inlineStorageUnsafe();
        }
        
        const Butterfly* butterfly() const { return m_butterfly; }
        Butterfly* butterfly() { return m_butterfly; }
        
        ConstPropertyStorage outOfLineStorage() const { return m_butterfly->propertyStorage(); }
        PropertyStorage outOfLineStorage() { return m_butterfly->propertyStorage(); }

        const WriteBarrierBase<Unknown>* locationForOffset(PropertyOffset offset) const
        {
            if (isInlineOffset(offset))
                return &inlineStorage()[offsetInInlineStorage(offset)];
            return &outOfLineStorage()[offsetInOutOfLineStorage(offset)];
        }

        WriteBarrierBase<Unknown>* locationForOffset(PropertyOffset offset)
        {
            if (isInlineOffset(offset))
                return &inlineStorage()[offsetInInlineStorage(offset)];
            return &outOfLineStorage()[offsetInOutOfLineStorage(offset)];
        }

        PropertyOffset offsetForLocation(WriteBarrierBase<Unknown>* location) const
        {
            PropertyOffset result;
            size_t offsetInInlineStorage = location - inlineStorageUnsafe();
            if (offsetInInlineStorage < static_cast<size_t>(inlineStorageCapacity))
                result = offsetInInlineStorage;
            else
                result = outOfLineStorage() - location + (inlineStorageCapacity - 1);
            validateOffset(result, structure()->typeInfo().type());
            return result;
        }

        void transitionTo(JSGlobalData&, Structure*);

        bool removeDirect(JSGlobalData&, PropertyName); // Return true if anything is removed.
        bool hasCustomProperties() { return structure()->didTransition(); }
        bool hasGetterSetterProperties() { return structure()->hasGetterSetterProperties(); }

        // putOwnDataProperty has 'put' like semantics, however this method:
        //  - assumes the object contains no own getter/setter properties.
        //  - provides no special handling for __proto__
        //  - does not walk the prototype chain (to check for accessors or non-writable properties).
        // This is used by JSActivation.
        bool putOwnDataProperty(JSGlobalData&, PropertyName, JSValue, PutPropertySlot&);

        // Fast access to known property offsets.
        JSValue getDirectOffset(PropertyOffset offset) const { return locationForOffset(offset)->get(); }
        void putDirectOffset(JSGlobalData& globalData, PropertyOffset offset, JSValue value) { locationForOffset(offset)->set(globalData, this, value); }
        void putUndefinedAtDirectOffset(PropertyOffset offset) { locationForOffset(offset)->setUndefined(); }

        JS_EXPORT_PRIVATE static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);

        bool isGlobalObject() const;
        bool isVariableObject() const;
        bool isNameScopeObject() const;
        bool isActivationObject() const;
        bool isErrorInstance() const;
        bool isGlobalThis() const;

        void seal(JSGlobalData&);
        void freeze(JSGlobalData&);
        JS_EXPORT_PRIVATE void preventExtensions(JSGlobalData&);
        bool isSealed(JSGlobalData& globalData) { return structure()->isSealed(globalData); }
        bool isFrozen(JSGlobalData& globalData) { return structure()->isFrozen(globalData); }
        bool isExtensible() { return structure()->isExtensible(); }
        bool indexingShouldBeSparse()
        {
            return !isExtensible()
                || structure()->typeInfo().interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero();
        }

        bool staticFunctionsReified() { return structure()->staticFunctionsReified(); }
        void reifyStaticFunctionsForDelete(ExecState* exec);

        JS_EXPORT_PRIVATE Butterfly* growOutOfLineStorage(JSGlobalData&, size_t oldSize, size_t newSize);
        void setButterfly(JSGlobalData&, Butterfly*, Structure*);
        void setButterflyWithoutChangingStructure(Butterfly*); // You probably don't want to call this.
        
        void setStructureAndReallocateStorageIfNecessary(JSGlobalData&, unsigned oldCapacity, Structure*);
        void setStructureAndReallocateStorageIfNecessary(JSGlobalData&, Structure*);

        void flattenDictionaryObject(JSGlobalData& globalData)
        {
            structure()->flattenDictionaryStructure(globalData, this);
        }

        JSGlobalObject* globalObject() const
        {
            ASSERT(structure()->globalObject());
            ASSERT(!isGlobalObject() || ((JSObject*)structure()->globalObject()) == this);
            return structure()->globalObject();
        }
        
        // Does everything possible to return the global object. If it encounters an object
        // that does not have a global object, it returns 0 instead (for example
        // JSNotAnObject).
        JSGlobalObject* unwrappedGlobalObject();
        
        void switchToSlowPutArrayStorage(JSGlobalData&);
        
        // The receiver is the prototype in this case. The following:
        //
        // asObject(foo->structure()->storedPrototype())->attemptToInterceptPutByIndexOnHoleForPrototype(...)
        //
        // is equivalent to:
        //
        // foo->attemptToInterceptPutByIndexOnHole(...);
        bool attemptToInterceptPutByIndexOnHoleForPrototype(ExecState*, JSValue thisValue, unsigned propertyName, JSValue, bool shouldThrow);
        
        // Ensure that the object is in a mode where it has array storage. Use
        // this if you're about to perform actions that would have required the
        // object to be converted to have array storage, if it didn't have it
        // already.
        ArrayStorage* ensureArrayStorage(JSGlobalData& globalData)
        {
            switch (structure()->indexingType()) {
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                return m_butterfly->arrayStorage();
                
            case ALL_BLANK_INDEXING_TYPES:
                return createInitialArrayStorage(globalData);
                
            default:
                ASSERT_NOT_REACHED();
                return 0;
            }
        }
        
        static size_t offsetOfInlineStorage();
        
        static ptrdiff_t butterflyOffset()
        {
            return OBJECT_OFFSETOF(JSObject, m_butterfly);
        }
        
        void* butterflyAddress()
        {
            return &m_butterfly;
        }

        static JS_EXPORTDATA const ClassInfo s_info;

    protected:
        void finishCreation(JSGlobalData& globalData)
        {
            Base::finishCreation(globalData);
            ASSERT(inherits(&s_info));
            ASSERT(!structure()->outOfLineCapacity());
            ASSERT(structure()->isEmpty());
            ASSERT(prototype().isNull() || Heap::heap(this) == Heap::heap(prototype()));
            ASSERT(structure()->isObject());
            ASSERT(classInfo());
        }

        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }

        // To instantiate objects you likely want JSFinalObject, below.
        // To create derived types you likely want JSNonFinalObject, below.
        JSObject(JSGlobalData&, Structure*, Butterfly* = 0);
        
        void resetInheritorID(JSGlobalData&);
        
        void visitButterfly(SlotVisitor&, Butterfly*, size_t storageSize);

        // Call this if you know that the object is in a mode where it has array
        // storage. This will assert otherwise.
        ArrayStorage* arrayStorage()
        {
            ASSERT(hasArrayStorage(structure()->indexingType()));
            return m_butterfly->arrayStorage();
        }
        
        // Call this if you want to predicate some actions on whether or not the
        // object is in a mode where it has array storage.
        ArrayStorage* arrayStorageOrNull()
        {
            switch (structure()->indexingType()) {
            case ALL_ARRAY_STORAGE_INDEXING_TYPES:
                return m_butterfly->arrayStorage();
                
            default:
                return 0;
            }
        }

        ArrayStorage* createArrayStorage(JSGlobalData&, unsigned length, unsigned vectorLength);
        ArrayStorage* createInitialArrayStorage(JSGlobalData&);
        
        ArrayStorage* ensureArrayStorageExistsAndEnterDictionaryIndexingMode(JSGlobalData&);
        
        bool defineOwnNonIndexProperty(ExecState*, PropertyName, PropertyDescriptor&, bool throwException);

        enum ConsistencyCheckType { NormalConsistencyCheck, DestructorConsistencyCheck, SortConsistencyCheck };
#if !CHECK_ARRAY_CONSISTENCY
        void checkIndexingConsistency(ConsistencyCheckType = NormalConsistencyCheck) { }
#else
        void checkIndexingConsistency(ConsistencyCheckType = NormalConsistencyCheck);
#endif
        
        void putByIndexBeyondVectorLengthWithArrayStorage(ExecState*, unsigned propertyName, JSValue, bool shouldThrow, ArrayStorage*);

        bool increaseVectorLength(JSGlobalData&, unsigned newLength);
        void deallocateSparseIndexMap();
        bool defineOwnIndexedProperty(ExecState*, unsigned, PropertyDescriptor&, bool throwException);
        SparseArrayValueMap* allocateSparseIndexMap(JSGlobalData&);
        
        void notifyPresenceOfIndexedAccessors(JSGlobalData&);
        
        bool attemptToInterceptPutByIndexOnHole(ExecState*, unsigned index, JSValue, bool shouldThrow);

    private:
        friend class LLIntOffsetsExtractor;
        
        // Nobody should ever ask any of these questions on something already known to be a JSObject.
        using JSCell::isAPIValueWrapper;
        using JSCell::isGetterSetter;
        void getObject();
        void getString(ExecState* exec);
        void isObject();
        void isString();
        
        ArrayStorage* enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(JSGlobalData&, ArrayStorage*);
        
        template<PutMode>
        bool putDirectInternal(JSGlobalData&, PropertyName, JSValue, unsigned attr, PutPropertySlot&, JSCell*);

        bool inlineGetOwnPropertySlot(ExecState*, PropertyName, PropertySlot&);
        JS_EXPORT_PRIVATE void fillGetterPropertySlot(PropertySlot&, PropertyOffset);

        const HashEntry* findPropertyHashEntry(ExecState*, PropertyName) const;
        Structure* createInheritorID(JSGlobalData&);
        
        void putIndexedDescriptor(ExecState*, SparseArrayEntry*, PropertyDescriptor&, PropertyDescriptor& old);
        
        void putByIndexBeyondVectorLength(ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
        bool putDirectIndexBeyondVectorLengthWithArrayStorage(ExecState*, unsigned propertyName, JSValue, unsigned attributes, PutDirectIndexMode, ArrayStorage*);
        JS_EXPORT_PRIVATE bool putDirectIndexBeyondVectorLength(ExecState*, unsigned propertyName, JSValue, unsigned attributes, PutDirectIndexMode);
        
        unsigned getNewVectorLength(unsigned currentVectorLength, unsigned currentLength, unsigned desiredLength);
        unsigned getNewVectorLength(unsigned desiredLength);

        JS_EXPORT_PRIVATE bool getOwnPropertySlotSlow(ExecState*, PropertyName, PropertySlot&);
        
    protected:
        Butterfly* m_butterfly;
    };


    // JSNonFinalObject is a type of JSObject that has some internal storage,
    // but also preserves some space in the collector cell for additional
    // data members in derived types.
    class JSNonFinalObject : public JSObject {
        friend class JSObject;

    public:
        typedef JSObject Base;

        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }

        static bool hasInlineStorage()
        {
            return false;
        }

    protected:
        explicit JSNonFinalObject(JSGlobalData& globalData, Structure* structure, Butterfly* butterfly = 0)
            : JSObject(globalData, structure, butterfly)
        {
        }

        void finishCreation(JSGlobalData& globalData)
        {
            Base::finishCreation(globalData);
            ASSERT(!this->structure()->totalStorageCapacity());
            ASSERT(classInfo());
        }
    };

    class JSFinalObject;

    // JSFinalObject is a type of JSObject that contains sufficent internal
    // storage to fully make use of the colloctor cell containing it.
    class JSFinalObject : public JSObject {
        friend class JSObject;

    public:
        typedef JSObject Base;

        static JSFinalObject* create(ExecState*, Structure*);
        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(globalData, globalObject, prototype, TypeInfo(FinalObjectType, StructureFlags), &s_info);
        }

        JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

        static JS_EXPORTDATA const ClassInfo s_info;

        static bool hasInlineStorage()
        {
            return true;
        }
    protected:
        void visitChildrenCommon(SlotVisitor&);
        
        void finishCreation(JSGlobalData& globalData)
        {
            Base::finishCreation(globalData);
            ASSERT(!(OBJECT_OFFSETOF(JSFinalObject, m_inlineStorage) % sizeof(double)));
            ASSERT(this->structure()->inlineCapacity() == static_cast<unsigned>(inlineStorageCapacity));
            ASSERT(this->structure()->totalStorageCapacity() == static_cast<unsigned>(inlineStorageCapacity));
            ASSERT(classInfo());
        }

    private:
        friend class LLIntOffsetsExtractor;
        
        explicit JSFinalObject(JSGlobalData& globalData, Structure* structure)
            : JSObject(globalData, structure)
        {
        }

        static const unsigned StructureFlags = JSObject::StructureFlags;

        WriteBarrierBase<Unknown> m_inlineStorage[INLINE_STORAGE_CAPACITY];
    };

inline JSFinalObject* JSFinalObject::create(ExecState* exec, Structure* structure)
{
    JSFinalObject* finalObject = new (NotNull, allocateCell<JSFinalObject>(*exec->heap())) JSFinalObject(exec->globalData(), structure);
    finalObject->finishCreation(exec->globalData());
    return finalObject;
}

inline bool isJSFinalObject(JSCell* cell)
{
    return cell->classInfo() == &JSFinalObject::s_info;
}

inline bool isJSFinalObject(JSValue value)
{
    return value.isCell() && isJSFinalObject(value.asCell());
}

inline size_t JSObject::offsetOfInlineStorage()
{
    return OBJECT_OFFSETOF(JSFinalObject, m_inlineStorage);
}

inline bool JSObject::isGlobalObject() const
{
    return structure()->typeInfo().type() == GlobalObjectType;
}

inline bool JSObject::isVariableObject() const
{
    return structure()->typeInfo().type() >= VariableObjectType;
}

inline bool JSObject::isNameScopeObject() const
{
    return structure()->typeInfo().type() == NameScopeObjectType;
}

inline bool JSObject::isActivationObject() const
{
    return structure()->typeInfo().type() == ActivationObjectType;
}

inline bool JSObject::isErrorInstance() const
{
    return structure()->typeInfo().type() == ErrorInstanceType;
}

inline bool JSObject::isGlobalThis() const
{
    return structure()->typeInfo().type() == GlobalThisType;
}

inline void JSObject::setButterfly(JSGlobalData& globalData, Butterfly* butterfly, Structure* structure)
{
    ASSERT(structure);
    ASSERT(!butterfly == (!structure->outOfLineCapacity() && !hasIndexingHeader(structure->indexingType())));
    setStructure(globalData, structure);
    m_butterfly = butterfly;
}

inline void JSObject::setButterflyWithoutChangingStructure(Butterfly* butterfly)
{
    m_butterfly = butterfly;
}

inline JSObject* constructEmptyObject(ExecState* exec, Structure* structure)
{
    return JSFinalObject::create(exec, structure);
}

inline CallType getCallData(JSValue value, CallData& callData)
{
    CallType result = value.isCell() ? value.asCell()->methodTable()->getCallData(value.asCell(), callData) : CallTypeNone;
    ASSERT(result == CallTypeNone || value.isValidCallee());
    return result;
}

inline ConstructType getConstructData(JSValue value, ConstructData& constructData)
{
    ConstructType result = value.isCell() ? value.asCell()->methodTable()->getConstructData(value.asCell(), constructData) : ConstructTypeNone;
    ASSERT(result == ConstructTypeNone || value.isValidCallee());
    return result;
}

inline Structure* createEmptyObjectStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
{
    return JSFinalObject::createStructure(globalData, globalObject, prototype);
}

inline JSObject* asObject(JSCell* cell)
{
    ASSERT(cell->isObject());
    return jsCast<JSObject*>(cell);
}

inline JSObject* asObject(JSValue value)
{
    return asObject(value.asCell());
}

inline JSObject::JSObject(JSGlobalData& globalData, Structure* structure, Butterfly* butterfly)
    : JSCell(globalData, structure)
    , m_butterfly(butterfly)
{
}

inline JSValue JSObject::prototype() const
{
    return structure()->storedPrototype();
}

inline bool JSCell::inherits(const ClassInfo* info) const
{
    return classInfo()->isSubClassOf(info);
}

inline const MethodTable* JSCell::methodTable() const
{
    return &classInfo()->methodTable;
}

// this method is here to be after the inline declaration of JSCell::inherits
inline bool JSValue::inherits(const ClassInfo* classInfo) const
{
    return isCell() && asCell()->inherits(classInfo);
}

inline JSObject* JSValue::toThisObject(ExecState* exec) const
{
    return isCell() ? asCell()->methodTable()->toThisObject(asCell(), exec) : toThisObjectSlowCase(exec);
}

ALWAYS_INLINE bool JSObject::inlineGetOwnPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    PropertyOffset offset = structure()->get(exec->globalData(), propertyName);
    if (LIKELY(isValidOffset(offset))) {
        JSValue value = getDirectOffset(offset);
        if (structure()->hasGetterSetterProperties() && value.isGetterSetter())
            fillGetterPropertySlot(slot, offset);
        else
            slot.setValue(this, value, offset);
        return true;
    }

    return getOwnPropertySlotSlow(exec, propertyName, slot);
}

// It may seem crazy to inline a function this large, especially a virtual function,
// but it makes a big difference to property lookup that derived classes can inline their
// base class call to this.
ALWAYS_INLINE bool JSObject::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return jsCast<JSObject*>(cell)->inlineGetOwnPropertySlot(exec, propertyName, slot);
}

ALWAYS_INLINE bool JSCell::fastGetOwnPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    if (!structure()->typeInfo().overridesGetOwnPropertySlot())
        return asObject(this)->inlineGetOwnPropertySlot(exec, propertyName, slot);
    return methodTable()->getOwnPropertySlot(this, exec, propertyName, slot);
}

// Fast call to get a property where we may not yet have converted the string to an
// identifier. The first time we perform a property access with a given string, try
// performing the property map lookup without forming an identifier. We detect this
// case by checking whether the hash has yet been set for this string.
ALWAYS_INLINE JSValue JSCell::fastGetOwnProperty(ExecState* exec, const String& name)
{
    if (!structure()->typeInfo().overridesGetOwnPropertySlot() && !structure()->hasGetterSetterProperties()) {
        PropertyOffset offset = name.impl()->hasHash()
            ? structure()->get(exec->globalData(), Identifier(exec, name))
            : structure()->get(exec->globalData(), name);
        if (offset != invalidOffset)
            return asObject(this)->locationForOffset(offset)->get();
    }
    return JSValue();
}

// It may seem crazy to inline a function this large but it makes a big difference
// since this is function very hot in variable lookup
ALWAYS_INLINE bool JSObject::getPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSObject* object = this;
    while (true) {
        if (object->fastGetOwnPropertySlot(exec, propertyName, slot))
            return true;
        JSValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

ALWAYS_INLINE bool JSObject::getPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    JSObject* object = this;
    while (true) {
        if (object->methodTable()->getOwnPropertySlotByIndex(object, exec, propertyName, slot))
            return true;
        JSValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

inline JSValue JSObject::get(ExecState* exec, PropertyName propertyName) const
{
    PropertySlot slot(this);
    if (const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);
    
    return jsUndefined();
}

inline JSValue JSObject::get(ExecState* exec, unsigned propertyName) const
{
    PropertySlot slot(this);
    if (const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);

    return jsUndefined();
}

template<JSObject::PutMode mode>
inline bool JSObject::putDirectInternal(JSGlobalData& globalData, PropertyName propertyName, JSValue value, unsigned attributes, PutPropertySlot& slot, JSCell* specificFunction)
{
    ASSERT(value);
    ASSERT(value.isGetterSetter() == !!(attributes & Accessor));
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    ASSERT(propertyName.asIndex() == PropertyName::NotAnIndex);

    if (structure()->isDictionary()) {
        unsigned currentAttributes;
        JSCell* currentSpecificFunction;
        PropertyOffset offset = structure()->get(globalData, propertyName, currentAttributes, currentSpecificFunction);
        if (offset != invalidOffset) {
            // If there is currently a specific function, and there now either isn't,
            // or the new value is different, then despecify.
            if (currentSpecificFunction && (specificFunction != currentSpecificFunction))
                structure()->despecifyDictionaryFunction(globalData, propertyName);
            if ((mode == PutModePut) && currentAttributes & ReadOnly)
                return false;

            putDirectOffset(globalData, offset, value);
            // At this point, the objects structure only has a specific value set if previously there
            // had been one set, and if the new value being specified is the same (otherwise we would
            // have despecified, above).  So, if currentSpecificFunction is not set, or if the new
            // value is different (or there is no new value), then the slot now has no value - and
            // as such it is cachable.
            // If there was previously a value, and the new value is the same, then we cannot cache.
            if (!currentSpecificFunction || (specificFunction != currentSpecificFunction))
                slot.setExistingProperty(this, offset);
            return true;
        }

        if ((mode == PutModePut) && !isExtensible())
            return false;

        Butterfly* newButterfly = m_butterfly;
        if (structure()->putWillGrowOutOfLineStorage())
            newButterfly = growOutOfLineStorage(globalData, structure()->outOfLineCapacity(), structure()->suggestedNewOutOfLineStorageCapacity());
        offset = structure()->addPropertyWithoutTransition(globalData, propertyName, attributes, specificFunction);
        setButterfly(globalData, newButterfly, structure());

        validateOffset(offset);
        ASSERT(structure()->isValidOffset(offset));
        putDirectOffset(globalData, offset, value);
        // See comment on setNewProperty call below.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return true;
    }

    PropertyOffset offset;
    size_t currentCapacity = structure()->outOfLineCapacity();
    if (Structure* structure = Structure::addPropertyTransitionToExistingStructure(this->structure(), propertyName, attributes, specificFunction, offset)) {
        Butterfly* newButterfly = m_butterfly;
        if (currentCapacity != structure->outOfLineCapacity())
            newButterfly = growOutOfLineStorage(globalData, currentCapacity, structure->outOfLineCapacity());

        validateOffset(offset);
        ASSERT(structure->isValidOffset(offset));
        setButterfly(globalData, newButterfly, structure);
        putDirectOffset(globalData, offset, value);
        // This is a new property; transitions with specific values are not currently cachable,
        // so leave the slot in an uncachable state.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return true;
    }

    unsigned currentAttributes;
    JSCell* currentSpecificFunction;
    offset = structure()->get(globalData, propertyName, currentAttributes, currentSpecificFunction);
    if (offset != invalidOffset) {
        if ((mode == PutModePut) && currentAttributes & ReadOnly)
            return false;

        // There are three possibilities here:
        //  (1) There is an existing specific value set, and we're overwriting with *the same value*.
        //       * Do nothing - no need to despecify, but that means we can't cache (a cached
        //         put could write a different value). Leave the slot in an uncachable state.
        //  (2) There is a specific value currently set, but we're writing a different value.
        //       * First, we have to despecify.  Having done so, this is now a regular slot
        //         with no specific value, so go ahead & cache like normal.
        //  (3) Normal case, there is no specific value set.
        //       * Go ahead & cache like normal.
        if (currentSpecificFunction) {
            // case (1) Do the put, then return leaving the slot uncachable.
            if (specificFunction == currentSpecificFunction) {
                putDirectOffset(globalData, offset, value);
                return true;
            }
            // case (2) Despecify, fall through to (3).
            setStructure(globalData, Structure::despecifyFunctionTransition(globalData, structure(), propertyName));
        }

        // case (3) set the slot, do the put, return.
        slot.setExistingProperty(this, offset);
        putDirectOffset(globalData, offset, value);
        return true;
    }

    if ((mode == PutModePut) && !isExtensible())
        return false;

    Structure* structure = Structure::addPropertyTransition(globalData, this->structure(), propertyName, attributes, specificFunction, offset);
    
    validateOffset(offset);
    ASSERT(structure->isValidOffset(offset));
    setStructureAndReallocateStorageIfNecessary(globalData, structure);

    putDirectOffset(globalData, offset, value);
    // This is a new property; transitions with specific values are not currently cachable,
    // so leave the slot in an uncachable state.
    if (!specificFunction)
        slot.setNewProperty(this, offset);
    return true;
}

inline void JSObject::setStructureAndReallocateStorageIfNecessary(JSGlobalData& globalData, unsigned oldCapacity, Structure* newStructure)
{
    ASSERT(oldCapacity <= newStructure->outOfLineCapacity());
    
    if (oldCapacity == newStructure->outOfLineCapacity()) {
        setStructure(globalData, newStructure);
        return;
    }
    
    Butterfly* newButterfly = growOutOfLineStorage(
        globalData, oldCapacity, newStructure->outOfLineCapacity());
    setButterfly(globalData, newButterfly, newStructure);
}

inline void JSObject::setStructureAndReallocateStorageIfNecessary(JSGlobalData& globalData, Structure* newStructure)
{
    setStructureAndReallocateStorageIfNecessary(
        globalData, structure()->outOfLineCapacity(), newStructure);
}

inline bool JSObject::putOwnDataProperty(JSGlobalData& globalData, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    ASSERT(!structure()->hasGetterSetterProperties());

    return putDirectInternal<PutModePut>(globalData, propertyName, value, 0, slot, getCallableObject(value));
}

inline void JSObject::putDirect(JSGlobalData& globalData, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    PutPropertySlot slot;
    putDirectInternal<PutModeDefineOwnProperty>(globalData, propertyName, value, attributes, slot, getCallableObject(value));
}

inline void JSObject::putDirect(JSGlobalData& globalData, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(!value.isGetterSetter());
    putDirectInternal<PutModeDefineOwnProperty>(globalData, propertyName, value, 0, slot, getCallableObject(value));
}

inline void JSObject::putDirectWithoutTransition(JSGlobalData& globalData, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    Butterfly* newButterfly = m_butterfly;
    if (structure()->putWillGrowOutOfLineStorage())
        newButterfly = growOutOfLineStorage(globalData, structure()->outOfLineCapacity(), structure()->suggestedNewOutOfLineStorageCapacity());
    PropertyOffset offset = structure()->addPropertyWithoutTransition(globalData, propertyName, attributes, getCallableObject(value));
    setButterfly(globalData, newButterfly, structure());
    putDirectOffset(globalData, offset, value);
}

inline JSValue JSObject::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
{
    return methodTable()->defaultValue(this, exec, preferredType);
}

inline JSValue JSValue::get(ExecState* exec, PropertyName propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline JSValue JSValue::get(ExecState* exec, PropertyName propertyName, PropertySlot& slot) const
{
    if (UNLIKELY(!isCell())) {
        JSObject* prototype = synthesizePrototype(exec);
        if (!prototype->getPropertySlot(exec, propertyName, slot))
            return jsUndefined();
        return slot.getValue(exec, propertyName);
    }
    JSCell* cell = asCell();
    while (true) {
        if (cell->fastGetOwnPropertySlot(exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        JSValue prototype = asObject(cell)->prototype();
        if (!prototype.isObject())
            return jsUndefined();
        cell = asObject(prototype);
    }
}

inline JSValue JSValue::get(ExecState* exec, unsigned propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline JSValue JSValue::get(ExecState* exec, unsigned propertyName, PropertySlot& slot) const
{
    if (UNLIKELY(!isCell())) {
        JSObject* prototype = synthesizePrototype(exec);
        if (!prototype->getPropertySlot(exec, propertyName, slot))
            return jsUndefined();
        return slot.getValue(exec, propertyName);
    }
    JSCell* cell = const_cast<JSCell*>(asCell());
    while (true) {
        if (cell->methodTable()->getOwnPropertySlotByIndex(cell, exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        JSValue prototype = asObject(cell)->prototype();
        if (!prototype.isObject())
            return jsUndefined();
        cell = prototype.asCell();
    }
}

inline void JSValue::put(ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    if (UNLIKELY(!isCell())) {
        putToPrimitive(exec, propertyName, value, slot);
        return;
    }
    asCell()->methodTable()->put(asCell(), exec, propertyName, value, slot);
}

inline void JSValue::putByIndex(ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
{
    if (UNLIKELY(!isCell())) {
        putToPrimitiveByIndex(exec, propertyName, value, shouldThrow);
        return;
    }
    asCell()->methodTable()->putByIndex(asCell(), exec, propertyName, value, shouldThrow);
}

ALWAYS_INLINE JSObject* Register::function() const
{
    if (!jsValue())
        return 0;
    return asObject(jsValue());
}

ALWAYS_INLINE Register Register::withCallee(JSObject* callee)
{
    Register r;
    r = JSValue(callee);
    return r;
}

inline size_t offsetInButterfly(PropertyOffset offset)
{
    return offsetInOutOfLineStorage(offset) + Butterfly::indexOfPropertyStorage();
}

// This is a helper for patching code where you want to emit a load or store and
// the base is:
// For inline offsets: a pointer to the out-of-line storage pointer.
// For out-of-line offsets: the base of the out-of-line storage.
inline size_t offsetRelativeToPatchedStorage(PropertyOffset offset)
{
    if (isOutOfLineOffset(offset))
        return sizeof(EncodedJSValue) * offsetInButterfly(offset);
    return JSObject::offsetOfInlineStorage() - JSObject::butterflyOffset() + sizeof(EncodedJSValue) * offsetInInlineStorage(offset);
}

inline int indexRelativeToBase(PropertyOffset offset)
{
    if (isOutOfLineOffset(offset))
        return offsetInOutOfLineStorage(offset) + Butterfly::indexOfPropertyStorage();
    ASSERT(!(JSObject::offsetOfInlineStorage() % sizeof(EncodedJSValue)));
    return JSObject::offsetOfInlineStorage() / sizeof(EncodedJSValue) + offsetInInlineStorage(offset);
}

inline int offsetRelativeToBase(PropertyOffset offset)
{
    if (isOutOfLineOffset(offset))
        return offsetInOutOfLineStorage(offset) * sizeof(EncodedJSValue) + Butterfly::offsetOfPropertyStorage();
    return JSObject::offsetOfInlineStorage() + offsetInInlineStorage(offset) * sizeof(EncodedJSValue);
}

} // namespace JSC

#endif // JSObject_h
