/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2013 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *  Copyright (C) 2012 Ericsson AB. All rights reserved.
 *  Copyright (C) 2013 Michael Pruett <michael@68k.org>
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
 */

#ifndef JSDOMBinding_h
#define JSDOMBinding_h

#include "JSDOMGlobalObject.h"
#include "JSDOMWrapper.h"
#include "DOMWrapperWorld.h"
#include "ScriptWrappable.h"
#include "ScriptWrappableInlines.h"
#include "WebCoreTypedArrayController.h"
#include <cstddef>
#include <heap/SlotVisitorInlines.h>
#include <heap/Weak.h>
#include <heap/WeakInlines.h>
#include <runtime/Error.h>
#include <runtime/JSArray.h>
#include <runtime/JSArrayBuffer.h>
#include <runtime/JSCJSValueInlines.h>
#include <runtime/JSCellInlines.h>
#include <runtime/JSTypedArrays.h>
#include <runtime/Lookup.h>
#include <runtime/StructureInlines.h>
#include <runtime/TypedArrayInlines.h>
#include <runtime/TypedArrays.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace JSC {
class HashEntry;
}

#if ENABLE(GAMEPAD)
namespace WTF {

template<typename T> inline T* getPtr(const Ref<T>& p)
{
    return const_cast<T*>(&p.get());
}

}
#endif

namespace WebCore {

class CachedScript;
class DOMStringList;
class DOMWindow;
class Frame;
class URL;
class Node;

typedef int ExceptionCode;

DOMWindow& activeDOMWindow(JSC::ExecState*);
DOMWindow& firstDOMWindow(JSC::ExecState*);

WEBCORE_EXPORT JSC::EncodedJSValue reportDeprecatedGetterError(JSC::ExecState&, const char* interfaceName, const char* attributeName);
WEBCORE_EXPORT void reportDeprecatedSetterError(JSC::ExecState&, const char* interfaceName, const char* attributeName);

void throwArrayElementTypeError(JSC::ExecState&);
void throwAttributeTypeError(JSC::ExecState&, const char* interfaceName, const char* attributeName, const char* expectedType);
WEBCORE_EXPORT void throwSequenceTypeError(JSC::ExecState&);
WEBCORE_EXPORT void throwSetterTypeError(JSC::ExecState&, const char* interfaceName, const char* attributeName);

JSC::EncodedJSValue throwArgumentMustBeEnumError(JSC::ExecState&, unsigned argumentIndex, const char* argumentName, const char* functionInterfaceName, const char* functionName, const char* expectedValues);
JSC::EncodedJSValue throwArgumentMustBeFunctionError(JSC::ExecState&, unsigned argumentIndex, const char* argumentName, const char* functionInterfaceName, const char* functionName);
JSC::EncodedJSValue throwArgumentTypeError(JSC::ExecState&, unsigned argumentIndex, const char* argumentName, const char* functionInterfaceName, const char* functionName, const char* expectedType);
JSC::EncodedJSValue throwConstructorDocumentUnavailableError(JSC::ExecState&, const char* interfaceName);
WEBCORE_EXPORT JSC::EncodedJSValue throwGetterTypeError(JSC::ExecState&, const char* interfaceName, const char* attributeName);
WEBCORE_EXPORT JSC::EncodedJSValue throwThisTypeError(JSC::ExecState&, const char* interfaceName, const char* functionName);

// Base class for all constructor objects in the JSC bindings.
class DOMConstructorObject : public JSDOMWrapper {
    typedef JSDOMWrapper Base;
public:
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

protected:
    static const unsigned StructureFlags = JSC::ImplementsHasInstance | JSDOMWrapper::StructureFlags;
    DOMConstructorObject(JSC::Structure* structure, JSDOMGlobalObject* globalObject)
        : JSDOMWrapper(structure, globalObject)
    {
    }
};

WEBCORE_EXPORT JSC::Structure* getCachedDOMStructure(JSDOMGlobalObject*, const JSC::ClassInfo*);
WEBCORE_EXPORT JSC::Structure* cacheDOMStructure(JSDOMGlobalObject*, JSC::Structure*, const JSC::ClassInfo*);

inline JSDOMGlobalObject* deprecatedGlobalObjectForPrototype(JSC::ExecState* exec)
{
    // FIXME: Callers to this function should be using the global object
    // from which the object is being created, instead of assuming the lexical one.
    // e.g. subframe.document.body should use the subframe's global object, not the lexical one.
    return JSC::jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
}

template<typename WrapperClass> inline JSC::Structure* getDOMStructure(JSC::VM& vm, JSDOMGlobalObject* globalObject)
{
    if (JSC::Structure* structure = getCachedDOMStructure(globalObject, WrapperClass::info()))
        return structure;
    return cacheDOMStructure(globalObject, WrapperClass::createStructure(vm, globalObject, WrapperClass::createPrototype(vm, globalObject)), WrapperClass::info());
}

template<typename WrapperClass> inline JSC::Structure* deprecatedGetDOMStructure(JSC::ExecState* exec)
{
    // FIXME: This function is wrong. It uses the wrong global object for creating the prototype structure.
    return getDOMStructure<WrapperClass>(exec->vm(), deprecatedGlobalObjectForPrototype(exec));
}

template<typename WrapperClass> inline JSC::JSObject* getDOMPrototype(JSC::VM& vm, JSC::JSGlobalObject* globalObject)
{
    return JSC::jsCast<JSC::JSObject*>(asObject(getDOMStructure<WrapperClass>(vm, JSC::jsCast<JSDOMGlobalObject*>(globalObject))->storedPrototype()));
}

inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld& world, JSC::ArrayBuffer*)
{
    return static_cast<WebCoreTypedArrayController*>(world.vm().m_typedArrayController.get())->wrapperOwner();
}

