/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DFGOperations.h"

#include "Arguments.h"
#include "ButterflyInlineMethods.h"
#include "CodeBlock.h"
#include "CopiedSpaceInlineMethods.h"
#include "DFGOSRExit.h"
#include "DFGRepatch.h"
#include "DFGThunks.h"
#include "HostCallReturnValue.h"
#include "GetterSetter.h"
#include "Interpreter.h"
#include "JIT.h"
#include "JITExceptions.h"
#include "JSActivation.h"
#include "JSGlobalData.h"
#include "JSNameScope.h"
#include "NameInstance.h"
#include "Operations.h"
#include <wtf/InlineASM.h>

#if ENABLE(JIT)

#if ENABLE(DFG_JIT)

#if CPU(X86_64)

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, register) \
    asm( \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    SYMBOL_STRING(function) ":" "\n" \
        "mov (%rsp), %" STRINGIZE(register) "\n" \
        "jmp " LOCAL_REFERENCE(function##WithReturnAddress) "\n" \
    );
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_E(function)    FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, rsi)
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_ECI(function)  FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, rcx)
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(function)  FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, rcx)
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(function) FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, r8)

#elif CPU(X86)

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, offset) \
    asm( \
    ".text" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    SYMBOL_STRING(function) ":" "\n" \
        "mov (%esp), %eax\n" \
        "mov %eax, " STRINGIZE(offset) "(%esp)\n" \
        "jmp " LOCAL_REFERENCE(function##WithReturnAddress) "\n" \
    );
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_E(function)    FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, 8)
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_ECI(function)  FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, 16)
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(function)  FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, 20)
#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(function) FUNCTION_WRAPPER_WITH_RETURN_ADDRESS(function, 24)

