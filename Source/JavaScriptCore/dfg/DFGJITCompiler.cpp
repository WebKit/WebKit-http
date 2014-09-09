/*
 * Copyright (C) 2011, 2013, 2014 Apple Inc. All rights reserved.
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
#include "DFGJITCompiler.h"

#if ENABLE(DFG_JIT)

#include "ArityCheckFailReturnThunks.h"
#include "CodeBlock.h"
#include "DFGFailedFinalizer.h"
#include "DFGInlineCacheWrapperInlines.h"
#include "DFGJITCode.h"
#include "DFGJITFinalizer.h"
#include "DFGOSRExitCompiler.h"
#include "DFGOperations.h"
#include "DFGRegisterBank.h"
#include "DFGSlowPathGenerator.h"
#include "DFGSpeculativeJIT.h"
#include "DFGThunks.h"
#include "JSCJSValueInlines.h"
#include "LinkBuffer.h"
#include "MaxFrameExtentForSlowPathCall.h"
#include "JSCInlines.h"
#include "VM.h"

namespace JSC { namespace DFG {

JITCompiler::JITCompiler(Graph& dfg)
    : CCallHelpers(&dfg.m_vm, dfg.m_codeBlock)
    , m_graph(dfg)
    , m_jitCode(adoptRef(new JITCode()))
    , m_blockHeads(dfg.numBlocks())
{
    if (shouldShowDisassembly() || m_graph.m_vm.m_perBytecodeProfiler)
        m_disassembler = adoptPtr(new Disassembler(dfg));
}

JITCompiler::~JITCompiler()
{
}

void JITCompiler::linkOSRExits()
{
    ASSERT(m_jitCode->osrExit.size() == m_exitCompilationInfo.size());
    if (m_graph.compilation()) {
        for (unsigned i = 0; i < m_jitCode->osrExit.size(); ++i) {
            OSRExitCompilationInfo& info = m_exitCompilationInfo[i];
            Vector<Label> labels;
            if (!info.m_failureJumps.empty()) {
                for (unsigned j = 0; j < info.m_failureJumps.jumps().size(); ++j)
                    labels.append(info.m_failureJumps.jumps()[j].label());
            } else
                labels.append(info.m_replacementSource);
            m_exitSiteLabels.append(labels);
        }
    }
    
    for (unsigned i = 0; i < m_jitCode->osrExit.size(); ++i) {
        OSRExit& exit = m_jitCode->osrExit[i];
        OSRExitCompilationInfo& info = m_exitCompilationInfo[i];
        JumpList& failureJumps = info.m_failureJumps;
        if (!failureJumps.empty())
            failureJumps.link(this);
        else
            info.m_replacementDestination = label();
        jitAssertHasValidCallFrame();
        store32(TrustedImm32(i), &vm()->osrExitIndex);
        exit.setPatchableCodeOffset(patchableJump());
    }
}

void JITCompiler::compileEntry()
{
    // This code currently matches the old JIT. In the function header we need to
    // save return address and call frame via the prologue and perform a fast stack check.
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=56292
    // We'll need to convert the remaining cti_ style calls (specifically the stack
    // check) which will be dependent on stack layout. (We'd need to account for this in
    // both normal return code and when jumping to an exception handler).
    emitFunctionPrologue();
    emitPutImmediateToCallFrameHeader(m_codeBlock, JSStack::CodeBlock);
    jitAssertTagsInPlace();
}

void JITCompiler::compileBody()
{
    // We generate the speculative code path, followed by OSR exit code to return
    // to the old JIT code if speculations fail.

    bool compiledSpeculative = m_speculative->compile();
    ASSERT_UNUSED(compiledSpeculative, compiledSpeculative);
}

void JITCompiler::compileExceptionHandlers()
{
    if (!m_exceptionChecksWithCallFrameRollback.empty()) {
        m_exceptionChecksWithCallFrameRollback.link(this);

        // lookupExceptionHandlerFromCallerFrame is passed two arguments, the VM and the exec (the CallFrame*).
        move(TrustedImmPtr(vm()), GPRInfo::argumentGPR0);
        move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR1);

#if CPU(X86)
        // FIXME: should use the call abstraction, but this is currently in the SpeculativeJIT layer!
        poke(GPRInfo::argumentGPR0);
        poke(GPRInfo::argumentGPR1, 1);
#endif
        m_calls.append(CallLinkRecord(call(), lookupExceptionHandlerFromCallerFrame));

        jumpToExceptionHandler();
    }

    if (!m_exceptionChecks.empty()) {
        m_exceptionChecks.link(this);

        // lookupExceptionHandler is passed two arguments, the VM and the exec (the CallFrame*).
        move(TrustedImmPtr(vm()), GPRInfo::argumentGPR0);
        move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR1);

#if CPU(X86)
        // FIXME: should use the call abstraction, but this is currently in the SpeculativeJIT layer!
        poke(GPRInfo::argumentGPR0);
        poke(GPRInfo::argumentGPR1, 1);
#endif
        m_calls.append(CallLinkRecord(call(), lookupExceptionHandler));

        jumpToExceptionHandler();
    }
}

void JITCompiler::link(LinkBuffer& linkBuffer)
{
    // Link the code, populate data in CodeBlock data structures.
    m_jitCode->common.frameRegisterCount = m_graph.frameRegisterCount();
    m_jitCode->common.requiredRegisterCountForExit = m_graph.requiredRegisterCountForExit();

    if (!m_graph.m_plan.inlineCallFrames->isEmpty())
        m_jitCode->common.inlineCallFrames = m_graph.m_plan.inlineCallFrames;
    
    m_jitCode->common.machineCaptureStart = m_graph.m_machineCaptureStart;
    m_jitCode->common.slowArguments = WTF::move(m_graph.m_slowArguments);

#if USE(JSVALUE32_64)
    m_jitCode->common.doubleConstants = WTF::move(m_graph.m_doubleConstants);
#endif
    
    m_graph.registerFrozenValues();

    BitVector usedJumpTables;
    for (Bag<SwitchData>::iterator iter = m_graph.m_switchData.begin(); !!iter; ++iter) {
        SwitchData& data = **iter;
        if (!data.didUseJumpTable)
            continue;
        
        if (data.kind == SwitchString)
            continue;
        
        RELEASE_ASSERT(data.kind == SwitchImm || data.kind == SwitchChar);
        
        usedJumpTables.set(data.switchTableIndex);
        SimpleJumpTable& table = m_codeBlock->switchJumpTable(data.switchTableIndex);
        table.ctiDefault = linkBuffer.locationOf(m_blockHeads[data.fallThrough.block->index]);
        table.ctiOffsets.grow(table.branchOffsets.size());
        for (unsigned j = table.ctiOffsets.size(); j--;)
            table.ctiOffsets[j] = table.ctiDefault;
        for (unsigned j = data.cases.size(); j--;) {
            SwitchCase& myCase = data.cases[j];
            table.ctiOffsets[myCase.value.switchLookupValue(data.kind) - table.min] =
                linkBuffer.locationOf(m_blockHeads[myCase.target.block->index]);
        }
    }
    
    for (unsigned i = m_codeBlock->numberOfSwitchJumpTables(); i--;) {
        if (usedJumpTables.get(i))
            continue;
        
        m_codeBlock->switchJumpTable(i).clear();
    }

    // NOTE: we cannot clear string switch tables because (1) we're running concurrently
    // and we cannot deref StringImpl's and (2) it would be weird to deref those
    // StringImpl's since we refer to them.
    for (Bag<SwitchData>::iterator switchDataIter = m_graph.m_switchData.begin(); !!switchDataIter; ++switchDataIter) {
        SwitchData& data = **switchDataIter;
        if (!data.didUseJumpTable)
            continue;
        
        if (data.kind != SwitchString)
            continue;
        
        StringJumpTable& table = m_codeBlock->stringSwitchJumpTable(data.switchTableIndex);
        table.ctiDefault = linkBuffer.locationOf(m_blockHeads[data.fallThrough.block->index]);
        StringJumpTable::StringOffsetTable::iterator iter;
        StringJumpTable::StringOffsetTable::iterator end = table.offsetTable.end();
        for (iter = table.offsetTable.begin(); iter != end; ++iter)
            iter->value.ctiOffset = table.ctiDefault;
        for (unsigned j = data.cases.size(); j--;) {
            SwitchCase& myCase = data.cases[j];
            iter = table.offsetTable.find(myCase.value.stringImpl());
            RELEASE_ASSERT(iter != end);
            iter->value.ctiOffset = linkBuffer.locationOf(m_blockHeads[myCase.target.block->index]);
        }
    }

    // Link all calls out from the JIT code to their respective functions.
    for (unsigned i = 0; i < m_calls.size(); ++i)
        linkBuffer.link(m_calls[i].m_call, m_calls[i].m_function);

    for (unsigned i = m_getByIds.size(); i--;)
        m_getByIds[i].finalize(linkBuffer);
    for (unsigned i = m_putByIds.size(); i--;)
        m_putByIds[i].finalize(linkBuffer);

    for (unsigned i = 0; i < m_ins.size(); ++i) {
        StructureStubInfo& info = *m_ins[i].m_stubInfo;
        CodeLocationCall callReturnLocation = linkBuffer.locationOf(m_ins[i].m_slowPathGenerator->call());
        info.patch.deltaCallToDone = differenceBetweenCodePtr(callReturnLocation, linkBuffer.locationOf(m_ins[i].m_done));
        info.patch.deltaCallToJump = differenceBetweenCodePtr(callReturnLocation, linkBuffer.locationOf(m_ins[i].m_jump));
        info.callReturnLocation = callReturnLocation;
        info.patch.deltaCallToSlowCase = differenceBetweenCodePtr(callReturnLocation, linkBuffer.locationOf(m_ins[i].m_slowPathGenerator->label()));
    }
    
    for (unsigned i = 0; i < m_jsCalls.size(); ++i) {
        JSCallRecord& record = m_jsCalls[i];
        CallLinkInfo& info = *record.m_info;
        ThunkGenerator generator = linkThunkGeneratorFor(
            info.callType == CallLinkInfo::Construct ? CodeForConstruct : CodeForCall,
            RegisterPreservationNotRequired);
        linkBuffer.link(record.m_slowCall, FunctionPtr(m_vm->getCTIStub(generator).code().executableAddress()));
        info.callReturnLocation = linkBuffer.locationOfNearCall(record.m_slowCall);
        info.hotPathBegin = linkBuffer.locationOf(record.m_targetToCheck);
        info.hotPathOther = linkBuffer.locationOfNearCall(record.m_fastCall);
    }
    
    MacroAssemblerCodeRef osrExitThunk = vm()->getCTIStub(osrExitGenerationThunkGenerator);
    CodeLocationLabel target = CodeLocationLabel(osrExitThunk.code());
    for (unsigned i = 0; i < m_jitCode->osrExit.size(); ++i) {
        OSRExit& exit = m_jitCode->osrExit[i];
        OSRExitCompilationInfo& info = m_exitCompilationInfo[i];
        linkBuffer.link(exit.getPatchableCodeOffsetAsJump(), target);
        exit.correctJump(linkBuffer);
        if (info.m_replacementSource.isSet()) {
            m_jitCode->common.jumpReplacements.append(JumpReplacement(
                linkBuffer.locationOf(info.m_replacementSource),
                linkBuffer.locationOf(info.m_replacementDestination)));
        }
    }
    
    if (m_graph.compilation()) {
        ASSERT(m_exitSiteLabels.size() == m_jitCode->osrExit.size());
        for (unsigned i = 0; i < m_exitSiteLabels.size(); ++i) {
            Vector<Label>& labels = m_exitSiteLabels[i];
            Vector<const void*> addresses;
            for (unsigned j = 0; j < labels.size(); ++j)
                addresses.append(linkBuffer.locationOf(labels[j]).executableAddress());
            m_graph.compilation()->addOSRExitSite(addresses);
        }
    } else
        ASSERT(!m_exitSiteLabels.size());
    
    m_jitCode->common.compilation = m_graph.compilation();
    
}

void JITCompiler::compile()
{
    SamplingRegion samplingRegion("DFG Backend");

    setStartOfCode();
    compileEntry();
    m_speculative = adoptPtr(new SpeculativeJIT(*this));
    addPtr(TrustedImm32(m_graph.stackPointerOffset() * sizeof(Register)), GPRInfo::callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();
    compileBody();
    setEndOfMainPath();

    // Generate slow path code.
    m_speculative->runSlowPathGenerators();
    
    compileExceptionHandlers();
    linkOSRExits();
    
    // Create OSR entry trampolines if necessary.
    m_speculative->createOSREntries();
    setEndOfCode();
}

void JITCompiler::link()
{
    OwnPtr<LinkBuffer> linkBuffer = adoptPtr(new LinkBuffer(*m_vm, *this, m_codeBlock, JITCompilationCanFail));
    if (linkBuffer->didFailToAllocate()) {
        m_graph.m_plan.finalizer = adoptPtr(new FailedFinalizer(m_graph.m_plan));
        return;
    }
    
    link(*linkBuffer);
    m_speculative->linkOSREntries(*linkBuffer);
    
    m_jitCode->shrinkToFit();
    codeBlock()->shrinkToFit(CodeBlock::LateShrink);

    disassemble(*linkBuffer);
    
    m_graph.m_plan.finalizer = adoptPtr(new JITFinalizer(
        m_graph.m_plan, m_jitCode.release(), linkBuffer.release()));
}

void JITCompiler::compileFunction()
{
    SamplingRegion samplingRegion("DFG Backend");
    
    setStartOfCode();
    compileEntry();

    // === Function header code generation ===
    // This is the main entry point, without performing an arity check.
    // If we needed to perform an arity check we will already have moved the return address,
    // so enter after this.
    Label fromArityCheck(this);
    // Plant a check that sufficient space is available in the JSStack.
    addPtr(TrustedImm32(virtualRegisterForLocal(m_graph.requiredRegisterCountForExecutionAndExit() - 1).offset() * sizeof(Register)), GPRInfo::callFrameRegister, GPRInfo::regT1);
    Jump stackOverflow = branchPtr(Above, AbsoluteAddress(m_vm->addressOfStackLimit()), GPRInfo::regT1);

    // Move the stack pointer down to accommodate locals
    addPtr(TrustedImm32(m_graph.stackPointerOffset() * sizeof(Register)), GPRInfo::callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    // === Function body code generation ===
    m_speculative = adoptPtr(new SpeculativeJIT(*this));
    compileBody();
    setEndOfMainPath();

    // === Function footer code generation ===
    //
    // Generate code to perform the stack overflow handling (if the stack check in
    // the function header fails), and generate the entry point with arity check.
    //
    // Generate the stack overflow handling; if the stack check in the function head fails,
    // we need to call out to a helper function to throw the StackOverflowError.
    stackOverflow.link(this);

    emitStoreCodeOrigin(CodeOrigin(0));

    if (maxFrameExtentForSlowPathCall)
        addPtr(TrustedImm32(-maxFrameExtentForSlowPathCall), stackPointerRegister);

    m_speculative->callOperationWithCallFrameRollbackOnException(operationThrowStackOverflowError, m_codeBlock);
    
    // The fast entry point into a function does not check the correct number of arguments
    // have been passed to the call (we only use the fast entry point where we can statically
    // determine the correct number of arguments have been passed, or have already checked).
    // In cases where an arity check is necessary, we enter here.
    // FIXME: change this from a cti call to a DFG style operation (normal C calling conventions).
    m_arityCheck = label();
    compileEntry();

    load32(AssemblyHelpers::payloadFor((VirtualRegister)JSStack::ArgumentCount), GPRInfo::regT1);
    branch32(AboveOrEqual, GPRInfo::regT1, TrustedImm32(m_codeBlock->numParameters())).linkTo(fromArityCheck, this);
    emitStoreCodeOrigin(CodeOrigin(0));
    if (maxFrameExtentForSlowPathCall)
        addPtr(TrustedImm32(-maxFrameExtentForSlowPathCall), stackPointerRegister);
    m_speculative->callOperationWithCallFrameRollbackOnException(m_codeBlock->m_isConstructor ? operationConstructArityCheck : operationCallArityCheck, GPRInfo::regT0);
    if (maxFrameExtentForSlowPathCall)
        addPtr(TrustedImm32(maxFrameExtentForSlowPathCall), stackPointerRegister);
    branchTest32(Zero, GPRInfo::regT0).linkTo(fromArityCheck, this);
    emitStoreCodeOrigin(CodeOrigin(0));
    GPRReg thunkReg;
#if USE(JSVALUE64)
    thunkReg = GPRInfo::regT7;
#else
    thunkReg = GPRInfo::regT5;
#endif
    move(TrustedImmPtr(m_vm->arityCheckFailReturnThunks->returnPCsFor(*m_vm, m_codeBlock->numParameters())), thunkReg);
    loadPtr(BaseIndex(thunkReg, GPRInfo::regT0, timesPtr()), thunkReg);
    m_callArityFixup = call();
    jump(fromArityCheck);
    
    // Generate slow path code.
    m_speculative->runSlowPathGenerators();
    
    compileExceptionHandlers();
    linkOSRExits();
    
    // Create OSR entry trampolines if necessary.
    m_speculative->createOSREntries();
    setEndOfCode();
}

void JITCompiler::linkFunction()
{
    // === Link ===
    OwnPtr<LinkBuffer> linkBuffer = adoptPtr(new LinkBuffer(*m_vm, *this, m_codeBlock, JITCompilationCanFail));
    if (linkBuffer->didFailToAllocate()) {
        m_graph.m_plan.finalizer = adoptPtr(new FailedFinalizer(m_graph.m_plan));
        return;
    }
    link(*linkBuffer);
    m_speculative->linkOSREntries(*linkBuffer);
    
    m_jitCode->shrinkToFit();
    codeBlock()->shrinkToFit(CodeBlock::LateShrink);
    
    linkBuffer->link(m_callArityFixup, FunctionPtr((m_vm->getCTIStub(arityFixupGenerator)).code().executableAddress()));
    
    disassemble(*linkBuffer);

    MacroAssemblerCodePtr withArityCheck = linkBuffer->locationOf(m_arityCheck);

    m_graph.m_plan.finalizer = adoptPtr(new JITFinalizer(
        m_graph.m_plan, m_jitCode.release(), linkBuffer.release(), withArityCheck));
}

void JITCompiler::disassemble(LinkBuffer& linkBuffer)
{
    if (shouldShowDisassembly())
        m_disassembler->dump(linkBuffer);
    
    if (m_graph.m_plan.compilation)
        m_disassembler->reportToProfiler(m_graph.m_plan.compilation.get(), linkBuffer);
}

#if USE(JSVALUE32_64)
void* JITCompiler::addressOfDoubleConstant(Node* node)
{
    double value = node->asNumber();
    int64_t valueBits = bitwise_cast<int64_t>(value);
    auto it = m_graph.m_doubleConstantsMap.find(valueBits);
    if (it != m_graph.m_doubleConstantsMap.end())
        return it->second;

    if (!m_graph.m_doubleConstants)
        m_graph.m_doubleConstants = std::make_unique<Bag<double>>();

    double* addressInConstantPool = m_graph.m_doubleConstants->add();
    *addressInConstantPool = value;
    m_graph.m_doubleConstantsMap[valueBits] = addressInConstantPool;
    return addressInConstantPool;
}
#endif

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
