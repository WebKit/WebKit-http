/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2009, 2011, 2013 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "ArrayPrototype.h"

#include "ButterflyInlines.h"
#include "CachedCall.h"
#include "CodeBlock.h"
#include "CopiedSpaceInlines.h"
#include "Error.h"
#include "Interpreter.h"
#include "JIT.h"
#include "JSArrayIterator.h"
#include "JSStringBuilder.h"
#include "JSStringJoiner.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "JSCInlines.h"
#include "StringRecursionChecker.h"
#include <algorithm>
#include <wtf/Assertions.h>
#include <wtf/HashSet.h>

namespace JSC {

EncodedJSValue JSC_HOST_CALL arrayProtoFuncToString(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncToLocaleString(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncConcat(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncJoin(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncPop(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncPush(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncReverse(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncShift(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncSlice(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncSort(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncSplice(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncUnShift(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncIndexOf(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncReduce(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncReduceRight(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncLastIndexOf(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncValues(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncKeys(ExecState*);
EncodedJSValue JSC_HOST_CALL arrayProtoFuncEntries(ExecState*);

}

#include "ArrayPrototype.lut.h"

namespace JSC {

static inline bool isNumericCompareFunction(ExecState* exec, JSValue function, CallType callType, const CallData& callData)
{
    if (callType != CallTypeJS)
        return false;

    FunctionExecutable* executable = callData.js.functionExecutable;
    JSScope* scope = callData.js.scope;

    JSObject* error = executable->prepareForExecution(exec, jsCast<JSFunction*>(function), &scope, CodeForCall);
    if (error)
        return false;

    return executable->codeBlockForCall()->isNumericCompareFunction();
}

// ------------------------------ ArrayPrototype ----------------------------

const ClassInfo ArrayPrototype::s_info = {"Array", &JSArray::s_info, &arrayPrototypeTable, CREATE_METHOD_TABLE(ArrayPrototype)};

/* Source for ArrayPrototype.lut.h
@begin arrayPrototypeTable 16
  toString       arrayProtoFuncToString       DontEnum|Function 0
  toLocaleString arrayProtoFuncToLocaleString DontEnum|Function 0
  concat         arrayProtoFuncConcat         DontEnum|Function 1
  fill           arrayProtoFuncFill           DontEnum|Function 1
  join           arrayProtoFuncJoin           DontEnum|Function 1
  pop            arrayProtoFuncPop            DontEnum|Function 0
  push           arrayProtoFuncPush           DontEnum|Function 1
  reverse        arrayProtoFuncReverse        DontEnum|Function 0
  shift          arrayProtoFuncShift          DontEnum|Function 0
  slice          arrayProtoFuncSlice          DontEnum|Function 2
  sort           arrayProtoFuncSort           DontEnum|Function 1
  splice         arrayProtoFuncSplice         DontEnum|Function 2
  unshift        arrayProtoFuncUnShift        DontEnum|Function 1
  every          arrayProtoFuncEvery          DontEnum|Function 1
  forEach        arrayProtoFuncForEach        DontEnum|Function 1
  some           arrayProtoFuncSome           DontEnum|Function 1
  indexOf        arrayProtoFuncIndexOf        DontEnum|Function 1
  lastIndexOf    arrayProtoFuncLastIndexOf    DontEnum|Function 1
  filter         arrayProtoFuncFilter         DontEnum|Function 1
  reduce         arrayProtoFuncReduce         DontEnum|Function 1
  reduceRight    arrayProtoFuncReduceRight    DontEnum|Function 1
  map            arrayProtoFuncMap            DontEnum|Function 1
  entries        arrayProtoFuncEntries        DontEnum|Function 0
  keys           arrayProtoFuncKeys           DontEnum|Function 0
  find           arrayProtoFuncFind           DontEnum|Function 1
  findIndex      arrayProtoFuncFindIndex      DontEnum|Function 1
@end
*/

ArrayPrototype* ArrayPrototype::create(VM& vm, JSGlobalObject* globalObject, Structure* structure)
{
    ArrayPrototype* prototype = new (NotNull, allocateCell<ArrayPrototype>(vm.heap)) ArrayPrototype(vm, structure);
    prototype->finishCreation(vm, globalObject);
    return prototype;
}

// ECMA 15.4.4
ArrayPrototype::ArrayPrototype(VM& vm, Structure* structure)
    : JSArray(vm, structure, 0)
{
}

void ArrayPrototype::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    vm.prototypeMap.addPrototype(this);
    JSC_NATIVE_FUNCTION(vm.propertyNames->iteratorPrivateName, arrayProtoFuncValues, DontEnum, 0);
}

bool ArrayPrototype::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSArray>(exec, arrayPrototypeTable, jsCast<ArrayPrototype*>(object), propertyName, slot);
}

// ------------------------------ Array Functions ----------------------------

// Helper function
static ALWAYS_INLINE JSValue getProperty(ExecState* exec, JSObject* obj, unsigned index)
{
    PropertySlot slot(obj);
    if (!obj->getPropertySlot(exec, index, slot))
        return JSValue();
    return slot.getValue(exec, index);
}

static ALWAYS_INLINE unsigned getLength(ExecState* exec, JSObject* obj)
{
    if (isJSArray(obj))
        return jsCast<JSArray*>(obj)->length();
    return obj->get(exec, exec->propertyNames().length).toUInt32(exec);
}

static void putLength(ExecState* exec, JSObject* obj, JSValue value)
{
    PutPropertySlot slot(obj);
    obj->methodTable()->put(obj, exec, exec->propertyNames().length, value, slot);
}

static unsigned argumentClampedIndexFromStartOrEnd(ExecState* exec, int argument, unsigned length, unsigned undefinedValue = 0)
{
    JSValue value = exec->argument(argument);
    if (value.isUndefined())
        return undefinedValue;

    double indexDouble = value.toInteger(exec);
    if (indexDouble < 0) {
        indexDouble += length;
        return indexDouble < 0 ? 0 : static_cast<unsigned>(indexDouble);
    }
    return indexDouble > length ? length : static_cast<unsigned>(indexDouble);
}


// The shift/unshift function implement the shift/unshift behaviour required
// by the corresponding array prototype methods, and by splice. In both cases,
// the methods are operating an an array or array like object.
//
//  header  currentCount  (remainder)
// [------][------------][-----------]
//  header  resultCount  (remainder)
// [------][-----------][-----------]
//
// The set of properties in the range 'header' must be unchanged. The set of
// properties in the range 'remainder' (where remainder = length - header -
// currentCount) will be shifted to the left or right as appropriate; in the
// case of shift this must be removing values, in the case of unshift this
// must be introducing new values.
template<JSArray::ShiftCountMode shiftCountMode>
void shift(ExecState* exec, JSObject* thisObj, unsigned header, unsigned currentCount, unsigned resultCount, unsigned length)
{
    RELEASE_ASSERT(currentCount > resultCount);
    unsigned count = currentCount - resultCount;

    RELEASE_ASSERT(header <= length);
    RELEASE_ASSERT(currentCount <= (length - header));

    if (isJSArray(thisObj)) {
        JSArray* array = asArray(thisObj);
        if (array->length() == length && asArray(thisObj)->shiftCount<shiftCountMode>(exec, header, count))
            return;
    }

    for (unsigned k = header; k < length - currentCount; ++k) {
        unsigned from = k + currentCount;
        unsigned to = k + resultCount;
        PropertySlot slot(thisObj);
        if (thisObj->getPropertySlot(exec, from, slot)) {
            JSValue value = slot.getValue(exec, from);
            if (exec->hadException())
                return;
            thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, to, value, true);
            if (exec->hadException())
                return;
        } else if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, to)) {
            throwTypeError(exec, ASCIILiteral("Unable to delete property."));
            return;
        }
    }
    for (unsigned k = length; k > length - count; --k) {
        if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, k - 1)) {
            throwTypeError(exec, ASCIILiteral("Unable to delete property."));
            return;
        }
    }
}
template<JSArray::ShiftCountMode shiftCountMode>
void unshift(ExecState* exec, JSObject* thisObj, unsigned header, unsigned currentCount, unsigned resultCount, unsigned length)
{
    RELEASE_ASSERT(resultCount > currentCount);
    unsigned count = resultCount - currentCount;

    RELEASE_ASSERT(header <= length);
    RELEASE_ASSERT(currentCount <= (length - header));

    // Guard against overflow.
    if (count > (UINT_MAX - length)) {
        throwOutOfMemoryError(exec);
        return;
    }

    if (isJSArray(thisObj)) {
        JSArray* array = asArray(thisObj);
        if (array->length() == length && array->unshiftCount<shiftCountMode>(exec, header, count))
            return;
    }
    
    for (unsigned k = length - currentCount; k > header; --k) {
        unsigned from = k + currentCount - 1;
        unsigned to = k + resultCount - 1;
        PropertySlot slot(thisObj);
        if (thisObj->getPropertySlot(exec, from, slot)) {
            JSValue value = slot.getValue(exec, from);
            if (exec->hadException())
                return;
            thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, to, value, true);
        } else if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, to)) {
            throwTypeError(exec, ASCIILiteral("Unable to delete property."));
            return;
        }
        if (exec->hadException())
            return;
    }
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);

    // 1. Let array be the result of calling ToObject on the this value.
    JSObject* thisObject = thisValue.toObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());
    
    // 2. Let func be the result of calling the [[Get]] internal method of array with argument "join".
    JSValue function = JSValue(thisObject).get(exec, exec->propertyNames().join);

    // 3. If IsCallable(func) is false, then let func be the standard built-in method Object.prototype.toString (15.2.4.2).
    if (!function.isCell())
        return JSValue::encode(jsMakeNontrivialString(exec, "[object ", thisObject->methodTable(exec->vm())->className(thisObject), "]"));
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return JSValue::encode(jsMakeNontrivialString(exec, "[object ", thisObject->methodTable(exec->vm())->className(thisObject), "]"));

    // 4. Return the result of calling the [[Call]] internal method of func providing array as the this value and an empty arguments list.
    if (!isJSArray(thisObject) || callType != CallTypeHost || callData.native.function != arrayProtoFuncJoin)
        return JSValue::encode(call(exec, function, callType, callData, thisObject, exec->emptyList()));

    ASSERT(isJSArray(thisValue));
    JSArray* thisObj = asArray(thisValue);
    
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    StringRecursionChecker checker(exec, thisObj);
    if (JSValue earlyReturnValue = checker.earlyReturnValue())
        return JSValue::encode(earlyReturnValue);

    String separator(",", String::ConstructFromLiteral);
    JSStringJoiner stringJoiner(separator, length);
    for (unsigned k = 0; k < length; k++) {
        JSValue element;
        if (thisObj->canGetIndexQuickly(k))
            element = thisObj->getIndexQuickly(k);
        else {
            element = thisObj->get(exec, k);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
        }

        if (element.isUndefinedOrNull())
            stringJoiner.append(String());
        else
            stringJoiner.append(element.toWTFString(exec));

        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }
    return JSValue::encode(stringJoiner.join(exec));
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncToLocaleString(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);

    JSObject* thisObj = thisValue.toObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    StringRecursionChecker checker(exec, thisObj);
    if (JSValue earlyReturnValue = checker.earlyReturnValue())
        return JSValue::encode(earlyReturnValue);

    String separator(",", String::ConstructFromLiteral);
    JSStringJoiner stringJoiner(separator, length);
    for (unsigned k = 0; k < length; k++) {
        JSValue element = thisObj->get(exec, k);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!element.isUndefinedOrNull()) {
            JSObject* o = element.toObject(exec);
            JSValue conversionFunction = o->get(exec, exec->propertyNames().toLocaleString);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
            String str;
            CallData callData;
            CallType callType = getCallData(conversionFunction, callData);
            if (callType != CallTypeNone)
                str = call(exec, conversionFunction, callType, callData, element, exec->emptyList()).toWTFString(exec);
            else
                str = element.toWTFString(exec);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
            stringJoiner.append(str);
        }
    }

    return JSValue::encode(stringJoiner.join(exec));
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncJoin(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    StringRecursionChecker checker(exec, thisObj);
    if (JSValue earlyReturnValue = checker.earlyReturnValue())
        return JSValue::encode(earlyReturnValue);

    String separator;
    if (!exec->argument(0).isUndefined())
        separator = exec->argument(0).toWTFString(exec);
    if (separator.isNull())
        separator = String(",", String::ConstructFromLiteral);

    JSStringJoiner stringJoiner(separator, length);

    unsigned k = 0;
    if (isJSArray(thisObj)) {
        JSArray* array = asArray(thisObj);

        for (; k < length; k++) {
            if (!array->canGetIndexQuickly(k))
                break;

            JSValue element = array->getIndexQuickly(k);
            if (!element.isUndefinedOrNull())
                stringJoiner.append(element.toWTFStringInline(exec));
            else
                stringJoiner.append(String());
        }
    }

    for (; k < length; k++) {
        JSValue element = thisObj->get(exec, k);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!element.isUndefinedOrNull())
            stringJoiner.append(element.toWTFStringInline(exec));
        else
            stringJoiner.append(String());
    }

    return JSValue::encode(stringJoiner.join(exec));
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncConcat(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);
    size_t argCount = exec->argumentCount();
    JSValue curArg = thisValue.toObject(exec);
    Checked<unsigned, RecordOverflow> finalArraySize = 0;

    for (size_t i = 0;;) {
        if (JSArray* currentArray = jsDynamicCast<JSArray*>(curArg)) {
            finalArraySize += getLength(exec, currentArray);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
        } else
            finalArraySize++;
        if (i == argCount)
            break;
        curArg = exec->uncheckedArgument(i);
        ++i;
    }

    if (finalArraySize.hasOverflowed())
        return JSValue::encode(throwOutOfMemoryError(exec));

    JSArray* arr = constructEmptyArray(exec, nullptr, finalArraySize.unsafeGet());
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    curArg = thisValue.toObject(exec);
    unsigned n = 0;
    for (size_t i = 0;;) {
        if (JSArray* currentArray = jsDynamicCast<JSArray*>(curArg)) {
            unsigned length = getLength(exec, currentArray);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
            for (unsigned k = 0; k < length; ++k) {
                JSValue v = getProperty(exec, currentArray, k);
                if (exec->hadException())
                    return JSValue::encode(jsUndefined());
                if (v)
                    arr->putDirectIndex(exec, n, v);
                n++;
            }
        } else {
            arr->putDirectIndex(exec, n, curArg);
            n++;
        }
        if (i == argCount)
            break;
        curArg = exec->uncheckedArgument(i);
        ++i;
    }
    arr->setLength(exec, n);
    return JSValue::encode(arr);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncPop(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);

    if (isJSArray(thisValue))
        return JSValue::encode(asArray(thisValue)->pop(exec));

    JSObject* thisObj = thisValue.toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    JSValue result;
    if (length == 0) {
        putLength(exec, thisObj, jsNumber(length));
        result = jsUndefined();
    } else {
        result = thisObj->get(exec, length - 1);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, length - 1)) {
            throwTypeError(exec, ASCIILiteral("Unable to delete property."));
            return JSValue::encode(jsUndefined());
        }
        putLength(exec, thisObj, jsNumber(length - 1));
    }
    return JSValue::encode(result);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncPush(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);

    if (isJSArray(thisValue) && exec->argumentCount() == 1) {
        JSArray* array = asArray(thisValue);
        array->push(exec, exec->uncheckedArgument(0));
        return JSValue::encode(jsNumber(array->length()));
    }
    
    JSObject* thisObj = thisValue.toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    for (unsigned n = 0; n < exec->argumentCount(); n++) {
        // Check for integer overflow; where safe we can do a fast put by index.
        if (length + n >= length)
            thisObj->methodTable()->putByIndex(thisObj, exec, length + n, exec->uncheckedArgument(n), true);
        else {
            PutPropertySlot slot(thisObj);
            Identifier propertyName(exec, JSValue(static_cast<int64_t>(length) + static_cast<int64_t>(n)).toWTFString(exec));
            thisObj->methodTable()->put(thisObj, exec, propertyName, exec->uncheckedArgument(n), slot);
        }
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }
    
    JSValue newLength(static_cast<int64_t>(length) + static_cast<int64_t>(exec->argumentCount()));
    putLength(exec, thisObj, newLength);
    return JSValue::encode(newLength);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncReverse(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    unsigned middle = length / 2;
    for (unsigned k = 0; k < middle; k++) {
        unsigned lk1 = length - k - 1;
        JSValue obj2 = getProperty(exec, thisObj, lk1);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        JSValue obj = getProperty(exec, thisObj, k);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());

        if (obj2) {
            thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, k, obj2, true);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
        } else if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, k)) {
            throwTypeError(exec, ASCIILiteral("Unable to delete property."));
            return JSValue::encode(jsUndefined());
        }

        if (obj) {
            thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, lk1, obj, true);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
        } else if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, lk1)) {
            throwTypeError(exec, ASCIILiteral("Unable to delete property."));
            return JSValue::encode(jsUndefined());
        }
    }
    return JSValue::encode(thisObj);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncShift(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    JSValue result;
    if (length == 0) {
        putLength(exec, thisObj, jsNumber(length));
        result = jsUndefined();
    } else {
        result = thisObj->get(exec, 0);
        shift<JSArray::ShiftCountForShift>(exec, thisObj, 0, 1, 0, length);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        putLength(exec, thisObj, jsNumber(length - 1));
    }
    return JSValue::encode(result);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncSlice(ExecState* exec)
{
    // http://developer.netscape.com/docs/manuals/js/client/jsref/array.htm#1193713 or 15.4.4.10
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    unsigned begin = argumentClampedIndexFromStartOrEnd(exec, 0, length);
    unsigned end = argumentClampedIndexFromStartOrEnd(exec, 1, length, length);

    JSArray* result = constructEmptyArray(exec, nullptr, end - begin);

    unsigned n = 0;
    for (unsigned k = begin; k < end; k++, n++) {
        JSValue v = getProperty(exec, thisObj, k);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (v)
            result->putDirectIndex(exec, n, v);
    }
    result->setLength(exec, n);
    return JSValue::encode(result);
}

inline JSValue getOrHole(JSObject* obj, ExecState* exec, unsigned propertyName)
{
    PropertySlot slot(obj);
    if (obj->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);

    return JSValue();
}

static bool attemptFastSort(ExecState* exec, JSObject* thisObj, JSValue function, CallData& callData, CallType& callType)
{
    if (thisObj->classInfo() != JSArray::info()
        || asArray(thisObj)->hasSparseMap()
        || shouldUseSlowPut(thisObj->indexingType()))
        return false;
    
    if (isNumericCompareFunction(exec, function, callType, callData))
        asArray(thisObj)->sortNumeric(exec, function, callType, callData);
    else if (callType != CallTypeNone)
        asArray(thisObj)->sort(exec, function, callType, callData);
    else
        asArray(thisObj)->sort(exec);
    return true;
}

static bool performSlowSort(ExecState* exec, JSObject* thisObj, unsigned length, JSValue function, CallData& callData, CallType& callType)
{
    // "Min" sort. Not the fastest, but definitely less code than heapsort
    // or quicksort, and much less swapping than bubblesort/insertionsort.
    for (unsigned i = 0; i < length - 1; ++i) {
        JSValue iObj = getOrHole(thisObj, exec, i);
        if (exec->hadException())
            return false;
        unsigned themin = i;
        JSValue minObj = iObj;
        for (unsigned j = i + 1; j < length; ++j) {
            JSValue jObj = getOrHole(thisObj, exec, j);
            if (exec->hadException())
                return false;
            double compareResult;
            if (!jObj)
                compareResult = 1;
            else if (!minObj)
                compareResult = -1;
            else if (jObj.isUndefined())
                compareResult = 1; // don't check minObj because there's no need to differentiate == (0) from > (1)
            else if (minObj.isUndefined())
                compareResult = -1;
            else if (callType != CallTypeNone) {
                MarkedArgumentBuffer l;
                l.append(jObj);
                l.append(minObj);
                compareResult = call(exec, function, callType, callData, jsUndefined(), l).toNumber(exec);
            } else
                compareResult = codePointCompareLessThan(jObj.toWTFStringInline(exec), minObj.toWTFStringInline(exec)) ? -1 : 1;

            if (compareResult < 0) {
                themin = j;
                minObj = jObj;
            }
        }
        // Swap themin and i
        if (themin > i) {
            if (minObj) {
                thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, i, minObj, true);
                if (exec->hadException())
                    return false;
            } else if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, i)) {
                throwTypeError(exec, ASCIILiteral("Unable to delete property."));
                return false;
            }
            if (iObj) {
                thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, themin, iObj, true);
                if (exec->hadException())
                    return false;
            } else if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, themin)) {
                throwTypeError(exec, ASCIILiteral("Unable to delete property."));
                return false;
            }
        }
    }
    return true;
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncSort(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (!length || exec->hadException())
        return JSValue::encode(thisObj);

    JSValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);

    if (attemptFastSort(exec, thisObj, function, callData, callType))
        return JSValue::encode(thisObj);
    
    // Assume that for small-ish arrays, doing the slow sort directly is better.
    if (length < 1000)
        return performSlowSort(exec, thisObj, length, function, callData, callType) ? JSValue::encode(thisObj) : JSValue::encode(jsUndefined());
    
    JSGlobalObject* globalObject = JSGlobalObject::create(
        exec->vm(), JSGlobalObject::createStructure(exec->vm(), jsNull()));
    JSArray* flatArray = constructEmptyArray(globalObject->globalExec(), nullptr);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());
    
    PropertyNameArray nameArray(exec);
    thisObj->methodTable(exec->vm())->getPropertyNames(thisObj, exec, nameArray, IncludeDontEnumProperties);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    Vector<uint32_t, 0, UnsafeVectorOverflow> keys;
    for (size_t i = 0; i < nameArray.size(); ++i) {
        PropertyName name = nameArray[i];
        uint32_t index = name.asIndex();
        if (index == PropertyName::NotAnIndex)
            continue;
        
        JSValue value = getOrHole(thisObj, exec, index);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!value)
            continue;
        keys.append(index);
        flatArray->push(exec, value);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }
    
    if (!attemptFastSort(exec, flatArray, function, callData, callType)
        && !performSlowSort(exec, flatArray, flatArray->length(), function, callData, callType))
        return JSValue::encode(jsUndefined());
    
    for (size_t i = 0; i < keys.size(); ++i) {
        size_t index = keys[i];
        if (index < flatArray->length())
            continue;
        
        if (!thisObj->methodTable(exec->vm())->deletePropertyByIndex(thisObj, exec, index)) {
            throwTypeError(exec, ASCIILiteral("Unable to delete property."));
            return JSValue::encode(jsUndefined());
        }
    }
    
    for (size_t i = flatArray->length(); i--;) {
        JSValue value = getOrHole(flatArray, exec, i);
        RELEASE_ASSERT(value);
        thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, i, value, true);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }
    
    return JSValue::encode(thisObj);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncSplice(ExecState* exec)
{
    // 15.4.4.12

    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());
    
    if (!exec->argumentCount())
        return JSValue::encode(constructEmptyArray(exec, nullptr));

    unsigned begin = argumentClampedIndexFromStartOrEnd(exec, 0, length);

    unsigned deleteCount = length - begin;
    if (exec->argumentCount() > 1) {
        double deleteDouble = exec->uncheckedArgument(1).toInteger(exec);
        if (deleteDouble < 0)
            deleteCount = 0;
        else if (deleteDouble > length - begin)
            deleteCount = length - begin;
        else
            deleteCount = static_cast<unsigned>(deleteDouble);
    }

    JSArray* resObj = JSArray::tryCreateUninitialized(exec->vm(), exec->lexicalGlobalObject()->arrayStructureForIndexingTypeDuringAllocation(ArrayWithUndecided), deleteCount);
    if (!resObj)
        return JSValue::encode(throwOutOfMemoryError(exec));

    JSValue result = resObj;
    VM& vm = exec->vm();
    for (unsigned k = 0; k < deleteCount; k++) {
        JSValue v = getProperty(exec, thisObj, k + begin);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        resObj->initializeIndex(vm, k, v);
    }

    unsigned additionalArgs = std::max<int>(exec->argumentCount() - 2, 0);
    if (additionalArgs < deleteCount) {
        shift<JSArray::ShiftCountForSplice>(exec, thisObj, begin, deleteCount, additionalArgs, length);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    } else if (additionalArgs > deleteCount) {
        unshift<JSArray::ShiftCountForSplice>(exec, thisObj, begin, deleteCount, additionalArgs, length);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }
    for (unsigned k = 0; k < additionalArgs; ++k) {
        thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, k + begin, exec->uncheckedArgument(k + 2), true);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }

    putLength(exec, thisObj, jsNumber(length - deleteCount + additionalArgs));
    return JSValue::encode(result);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncUnShift(ExecState* exec)
{
    // 15.4.4.13

    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    unsigned nrArgs = exec->argumentCount();
    if (nrArgs) {
        unshift<JSArray::ShiftCountForShift>(exec, thisObj, 0, 0, nrArgs, length);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }
    for (unsigned k = 0; k < nrArgs; ++k) {
        thisObj->methodTable(exec->vm())->putByIndex(thisObj, exec, k, exec->uncheckedArgument(k), true);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
    }
    JSValue result = jsNumber(length + nrArgs);
    putLength(exec, thisObj, result);
    return JSValue::encode(result);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncReduce(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    JSValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    unsigned i = 0;
    JSValue rv;
    if (!length && exec->argumentCount() == 1)
        return throwVMTypeError(exec);

    JSArray* array = 0;
    if (isJSArray(thisObj))
        array = asArray(thisObj);

    if (exec->argumentCount() >= 2)
        rv = exec->uncheckedArgument(1);
    else if (array && array->canGetIndexQuickly(0)) {
        rv = array->getIndexQuickly(0);
        i = 1;
    } else {
        for (i = 0; i < length; i++) {
            rv = getProperty(exec, thisObj, i);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
            if (rv)
                break;
        }
        if (!rv)
            return throwVMTypeError(exec);
        i++;
    }

    if (callType == CallTypeJS && array) {
        CachedCall cachedCall(exec, jsCast<JSFunction*>(function), 4);
        for (; i < length && !exec->hadException(); ++i) {
            cachedCall.setThis(jsUndefined());
            cachedCall.setArgument(0, rv);
            JSValue v;
            if (LIKELY(array->canGetIndexQuickly(i)))
                v = array->getIndexQuickly(i);
            else
                break; // length has been made unsafe while we enumerate fallback to slow path
            cachedCall.setArgument(1, v);
            cachedCall.setArgument(2, jsNumber(i));
            cachedCall.setArgument(3, array);
            rv = cachedCall.call();
        }
        if (i == length) // only return if we reached the end of the array
            return JSValue::encode(rv);
    }

    for (; i < length && !exec->hadException(); ++i) {
        JSValue prop = getProperty(exec, thisObj, i);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!prop)
            continue;
        
        MarkedArgumentBuffer eachArguments;
        eachArguments.append(rv);
        eachArguments.append(prop);
        eachArguments.append(jsNumber(i));
        eachArguments.append(thisObj);
        
        rv = call(exec, function, callType, callData, jsUndefined(), eachArguments);
    }
    return JSValue::encode(rv);
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncReduceRight(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    JSValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);
    
    unsigned i = 0;
    JSValue rv;
    if (!length && exec->argumentCount() == 1)
        return throwVMTypeError(exec);

    JSArray* array = 0;
    if (isJSArray(thisObj))
        array = asArray(thisObj);
    
    if (exec->argumentCount() >= 2)
        rv = exec->uncheckedArgument(1);
    else if (array && array->canGetIndexQuickly(length - 1)) {
        rv = array->getIndexQuickly(length - 1);
        i = 1;
    } else {
        for (i = 0; i < length; i++) {
            rv = getProperty(exec, thisObj, length - i - 1);
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
            if (rv)
                break;
        }
        if (!rv)
            return throwVMTypeError(exec);
        i++;
    }
    
    if (callType == CallTypeJS && array) {
        CachedCall cachedCall(exec, jsCast<JSFunction*>(function), 4);
        for (; i < length && !exec->hadException(); ++i) {
            unsigned idx = length - i - 1;
            cachedCall.setThis(jsUndefined());
            cachedCall.setArgument(0, rv);
            if (UNLIKELY(!array->canGetIndexQuickly(idx)))
                break; // length has been made unsafe while we enumerate fallback to slow path
            cachedCall.setArgument(1, array->getIndexQuickly(idx));
            cachedCall.setArgument(2, jsNumber(idx));
            cachedCall.setArgument(3, array);
            rv = cachedCall.call();
        }
        if (i == length) // only return if we reached the end of the array
            return JSValue::encode(rv);
    }
    
    for (; i < length && !exec->hadException(); ++i) {
        unsigned idx = length - i - 1;
        JSValue prop = getProperty(exec, thisObj, idx);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!prop)
            continue;
        
        MarkedArgumentBuffer eachArguments;
        eachArguments.append(rv);
        eachArguments.append(prop);
        eachArguments.append(jsNumber(idx));
        eachArguments.append(thisObj);
        
        rv = call(exec, function, callType, callData, jsUndefined(), eachArguments);
    }
    return JSValue::encode(rv);        
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncIndexOf(ExecState* exec)
{
    // 15.4.4.14
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    unsigned index = argumentClampedIndexFromStartOrEnd(exec, 1, length);
    JSValue searchElement = exec->argument(0);
    for (; index < length; ++index) {
        JSValue e = getProperty(exec, thisObj, index);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!e)
            continue;
        if (JSValue::strictEqual(exec, searchElement, e))
            return JSValue::encode(jsNumber(index));
    }

    return JSValue::encode(jsNumber(-1));
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncLastIndexOf(ExecState* exec)
{
    // 15.4.4.15
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    unsigned length = getLength(exec, thisObj);
    if (!length)
        return JSValue::encode(jsNumber(-1));

    unsigned index = length - 1;
    if (exec->argumentCount() >= 2) {
        JSValue fromValue = exec->uncheckedArgument(1);
        double fromDouble = fromValue.toInteger(exec);
        if (fromDouble < 0) {
            fromDouble += length;
            if (fromDouble < 0)
                return JSValue::encode(jsNumber(-1));
        }
        if (fromDouble < length)
            index = static_cast<unsigned>(fromDouble);
    }

    JSValue searchElement = exec->argument(0);
    do {
        RELEASE_ASSERT(index < length);
        JSValue e = getProperty(exec, thisObj, index);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());
        if (!e)
            continue;
        if (JSValue::strictEqual(exec, searchElement, e))
            return JSValue::encode(jsNumber(index));
    } while (index--);

    return JSValue::encode(jsNumber(-1));
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncValues(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    return JSValue::encode(JSArrayIterator::create(exec, exec->callee()->globalObject()->arrayIteratorStructure(), ArrayIterateValue, thisObj));
}

EncodedJSValue JSC_HOST_CALL arrayProtoFuncEntries(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    return JSValue::encode(JSArrayIterator::create(exec, exec->callee()->globalObject()->arrayIteratorStructure(), ArrayIterateKeyValue, thisObj));
}
    
EncodedJSValue JSC_HOST_CALL arrayProtoFuncKeys(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    return JSValue::encode(JSArrayIterator::create(exec, exec->callee()->globalObject()->arrayIteratorStructure(), ArrayIterateKey, thisObj));
}

} // namespace JSC
