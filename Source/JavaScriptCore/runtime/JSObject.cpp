/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel (eric@webkit.org)
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

#include "config.h"
#include "JSObject.h"

#include "CopiedSpaceInlineMethods.h"
#include "DatePrototype.h"
#include "ErrorConstructor.h"
#include "GetterSetter.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSGlobalThis.h"
#include "Lookup.h"
#include "NativeErrorConstructor.h"
#include "Nodes.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "PropertyDescriptor.h"
#include "PropertyNameArray.h"
#include <math.h>
#include <wtf/Assertions.h>

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(JSObject);
ASSERT_CLASS_FITS_IN_CELL(JSNonFinalObject);
ASSERT_CLASS_FITS_IN_CELL(JSFinalObject);

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSObject);
ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSFinalObject);

const char* StrictModeReadonlyPropertyWriteError = "Attempted to assign to readonly property.";

const ClassInfo JSObject::s_info = { "Object", 0, 0, 0, CREATE_METHOD_TABLE(JSObject) };

const ClassInfo JSFinalObject::s_info = { "Object", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSFinalObject) };

static inline void getClassPropertyNames(ExecState* exec, const ClassInfo* classInfo, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Add properties from the static hashtables of properties
    for (; classInfo; classInfo = classInfo->parentClass) {
        const HashTable* table = classInfo->propHashTable(exec);
        if (!table)
            continue;
        table->initializeIfNeeded(exec);
        ASSERT(table->table);

        int hashSizeMask = table->compactSize - 1;
        const HashEntry* entry = table->table;
        for (int i = 0; i <= hashSizeMask; ++i, ++entry) {
            if (entry->key() && (!(entry->attributes() & DontEnum) || (mode == IncludeDontEnumProperties)))
                propertyNames.add(entry->key());
        }
    }
}

void JSObject::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
#if !ASSERT_DISABLED
    bool wasCheckingForDefaultMarkViolation = visitor.m_isCheckingForDefaultMarkViolation;
    visitor.m_isCheckingForDefaultMarkViolation = false;
#endif

    JSCell::visitChildren(thisObject, visitor);

    PropertyStorage storage = thisObject->propertyStorage();
    size_t storageSize = thisObject->structure()->propertyStorageSize();
    if (thisObject->isUsingInlineStorage())
        visitor.appendValues(storage, storageSize);
    else {
        // We have this extra temp here to slake GCC's thirst for the blood of those who dereference type-punned pointers.
        void* temp = storage;
        visitor.copyAndAppend(&temp, thisObject->structure()->propertyStorageCapacity() * sizeof(WriteBarrierBase<Unknown>), storage->slot(), storageSize);
        storage = static_cast<PropertyStorage>(temp);
        thisObject->m_propertyStorage.set(storage, StorageBarrier::Unchecked);
    }

    if (thisObject->m_inheritorID)
        visitor.append(&thisObject->m_inheritorID);

#if !ASSERT_DISABLED
    visitor.m_isCheckingForDefaultMarkViolation = wasCheckingForDefaultMarkViolation;
#endif
}

UString JSObject::className(const JSObject* object)
{
    const ClassInfo* info = object->classInfo();
    ASSERT(info);
    return info->className;
}

bool JSObject::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    return thisObject->methodTable()->getOwnPropertySlot(thisObject, exec, Identifier::from(exec, propertyName), slot);
}