inline void* wrapperContext(DOMWrapperWorld& world, JSC::ArrayBuffer*)
{
    return &world;
}

inline JSDOMWrapper* getInlineCachedWrapper(DOMWrapperWorld&, void*) { return nullptr; }
inline bool setInlineCachedWrapper(DOMWrapperWorld&, void*, JSDOMWrapper*, JSC::WeakHandleOwner*, void*) { return false; }
inline bool clearInlineCachedWrapper(DOMWrapperWorld&, void*, JSDOMWrapper*) { return false; }

inline JSDOMWrapper* getInlineCachedWrapper(DOMWrapperWorld& world, ScriptWrappable* domObject)
{
    if (!world.isNormal())
        return nullptr;
    return domObject->wrapper();
}

inline JSC::JSArrayBuffer* getInlineCachedWrapper(DOMWrapperWorld& world, JSC::ArrayBuffer* buffer)
{
    if (!world.isNormal())
        return nullptr;
    return buffer->m_wrapper.get();
}

inline bool setInlineCachedWrapper(DOMWrapperWorld& world, ScriptWrappable* domObject, JSDOMWrapper* wrapper, JSC::WeakHandleOwner* wrapperOwner, void* context)
{
    if (!world.isNormal())
        return false;
    domObject->setWrapper(wrapper, wrapperOwner, context);
    return true;
}

inline bool setInlineCachedWrapper(DOMWrapperWorld& world, JSC::ArrayBuffer* domObject, JSC::JSArrayBuffer* wrapper, JSC::WeakHandleOwner* wrapperOwner, void* context)
{
    if (!world.isNormal())
        return false;
    domObject->m_wrapper = JSC::Weak<JSC::JSArrayBuffer>(wrapper, wrapperOwner, context);
    return true;
}

inline bool clearInlineCachedWrapper(DOMWrapperWorld& world, ScriptWrappable* domObject, JSDOMWrapper* wrapper)
{
    if (!world.isNormal())
        return false;
    domObject->clearWrapper(wrapper);
    return true;
}

inline bool clearInlineCachedWrapper(DOMWrapperWorld& world, JSC::ArrayBuffer* domObject, JSC::JSArrayBuffer* wrapper)
{
    if (!world.isNormal())
        return false;
    weakClear(domObject->m_wrapper, wrapper);
    return true;
}

template<typename DOMClass> inline JSC::JSObject* getCachedWrapper(DOMWrapperWorld& world, DOMClass* domObject)
{
    if (JSC::JSObject* wrapper = getInlineCachedWrapper(world, domObject))
        return wrapper;
    return world.m_wrappers.get(domObject);
}

