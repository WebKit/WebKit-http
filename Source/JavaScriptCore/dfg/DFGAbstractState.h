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

#ifndef DFGAbstractState_h
#define DFGAbstractState_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGAbstractValue.h"
#include "DFGGraph.h"
#include "DFGNode.h"
#include <wtf/Vector.h>

namespace JSC {

class CodeBlock;

namespace DFG {

struct BasicBlock;

// This implements the notion of an abstract state for flow-sensitive intraprocedural
// control flow analysis (CFA), with a focus on the elimination of redundant type checks.
// It also implements most of the mechanisms of abstract interpretation that such an
// analysis would use. This class should be used in two idioms:
//
// 1) Performing the CFA. In this case, AbstractState should be run over all basic
//    blocks repeatedly until convergence is reached. Convergence is defined by
//    endBasicBlock(AbstractState::MergeToSuccessors) returning false for all blocks.
//
// 2) Rematerializing the results of a previously executed CFA. In this case,
//    AbstractState should be run over whatever basic block you're interested in up
//    to the point of the node at which you'd like to interrogate the known type
//    of all other nodes. At this point it's safe to discard the AbstractState entirely,
//    call reset(), or to run it to the end of the basic block and call
//    endBasicBlock(AbstractState::DontMerge). The latter option is safest because
//    it performs some useful integrity checks.
//
// After the CFA is run, the inter-block state is saved at the heads and tails of all
// basic blocks. This allows the intra-block state to be rematerialized by just
// executing the CFA for that block. If you need to know inter-block state only, then
// you only need to examine the BasicBlock::m_valuesAtHead or m_valuesAtTail fields.
//
// Running this analysis involves the following, modulo the inter-block state
// merging and convergence fixpoint:
//
// AbstractState state(codeBlock, graph);
// state.beginBasicBlock(basicBlock);
// bool endReached = true;
// for (NodeIndex idx = basicBlock.begin; idx < basicBlock.end; ++idx) {
//     if (!state.execute(idx))
//         break;
// }
// bool result = state.endBasicBlock(<either Merge or DontMerge>);

class AbstractState {
public:
    enum MergeMode {
        // Don't merge the state in AbstractState with basic blocks.
        DontMerge,
        
        // Merge the state in AbstractState with the tail of the basic
        // block being analyzed.
        MergeToTail,
        
        // Merge the state in AbstractState with the tail of the basic
        // block, and with the heads of successor blocks.
        MergeToSuccessors
    };
    
    enum BranchDirection {
        // This is not a branch and so there is no branch direction, or
        // the branch direction has yet to be set.
        InvalidBranchDirection,
        
        // The branch takes the true case.
        TakeTrue,
        
        // The branch takes the false case.
        TakeFalse,
        
        // For all we know, the branch could go either direction, so we
        // have to assume the worst.
        TakeBoth
    };
    
    static const char* branchDirectionToString(BranchDirection branchDirection)
    {
        switch (branchDirection) {
        case InvalidBranchDirection:
            return "Invalid";
        case TakeTrue:
            return "TakeTrue";
        case TakeFalse:
            return "TakeFalse";
        case TakeBoth:
            return "TakeBoth";
        }
    }

    AbstractState(Graph&);
    
    ~AbstractState();
    
    AbstractValue& forNode(NodeIndex nodeIndex)
    {
        return m_nodes[nodeIndex];
    }
    
    AbstractValue& forNode(Edge nodeUse)
    {
        return forNode(nodeUse.index());
    }
    
    Operands<AbstractValue>& variables()
    {
        return m_variables;
    }
    
    // Call this before beginning CFA to initialize the abstract values of
    // arguments, and to indicate which blocks should be listed for CFA
    // execution.
    static void initialize(Graph&);

    // Start abstractly executing the given basic block. Initializes the
    // notion of abstract state to what we believe it to be at the head
    // of the basic block, according to the basic block's data structures.
    // This method also sets cfaShouldRevisit to false.
    void beginBasicBlock(BasicBlock*);
    
    // Finish abstractly executing a basic block. If MergeToTail or
    // MergeToSuccessors is passed, then this merges everything we have
    // learned about how the state changes during this block's execution into
    // the block's data structures. There are three return modes, depending
    // on the value of mergeMode:
    //
    // DontMerge:
    //    Always returns false.
    //
    // MergeToTail:
    //    Returns true if the state of the block at the tail was changed.
    //    This means that you must call mergeToSuccessors(), and if that
    //    returns true, then you must revisit (at least) the successor
    //    blocks. False will always be returned if the block is terminal
    //    (i.e. ends in Throw or Return, or has a ForceOSRExit inside it).
    //
    // MergeToSuccessors:
    //    Returns true if the state of the block at the tail was changed,
    //    and, if the state at the heads of successors was changed.
    //    A true return means that you must revisit (at least) the successor
    //    blocks. This also sets cfaShouldRevisit to true for basic blocks
    //    that must be visited next.
    //
    // If you'd like to know what direction the branch at the end of the
    // basic block is thought to have taken, you can pass a non-0 pointer
    // for BranchDirection.
    bool endBasicBlock(MergeMode, BranchDirection* = 0);
    