// ECMA 8.6.2.2
void JSObject::put(JSCell* cell, ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(thisObject));
    JSGlobalData& globalData = exec->globalData();

    // Check if there are any setters or getters in the prototype chain
    JSValue prototype;
    if (propertyName != exec->propertyNames().underscoreProto) {
        for (JSObject* obj = thisObject; !obj->structure()->hasReadOnlyOrGetterSetterPropertiesExcludingProto(); obj = asObject(prototype)) {
            prototype = obj->prototype();
            if (prototype.isNull()) {
                if (!thisObject->putDirectInternal<PutModePut>(globalData, propertyName, value, 0, slot, getJSFunction(value)) && slot.isStrictMode())
                    throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
                return;
            }
        }
    }

    for (JSObject* obj = thisObject; ; obj = asObject(prototype)) {
        unsigned attributes;
        JSCell* specificValue;
        size_t offset = obj->structure()->get(globalData, propertyName, attributes, specificValue);
        if (offset != WTF::notFound) {
            if (attributes & ReadOnly) {
                if (slot.isStrictMode())
                    throwError(exec, createTypeError(exec, StrictModeReadonlyPropertyWriteError));
                return;
            }

            JSValue gs = obj->getDirectOffset(offset);
            if (gs.isGetterSetter()) {
                JSObject* setterFunc = asGetterSetter(gs)->setter();        
                if (!setterFunc) {
                    if (slot.isStrictMode())
                        throwError(exec, createTypeError(exec, "setting a property that has only a getter"));
                    return;
                }
                
                CallData callData;
                CallType callType = setterFunc->methodTable()->getCallData(setterFunc, callData);
                MarkedArgumentBuffer args;
                args.append(value);

                // If this is WebCore's global object then we need to substitute the shell.
                call(exec, setterFunc, callType, callData, thisObject->methodTable()->toThisObject(thisObject, exec), args);
                return;
            }

            // If there's an existing property on the object or one of its 
            // prototypes it should be replaced, so break here.
            break;
        }

        prototype = obj->prototype();
        if (prototype.isNull())
            break;
    }
    
    if (!thisObject->putDirectInternal<PutModePut>(globalData, propertyName, value, 0, slot, getJSFunction(value)) && slot.isStrictMode())
        throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
    return;
}

void JSObject::putByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
{
    PutPropertySlot slot(shouldThrow);
    JSObject* thisObject = jsCast<JSObject*>(cell);
    thisObject->methodTable()->put(thisObject, exec, Identifier::from(exec, propertyName), value, slot);
}

void JSObject::putDirectVirtual(JSObject* object, ExecState* exec, const Identifier& propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    PutPropertySlot slot;
    object->putDirectInternal<PutModeDefineOwnProperty>(exec->globalData(), propertyName, value, attributes, slot, getJSFunction(value));
}

bool JSObject::setPrototypeWithCycleCheck(JSGlobalData& globalData, JSValue prototype)
{
    JSValue checkFor = this;
    if (this->isGlobalObject())
        checkFor = jsCast<JSGlobalObject*>(this)->globalExec()->thisValue();

    JSValue nextPrototype = prototype;
    while (nextPrototype && nextPrototype.isObject()) {
        if (nextPrototype == checkFor)
            return false;
        nextPrototype = asObject(nextPrototype)->prototype();
    }
    setPrototype(globalData, prototype);
    return true;
}

bool JSObject::allowsAccessFrom(ExecState* exec)
{
    JSGlobalObject* globalObject = isGlobalThis() ? jsCast<JSGlobalThis*>(this)->unwrappedObject() : this->globalObject();
    return globalObject->globalObjectMethodTable()->allowsAccessFrom(globalObject, exec);
}

void JSObject::putDirectAccessor(JSGlobalData& globalData, const Identifier& propertyName, JSValue value, unsigned attributes)
{
    ASSERT(value.isGetterSetter() && (attributes & Accessor));

    PutPropertySlot slot;
    putDirectInternal<PutModeDefineOwnProperty>(globalData, propertyName, value, attributes, slot, getJSFunction(value));

    // putDirect will change our Structure if we add a new property. For
    // getters and setters, though, we also need to change our Structure
    // if we override an existing non-getter or non-setter.
    if (slot.type() != PutPropertySlot::NewProperty)
        setStructure(globalData, Structure::attributeChangeTransition(globalData, structure(), propertyName, attributes));

    if (attributes & ReadOnly)
        structure()->setContainsReadOnlyProperties();

    structure()->setHasGetterSetterProperties(propertyName == globalData.propertyNames->underscoreProto);
}

