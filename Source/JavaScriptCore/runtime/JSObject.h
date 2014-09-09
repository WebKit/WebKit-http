/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012, 2013 Apple Inc. All rights reserved.
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
#include "CallFrame.h"
#include "ClassInfo.h"
#include "CommonIdentifiers.h"
#include "CopyWriteBarrier.h"
#include "CustomGetterSetter.h"
#include "DeferGC.h"
#include "Heap.h"
#include "HeapInlines.h"
#include "IndexingHeaderInlines.h"
#include "JSCell.h"
#include "PropertySlot.h"
#include "PropertyStorage.h"
#include "PutDirectIndexMode.h"
#include "PutPropertySlot.h"

#include "Structure.h"
#include "VM.h"
#include "JSString.h"
#include "SparseArrayValueMap.h"
#include <wtf/StdLibExtras.h>

namespace JSC {

inline JSCell* getJSFunction(JSValue value)
{
    if (value.isCell() && (value.asCell()->type() == JSFunctionType))
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
class InternalFunction;
class JSFunction;
class LLIntOffsetsExtractor;
class MarkedBlock;
class PropertyDescriptor;
class PropertyNameArray;
class Structure;
struct HashTable;
struct HashTableValue;

JS_EXPORT_PRIVATE JSObject* throwTypeError(ExecState*, const String&);
extern JS_EXPORTDATA const char* StrictModeReadonlyPropertyWriteError;

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
    JS_EXPORT_PRIVATE friend bool setUpStaticFunctionSlot(ExecState*, const HashTableValue*, JSObject*, PropertyName, PropertySlot&);

    enum PutMode {
        PutModePut,
        PutModeDefineOwnProperty,
    };

public:
    typedef JSCell Base;
        
    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);
    JS_EXPORT_PRIVATE static void copyBackingStore(JSCell*, CopyVisitor&, CopyToken);

    JS_EXPORT_PRIVATE static String className(const JSObject*);

    JSValue prototype() const;
    JS_EXPORT_PRIVATE void setPrototype(VM&, JSValue prototype);
    JS_EXPORT_PRIVATE bool setPrototypeWithCycleCheck(ExecState*, JSValue prototype);
        
    bool mayInterceptIndexedAccesses()
    {
        return structure()->mayInterceptIndexedAccesses();
    }
        
    JSValue get(ExecState*, PropertyName) const;
    JSValue get(ExecState*, unsigned propertyName) const;

