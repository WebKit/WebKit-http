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
#include "JIT.h"
#include "Parser.h"
#include "UStringBuilder.h"
#include "Vector.h"

namespace JSC {

const ClassInfo ExecutableBase::s_info = { "Executable", 0, 0, 0, CREATE_METHOD_TABLE(ExecutableBase) };

void ExecutableBase::clearCode(JSCell* cell)
{
    static_cast<ExecutableBase*>(cell)->clearCodeVirtual();
}

void ExecutableBase::clearCodeVirtual()
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
DFG::Intrinsic ExecutableBase::intrinsic() const
{
    return DFG::NoIntrinsic;
}
#endif

const ClassInfo NativeExecutable::s_info = { "NativeExecutable", &ExecutableBase::s_info, 0, 0, CREATE_METHOD_TABLE(NativeExecutable) };

NativeExecutable::~NativeExecutable()
{
}

#if ENABLE(DFG_JIT)
DFG::Intrinsic NativeExecutable::intrinsic() const
{
    return m_intrinsic;
}
#endif

#if ENABLE(JIT)
// Utility method used for jettisoning code blocks.
template<typename T>
static void jettisonCodeBlock(JSGlobalData& globalData, OwnPtr<T>& codeBlock)
{
    ASSERT(codeBlock->getJITType() != JITCode::BaselineJIT);
    ASSERT(codeBlock->alternative());
    OwnPtr<T> codeBlockToJettison = codeBlock.release();
    codeBlock = static_pointer_cast<T>(codeBlockToJettison->releaseAlternative());
    codeBlockToJettison->unlinkIncomingCalls();
    globalData.heap.addJettisonedCodeBlock(static_pointer_cast<CodeBlock>(codeBlockToJettison.release()));
}
#endif

const ClassInfo ScriptExecutable::s_info = { "ScriptExecutable", &ExecutableBase::s_info, 0, 0, CREATE_METHOD_TABLE(ScriptExecutable) };

const ClassInfo EvalExecutable::s_info = { "EvalExecutable", &ScriptExecutable::s_info, 0, 0, CREATE_METHOD_TABLE(EvalExecutable) };

EvalExecutable::EvalExecutable(ExecState* exec, const SourceCode& source, bool inStrictContext)
    : ScriptExecutable(exec->globalData().evalExecutableStructure.get(), exec, source, inStrictContext)
{
}

EvalExecutable::~EvalExecutable()
{
}

const ClassInfo ProgramExecutable::s_info = { "ProgramExecutable", &ScriptExecutable::s_info, 0, 0, CREATE_METHOD_TABLE(ProgramExecutable) };

ProgramExecutable::ProgramExecutable(ExecState* exec, const SourceCode& source)
    : ScriptExecutable(exec->globalData().programExecutableStructure.get(), exec, source, false)
{
}

ProgramExecutable::~ProgramExecutable()
{
}

const ClassInfo FunctionExecutable::s_info = { "FunctionExecutable", &ScriptExecutable::s_info, 0, 0, CREATE_METHOD_TABLE(FunctionExecutable) };

FunctionExecutable::FunctionExecutable(JSGlobalData& globalData, const Identifier& name, const SourceCode& source, bool forceUsesArguments, FunctionParameters* parameters, bool inStrictContext)
    : ScriptExecutable(globalData.functionExecutableStructure.get(), globalData, source, inStrictContext)
    , m_numCapturedVariables(0)
    , m_forceUsesArguments(forceUsesArguments)
    , m_parameters(parameters)
    , m_name(name)
    , m_symbolTable(0)
{
}

FunctionExecutable::FunctionExecutable(ExecState* exec, const Identifier& name, const SourceCode& source, bool forceUsesArguments, FunctionParameters* parameters, bool inStrictContext)
    : ScriptExecutable(exec->globalData().functionExecutableStructure.get(), exec, source, inStrictContext)
    , m_numCapturedVariables(0)
    , m_forceUsesArguments(forceUsesArguments)
    , m_parameters(parameters)
    , m_name(name)
    , m_symbolTable(0)
{
}

JSObject* EvalExecutable::compileOptimized(ExecState* exec, ScopeChainNode* scopeChainNode)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_evalCodeBlock);
    JSObject* error = 0;
    if (m_evalCodeBlock->getJITType() != JITCode::topTierJIT())
        error = compileInternal(exec, scopeChainNode, JITCode::nextTierJIT(m_evalCodeBlock->getJITType()));
    ASSERT(!!m_evalCodeBlock);
    return error;
}