#elif COMPILER(GCC) && CPU(ARM_THUMB2)

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_E(function) \
    asm ( \
    ".text" "\n" \
    ".align 2" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    ".thumb" "\n" \
    ".thumb_func " THUMB_FUNC_PARAM(function) "\n" \
    SYMBOL_STRING(function) ":" "\n" \
        "mov a2, lr" "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_ECI(function) \
    asm ( \
    ".text" "\n" \
    ".align 2" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    ".thumb" "\n" \
    ".thumb_func " THUMB_FUNC_PARAM(function) "\n" \
    SYMBOL_STRING(function) ":" "\n" \
        "mov a4, lr" "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

// EncodedJSValue in JSVALUE32_64 is a 64-bit integer. When being compiled in ARM EABI, it must be aligned even-numbered register (r0, r2 or [sp]).
// As a result, return address will be at a 4-byte further location in the following cases.
#if COMPILER_SUPPORTS(EABI) && CPU(ARM)
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJI "str lr, [sp, #4]"
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJCI "str lr, [sp, #8]"
#else
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJI "str lr, [sp, #0]"
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJCI "str lr, [sp, #4]"
#endif

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(function) \
    asm ( \
    ".text" "\n" \
    ".align 2" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    ".thumb" "\n" \
    ".thumb_func " THUMB_FUNC_PARAM(function) "\n" \
    SYMBOL_STRING(function) ":" "\n" \
        INSTRUCTION_STORE_RETURN_ADDRESS_EJI "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(function) \
    asm ( \
    ".text" "\n" \
    ".align 2" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    ".thumb" "\n" \
    ".thumb_func " THUMB_FUNC_PARAM(function) "\n" \
    SYMBOL_STRING(function) ":" "\n" \
        INSTRUCTION_STORE_RETURN_ADDRESS_EJCI "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

#elif COMPILER(GCC) && CPU(ARM_TRADITIONAL)

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_E(function) \
    asm ( \
    ".text" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    INLINE_ARM_FUNCTION(function) \
    SYMBOL_STRING(function) ":" "\n" \
        "mov a2, lr" "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_ECI(function) \
    asm ( \
    ".text" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    INLINE_ARM_FUNCTION(function) \
    SYMBOL_STRING(function) ":" "\n" \
        "mov a4, lr" "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

// EncodedJSValue in JSVALUE32_64 is a 64-bit integer. When being compiled in ARM EABI, it must be aligned even-numbered register (r0, r2 or [sp]).
// As a result, return address will be at a 4-byte further location in the following cases.
#if COMPILER_SUPPORTS(EABI) && CPU(ARM)
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJI "str lr, [sp, #4]"
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJCI "str lr, [sp, #8]"
#else
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJI "str lr, [sp, #0]"
#define INSTRUCTION_STORE_RETURN_ADDRESS_EJCI "str lr, [sp, #4]"
#endif

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(function) \
    asm ( \
    ".text" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    INLINE_ARM_FUNCTION(function) \
    SYMBOL_STRING(function) ":" "\n" \
        INSTRUCTION_STORE_RETURN_ADDRESS_EJI "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

#define FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(function) \
    asm ( \
    ".text" "\n" \
    ".globl " SYMBOL_STRING(function) "\n" \
    HIDE_SYMBOL(function) "\n" \
    INLINE_ARM_FUNCTION(function) \
    SYMBOL_STRING(function) ":" "\n" \
        INSTRUCTION_STORE_RETURN_ADDRESS_EJCI "\n" \
        "b " LOCAL_REFERENCE(function) "WithReturnAddress" "\n" \
    );

#endif

#define P_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_E(function) \
void* DFG_OPERATION function##WithReturnAddress(ExecState*, ReturnAddressPtr) REFERENCED_FROM_ASM WTF_INTERNAL; \
FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_E(function)

#define J_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_ECI(function) \
EncodedJSValue DFG_OPERATION function##WithReturnAddress(ExecState*, JSCell*, Identifier*, ReturnAddressPtr) REFERENCED_FROM_ASM WTF_INTERNAL; \
FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_ECI(function)

#define J_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(function) \
EncodedJSValue DFG_OPERATION function##WithReturnAddress(ExecState*, EncodedJSValue, Identifier*, ReturnAddressPtr) REFERENCED_FROM_ASM WTF_INTERNAL; \
FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(function)

#define V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(function) \
void DFG_OPERATION function##WithReturnAddress(ExecState*, EncodedJSValue, JSCell*, Identifier*, ReturnAddressPtr) REFERENCED_FROM_ASM WTF_INTERNAL; \
FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(function)

namespace JSC { namespace DFG {

template<bool strict>
static inline void putByVal(ExecState* exec, JSValue baseValue, uint32_t index, JSValue value)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    
    if (baseValue.isObject()) {
        JSObject* object = asObject(baseValue);
        if (object->canSetIndexQuickly(index)) {
            object->setIndexQuickly(globalData, index, value);
            return;
        }

        object->methodTable()->putByIndex(object, exec, index, value, strict);
        return;
    }

    baseValue.putByIndex(exec, index, value, strict);
}

template<bool strict>
ALWAYS_INLINE static void DFG_OPERATION operationPutByValInternal(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    JSValue baseValue = JSValue::decode(encodedBase);
    JSValue property = JSValue::decode(encodedProperty);
    JSValue value = JSValue::decode(encodedValue);

    if (LIKELY(property.isUInt32())) {
        putByVal<strict>(exec, baseValue, property.asUInt32(), value);
        return;
    }

    if (property.isDouble()) {
        double propertyAsDouble = property.asDouble();
        uint32_t propertyAsUInt32 = static_cast<uint32_t>(propertyAsDouble);
        if (propertyAsDouble == propertyAsUInt32) {
            putByVal<strict>(exec, baseValue, propertyAsUInt32, value);
            return;
        }
    }

    if (isName(property)) {
        PutPropertySlot slot(strict);
        baseValue.put(exec, jsCast<NameInstance*>(property.asCell())->privateName(), value, slot);
        return;
    }

    // Don't put to an object if toString throws an exception.
    Identifier ident(exec, property.toString(exec)->value(exec));
    if (!globalData->exception) {
        PutPropertySlot slot(strict);
        baseValue.put(exec, ident, value, slot);
    }
}

extern "C" {

EncodedJSValue DFG_OPERATION operationConvertThis(ExecState* exec, EncodedJSValue encodedOp)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    return JSValue::encode(JSValue::decode(encodedOp).toThisObject(exec));
}

JSCell* DFG_OPERATION operationCreateThis(ExecState* exec, JSCell* constructor)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

#if !ASSERT_DISABLED
    ConstructData constructData;
    ASSERT(jsCast<JSFunction*>(constructor)->methodTable()->getConstructData(jsCast<JSFunction*>(constructor), constructData) == ConstructTypeJS);
#endif
    
    return constructEmptyObject(exec, jsCast<JSFunction*>(constructor)->cachedInheritorID(exec));
}

JSCell* DFG_OPERATION operationNewObject(ExecState* exec)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return constructEmptyObject(exec);
}

EncodedJSValue DFG_OPERATION operationValueAdd(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    JSValue op1 = JSValue::decode(encodedOp1);
    JSValue op2 = JSValue::decode(encodedOp2);
    
    return JSValue::encode(jsAdd(exec, op1, op2));
}

EncodedJSValue DFG_OPERATION operationValueAddNotNumber(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    JSValue op1 = JSValue::decode(encodedOp1);
    JSValue op2 = JSValue::decode(encodedOp2);
    
    ASSERT(!op1.isNumber() || !op2.isNumber());
    
    if (op1.isString() && !op2.isObject())
        return JSValue::encode(jsString(exec, asString(op1), op2.toString(exec)));

    return JSValue::encode(jsAddSlowCase(exec, op1, op2));
}

static inline EncodedJSValue getByVal(ExecState* exec, JSCell* base, uint32_t index)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    
    if (base->isObject()) {
        JSObject* object = asObject(base);
        if (object->canGetIndexQuickly(index))
            return JSValue::encode(object->getIndexQuickly(index));
    }

    if (isJSString(base) && asString(base)->canGetIndex(index))
        return JSValue::encode(asString(base)->getIndex(exec, index));

    return JSValue::encode(JSValue(base).get(exec, index));
}

EncodedJSValue DFG_OPERATION operationGetByVal(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    JSValue baseValue = JSValue::decode(encodedBase);
    JSValue property = JSValue::decode(encodedProperty);

    if (LIKELY(baseValue.isCell())) {
        JSCell* base = baseValue.asCell();

        if (property.isUInt32()) {
            return getByVal(exec, base, property.asUInt32());
        } else if (property.isDouble()) {
            double propertyAsDouble = property.asDouble();
            uint32_t propertyAsUInt32 = static_cast<uint32_t>(propertyAsDouble);
            if (propertyAsUInt32 == propertyAsDouble)
                return getByVal(exec, base, propertyAsUInt32);
        } else if (property.isString()) {
            if (JSValue result = base->fastGetOwnProperty(exec, asString(property)->value(exec)))
                return JSValue::encode(result);
        }
    }

    if (isName(property))
        return JSValue::encode(baseValue.get(exec, jsCast<NameInstance*>(property.asCell())->privateName()));

    Identifier ident(exec, property.toString(exec)->value(exec));
    return JSValue::encode(baseValue.get(exec, ident));
}

EncodedJSValue DFG_OPERATION operationGetByValCell(ExecState* exec, JSCell* base, EncodedJSValue encodedProperty)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    JSValue property = JSValue::decode(encodedProperty);

    if (property.isUInt32())
        return getByVal(exec, base, property.asUInt32());
    if (property.isDouble()) {
        double propertyAsDouble = property.asDouble();
        uint32_t propertyAsUInt32 = static_cast<uint32_t>(propertyAsDouble);
        if (propertyAsUInt32 == propertyAsDouble)
            return getByVal(exec, base, propertyAsUInt32);
    } else if (property.isString()) {
        if (JSValue result = base->fastGetOwnProperty(exec, asString(property)->value(exec)))
            return JSValue::encode(result);
    }

    if (isName(property))
        return JSValue::encode(JSValue(base).get(exec, jsCast<NameInstance*>(property.asCell())->privateName()));

    Identifier ident(exec, property.toString(exec)->value(exec));
    return JSValue::encode(JSValue(base).get(exec, ident));
}

EncodedJSValue DFG_OPERATION operationGetByValArrayInt(ExecState* exec, JSArray* base, int32_t index)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    if (index < 0) {
        // Go the slowest way possible becase negative indices don't use indexed storage.
        return JSValue::encode(JSValue(base).get(exec, Identifier::from(exec, index)));
    }

    // Use this since we know that the value is out of bounds.
    return JSValue::encode(JSValue(base).get(exec, index));
}

