/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich (cwzwarich@uwaterloo.ca)
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

#include "config.h"
#include "JSGlobalObject.h"

#include "JSCallbackConstructor.h"
#include "JSCallbackFunction.h"
#include "JSCallbackObject.h"

#include "Arguments.h"
#include "ArrayConstructor.h"
#include "ArrayPrototype.h"
#include "BooleanConstructor.h"
#include "BooleanPrototype.h"
#include "CodeBlock.h"
#include "DateConstructor.h"
#include "DatePrototype.h"
#include "ErrorConstructor.h"
#include "ErrorPrototype.h"
#include "FunctionConstructor.h"
#include "FunctionPrototype.h"
#include "JSFunction.h"
#include "JSGlobalObjectFunctions.h"
#include "JSLock.h"
#include "JSONObject.h"
#include "Interpreter.h"
#include "Lookup.h"
#include "MathObject.h"
#include "NativeErrorConstructor.h"
#include "NativeErrorPrototype.h"
#include "NumberConstructor.h"
#include "NumberPrototype.h"
#include "ObjectConstructor.h"
#include "ObjectPrototype.h"
#include "Profiler.h"
#include "RegExpConstructor.h"
#include "RegExpMatchesArray.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "ScopeChainMark.h"
#include "StringConstructor.h"
#include "StringPrototype.h"
#include "Debugger.h"

#include "JSGlobalObject.lut.h"

namespace JSC {

const ClassInfo JSGlobalObject::s_info = { "GlobalObject", &JSVariableObject::s_info, 0, ExecState::globalObjectTable };

/* Source for JSGlobalObject.lut.h
@begin globalObjectTable
  parseInt              globalFuncParseInt              DontEnum|Function 2
  parseFloat            globalFuncParseFloat            DontEnum|Function 1
  isNaN                 globalFuncIsNaN                 DontEnum|Function 1
  isFinite              globalFuncIsFinite              DontEnum|Function 1
  escape                globalFuncEscape                DontEnum|Function 1
  unescape              globalFuncUnescape              DontEnum|Function 1
  decodeURI             globalFuncDecodeURI             DontEnum|Function 1
  decodeURIComponent    globalFuncDecodeURIComponent    DontEnum|Function 1
  encodeURI             globalFuncEncodeURI             DontEnum|Function 1
  encodeURIComponent    globalFuncEncodeURIComponent    DontEnum|Function 1
@end
*/

ASSERT_CLASS_FITS_IN_CELL(JSGlobalObject);

// Default number of ticks before a timeout check should be done.
static const int initialTickCountThreshold = 255;

// Preferred number of milliseconds between each timeout check
static const int preferredScriptCheckTimeInterval = 1000;

template <typename T> static inline void visitIfNeeded(SlotVisitor& visitor, WriteBarrier<T>* v)
{
    if (*v)
        visitor.append(v);
}

JSGlobalObject::~JSGlobalObject()
{
    ASSERT(JSLock::currentThreadIsHoldingLock());

    if (m_debugger)
        m_debugger->detach(this);

    Profiler** profiler = Profiler::enabledProfilerReference();
    if (UNLIKELY(*profiler != 0)) {
        (*profiler)->stopProfiling(this);
    }
}

void JSGlobalObject::init(JSObject* thisValue)
{
    ASSERT(JSLock::currentThreadIsHoldingLock());
    
    structure()->disableSpecificFunctionTracking();

    m_globalData = Heap::heap(this)->globalData();
    m_globalScopeChain.set(*m_globalData, this, ScopeChainNode::create(0, this, m_globalData.get(), this, thisValue));

    JSGlobalObject::globalExec()->init(0, 0, m_globalScopeChain.get(), CallFrame::noCaller(), 0, 0);

    m_debugger = 0;

    reset(prototype());
}

void JSGlobalObject::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (symbolTablePut(exec->globalData(), propertyName, value))
        return;
    JSVariableObject::put(exec, propertyName, value, slot);
}

void JSGlobalObject::putWithAttributes(ExecState* exec, const Identifier& propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (symbolTablePutWithAttributes(exec->globalData(), propertyName, value, attributes))
        return;

    JSValue valueBefore = getDirect(exec->globalData(), propertyName);
    PutPropertySlot slot;
    JSVariableObject::put(exec, propertyName, value, slot);
    if (!valueBefore) {
        JSValue valueAfter = getDirect(exec->globalData(), propertyName);
        if (valueAfter)
            JSObject::putWithAttributes(exec, propertyName, valueAfter, attributes);
    }
}