JSObject* EvalExecutable::compileInternal(ExecState* exec, ScopeChainNode* scopeChainNode, JITCode::JITType jitType)
{
#if !ENABLE(JIT)
    UNUSED_PARAM(jitType);
#endif
    JSObject* exception = 0;
    JSGlobalData* globalData = &exec->globalData();
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    if (!lexicalGlobalObject->evalEnabled())
        return throwError(exec, createEvalError(exec, "Eval is disabled"));
    RefPtr<EvalNode> evalNode = globalData->parser->parse<EvalNode>(lexicalGlobalObject, lexicalGlobalObject->debugger(), exec, m_source, 0, isStrictMode() ? JSParseStrict : JSParseNormal, &exception);
    if (!evalNode) {
        ASSERT(exception);
        return exception;
    }
    recordParse(evalNode->features(), evalNode->hasCapturedVariables(), evalNode->lineNo(), evalNode->lastLine());

    JSGlobalObject* globalObject = scopeChainNode->globalObject.get();

    OwnPtr<CodeBlock> previousCodeBlock = m_evalCodeBlock.release();
    ASSERT((jitType == JITCode::bottomTierJIT()) == !previousCodeBlock);
    m_evalCodeBlock = adoptPtr(new EvalCodeBlock(this, globalObject, source().provider(), scopeChainNode->localDepth(), previousCodeBlock.release()));
    OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(evalNode.get(), scopeChainNode, m_evalCodeBlock->symbolTable(), m_evalCodeBlock.get(), !!m_evalCodeBlock->alternative() ? BytecodeGenerator::OptimizingCompilation : BytecodeGenerator::FirstCompilation)));
    if ((exception = generator->generate())) {
        m_evalCodeBlock = static_pointer_cast<EvalCodeBlock>(m_evalCodeBlock->releaseAlternative());
        evalNode->destroyData();
        return exception;
    }

    evalNode->destroyData();
    m_evalCodeBlock->copyDataFromAlternative();

#if ENABLE(JIT)
    if (exec->globalData().canUseJIT()) {
        bool dfgCompiled = false;
        if (jitType == JITCode::DFGJIT)
            dfgCompiled = DFG::tryCompile(exec, m_evalCodeBlock.get(), m_jitCodeForCall);
        if (dfgCompiled)
            ASSERT(!m_evalCodeBlock->alternative() || !m_evalCodeBlock->alternative()->hasIncomingCalls());
        else {
            if (m_evalCodeBlock->alternative()) {
                // There is already an alternative piece of code compiled with a different
                // JIT, so we can silently fail.
                m_evalCodeBlock = static_pointer_cast<EvalCodeBlock>(m_evalCodeBlock->releaseAlternative());
                return 0;
            }
            m_jitCodeForCall = JIT::compile(scopeChainNode->globalData, m_evalCodeBlock.get());
        }
#if !ENABLE(OPCODE_SAMPLING)
        if (!BytecodeGenerator::dumpsGeneratedCode())
            m_evalCodeBlock->discardBytecode();
#endif
        m_evalCodeBlock->setJITCode(m_jitCodeForCall, MacroAssemblerCodePtr());
    }
#endif

#if ENABLE(JIT)
#if ENABLE(INTERPRETER)
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
    EvalExecutable* thisObject = static_cast<EvalExecutable*>(cell);
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

void EvalExecutable::clearCodeVirtual()
{
    if (m_evalCodeBlock) {
        m_evalCodeBlock->clearEvalCache();
        m_evalCodeBlock.clear();
    }
    Base::clearCodeVirtual();
}