bool JSObject::hasProperty(ExecState* exec, const Identifier& propertyName) const
{
    PropertySlot slot;
    return const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot);
}

bool JSObject::hasProperty(ExecState* exec, unsigned propertyName) const
{
    PropertySlot slot;
    return const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot);
}

// ECMA 8.6.2.5
bool JSObject::deleteProperty(JSCell* cell, ExecState* exec, const Identifier& propertyName)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);

    if (!thisObject->staticFunctionsReified())
        thisObject->reifyStaticFunctionsForDelete(exec);

    unsigned attributes;
    JSCell* specificValue;
    if (thisObject->structure()->get(exec->globalData(), propertyName, attributes, specificValue) != WTF::notFound) {
        if (attributes & DontDelete && !exec->globalData().isInDefineOwnProperty())
            return false;
        thisObject->removeDirect(exec->globalData(), propertyName);
        return true;
    }

    // Look in the static hashtable of properties
    const HashEntry* entry = thisObject->findPropertyHashEntry(exec, propertyName);
    if (entry && entry->attributes() & DontDelete && !exec->globalData().isInDefineOwnProperty())
        return false; // this builtin property can't be deleted

    // FIXME: Should the code here actually do some deletion?
    return true;
}

bool JSObject::hasOwnProperty(ExecState* exec, const Identifier& propertyName) const
{
    PropertySlot slot;
    return const_cast<JSObject*>(this)->methodTable()->getOwnPropertySlot(const_cast<JSObject*>(this), exec, propertyName, slot);
}

bool JSObject::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    return thisObject->methodTable()->deleteProperty(thisObject, exec, Identifier::from(exec, propertyName));
}

static ALWAYS_INLINE JSValue callDefaultValueFunction(ExecState* exec, const JSObject* object, const Identifier& propertyName)
{
    JSValue function = object->get(exec, propertyName);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return exec->exception();

    // Prevent "toString" and "valueOf" from observing execution if an exception
    // is pending.
    if (exec->hadException())
        return exec->exception();

    JSValue result = call(exec, function, callType, callData, const_cast<JSObject*>(object), exec->emptyList());
    ASSERT(!result.isGetterSetter());
    if (exec->hadException())
        return exec->exception();
    if (result.isObject())
        return JSValue();
    return result;
}

bool JSObject::getPrimitiveNumber(ExecState* exec, double& number, JSValue& result) const
{
    result = methodTable()->defaultValue(this, exec, PreferNumber);
    number = result.toNumber(exec);
    return !result.isString();
}

// ECMA 8.6.2.6
JSValue JSObject::defaultValue(const JSObject* object, ExecState* exec, PreferredPrimitiveType hint)
{
    // Must call toString first for Date objects.
    if ((hint == PreferString) || (hint != PreferNumber && object->prototype() == exec->lexicalGlobalObject()->datePrototype())) {
        JSValue value = callDefaultValueFunction(exec, object, exec->propertyNames().toString);
        if (value)
            return value;
        value = callDefaultValueFunction(exec, object, exec->propertyNames().valueOf);
        if (value)
            return value;
    } else {
        JSValue value = callDefaultValueFunction(exec, object, exec->propertyNames().valueOf);
        if (value)
            return value;
        value = callDefaultValueFunction(exec, object, exec->propertyNames().toString);
        if (value)
            return value;
    }

    ASSERT(!exec->hadException());

    return throwError(exec, createTypeError(exec, "No default value"));
}

const HashEntry* JSObject::findPropertyHashEntry(ExecState* exec, const Identifier& propertyName) const
{
    for (const ClassInfo* info = classInfo(); info; info = info->parentClass) {
        if (const HashTable* propHashTable = info->propHashTable(exec)) {
            if (const HashEntry* entry = propHashTable->entry(exec, propertyName))
                return entry;
        }
    }
    return 0;
}