    bool fastGetOwnPropertySlot(ExecState*, VM&, Structure&, PropertyName, PropertySlot&);
    bool getPropertySlot(ExecState*, PropertyName, PropertySlot&);
    bool getPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);

    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    JS_EXPORT_PRIVATE static bool getOwnPropertySlotByIndex(JSObject*, ExecState*, unsigned propertyName, PropertySlot&);

    // The key difference between this and getOwnPropertySlot is that getOwnPropertySlot
    // currently returns incorrect results for the DOM window (with non-own properties)
    // being returned. Once this is fixed we should migrate code & remove this method.
    JS_EXPORT_PRIVATE bool getOwnPropertyDescriptor(ExecState*, PropertyName, PropertyDescriptor&);

    JS_EXPORT_PRIVATE bool allowsAccessFrom(ExecState*);

    unsigned getArrayLength() const
    {
        if (!hasIndexedProperties(indexingType()))
            return 0;
        return m_butterfly->publicLength();
    }
        
    unsigned getVectorLength()
    {
        if (!hasIndexedProperties(indexingType()))
            return 0;
        return m_butterfly->vectorLength();
    }
        
    JS_EXPORT_PRIVATE static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    JS_EXPORT_PRIVATE static void putByIndex(JSCell*, ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
        
    void putByIndexInline(ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
    {
        if (canSetIndexQuickly(propertyName)) {
            setIndexQuickly(exec->vm(), propertyName, value);
            return;
        }
        methodTable(exec->vm())->putByIndex(this, exec, propertyName, value, shouldThrow);
    }
        
    // This is similar to the putDirect* methods:
    //  - the prototype chain is not consulted
    //  - accessors are not called.
    //  - it will ignore extensibility and read-only properties if PutDirectIndexLikePutDirect is passed as the mode (the default).
    // This method creates a property with attributes writable, enumerable and configurable all set to true.
    bool putDirectIndex(ExecState* exec, unsigned propertyName, JSValue value, unsigned attributes, PutDirectIndexMode mode)
    {
        if (!attributes && canSetIndexQuicklyForPutDirect(propertyName)) {
            setIndexQuickly(exec->vm(), propertyName, value);
            return true;
        }
        return putDirectIndexBeyondVectorLength(exec, propertyName, value, attributes, mode);
    }
    bool putDirectIndex(ExecState* exec, unsigned propertyName, JSValue value)
    {
        return putDirectIndex(exec, propertyName, value, 0, PutDirectIndexLikePutDirect);
    }

    // A non-throwing version of putDirect and putDirectIndex.
    JS_EXPORT_PRIVATE void putDirectMayBeIndex(ExecState*, PropertyName, JSValue);
        
    bool hasIndexingHeader() const
    {
        return structure()->hasIndexingHeader(this);
    }
    
    bool canGetIndexQuickly(unsigned i)
    {
        switch (indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            return false;
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return i < m_butterfly->vectorLength() && m_butterfly->contiguous()[i];
        case ALL_DOUBLE_INDEXING_TYPES: {
            if (i >= m_butterfly->vectorLength())
                return false;
            double value = m_butterfly->contiguousDouble()[i];
            if (value != value)
                return false;
            return true;
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return i < m_butterfly->arrayStorage()->vectorLength() && m_butterfly->arrayStorage()->m_vector[i];
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    JSValue getIndexQuickly(unsigned i)
    {
        switch (indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
            return jsNumber(m_butterfly->contiguous()[i].get().asInt32());
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous()[i].get();
        case ALL_DOUBLE_INDEXING_TYPES:
            return JSValue(JSValue::EncodeAsDouble, m_butterfly->contiguousDouble()[i]);
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->m_vector[i].get();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return JSValue();
        }
    }
        
    JSValue tryGetIndexQuickly(unsigned i)
    {
        switch (indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            break;
        case ALL_INT32_INDEXING_TYPES:
            if (i < m_butterfly->publicLength()) {
                JSValue result = m_butterfly->contiguous()[i].get();
                ASSERT(result.isInt32() || !result);
                return result;
            }
            break;
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            if (i < m_butterfly->publicLength())
                return m_butterfly->contiguous()[i].get();
            break;
        case ALL_DOUBLE_INDEXING_TYPES: {
            if (i >= m_butterfly->publicLength())
                break;
            double result = m_butterfly->contiguousDouble()[i];
            if (result != result)
                break;
            return JSValue(JSValue::EncodeAsDouble, result);
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            if (i < m_butterfly->arrayStorage()->vectorLength())
                return m_butterfly->arrayStorage()->m_vector[i].get();
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        return JSValue();
    }
        
    JSValue getDirectIndex(ExecState* exec, unsigned i)
    {
        if (JSValue result = tryGetIndexQuickly(i))
            return result;
        PropertySlot slot(this);
        if (methodTable(exec->vm())->getOwnPropertySlotByIndex(this, exec, i, slot))
            return slot.getValue(exec, i);
        return JSValue();
    }
        
    JSValue getIndex(ExecState* exec, unsigned i)
    {
        if (JSValue result = tryGetIndexQuickly(i))
            return result;
        return get(exec, i);
    }
        
    bool canSetIndexQuickly(unsigned i)
    {
        switch (indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            return false;
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
        case NonArrayWithArrayStorage:
        case ArrayWithArrayStorage:
            return i < m_butterfly->vectorLength();
        case NonArrayWithSlowPutArrayStorage:
        case ArrayWithSlowPutArrayStorage:
            return i < m_butterfly->arrayStorage()->vectorLength()
                && !!m_butterfly->arrayStorage()->m_vector[i];
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    bool canSetIndexQuicklyForPutDirect(unsigned i)
    {
        switch (indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            return false;
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return i < m_butterfly->vectorLength();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    void setIndexQuickly(VM& vm, unsigned i, JSValue v)
    {
        switch (indexingType()) {
        case ALL_INT32_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isInt32()) {
                convertInt32ToDoubleOrContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            FALLTHROUGH;
        }
        case ALL_CONTIGUOUS_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->vectorLength());
            m_butterfly->contiguous()[i].set(vm, this, v);
            if (i >= m_butterfly->publicLength())
                m_butterfly->setPublicLength(i + 1);
            break;
        }
        case ALL_DOUBLE_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isNumber()) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            double value = v.asNumber();
            if (value != value) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            m_butterfly->contiguousDouble()[i] = value;
            if (i >= m_butterfly->publicLength())
                m_butterfly->setPublicLength(i + 1);
            break;
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
            ArrayStorage* storage = m_butterfly->arrayStorage();
            WriteBarrier<Unknown>& x = storage->m_vector[i];
            JSValue old = x.get();
            x.set(vm, this, v);
            if (!old) {
                ++storage->m_numValuesInVector;
                if (i >= storage->length())
                    storage->setLength(i + 1);
            }
            break;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
        
    void initializeIndex(VM& vm, unsigned i, JSValue v)
    {
        switch (indexingType()) {
        case ALL_UNDECIDED_INDEXING_TYPES: {
            setIndexQuicklyToUndecided(vm, i, v);
            break;
        }
        case ALL_INT32_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->publicLength());
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isInt32()) {
                convertInt32ToDoubleOrContiguousWhilePerformingSetIndex(vm, i, v);
                break;
            }
            FALLTHROUGH;
        }
        case ALL_CONTIGUOUS_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->publicLength());
            ASSERT(i < m_butterfly->vectorLength());
            m_butterfly->contiguous()[i].set(vm, this, v);
            break;
        }
        case ALL_DOUBLE_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->publicLength());
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isNumber()) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            double value = v.asNumber();
            if (value != value) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            m_butterfly->contiguousDouble()[i] = value;
            break;
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
            ArrayStorage* storage = m_butterfly->arrayStorage();
            ASSERT(i < storage->length());
            ASSERT(i < storage->m_numValuesInVector);
            storage->m_vector[i].set(vm, this, v);
            break;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
        
    bool hasSparseMap()
    {
        switch (indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return false;
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->m_sparseMap;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    bool inSparseIndexingMode()
    {
        switch (indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return false;
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->inSparseMode();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    void enterDictionaryIndexingMode(VM&);

    // putDirect is effectively an unchecked vesion of 'defineOwnProperty':
    //  - the prototype chain is not consulted
    //  - accessors are not called.
    //  - attributes will be respected (after the call the property will exist with the given attributes)
    //  - the property name is assumed to not be an index.
    void putDirect(VM&, PropertyName, JSValue, unsigned attributes = 0);
    void putDirect(VM&, PropertyName, JSValue, PutPropertySlot&);
    void putDirectWithoutTransition(VM&, PropertyName, JSValue, unsigned attributes = 0);
    void putDirectNonIndexAccessor(VM&, PropertyName, JSValue, unsigned attributes);
    void putDirectAccessor(ExecState*, PropertyName, JSValue, unsigned attributes);
    JS_EXPORT_PRIVATE void putDirectCustomAccessor(VM&, PropertyName, JSValue, unsigned attributes);

    JS_EXPORT_PRIVATE bool hasProperty(ExecState*, PropertyName) const;
    JS_EXPORT_PRIVATE bool hasProperty(ExecState*, unsigned propertyName) const;
    bool hasOwnProperty(ExecState*, PropertyName) const;

    JS_EXPORT_PRIVATE static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    JS_EXPORT_PRIVATE static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned propertyName);

    JS_EXPORT_PRIVATE static JSValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);

    bool hasInstance(ExecState*, JSValue);
    static bool defaultHasInstance(ExecState*, JSValue, JSValue prototypeProperty);

    JS_EXPORT_PRIVATE static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    JS_EXPORT_PRIVATE static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    JS_EXPORT_PRIVATE static void getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);

    JSValue toPrimitive(ExecState*, PreferredPrimitiveType = NoPreference) const;
    bool getPrimitiveNumber(ExecState*, double& number, JSValue&) const;
    JS_EXPORT_PRIVATE double toNumber(ExecState*) const;
    JS_EXPORT_PRIVATE JSString* toString(ExecState*) const;

    JS_EXPORT_PRIVATE static JSValue toThis(JSCell*, ExecState*, ECMAMode);

    bool getPropertySpecificValue(ExecState*, PropertyName, JSCell*& specificFunction) const;

    // This get function only looks at the property map.
    JSValue getDirect(VM& vm, PropertyName propertyName) const
    {
        Structure* structure = this->structure(vm);
        PropertyOffset offset = structure->get(vm, propertyName);
        checkOffset(offset, structure->inlineCapacity());
        return offset != invalidOffset ? getDirect(offset) : JSValue();
    }

    JSValue getDirect(VM& vm, PropertyName propertyName, unsigned& attributes) const
    {
        JSCell* specific;
        Structure* structure = this->structure(vm);
        PropertyOffset offset = structure->get(vm, propertyName, attributes, specific);
        checkOffset(offset, structure->inlineCapacity());
        return offset != invalidOffset ? getDirect(offset) : JSValue();
    }

    PropertyOffset getDirectOffset(VM& vm, PropertyName propertyName)
    {
        Structure* structure = this->structure(vm);
        PropertyOffset offset = structure->get(vm, propertyName);
        checkOffset(offset, structure->inlineCapacity());
        return offset;
    }

    PropertyOffset getDirectOffset(VM& vm, PropertyName propertyName, unsigned& attributes)
    {
        JSCell* specific;
        Structure* structure = this->structure(vm);
        PropertyOffset offset = structure->get(vm, propertyName, attributes, specific);
        checkOffset(offset, structure->inlineCapacity());
        return offset;
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
        
    const Butterfly* butterfly() const { return m_butterfly.get(); }
    Butterfly* butterfly() { return m_butterfly.get(); }
        
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

    void transitionTo(VM&, Structure*);

    JS_EXPORT_PRIVATE bool removeDirect(VM&, PropertyName); // Return true if anything is removed.
    bool hasCustomProperties() { return structure()->didTransition(); }
    bool hasGetterSetterProperties() { return structure()->hasGetterSetterProperties(); }
    bool hasCustomGetterSetterProperties() { return structure()->hasCustomGetterSetterProperties(); }

    // putOwnDataProperty has 'put' like semantics, however this method:
    //  - assumes the object contains no own getter/setter properties.
    //  - provides no special handling for __proto__
    //  - does not walk the prototype chain (to check for accessors or non-writable properties).
    // This is used by JSActivation.
    bool putOwnDataProperty(VM&, PropertyName, JSValue, PutPropertySlot&);

    // Fast access to known property offsets.
    JSValue getDirect(PropertyOffset offset) const { return locationForOffset(offset)->get(); }
    void putDirect(VM& vm, PropertyOffset offset, JSValue value) { locationForOffset(offset)->set(vm, this, value); }
    void putDirectUndefined(PropertyOffset offset) { locationForOffset(offset)->setUndefined(); }

    JS_EXPORT_PRIVATE void putDirectNativeFunction(VM&, JSGlobalObject*, const PropertyName&, unsigned functionLength, NativeFunction, Intrinsic, unsigned attributes);
    JS_EXPORT_PRIVATE JSFunction* putDirectBuiltinFunction(VM&, JSGlobalObject*, const PropertyName&, FunctionExecutable*, unsigned attributes);
    JSFunction* putDirectBuiltinFunctionWithoutTransition(VM&, JSGlobalObject*, const PropertyName&, FunctionExecutable*, unsigned attributes);
    JS_EXPORT_PRIVATE void putDirectNativeFunctionWithoutTransition(VM&, JSGlobalObject*, const PropertyName&, unsigned functionLength, NativeFunction, Intrinsic, unsigned attributes);

    JS_EXPORT_PRIVATE static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, const PropertyDescriptor&, bool shouldThrow);

    bool isGlobalObject() const;
    bool isVariableObject() const;
    bool isStaticScopeObject() const;
    bool isNameScopeObject() const;
    bool isActivationObject() const;
    bool isErrorInstance() const;

    JS_EXPORT_PRIVATE void seal(VM&);
    JS_EXPORT_PRIVATE void freeze(VM&);
    JS_EXPORT_PRIVATE void preventExtensions(VM&);
    bool isSealed(VM& vm) { return structure(vm)->isSealed(vm); }
    bool isFrozen(VM& vm) { return structure(vm)->isFrozen(vm); }
    bool isExtensible() { return structure()->isExtensible(); }
    bool indexingShouldBeSparse()
    {
        return !isExtensible()
            || structure()->typeInfo().interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero();
    }

    bool staticFunctionsReified() { return structure()->staticFunctionsReified(); }
    void reifyStaticFunctionsForDelete(ExecState* exec);

    JS_EXPORT_PRIVATE Butterfly* growOutOfLineStorage(VM&, size_t oldSize, size_t newSize);
    void setButterflyWithoutChangingStructure(VM&, Butterfly*);
        
    void setStructure(VM&, Structure*);
    void setStructureAndButterfly(VM&, Structure*, Butterfly*);
    void setStructureAndReallocateStorageIfNecessary(VM&, unsigned oldCapacity, Structure*);
    void setStructureAndReallocateStorageIfNecessary(VM&, Structure*);

    void convertToDictionary(VM& vm)
    {
        setStructure(vm, Structure::toCacheableDictionaryTransition(vm, structure(vm)));
    }

    void flattenDictionaryObject(VM& vm)
    {
        structure(vm)->flattenDictionaryStructure(vm, this);
    }
    void shiftButterflyAfterFlattening(VM&, size_t outOfLineCapacityBefore, size_t outOfLineCapacityAfter);

    JSGlobalObject* globalObject() const
    {
        ASSERT(structure()->globalObject());
        ASSERT(!isGlobalObject() || ((JSObject*)structure()->globalObject()) == this);
        return structure()->globalObject();
    }
        
    void switchToSlowPutArrayStorage(VM&);
        
    // The receiver is the prototype in this case. The following:
    //
    // asObject(foo->structure()->storedPrototype())->attemptToInterceptPutByIndexOnHoleForPrototype(...)
    //
    // is equivalent to:
    //
    // foo->attemptToInterceptPutByIndexOnHole(...);
    bool attemptToInterceptPutByIndexOnHoleForPrototype(ExecState*, JSValue thisValue, unsigned propertyName, JSValue, bool shouldThrow);
        
    // Returns 0 if int32 storage cannot be created - either because
    // indexing should be sparse, we're having a bad time, or because
    // we already have a more general form of storage (double,
    // contiguous, array storage).
    ContiguousJSValues ensureInt32(VM& vm)
    {
        if (LIKELY(hasInt32(indexingType())))
            return m_butterfly->contiguousInt32();
            
        return ensureInt32Slow(vm);
    }
        
    // Returns 0 if double storage cannot be created - either because
    // indexing should be sparse, we're having a bad time, or because
    // we already have a more general form of storage (contiguous,
    // or array storage).
    ContiguousDoubles ensureDouble(VM& vm)
    {
        if (LIKELY(hasDouble(indexingType())))
            return m_butterfly->contiguousDouble();
            
        return ensureDoubleSlow(vm);
    }
        
    // Returns 0 if contiguous storage cannot be created - either because
    // indexing should be sparse or because we're having a bad time.
    ContiguousJSValues ensureContiguous(VM& vm)
    {
        if (LIKELY(hasContiguous(indexingType())))
            return m_butterfly->contiguous();
            
        return ensureContiguousSlow(vm);
    }
        
    // Same as ensureContiguous(), except that if the indexed storage is in
    // double mode, then it does a rage conversion to contiguous: it
    // attempts to convert each double to an int32.
    ContiguousJSValues rageEnsureContiguous(VM& vm)
    {
        if (LIKELY(hasContiguous(indexingType())))
            return m_butterfly->contiguous();
            
        return rageEnsureContiguousSlow(vm);
    }
        
    // Ensure that the object is in a mode where it has array storage. Use
    // this if you're about to perform actions that would have required the
    // object to be converted to have array storage, if it didn't have it
    // already.
    ArrayStorage* ensureArrayStorage(VM& vm)
    {
        if (LIKELY(hasAnyArrayStorage(indexingType())))
            return m_butterfly->arrayStorage();

        return ensureArrayStorageSlow(vm);
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

    DECLARE_EXPORT_INFO;

protected:
    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        ASSERT(inherits(info()));
        ASSERT(!structure()->outOfLineCapacity());
        ASSERT(structure()->isEmpty());
        ASSERT(prototype().isNull() || Heap::heap(this) == Heap::heap(prototype()));
        ASSERT(structure()->isObject());
        ASSERT(classInfo());
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

    // To instantiate objects you likely want JSFinalObject, below.
    // To create derived types you likely want JSNonFinalObject, below.
    JSObject(VM&, Structure*, Butterfly* = 0);
        
    void visitButterfly(SlotVisitor&, Butterfly*, size_t storageSize);
    void copyButterfly(CopyVisitor&, Butterfly*, size_t storageSize);

    // Call this if you know that the object is in a mode where it has array
    // storage. This will assert otherwise.
    ArrayStorage* arrayStorage()
    {
        ASSERT(hasAnyArrayStorage(indexingType()));
        return m_butterfly->arrayStorage();
    }
        
    // Call this if you want to predicate some actions on whether or not the
    // object is in a mode where it has array storage.
    ArrayStorage* arrayStorageOrNull()
    {
        switch (indexingType()) {
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage();
                
        default:
            return 0;
        }
    }
        
    size_t butterflyTotalSize();
    size_t butterflyPreCapacity();

    Butterfly* createInitialUndecided(VM&, unsigned length);
    ContiguousJSValues createInitialInt32(VM&, unsigned length);
    ContiguousDoubles createInitialDouble(VM&, unsigned length);
    ContiguousJSValues createInitialContiguous(VM&, unsigned length);
        
    void convertUndecidedForValue(VM&, JSValue);
    void createInitialForValueAndSet(VM&, unsigned index, JSValue);
    void convertInt32ForValue(VM&, JSValue);
        
    ArrayStorage* createArrayStorage(VM&, unsigned length, unsigned vectorLength);
    ArrayStorage* createInitialArrayStorage(VM&);
        
    ContiguousJSValues convertUndecidedToInt32(VM&);
    ContiguousDoubles convertUndecidedToDouble(VM&);
    ContiguousJSValues convertUndecidedToContiguous(VM&);
    ArrayStorage* convertUndecidedToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertUndecidedToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertUndecidedToArrayStorage(VM&);
        
    ContiguousDoubles convertInt32ToDouble(VM&);
    ContiguousJSValues convertInt32ToContiguous(VM&);
    ArrayStorage* convertInt32ToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertInt32ToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertInt32ToArrayStorage(VM&);
    
    ContiguousJSValues convertDoubleToContiguous(VM&);
    ContiguousJSValues rageConvertDoubleToContiguous(VM&);
    ArrayStorage* convertDoubleToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertDoubleToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertDoubleToArrayStorage(VM&);
        
    ArrayStorage* convertContiguousToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertContiguousToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertContiguousToArrayStorage(VM&);

        
    ArrayStorage* ensureArrayStorageExistsAndEnterDictionaryIndexingMode(VM&);
        
    bool defineOwnNonIndexProperty(ExecState*, PropertyName, const PropertyDescriptor&, bool throwException);

    template<IndexingType indexingShape>
    void putByIndexBeyondVectorLengthWithoutAttributes(ExecState*, unsigned propertyName, JSValue);
    void putByIndexBeyondVectorLengthWithArrayStorage(ExecState*, unsigned propertyName, JSValue, bool shouldThrow, ArrayStorage*);

    bool increaseVectorLength(VM&, unsigned newLength);
    void deallocateSparseIndexMap();
    bool defineOwnIndexedProperty(ExecState*, unsigned, const PropertyDescriptor&, bool throwException);
    SparseArrayValueMap* allocateSparseIndexMap(VM&);
        
    void notifyPresenceOfIndexedAccessors(VM&);
        
    bool attemptToInterceptPutByIndexOnHole(ExecState*, unsigned index, JSValue, bool shouldThrow);
        
    // Call this if you want setIndexQuickly to succeed and you're sure that
    // the array is contiguous.
    void ensureLength(VM& vm, unsigned length)
    {
        ASSERT(length < MAX_ARRAY_INDEX);
        ASSERT(hasContiguous(indexingType()) || hasInt32(indexingType()) || hasDouble(indexingType()) || hasUndecided(indexingType()));
            
        if (m_butterfly->vectorLength() < length)
            ensureLengthSlow(vm, length);
            
        if (m_butterfly->publicLength() < length)
            m_butterfly->setPublicLength(length);
    }
        
    template<IndexingType indexingShape>
    unsigned countElements(Butterfly*);
        
    // This is relevant to undecided, int32, double, and contiguous.
    unsigned countElements();
        
    // This strange method returns a pointer to the start of the indexed data
    // as if it contained JSValues. But it won't always contain JSValues.
    // Make sure you cast this to the appropriate type before using.
    template<IndexingType indexingType>
    ContiguousJSValues indexingData()
    {
        switch (indexingType) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous();
                
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->vector();

        default:
            CRASH();
            return ContiguousJSValues();
        }
    }

    ContiguousJSValues currentIndexingData()
    {
        switch (indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous();

        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->vector();

        default:
            CRASH();
            return ContiguousJSValues();
        }
    }
        
    JSValue getHolyIndexQuickly(unsigned i)
    {
        ASSERT(i < m_butterfly->vectorLength());
        switch (indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous()[i].get();
        case ALL_DOUBLE_INDEXING_TYPES: {
            double value = m_butterfly->contiguousDouble()[i];
            if (value == value)
                return JSValue(JSValue::EncodeAsDouble, value);
            return JSValue();
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->m_vector[i].get();
        default:
            CRASH();
            return JSValue();
        }
    }
        
    template<IndexingType indexingType>
    unsigned relevantLength()
    {
        switch (indexingType) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->publicLength();
                
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return std::min(
                m_butterfly->arrayStorage()->length(),
                m_butterfly->arrayStorage()->vectorLength());
                
        default:
            CRASH();
            return 0;
        }
    }

    unsigned currentRelevantLength()
    {
        switch (indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->publicLength();

        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return std::min(
                m_butterfly->arrayStorage()->length(),
                m_butterfly->arrayStorage()->vectorLength());

        default:
            CRASH();
            return 0;
        }
    }

private:
    friend class LLIntOffsetsExtractor;
        
    // Nobody should ever ask any of these questions on something already known to be a JSObject.
    using JSCell::isAPIValueWrapper;
    using JSCell::isGetterSetter;
    void getObject();
    void getString(ExecState* exec);
    void isObject();
    void isString();
        
    Butterfly* createInitialIndexedStorage(VM&, unsigned length, size_t elementSize);
        
    ArrayStorage* enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(VM&, ArrayStorage*);
        
    template<PutMode>
    bool putDirectInternal(VM&, PropertyName, JSValue, unsigned attr, PutPropertySlot&, JSCell*);

    bool inlineGetOwnPropertySlot(VM&, Structure&, PropertyName, PropertySlot&);
    JS_EXPORT_PRIVATE void fillGetterPropertySlot(PropertySlot&, JSValue, unsigned, PropertyOffset);
    void fillCustomGetterPropertySlot(PropertySlot&, JSValue, unsigned, Structure&);

    const HashTableValue* findPropertyHashEntry(PropertyName) const;
        
    void putIndexedDescriptor(ExecState*, SparseArrayEntry*, const PropertyDescriptor&, PropertyDescriptor& old);
        
    void putByIndexBeyondVectorLength(ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
    bool putDirectIndexBeyondVectorLengthWithArrayStorage(ExecState*, unsigned propertyName, JSValue, unsigned attributes, PutDirectIndexMode, ArrayStorage*);
    JS_EXPORT_PRIVATE bool putDirectIndexBeyondVectorLength(ExecState*, unsigned propertyName, JSValue, unsigned attributes, PutDirectIndexMode);
        
    unsigned getNewVectorLength(unsigned currentVectorLength, unsigned currentLength, unsigned desiredLength);
    unsigned getNewVectorLength(unsigned desiredLength);

    ArrayStorage* constructConvertedArrayStorageWithoutCopyingElements(VM&, unsigned neededLength);
        
    JS_EXPORT_PRIVATE void setIndexQuicklyToUndecided(VM&, unsigned index, JSValue);
    JS_EXPORT_PRIVATE void convertInt32ToDoubleOrContiguousWhilePerformingSetIndex(VM&, unsigned index, JSValue);
    JS_EXPORT_PRIVATE void convertDoubleToContiguousWhilePerformingSetIndex(VM&, unsigned index, JSValue);
        
    void ensureLengthSlow(VM&, unsigned length);
        
    ContiguousJSValues ensureInt32Slow(VM&);
    ContiguousDoubles ensureDoubleSlow(VM&);
    ContiguousJSValues ensureContiguousSlow(VM&);
    ContiguousJSValues rageEnsureContiguousSlow(VM&);
    JS_EXPORT_PRIVATE ArrayStorage* ensureArrayStorageSlow(VM&);
    
    enum DoubleToContiguousMode { EncodeValueAsDouble, RageConvertDoubleToValue };
    template<DoubleToContiguousMode mode>
    ContiguousJSValues genericConvertDoubleToContiguous(VM&);
    ContiguousJSValues ensureContiguousSlow(VM&, DoubleToContiguousMode);
    
protected:
    CopyWriteBarrier<Butterfly> m_butterfly;
#if USE(JSVALUE32_64)
private:
    uint32_t m_padding;
#endif
};

// JSNonFinalObject is a type of JSObject that has some internal storage,
// but also preserves some space in the collector cell for additional
// data members in derived types.
class JSNonFinalObject : public JSObject {
    friend class JSObject;

public:
    typedef JSObject Base;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

protected:
    explicit JSNonFinalObject(VM& vm, Structure* structure, Butterfly* butterfly = 0)
        : JSObject(vm, structure, butterfly)
    {
    }

    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
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

    static size_t allocationSize(size_t inlineCapacity)
    {
        return sizeof(JSObject) + inlineCapacity * sizeof(WriteBarrierBase<Unknown>);
    }
        
    static const unsigned defaultSize = 64;
    static inline unsigned defaultInlineCapacity()
    {
        return (defaultSize - allocationSize(0)) / sizeof(WriteBarrier<Unknown>);
    }

    static const unsigned maxSize = 512;
    static inline unsigned maxInlineCapacity()
    {
        return (maxSize - allocationSize(0)) / sizeof(WriteBarrier<Unknown>);
    }

    static JSFinalObject* create(ExecState*, Structure*);
    static JSFinalObject* create(VM&, Structure*);
    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype, unsigned inlineCapacity)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(FinalObjectType, StructureFlags), info(), NonArray, inlineCapacity);
    }

    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

    DECLARE_EXPORT_INFO;

protected:
    void visitChildrenCommon(SlotVisitor&);
        
    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        ASSERT(structure()->totalStorageCapacity() == structure()->inlineCapacity());
        ASSERT(classInfo());
    }

private:
    friend class LLIntOffsetsExtractor;

    explicit JSFinalObject(VM& vm, Structure* structure)
        : JSObject(vm, structure)
    {
    }

    static const unsigned StructureFlags = JSObject::StructureFlags;
};

inline JSFinalObject* JSFinalObject::create(ExecState* exec, Structure* structure)
{
    JSFinalObject* finalObject = new (
        NotNull, 
        allocateCell<JSFinalObject>(
            *exec->heap(),
            allocationSize(structure->inlineCapacity())
        )
    ) JSFinalObject(exec->vm(), structure);
    finalObject->finishCreation(exec->vm());
    return finalObject;
}

inline JSFinalObject* JSFinalObject::create(VM& vm, Structure* structure)
{
    JSFinalObject* finalObject = new (NotNull, allocateCell<JSFinalObject>(vm.heap, allocationSize(structure->inlineCapacity()))) JSFinalObject(vm, structure);
    finalObject->finishCreation(vm);
    return finalObject;
}

inline bool isJSFinalObject(JSCell* cell)
{
    return cell->classInfo() == JSFinalObject::info();
}

inline bool isJSFinalObject(JSValue value)
{
    return value.isCell() && isJSFinalObject(value.asCell());
}

inline size_t JSObject::offsetOfInlineStorage()
{
    return sizeof(JSObject);
}

inline bool JSObject::isGlobalObject() const
{
    return type() == GlobalObjectType;
}

inline bool JSObject::isVariableObject() const
{
    return type() == GlobalObjectType || type() == ActivationObjectType;
}

inline bool JSObject::isStaticScopeObject() const
{
    JSType type = this->type();
    return type == NameScopeObjectType || type == ActivationObjectType;
}


inline bool JSObject::isNameScopeObject() const
{
    return type() == NameScopeObjectType;
}

inline bool JSObject::isActivationObject() const
{
    return type() == ActivationObjectType;
}

inline bool JSObject::isErrorInstance() const
{
    return type() == ErrorInstanceType;
}

inline void JSObject::setStructureAndButterfly(VM& vm, Structure* structure, Butterfly* butterfly)
{
    ASSERT(structure);
    ASSERT(!butterfly == (!structure->outOfLineCapacity() && !structure->hasIndexingHeader(this)));
    m_butterfly.set(vm, this, butterfly);
    setStructure(vm, structure);
}

inline void JSObject::setStructure(VM& vm, Structure* structure)
{
    ASSERT(structure);
    ASSERT(!m_butterfly == !(structure->outOfLineCapacity() || structure->hasIndexingHeader(this)));
    JSCell::setStructure(vm, structure);
}

inline void JSObject::setButterflyWithoutChangingStructure(VM& vm, Butterfly* butterfly)
{
    m_butterfly.set(vm, this, butterfly);
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

inline JSObject* asObject(JSCell* cell)
{
    ASSERT(cell->isObject());
    return jsCast<JSObject*>(cell);
}

inline JSObject* asObject(JSValue value)
{
    return asObject(value.asCell());
}

inline JSObject::JSObject(VM& vm, Structure* structure, Butterfly* butterfly)
    : JSCell(vm, structure)
    , m_butterfly(vm, this, butterfly)
{
    vm.heap.ascribeOwner(this, butterfly);
}

inline JSValue JSObject::prototype() const
{
    return structure()->storedPrototype();
}

ALWAYS_INLINE bool JSObject::inlineGetOwnPropertySlot(VM& vm, Structure& structure, PropertyName propertyName, PropertySlot& slot)
{
    unsigned attributes;
    JSCell* specific;
    PropertyOffset offset = structure.get(vm, propertyName, attributes, specific);
    if (!isValidOffset(offset))
        return false;

    JSValue value = getDirect(offset);
    if (structure.hasGetterSetterProperties() && value.isGetterSetter())
        fillGetterPropertySlot(slot, value, attributes, offset);
    else if (structure.hasCustomGetterSetterProperties() && value.isCustomGetterSetter())
        fillCustomGetterPropertySlot(slot, value, attributes, structure);
    else
        slot.setValue(this, attributes, value, offset);

    return true;
}

ALWAYS_INLINE void JSObject::fillCustomGetterPropertySlot(PropertySlot& slot, JSValue customGetterSetter, unsigned attributes, Structure& structure)
{
    if (structure.isDictionary()) {
        slot.setCustom(this, attributes, jsCast<CustomGetterSetter*>(customGetterSetter)->getter());
        return;
    }
    slot.setCacheableCustom(this, attributes, jsCast<CustomGetterSetter*>(customGetterSetter)->getter());
}

// It may seem crazy to inline a function this large, especially a virtual function,
// but it makes a big difference to property lookup that derived classes can inline their
// base class call to this.
ALWAYS_INLINE bool JSObject::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    VM& vm = exec->vm();
    Structure& structure = *object->structure(vm);
    if (object->inlineGetOwnPropertySlot(vm, structure, propertyName, slot))
        return true;
    unsigned index = propertyName.asIndex();
    if (index != PropertyName::NotAnIndex)
        return getOwnPropertySlotByIndex(object, exec, index, slot);
    return false;
}

ALWAYS_INLINE bool JSObject::fastGetOwnPropertySlot(ExecState* exec, VM& vm, Structure& structure, PropertyName propertyName, PropertySlot& slot)
{
    if (LIKELY(!TypeInfo::overridesGetOwnPropertySlot(inlineTypeFlags())))
        return inlineGetOwnPropertySlot(vm, structure, propertyName, slot);
    return structure.classInfo()->methodTable.getOwnPropertySlot(this, exec, propertyName, slot);
}

// It may seem crazy to inline a function this large but it makes a big difference
// since this is function very hot in variable lookup
ALWAYS_INLINE bool JSObject::getPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    VM& vm = exec->vm();
    auto& structureIDTable = vm.heap.structureIDTable();
    JSObject* object = this;
    while (true) {
        Structure& structure = *structureIDTable.get(object->structureID());
        if (object->fastGetOwnPropertySlot(exec, vm, structure, propertyName, slot))
            return true;
        JSValue prototype = structure.storedPrototype();
        if (!prototype.isObject())
            break;
        object = asObject(prototype);
    }

    unsigned index = propertyName.asIndex();
    if (index != PropertyName::NotAnIndex)
        return getPropertySlot(exec, index, slot);
    return false;
}