EncodedJSValue DFG_OPERATION operationGetById(ExecState* exec, EncodedJSValue base, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    JSValue baseValue = JSValue::decode(base);
    PropertySlot slot(baseValue);
    return JSValue::encode(baseValue.get(exec, *propertyName, slot));
}

J_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(operationGetByIdBuildList);
EncodedJSValue DFG_OPERATION operationGetByIdBuildListWithReturnAddress(ExecState* exec, EncodedJSValue base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue baseValue = JSValue::decode(base);
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(exec, *propertyName, slot);

    if (accessType == static_cast<AccessType>(stubInfo.accessType))
        dfgBuildGetByIDList(exec, baseValue, *propertyName, slot, stubInfo);

    return JSValue::encode(result);
}

J_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(operationGetByIdProtoBuildList);
EncodedJSValue DFG_OPERATION operationGetByIdProtoBuildListWithReturnAddress(ExecState* exec, EncodedJSValue base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue baseValue = JSValue::decode(base);
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(exec, *propertyName, slot);
    
    if (accessType == static_cast<AccessType>(stubInfo.accessType))
        dfgBuildGetByIDProtoList(exec, baseValue, *propertyName, slot, stubInfo);

    return JSValue::encode(result);
}

J_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJI(operationGetByIdOptimize);
EncodedJSValue DFG_OPERATION operationGetByIdOptimizeWithReturnAddress(ExecState* exec, EncodedJSValue base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue baseValue = JSValue::decode(base);
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(exec, *propertyName, slot);
    
    if (accessType == static_cast<AccessType>(stubInfo.accessType)) {
        if (stubInfo.seen)
            dfgRepatchGetByID(exec, baseValue, *propertyName, slot, stubInfo);
        else
            stubInfo.seen = true;
    }

    return JSValue::encode(result);
}

EncodedJSValue DFG_OPERATION operationCallCustomGetter(ExecState* exec, JSCell* base, PropertySlot::GetValueFunc function, Identifier* ident)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::encode(function(exec, asObject(base), *ident));
}

EncodedJSValue DFG_OPERATION operationCallGetter(ExecState* exec, JSCell* base, JSCell* value)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    GetterSetter* getterSetter = asGetterSetter(value);
    JSObject* getter = getterSetter->getter();
    if (!getter)
        return JSValue::encode(jsUndefined());
    CallData callData;
    CallType callType = getter->methodTable()->getCallData(getter, callData);
    return JSValue::encode(call(exec, getter, callType, callData, asObject(base), ArgList()));
}

void DFG_OPERATION operationPutByValStrict(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    operationPutByValInternal<true>(exec, encodedBase, encodedProperty, encodedValue);
}

void DFG_OPERATION operationPutByValNonStrict(ExecState* exec, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    operationPutByValInternal<false>(exec, encodedBase, encodedProperty, encodedValue);
}

void DFG_OPERATION operationPutByValCellStrict(ExecState* exec, JSCell* cell, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    operationPutByValInternal<true>(exec, JSValue::encode(cell), encodedProperty, encodedValue);
}

void DFG_OPERATION operationPutByValCellNonStrict(ExecState* exec, JSCell* cell, EncodedJSValue encodedProperty, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    operationPutByValInternal<false>(exec, JSValue::encode(cell), encodedProperty, encodedValue);
}

void DFG_OPERATION operationPutByValBeyondArrayBoundsStrict(ExecState* exec, JSObject* array, int32_t index, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    if (index >= 0) {
        // We should only get here if index is outside the existing vector.
        ASSERT(!array->canSetIndexQuickly(index));
        array->methodTable()->putByIndex(array, exec, index, JSValue::decode(encodedValue), true);
        return;
    }
    
    PutPropertySlot slot(true);
    array->methodTable()->put(
        array, exec, Identifier::from(exec, index), JSValue::decode(encodedValue), slot);
}

void DFG_OPERATION operationPutByValBeyondArrayBoundsNonStrict(ExecState* exec, JSObject* array, int32_t index, EncodedJSValue encodedValue)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    if (index >= 0) {
        // We should only get here if index is outside the existing vector.
        ASSERT(!array->canSetIndexQuickly(index));
        array->methodTable()->putByIndex(array, exec, index, JSValue::decode(encodedValue), false);
        return;
    }
    
    PutPropertySlot slot(false);
    array->methodTable()->put(
        array, exec, Identifier::from(exec, index), JSValue::decode(encodedValue), slot);
}

