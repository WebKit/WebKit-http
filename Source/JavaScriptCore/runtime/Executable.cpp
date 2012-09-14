/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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
#include "Executable.h"

#include "BytecodeGenerator.h"
#include "CodeBlock.h"
#include "DFGDriver.h"
#include "ExecutionHarness.h"
#include "JIT.h"
#include "JITDriver.h"
#include "Parser.h"
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>

namespace JSC {

const ClassInfo ExecutableBase::s_info = { "Executable", 0, 0, 0, CREATE_METHOD_TABLE(ExecutableBase) };

#if ENABLE(JIT)
void ExecutableBase::destroy(JSCell* cell)
{
    static_cast<ExecutableBase*>(cell)->ExecutableBase::~ExecutableBase();
}
#endif

void ExecutableBase::clearCode()
{
#if ENABLE(JIT)
    m_jitCodeForCall.clear();
    m_jitCodeForConstruct.clear();
    m_jitCodeForCallWithArityCheck = MacroAssemblerCodePtr();
    m_jitCodeForConstructWithArityCheck = MacroAssemblerCodePtr();
#endif
    m_numParametersForCall = NUM_PARAMETERS_NOT_COMPILED;
    m_numParametersForConstruct = NUM_PARAMETERS_NOT_COMPILED;
}

#if ENABLE(DFG_JIT)
Intrinsic ExecutableBase::intrinsic() const
{
    if (const NativeExecutable* nativeExecutable = jsDynamicCast<const NativeExecutable*>(this))
        return nativeExecutable->intrinsic();
    return NoIntrinsic;
}
#endif

const ClassInfo NativeExecutable::s_info = { "NativeExecutable", &ExecutableBase::s_info, 0, 0, CREATE_METHOD_TABLE(NativeExecutable) };

#if ENABLE(JIT)
void NativeExecutable::destroy(JSCell* cell)
{
    static_cast<NativeExecutable*>(cell)->NativeExecutable::~NativeExecutable();
}
#endif

#if ENABLE(DFG_JIT)
Intrinsic NativeExecutable::intrinsic() const
{
    return m_intrinsic;
}
#endif

#if ENABLE(JIT)
// Utility method used for jettisoning code blocks.
template<typename T>
static void jettisonCodeBlock(JSGlobalData& globalData, OwnPtr<T>& codeBlock)
{
    ASSERT(JITCode::isOptimizingJIT(codeBlock->getJITType()));
    ASSERT(codeBlock->alternative());
    OwnPtr<T> codeBlockToJettison = codeBlock.release();
    codeBlock = static_pointer_cast<T>(codeBlockToJettison->releaseAlternative());
    codeBlockToJettison->unlinkIncomingCalls();
    globalData.heap.jettisonDFGCodeBlock(static_pointer_cast<CodeBlock>(codeBlockToJettison.release()));
}
#endif

const ClassInfo ScriptExecutable::s_info = { "ScriptExecutable", &ExecutableBase::s_info, 0, 0, CREATE_METHOD_TABLE(ScriptExecutable) };

#if ENABLE(JIT)
void ScriptExecutable::destroy(JSCell* cell)
{
    static_cast<ScriptExecutable*>(cell)->ScriptExecutable::~ScriptExecutable();
}
#endif

const ClassInfo EvalExecutable::s_info = { "EvalExecutable", &ScriptExecutable::s_info, 0, 0, CREATE_METHOD_TABLE(EvalExecutable) };

EvalExecutable::EvalExecutable(ExecState* exec, const SourceCode& source, bool inStrictContext)
    : ScriptExecutable(exec->globalData().evalExecutableStructure.get(), exec, source, inStrictContext)
{
}

void EvalExecutable::destroy(JSCell* cell)
{
    static_cast<EvalExecutable*>(cell)->EvalExecutable::~EvalExecutable();
}

const ClassInfo ProgramExecutable::s_info = { "ProgramExecutable", &ScriptExecutable::s_info, 0, 0, CREATE_METHOD_TABLE(ProgramExecutable) };

ProgramExecutable::ProgramExecutable(ExecState* exec, const SourceCode& source)
    : ScriptExecutable(exec->globalData().programExecutableStructure.get(), exec, source, false)
{
}

void ProgramExecutable::destroy(JSCell* cell)
{
    static_cast<ProgramExecutable*>(cell)->ProgramExecutable::~ProgramExecutable();
}

const ClassInfo FunctionExecutable::s_info = { "FunctionExecutable", &ScriptExecutable::s_info, 0, 0, CREATE_METHOD_TABLE(FunctionExecutable) };

FunctionExecutable::FunctionExecutable(JSGlobalData& globalData, FunctionBodyNode* node)
    : ScriptExecutable(globalData.functionExecutableStructure.get(), globalData, node->source(), node->isStrictMode())
    , m_numCapturedVariables(0)
    , m_forceUsesArguments(node->usesArguments())
    , m_parameters(node->parameters())
    , m_name(node->ident())
    , m_inferredName(node->inferredName().isNull() ? globalData.propertyNames->emptyIdentifier : node->inferredName())
    , m_functionNameIsInScopeToggle(node->functionNameIsInScopeToggle())
{
    m_firstLine = node->lineNo();
    m_lastLine = node->lastLine();
}

void FunctionExecutable::destroy(JSCell* cell)
{
    static_cast<FunctionExecutable*>(cell)->FunctionExecutable::~FunctionExecutable();
}

JSObject* EvalExecutable::compileOptimized(ExecState* exec, JSScope* scope, unsigned bytecodeIndex)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_evalCodeBlock);
    JSObject* error = 0;
    if (m_evalCodeBlock->getJITType() != JITCode::topTierJIT())
        error = compileInternal(exec, scope, JITCode::nextTierJIT(m_evalCodeBlock->getJITType()), bytecodeIndex);
    ASSERT(!!m_evalCodeBlock);
    return error;
}

