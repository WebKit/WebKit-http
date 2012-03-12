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
#include "DFGCFAPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractState.h"
#include "DFGGraph.h"
#include "DFGPhase.h"

namespace JSC { namespace DFG {

class CFAPhase : public Phase {
public:
    CFAPhase(Graph& graph)
        : Phase(graph, "control flow analysis")
        , m_state(graph)
    {
    }
    
    void run()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        m_count = 0;
#endif
        
        // This implements a pseudo-worklist-based forward CFA, except that the visit order
        // of blocks is the bytecode program order (which is nearly topological), and
        // instead of a worklist we just walk all basic blocks checking if cfaShouldRevisit
        // is set to true. This is likely to balance the efficiency properties of both
        // worklist-based and forward fixpoint-based approaches. Like a worklist-based
        // approach, it won't visit code if it's meaningless to do so (nothing changed at
        // the head of the block or the predecessors have not been visited). Like a forward
        // fixpoint-based approach, it has a high probability of only visiting a block
        // after all predecessors have been visited. Only loops will cause this analysis to
        // revisit blocks, and the amount of revisiting is proportional to loop depth.
        
        AbstractState::initialize(m_graph);
        
        do {
            m_changed = false;
            performForwardCFA();
        } while (m_changed);
    }
    
private:
    void performBlockCFA(BlockIndex blockIndex)
    {
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        if (!block->cfaShouldRevisit)
            return;
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("   Block #%u (bc#%u):\n", blockIndex, block->bytecodeBegin);
#endif
        m_state.beginBasicBlock(block);
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("      head vars: ");
        dumpOperands(block->valuesAtHead, WTF::dataFile());
        dataLog("\n");
#endif
        for (unsigned i = 0; i < block->size(); ++i) {
            NodeIndex nodeIndex = block->at(i);
            if (!m_graph[nodeIndex].shouldGenerate())
                continue;
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("      %s @%u: ", Graph::opName(m_graph[nodeIndex].op()), nodeIndex);
            m_state.dump(WTF::dataFile());
            dataLog("\n");
#endif
            if (!m_state.execute(i))
                break;
        }
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("      tail regs: ");
        m_state.dump(WTF::dataFile());
        dataLog("\n");
#endif
        m_changed |= m_state.endBasicBlock(AbstractState::MergeToSuccessors);
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("      tail vars: ");
        dumpOperands(block->valuesAtTail, WTF::dataFile());
        dataLog("\n");
#endif
    }
    
    void performForwardCFA()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("CFA [%u]\n", ++m_count);
#endif
        
        for (BlockIndex block = 0; block < m_graph.m_blocks.size(); ++block)
            performBlockCFA(block);
    }

private:
    AbstractState m_state;
    
    bool m_changed;
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
    unsigned m_count;
#endif
};

void performCFA(Graph& graph)
{
    runPhase<CFAPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