EncodedJSValue DFG_OPERATION operationArrayPush(ExecState* exec, EncodedJSValue encodedValue, JSArray* array)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    array->push(exec, JSValue::decode(encodedValue));
    return JSValue::encode(jsNumber(array->length()));
}

EncodedJSValue DFG_OPERATION operationArrayPop(ExecState* exec, JSArray* array)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::encode(array->pop(exec));
}
        
EncodedJSValue DFG_OPERATION operationRegExpExec(ExecState* exec, JSCell* base, JSCell* argument)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    
    if (!base->inherits(&RegExpObject::s_info))
        return throwVMTypeError(exec);

    ASSERT(argument->isString() || argument->isObject());
    JSString* input = argument->isString() ? asString(argument) : asObject(argument)->toString(exec);
    return JSValue::encode(asRegExpObject(base)->exec(exec, input));
}
        
size_t DFG_OPERATION operationRegExpTest(ExecState* exec, JSCell* base, JSCell* argument)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    if (!base->inherits(&RegExpObject::s_info)) {
        throwTypeError(exec);
        return false;
    }

    ASSERT(argument->isString() || argument->isObject());
    JSString* input = argument->isString() ? asString(argument) : asObject(argument)->toString(exec);
    return asRegExpObject(base)->test(exec, input);
}
        
void DFG_OPERATION operationPutByIdStrict(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    PutPropertySlot slot(true);
    base->methodTable()->put(base, exec, *propertyName, JSValue::decode(encodedValue), slot);
}

void DFG_OPERATION operationPutByIdNonStrict(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    PutPropertySlot slot(false);
    base->methodTable()->put(base, exec, *propertyName, JSValue::decode(encodedValue), slot);
}

void DFG_OPERATION operationPutByIdDirectStrict(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    PutPropertySlot slot(true);
    ASSERT(base->isObject());
    asObject(base)->putDirect(exec->globalData(), *propertyName, JSValue::decode(encodedValue), slot);
}

void DFG_OPERATION operationPutByIdDirectNonStrict(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    PutPropertySlot slot(false);
    ASSERT(base->isObject());
    asObject(base)->putDirect(exec->globalData(), *propertyName, JSValue::decode(encodedValue), slot);
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdStrictOptimize);
void DFG_OPERATION operationPutByIdStrictOptimizeWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue value = JSValue::decode(encodedValue);
    JSValue baseValue(base);
    PutPropertySlot slot(true);
    
    baseValue.put(exec, *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    if (stubInfo.seen)
        dfgRepatchPutByID(exec, baseValue, *propertyName, slot, stubInfo, NotDirect);
    else
        stubInfo.seen = true;
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdNonStrictOptimize);
void DFG_OPERATION operationPutByIdNonStrictOptimizeWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue value = JSValue::decode(encodedValue);
    JSValue baseValue(base);
    PutPropertySlot slot(false);
    
    baseValue.put(exec, *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    if (stubInfo.seen)
        dfgRepatchPutByID(exec, baseValue, *propertyName, slot, stubInfo, NotDirect);
    else
        stubInfo.seen = true;
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdDirectStrictOptimize);
void DFG_OPERATION operationPutByIdDirectStrictOptimizeWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue value = JSValue::decode(encodedValue);
    PutPropertySlot slot(true);
    
    ASSERT(base->isObject());
    asObject(base)->putDirect(exec->globalData(), *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    if (stubInfo.seen)
        dfgRepatchPutByID(exec, base, *propertyName, slot, stubInfo, Direct);
    else
        stubInfo.seen = true;
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdDirectNonStrictOptimize);
void DFG_OPERATION operationPutByIdDirectNonStrictOptimizeWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue value = JSValue::decode(encodedValue);
    PutPropertySlot slot(false);
    
    ASSERT(base->isObject());
    asObject(base)->putDirect(exec->globalData(), *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    if (stubInfo.seen)
        dfgRepatchPutByID(exec, base, *propertyName, slot, stubInfo, Direct);
    else
        stubInfo.seen = true;
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdStrictBuildList);
void DFG_OPERATION operationPutByIdStrictBuildListWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue value = JSValue::decode(encodedValue);
    JSValue baseValue(base);
    PutPropertySlot slot(true);
    
    baseValue.put(exec, *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    dfgBuildPutByIdList(exec, baseValue, *propertyName, slot, stubInfo, NotDirect);
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdNonStrictBuildList);
void DFG_OPERATION operationPutByIdNonStrictBuildListWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue value = JSValue::decode(encodedValue);
    JSValue baseValue(base);
    PutPropertySlot slot(false);
    
    baseValue.put(exec, *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    dfgBuildPutByIdList(exec, baseValue, *propertyName, slot, stubInfo, NotDirect);
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdDirectStrictBuildList);
void DFG_OPERATION operationPutByIdDirectStrictBuildListWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);
    
    JSValue value = JSValue::decode(encodedValue);
    PutPropertySlot slot(true);
    
    ASSERT(base->isObject());
    asObject(base)->putDirect(exec->globalData(), *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    dfgBuildPutByIdList(exec, base, *propertyName, slot, stubInfo, Direct);
}