#if ENABLE(JIT)
bool EvalExecutable::jitCompile(ExecState* exec)
{
    return jitCompileIfAppropriate(exec, m_evalCodeBlock, m_jitCodeForCall, JITCode::bottomTierJIT(), UINT_MAX, JITCompilationCanFail);
}
#endif

inline const char* samplingDescription(JITCode::JITType jitType)
{
    switch (jitType) {
    case JITCode::InterpreterThunk:
        return "Interpreter Compilation (TOTAL)";
    case JITCode::BaselineJIT:
        return "Baseline Compilation (TOTAL)";
    case JITCode::DFGJIT:
        return "DFG Compilation (TOTAL)";
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

JSObject* EvalExecutable::compileInternal(ExecState* exec, JSScope* scope, JITCode::JITType jitType, unsigned bytecodeIndex)
{
    SamplingRegion samplingRegion(samplingDescription(jitType));
    
#if !ENABLE(JIT)
    UNUSED_PARAM(jitType);
    UNUSED_PARAM(bytecodeIndex);
#endif
    JSObject* exception = 0;
    JSGlobalData* globalData = &exec->globalData();
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    
    if (!!m_evalCodeBlock) {
        OwnPtr<EvalCodeBlock> newCodeBlock = adoptPtr(new EvalCodeBlock(CodeBlock::CopyParsedBlock, *m_evalCodeBlock));
        newCodeBlock->setAlternative(static_pointer_cast<CodeBlock>(m_evalCodeBlock.release()));
        m_evalCodeBlock = newCodeBlock.release();
    } else {
        if (!lexicalGlobalObject->evalEnabled())
            return throwError(exec, createEvalError(exec, lexicalGlobalObject->evalDisabledErrorMessage()));
        RefPtr<EvalNode> evalNode = parse<EvalNode>(globalData, lexicalGlobalObject, m_source, 0, Identifier(), isStrictMode() ? JSParseStrict : JSParseNormal, EvalNode::isFunctionNode ? JSParseFunctionCode : JSParseProgramCode, lexicalGlobalObject->debugger(), exec, &exception);
        if (!evalNode) {
            ASSERT(exception);
            return exception;
        }
        recordParse(evalNode->features(), evalNode->hasCapturedVariables(), evalNode->lineNo(), evalNode->lastLine());
        
        JSGlobalObject* globalObject = scope->globalObject();
        
        OwnPtr<CodeBlock> previousCodeBlock = m_evalCodeBlock.release();
        ASSERT((jitType == JITCode::bottomTierJIT()) == !previousCodeBlock);
        m_evalCodeBlock = adoptPtr(new EvalCodeBlock(this, globalObject, source().provider(), scope->localDepth(), previousCodeBlock.release()));
        OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(evalNode.get(), scope, m_evalCodeBlock->symbolTable(), m_evalCodeBlock.get(), !!m_evalCodeBlock->alternative() ? OptimizingCompilation : FirstCompilation)));
        if ((exception = generator->generate())) {
            m_evalCodeBlock = static_pointer_cast<EvalCodeBlock>(m_evalCodeBlock->releaseAlternative());
            evalNode->destroyData();
            return exception;
        }
        
        evalNode->destroyData();
        m_evalCodeBlock->copyPostParseDataFromAlternative();
    }

#if ENABLE(JIT)
    if (!prepareForExecution(exec, m_evalCodeBlock, m_jitCodeForCall, jitType, bytecodeIndex))
        return 0;
#endif

#if ENABLE(JIT)
#if ENABLE(CLASSIC_INTERPRETER)
    if (!m_jitCodeForCall)
        Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_evalCodeBlock));
    else