ALWAYS_INLINE bool JSObject::getPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    VM& vm = exec->vm();
    auto& structureIDTable = vm.heap.structureIDTable();
    JSObject* object = this;
    while (true) {
        Structure& structure = *structureIDTable.get(object->structureID());
        if (structure.classInfo()->methodTable.getOwnPropertySlotByIndex(object, exec, propertyName, slot))
            return true;
        JSValue prototype = structure.storedPrototype();
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
inline bool JSObject::putDirectInternal(VM& vm, PropertyName propertyName, JSValue value, unsigned attributes, PutPropertySlot& slot, JSCell* specificFunction)
{
    ASSERT(value);
    ASSERT(value.isGetterSetter() == !!(attributes & Accessor));
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    ASSERT(propertyName.asIndex() == PropertyName::NotAnIndex);

    Structure* structure = this->structure(vm);
    if (structure->isDictionary()) {
        unsigned currentAttributes;
        JSCell* currentSpecificFunction;
        PropertyOffset offset = structure->get(vm, propertyName, currentAttributes, currentSpecificFunction);
        if (offset != invalidOffset) {
            // If there is currently a specific function, and there now either isn't,
            // or the new value is different, then despecify.
            if (currentSpecificFunction && (specificFunction != currentSpecificFunction))
                structure->despecifyDictionaryFunction(vm, propertyName);
            if ((mode == PutModePut) && currentAttributes & ReadOnly)
                return false;

            putDirect(vm, offset, value);
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

        DeferGC deferGC(vm.heap);
        Butterfly* newButterfly = butterfly();
        if (this->structure()->putWillGrowOutOfLineStorage())
            newButterfly = growOutOfLineStorage(vm, this->structure()->outOfLineCapacity(), this->structure()->suggestedNewOutOfLineStorageCapacity());
        offset = this->structure()->addPropertyWithoutTransition(vm, propertyName, attributes, specificFunction);
        setStructureAndButterfly(vm, this->structure(), newButterfly);

        validateOffset(offset);
        ASSERT(this->structure()->isValidOffset(offset));
        putDirect(vm, offset, value);
        // See comment on setNewProperty call below.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        if (attributes & ReadOnly)
            this->structure()->setContainsReadOnlyProperties();
        return true;
    }

    PropertyOffset offset;
    size_t currentCapacity = this->structure()->outOfLineCapacity();
    if (Structure* structure = Structure::addPropertyTransitionToExistingStructure(this->structure(), propertyName, attributes, specificFunction, offset)) {
        DeferGC deferGC(vm.heap);
        Butterfly* newButterfly = butterfly();
        if (currentCapacity != structure->outOfLineCapacity()) {
            ASSERT(structure != this->structure());
            newButterfly = growOutOfLineStorage(vm, currentCapacity, structure->outOfLineCapacity());
        }

        validateOffset(offset);
        ASSERT(structure->isValidOffset(offset));
        setStructureAndButterfly(vm, structure, newButterfly);
        putDirect(vm, offset, value);
        // This is a new property; transitions with specific values are not currently cachable,
        // so leave the slot in an uncachable state.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return true;
    }

    unsigned currentAttributes;
    JSCell* currentSpecificFunction;
    offset = structure->get(vm, propertyName, currentAttributes, currentSpecificFunction);
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
                putDirect(vm, offset, value);
                return true;
            }
            // case (2) Despecify, fall through to (3).
            setStructure(vm, Structure::despecifyFunctionTransition(vm, structure, propertyName));
        }

        // case (3) set the slot, do the put, return.
        slot.setExistingProperty(this, offset);
        putDirect(vm, offset, value);
        return true;
    }

    if ((mode == PutModePut) && !isExtensible())
        return false;

    structure = Structure::addPropertyTransition(vm, structure, propertyName, attributes, specificFunction, offset, slot.context());
    
    validateOffset(offset);
    ASSERT(structure->isValidOffset(offset));
    setStructureAndReallocateStorageIfNecessary(vm, structure);

    putDirect(vm, offset, value);
    // This is a new property; transitions with specific values are not currently cachable,
    // so leave the slot in an uncachable state.
    if (!specificFunction)
        slot.setNewProperty(this, offset);
    if (attributes & ReadOnly)
        structure->setContainsReadOnlyProperties();
    return true;
}