bool JSObject::hasInstance(JSObject*, ExecState* exec, JSValue value, JSValue proto)
{
    if (!value.isObject())
        return false;

    if (!proto.isObject()) {
        throwError(exec, createTypeError(exec, "instanceof called on an object with an invalid prototype property."));
        return false;
    }

    JSObject* object = asObject(value);
    while ((object = object->prototype().getObject())) {
        if (proto == object)
            return true;
    }
    return false;
}

bool JSObject::propertyIsEnumerable(ExecState* exec, const Identifier& propertyName) const
{
    PropertyDescriptor descriptor;
    if (!const_cast<JSObject*>(this)->methodTable()->getOwnPropertyDescriptor(const_cast<JSObject*>(this), exec, propertyName, descriptor))
        return false;
    return descriptor.enumerable();
}

bool JSObject::getPropertySpecificValue(ExecState* exec, const Identifier& propertyName, JSCell*& specificValue) const
{
    unsigned attributes;
    if (structure()->get(exec->globalData(), propertyName, attributes, specificValue) != WTF::notFound)
        return true;

    // This could be a function within the static table? - should probably
    // also look in the hash?  This currently should not be a problem, since
    // we've currently always call 'get' first, which should have populated
    // the normal storage.
    return false;
}

void JSObject::getPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    object->methodTable()->getOwnPropertyNames(object, exec, propertyNames, mode);

    if (object->prototype().isNull())
        return;

    JSObject* prototype = asObject(object->prototype());
    while(1) {
        if (prototype->structure()->typeInfo().overridesGetPropertyNames()) {
            prototype->methodTable()->getPropertyNames(prototype, exec, propertyNames, mode);
            break;
        }
        prototype->methodTable()->getOwnPropertyNames(prototype, exec, propertyNames, mode);
        JSValue nextProto = prototype->prototype();
        if (nextProto.isNull())
            break;
        prototype = asObject(nextProto);
    }
}

void JSObject::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    object->structure()->getPropertyNamesFromStructure(exec->globalData(), propertyNames, mode);
    if (!object->staticFunctionsReified())
        getClassPropertyNames(exec, object->classInfo(), propertyNames, mode);
}

bool JSObject::toBoolean(ExecState*) const
{
    return true;
}

double JSObject::toNumber(ExecState* exec) const
{
    JSValue primitive = toPrimitive(exec, PreferNumber);
    if (exec->hadException()) // should be picked up soon in Nodes.cpp
        return 0.0;
    return primitive.toNumber(exec);
}

JSString* JSObject::toString(ExecState* exec) const
{
    JSValue primitive = toPrimitive(exec, PreferString);
    if (exec->hadException())
        return jsEmptyString(exec);
    return primitive.toString(exec);
}

JSObject* JSObject::toThisObject(JSCell* cell, ExecState*)
{
    return jsCast<JSObject*>(cell);
}

JSObject* JSObject::unwrappedObject()
{
    if (isGlobalThis())
        return jsCast<JSGlobalThis*>(this)->unwrappedObject();
    return this;
}

void JSObject::seal(JSGlobalData& globalData)
{
    if (isSealed(globalData))
        return;
    preventExtensions(globalData);
    setStructure(globalData, Structure::sealTransition(globalData, structure()));
}

void JSObject::freeze(JSGlobalData& globalData)
{
    if (isFrozen(globalData))
        return;
    preventExtensions(globalData);
    setStructure(globalData, Structure::freezeTransition(globalData, structure()));
}

void JSObject::preventExtensions(JSGlobalData& globalData)
{
    if (isJSArray(this))
        asArray(this)->enterDictionaryMode(globalData);
    if (isExtensible())
        setStructure(globalData, Structure::preventExtensionsTransition(globalData, structure()));
}