#endif
    Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_evalCodeBlock) + m_jitCodeForCall.size());
#else
    Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_evalCodeBlock));
#endif

    return 0;
}

#if ENABLE(JIT)
void EvalExecutable::jettisonOptimizedCode(JSGlobalData& globalData)
{
    jettisonCodeBlock(globalData, m_evalCodeBlock);
    m_jitCodeForCall = m_evalCodeBlock->getJITCode();
    ASSERT(!m_jitCodeForCallWithArityCheck);
}
#endif

void EvalExecutable::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    EvalExecutable* thisObject = jsCast<EvalExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    ScriptExecutable::visitChildren(thisObject, visitor);
    if (thisObject->m_evalCodeBlock)
        thisObject->m_evalCodeBlock->visitAggregate(visitor);
}

void EvalExecutable::unlinkCalls()
{
#if ENABLE(JIT)
    if (!m_jitCodeForCall)
        return;
    ASSERT(m_evalCodeBlock);
    m_evalCodeBlock->unlinkCalls();
#endif
}

void EvalExecutable::clearCode()
{
    m_evalCodeBlock.clear();
    Base::clearCode();
}

JSObject* ProgramExecutable::checkSyntax(ExecState* exec)
{
    JSObject* exception = 0;
    JSGlobalData* globalData = &exec->globalData();
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    RefPtr<ProgramNode> programNode = parse<ProgramNode>(globalData, lexicalGlobalObject, m_source, 0, Identifier(), JSParseNormal, ProgramNode::isFunctionNode ? JSParseFunctionCode : JSParseProgramCode, lexicalGlobalObject->debugger(), exec, &exception);
    if (programNode)
        return 0;
    ASSERT(exception);
    return exception;
}

JSObject* ProgramExecutable::compileOptimized(ExecState* exec, JSScope* scope, unsigned bytecodeIndex)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_programCodeBlock);
    JSObject* error = 0;
    if (m_programCodeBlock->getJITType() != JITCode::topTierJIT())
        error = compileInternal(exec, scope, JITCode::nextTierJIT(m_programCodeBlock->getJITType()), bytecodeIndex);
    ASSERT(!!m_programCodeBlock);
    return error;
}

#if ENABLE(JIT)
bool ProgramExecutable::jitCompile(ExecState* exec)
{
    return jitCompileIfAppropriate(exec, m_programCodeBlock, m_jitCodeForCall, JITCode::bottomTierJIT(), UINT_MAX, JITCompilationCanFail);
}
#endif