inline void JSObject::setStructureAndReallocateStorageIfNecessary(VM& vm, unsigned oldCapacity, Structure* newStructure)
{
    ASSERT(oldCapacity <= newStructure->outOfLineCapacity());
    
    if (oldCapacity == newStructure->outOfLineCapacity()) {
        setStructure(vm, newStructure);
        return;
    }

    DeferGC deferGC(vm.heap); 
    Butterfly* newButterfly = growOutOfLineStorage(
        vm, oldCapacity, newStructure->outOfLineCapacity());
    setStructureAndButterfly(vm, newStructure, newButterfly);
}

inline void JSObject::setStructureAndReallocateStorageIfNecessary(VM& vm, Structure* newStructure)
{
    setStructureAndReallocateStorageIfNecessary(
        vm, structure(vm)->outOfLineCapacity(), newStructure);
}

inline bool JSObject::putOwnDataProperty(VM& vm, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    ASSERT(!structure()->hasGetterSetterProperties());
    ASSERT(!structure()->hasCustomGetterSetterProperties());

    return putDirectInternal<PutModePut>(vm, propertyName, value, 0, slot, getCallableObject(value));
}

inline void JSObject::putDirect(VM& vm, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    ASSERT(!value.isCustomGetterSetter());
    PutPropertySlot slot(this);
    putDirectInternal<PutModeDefineOwnProperty>(vm, propertyName, value, attributes, slot, getCallableObject(value));
}