template<typename DOMClass, typename WrapperClass> inline void cacheWrapper(DOMWrapperWorld& world, DOMClass* domObject, WrapperClass* wrapper)
{
    JSC::WeakHandleOwner* owner = wrapperOwner(world, domObject);
    void* context = wrapperContext(world, domObject);
    if (setInlineCachedWrapper(world, domObject, wrapper, owner, context))
        return;
    weakAdd(world.m_wrappers, (void*)domObject, JSC::Weak<JSC::JSObject>(wrapper, owner, context));
}

template<typename DOMClass, typename WrapperClass> inline void uncacheWrapper(DOMWrapperWorld& world, DOMClass* domObject, WrapperClass* wrapper)
{
    if (clearInlineCachedWrapper(world, domObject, wrapper))
        return;
    weakRemove(world.m_wrappers, (void*)domObject, wrapper);
}

#define CREATE_DOM_WRAPPER(globalObject, className, object) createWrapper<JS##className>(globalObject, static_cast<className*>(object))
template<typename WrapperClass, typename DOMClass> inline JSDOMWrapper* createWrapper(JSDOMGlobalObject* globalObject, DOMClass* node)
{
    ASSERT(node);
    ASSERT(!getCachedWrapper(globalObject->world(), node));
    WrapperClass* wrapper = WrapperClass::create(getDOMStructure<WrapperClass>(globalObject->vm(), globalObject), globalObject, node);
    cacheWrapper(globalObject->world(), node, wrapper);
    return wrapper;
}

template<typename WrapperClass, typename DOMClass> inline JSC::JSValue wrap(JSDOMGlobalObject* globalObject, DOMClass* domObject)
{
    if (!domObject)
        return JSC::jsNull();
    if (JSC::JSObject* wrapper = getCachedWrapper(globalObject->world(), domObject))
        return wrapper;
    return createWrapper<WrapperClass>(globalObject, domObject);
}

template<typename WrapperClass, typename DOMClass> inline JSC::JSValue getExistingWrapper(JSDOMGlobalObject* globalObject, DOMClass* domObject)
{
    ASSERT(domObject);
    return getCachedWrapper(globalObject->world(), domObject);
}

template<typename WrapperClass, typename DOMClass> inline JSC::JSValue createNewWrapper(JSDOMGlobalObject* globalObject, DOMClass* domObject)
{
    ASSERT(domObject);
    ASSERT(!getCachedWrapper(globalObject->world(), domObject));
    return createWrapper<WrapperClass>(globalObject, domObject);
}

inline JSC::JSValue argumentOrNull(JSC::ExecState* exec, unsigned index)
{
    return index >= exec->argumentCount() ? JSC::JSValue() : exec->argument(index);
}

void addImpureProperty(const AtomicString&);

const JSC::HashTable& getHashTableForGlobalData(JSC::VM&, const JSC::HashTable& staticTable);

WEBCORE_EXPORT void reportException(JSC::ExecState*, JSC::JSValue exception, CachedScript* = nullptr);
void reportCurrentException(JSC::ExecState*);

// Convert a DOM implementation exception code into a JavaScript exception in the execution state.
WEBCORE_EXPORT void setDOMException(JSC::ExecState*, ExceptionCode);

JSC::JSValue jsString(JSC::ExecState*, const URL&); // empty if the URL is null

JSC::JSValue jsStringOrNull(JSC::ExecState*, const String&); // null if the string is null
JSC::JSValue jsStringOrNull(JSC::ExecState*, const URL&); // null if the URL is null

JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const String&); // undefined if the string is null
JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const URL&); // undefined if the URL is null

// See JavaScriptCore for explanation: Should be used for any string that is already owned by another
// object, to let the engine know that collecting the JSString wrapper is unlikely to save memory.
JSC::JSValue jsOwnedStringOrNull(JSC::ExecState*, const String&);

String propertyNameToString(JSC::PropertyName);

AtomicString propertyNameToAtomicString(JSC::PropertyName);

String valueToStringWithNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null
String valueToStringWithUndefinedOrNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null or undefined

