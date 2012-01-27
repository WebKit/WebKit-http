/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "CallFrame.h"

#include "CodeBlock.h"
#include "Interpreter.h"

namespace JSC {

#ifndef NDEBUG
void CallFrame::dumpCaller()
{
    int signedLineNumber;
    intptr_t sourceID;
    UString urlString;
    JSValue function;
    
    interpreter()->retrieveLastCaller(this, signedLineNumber, sourceID, urlString, function);
    printf("Callpoint => %s:%d\n", urlString.utf8().data(), signedLineNumber);
}

RegisterFile* CallFrame::registerFile()
{
    return &interpreter()->registerFile();
}

#endif

#if ENABLE(DFG_JIT)
bool CallFrame::isInlineCallFrameSlow()
{
    if (!callee())
        return false;
    JSCell* calleeAsFunctionCell = getJSFunction(callee());
    if (!calleeAsFunctionCell)
        return false;
    JSFunction* calleeAsFunction = asFunction(calleeAsFunctionCell);
    return calleeAsFunction->executable() != codeBlock()->ownerExecutable();
}

CallFrame* CallFrame::trueCallFrame(AbstractPC pc)
{
    // Am I an inline call frame? If so, we're done.
    if (isInlineCallFrame())
        return this;
    
    // If I don't have a code block, then I'm not DFG code, so I'm the true call frame.
    CodeBlock* machineCodeBlock = codeBlock();
    if (!machineCodeBlock)
        return this;
    
    // If the code block does not have any code origins, then there was no inlining, so
    // I'm done.
    if (!machineCodeBlock->hasCodeOrigins())
        return this;
    
    // At this point the PC must be due either to the DFG, or it must be unset.
    ASSERT(pc.hasJITReturnAddress() || !pc);
    
    // Try to determine the CodeOrigin. If we don't have a pc set then the only way
    // that this makes sense is if the CodeOrigin index was set in the call frame.
    // FIXME: Note that you will see "Not currently in inlined code" comments below.
    // Currently, we do not record code origins for code that is not inlined, because
    // the only thing that we use code origins for is determining the inline stack.
    // But in the future, we'll want to use this same functionality (having a code
    // origin mapping for any calls out of JIT code) to determine the PC at any point
    // in the stack even if not in inlined code. When that happens, the code below
    // will have to change the way it detects the presence of inlining: it will always
    // get a code origin, but sometimes, that code origin will not have an inline call
    // frame. In that case, this method should bail and return this.
    CodeOrigin codeOrigin;
    if (pc.isSet()) {
        ReturnAddressPtr currentReturnPC = pc.jitReturnAddress();
        
        if (!machineCodeBlock->codeOriginForReturn(currentReturnPC, codeOrigin))
            return this; // Not currently in inlined code.
    } else {
        unsigned index = codeOriginIndexForDFGWithInlining();
        if (index == UINT_MAX)
            return this; // Not currently in inlined code.
        
        codeOrigin = machineCodeBlock->codeOrigin(index);
    }
    
    for (InlineCallFrame* inlineCallFrame = codeOrigin.inlineCallFrame; inlineCallFrame;) {
        InlineCallFrame* nextInlineCallFrame = inlineCallFrame->caller.inlineCallFrame;
        
        CallFrame* inlinedCaller = this + inlineCallFrame->stackOffset;
        
        JSFunction* calleeAsFunction = inlineCallFrame->callee.get();
        
        // Fill in the inlinedCaller
        inlinedCaller->setCodeBlock(machineCodeBlock);
        
        inlinedCaller->setScopeChain(calleeAsFunction->scope());
        if (nextInlineCallFrame)
            inlinedCaller->setCallerFrame(this + nextInlineCallFrame->stackOffset);
        else
            inlinedCaller->setCallerFrame(this);
        
        inlinedCaller->setInlineCallFrame(inlineCallFrame);
        inlinedCaller->setArgumentCountIncludingThis(inlineCallFrame->arguments.size());
        inlinedCaller->setCallee(calleeAsFunction);
        
        inlineCallFrame = nextInlineCallFrame;
    }
    
    return this + codeOrigin.inlineCallFrame->stackOffset;
}
        
CallFrame* CallFrame::trueCallerFrame()
{
    // this -> The callee; this is either an inlined callee in which case it already has
    //    a pointer to the true caller. Otherwise it contains current PC in the machine
    //    caller.
    //
    // machineCaller -> The caller according to the machine, which may be zero or
    //    more frames above the true caller due to inlining.
    
    // Am I an inline call frame? If so, we're done.
    if (isInlineCallFrame())
        return callerFrame();
    
    // I am a machine call frame, so the question is: is my caller a machine call frame
    // that has inlines or a machine call frame that doesn't?
    CallFrame* machineCaller = callerFrame()->removeHostCallFrameFlag();
    if (!machineCaller)
        return 0;
    ASSERT(!machineCaller->isInlineCallFrame());
    
    // Figure out how we want to get the current code location.
    if (hasHostCallFrameFlag() || returnAddressIsInCtiTrampoline(returnPC()))
        return machineCaller->trueCallFrameFromVMCode();
    
    return machineCaller->trueCallFrame(returnPC());
}
#endif

}
