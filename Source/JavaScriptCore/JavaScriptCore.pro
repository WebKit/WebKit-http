# JavaScriptCore - qmake build info
CONFIG += building-libs
include($$PWD/../WebKit.pri)
include(JavaScriptCore.pri)

TEMPLATE = lib
CONFIG += staticlib
# Don't use JavaScriptCore as the target name. qmake would create a JavaScriptCore.vcproj for msvc
# which already exists as a directory
TARGET = $$JAVASCRIPTCORE_TARGET
DESTDIR = $$JAVASCRIPTCORE_DESTDIR
QT += core
QT -= gui

CONFIG += depend_includepath

contains(QT_CONFIG, embedded):CONFIG += embedded
contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
unix:contains(QT_CONFIG, reduce_relocations):CONFIG += bsymbolic_functions

*-g++*:QMAKE_CXXFLAGS_RELEASE -= -O2
*-g++*:QMAKE_CXXFLAGS_RELEASE += -O3

# Rules when JIT enabled (not disabled)
!contains(DEFINES, ENABLE_JIT=0) {
    linux*-g++*:greaterThan(QT_GCC_MAJOR_VERSION,3):greaterThan(QT_GCC_MINOR_VERSION,0) {
        QMAKE_CXXFLAGS += -fno-stack-protector
        QMAKE_CFLAGS += -fno-stack-protector
    }
}

wince* {
    SOURCES += $$QT_SOURCE_TREE/src/3rdparty/ce-compat/ce_time.c
}

include(yarr/yarr.pri)
include(wtf/wtf.pri)

INSTALLDEPS += all