inline int32_t finiteInt32Value(JSC::JSValue value, JSC::ExecState* exec, bool& okay)
{
    double number = value.toNumber(exec);
    okay = std::isfinite(number);
    return JSC::toInt32(number);
}

enum IntegerConversionConfiguration {
    NormalConversion,
    EnforceRange,
    // FIXME: Implement Clamp
};

WEBCORE_EXPORT int32_t toInt32EnforceRange(JSC::ExecState*, JSC::JSValue);
WEBCORE_EXPORT uint32_t toUInt32EnforceRange(JSC::ExecState*, JSC::JSValue);

WEBCORE_EXPORT int8_t toInt8(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);
WEBCORE_EXPORT uint8_t toUInt8(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);

WEBCORE_EXPORT int16_t toInt16(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);
WEBCORE_EXPORT uint16_t toUInt16(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);

/*
    Convert a value to an integer as per <http://www.w3.org/TR/WebIDL/>.
    The conversion fails if the value cannot be converted to a number or,
    if EnforceRange is specified, the value is outside the range of the
    destination integer type.
*/
inline int32_t toInt32(JSC::ExecState* exec, JSC::JSValue value, IntegerConversionConfiguration configuration)
{
    if (configuration == EnforceRange)
        return toInt32EnforceRange(exec, value);
    return value.toInt32(exec);
}

inline uint32_t toUInt32(JSC::ExecState* exec, JSC::JSValue value, IntegerConversionConfiguration configuration)
{
    if (configuration == EnforceRange)
        return toUInt32EnforceRange(exec, value);
    return value.toUInt32(exec);
}

WEBCORE_EXPORT int64_t toInt64(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);
WEBCORE_EXPORT uint64_t toUInt64(JSC::ExecState*, JSC::JSValue, IntegerConversionConfiguration);

// Returns a Date instance for the specified value, or null if the value is NaN or infinity.
JSC::JSValue jsDateOrNull(JSC::ExecState*, double);
// NaN if the value can't be converted to a date.
double valueToDate(JSC::ExecState*, JSC::JSValue);

// Validates that the passed object is a sequence type per section 4.1.13 of the WebIDL spec.
inline JSC::JSObject* toJSSequence(JSC::ExecState* exec, JSC::JSValue value, unsigned& length)
{
    JSC::JSObject* object = value.getObject();
    if (!object) {
        throwSequenceTypeError(*exec);
        return nullptr;
    }

    JSC::JSValue lengthValue = object->get(exec, exec->propertyNames().length);
    if (exec->hadException())
        return nullptr;

    if (lengthValue.isUndefinedOrNull()) {
        throwSequenceTypeError(*exec);
        return nullptr;
    }

    length = lengthValue.toUInt32(exec);
    if (exec->hadException())
        return nullptr;

    return object;
}

inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, JSC::ArrayBuffer* buffer)
{
    if (!buffer)
        return JSC::jsNull();
    if (JSC::JSValue result = getExistingWrapper<JSC::JSArrayBuffer>(globalObject, buffer))
        return result;
    buffer->ref();
    JSC::JSArrayBuffer* wrapper = JSC::JSArrayBuffer::create(exec->vm(), globalObject->arrayBufferStructure(), buffer);
    cacheWrapper(globalObject->world(), buffer, wrapper);
    return wrapper;
}

inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, JSC::ArrayBufferView* view)
{
    if (!view)
        return JSC::jsNull();
    return view->wrap(exec, globalObject);
}

template<typename T> inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<T> ptr)
{
    return toJS(exec, globalObject, ptr.get());
}

template<typename T> inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const Vector<T>& vector)
{
    JSC::JSArray* array = constructEmptyArray(exec, nullptr, vector.size());
    for (size_t i = 0; i < vector.size(); ++i)
        array->putDirectIndex(exec, i, toJS(exec, globalObject, vector[i]));
    return array;
}

template<typename T> inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const Vector<RefPtr<T>>& vector)
{
    JSC::JSArray* array = constructEmptyArray(exec, nullptr, vector.size());
    for (size_t i = 0; i < vector.size(); ++i)
        array->putDirectIndex(exec, i, toJS(exec, globalObject, vector[i].get()));
    return array;
}

inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject*, const String& value)
{
    return jsStringOrNull(exec, value);
}

