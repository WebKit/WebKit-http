/*
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *  Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef JSGlobalObject_h
#define JSGlobalObject_h

#include "ArrayAllocationProfile.h"
#include "JSArray.h"
#include "JSArrayBufferPrototype.h"
#include "JSClassRef.h"
#include "JSSegmentedVariableObject.h"
#include "JSWeakObjectMapRefInternal.h"
#include "NumberPrototype.h"
#include "SpecialPointer.h"
#include "StringPrototype.h"
#include "StructureChain.h"
#include "StructureRareDataInlines.h"
#include "VM.h"
#include "Watchpoint.h"
#include <JavaScriptCore/JSBase.h>
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RandomNumber.h>

struct OpaqueJSClass;
struct OpaqueJSClassContextData;

namespace JSC {

class ArrayPrototype;
class BooleanPrototype;
class Debugger;
class ErrorConstructor;
class ErrorPrototype;
class EvalCodeBlock;
class EvalExecutable;
class FunctionCodeBlock;
class FunctionExecutable;
class FunctionPrototype;
class GetterSetter;
class GlobalCodeBlock;
class JSPromisePrototype;
class JSPromiseResolverPrototype;
class JSStack;
class LLIntOffsetsExtractor;
class NativeErrorConstructor;
class ProgramCodeBlock;
class ProgramExecutable;
class RegExpConstructor;
class RegExpPrototype;
class SourceCode;
struct ActivationStackNode;
struct HashTable;

#define FOR_EACH_SIMPLE_BUILTIN_TYPE(macro) \
    macro(Set, set, set, JSSet, Set) \
    macro(Map, map, map, JSMap, Map) \
    macro(Date, date, date, DateInstance, Date) \
    macro(String, string, stringObject, StringObject, String) \
    macro(Boolean, boolean, booleanObject, BooleanObject, Boolean) \
    macro(Number, number, numberObject, NumberObject, Number) \
    macro(Error, error, error, ErrorInstance, Error) \
    macro(JSArrayBuffer, arrayBuffer, arrayBuffer, JSArrayBuffer, ArrayBuffer) \
    macro(WeakMap, weakMap, weakMap, JSWeakMap, WeakMap) \
    macro(ArrayIterator, arrayIterator, arrayIterator, JSArrayIterator, ArrayIterator) \

#define DECLARE_SIMPLE_BUILTIN_TYPE(capitalName, lowerName, properName, instanceType, jsName) \
    class JS ## capitalName; \
    class capitalName ## Prototype; \
    class capitalName ## Constructor;

FOR_EACH_SIMPLE_BUILTIN_TYPE(DECLARE_SIMPLE_BUILTIN_TYPE)

#undef DECLARE_SIMPLE_BUILTIN_TYPE

typedef Vector<ExecState*, 16> ExecStateStack;

class TaskContext : public RefCounted<TaskContext> {
public:
    virtual ~TaskContext()
    {
    }
};

struct GlobalObjectMethodTable {
    typedef bool (*AllowsAccessFromFunctionPtr)(const JSGlobalObject*, ExecState*);
    AllowsAccessFromFunctionPtr allowsAccessFrom;

    typedef bool (*SupportsProfilingFunctionPtr)(const JSGlobalObject*); 
    SupportsProfilingFunctionPtr supportsProfiling;

    typedef bool (*SupportsRichSourceInfoFunctionPtr)(const JSGlobalObject*);
    SupportsRichSourceInfoFunctionPtr supportsRichSourceInfo;

    typedef bool (*ShouldInterruptScriptFunctionPtr)(const JSGlobalObject*);
    ShouldInterruptScriptFunctionPtr shouldInterruptScript;

    typedef bool (*JavaScriptExperimentsEnabledFunctionPtr)(const JSGlobalObject*);
    JavaScriptExperimentsEnabledFunctionPtr javaScriptExperimentsEnabled;

    typedef void (*QueueTaskToEventLoopCallbackFunctionPtr)(ExecState*, TaskContext*);
    typedef void (*QueueTaskToEventLoopFunctionPtr)(const JSGlobalObject*, QueueTaskToEventLoopCallbackFunctionPtr, PassRefPtr<TaskContext>);
    QueueTaskToEventLoopFunctionPtr queueTaskToEventLoop;

    typedef bool (*ShouldInterruptScriptBeforeTimeoutPtr)(const JSGlobalObject*);
    ShouldInterruptScriptBeforeTimeoutPtr shouldInterruptScriptBeforeTimeout;
};

class JSGlobalObject : public JSSegmentedVariableObject {
private:
    typedef HashSet<RefPtr<OpaqueJSWeakObjectMap>> WeakMapSet;
    typedef HashMap<OpaqueJSClass*, std::unique_ptr<OpaqueJSClassContextData>> OpaqueJSClassDataMap;

    struct JSGlobalObjectRareData {
        JSGlobalObjectRareData()
            : profileGroup(0)
        {
        }

        WeakMapSet weakMaps;
        unsigned profileGroup;
        
        OpaqueJSClassDataMap opaqueJSClassData;
    };

protected:

    Register m_globalCallFrame[JSStack::CallFrameHeaderSize];

    WriteBarrier<JSObject> m_globalThis;

    WriteBarrier<RegExpConstructor> m_regExpConstructor;
    WriteBarrier<ErrorConstructor> m_errorConstructor;
    WriteBarrier<NativeErrorConstructor> m_evalErrorConstructor;
    WriteBarrier<NativeErrorConstructor> m_rangeErrorConstructor;
    WriteBarrier<NativeErrorConstructor> m_referenceErrorConstructor;
    WriteBarrier<NativeErrorConstructor> m_syntaxErrorConstructor;
    WriteBarrier<NativeErrorConstructor> m_typeErrorConstructor;
    WriteBarrier<NativeErrorConstructor> m_URIErrorConstructor;

    WriteBarrier<JSFunction> m_evalFunction;
    WriteBarrier<JSFunction> m_callFunction;
    WriteBarrier<JSFunction> m_applyFunction;
    WriteBarrier<GetterSetter> m_throwTypeErrorGetterSetter;

    WriteBarrier<ObjectPrototype> m_objectPrototype;
    WriteBarrier<FunctionPrototype> m_functionPrototype;
    WriteBarrier<ArrayPrototype> m_arrayPrototype;
    WriteBarrier<RegExpPrototype> m_regExpPrototype;
    WriteBarrier<JSPromisePrototype> m_promisePrototype;
    WriteBarrier<JSPromiseResolverPrototype> m_promiseResolverPrototype;

    WriteBarrier<Structure> m_withScopeStructure;
    WriteBarrier<Structure> m_strictEvalActivationStructure;
    WriteBarrier<Structure> m_activationStructure;
    WriteBarrier<Structure> m_nameScopeStructure;
    WriteBarrier<Structure> m_argumentsStructure;
        
    // Lists the actual structures used for having these particular indexing shapes.
    WriteBarrier<Structure> m_originalArrayStructureForIndexingShape[NumberOfIndexingShapes];
    // Lists the structures we should use during allocation for these particular indexing shapes.
    WriteBarrier<Structure> m_arrayStructureForIndexingShapeDuringAllocation[NumberOfIndexingShapes];

    WriteBarrier<Structure> m_callbackConstructorStructure;
    WriteBarrier<Structure> m_callbackFunctionStructure;
    WriteBarrier<Structure> m_callbackObjectStructure;
#if JSC_OBJC_API_ENABLED
    WriteBarrier<Structure> m_objcCallbackFunctionStructure;
    WriteBarrier<Structure> m_objcWrapperObjectStructure;
#endif
    WriteBarrier<Structure> m_nullPrototypeObjectStructure;
    WriteBarrier<Structure> m_functionStructure;
    WriteBarrier<Structure> m_boundFunctionStructure;
    WriteBarrier<Structure> m_namedFunctionStructure;
    PropertyOffset m_functionNameOffset;
    WriteBarrier<Structure> m_privateNameStructure;
    WriteBarrier<Structure> m_regExpMatchesArrayStructure;
    WriteBarrier<Structure> m_regExpStructure;
    WriteBarrier<Structure> m_internalFunctionStructure;
    
    WriteBarrier<Structure> m_iteratorResultStructure;

#if ENABLE(PROMISES)
    WriteBarrier<Structure> m_promiseStructure;
    WriteBarrier<Structure> m_promiseResolverStructure;
    WriteBarrier<Structure> m_promiseCallbackStructure;
    WriteBarrier<Structure> m_promiseWrapperCallbackStructure;
#endif // ENABLE(PROMISES)

#define DEFINE_STORAGE_FOR_SIMPLE_TYPE(capitalName, lowerName, properName, instanceType, jsName) \
    WriteBarrier<capitalName ## Prototype> m_ ## lowerName ## Prototype; \
    WriteBarrier<Structure> m_ ## properName ## Structure;

    FOR_EACH_SIMPLE_BUILTIN_TYPE(DEFINE_STORAGE_FOR_SIMPLE_TYPE)

#undef DEFINE_STORAGE_FOR_SIMPLE_TYPE

    struct TypedArrayData {
        WriteBarrier<JSObject> prototype;
        WriteBarrier<Structure> structure;
    };
    
    FixedArray<TypedArrayData, NUMBER_OF_TYPED_ARRAY_TYPES> m_typedArrays;
        
    void* m_specialPointers[Special::TableSize]; // Special pointers used by the LLInt and JIT.

    Debugger* m_debugger;

    RefPtr<WatchpointSet> m_masqueradesAsUndefinedWatchpoint;
    RefPtr<WatchpointSet> m_havingABadTimeWatchpoint;
    RefPtr<WatchpointSet> m_varInjectionWatchpoint;

    OwnPtr<JSGlobalObjectRareData> m_rareData;

    WeakRandom m_weakRandom;

    bool m_evalEnabled;
    String m_evalDisabledErrorMessage;
    bool m_experimentsEnabled;

    static JS_EXPORTDATA const GlobalObjectMethodTable s_globalObjectMethodTable;
    const GlobalObjectMethodTable* m_globalObjectMethodTable;

    void createRareDataIfNeeded()
    {
        if (m_rareData)
            return;
        m_rareData = adoptPtr(new JSGlobalObjectRareData);
    }
        
public:
    typedef JSSegmentedVariableObject Base;

    static JSGlobalObject* create(VM& vm, Structure* structure)
    {
        JSGlobalObject* globalObject = new (NotNull, allocateCell<JSGlobalObject>(vm.heap)) JSGlobalObject(vm, structure);
        globalObject->finishCreation(vm);
        vm.heap.addFinalizer(globalObject, destroy);
        return globalObject;
    }

    DECLARE_EXPORT_INFO;

    bool hasDebugger() const { return m_debugger; }
    bool hasProfiler() const { return globalObjectMethodTable()->supportsProfiling(this); }

protected:
    JS_EXPORT_PRIVATE explicit JSGlobalObject(VM&, Structure*, const GlobalObjectMethodTable* = 0);

    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        structure()->setGlobalObject(vm, this);
        m_experimentsEnabled = m_globalObjectMethodTable->javaScriptExperimentsEnabled(this);
        init(this);
    }

    void finishCreation(VM& vm, JSObject* thisValue)
    {
        Base::finishCreation(vm);
        structure()->setGlobalObject(vm, this);
        m_experimentsEnabled = m_globalObjectMethodTable->javaScriptExperimentsEnabled(this);
        init(thisValue);
    }

    enum ConstantMode { IsConstant, IsVariable };
    enum FunctionMode { IsFunctionToSpecialize, NotFunctionOrNotSpecializable };
    int addGlobalVar(const Identifier&, ConstantMode, FunctionMode);

public:
    JS_EXPORT_PRIVATE ~JSGlobalObject();
    JS_EXPORT_PRIVATE static void destroy(JSCell*);
    // We don't need a destructor because we use a finalizer instead.
    static const bool needsDestruction = false;

    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

    JS_EXPORT_PRIVATE static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    bool hasOwnPropertyForWrite(ExecState*, PropertyName);
    JS_EXPORT_PRIVATE static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);

    JS_EXPORT_PRIVATE static void defineGetter(JSObject*, ExecState*, PropertyName, JSObject* getterFunc, unsigned attributes);
    JS_EXPORT_PRIVATE static void defineSetter(JSObject*, ExecState*, PropertyName, JSObject* setterFunc, unsigned attributes);
    JS_EXPORT_PRIVATE static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, const PropertyDescriptor&, bool shouldThrow);

    // We use this in the code generator as we perform symbol table
    // lookups prior to initializing the properties
    bool symbolTableHasProperty(PropertyName);

    void addVar(ExecState* exec, const Identifier& propertyName)
    {
        if (!hasProperty(exec, propertyName))
            addGlobalVar(propertyName, IsVariable, NotFunctionOrNotSpecializable);
    }
    void addConst(ExecState* exec, const Identifier& propertyName)
    {
        if (!hasProperty(exec, propertyName))
            addGlobalVar(propertyName, IsConstant, NotFunctionOrNotSpecializable);
    }
    void addFunction(ExecState* exec, const Identifier& propertyName, JSValue value)
    {
        bool propertyDidExist = removeDirect(exec->vm(), propertyName); // Newly declared functions overwrite existing properties.
        int index = addGlobalVar(propertyName, IsVariable, !propertyDidExist ? IsFunctionToSpecialize : NotFunctionOrNotSpecializable);
        registerAt(index).set(exec->vm(), this, value);
    }

    // The following accessors return pristine values, even if a script 
    // replaces the global object's associated property.

    RegExpConstructor* regExpConstructor() const { return m_regExpConstructor.get(); }

    ErrorConstructor* errorConstructor() const { return m_errorConstructor.get(); }
    NativeErrorConstructor* evalErrorConstructor() const { return m_evalErrorConstructor.get(); }
    NativeErrorConstructor* rangeErrorConstructor() const { return m_rangeErrorConstructor.get(); }
    NativeErrorConstructor* referenceErrorConstructor() const { return m_referenceErrorConstructor.get(); }
    NativeErrorConstructor* syntaxErrorConstructor() const { return m_syntaxErrorConstructor.get(); }
    NativeErrorConstructor* typeErrorConstructor() const { return m_typeErrorConstructor.get(); }
    NativeErrorConstructor* URIErrorConstructor() const { return m_URIErrorConstructor.get(); }

    JSFunction* evalFunction() const { return m_evalFunction.get(); }
    JSFunction* callFunction() const { return m_callFunction.get(); }
    JSFunction* applyFunction() const { return m_applyFunction.get(); }
    GetterSetter* throwTypeErrorGetterSetter(VM& vm)
    {
        if (!m_throwTypeErrorGetterSetter)
            createThrowTypeError(vm);
        return m_throwTypeErrorGetterSetter.get();
    }

    ObjectPrototype* objectPrototype() const { return m_objectPrototype.get(); }
    FunctionPrototype* functionPrototype() const { return m_functionPrototype.get(); }
    ArrayPrototype* arrayPrototype() const { return m_arrayPrototype.get(); }
    BooleanPrototype* booleanPrototype() const { return m_booleanPrototype.get(); }
    StringPrototype* stringPrototype() const { return m_stringPrototype.get(); }
    NumberPrototype* numberPrototype() const { return m_numberPrototype.get(); }
    DatePrototype* datePrototype() const { return m_datePrototype.get(); }
    RegExpPrototype* regExpPrototype() const { return m_regExpPrototype.get(); }
    ErrorPrototype* errorPrototype() const { return m_errorPrototype.get(); }
    JSPromisePrototype* promisePrototype() const { return m_promisePrototype.get(); }
    JSPromiseResolverPrototype* promiseResolverPrototype() const { return m_promiseResolverPrototype.get(); }

    Structure* withScopeStructure() const { return m_withScopeStructure.get(); }
    Structure* strictEvalActivationStructure() const { return m_strictEvalActivationStructure.get(); }
    Structure* activationStructure() const { return m_activationStructure.get(); }
    Structure* nameScopeStructure() const { return m_nameScopeStructure.get(); }
    Structure* argumentsStructure() const { return m_argumentsStructure.get(); }
    Structure* originalArrayStructureForIndexingType(IndexingType indexingType) const
    {
        ASSERT(indexingType & IsArray);
        return m_originalArrayStructureForIndexingShape[(indexingType & IndexingShapeMask) >> IndexingShapeShift].get();
    }
    Structure* arrayStructureForIndexingTypeDuringAllocation(IndexingType indexingType) const
    {
        ASSERT(indexingType & IsArray);
        return m_arrayStructureForIndexingShapeDuringAllocation[(indexingType & IndexingShapeMask) >> IndexingShapeShift].get();
    }
    Structure* arrayStructureForProfileDuringAllocation(ArrayAllocationProfile* profile) const
    {
        return arrayStructureForIndexingTypeDuringAllocation(ArrayAllocationProfile::selectIndexingTypeFor(profile));
    }
        
    bool isOriginalArrayStructure(Structure* structure)
    {
        return originalArrayStructureForIndexingType(structure->indexingType() | IsArray) == structure;
    }
        
    Structure* booleanObjectStructure() const { return m_booleanObjectStructure.get(); }
    Structure* callbackConstructorStructure() const { return m_callbackConstructorStructure.get(); }
    Structure* callbackFunctionStructure() const { return m_callbackFunctionStructure.get(); }
    Structure* callbackObjectStructure() const { return m_callbackObjectStructure.get(); }
#if JSC_OBJC_API_ENABLED
    Structure* objcCallbackFunctionStructure() const { return m_objcCallbackFunctionStructure.get(); }
    Structure* objcWrapperObjectStructure() const { return m_objcWrapperObjectStructure.get(); }
#endif
    Structure* dateStructure() const { return m_dateStructure.get(); }
    Structure* nullPrototypeObjectStructure() const { return m_nullPrototypeObjectStructure.get(); }
    Structure* errorStructure() const { return m_errorStructure.get(); }
    Structure* functionStructure() const { return m_functionStructure.get(); }
    Structure* boundFunctionStructure() const { return m_boundFunctionStructure.get(); }
    Structure* namedFunctionStructure() const { return m_namedFunctionStructure.get(); }
    PropertyOffset functionNameOffset() const { return m_functionNameOffset; }
    Structure* numberObjectStructure() const { return m_numberObjectStructure.get(); }
    Structure* privateNameStructure() const { return m_privateNameStructure.get(); }
    Structure* internalFunctionStructure() const { return m_internalFunctionStructure.get(); }
    Structure* mapStructure() const { return m_mapStructure.get(); }
    Structure* regExpMatchesArrayStructure() const { return m_regExpMatchesArrayStructure.get(); }
    Structure* regExpStructure() const { return m_regExpStructure.get(); }
    Structure* setStructure() const { return m_setStructure.get(); }
    Structure* stringObjectStructure() const { return m_stringObjectStructure.get(); }
    Structure* iteratorResultStructure() const { return m_iteratorResultStructure.get(); }
    static ptrdiff_t iteratorResultStructureOffset() { return OBJECT_OFFSETOF(JSGlobalObject, m_iteratorResultStructure); }

#if ENABLE(PROMISES)
    Structure* promiseStructure() const { return m_promiseStructure.get(); }
    Structure* promiseResolverStructure() const { return m_promiseResolverStructure.get(); }
    Structure* promiseCallbackStructure() const { return m_promiseCallbackStructure.get(); }
    Structure* promiseWrapperCallbackStructure() const { return m_promiseWrapperCallbackStructure.get(); }
#endif // ENABLE(PROMISES)

    JSArrayBufferPrototype* arrayBufferPrototype() const { return m_arrayBufferPrototype.get(); }

#define DEFINE_ACCESSORS_FOR_SIMPLE_TYPE(capitalName, lowerName, properName, instanceType, jsName) \
    Structure* properName ## Structure() { return m_ ## properName ## Structure.get(); }

    FOR_EACH_SIMPLE_BUILTIN_TYPE(DEFINE_ACCESSORS_FOR_SIMPLE_TYPE)

#undef DEFINE_ACCESSORS_FOR_SIMPLE_TYPE

    Structure* typedArrayStructure(TypedArrayType type) const
    {
        return m_typedArrays[toIndex(type)].structure.get();
    }
    bool isOriginalTypedArrayStructure(Structure* structure)
    {
        TypedArrayType type = structure->classInfo()->typedArrayStorageType;
        if (type == NotTypedArray)
            return false;
        return typedArrayStructure(type) == structure;
    }

    void* actualPointerFor(Special::Pointer pointer)
    {
        ASSERT(pointer < Special::TableSize);
        return m_specialPointers[pointer];
    }

    WatchpointSet* masqueradesAsUndefinedWatchpoint() { return m_masqueradesAsUndefinedWatchpoint.get(); }
    WatchpointSet* havingABadTimeWatchpoint() { return m_havingABadTimeWatchpoint.get(); }
    WatchpointSet* varInjectionWatchpoint() { return m_varInjectionWatchpoint.get(); }
        
    bool isHavingABadTime() const
    {
        return m_havingABadTimeWatchpoint->hasBeenInvalidated();
    }
        
    void haveABadTime(VM&);
        
    bool objectPrototypeIsSane();
    bool arrayPrototypeChainIsSane();
    bool stringPrototypeChainIsSane();

    void setProfileGroup(unsigned value) { createRareDataIfNeeded(); m_rareData->profileGroup = value; }
    unsigned profileGroup() const
    { 
        if (!m_rareData)
            return 0;
        return m_rareData->profileGroup;
    }

    Debugger* debugger() const { return m_debugger; }
    void setDebugger(Debugger* debugger) { m_debugger = debugger; }
    static ptrdiff_t debuggerOffset() { return OBJECT_OFFSETOF(JSGlobalObject, m_debugger); }

    const GlobalObjectMethodTable* globalObjectMethodTable() const { return m_globalObjectMethodTable; }

    static bool allowsAccessFrom(const JSGlobalObject*, ExecState*) { return true; }
    static bool supportsProfiling(const JSGlobalObject*) { return false; }
    static bool supportsRichSourceInfo(const JSGlobalObject*) { return true; }

    JS_EXPORT_PRIVATE ExecState* globalExec();

    static bool shouldInterruptScript(const JSGlobalObject*) { return true; }
    static bool shouldInterruptScriptBeforeTimeout(const JSGlobalObject*) { return false; }
    static bool javaScriptExperimentsEnabled(const JSGlobalObject*) { return false; }

    bool evalEnabled() const { return m_evalEnabled; }
    const String& evalDisabledErrorMessage() const { return m_evalDisabledErrorMessage; }
    void setEvalEnabled(bool enabled, const String& errorMessage = String())
    {
        m_evalEnabled = enabled;
        m_evalDisabledErrorMessage = errorMessage;
    }

    void resetPrototype(VM&, JSValue prototype);

    VM& vm() const { return *Heap::heap(this)->vm(); }
    JSObject* globalThis() const;
    JS_EXPORT_PRIVATE void setGlobalThis(VM&, JSObject* globalThis);

    static Structure* createStructure(VM& vm, JSValue prototype)
    {
        return Structure::create(vm, 0, prototype, TypeInfo(GlobalObjectType, StructureFlags), info());
    }

    void registerWeakMap(OpaqueJSWeakObjectMap* map)
    {
        createRareDataIfNeeded();
        m_rareData->weakMaps.add(map);
    }

    void unregisterWeakMap(OpaqueJSWeakObjectMap* map)
    {
        if (m_rareData)
            m_rareData->weakMaps.remove(map);
    }

    OpaqueJSClassDataMap& opaqueJSClassData()
    {
        createRareDataIfNeeded();
        return m_rareData->opaqueJSClassData;
    }

    double weakRandomNumber() { return m_weakRandom.get(); }
    unsigned weakRandomInteger() { return m_weakRandom.getUint32(); }

    UnlinkedProgramCodeBlock* createProgramCodeBlock(CallFrame*, ProgramExecutable*, JSObject** exception);
    UnlinkedEvalCodeBlock* createEvalCodeBlock(CallFrame*, EvalExecutable*);

protected:

    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames | Base::StructureFlags;

    struct GlobalPropertyInfo {
        GlobalPropertyInfo(const Identifier& i, JSValue v, unsigned a)
            : identifier(i)
            , value(v)
            , attributes(a)
        {
        }

        const Identifier identifier;
        JSValue value;
        unsigned attributes;
    };
    JS_EXPORT_PRIVATE void addStaticGlobals(GlobalPropertyInfo*, int count);

    JS_EXPORT_PRIVATE static JSC::JSValue toThis(JSC::JSCell*, JSC::ExecState*, ECMAMode);

private:
    friend class LLIntOffsetsExtractor;
        
    // FIXME: Fold reset into init.
    JS_EXPORT_PRIVATE void init(JSObject* thisValue);
    void reset(JSValue prototype);

    void createThrowTypeError(VM&);

    JS_EXPORT_PRIVATE static void clearRareData(JSCell*);
};

JSGlobalObject* asGlobalObject(JSValue);

inline JSGlobalObject* asGlobalObject(JSValue value)
{
    ASSERT(asObject(value)->isGlobalObject());
    return jsCast<JSGlobalObject*>(asObject(value));
}

inline bool JSGlobalObject::hasOwnPropertyForWrite(ExecState* exec, PropertyName propertyName)
{
    PropertySlot slot(this);
    if (Base::getOwnPropertySlot(this, exec, propertyName, slot))
        return true;
    bool slotIsWriteable;
    return symbolTableGet(this, propertyName, slot, slotIsWriteable);
}

inline bool JSGlobalObject::symbolTableHasProperty(PropertyName propertyName)
{
    SymbolTableEntry entry = symbolTable()->inlineGet(propertyName.publicName());
    return !entry.isNull();
}

inline JSGlobalObject* ExecState::dynamicGlobalObject()
{
    if (this == lexicalGlobalObject()->globalExec())
        return lexicalGlobalObject();

    // For any ExecState that's not a globalExec, the 
    // dynamic global object must be set since code is running
    ASSERT(vm().dynamicGlobalObject);
    return vm().dynamicGlobalObject;
}

inline JSArray* constructEmptyArray(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, unsigned initialLength = 0)
{
    return ArrayAllocationProfile::updateLastAllocationFor(profile, JSArray::create(exec->vm(), initialLength >= MIN_SPARSE_ARRAY_INDEX ? globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithArrayStorage) : globalObject->arrayStructureForProfileDuringAllocation(profile), initialLength));
}

inline JSArray* constructEmptyArray(ExecState* exec, ArrayAllocationProfile* profile, unsigned initialLength = 0)
{
    return constructEmptyArray(exec, profile, exec->lexicalGlobalObject(), initialLength);
}
 
inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, const ArgList& values)
{
    return ArrayAllocationProfile::updateLastAllocationFor(profile, constructArray(exec, globalObject->arrayStructureForProfileDuringAllocation(profile), values));
}

inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, const ArgList& values)
{
    return constructArray(exec, profile, exec->lexicalGlobalObject(), values);
}

inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, const JSValue* values, unsigned length)
{
    return ArrayAllocationProfile::updateLastAllocationFor(profile, constructArray(exec, globalObject->arrayStructureForProfileDuringAllocation(profile), values, length));
}

inline JSArray* constructArray(ExecState* exec, ArrayAllocationProfile* profile, const JSValue* values, unsigned length)
{
    return constructArray(exec, profile, exec->lexicalGlobalObject(), values, length);
}

inline JSArray* constructArrayNegativeIndexed(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, const JSValue* values, unsigned length)
{
    return ArrayAllocationProfile::updateLastAllocationFor(profile, constructArrayNegativeIndexed(exec, globalObject->arrayStructureForProfileDuringAllocation(profile), values, length));
}

inline JSArray* constructArrayNegativeIndexed(ExecState* exec, ArrayAllocationProfile* profile, const JSValue* values, unsigned length)
{
    return constructArrayNegativeIndexed(exec, profile, exec->lexicalGlobalObject(), values, length);
}

class DynamicGlobalObjectScope {
    WTF_MAKE_NONCOPYABLE(DynamicGlobalObjectScope);
public:
    JS_EXPORT_PRIVATE DynamicGlobalObjectScope(VM&, JSGlobalObject*);

    ~DynamicGlobalObjectScope()
    {
        m_dynamicGlobalObjectSlot = m_savedDynamicGlobalObject;
    }

private:
    JSGlobalObject*& m_dynamicGlobalObjectSlot;
    JSGlobalObject* m_savedDynamicGlobalObject;
};

inline JSObject* JSScope::globalThis()
{ 
    return globalObject()->globalThis();
}

inline JSObject* JSGlobalObject::globalThis() const
{ 
    return m_globalThis.get();
}

} // namespace JSC

#endif // JSGlobalObject_h