JSObject* ProgramExecutable::compileInternal(ExecState* exec, JSScope* scope, JITCode::JITType jitType, unsigned bytecodeIndex)
{
    SamplingRegion samplingRegion(samplingDescription(jitType));
    
#if !ENABLE(JIT)
    UNUSED_PARAM(jitType);
    UNUSED_PARAM(bytecodeIndex);
#endif
    JSObject* exception = 0;
    JSGlobalData* globalData = &exec->globalData();
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    
    if (!!m_programCodeBlock) {
        OwnPtr<ProgramCodeBlock> newCodeBlock = adoptPtr(new ProgramCodeBlock(CodeBlock::CopyParsedBlock, *m_programCodeBlock));
        newCodeBlock->setAlternative(static_pointer_cast<CodeBlock>(m_programCodeBlock.release()));
        m_programCodeBlock = newCodeBlock.release();
    } else {
        RefPtr<ProgramNode> programNode = parse<ProgramNode>(globalData, lexicalGlobalObject, m_source, 0, Identifier(), isStrictMode() ? JSParseStrict : JSParseNormal, ProgramNode::isFunctionNode ? JSParseFunctionCode : JSParseProgramCode, lexicalGlobalObject->debugger(), exec, &exception);
        if (!programNode) {
            ASSERT(exception);
            return exception;
        }
        recordParse(programNode->features(), programNode->hasCapturedVariables(), programNode->lineNo(), programNode->lastLine());

        JSGlobalObject* globalObject = scope->globalObject();
    
        OwnPtr<CodeBlock> previousCodeBlock = m_programCodeBlock.release();
        ASSERT((jitType == JITCode::bottomTierJIT()) == !previousCodeBlock);
        m_programCodeBlock = adoptPtr(new ProgramCodeBlock(this, GlobalCode, globalObject, source().provider(), previousCodeBlock.release()));
        OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(programNode.get(), scope, globalObject->symbolTable(), m_programCodeBlock.get(), !!m_programCodeBlock->alternative() ? OptimizingCompilation : FirstCompilation)));
        if ((exception = generator->generate())) {
            m_programCodeBlock = static_pointer_cast<ProgramCodeBlock>(m_programCodeBlock->releaseAlternative());
            programNode->destroyData();
            return exception;
        }

        programNode->destroyData();
        m_programCodeBlock->copyPostParseDataFromAlternative();
    }

#if ENABLE(JIT)
    if (!prepareForExecution(exec, m_programCodeBlock, m_jitCodeForCall, jitType, bytecodeIndex))
        return 0;
#endif

#if ENABLE(JIT)
#if ENABLE(CLASSIC_INTERPRETER)
    if (!m_jitCodeForCall)
        Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_programCodeBlock));
    else
#endif
        Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_programCodeBlock) + m_jitCodeForCall.size());
#else
    Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_programCodeBlock));
#endif

    return 0;
}

#if ENABLE(JIT)
void ProgramExecutable::jettisonOptimizedCode(JSGlobalData& globalData)
{
    jettisonCodeBlock(globalData, m_programCodeBlock);
    m_jitCodeForCall = m_programCodeBlock->getJITCode();
    ASSERT(!m_jitCodeForCallWithArityCheck);
}
#endif

void ProgramExecutable::unlinkCalls()
{
#if ENABLE(JIT)
    if (!m_jitCodeForCall)
        return;
    ASSERT(m_programCodeBlock);
    m_programCodeBlock->unlinkCalls();
#endif
}

void ProgramExecutable::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    ProgramExecutable* thisObject = jsCast<ProgramExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    ScriptExecutable::visitChildren(thisObject, visitor);
    if (thisObject->m_programCodeBlock)
        thisObject->m_programCodeBlock->visitAggregate(visitor);
}

void ProgramExecutable::clearCode()
{
    m_programCodeBlock.clear();
    Base::clearCode();
}

