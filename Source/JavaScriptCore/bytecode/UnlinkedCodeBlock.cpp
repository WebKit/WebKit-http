/*
 * Copyright (C) 2012, 2013 Apple Inc. All Rights Reserved.
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

#include "UnlinkedCodeBlock.h"

#include "BytecodeGenerator.h"
#include "ClassInfo.h"
#include "CodeCache.h"
#include "Executable.h"
#include "JSString.h"
#include "JSCInlines.h"
#include "Parser.h"
#include "SourceProvider.h"
#include "Structure.h"
#include "SymbolTable.h"
#include "UnlinkedInstructionStream.h"
#include <wtf/DataLog.h>

namespace JSC {

const ClassInfo UnlinkedFunctionExecutable::s_info = { "UnlinkedFunctionExecutable", 0, 0, CREATE_METHOD_TABLE(UnlinkedFunctionExecutable) };
const ClassInfo UnlinkedCodeBlock::s_info = { "UnlinkedCodeBlock", 0, 0, CREATE_METHOD_TABLE(UnlinkedCodeBlock) };
const ClassInfo UnlinkedGlobalCodeBlock::s_info = { "UnlinkedGlobalCodeBlock", &Base::s_info, 0, CREATE_METHOD_TABLE(UnlinkedGlobalCodeBlock) };
const ClassInfo UnlinkedProgramCodeBlock::s_info = { "UnlinkedProgramCodeBlock", &Base::s_info, 0, CREATE_METHOD_TABLE(UnlinkedProgramCodeBlock) };
const ClassInfo UnlinkedEvalCodeBlock::s_info = { "UnlinkedEvalCodeBlock", &Base::s_info, 0, CREATE_METHOD_TABLE(UnlinkedEvalCodeBlock) };
const ClassInfo UnlinkedFunctionCodeBlock::s_info = { "UnlinkedFunctionCodeBlock", &Base::s_info, 0, CREATE_METHOD_TABLE(UnlinkedFunctionCodeBlock) };

static UnlinkedFunctionCodeBlock* generateFunctionCodeBlock(VM& vm, UnlinkedFunctionExecutable* executable, const SourceCode& source, CodeSpecializationKind kind, DebuggerMode debuggerMode, ProfilerMode profilerMode, UnlinkedFunctionKind functionKind, bool bodyIncludesBraces, ParserError& error)
{
    RefPtr<FunctionBodyNode> body = parse<FunctionBodyNode>(&vm, source, executable->parameters(), executable->name(), executable->toStrictness(), JSParseFunctionCode, error, 0, bodyIncludesBraces);

    if (!body) {
        ASSERT(error.m_type != ParserError::ErrorNone);
        return 0;
    }

    if (executable->forceUsesArguments())
        body->setUsesArguments();
    body->finishParsing(executable->parameters(), executable->name(), executable->functionMode());
    executable->recordParse(body->features(), body->hasCapturedVariables());
    
    UnlinkedFunctionCodeBlock* result = UnlinkedFunctionCodeBlock::create(&vm, FunctionCode, ExecutableInfo(body->needsActivation(), body->usesEval(), body->isStrictMode(), kind == CodeForConstruct, functionKind == UnlinkedBuiltinFunction));
    OwnPtr<BytecodeGenerator> generator(adoptPtr(new BytecodeGenerator(vm, body.get(), result, debuggerMode, profilerMode)));
    error = generator->generate();
    body->destroyData();
    if (error.m_type != ParserError::ErrorNone)
        return 0;
    return result;
}

unsigned UnlinkedCodeBlock::addOrFindConstant(JSValue v)
{
    unsigned numberOfConstants = numberOfConstantRegisters();
    for (unsigned i = 0; i < numberOfConstants; ++i) {
        if (getConstant(FirstConstantRegisterIndex + i) == v)
            return i;
    }
    return addConstant(v);
}

UnlinkedFunctionExecutable::UnlinkedFunctionExecutable(VM* vm, Structure* structure, const SourceCode& source, FunctionBodyNode* node, UnlinkedFunctionKind kind)
    : Base(*vm, structure)
    , m_numCapturedVariables(node->capturedVariableCount())
    , m_forceUsesArguments(node->usesArguments())
    , m_isInStrictContext(node->isStrictMode())
    , m_hasCapturedVariables(node->hasCapturedVariables())
    , m_isBuiltinFunction(kind == UnlinkedBuiltinFunction)
    , m_name(node->ident())
    , m_inferredName(node->inferredName())
    , m_parameters(node->parameters())
    , m_firstLineOffset(node->firstLine() - source.firstLine())
    , m_lineCount(node->lastLine() - node->firstLine())
    , m_unlinkedFunctionNameStart(node->functionNameStart() - source.startOffset())
    , m_unlinkedBodyStartColumn(node->startColumn())
    , m_unlinkedBodyEndColumn(m_lineCount ? node->endColumn() : node->endColumn() - node->startColumn())
    , m_startOffset(node->source().startOffset() - source.startOffset())
    , m_sourceLength(node->source().length())
    , m_typeProfilingStartOffset(node->functionNameStart())
    , m_typeProfilingEndOffset(node->startStartOffset() + node->source().length() - 1)
    , m_features(node->features())
    , m_functionMode(node->functionMode())
{
}

size_t UnlinkedFunctionExecutable::parameterCount() const
{
    return m_parameters->size();
}

void UnlinkedFunctionExecutable::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    UnlinkedFunctionExecutable* thisObject = jsCast<UnlinkedFunctionExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_codeBlockForCall);
    visitor.append(&thisObject->m_codeBlockForConstruct);
    visitor.append(&thisObject->m_nameValue);
    visitor.append(&thisObject->m_symbolTableForCall);
    visitor.append(&thisObject->m_symbolTableForConstruct);
}

FunctionExecutable* UnlinkedFunctionExecutable::link(VM& vm, const SourceCode& source, size_t lineOffset)
{
    unsigned firstLine = lineOffset + m_firstLineOffset;
    bool startColumnIsOnFirstSourceLine = !m_firstLineOffset;
    unsigned startColumn = m_unlinkedBodyStartColumn + (startColumnIsOnFirstSourceLine ? source.startColumn() : 1);
    bool endColumnIsOnStartLine = !m_lineCount;
    unsigned endColumn = m_unlinkedBodyEndColumn + (endColumnIsOnStartLine ? startColumn : 1);
    SourceCode code(source.provider(), m_startOffset, m_startOffset + m_sourceLength, firstLine, startColumn);
    return FunctionExecutable::create(vm, code, this, firstLine, firstLine + m_lineCount, startColumn, endColumn);
}

UnlinkedFunctionExecutable* UnlinkedFunctionExecutable::fromGlobalCode(const Identifier& name, ExecState* exec, Debugger*, const SourceCode& source, JSObject** exception)
{
    ParserError error;
    VM& vm = exec->vm();
    CodeCache* codeCache = vm.codeCache();
    UnlinkedFunctionExecutable* executable = codeCache->getFunctionExecutableFromGlobalCode(vm, name, source, error);

    if (exec->lexicalGlobalObject()->hasDebugger())
        exec->lexicalGlobalObject()->debugger()->sourceParsed(exec, source.provider(), error.m_line, error.m_message);

    if (error.m_type != ParserError::ErrorNone) {
        *exception = error.toErrorObject(exec->lexicalGlobalObject(), source);
        return 0;
    }

    return executable;
}

UnlinkedFunctionCodeBlock* UnlinkedFunctionExecutable::codeBlockFor(VM& vm, const SourceCode& source, CodeSpecializationKind specializationKind, DebuggerMode debuggerMode, ProfilerMode profilerMode, bool bodyIncludesBraces, ParserError& error)
{
    switch (specializationKind) {
    case CodeForCall:
        if (UnlinkedFunctionCodeBlock* codeBlock = m_codeBlockForCall.get())
            return codeBlock;
        break;
    case CodeForConstruct:
        if (UnlinkedFunctionCodeBlock* codeBlock = m_codeBlockForConstruct.get())
            return codeBlock;
        break;
    }

    UnlinkedFunctionCodeBlock* result = generateFunctionCodeBlock(vm, this, source, specializationKind, debuggerMode, profilerMode, isBuiltinFunction() ? UnlinkedBuiltinFunction : UnlinkedNormalFunction, bodyIncludesBraces, error);
    
    if (error.m_type != ParserError::ErrorNone)
        return 0;

    switch (specializationKind) {
    case CodeForCall:
        m_codeBlockForCall.set(vm, this, result);
        m_symbolTableForCall.set(vm, this, result->symbolTable());
        break;
    case CodeForConstruct:
        m_codeBlockForConstruct.set(vm, this, result);
        m_symbolTableForConstruct.set(vm, this, result->symbolTable());
        break;
    }
    return result;
}

String UnlinkedFunctionExecutable::paramString() const
{
    FunctionParameters& parameters = *m_parameters;
    StringBuilder builder;
    for (size_t pos = 0; pos < parameters.size(); ++pos) {
        if (!builder.isEmpty())
            builder.appendLiteral(", ");
        parameters.at(pos)->toString(builder);
    }
    return builder.toString();
}

UnlinkedCodeBlock::UnlinkedCodeBlock(VM* vm, Structure* structure, CodeType codeType, const ExecutableInfo& info)
    : Base(*vm, structure)
    , m_numVars(0)
    , m_numCalleeRegisters(0)
    , m_numParameters(0)
    , m_vm(vm)
    , m_argumentsRegister(VirtualRegister())
    , m_globalObjectRegister(VirtualRegister())
    , m_needsFullScopeChain(info.m_needsActivation)
    , m_usesEval(info.m_usesEval)
    , m_isNumericCompareFunction(false)
    , m_isStrictMode(info.m_isStrictMode)
    , m_isConstructor(info.m_isConstructor)
    , m_hasCapturedVariables(false)
    , m_isBuiltinFunction(info.m_isBuiltinFunction)
    , m_firstLine(0)
    , m_lineCount(0)
    , m_endColumn(UINT_MAX)
    , m_features(0)
    , m_codeType(codeType)
    , m_arrayProfileCount(0)
    , m_arrayAllocationProfileCount(0)
    , m_objectAllocationProfileCount(0)
    , m_valueProfileCount(0)
    , m_llintCallLinkInfoCount(0)
#if ENABLE(BYTECODE_COMMENTS)
    , m_bytecodeCommentIterator(0)
#endif
{

}

void UnlinkedCodeBlock::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    UnlinkedCodeBlock* thisObject = jsCast<UnlinkedCodeBlock*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_symbolTable);
    for (FunctionExpressionVector::iterator ptr = thisObject->m_functionDecls.begin(), end = thisObject->m_functionDecls.end(); ptr != end; ++ptr)
        visitor.append(ptr);
    for (FunctionExpressionVector::iterator ptr = thisObject->m_functionExprs.begin(), end = thisObject->m_functionExprs.end(); ptr != end; ++ptr)
        visitor.append(ptr);
    visitor.appendValues(thisObject->m_constantRegisters.data(), thisObject->m_constantRegisters.size());
    if (thisObject->m_rareData) {
        for (size_t i = 0, end = thisObject->m_rareData->m_regexps.size(); i != end; i++)
            visitor.append(&thisObject->m_rareData->m_regexps[i]);
    }
}

int UnlinkedCodeBlock::lineNumberForBytecodeOffset(unsigned bytecodeOffset)
{
    ASSERT(bytecodeOffset < instructions().count());
    int divot;
    int startOffset;
    int endOffset;
    unsigned line;
    unsigned column;
    expressionRangeForBytecodeOffset(bytecodeOffset, divot, startOffset, endOffset, line, column);
    return line;
}

inline void UnlinkedCodeBlock::getLineAndColumn(ExpressionRangeInfo& info,
    unsigned& line, unsigned& column)
{
    switch (info.mode) {
    case ExpressionRangeInfo::FatLineMode:
        info.decodeFatLineMode(line, column);
        break;
    case ExpressionRangeInfo::FatColumnMode:
        info.decodeFatColumnMode(line, column);
        break;
    case ExpressionRangeInfo::FatLineAndColumnMode: {
        unsigned fatIndex = info.position;
        ExpressionRangeInfo::FatPosition& fatPos = m_rareData->m_expressionInfoFatPositions[fatIndex];
        line = fatPos.line;
        column = fatPos.column;
        break;
    }
    } // switch
}

#ifndef NDEBUG
static void dumpLineColumnEntry(size_t index, const UnlinkedInstructionStream& instructionStream, unsigned instructionOffset, unsigned line, unsigned column)
{
    const auto& instructions = instructionStream.unpackForDebugging();
    OpcodeID opcode = instructions[instructionOffset].u.opcode;
    const char* event = "";
    if (opcode == op_debug) {
        switch (instructions[instructionOffset + 1].u.operand) {
        case WillExecuteProgram: event = " WillExecuteProgram"; break;
        case DidExecuteProgram: event = " DidExecuteProgram"; break;
        case DidEnterCallFrame: event = " DidEnterCallFrame"; break;
        case DidReachBreakpoint: event = " DidReachBreakpoint"; break;
        case WillLeaveCallFrame: event = " WillLeaveCallFrame"; break;
        case WillExecuteStatement: event = " WillExecuteStatement"; break;
        }
    }
    dataLogF("  [%zu] pc %u @ line %u col %u : %s%s\n", index, instructionOffset, line, column, opcodeNames[opcode], event);
}

void UnlinkedCodeBlock::dumpExpressionRangeInfo()
{
    Vector<ExpressionRangeInfo>& expressionInfo = m_expressionInfo;

    size_t size = m_expressionInfo.size();
    dataLogF("UnlinkedCodeBlock %p expressionRangeInfo[%zu] {\n", this, size);
    for (size_t i = 0; i < size; i++) {
        ExpressionRangeInfo& info = expressionInfo[i];
        unsigned line;
        unsigned column;
        getLineAndColumn(info, line, column);
        dumpLineColumnEntry(i, instructions(), info.instructionOffset, line, column);
    }
    dataLog("}\n");
}
#endif

void UnlinkedCodeBlock::expressionRangeForBytecodeOffset(unsigned bytecodeOffset,
    int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column)
{
    ASSERT(bytecodeOffset < instructions().count());

    if (!m_expressionInfo.size()) {
        startOffset = 0;
        endOffset = 0;
        divot = 0;
        line = 0;
        column = 0;
        return;
    }

    Vector<ExpressionRangeInfo>& expressionInfo = m_expressionInfo;

    int low = 0;
    int high = expressionInfo.size();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (expressionInfo[mid].instructionOffset <= bytecodeOffset)
            low = mid + 1;
        else
            high = mid;
    }

    if (!low)
        low = 1;

    ExpressionRangeInfo& info = expressionInfo[low - 1];
    startOffset = info.startOffset;
    endOffset = info.endOffset;
    divot = info.divotPoint;
    getLineAndColumn(info, line, column);
}

void UnlinkedCodeBlock::addExpressionInfo(unsigned instructionOffset,
    int divot, int startOffset, int endOffset, unsigned line, unsigned column)
{
    if (divot > ExpressionRangeInfo::MaxDivot) {
        // Overflow has occurred, we can only give line number info for errors for this region
        divot = 0;
        startOffset = 0;
        endOffset = 0;
    } else if (startOffset > ExpressionRangeInfo::MaxOffset) {
        // If the start offset is out of bounds we clear both offsets
        // so we only get the divot marker. Error message will have to be reduced
        // to line and charPosition number.
        startOffset = 0;
        endOffset = 0;
    } else if (endOffset > ExpressionRangeInfo::MaxOffset) {
        // The end offset is only used for additional context, and is much more likely
        // to overflow (eg. function call arguments) so we are willing to drop it without
        // dropping the rest of the range.
        endOffset = 0;
    }

    unsigned positionMode =
        (line <= ExpressionRangeInfo::MaxFatLineModeLine && column <= ExpressionRangeInfo::MaxFatLineModeColumn) 
        ? ExpressionRangeInfo::FatLineMode
        : (line <= ExpressionRangeInfo::MaxFatColumnModeLine && column <= ExpressionRangeInfo::MaxFatColumnModeColumn)
        ? ExpressionRangeInfo::FatColumnMode
        : ExpressionRangeInfo::FatLineAndColumnMode;

    ExpressionRangeInfo info;
    info.instructionOffset = instructionOffset;
    info.divotPoint = divot;
    info.startOffset = startOffset;
    info.endOffset = endOffset;

    info.mode = positionMode;
    switch (positionMode) {
    case ExpressionRangeInfo::FatLineMode:
        info.encodeFatLineMode(line, column);
        break;
    case ExpressionRangeInfo::FatColumnMode:
        info.encodeFatColumnMode(line, column);
        break;
    case ExpressionRangeInfo::FatLineAndColumnMode: {
        createRareDataIfNecessary();
        unsigned fatIndex = m_rareData->m_expressionInfoFatPositions.size();
        ExpressionRangeInfo::FatPosition fatPos = { line, column };
        m_rareData->m_expressionInfoFatPositions.append(fatPos);
        info.position = fatIndex;
    }
    } // switch

    m_expressionInfo.append(info);
}

bool UnlinkedCodeBlock::typeProfilerExpressionInfoForBytecodeOffset(unsigned bytecodeOffset, unsigned& startDivot, unsigned& endDivot)
{
    static const bool verbose = false;
    auto iter = m_typeProfilerInfoMap.find(bytecodeOffset);
    if (iter == m_typeProfilerInfoMap.end()) {
        if (verbose)
            dataLogF("Don't have assignment info for offset:%u\n", bytecodeOffset);
        startDivot = UINT_MAX;
        endDivot = UINT_MAX;
        return false;
    }
    
    TypeProfilerExpressionRange& range = iter->value;
    startDivot = range.m_startDivot;
    endDivot = range.m_endDivot;
    return true;
}

void UnlinkedCodeBlock::addTypeProfilerExpressionInfo(unsigned instructionOffset, unsigned startDivot, unsigned endDivot)
{
    TypeProfilerExpressionRange range;
    range.m_startDivot = startDivot;
    range.m_endDivot = endDivot;
    m_typeProfilerInfoMap.set(instructionOffset, range);  
}

void UnlinkedProgramCodeBlock::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    UnlinkedProgramCodeBlock* thisObject = jsCast<UnlinkedProgramCodeBlock*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);
    for (size_t i = 0, end = thisObject->m_functionDeclarations.size(); i != end; i++)
        visitor.append(&thisObject->m_functionDeclarations[i].second);
}

UnlinkedCodeBlock::~UnlinkedCodeBlock()
{
}

void UnlinkedProgramCodeBlock::destroy(JSCell* cell)
{
    jsCast<UnlinkedProgramCodeBlock*>(cell)->~UnlinkedProgramCodeBlock();
}

void UnlinkedEvalCodeBlock::destroy(JSCell* cell)
{
    jsCast<UnlinkedEvalCodeBlock*>(cell)->~UnlinkedEvalCodeBlock();
}

void UnlinkedFunctionCodeBlock::destroy(JSCell* cell)
{
    jsCast<UnlinkedFunctionCodeBlock*>(cell)->~UnlinkedFunctionCodeBlock();
}

void UnlinkedFunctionExecutable::destroy(JSCell* cell)
{
    jsCast<UnlinkedFunctionExecutable*>(cell)->~UnlinkedFunctionExecutable();
}

void UnlinkedCodeBlock::setInstructions(std::unique_ptr<UnlinkedInstructionStream> instructions)
{
    m_unlinkedInstructions = WTF::move(instructions);
}

const UnlinkedInstructionStream& UnlinkedCodeBlock::instructions() const
{
    ASSERT(m_unlinkedInstructions.get());
    return *m_unlinkedInstructions;
}

}