V_FUNCTION_WRAPPER_WITH_RETURN_ADDRESS_EJCI(operationPutByIdDirectNonStrictBuildList);
void DFG_OPERATION operationPutByIdDirectNonStrictBuildListWithReturnAddress(ExecState* exec, EncodedJSValue encodedValue, JSCell* base, Identifier* propertyName, ReturnAddressPtr returnAddress)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    StructureStubInfo& stubInfo = exec->codeBlock()->getStubInfo(returnAddress);
    AccessType accessType = static_cast<AccessType>(stubInfo.accessType);

    JSValue value = JSValue::decode(encodedValue);
    PutPropertySlot slot(false);
    
    ASSERT(base->isObject());
    asObject(base)->putDirect(exec->globalData(), *propertyName, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo.accessType))
        return;
    
    dfgBuildPutByIdList(exec, base, *propertyName, slot, stubInfo, Direct);
}

size_t DFG_OPERATION operationCompareLess(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return jsLess<true>(exec, JSValue::decode(encodedOp1), JSValue::decode(encodedOp2));
}

size_t DFG_OPERATION operationCompareLessEq(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return jsLessEq<true>(exec, JSValue::decode(encodedOp1), JSValue::decode(encodedOp2));
}

size_t DFG_OPERATION operationCompareGreater(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return jsLess<false>(exec, JSValue::decode(encodedOp2), JSValue::decode(encodedOp1));
}

size_t DFG_OPERATION operationCompareGreaterEq(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return jsLessEq<false>(exec, JSValue::decode(encodedOp2), JSValue::decode(encodedOp1));
}

size_t DFG_OPERATION operationCompareEq(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::equalSlowCaseInline(exec, JSValue::decode(encodedOp1), JSValue::decode(encodedOp2));
}

size_t DFG_OPERATION operationCompareStrictEqCell(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    JSValue op1 = JSValue::decode(encodedOp1);
    JSValue op2 = JSValue::decode(encodedOp2);
    
    ASSERT(op1.isCell());
    ASSERT(op2.isCell());
    
    return JSValue::strictEqualSlowCaseInline(exec, op1, op2);
}

size_t DFG_OPERATION operationCompareStrictEq(ExecState* exec, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    JSValue src1 = JSValue::decode(encodedOp1);
    JSValue src2 = JSValue::decode(encodedOp2);
    
    return JSValue::strictEqual(exec, src1, src2);
}

static void* handleHostCall(ExecState* execCallee, JSValue callee, CodeSpecializationKind kind)
{
    ExecState* exec = execCallee->callerFrame();
    JSGlobalData* globalData = &exec->globalData();

    execCallee->setScope(exec->scope());
    execCallee->setCodeBlock(0);

    if (kind == CodeForCall) {
        CallData callData;
        CallType callType = getCallData(callee, callData);
    
        ASSERT(callType != CallTypeJS);
    
        if (callType == CallTypeHost) {
            NativeCallFrameTracer tracer(globalData, execCallee);
            execCallee->setCallee(asObject(callee));
            globalData->hostCallReturnValue = JSValue::decode(callData.native.function(execCallee));
            if (globalData->exception)
                return globalData->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();

            return reinterpret_cast<void*>(getHostCallReturnValue);
        }
    
        ASSERT(callType == CallTypeNone);
        exec->globalData().exception = createNotAFunctionError(exec, callee);
        return globalData->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();
    }

    ASSERT(kind == CodeForConstruct);
    
    ConstructData constructData;
    ConstructType constructType = getConstructData(callee, constructData);
    
    ASSERT(constructType != ConstructTypeJS);
    
    if (constructType == ConstructTypeHost) {
        NativeCallFrameTracer tracer(globalData, execCallee);
        execCallee->setCallee(asObject(callee));
        globalData->hostCallReturnValue = JSValue::decode(constructData.native.function(execCallee));
        if (globalData->exception)
            return globalData->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();

        return reinterpret_cast<void*>(getHostCallReturnValue);
    }
    
    ASSERT(constructType == ConstructTypeNone);
    exec->globalData().exception = createNotAConstructorError(exec, callee);
    return globalData->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();
}

inline char* linkFor(ExecState* execCallee, CodeSpecializationKind kind)
{
    ExecState* exec = execCallee->callerFrame();
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    JSValue calleeAsValue = execCallee->calleeAsValue();
    JSCell* calleeAsFunctionCell = getJSFunction(calleeAsValue);
    if (!calleeAsFunctionCell)
        return reinterpret_cast<char*>(handleHostCall(execCallee, calleeAsValue, kind));

    JSFunction* callee = jsCast<JSFunction*>(calleeAsFunctionCell);
    execCallee->setScope(callee->scopeUnchecked());
    ExecutableBase* executable = callee->executable();

    MacroAssemblerCodePtr codePtr;
    CodeBlock* codeBlock = 0;
    if (executable->isHostFunction())
        codePtr = executable->generatedJITCodeFor(kind).addressForCall();
    else {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->compileFor(execCallee, callee->scope(), kind);
        if (error) {
            globalData->exception = createStackOverflowError(exec);
            return reinterpret_cast<char*>(globalData->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress());
        }
        codeBlock = &functionExecutable->generatedBytecodeFor(kind);
        if (execCallee->argumentCountIncludingThis() < static_cast<size_t>(codeBlock->numParameters()))
            codePtr = functionExecutable->generatedJITCodeWithArityCheckFor(kind);
        else
            codePtr = functionExecutable->generatedJITCodeFor(kind).addressForCall();
    }
    CallLinkInfo& callLinkInfo = exec->codeBlock()->getCallLinkInfo(execCallee->returnPC());
    if (!callLinkInfo.seenOnce())
        callLinkInfo.setSeen();
    else
        dfgLinkFor(execCallee, callLinkInfo, codeBlock, callee, codePtr, kind);
    return reinterpret_cast<char*>(codePtr.executableAddress());
}