void JSGlobalObject::defineGetter(ExecState* exec, const Identifier& propertyName, JSObject* getterFunc, unsigned attributes)
{
    PropertySlot slot;
    if (!symbolTableGet(propertyName, slot))
        JSVariableObject::defineGetter(exec, propertyName, getterFunc, attributes);
}

void JSGlobalObject::defineSetter(ExecState* exec, const Identifier& propertyName, JSObject* setterFunc, unsigned attributes)
{
    PropertySlot slot;
    if (!symbolTableGet(propertyName, slot))
        JSVariableObject::defineSetter(exec, propertyName, setterFunc, attributes);
}

static inline JSObject* lastInPrototypeChain(JSObject* object)
{
    JSObject* o = object;
    while (o->prototype().isObject())
        o = asObject(o->prototype());
    return o;
}

void JSGlobalObject::reset(JSValue prototype)
{
    ExecState* exec = JSGlobalObject::globalExec();

    m_functionPrototype.set(exec->globalData(), this, FunctionPrototype::create(exec, this, FunctionPrototype::createStructure(exec->globalData(), jsNull()))); // The real prototype will be set once ObjectPrototype is created.
    m_functionStructure.set(exec->globalData(), this, JSFunction::createStructure(exec->globalData(), m_functionPrototype.get()));
    m_namedFunctionStructure.set(exec->globalData(), this, Structure::addPropertyTransition(exec->globalData(), m_functionStructure.get(), exec->globalData().propertyNames->name, DontDelete | ReadOnly | DontEnum, 0, m_functionNameOffset));
    m_internalFunctionStructure.set(exec->globalData(), this, InternalFunction::createStructure(exec->globalData(), m_functionPrototype.get()));
    JSFunction* callFunction = 0;
    JSFunction* applyFunction = 0;
    m_functionPrototype->addFunctionProperties(exec, this, m_functionStructure.get(), &callFunction, &applyFunction);
    m_callFunction.set(exec->globalData(), this, callFunction);
    m_applyFunction.set(exec->globalData(), this, applyFunction);
    m_objectPrototype.set(exec->globalData(), this, ObjectPrototype::create(exec, this, ObjectPrototype::createStructure(exec->globalData(), jsNull())));
    m_functionPrototype->structure()->setPrototypeWithoutTransition(exec->globalData(), m_objectPrototype.get());

    m_emptyObjectStructure.set(exec->globalData(), this, m_objectPrototype->inheritorID(exec->globalData()));
    m_nullPrototypeObjectStructure.set(exec->globalData(), this, createEmptyObjectStructure(exec->globalData(), jsNull()));

    m_callbackFunctionStructure.set(exec->globalData(), this, JSCallbackFunction::createStructure(exec->globalData(), m_functionPrototype.get()));
    m_argumentsStructure.set(exec->globalData(), this, Arguments::createStructure(exec->globalData(), m_objectPrototype.get()));
    m_callbackConstructorStructure.set(exec->globalData(), this, JSCallbackConstructor::createStructure(exec->globalData(), m_objectPrototype.get()));
    m_callbackObjectStructure.set(exec->globalData(), this, JSCallbackObject<JSObjectWithGlobalObject>::createStructure(exec->globalData(), m_objectPrototype.get()));

    m_arrayPrototype.set(exec->globalData(), this, ArrayPrototype::create(exec, this, ArrayPrototype::createStructure(exec->globalData(), m_objectPrototype.get())));
    m_arrayStructure.set(exec->globalData(), this, JSArray::createStructure(exec->globalData(), m_arrayPrototype.get()));
    m_regExpMatchesArrayStructure.set(exec->globalData(), this, RegExpMatchesArray::createStructure(exec->globalData(), m_arrayPrototype.get()));

    m_stringPrototype.set(exec->globalData(), this, StringPrototype::create(exec, this, StringPrototype::createStructure(exec->globalData(), m_objectPrototype.get())));
    m_stringObjectStructure.set(exec->globalData(), this, StringObject::createStructure(exec->globalData(), m_stringPrototype.get()));

    m_booleanPrototype.set(exec->globalData(), this, BooleanPrototype::create(exec, this, BooleanPrototype::createStructure(exec->globalData(), m_objectPrototype.get())));
    m_booleanObjectStructure.set(exec->globalData(), this, BooleanObject::createStructure(exec->globalData(), m_booleanPrototype.get()));

    m_numberPrototype.set(exec->globalData(), this, NumberPrototype::create(exec, this, NumberPrototype::createStructure(exec->globalData(), m_objectPrototype.get())));
    m_numberObjectStructure.set(exec->globalData(), this, NumberObject::createStructure(exec->globalData(), m_numberPrototype.get()));

    m_datePrototype.set(exec->globalData(), this, DatePrototype::create(exec, this, DatePrototype::createStructure(exec->globalData(), m_objectPrototype.get())));
    m_dateStructure.set(exec->globalData(), this, DateInstance::createStructure(exec->globalData(), m_datePrototype.get()));

    RegExp* emptyRegex = RegExp::create(exec->globalData(), "", NoFlags);
    
    m_regExpPrototype.set(exec->globalData(), this, RegExpPrototype::create(exec, this, RegExpPrototype::createStructure(exec->globalData(), m_objectPrototype.get()), emptyRegex));
    m_regExpStructure.set(exec->globalData(), this, RegExpObject::createStructure(exec->globalData(), m_regExpPrototype.get()));

    m_methodCallDummy.set(exec->globalData(), this, constructEmptyObject(exec));

    ErrorPrototype* errorPrototype = ErrorPrototype::create(exec, this, ErrorPrototype::createStructure(exec->globalData(), m_objectPrototype.get()));
    m_errorStructure.set(exec->globalData(), this, ErrorInstance::createStructure(exec->globalData(), errorPrototype));

    // Constructors

    JSCell* objectConstructor = ObjectConstructor::create(exec, this, ObjectConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_objectPrototype.get());
    JSCell* functionConstructor = FunctionConstructor::create(exec, this, FunctionConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_functionPrototype.get());
    JSCell* arrayConstructor = ArrayConstructor::create(exec, this, ArrayConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_arrayPrototype.get());
    JSCell* stringConstructor = StringConstructor::create(exec, this, StringConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_stringPrototype.get());
    JSCell* booleanConstructor = BooleanConstructor::create(exec, this, BooleanConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_booleanPrototype.get());
    JSCell* numberConstructor = NumberConstructor::create(exec, this, NumberConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_numberPrototype.get());
    JSCell* dateConstructor = DateConstructor::create(exec, this, DateConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_datePrototype.get());

    m_regExpConstructor.set(exec->globalData(), this, RegExpConstructor::create(exec, this, RegExpConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), m_regExpPrototype.get()));

    m_errorConstructor.set(exec->globalData(), this, ErrorConstructor::create(exec, this, ErrorConstructor::createStructure(exec->globalData(), m_functionPrototype.get()), errorPrototype));

    Structure* nativeErrorPrototypeStructure = NativeErrorPrototype::createStructure(exec->globalData(), errorPrototype);
    Structure* nativeErrorStructure = NativeErrorConstructor::createStructure(exec->globalData(), m_functionPrototype.get());
    m_evalErrorConstructor.set(exec->globalData(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, "EvalError"));
    m_rangeErrorConstructor.set(exec->globalData(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, "RangeError"));
    m_referenceErrorConstructor.set(exec->globalData(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, "ReferenceError"));
    m_syntaxErrorConstructor.set(exec->globalData(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, "SyntaxError"));
    m_typeErrorConstructor.set(exec->globalData(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, "TypeError"));
    m_URIErrorConstructor.set(exec->globalData(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, "URIError"));

    m_objectPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, objectConstructor, DontEnum);
    m_functionPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, functionConstructor, DontEnum);
    m_arrayPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, arrayConstructor, DontEnum);
    m_booleanPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, booleanConstructor, DontEnum);
    m_stringPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, stringConstructor, DontEnum);
    m_numberPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, numberConstructor, DontEnum);
    m_datePrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, dateConstructor, DontEnum);
    m_regExpPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, m_regExpConstructor.get(), DontEnum);
    errorPrototype->putDirectFunctionWithoutTransition(exec->globalData(), exec->propertyNames().constructor, m_errorConstructor.get(), DontEnum);

    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "Object"), objectConstructor, DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "Function"), functionConstructor, DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "Array"), arrayConstructor, DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "Boolean"), booleanConstructor, DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "String"), stringConstructor, DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "Number"), numberConstructor, DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "Date"), dateConstructor, DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "RegExp"), m_regExpConstructor.get(), DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "Error"), m_errorConstructor.get(), DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "EvalError"), m_evalErrorConstructor.get(), DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "RangeError"), m_rangeErrorConstructor.get(), DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "ReferenceError"), m_referenceErrorConstructor.get(), DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "SyntaxError"), m_syntaxErrorConstructor.get(), DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "TypeError"), m_typeErrorConstructor.get(), DontEnum);
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, "URIError"), m_URIErrorConstructor.get(), DontEnum);

    m_evalFunction.set(exec->globalData(), this, JSFunction::create(exec, this, m_functionStructure.get(), 1, exec->propertyNames().eval, globalFuncEval));
    putDirectFunctionWithoutTransition(exec, m_evalFunction.get(), DontEnum);

    GlobalPropertyInfo staticGlobals[] = {
        GlobalPropertyInfo(Identifier(exec, "Math"), MathObject::create(exec, this, MathObject::createStructure(exec->globalData(), m_objectPrototype.get())), DontEnum | DontDelete),
        GlobalPropertyInfo(Identifier(exec, "NaN"), jsNaN(), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(Identifier(exec, "Infinity"), jsNumber(std::numeric_limits<double>::infinity()), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(Identifier(exec, "undefined"), jsUndefined(), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(Identifier(exec, "JSON"), JSONObject::create(exec, this, JSONObject::createStructure(exec->globalData(), m_objectPrototype.get())), DontEnum | DontDelete)
    };
    addStaticGlobals(staticGlobals, WTF_ARRAY_LENGTH(staticGlobals));

    resetPrototype(exec->globalData(), prototype);
}

// Set prototype, and also insert the object prototype at the end of the chain.
void JSGlobalObject::resetPrototype(JSGlobalData& globalData, JSValue prototype)
{
    setPrototype(globalData, prototype);

    JSObject* oldLastInPrototypeChain = lastInPrototypeChain(this);
    JSObject* objectPrototype = m_objectPrototype.get();
    if (oldLastInPrototypeChain != objectPrototype)
        oldLastInPrototypeChain->setPrototype(globalData, objectPrototype);
}

void JSGlobalObject::visitChildren(SlotVisitor& visitor)
{
    ASSERT_GC_OBJECT_INHERITS(this, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(structure()->typeInfo().overridesVisitChildren());
    JSVariableObject::visitChildren(visitor);

    visitIfNeeded(visitor, &m_globalScopeChain);
    visitIfNeeded(visitor, &m_methodCallDummy);

    visitIfNeeded(visitor, &m_regExpConstructor);
    visitIfNeeded(visitor, &m_errorConstructor);
    visitIfNeeded(visitor, &m_evalErrorConstructor);
    visitIfNeeded(visitor, &m_rangeErrorConstructor);
    visitIfNeeded(visitor, &m_referenceErrorConstructor);
    visitIfNeeded(visitor, &m_syntaxErrorConstructor);
    visitIfNeeded(visitor, &m_typeErrorConstructor);
    visitIfNeeded(visitor, &m_URIErrorConstructor);

    visitIfNeeded(visitor, &m_evalFunction);
    visitIfNeeded(visitor, &m_callFunction);
    visitIfNeeded(visitor, &m_applyFunction);

    visitIfNeeded(visitor, &m_objectPrototype);
    visitIfNeeded(visitor, &m_functionPrototype);
    visitIfNeeded(visitor, &m_arrayPrototype);
    visitIfNeeded(visitor, &m_booleanPrototype);
    visitIfNeeded(visitor, &m_stringPrototype);
    visitIfNeeded(visitor, &m_numberPrototype);
    visitIfNeeded(visitor, &m_datePrototype);
    visitIfNeeded(visitor, &m_regExpPrototype);

    visitIfNeeded(visitor, &m_argumentsStructure);
    visitIfNeeded(visitor, &m_arrayStructure);
    visitIfNeeded(visitor, &m_booleanObjectStructure);
    visitIfNeeded(visitor, &m_callbackConstructorStructure);
    visitIfNeeded(visitor, &m_callbackFunctionStructure);
    visitIfNeeded(visitor, &m_callbackObjectStructure);
    visitIfNeeded(visitor, &m_dateStructure);
    visitIfNeeded(visitor, &m_emptyObjectStructure);
    visitIfNeeded(visitor, &m_nullPrototypeObjectStructure);
    visitIfNeeded(visitor, &m_errorStructure);
    visitIfNeeded(visitor, &m_functionStructure);
    visitIfNeeded(visitor, &m_namedFunctionStructure);
    visitIfNeeded(visitor, &m_numberObjectStructure);
    visitIfNeeded(visitor, &m_regExpMatchesArrayStructure);
    visitIfNeeded(visitor, &m_regExpStructure);
    visitIfNeeded(visitor, &m_stringObjectStructure);
    visitIfNeeded(visitor, &m_internalFunctionStructure);

    if (m_registerArray) {
        // Outside the execution of global code, when our variables are torn off,
        // we can mark the torn-off array.
        visitor.appendValues(m_registerArray.get(), m_registerArraySize);
    } else if (m_registers) {
        // During execution of global code, when our variables are in the register file,
        // the symbol table tells us how many variables there are, and registers
        // points to where they end, and the registers used for execution begin.
        visitor.appendValues(m_registers - symbolTable().size(), symbolTable().size());
    }
}

ExecState* JSGlobalObject::globalExec()
{
    return CallFrame::create(m_globalCallFrame + RegisterFile::CallFrameHeaderSize);
}

bool JSGlobalObject::isDynamicScope(bool&) const
{
    return true;
}

void JSGlobalObject::resizeRegisters(size_t newSize)
{
    // Previous duplicate symbols may have created spare capacity in m_registerArray.
    if (newSize <= m_registerArraySize)
        return;

    size_t oldSize = m_registerArraySize;
    OwnArrayPtr<WriteBarrier<Unknown> > registerArray = adoptArrayPtr(new WriteBarrier<Unknown>[newSize]);
    for (size_t i = 0; i < oldSize; ++i)
        registerArray[i].set(globalData(), this, m_registerArray[i].get());
    for (size_t i = oldSize; i < newSize; ++i)
        registerArray[i].setUndefined();

    WriteBarrier<Unknown>* registers = registerArray.get();
    setRegisters(registers, registerArray.release(), newSize);
}

void JSGlobalObject::addStaticGlobals(GlobalPropertyInfo* globals, int count)
{
    resizeRegisters(symbolTable().size() + count);

    for (int i = 0; i < count; ++i) {
        GlobalPropertyInfo& global = globals[i];
        ASSERT(global.attributes & DontDelete);
        
        int index = symbolTable().size();
        SymbolTableEntry newEntry(index, global.attributes);
        symbolTable().add(global.identifier.impl(), newEntry);
        registerAt(index).set(globalData(), this, global.value);
    }
}

bool JSGlobalObject::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (getStaticFunctionSlot<JSVariableObject>(exec, ExecState::globalObjectTable(exec), this, propertyName, slot))
        return true;
    return symbolTableGet(propertyName, slot);
}

bool JSGlobalObject::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (getStaticFunctionDescriptor<JSVariableObject>(exec, ExecState::globalObjectTable(exec), this, propertyName, descriptor))
        return true;
    return symbolTableGet(propertyName, descriptor);
}

void JSGlobalObject::WeakMapsFinalizer::finalize(Handle<Unknown> handle, void*)
{
    JSGlobalObject* globalObject = asGlobalObject(handle.get());
    globalObject->m_rareData.clear();
}

JSGlobalObject::WeakMapsFinalizer* JSGlobalObject::weakMapsFinalizer()
{
    static WeakMapsFinalizer* finalizer = new WeakMapsFinalizer();
    return finalizer;
}

DynamicGlobalObjectScope::DynamicGlobalObjectScope(JSGlobalData& globalData, JSGlobalObject* dynamicGlobalObject)
    : m_dynamicGlobalObjectSlot(globalData.dynamicGlobalObject)
    , m_savedDynamicGlobalObject(m_dynamicGlobalObjectSlot)
{
    if (!m_dynamicGlobalObjectSlot) {
#if ENABLE(ASSEMBLER)
        if (ExecutableAllocator::underMemoryPressure())
            globalData.recompileAllJSFunctions();
#endif

        m_dynamicGlobalObjectSlot = dynamicGlobalObject;

        // Reset the date cache between JS invocations to force the VM
        // to observe time zone changes.
        globalData.resetDateCache();
    }
}

void slowValidateCell(JSGlobalObject* globalObject)
{
    if (!globalObject->isGlobalObject())
        CRASH();
    ASSERT_GC_OBJECT_INHERITS(globalObject, &JSGlobalObject::s_info);
}

} // namespace JSC