FunctionCodeBlock* FunctionExecutable::baselineCodeBlockFor(CodeSpecializationKind kind)
{
    FunctionCodeBlock* result;
    if (kind == CodeForCall)
        result = m_codeBlockForCall.get();
    else {
        ASSERT(kind == CodeForConstruct);
        result = m_codeBlockForConstruct.get();
    }
    if (!result)
        return 0;
    while (result->alternative())
        result = static_cast<FunctionCodeBlock*>(result->alternative());
    ASSERT(result);
    ASSERT(JITCode::isBaselineCode(result->getJITType()));
    return result;
}

JSObject* FunctionExecutable::compileOptimizedForCall(ExecState* exec, JSScope* scope, unsigned bytecodeIndex)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_codeBlockForCall);
    JSObject* error = 0;
    if (m_codeBlockForCall->getJITType() != JITCode::topTierJIT())
        error = compileForCallInternal(exec, scope, JITCode::nextTierJIT(m_codeBlockForCall->getJITType()), bytecodeIndex);
    ASSERT(!!m_codeBlockForCall);
    return error;
}

JSObject* FunctionExecutable::compileOptimizedForConstruct(ExecState* exec, JSScope* scope, unsigned bytecodeIndex)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_codeBlockForConstruct);
    JSObject* error = 0;
    if (m_codeBlockForConstruct->getJITType() != JITCode::topTierJIT())
        error = compileForConstructInternal(exec, scope, JITCode::nextTierJIT(m_codeBlockForConstruct->getJITType()), bytecodeIndex);
    ASSERT(!!m_codeBlockForConstruct);
    return error;
}

#if ENABLE(JIT)
bool FunctionExecutable::jitCompileForCall(ExecState* exec)
{
    return jitCompileFunctionIfAppropriate(exec, m_codeBlockForCall, m_jitCodeForCall, m_jitCodeForCallWithArityCheck, m_symbolTable, JITCode::bottomTierJIT(), UINT_MAX, JITCompilationCanFail);
}

bool FunctionExecutable::jitCompileForConstruct(ExecState* exec)
{
    return jitCompileFunctionIfAppropriate(exec, m_codeBlockForConstruct, m_jitCodeForConstruct, m_jitCodeForConstructWithArityCheck, m_symbolTable, JITCode::bottomTierJIT(), UINT_MAX, JITCompilationCanFail);
}
#endif

FunctionCodeBlock* FunctionExecutable::codeBlockWithBytecodeFor(CodeSpecializationKind kind)
{
    return baselineCodeBlockFor(kind);
}

PassOwnPtr<FunctionCodeBlock> FunctionExecutable::produceCodeBlockFor(JSScope* scope, CompilationKind compilationKind, CodeSpecializationKind specializationKind, JSObject*& exception)
{
    if (!!codeBlockFor(specializationKind))
        return adoptPtr(new FunctionCodeBlock(CodeBlock::CopyParsedBlock, *codeBlockFor(specializationKind)));
    
    exception = 0;
    JSGlobalData* globalData = scope->globalData();
    JSGlobalObject* globalObject = scope->globalObject();
    RefPtr<FunctionBodyNode> body = parse<FunctionBodyNode>(
        globalData,
        globalObject,
        m_source,
        m_parameters.get(),
        name(),
        isStrictMode() ? JSParseStrict : JSParseNormal,
        FunctionBodyNode::isFunctionNode ? JSParseFunctionCode : JSParseProgramCode,
        0,
        0,
        &exception
    );

    if (!body) {
        ASSERT(exception);
        return nullptr;
    }
    if (m_forceUsesArguments)
        body->setUsesArguments();
    body->finishParsing(m_parameters, m_name, m_functionNameIsInScopeToggle);
    recordParse(body->features(), body->hasCapturedVariables(), body->lineNo(), body->lastLine());

    OwnPtr<FunctionCodeBlock> result;
    ASSERT((compilationKind == FirstCompilation) == !codeBlockFor(specializationKind));
    result = adoptPtr(new FunctionCodeBlock(this, FunctionCode, globalObject, source().provider(), source().startOffset(), specializationKind == CodeForConstruct));
    OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(body.get(), scope, result->symbolTable(), result.get(), compilationKind)));
    exception = generator->generate();
    body->destroyData();
    if (exception)
        return nullptr;

    result->copyPostParseDataFrom(codeBlockFor(specializationKind).get());
    return result.release();
}