JSObject* ProgramExecutable::checkSyntax(ExecState* exec)
{
    JSObject* exception = 0;
    JSGlobalData* globalData = &exec->globalData();
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    RefPtr<ProgramNode> programNode = globalData->parser->parse<ProgramNode>(lexicalGlobalObject, lexicalGlobalObject->debugger(), exec, m_source, 0, JSParseNormal, &exception);
    if (programNode)
        return 0;
    ASSERT(exception);
    return exception;
}

JSObject* ProgramExecutable::compileOptimized(ExecState* exec, ScopeChainNode* scopeChainNode)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_programCodeBlock);
    JSObject* error = 0;
    if (m_programCodeBlock->getJITType() != JITCode::topTierJIT())
        error = compileInternal(exec, scopeChainNode, JITCode::nextTierJIT(m_programCodeBlock->getJITType()));
    ASSERT(!!m_programCodeBlock);
    return error;
}

JSObject* ProgramExecutable::compileInternal(ExecState* exec, ScopeChainNode* scopeChainNode, JITCode::JITType jitType)
{
#if !ENABLE(JIT)
    UNUSED_PARAM(jitType);
#endif
    JSObject* exception = 0;
    JSGlobalData* globalData = &exec->globalData();
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    RefPtr<ProgramNode> programNode = globalData->parser->parse<ProgramNode>(lexicalGlobalObject, lexicalGlobalObject->debugger(), exec, m_source, 0, isStrictMode() ? JSParseStrict : JSParseNormal, &exception);
    if (!programNode) {
        ASSERT(exception);
        return exception;
    }
    recordParse(programNode->features(), programNode->hasCapturedVariables(), programNode->lineNo(), programNode->lastLine());

    JSGlobalObject* globalObject = scopeChainNode->globalObject.get();
    
    OwnPtr<CodeBlock> previousCodeBlock = m_programCodeBlock.release();
    ASSERT((jitType == JITCode::bottomTierJIT()) == !previousCodeBlock);
    m_programCodeBlock = adoptPtr(new ProgramCodeBlock(this, GlobalCode, globalObject, source().provider(), previousCodeBlock.release()));
    OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(programNode.get(), scopeChainNode, &globalObject->symbolTable(), m_programCodeBlock.get(), !!m_programCodeBlock->alternative() ? BytecodeGenerator::OptimizingCompilation : BytecodeGenerator::FirstCompilation)));
    if ((exception = generator->generate())) {
        m_programCodeBlock = static_pointer_cast<ProgramCodeBlock>(m_programCodeBlock->releaseAlternative());
        programNode->destroyData();
        return exception;
    }

    programNode->destroyData();
    m_programCodeBlock->copyDataFromAlternative();

#if ENABLE(JIT)
    if (exec->globalData().canUseJIT()) {
        bool dfgCompiled = false;
        if (jitType == JITCode::DFGJIT)
            dfgCompiled = DFG::tryCompile(exec, m_programCodeBlock.get(), m_jitCodeForCall);
        if (dfgCompiled) {
            if (m_programCodeBlock->alternative())
                m_programCodeBlock->alternative()->unlinkIncomingCalls();
        } else {
            if (m_programCodeBlock->alternative()) {
                m_programCodeBlock = static_pointer_cast<ProgramCodeBlock>(m_programCodeBlock->releaseAlternative());
                return 0;
            }
            m_jitCodeForCall = JIT::compile(scopeChainNode->globalData, m_programCodeBlock.get());
        }
#if !ENABLE(OPCODE_SAMPLING)
        if (!BytecodeGenerator::dumpsGeneratedCode())
            m_programCodeBlock->discardBytecode();
#endif
        m_programCodeBlock->setJITCode(m_jitCodeForCall, MacroAssemblerCodePtr());
    }
#endif