SOURCES += \
    API/JSBase.cpp \
    API/JSCallbackConstructor.cpp \
    API/JSCallbackFunction.cpp \
    API/JSCallbackObject.cpp \
    API/JSClassRef.cpp \
    API/JSContextRef.cpp \
    API/JSObjectRef.cpp \
    API/JSStringRef.cpp \
    API/JSValueRef.cpp \
    API/OpaqueJSString.cpp \
    assembler/ARMAssembler.cpp \
    assembler/ARMv7Assembler.cpp \
    assembler/MacroAssemblerARM.cpp \
    assembler/MacroAssemblerSH4.cpp \
    bytecode/CodeBlock.cpp \
    bytecode/JumpTable.cpp \
    bytecode/Opcode.cpp \
    bytecode/PredictedType.cpp \
    bytecode/SamplingTool.cpp \
    bytecode/StructureStubInfo.cpp \
    bytecode/ValueProfile.cpp \
    bytecompiler/BytecodeGenerator.cpp \
    bytecompiler/NodesCodegen.cpp \
    heap/AllocationSpace.cpp \
    heap/ConservativeRoots.cpp \
    heap/HandleHeap.cpp \
    heap/HandleStack.cpp \
    heap/Heap.cpp \
    heap/JettisonedCodeBlocks.cpp \
    heap/MachineStackMarker.cpp \
    heap/MarkStack.cpp \
    heap/MarkedBlock.cpp \
    heap/MarkedSpace.cpp \
    heap/VTableSpectrum.cpp \
    heap/WriteBarrierSupport.cpp \
    debugger/DebuggerActivation.cpp \
    debugger/DebuggerCallFrame.cpp \
    debugger/Debugger.cpp \
    dfg/DFGAbstractState.cpp \
    dfg/DFGByteCodeParser.cpp \
    dfg/DFGCapabilities.cpp \
    dfg/DFGDriver.cpp \
    dfg/DFGGraph.cpp \
    dfg/DFGJITCodeGenerator.cpp \
    dfg/DFGJITCodeGenerator32_64.cpp \
    dfg/DFGJITCodeGenerator64.cpp \
    dfg/DFGJITCompiler.cpp \
    dfg/DFGJITCompiler32_64.cpp \
    dfg/DFGOperations.cpp \
    dfg/DFGOSREntry.cpp \
    dfg/DFGPropagator.cpp \
    dfg/DFGRepatch.cpp \
    dfg/DFGSpeculativeJIT.cpp \
    dfg/DFGSpeculativeJIT32_64.cpp \
    dfg/DFGSpeculativeJIT64.cpp \
    interpreter/CallFrame.cpp \
    interpreter/Interpreter.cpp \
    interpreter/RegisterFile.cpp \
    jit/ExecutableAllocatorFixedVMPool.cpp \
    jit/ExecutableAllocator.cpp \
    jit/JITArithmetic.cpp \
    jit/JITArithmetic32_64.cpp \
    jit/JITCall.cpp \
    jit/JITCall32_64.cpp \
    jit/JIT.cpp \
    jit/JITOpcodes.cpp \
    jit/JITOpcodes32_64.cpp \
    jit/JITPropertyAccess.cpp \
    jit/JITPropertyAccess32_64.cpp \
    jit/JITStubs.cpp \
    jit/ThunkGenerators.cpp \
    parser/Lexer.cpp \
    parser/Nodes.cpp \
    parser/ParserArena.cpp \
    parser/Parser.cpp \
    parser/SourceProviderCache.cpp \
    profiler/Profile.cpp \
    profiler/ProfileGenerator.cpp \
    profiler/ProfileNode.cpp \
    profiler/Profiler.cpp \
    runtime/ArgList.cpp \
    runtime/Arguments.cpp \
    runtime/ArrayConstructor.cpp \
    runtime/ArrayPrototype.cpp \
    runtime/BooleanConstructor.cpp \
    runtime/BooleanObject.cpp \
    runtime/BooleanPrototype.cpp \
    runtime/CallData.cpp \
    runtime/CommonIdentifiers.cpp \
    runtime/Completion.cpp \
    runtime/ConstructData.cpp \
    runtime/DateConstructor.cpp \
    runtime/DateConversion.cpp \
    runtime/DateInstance.cpp \
    runtime/DatePrototype.cpp \
    runtime/ErrorConstructor.cpp \
    runtime/Error.cpp \
    runtime/ErrorInstance.cpp \
    runtime/ErrorPrototype.cpp \
    runtime/ExceptionHelpers.cpp \
    runtime/Executable.cpp \
    runtime/FunctionConstructor.cpp \
    runtime/FunctionPrototype.cpp \
    runtime/GCActivityCallback.cpp \
    runtime/GetterSetter.cpp \
    runtime/Heuristics.cpp \
    runtime/Identifier.cpp \
    runtime/InitializeThreading.cpp \
    runtime/InternalFunction.cpp \
    runtime/JSActivation.cpp \
    runtime/JSAPIValueWrapper.cpp \
    runtime/JSArray.cpp \
    runtime/JSByteArray.cpp \
    runtime/JSCell.cpp \
    runtime/JSFunction.cpp \
    runtime/JSBoundFunction.cpp \
    runtime/JSGlobalData.cpp \
    runtime/JSGlobalObject.cpp \
    runtime/JSGlobalObjectFunctions.cpp \
    runtime/JSGlobalThis.cpp \
    runtime/JSLock.cpp \
    runtime/JSNotAnObject.cpp \
    runtime/JSObject.cpp \
    runtime/JSONObject.cpp \
    runtime/JSPropertyNameIterator.cpp \
    runtime/JSStaticScopeObject.cpp \
    runtime/JSString.cpp \
    runtime/JSValue.cpp \
    runtime/JSVariableObject.cpp \
    runtime/JSWrapperObject.cpp \
    runtime/LiteralParser.cpp \
    runtime/Lookup.cpp \
    runtime/MathObject.cpp \
    runtime/NativeErrorConstructor.cpp \
    runtime/NativeErrorPrototype.cpp \
    runtime/NumberConstructor.cpp \
    runtime/NumberObject.cpp \
    runtime/NumberPrototype.cpp \
    runtime/ObjectConstructor.cpp \
    runtime/ObjectPrototype.cpp \
    runtime/Operations.cpp \
    runtime/PropertyDescriptor.cpp \
    runtime/PropertyNameArray.cpp \
    runtime/PropertySlot.cpp \
    runtime/RegExpConstructor.cpp \
    runtime/RegExp.cpp \
    runtime/RegExpObject.cpp \
    runtime/RegExpPrototype.cpp \
    runtime/RegExpCache.cpp \
    runtime/SamplingCounter.cpp \
    runtime/ScopeChain.cpp \
    runtime/SmallStrings.cpp \
    runtime/StrictEvalActivation.cpp \
    runtime/StringConstructor.cpp \
    runtime/StringObject.cpp \
    runtime/StringPrototype.cpp \
    runtime/StringRecursionChecker.cpp \
    runtime/StructureChain.cpp \
    runtime/Structure.cpp \
    runtime/TimeoutChecker.cpp \
    runtime/UString.cpp \
    yarr/YarrJIT.cpp \

*sh4* {
    QMAKE_CXXFLAGS += -mieee -w
    QMAKE_CFLAGS   += -mieee -w
}

# Generated files, simply list them for JavaScriptCore

lessThan(QT_GCC_MAJOR_VERSION, 5) {
    # GCC 4.5 and before
    lessThan(QT_GCC_MINOR_VERSION, 6) {
        # Disable C++0x mode in JSC for those who enabled it in their Qt's mkspec.
        *-g++*:QMAKE_CXXFLAGS -= -std=c++0x -std=gnu++0x
    }

    # GCC 4.6 and after.
    greaterThan(QT_GCC_MINOR_VERSION, 5) {
        if (!contains(QMAKE_CXXFLAGS, -std=c++0x) && !contains(QMAKE_CXXFLAGS, -std=gnu++0x)) {
            # We need to deactivate those warnings because some names conflicts with upcoming c++0x types (e.g.nullptr).
            QMAKE_CFLAGS_WARN_ON += -Wno-c++0x-compat
            QMAKE_CXXFLAGS_WARN_ON += -Wno-c++0x-compat
            QMAKE_CFLAGS += -Wno-c++0x-compat
            QMAKE_CXXFLAGS += -Wno-c++0x-compat
        }
    }
}