template<typename T> struct JSValueTraits {
    static JSC::JSValue arrayJSValue(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const T& value)
    {
        return toJS(exec, globalObject, WTF::getPtr(value));
    }
};

template<> struct JSValueTraits<String> {
    static JSC::JSValue arrayJSValue(JSC::ExecState* exec, JSDOMGlobalObject*, const String& value)
    {
        return JSC::jsStringWithCache(exec, value);
    }
};

template<> struct JSValueTraits<double> {
    static JSC::JSValue arrayJSValue(JSC::ExecState*, JSDOMGlobalObject*, const double& value)
    {
        return JSC::jsNumber(value);
    }
};

template<> struct JSValueTraits<float> {
    static JSC::JSValue arrayJSValue(JSC::ExecState*, JSDOMGlobalObject*, const float& value)
    {
        return JSC::jsNumber(value);
    }
};

template<> struct JSValueTraits<unsigned long> {
    static JSC::JSValue arrayJSValue(JSC::ExecState*, JSDOMGlobalObject*, const unsigned long& value)
    {
        return JSC::jsNumber(value);
    }
};

template<typename T, size_t inlineCapacity> JSC::JSValue jsArray(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const Vector<T, inlineCapacity>& vector)
{
    JSC::MarkedArgumentBuffer list;
    for (auto& element : vector)
        list.append(JSValueTraits<T>::arrayJSValue(exec, globalObject, element));
    return JSC::constructArray(exec, nullptr, globalObject, list);
}

WEBCORE_EXPORT JSC::JSValue jsArray(JSC::ExecState*, JSDOMGlobalObject*, PassRefPtr<DOMStringList>);

inline PassRefPtr<JSC::ArrayBufferView> toArrayBufferView(JSC::JSValue value)
{
    JSC::JSArrayBufferView* wrapper = JSC::jsDynamicCast<JSC::JSArrayBufferView*>(value);
    if (!wrapper)
        return nullptr;
    return wrapper->impl();
}

inline PassRefPtr<JSC::Int8Array> toInt8Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Int8Adaptor>(value); }
inline PassRefPtr<JSC::Int16Array> toInt16Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Int16Adaptor>(value); }
inline PassRefPtr<JSC::Int32Array> toInt32Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Int32Adaptor>(value); }
inline PassRefPtr<JSC::Uint8Array> toUint8Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Uint8Adaptor>(value); }
inline PassRefPtr<JSC::Uint8ClampedArray> toUint8ClampedArray(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Uint8ClampedAdaptor>(value); }
inline PassRefPtr<JSC::Uint16Array> toUint16Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Uint16Adaptor>(value); }
inline PassRefPtr<JSC::Uint32Array> toUint32Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Uint32Adaptor>(value); }
inline PassRefPtr<JSC::Float32Array> toFloat32Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Float32Adaptor>(value); }
inline PassRefPtr<JSC::Float64Array> toFloat64Array(JSC::JSValue value) { return JSC::toNativeTypedView<JSC::Float64Adaptor>(value); }

template<typename T> struct NativeValueTraits;

template<> struct NativeValueTraits<String> {
    static inline bool nativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, String& indexedValue)
    {
        indexedValue = jsValue.toString(exec)->value(exec);
        return true;
    }
};

template<> struct NativeValueTraits<unsigned> {
    static inline bool nativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, unsigned& indexedValue)
    {
        if (!jsValue.isNumber())
            return false;

        indexedValue = jsValue.toUInt32(exec);
        if (exec->hadException())
            return false;

        return true;
    }
};

template<> struct NativeValueTraits<float> {
    static inline bool nativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, float& indexedValue)
    {
        indexedValue = jsValue.toFloat(exec);
        return !exec->hadException();
    }
};

template<typename T, typename JST> Vector<RefPtr<T>> toRefPtrNativeArray(JSC::ExecState* exec, JSC::JSValue value, T* (*toT)(JSC::JSValue value))
{
    if (!isJSArray(value))
        return Vector<RefPtr<T>>();

    Vector<RefPtr<T>> result;
    JSC::JSArray* array = asArray(value);
    size_t size = array->length();
    result.reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i) {
        JSC::JSValue element = array->getIndex(exec, i);
        if (element.inherits(JST::info()))
            result.uncheckedAppend((*toT)(element));
        else {
            throwArrayElementTypeError(*exec);
            return Vector<RefPtr<T>>();
        }
    }
    return result;
}