char* DFG_OPERATION operationLinkCall(ExecState* execCallee)
{
    return linkFor(execCallee, CodeForCall);
}

char* DFG_OPERATION operationLinkConstruct(ExecState* execCallee)
{
    return linkFor(execCallee, CodeForConstruct);
}

inline char* virtualFor(ExecState* execCallee, CodeSpecializationKind kind)
{
    ExecState* exec = execCallee->callerFrame();
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    JSValue calleeAsValue = execCallee->calleeAsValue();
    JSCell* calleeAsFunctionCell = getJSFunction(calleeAsValue);
    if (UNLIKELY(!calleeAsFunctionCell))
        return reinterpret_cast<char*>(handleHostCall(execCallee, calleeAsValue, kind));
    
    JSFunction* function = jsCast<JSFunction*>(calleeAsFunctionCell);
    execCallee->setScope(function->scopeUnchecked());
    ExecutableBase* executable = function->executable();
    if (UNLIKELY(!executable->hasJITCodeFor(kind))) {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->compileFor(execCallee, function->scope(), kind);
        if (error) {
            exec->globalData().exception = error;
            return reinterpret_cast<char*>(globalData->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress());
        }
    }
    return reinterpret_cast<char*>(executable->generatedJITCodeWithArityCheckFor(kind).executableAddress());
}

char* DFG_OPERATION operationVirtualCall(ExecState* execCallee)
{    
    return virtualFor(execCallee, CodeForCall);
}

char* DFG_OPERATION operationVirtualConstruct(ExecState* execCallee)
{
    return virtualFor(execCallee, CodeForConstruct);
}

void DFG_OPERATION operationNotifyGlobalVarWrite(WatchpointSet* watchpointSet)
{
    watchpointSet->notifyWrite();
}

EncodedJSValue DFG_OPERATION operationResolve(ExecState* exec, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    return JSValue::encode(JSScope::resolve(exec, *propertyName));
}

EncodedJSValue DFG_OPERATION operationResolveBase(ExecState* exec, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::encode(JSScope::resolveBase(exec, *propertyName, false));
}

EncodedJSValue DFG_OPERATION operationResolveBaseStrictPut(ExecState* exec, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::encode(JSScope::resolveBase(exec, *propertyName, true));
}

EncodedJSValue DFG_OPERATION operationResolveGlobal(ExecState* exec, GlobalResolveInfo* resolveInfo, JSGlobalObject* globalObject, Identifier* propertyName)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    return JSValue::encode(JSScope::resolveGlobal(exec, *propertyName, globalObject, &resolveInfo->structure, &resolveInfo->offset));
}

EncodedJSValue DFG_OPERATION operationToPrimitive(ExecState* exec, EncodedJSValue value)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::encode(JSValue::decode(value).toPrimitive(exec));
}

EncodedJSValue DFG_OPERATION operationStrCat(ExecState* exec, void* buffer, size_t size)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    return JSValue::encode(jsString(exec, static_cast<Register*>(buffer), size));
}

EncodedJSValue DFG_OPERATION operationNewArray(ExecState* exec, Structure* arrayStructure, void* buffer, size_t size)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    return JSValue::encode(constructArray(exec, arrayStructure, static_cast<JSValue*>(buffer), size));
}

EncodedJSValue DFG_OPERATION operationNewEmptyArray(ExecState* exec, Structure* arrayStructure)
{
    return JSValue::encode(JSArray::create(exec->globalData(), arrayStructure));
}

EncodedJSValue DFG_OPERATION operationNewArrayWithSize(ExecState* exec, Structure* arrayStructure, int32_t size)
{
    return JSValue::encode(JSArray::create(exec->globalData(), arrayStructure, size));
}

EncodedJSValue DFG_OPERATION operationNewArrayBuffer(ExecState* exec, size_t start, size_t size)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    return JSValue::encode(constructArray(exec, exec->codeBlock()->constantBuffer(start), size));
}

EncodedJSValue DFG_OPERATION operationNewRegexp(ExecState* exec, void* regexpPtr)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    RegExp* regexp = static_cast<RegExp*>(regexpPtr);
    if (!regexp->isValid()) {
        throwError(exec, createSyntaxError(exec, "Invalid flags supplied to RegExp constructor."));
        return JSValue::encode(jsUndefined());
    }
    
    return JSValue::encode(RegExpObject::create(exec->globalData(), exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->regExpStructure(), regexp));
}

JSCell* DFG_OPERATION operationCreateActivation(ExecState* exec)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    JSActivation* activation = JSActivation::create(
        globalData, exec, static_cast<FunctionExecutable*>(exec->codeBlock()->ownerExecutable()));
    exec->setScope(activation);
    return activation;
}

JSCell* DFG_OPERATION operationCreateArguments(ExecState* exec)
{
    // NB: This needs to be exceedingly careful with top call frame tracking, since it
    // may be called from OSR exit, while the state of the call stack is bizarre.
    Arguments* result = Arguments::create(exec->globalData(), exec);
    ASSERT(!exec->globalData().exception);
    return result;
}

JSCell* DFG_OPERATION operationCreateInlinedArguments(
    ExecState* exec, InlineCallFrame* inlineCallFrame)
{
    // NB: This needs to be exceedingly careful with top call frame tracking, since it
    // may be called from OSR exit, while the state of the call stack is bizarre.
    Arguments* result = Arguments::create(exec->globalData(), exec, inlineCallFrame);
    ASSERT(!exec->globalData().exception);
    return result;
}

