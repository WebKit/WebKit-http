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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SlotVisitor_h
#define SlotVisitor_h

#include "MarkStack.h"

namespace JSC {

class Heap;

class SlotVisitor : public MarkStack {
    friend class HeapRootVisitor;
public:
    SlotVisitor(MarkStackThreadSharedData&, void* jsArrayVPtr, void* jsFinalObjectVPtr, void* jsStringVPtr);

    void donate()
    {
        ASSERT(m_isInParallelMode);
        if (Options::numberOfGCMarkers == 1)
            return;
        
        donateKnownParallel();
    }
    
    void drain();
    
    void donateAndDrain()
    {
        donate();
        drain();
    }
    
    enum SharedDrainMode { SlaveDrain, MasterDrain };
    void drainFromShared(SharedDrainMode);

    void harvestWeakReferences();
    void finalizeUnconditionalFinalizers();
        
private:
    void donateSlow();
    
    void donateKnownParallel()
    {
        if (!m_stack.canDonateSomeCells())
            return;
        donateSlow();
    }
};

inline SlotVisitor::SlotVisitor(MarkStackThreadSharedData& shared, void* jsArrayVPtr, void* jsFinalObjectVPtr, void* jsStringVPtr)
    : MarkStack(shared, jsArrayVPtr, jsFinalObjectVPtr, jsStringVPtr)
{
}

} // namespace JSC

#endif // SlotVisitor_h