#if ENABLE(JIT)
#if ENABLE(INTERPRETER)
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
    ProgramExecutable* thisObject = static_cast<ProgramExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    ScriptExecutable::visitChildren(thisObject, visitor);
    if (thisObject->m_programCodeBlock)
        thisObject->m_programCodeBlock->visitAggregate(visitor);
}

void ProgramExecutable::clearCodeVirtual()
{
    if (m_programCodeBlock) {
        m_programCodeBlock->clearEvalCache();
        m_programCodeBlock.clear();
    }
    Base::clearCodeVirtual();
}

JSObject* FunctionExecutable::compileOptimizedForCall(ExecState* exec, ScopeChainNode* scopeChainNode, ExecState* calleeArgsExec)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_codeBlockForCall);
    JSObject* error = 0;
    if (m_codeBlockForCall->getJITType() != JITCode::topTierJIT())
        error = compileForCallInternal(exec, scopeChainNode, calleeArgsExec, JITCode::nextTierJIT(m_codeBlockForCall->getJITType()));
    ASSERT(!!m_codeBlockForCall);
    return error;
}

JSObject* FunctionExecutable::compileOptimizedForConstruct(ExecState* exec, ScopeChainNode* scopeChainNode, ExecState* calleeArgsExec)
{
    ASSERT(exec->globalData().dynamicGlobalObject);
    ASSERT(!!m_codeBlockForConstruct);
    JSObject* error = 0;
    if (m_codeBlockForConstruct->getJITType() != JITCode::topTierJIT())
        error = compileForConstructInternal(exec, scopeChainNode, calleeArgsExec, JITCode::nextTierJIT(m_codeBlockForConstruct->getJITType()));
    ASSERT(!!m_codeBlockForConstruct);
    return error;
}

JSObject* FunctionExecutable::compileForCallInternal(ExecState* exec, ScopeChainNode* scopeChainNode, ExecState* calleeArgsExec, JITCode::JITType jitType)
{
#if !ENABLE(JIT)
    UNUSED_PARAM(jitType);
#endif
    JSObject* exception = 0;
    JSGlobalData* globalData = scopeChainNode->globalData;
    RefPtr<FunctionBodyNode> body = globalData->parser->parse<FunctionBodyNode>(exec->lexicalGlobalObject(), 0, 0, m_source, m_parameters.get(), isStrictMode() ? JSParseStrict : JSParseNormal, &exception);
    if (!body) {
        ASSERT(exception);
        return exception;
    }
    if (m_forceUsesArguments)
        body->setUsesArguments();
    body->finishParsing(m_parameters, m_name);
    recordParse(body->features(), body->hasCapturedVariables(), body->lineNo(), body->lastLine());

    JSGlobalObject* globalObject = scopeChainNode->globalObject.get();

    OwnPtr<CodeBlock> previousCodeBlock = m_codeBlockForCall.release();
    ASSERT((jitType == JITCode::bottomTierJIT()) == !previousCodeBlock);
    m_codeBlockForCall = adoptPtr(new FunctionCodeBlock(this, FunctionCode, globalObject, source().provider(), source().startOffset(), false, previousCodeBlock.release()));
    OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(body.get(), scopeChainNode, m_codeBlockForCall->symbolTable(), m_codeBlockForCall.get(), !!m_codeBlockForCall->alternative() ? BytecodeGenerator::OptimizingCompilation : BytecodeGenerator::FirstCompilation)));
    if ((exception = generator->generate())) {
        m_codeBlockForCall = static_pointer_cast<FunctionCodeBlock>(m_codeBlockForCall->releaseAlternative());
        body->destroyData();
        return exception;
    }

    m_numParametersForCall = m_codeBlockForCall->m_numParameters;
    ASSERT(m_numParametersForCall);
    m_numCapturedVariables = m_codeBlockForCall->m_numCapturedVars;
    m_symbolTable = m_codeBlockForCall->sharedSymbolTable();

    body->destroyData();
    m_codeBlockForCall->copyDataFromAlternative();