void DFG_OPERATION operationTearOffActivation(ExecState* exec, JSCell* activationCell)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    jsCast<JSActivation*>(activationCell)->tearOff(exec->globalData());
}

void DFG_OPERATION operationTearOffArguments(ExecState* exec, JSCell* argumentsCell, JSCell* activationCell)
{
    ASSERT(exec->codeBlock()->usesArguments());
    if (activationCell) {
        jsCast<Arguments*>(argumentsCell)->didTearOffActivation(exec, jsCast<JSActivation*>(activationCell));
        return;
    }
    jsCast<Arguments*>(argumentsCell)->tearOff(exec);
}

void DFG_OPERATION operationTearOffInlinedArguments(
    ExecState* exec, JSCell* argumentsCell, JSCell* activationCell, InlineCallFrame* inlineCallFrame)
{
    ASSERT_UNUSED(activationCell, !activationCell); // Currently, we don't inline functions with activations.
    jsCast<Arguments*>(argumentsCell)->tearOff(exec, inlineCallFrame);
}

EncodedJSValue DFG_OPERATION operationGetArgumentsLength(ExecState* exec, int32_t argumentsRegister)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    // Here we can assume that the argumernts were created. Because otherwise the JIT code would
    // have not made this call.
    Identifier ident(&globalData, "length");
    JSValue baseValue = exec->uncheckedR(argumentsRegister).jsValue();
    PropertySlot slot(baseValue);
    return JSValue::encode(baseValue.get(exec, ident, slot));
}

EncodedJSValue DFG_OPERATION operationGetArgumentByVal(ExecState* exec, int32_t argumentsRegister, int32_t index)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    JSValue argumentsValue = exec->uncheckedR(argumentsRegister).jsValue();
    
    // If there are no arguments, and we're accessing out of bounds, then we have to create the
    // arguments in case someone has installed a getter on a numeric property.
    if (!argumentsValue)
        exec->uncheckedR(argumentsRegister) = argumentsValue = Arguments::create(exec->globalData(), exec);
    
    return JSValue::encode(argumentsValue.get(exec, index));
}

EncodedJSValue DFG_OPERATION operationGetInlinedArgumentByVal(
    ExecState* exec, int32_t argumentsRegister, InlineCallFrame* inlineCallFrame, int32_t index)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    JSValue argumentsValue = exec->uncheckedR(argumentsRegister).jsValue();
    
    // If there are no arguments, and we're accessing out of bounds, then we have to create the
    // arguments in case someone has installed a getter on a numeric property.
    if (!argumentsValue) {
        exec->uncheckedR(argumentsRegister) = argumentsValue =
            Arguments::create(exec->globalData(), exec, inlineCallFrame);
    }
    
    return JSValue::encode(argumentsValue.get(exec, index));
}

JSCell* DFG_OPERATION operationNewFunction(ExecState* exec, JSCell* functionExecutable)
{
    ASSERT(functionExecutable->inherits(&FunctionExecutable::s_info));
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);
    return JSFunction::create(exec, static_cast<FunctionExecutable*>(functionExecutable), exec->scope());
}

JSCell* DFG_OPERATION operationNewFunctionExpression(ExecState* exec, JSCell* functionExecutableAsCell)
{
    ASSERT(functionExecutableAsCell->inherits(&FunctionExecutable::s_info));

    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    FunctionExecutable* functionExecutable =
        static_cast<FunctionExecutable*>(functionExecutableAsCell);
    return JSFunction::create(exec, functionExecutable, exec->scope());
}

size_t DFG_OPERATION operationIsObject(ExecState* exec, EncodedJSValue value)
{
    return jsIsObjectType(exec, JSValue::decode(value));
}

size_t DFG_OPERATION operationIsFunction(EncodedJSValue value)
{
    return jsIsFunctionType(JSValue::decode(value));
}

void DFG_OPERATION operationReallocateStorageAndFinishPut(ExecState* exec, JSObject* base, Structure* structure, PropertyOffset offset, EncodedJSValue value)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    ASSERT(structure->outOfLineCapacity() > base->structure()->outOfLineCapacity());
    ASSERT(!globalData.heap.storageAllocator().fastPathShouldSucceed(structure->outOfLineCapacity() * sizeof(JSValue)));
    base->setStructureAndReallocateStorageIfNecessary(globalData, structure);
    base->putDirectOffset(globalData, offset, JSValue::decode(value));
}

char* DFG_OPERATION operationAllocatePropertyStorageWithInitialCapacity(ExecState* exec)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    return reinterpret_cast<char*>(
        Butterfly::createUninitialized(globalData, 0, initialOutOfLineCapacity, false, 0));
}

char* DFG_OPERATION operationAllocatePropertyStorage(ExecState* exec, size_t newSize)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    return reinterpret_cast<char*>(
        Butterfly::createUninitialized(globalData, 0, newSize, false, 0));
}

char* DFG_OPERATION operationReallocateButterflyToHavePropertyStorageWithInitialCapacity(ExecState* exec, JSObject* object)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    ASSERT(!object->structure()->outOfLineCapacity());
    Butterfly* result = object->growOutOfLineStorage(globalData, 0, initialOutOfLineCapacity);
    object->setButterflyWithoutChangingStructure(result);
    return reinterpret_cast<char*>(result);
}

char* DFG_OPERATION operationReallocateButterflyToGrowPropertyStorage(ExecState* exec, JSObject* object, size_t newSize)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    Butterfly* result = object->growOutOfLineStorage(globalData, object->structure()->outOfLineCapacity(), newSize);
    object->setButterflyWithoutChangingStructure(result);
    return reinterpret_cast<char*>(result);
}