JSObject* FunctionExecutable::compileForCallInternal(ExecState* exec, JSScope* scope, JITCode::JITType jitType, unsigned bytecodeIndex)
{
    SamplingRegion samplingRegion(samplingDescription(jitType));
    
#if !ENABLE(JIT)
    UNUSED_PARAM(exec);
    UNUSED_PARAM(jitType);
    UNUSED_PARAM(exec);
    UNUSED_PARAM(bytecodeIndex);
#endif
    ASSERT((jitType == JITCode::bottomTierJIT()) == !m_codeBlockForCall);
    JSObject* exception;
    OwnPtr<FunctionCodeBlock> newCodeBlock = produceCodeBlockFor(scope, !!m_codeBlockForCall ? OptimizingCompilation : FirstCompilation, CodeForCall, exception);
    if (!newCodeBlock)
        return exception;

    newCodeBlock->setAlternative(static_pointer_cast<CodeBlock>(m_codeBlockForCall.release()));
    m_codeBlockForCall = newCodeBlock.release();
    
    m_numParametersForCall = m_codeBlockForCall->numParameters();
    ASSERT(m_numParametersForCall);
    m_numCapturedVariables = m_codeBlockForCall->m_numCapturedVars;
    m_symbolTable.set(exec->globalData(), this, m_codeBlockForCall->symbolTable());

#if ENABLE(JIT)
    if (!prepareFunctionForExecution(exec, m_codeBlockForCall, m_jitCodeForCall, m_jitCodeForCallWithArityCheck, m_symbolTable, jitType, bytecodeIndex, CodeForCall))
        return 0;
#endif

#if ENABLE(JIT)
#if ENABLE(CLASSIC_INTERPRETER)
    if (!m_jitCodeForCall)
        Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_codeBlockForCall));
    else
#endif
        Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_codeBlockForCall) + m_jitCodeForCall.size());
#else
    Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_codeBlockForCall));
#endif

    return 0;
}

JSObject* FunctionExecutable::compileForConstructInternal(ExecState* exec, JSScope* scope, JITCode::JITType jitType, unsigned bytecodeIndex)
{
    SamplingRegion samplingRegion(samplingDescription(jitType));
    
#if !ENABLE(JIT)
    UNUSED_PARAM(jitType);
    UNUSED_PARAM(exec);
    UNUSED_PARAM(bytecodeIndex);
#endif
    
    ASSERT((jitType == JITCode::bottomTierJIT()) == !m_codeBlockForConstruct);
    JSObject* exception;
    OwnPtr<FunctionCodeBlock> newCodeBlock = produceCodeBlockFor(scope, !!m_codeBlockForConstruct ? OptimizingCompilation : FirstCompilation, CodeForConstruct, exception);
    if (!newCodeBlock)
        return exception;

    newCodeBlock->setAlternative(static_pointer_cast<CodeBlock>(m_codeBlockForConstruct.release()));
    m_codeBlockForConstruct = newCodeBlock.release();
    
    m_numParametersForConstruct = m_codeBlockForConstruct->numParameters();
    ASSERT(m_numParametersForConstruct);
    m_numCapturedVariables = m_codeBlockForConstruct->m_numCapturedVars;
    m_symbolTable.set(exec->globalData(), this, m_codeBlockForConstruct->symbolTable());

#if ENABLE(JIT)
    if (!prepareFunctionForExecution(exec, m_codeBlockForConstruct, m_jitCodeForConstruct, m_jitCodeForConstructWithArityCheck, m_symbolTable, jitType, bytecodeIndex, CodeForConstruct))
        return 0;
#endif

#if ENABLE(JIT)
#if ENABLE(CLASSIC_INTERPRETER)
    if (!m_jitCodeForConstruct)
        Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_codeBlockForConstruct));
    else