#if ENABLE(JIT)
    if (exec->globalData().canUseJIT()) {
        bool dfgCompiled = false;
        if (jitType == JITCode::DFGJIT)
            dfgCompiled = DFG::tryCompileFunction(exec, calleeArgsExec, m_codeBlockForCall.get(), m_jitCodeForCall, m_jitCodeForCallWithArityCheck);
        if (dfgCompiled) {
            if (m_codeBlockForCall->alternative())
                m_codeBlockForCall->alternative()->unlinkIncomingCalls();
        } else {
            if (m_codeBlockForCall->alternative()) {
                m_codeBlockForCall = static_pointer_cast<FunctionCodeBlock>(m_codeBlockForCall->releaseAlternative());
                m_symbolTable = m_codeBlockForCall->sharedSymbolTable();
                return 0;
            }
            m_jitCodeForCall = JIT::compile(scopeChainNode->globalData, m_codeBlockForCall.get(), &m_jitCodeForCallWithArityCheck);
        }
#if !ENABLE(OPCODE_SAMPLING)
        if (!BytecodeGenerator::dumpsGeneratedCode())
            m_codeBlockForCall->discardBytecode();
#endif
        
        m_codeBlockForCall->setJITCode(m_jitCodeForCall, m_jitCodeForCallWithArityCheck);
    }
#else
    UNUSED_PARAM(calleeArgsExec);
#endif

#if ENABLE(JIT)
#if ENABLE(INTERPRETER)
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

JSObject* FunctionExecutable::compileForConstructInternal(ExecState* exec, ScopeChainNode* scopeChainNode, ExecState* calleeArgsExec, JITCode::JITType jitType)
{
    UNUSED_PARAM(jitType);
    
    JSObject* exception = 0;
    JSGlobalData* globalData = scopeChainNode->globalData;
    RefPtr<FunctionBodyNode> body = globalData->parser->parse<FunctionBodyNode>(exec->lexicalGlobalObject(), 0, 0, m_source, m_parameters.get(), isStrictMode() ? JSParseStrict : JSParseNormal, &exception);
    if (!body) {
        ASSERT(exception);
        return exception;
    }
    if (m_forceUsesArguments)
        body->setUsesArguments();
    body->finishParsing(m_parameters, m_name);
    recordParse(body->features(), body->hasCapturedVariables(), body->lineNo(), body->lastLine());

    JSGlobalObject* globalObject = scopeChainNode->globalObject.get();

    OwnPtr<CodeBlock> previousCodeBlock = m_codeBlockForConstruct.release();
    ASSERT((jitType == JITCode::bottomTierJIT()) == !previousCodeBlock);
    m_codeBlockForConstruct = adoptPtr(new FunctionCodeBlock(this, FunctionCode, globalObject, source().provider(), source().startOffset(), true, previousCodeBlock.release()));
    OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(body.get(), scopeChainNode, m_codeBlockForConstruct->symbolTable(), m_codeBlockForConstruct.get(), !!m_codeBlockForConstruct->alternative() ? BytecodeGenerator::OptimizingCompilation : BytecodeGenerator::FirstCompilation)));
    if ((exception = generator->generate())) {
        m_codeBlockForConstruct = static_pointer_cast<FunctionCodeBlock>(m_codeBlockForConstruct->releaseAlternative());
        body->destroyData();
        return exception;
    }

    m_numParametersForConstruct = m_codeBlockForConstruct->m_numParameters;
    ASSERT(m_numParametersForConstruct);
    m_numCapturedVariables = m_codeBlockForConstruct->m_numCapturedVars;
    m_symbolTable = m_codeBlockForConstruct->sharedSymbolTable();

    body->destroyData();
    m_codeBlockForConstruct->copyDataFromAlternative();