template<typename T> Vector<T> toNativeArray(JSC::ExecState* exec, JSC::JSValue value)
{
    JSC::JSObject* object = value.getObject();
    if (!object)
        return Vector<T>();

    unsigned length = 0;
    if (isJSArray(value)) {
        JSC::JSArray* array = asArray(value);
        length = array->length();
    } else
        toJSSequence(exec, value, length);

    Vector<T> result;
    result.reserveInitialCapacity(length);
    typedef NativeValueTraits<T> TraitsType;

    for (unsigned i = 0; i < length; ++i) {
        T indexValue;
        if (!TraitsType::nativeValue(exec, object->get(exec, i), indexValue))
            return Vector<T>();
        result.uncheckedAppend(indexValue);
    }
    return result;
}

template<typename T> Vector<T> toNativeArguments(JSC::ExecState* exec, size_t startIndex = 0)
{
    size_t length = exec->argumentCount();
    ASSERT(startIndex <= length);

    Vector<T> result;
    result.reserveInitialCapacity(length - startIndex);
    typedef NativeValueTraits<T> TraitsType;

    for (size_t i = startIndex; i < length; ++i) {
        T indexValue;
        if (!TraitsType::nativeValue(exec, exec->argument(i), indexValue))
            return Vector<T>();
        result.uncheckedAppend(indexValue);
    }
    return result;
}

bool shouldAllowAccessToNode(JSC::ExecState*, Node*);
bool shouldAllowAccessToFrame(JSC::ExecState*, Frame*);
bool shouldAllowAccessToFrame(JSC::ExecState*, Frame*, String& message);
bool shouldAllowAccessToDOMWindow(JSC::ExecState*, DOMWindow&, String& message);

void printErrorMessageForFrame(Frame*, const String& message);
JSC::EncodedJSValue objectToStringFunctionGetter(JSC::ExecState*, JSC::JSObject*, JSC::EncodedJSValue, JSC::PropertyName);

inline String propertyNameToString(JSC::PropertyName propertyName)
{
    return propertyName.publicName();
}

inline AtomicString propertyNameToAtomicString(JSC::PropertyName propertyName)
{
    return AtomicString(propertyName.publicName());
}

template<typename DOMClass> inline const JSC::HashTableValue* getStaticValueSlotEntryWithoutCaching(JSC::ExecState* exec, JSC::PropertyName propertyName)
{
    const JSC::HashTable* table = DOMClass::info()->staticPropHashTable;
    if (!table)
        return getStaticValueSlotEntryWithoutCaching<typename DOMClass::Base>(exec, propertyName);
    const JSC::HashTableValue* entry = table->entry(propertyName);
    if (!entry) // not found, forward to parent
        return getStaticValueSlotEntryWithoutCaching<typename DOMClass::Base>(exec, propertyName);
    return entry;
}

template<> inline const JSC::HashTableValue* getStaticValueSlotEntryWithoutCaching<JSDOMWrapper>(JSC::ExecState*, JSC::PropertyName)
{
    return nullptr;
}

template<JSC::NativeFunction nativeFunction, int length>
JSC::EncodedJSValue nonCachingStaticFunctionGetter(JSC::ExecState* exec, JSC::JSObject*, JSC::EncodedJSValue, JSC::PropertyName propertyName)
{
    return JSC::JSValue::encode(JSC::JSFunction::create(exec->vm(), exec->lexicalGlobalObject(), length, propertyName.publicName(), nativeFunction));
}

enum SecurityReportingOption {
    DoNotReportSecurityError,
    ReportSecurityError,
};

class BindingSecurity {
public:
    static bool shouldAllowAccessToNode(JSC::ExecState*, Node*);
    static bool shouldAllowAccessToDOMWindow(JSC::ExecState*, DOMWindow&, SecurityReportingOption = ReportSecurityError);
    static bool shouldAllowAccessToFrame(JSC::ExecState*, Frame*, SecurityReportingOption = ReportSecurityError);
};
    
} // namespace WebCore

#endif // JSDOMBinding_h