#endif
    Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_codeBlockForConstruct) + m_jitCodeForConstruct.size());
#else
    Heap::heap(this)->reportExtraMemoryCost(sizeof(*m_codeBlockForConstruct));
#endif

    return 0;
}

#if ENABLE(JIT)
void FunctionExecutable::jettisonOptimizedCodeForCall(JSGlobalData& globalData)
{
    jettisonCodeBlock(globalData, m_codeBlockForCall);
    m_jitCodeForCall = m_codeBlockForCall->getJITCode();
    m_jitCodeForCallWithArityCheck = m_codeBlockForCall->getJITCodeWithArityCheck();
}

void FunctionExecutable::jettisonOptimizedCodeForConstruct(JSGlobalData& globalData)
{
    jettisonCodeBlock(globalData, m_codeBlockForConstruct);
    m_jitCodeForConstruct = m_codeBlockForConstruct->getJITCode();
    m_jitCodeForConstructWithArityCheck = m_codeBlockForConstruct->getJITCodeWithArityCheck();
}
#endif

void FunctionExecutable::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    FunctionExecutable* thisObject = jsCast<FunctionExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    ScriptExecutable::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_nameValue);
    visitor.append(&thisObject->m_symbolTable);
    if (thisObject->m_codeBlockForCall)
        thisObject->m_codeBlockForCall->visitAggregate(visitor);
    if (thisObject->m_codeBlockForConstruct)
        thisObject->m_codeBlockForConstruct->visitAggregate(visitor);
}

void FunctionExecutable::clearCodeIfNotCompiling()
{
    if (isCompiling())
        return;
    clearCode();
}

void FunctionExecutable::clearCode()
{
    m_codeBlockForCall.clear();
    m_codeBlockForConstruct.clear();
    Base::clearCode();
}

void FunctionExecutable::unlinkCalls()
{
#if ENABLE(JIT)
    if (!!m_jitCodeForCall) {
        ASSERT(m_codeBlockForCall);
        m_codeBlockForCall->unlinkCalls();
    }
    if (!!m_jitCodeForConstruct) {
        ASSERT(m_codeBlockForConstruct);
        m_codeBlockForConstruct->unlinkCalls();
    }
#endif
}

FunctionExecutable* FunctionExecutable::fromGlobalCode(const Identifier& name, ExecState* exec, Debugger* debugger, const SourceCode& source, JSObject** exception)
{
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    RefPtr<ProgramNode> program = parse<ProgramNode>(&exec->globalData(), lexicalGlobalObject, source, 0, Identifier(), JSParseNormal, ProgramNode::isFunctionNode ? JSParseFunctionCode : JSParseProgramCode, debugger, exec, exception);
    if (!program) {
        ASSERT(*exception);
        return 0;
    }

    // This function assumes an input string that would result in a single anonymous function expression.
    StatementNode* exprStatement = program->singleStatement();
    ASSERT(exprStatement);
    ASSERT(exprStatement->isExprStatement());
    ExpressionNode* funcExpr = static_cast<ExprStatementNode*>(exprStatement)->expr();
    ASSERT(funcExpr);
    ASSERT(funcExpr->isFuncExprNode());
    FunctionBodyNode* body = static_cast<FuncExprNode*>(funcExpr)->body();
    ASSERT(body);
    ASSERT(body->ident().isNull());

    FunctionExecutable* functionExecutable = FunctionExecutable::create(exec->globalData(), body);
    functionExecutable->m_nameValue.set(exec->globalData(), functionExecutable, jsString(&exec->globalData(), name.string()));
    return functionExecutable;
}

String FunctionExecutable::paramString() const
{
    FunctionParameters& parameters = *m_parameters;
    StringBuilder builder;
    for (size_t pos = 0; pos < parameters.size(); ++pos) {
        if (!builder.isEmpty())
            builder.appendLiteral(", ");
        builder.append(parameters[pos].string());
    }
    return builder.toString();
}

}