#if ENABLE(JIT)
    if (exec->globalData().canUseJIT()) {
        bool dfgCompiled = false;
        if (jitType == JITCode::DFGJIT)
            dfgCompiled = DFG::tryCompileFunction(exec, calleeArgsExec, m_codeBlockForConstruct.get(), m_jitCodeForConstruct, m_jitCodeForConstructWithArityCheck);
        if (dfgCompiled) {
            if (m_codeBlockForConstruct->alternative())
                m_codeBlockForConstruct->alternative()->unlinkIncomingCalls();
        } else {
            if (m_codeBlockForConstruct->alternative()) {
                m_codeBlockForConstruct = static_pointer_cast<FunctionCodeBlock>(m_codeBlockForConstruct->releaseAlternative());
                m_symbolTable = m_codeBlockForConstruct->sharedSymbolTable();
                return 0;
            }
            m_jitCodeForConstruct = JIT::compile(scopeChainNode->globalData, m_codeBlockForConstruct.get(), &m_jitCodeForConstructWithArityCheck);
        }
#if !ENABLE(OPCODE_SAMPLING)
        if (!BytecodeGenerator::dumpsGeneratedCode())
            m_codeBlockForConstruct->discardBytecode();
#endif
        
        m_codeBlockForConstruct->setJITCode(m_jitCodeForConstruct, m_jitCodeForConstructWithArityCheck);
    }
#else
    UNUSED_PARAM(calleeArgsExec);
#endif

#if ENABLE(JIT)
#if ENABLE(INTERPRETER)
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
    FunctionExecutable* thisObject = static_cast<FunctionExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    ScriptExecutable::visitChildren(thisObject, visitor);
    if (thisObject->m_nameValue)
        visitor.append(&thisObject->m_nameValue);
    if (thisObject->m_codeBlockForCall)
        thisObject->m_codeBlockForCall->visitAggregate(visitor);
    if (thisObject->m_codeBlockForConstruct)
        thisObject->m_codeBlockForConstruct->visitAggregate(visitor);
}

void FunctionExecutable::discardCode()
{
#if ENABLE(JIT)
    // These first two checks are to handle the rare case where
    // we are trying to evict code for a function during its
    // codegen.
    if (!m_jitCodeForCall && m_codeBlockForCall)
        return;
    if (!m_jitCodeForConstruct && m_codeBlockForConstruct)
        return;
#endif
    clearCodeVirtual();
}

void FunctionExecutable::clearCodeVirtual()
{
    if (m_codeBlockForCall) {
        m_codeBlockForCall->clearEvalCache();
        m_codeBlockForCall.clear();
    }
    if (m_codeBlockForConstruct) {
        m_codeBlockForConstruct->clearEvalCache();
        m_codeBlockForConstruct.clear();
    }
    Base::clearCodeVirtual();
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

FunctionExecutable* FunctionExecutable::fromGlobalCode(const Identifier& functionName, ExecState* exec, Debugger* debugger, const SourceCode& source, JSObject** exception)
{
    JSGlobalObject* lexicalGlobalObject = exec->lexicalGlobalObject();
    RefPtr<ProgramNode> program = exec->globalData().parser->parse<ProgramNode>(lexicalGlobalObject, debugger, exec, source, 0, JSParseNormal, exception);
    if (!program) {
        ASSERT(*exception);
        return 0;
    }

    // Uses of this function that would not result in a single function expression are invalid.
    StatementNode* exprStatement = program->singleStatement();
    ASSERT(exprStatement);
    ASSERT(exprStatement->isExprStatement());
    ExpressionNode* funcExpr = static_cast<ExprStatementNode*>(exprStatement)->expr();
    ASSERT(funcExpr);
    ASSERT(funcExpr->isFuncExprNode());
    FunctionBodyNode* body = static_cast<FuncExprNode*>(funcExpr)->body();
    ASSERT(body);

    return FunctionExecutable::create(exec->globalData(), functionName, body->source(), body->usesArguments(), body->parameters(), body->isStrictMode(), body->lineNo(), body->lastLine());
}

UString FunctionExecutable::paramString() const
{
    FunctionParameters& parameters = *m_parameters;
    UStringBuilder builder;
    for (size_t pos = 0; pos < parameters.size(); ++pos) {
        if (!builder.isEmpty())
            builder.append(", ");
        builder.append(parameters[pos].ustring());
    }
    return builder.toUString();
}

}