// This presently will flatten to an uncachable dictionary; this is suitable
// for use in delete, we may want to do something different elsewhere.
void JSObject::reifyStaticFunctionsForDelete(ExecState* exec)
{
    ASSERT(!staticFunctionsReified());
    JSGlobalData& globalData = exec->globalData();

    // If this object's ClassInfo has no static properties, then nothing to reify!
    // We can safely set the flag to avoid the expensive check again in the future.
    if (!classInfo()->hasStaticProperties()) {
        structure()->setStaticFunctionsReified();
        return;
    }

    if (!structure()->isUncacheableDictionary())
        setStructure(globalData, Structure::toUncacheableDictionaryTransition(globalData, structure()));

    for (const ClassInfo* info = classInfo(); info; info = info->parentClass) {
        const HashTable* hashTable = info->propHashTable(globalObject()->globalExec());
        if (!hashTable)
            continue;
        PropertySlot slot;
        for (HashTable::ConstIterator iter = hashTable->begin(globalData); iter != hashTable->end(globalData); ++iter) {
            if (iter->attributes() & Function)
                setUpStaticFunctionSlot(globalObject()->globalExec(), *iter, this, Identifier(&globalData, iter->key()), slot);
        }
    }

    structure()->setStaticFunctionsReified();
}

void JSObject::removeDirect(JSGlobalData& globalData, const Identifier& propertyName)
{
    if (structure()->get(globalData, propertyName) == WTF::notFound)
        return;

    size_t offset;
    if (structure()->isUncacheableDictionary()) {
        offset = structure()->removePropertyWithoutTransition(globalData, propertyName);
        if (offset != WTF::notFound)
            putUndefinedAtDirectOffset(offset);
        return;
    }

    setStructure(globalData, Structure::removePropertyTransition(globalData, structure(), propertyName, offset));
    if (offset != WTF::notFound)
        putUndefinedAtDirectOffset(offset);
}

NEVER_INLINE void JSObject::fillGetterPropertySlot(PropertySlot& slot, WriteBarrierBase<Unknown>* location)
{
    if (JSObject* getterFunction = asGetterSetter(location->get())->getter()) {
        if (!structure()->isDictionary())
            slot.setCacheableGetterSlot(this, getterFunction, offsetForLocation(location));
        else
            slot.setGetterSlot(getterFunction);
    } else
        slot.setUndefined();
}

Structure* JSObject::createInheritorID(JSGlobalData& globalData)
{
    JSGlobalObject* globalObject;
    if (isGlobalThis())
        globalObject = static_cast<JSGlobalThis*>(this)->unwrappedObject();
    else
        globalObject = structure()->globalObject();
    ASSERT(globalObject);
    m_inheritorID.set(globalData, this, createEmptyObjectStructure(globalData, globalObject, this));
    ASSERT(m_inheritorID->isEmpty());
    return m_inheritorID.get();
}

PropertyStorage JSObject::growPropertyStorage(JSGlobalData& globalData, size_t oldSize, size_t newSize)
{
    ASSERT(newSize > oldSize);

    // It's important that this function not rely on structure(), since
    // we might be in the middle of a transition.

    PropertyStorage oldPropertyStorage = m_propertyStorage.get();
    PropertyStorage newPropertyStorage = 0;

    if (isUsingInlineStorage()) {
        // We have this extra temp here to slake GCC's thirst for the blood of those who dereference type-punned pointers.
        void* temp = newPropertyStorage;
        if (!globalData.heap.tryAllocateStorage(sizeof(WriteBarrierBase<Unknown>) * newSize, &temp))
            CRASH();
        newPropertyStorage = static_cast<PropertyStorage>(temp);

        for (unsigned i = 0; i < oldSize; ++i)
            newPropertyStorage[i] = oldPropertyStorage[i];
    } else {
        // We have this extra temp here to slake GCC's thirst for the blood of those who dereference type-punned pointers.
        void* temp = oldPropertyStorage;
        if (!globalData.heap.tryReallocateStorage(&temp, sizeof(WriteBarrierBase<Unknown>) * oldSize, sizeof(WriteBarrierBase<Unknown>) * newSize))
            CRASH();
        newPropertyStorage = static_cast<PropertyStorage>(temp);
    }

    ASSERT(newPropertyStorage);
    return newPropertyStorage;
}