char* DFG_OPERATION operationEnsureArrayStorage(ExecState* exec, JSObject* object)
{
    JSGlobalData& globalData = exec->globalData();
    NativeCallFrameTracer tracer(&globalData, exec);

    return reinterpret_cast<char*>(object->ensureArrayStorage(globalData));
}

double DFG_OPERATION operationFModOnInts(int32_t a, int32_t b)
{
    return fmod(a, b);
}

DFGHandlerEncoded DFG_OPERATION lookupExceptionHandler(ExecState* exec, uint32_t callIndex)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    JSValue exceptionValue = exec->exception();
    ASSERT(exceptionValue);
    
    unsigned vPCIndex = exec->codeBlock()->bytecodeOffsetForCallAtIndex(callIndex);
    ExceptionHandler handler = genericThrow(globalData, exec, exceptionValue, vPCIndex);
    ASSERT(handler.catchRoutine);
    return dfgHandlerEncoded(handler.callFrame, handler.catchRoutine);
}

DFGHandlerEncoded DFG_OPERATION lookupExceptionHandlerInStub(ExecState* exec, StructureStubInfo* stubInfo)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);

    JSValue exceptionValue = exec->exception();
    ASSERT(exceptionValue);
    
    CodeOrigin codeOrigin = stubInfo->codeOrigin;
    while (codeOrigin.inlineCallFrame)
        codeOrigin = codeOrigin.inlineCallFrame->caller;
    
    ExceptionHandler handler = genericThrow(globalData, exec, exceptionValue, codeOrigin.bytecodeIndex);
    ASSERT(handler.catchRoutine);
    return dfgHandlerEncoded(handler.callFrame, handler.catchRoutine);
}

double DFG_OPERATION dfgConvertJSValueToNumber(ExecState* exec, EncodedJSValue value)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::decode(value).toNumber(exec);
}

size_t DFG_OPERATION dfgConvertJSValueToInt32(ExecState* exec, EncodedJSValue value)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    // toInt32/toUInt32 return the same value; we want the value zero extended to fill the register.
    return JSValue::decode(value).toUInt32(exec);
}

size_t DFG_OPERATION dfgConvertJSValueToBoolean(ExecState* exec, EncodedJSValue encodedOp)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    return JSValue::decode(encodedOp).toBoolean(exec);
}

#if DFG_ENABLE(VERBOSE_SPECULATION_FAILURE)
void DFG_OPERATION debugOperationPrintSpeculationFailure(ExecState* exec, void* debugInfoRaw)
{
    JSGlobalData* globalData = &exec->globalData();
    NativeCallFrameTracer tracer(globalData, exec);
    
    SpeculationFailureDebugInfo* debugInfo = static_cast<SpeculationFailureDebugInfo*>(debugInfoRaw);
    CodeBlock* codeBlock = debugInfo->codeBlock;
    CodeBlock* alternative = codeBlock->alternative();
    dataLog("Speculation failure in %p at @%u with executeCounter = %s, "
            "reoptimizationRetryCounter = %u, optimizationDelayCounter = %u, "
            "osrExitCounter = %u\n",
            codeBlock,
            debugInfo->nodeIndex,
            alternative ? alternative->jitExecuteCounter().status() : 0,
            alternative ? alternative->reoptimizationRetryCounter() : 0,
            alternative ? alternative->optimizationDelayCounter() : 0,
            codeBlock->osrExitCounter());
}
#endif

extern "C" void DFG_OPERATION triggerReoptimizationNow(CodeBlock* codeBlock)
{
#if ENABLE(JIT_VERBOSE_OSR)
    dataLog("%p: Entered reoptimize\n", codeBlock);
#endif
    // We must be called with the baseline code block.
    ASSERT(JITCode::isBaselineCode(codeBlock->getJITType()));

    // If I am my own replacement, then reoptimization has already been triggered.
    // This can happen in recursive functions.
    if (codeBlock->replacement() == codeBlock)
        return;

    // Otherwise, the replacement must be optimized code. Use this as an opportunity
    // to check our logic.
    ASSERT(codeBlock->hasOptimizedReplacement());
    ASSERT(codeBlock->replacement()->getJITType() == JITCode::DFGJIT);

    codeBlock->reoptimize();
}

} // extern "C"
} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#if COMPILER(GCC)

namespace JSC {

#if CPU(X86_64)
asm (
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "mov -40(%r13), %r13\n"
    "mov %r13, %rdi\n"
    "jmp " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);
#elif CPU(X86)
asm (
".text" "\n" \
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "mov -40(%edi), %edi\n"
    "mov %edi, 4(%esp)\n"
    "jmp " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);
#elif CPU(ARM_THUMB2)
asm (
".text" "\n"
".align 2" "\n"
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
".thumb" "\n"
".thumb_func " THUMB_FUNC_PARAM(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "ldr r5, [r5, #-40]" "\n"
    "mov r0, r5" "\n"
    "b " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);
#elif CPU(ARM_TRADITIONAL)
asm (
".text" "\n"
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
INLINE_ARM_FUNCTION(getHostCallReturnValue)
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "ldr r5, [r5, #-40]" "\n"
    "mov r0, r5" "\n"
    "b " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);
#endif

extern "C" EncodedJSValue HOST_CALL_RETURN_VALUE_OPTION getHostCallReturnValueWithExecState(ExecState* exec)
{
    if (!exec)
        return JSValue::encode(JSValue());
    return JSValue::encode(exec->globalData().hostCallReturnValue);
}

} // namespace JSC

#endif // COMPILER(GCC)

#endif // ENABLE(JIT)