    // Reset the AbstractState. This throws away any results, and at this point
    // you can safely call beginBasicBlock() on any basic block.
    void reset();
    
    // Abstractly executes the given node. The new abstract state is stored into an
    // abstract register file stored in *this. Loads of local variables (that span
    // basic blocks) interrogate the basic block's notion of the state at the head.
    // Stores to local variables are handled in endBasicBlock(). This returns true
    // if execution should continue past this node. Notably, it will return true
    // for block terminals, so long as those terminals are not Return or variants
    // of Throw.
    bool execute(unsigned);
    
    // Did the last executed node clobber the world?
    bool didClobber() const { return m_didClobber; }
    
    // Is the execution state still valid? This will be false if execute() has
    // returned false previously.
    bool isValid() const { return m_isValid; }
    
    // Merge the abstract state stored at the first block's tail into the second
    // block's head. Returns true if the second block's state changed. If so,
    // that block must be abstractly interpreted again. This also sets
    // to->cfaShouldRevisit to true, if it returns true, or if to has not been
    // visited yet.
    bool merge(BasicBlock* from, BasicBlock* to);
    
    // Merge the abstract state stored at the block's tail into all of its
    // successors. Returns true if any of the successors' states changed. Note
    // that this is automatically called in endBasicBlock() if MergeMode is
    // MergeToSuccessors.
    bool mergeToSuccessors(Graph&, BasicBlock*, BranchDirection);

    void dump(FILE* out);
    
private:
    void clobberWorld(const CodeOrigin&, unsigned indexInBlock);
    void clobberCapturedVars(const CodeOrigin&);
    void clobberStructures(unsigned indexInBlock);
    
    bool mergeStateAtTail(AbstractValue& destination, AbstractValue& inVariable, NodeIndex);
    
    static bool mergeVariableBetweenBlocks(AbstractValue& destination, AbstractValue& source, NodeIndex destinationNodeIndex, NodeIndex sourceNodeIndex);
    
    void speculateInt32Unary(Node& node, bool forceCanExit = false)
    {
        AbstractValue& childValue = forNode(node.child1());
        node.setCanExit(forceCanExit || !isInt32Speculation(childValue.m_type));
        childValue.filter(SpecInt32);
    }
    
    void speculateNumberUnary(Node& node)
    {
        AbstractValue& childValue = forNode(node.child1());
        node.setCanExit(!isNumberSpeculation(childValue.m_type));
        childValue.filter(SpecNumber);
    }
    
    void speculateBooleanUnary(Node& node)
    {
        AbstractValue& childValue = forNode(node.child1());
        node.setCanExit(!isBooleanSpeculation(childValue.m_type));
        childValue.filter(SpecBoolean);
    }
    
    void speculateInt32Binary(Node& node, bool forceCanExit = false)
    {
        AbstractValue& childValue1 = forNode(node.child1());
        AbstractValue& childValue2 = forNode(node.child2());
        node.setCanExit(
            forceCanExit
            || !isInt32Speculation(childValue1.m_type)
            || !isInt32Speculation(childValue2.m_type));
        childValue1.filter(SpecInt32);
        childValue2.filter(SpecInt32);
    }
    
    void speculateNumberBinary(Node& node)
    {
        AbstractValue& childValue1 = forNode(node.child1());
        AbstractValue& childValue2 = forNode(node.child2());
        node.setCanExit(
            !isNumberSpeculation(childValue1.m_type)
            || !isNumberSpeculation(childValue2.m_type));
        childValue1.filter(SpecNumber);
        childValue2.filter(SpecNumber);
    }
    
    bool trySetConstant(NodeIndex nodeIndex, JSValue value)
    {
        // Make sure we don't constant fold something that will produce values that contravene
        // predictions. If that happens then we know that the code will OSR exit, forcing
        // recompilation. But if we tried to constant fold then we'll have a very degenerate
        // IR: namely we'll have a JSConstant that contravenes its own prediction. There's a
        // lot of subtle code that assumes that
        // speculationFromValue(jsConstant) == jsConstant.prediction(). "Hardening" that code
        // is probably less sane than just pulling back on constant folding.
        SpeculatedType oldType = m_graph[nodeIndex].prediction();
        if (mergeSpeculations(speculationFromValue(value), oldType) != oldType)
            return false;
        
        forNode(nodeIndex).set(value);
        return true;
    }
    
    CodeBlock* m_codeBlock;
    Graph& m_graph;
    
    Vector<AbstractValue, 64> m_nodes;
    Operands<AbstractValue> m_variables;
    BasicBlock* m_block;
    bool m_haveStructures;
    bool m_foundConstants;
    
    bool m_isValid;
    bool m_didClobber;
    
    BranchDirection m_branchDirection; // This is only set for blocks that end in Branch and that execute to completion (i.e. m_isValid == true).
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGAbstractState_h