bool JSObject::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    unsigned attributes = 0;
    JSCell* cell = 0;
    size_t offset = object->structure()->get(exec->globalData(), propertyName, attributes, cell);
    if (offset == WTF::notFound)
        return false;
    descriptor.setDescriptor(object->getDirectOffset(offset), attributes);
    return true;
}

bool JSObject::getPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    JSObject* object = this;
    while (true) {
        if (object->methodTable()->getOwnPropertyDescriptor(object, exec, propertyName, descriptor))
            return true;
        JSValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

static bool putDescriptor(ExecState* exec, JSObject* target, const Identifier& propertyName, PropertyDescriptor& descriptor, unsigned attributes, const PropertyDescriptor& oldDescriptor)
{
    if (descriptor.isGenericDescriptor() || descriptor.isDataDescriptor()) {
        if (descriptor.isGenericDescriptor() && oldDescriptor.isAccessorDescriptor()) {
            GetterSetter* accessor = GetterSetter::create(exec);
            if (oldDescriptor.getterPresent())
                accessor->setGetter(exec->globalData(), oldDescriptor.getterObject());
            if (oldDescriptor.setterPresent())
                accessor->setSetter(exec->globalData(), oldDescriptor.setterObject());
            target->putDirectAccessor(exec->globalData(), propertyName, accessor, attributes | Accessor);
            return true;
        }
        JSValue newValue = jsUndefined();
        if (descriptor.value())
            newValue = descriptor.value();
        else if (oldDescriptor.value())
            newValue = oldDescriptor.value();
        target->putDirect(exec->globalData(), propertyName, newValue, attributes & ~Accessor);
        if (attributes & ReadOnly)
            target->structure()->setContainsReadOnlyProperties();
        return true;
    }
    attributes &= ~ReadOnly;
    GetterSetter* accessor = GetterSetter::create(exec);

    if (descriptor.getterPresent())
        accessor->setGetter(exec->globalData(), descriptor.getterObject());
    else if (oldDescriptor.getterPresent())
        accessor->setGetter(exec->globalData(), oldDescriptor.getterObject());
    if (descriptor.setterPresent())
        accessor->setSetter(exec->globalData(), descriptor.setterObject());
    else if (oldDescriptor.setterPresent())
        accessor->setSetter(exec->globalData(), oldDescriptor.setterObject());

    target->putDirectAccessor(exec->globalData(), propertyName, accessor, attributes | Accessor);
    return true;
}

class DefineOwnPropertyScope {
public:
    DefineOwnPropertyScope(ExecState* exec)
        : m_globalData(exec->globalData())
    {
        m_globalData.setInDefineOwnProperty(true);
    }

    ~DefineOwnPropertyScope()
    {
        m_globalData.setInDefineOwnProperty(false);
    }

private:
    JSGlobalData& m_globalData;
};

bool JSObject::defineOwnProperty(JSObject* object, ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor, bool throwException)
{
    // Track on the globaldata that we're in define property.
    // Currently DefineOwnProperty uses delete to remove properties when they are being replaced
    // (particularly when changing attributes), however delete won't allow non-configurable (i.e.
    // DontDelete) properties to be deleted. For now, we can use this flag to make this work.
    DefineOwnPropertyScope scope(exec);

    // If we have a new property we can just put it on normally
    PropertyDescriptor current;
    if (!object->methodTable()->getOwnPropertyDescriptor(object, exec, propertyName, current)) {
        // unless extensions are prevented!
        if (!object->isExtensible()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to define property on object that is not extensible."));
            return false;
        }
        PropertyDescriptor oldDescriptor;
        oldDescriptor.setValue(jsUndefined());
        return putDescriptor(exec, object, propertyName, descriptor, descriptor.attributes(), oldDescriptor);
    }

    if (descriptor.isEmpty())
        return true;

    if (current.equalTo(exec, descriptor))
        return true;

    // Filter out invalid changes
    if (!current.configurable()) {
        if (descriptor.configurable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to configurable attribute of unconfigurable property."));
            return false;
        }
        if (descriptor.enumerablePresent() && descriptor.enumerable() != current.enumerable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change enumerable attribute of unconfigurable property."));
            return false;
        }
    }

    // A generic descriptor is simply changing the attributes of an existing property
    if (descriptor.isGenericDescriptor()) {
        if (!current.attributesEqual(descriptor)) {
            object->methodTable()->deleteProperty(object, exec, propertyName);
            return putDescriptor(exec, object, propertyName, descriptor, descriptor.attributesOverridingCurrent(current), current);
        }
        return true;
    }

    // Changing between a normal property or an accessor property
    if (descriptor.isDataDescriptor() != current.isDataDescriptor()) {
        if (!current.configurable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change access mechanism for an unconfigurable property."));
            return false;
        }
        object->methodTable()->deleteProperty(object, exec, propertyName);
        return putDescriptor(exec, object, propertyName, descriptor, descriptor.attributesOverridingCurrent(current), current);
    }

    // Changing the value and attributes of an existing property
    if (descriptor.isDataDescriptor()) {
        if (!current.configurable()) {
            if (!current.writable() && descriptor.writable()) {
                if (throwException)
                    throwError(exec, createTypeError(exec, "Attempting to change writable attribute of unconfigurable property."));
                return false;
            }
            if (!current.writable()) {
                if (descriptor.value() && !sameValue(exec, current.value(), descriptor.value())) {
                    if (throwException)
                        throwError(exec, createTypeError(exec, "Attempting to change value of a readonly property."));
                    return false;
                }
            }
        }
        if (current.attributesEqual(descriptor) && !descriptor.value())
            return true;
        object->methodTable()->deleteProperty(object, exec, propertyName);
        return putDescriptor(exec, object, propertyName, descriptor, descriptor.attributesOverridingCurrent(current), current);
    }

    // Changing the accessor functions of an existing accessor property
    ASSERT(descriptor.isAccessorDescriptor());
    if (!current.configurable()) {
        if (descriptor.setterPresent() && !(current.setterPresent() && JSValue::strictEqual(exec, current.setter(), descriptor.setter()))) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change the setter of an unconfigurable property."));
            return false;
        }
        if (descriptor.getterPresent() && !(current.getterPresent() && JSValue::strictEqual(exec, current.getter(), descriptor.getter()))) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change the getter of an unconfigurable property."));
            return false;
        }
    }
    JSValue accessor = object->getDirect(exec->globalData(), propertyName);
    if (!accessor)
        return false;
    GetterSetter* getterSetter = asGetterSetter(accessor);
    if (descriptor.setterPresent())
        getterSetter->setSetter(exec->globalData(), descriptor.setterObject());
    if (descriptor.getterPresent())
        getterSetter->setGetter(exec->globalData(), descriptor.getterObject());
    if (current.attributesEqual(descriptor))
        return true;
    object->methodTable()->deleteProperty(object, exec, propertyName);
    unsigned attrs = descriptor.attributesOverridingCurrent(current);
    object->putDirectAccessor(exec->globalData(), propertyName, getterSetter, attrs | Accessor);
    return true;
}

JSObject* throwTypeError(ExecState* exec, const UString& message)
{
    return throwError(exec, createTypeError(exec, message));
}

} // namespace JSC