inline void JSObject::putDirect(VM& vm, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(!value.isGetterSetter());
    ASSERT(!value.isCustomGetterSetter());
    putDirectInternal<PutModeDefineOwnProperty>(vm, propertyName, value, 0, slot, getCallableObject(value));
}

inline void JSObject::putDirectWithoutTransition(VM& vm, PropertyName propertyName, JSValue value, unsigned attributes)
{
    DeferGC deferGC(vm.heap);
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    ASSERT(!value.isCustomGetterSetter());
    Butterfly* newButterfly = m_butterfly.get();
    if (structure()->putWillGrowOutOfLineStorage())
        newButterfly = growOutOfLineStorage(vm, structure()->outOfLineCapacity(), structure()->suggestedNewOutOfLineStorageCapacity());
    PropertyOffset offset = structure()->addPropertyWithoutTransition(vm, propertyName, attributes, getCallableObject(value));
    setStructureAndButterfly(vm, structure(), newButterfly);
    putDirect(vm, offset, value);
}

inline JSValue JSObject::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
{
    return methodTable()->defaultValue(this, exec, preferredType);
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

inline size_t JSObject::butterflyPreCapacity()
{
    if (UNLIKELY(hasIndexingHeader()))
        return butterfly()->indexingHeader()->preCapacity(structure());
    return 0;
}

inline size_t JSObject::butterflyTotalSize()
{
    Structure* structure = this->structure();
    Butterfly* butterfly = this->butterfly();
    size_t preCapacity;
    size_t indexingPayloadSizeInBytes;
    bool hasIndexingHeader = this->hasIndexingHeader();

    if (UNLIKELY(hasIndexingHeader)) {
        preCapacity = butterfly->indexingHeader()->preCapacity(structure);
        indexingPayloadSizeInBytes = butterfly->indexingHeader()->indexingPayloadSizeInBytes(structure);
    } else {
        preCapacity = 0;
        indexingPayloadSizeInBytes = 0;
    }

    return Butterfly::totalSize(preCapacity, structure->outOfLineCapacity(), hasIndexingHeader, indexingPayloadSizeInBytes);
}

// Helpers for patching code where you want to emit a load or store and
// the base is:
// For inline offsets: a pointer to the out-of-line storage pointer.
// For out-of-line offsets: the base of the out-of-line storage.
inline size_t offsetRelativeToPatchedStorage(PropertyOffset offset)
{
    if (isOutOfLineOffset(offset))
        return sizeof(EncodedJSValue) * offsetInButterfly(offset);
    return JSObject::offsetOfInlineStorage() - JSObject::butterflyOffset() + sizeof(EncodedJSValue) * offsetInInlineStorage(offset);
}

// Returns the maximum offset (away from zero) a load instruction will encode.
inline size_t maxOffsetRelativeToPatchedStorage(PropertyOffset offset)
{
    ptrdiff_t addressOffset = static_cast<ptrdiff_t>(offsetRelativeToPatchedStorage(offset));
#if USE(JSVALUE32_64)
    if (addressOffset >= 0)
        return static_cast<size_t>(addressOffset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag);
#endif
    return static_cast<size_t>(addressOffset);
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

COMPILE_ASSERT(!(sizeof(JSObject) % sizeof(WriteBarrierBase<Unknown>)), JSObject_inline_storage_has_correct_alignment);

ALWAYS_INLINE Identifier makeIdentifier(VM& vm, const char* name)
{
    return Identifier(&vm, name);
}

ALWAYS_INLINE Identifier makeIdentifier(VM&, const Identifier& name)
{
    return name;
}

// Helper for defining native functions, if you're not using a static hash table.
// Use this macro from within finishCreation() methods in prototypes. This assumes
// you've defined variables called exec, globalObject, and vm, and they
// have the expected meanings.
#define JSC_NATIVE_INTRINSIC_FUNCTION(jsName, cppName, attributes, length, intrinsic) \
    putDirectNativeFunction(\
        vm, globalObject, makeIdentifier(vm, (jsName)), (length), cppName, \
        (intrinsic), (attributes))

// As above, but this assumes that the function you're defining doesn't have an
// intrinsic.
#define JSC_NATIVE_FUNCTION(jsName, cppName, attributes, length) \
    JSC_NATIVE_INTRINSIC_FUNCTION(jsName, cppName, (attributes), (length), NoIntrinsic)

ALWAYS_INLINE JSValue PropertySlot::getValue(ExecState* exec, PropertyName propertyName) const
{
    if (m_propertyType == TypeValue)
        return JSValue::decode(m_data.value);
    if (m_propertyType == TypeGetter)
        return functionGetter(exec);
    return JSValue::decode(m_data.custom.getValue(exec, slotBase(), JSValue::encode(m_thisValue), propertyName));
}

ALWAYS_INLINE JSValue PropertySlot::getValue(ExecState* exec, unsigned propertyName) const
{
    if (m_propertyType == TypeValue)
        return JSValue::decode(m_data.value);
    if (m_propertyType == TypeGetter)
        return functionGetter(exec);
    return JSValue::decode(m_data.custom.getValue(exec, slotBase(), JSValue::encode(m_thisValue), Identifier::from(exec, propertyName)));
}

} // namespace JSC

#endif // JSObject_h
