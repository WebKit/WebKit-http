/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "DFGArgumentsSimplificationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractState.h"
#include "DFGBasicBlock.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "DFGValidate.h"
#include <wtf/HashSet.h>
#include <wtf/HashMap.h>

namespace JSC { namespace DFG {

namespace {

template<typename T>
struct NullableHashTraits : public HashTraits<T> {
    static const bool emptyValueIsZero = false;
    static T emptyValue() { return reinterpret_cast<T>(1); }
};

struct ArgumentsAliasingData {
    InlineCallFrame* callContext;
    bool callContextSet;
    bool multipleCallContexts;
    
    bool assignedFromArguments;
    bool assignedFromManyThings;
    
    bool escapes;
    
    ArgumentsAliasingData()
        : callContext(0)
        , callContextSet(false)
        , multipleCallContexts(false)
        , assignedFromArguments(false)
        , assignedFromManyThings(false)
        , escapes(false)
    {
    }
    
    void mergeCallContext(InlineCallFrame* newCallContext)
    {
        if (multipleCallContexts)
            return;
        
        if (!callContextSet) {
            callContext = newCallContext;
            callContextSet = true;
            return;
        }
        
        if (callContext == newCallContext)
            return;
        
        multipleCallContexts = true;
    }
    
    bool callContextIsValid()
    {
        return callContextSet && !multipleCallContexts;
    }
    
    void mergeArgumentsAssignment()
    {
        assignedFromArguments = true;
    }
    
    void mergeNonArgumentsAssignment()
    {
        assignedFromManyThings = true;
    }
    
    bool argumentsAssignmentIsValid()
    {
        return assignedFromArguments && !assignedFromManyThings;
    }
    
    bool isValid()
    {
        return callContextIsValid() && argumentsAssignmentIsValid() && !escapes;
    }
};

} // end anonymous namespace

class ArgumentsSimplificationPhase : public Phase {
public:
    ArgumentsSimplificationPhase(Graph& graph)
        : Phase(graph, "arguments simplification")
    {
    }
    
    bool run()
    {
        if (!m_graph.m_hasArguments)
            return false;
        
        bool changed = false;
        
        // Record which arguments are known to escape no matter what.
        for (unsigned i = codeBlock()->inlineCallFrames().size(); i--;) {
            InlineCallFrame* inlineCallFrame = &codeBlock()->inlineCallFrames()[i];
            if (m_graph.m_executablesWhoseArgumentsEscaped.contains(
                    m_graph.executableFor(inlineCallFrame)))
                m_createsArguments.add(inlineCallFrame);
        }
        
        // Create data for variable access datas that we will want to analyze.
        for (unsigned i = m_graph.m_variableAccessData.size(); i--;) {
            VariableAccessData* variableAccessData = &m_graph.m_variableAccessData[i];
            if (!variableAccessData->isRoot())
                continue;
            if (variableAccessData->isCaptured())
                continue;
            m_argumentsAliasing.add(variableAccessData, ArgumentsAliasingData());
        }
        
        // Figure out which variables alias the arguments and nothing else, and are
        // used only for GetByVal and GetArrayLength accesses. At the same time,
        // identify uses of CreateArguments that are not consistent with the arguments
        // being aliased only to variables that satisfy these constraints.
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                NodeIndex nodeIndex = block->at(indexInBlock);
                Node& node = m_graph[nodeIndex];
                if (!node.shouldGenerate())
                    continue;
                switch (node.op()) {
                case CreateArguments: {
                    // Ignore this op. If we see a lone CreateArguments then we want to
                    // completely ignore it because:
                    // 1) The default would be to see that the child is a GetLocal on the
                    //    arguments register and conclude that we have an arguments escape.
                    // 2) The fact that a CreateArguments exists does not mean that it
                    //    will continue to exist after we're done with this phase. As far
                    //    as this phase is concerned, a CreateArguments only "exists" if it
                    //    is used in a manner that necessitates its existance.
                    break;
                }
                    
                case TearOffArguments: {
                    // Ignore arguments tear off, because it's only relevant if we actually
                    // need to create the arguments.
                    break;
                }
                    
                case SetLocal: {
                    Node& source = m_graph[node.child1()];
                    VariableAccessData* variableAccessData = node.variableAccessData();
                    int argumentsRegister =
                        m_graph.uncheckedArgumentsRegisterFor(node.codeOrigin);
                    if (source.op() != CreateArguments) {
                        // Make sure that the source of the SetLocal knows that if it's
                        // a variable that we think is aliased to the arguments, then it
                        // may escape at this point. In future, we could track transitive
                        // aliasing. But not yet.
                        observeBadArgumentsUse(node.child1());
                        
                        // If this is an assignment to the arguments register, then
                        // pretend as if the arguments were created. We don't want to
                        // optimize code that explicitly assigns to the arguments,
                        // because that seems too ugly.
                        
                        // But, before getting rid of CreateArguments, we will have
                        // an assignment to the arguments registers with JSValue().
                        // That's because CSE will refuse to get rid of the
                        // init_lazy_reg since it treats CreateArguments as reading
                        // local variables. That could be fixed, but it's easier to
                        // work around this here.
                        if (source.op() == JSConstant
                            && !source.valueOfJSConstant(codeBlock()))
                            break;
                        
                        if (argumentsRegister != InvalidVirtualRegister
                            && (variableAccessData->local() == argumentsRegister
                                || variableAccessData->local() == unmodifiedArgumentsRegister(argumentsRegister))) {
                            m_createsArguments.add(node.codeOrigin.inlineCallFrame);
                            break;
                        }

                        if (variableAccessData->isCaptured())
                            break;
                        
                        // Make sure that if it's a variable that we think is aliased to
                        // the arguments, that we know that it might actually not be.
                        ArgumentsAliasingData& data =
                            m_argumentsAliasing.find(variableAccessData)->second;
                        data.mergeNonArgumentsAssignment();
                        data.mergeCallContext(node.codeOrigin.inlineCallFrame);
                        break;
                    }
                    if (argumentsRegister != InvalidVirtualRegister
                        && (variableAccessData->local() == argumentsRegister
                            || variableAccessData->local() == unmodifiedArgumentsRegister(argumentsRegister))) {
                        if (node.codeOrigin.inlineCallFrame == source.codeOrigin.inlineCallFrame)
                            break;
                        m_createsArguments.add(source.codeOrigin.inlineCallFrame);
                        break;
                    }
                    if (variableAccessData->isCaptured()) {
                        m_createsArguments.add(source.codeOrigin.inlineCallFrame);
                        break;
                    }
                    ArgumentsAliasingData& data =
                        m_argumentsAliasing.find(variableAccessData)->second;
                    data.mergeArgumentsAssignment();
                    // This ensures that the variable's uses are in the same context as
                    // the arguments it is aliasing.
                    data.mergeCallContext(node.codeOrigin.inlineCallFrame);
                    data.mergeCallContext(source.codeOrigin.inlineCallFrame);
                    break;
                }
                    
                case GetLocal:
                case Phi: {
                    VariableAccessData* variableAccessData = node.variableAccessData();
                    if (variableAccessData->isCaptured())
                        break;
                    ArgumentsAliasingData& data =
                        m_argumentsAliasing.find(variableAccessData)->second;
                    data.mergeCallContext(node.codeOrigin.inlineCallFrame);
                    break;
                }
                    
                case Flush: {
                    VariableAccessData* variableAccessData = node.variableAccessData();
                    if (variableAccessData->isCaptured())
                        break;
                    ArgumentsAliasingData& data =
                        m_argumentsAliasing.find(variableAccessData)->second;
                    data.mergeCallContext(node.codeOrigin.inlineCallFrame);
                    
                    // If a variable is used in a flush then by definition it escapes.
                    data.escapes = true;
                    break;
                }
                    
                case SetArgument: {
                    VariableAccessData* variableAccessData = node.variableAccessData();
                    if (variableAccessData->isCaptured())
                        break;
                    ArgumentsAliasingData& data =
                        m_argumentsAliasing.find(variableAccessData)->second;
                    data.mergeNonArgumentsAssignment();
                    data.mergeCallContext(node.codeOrigin.inlineCallFrame);
                    break;
                }
                    
                case GetByVal: {
                    if (node.arrayMode() != Array::Arguments) {
                        observeBadArgumentsUses(node);
                        break;
                    }

                    // That's so awful and pretty much impossible since it would
                    // imply that the arguments were predicted integer, but it's
                    // good to be defensive and thorough.
                    observeBadArgumentsUse(node.child2());
                    observeProperArgumentsUse(node, node.child1());
                    break;
                }
                    
                case GetArrayLength: {
                    if (node.arrayMode() != Array::Arguments) {
                        observeBadArgumentsUses(node);
                        break;
                    }
                        
                    observeProperArgumentsUse(node, node.child1());
                    break;
                }
                    
                case Phantom:
                    // We don't care about phantom uses, since phantom uses are all about
                    // just keeping things alive for OSR exit. If something - like the
                    // CreateArguments - is just being kept alive, then this transformation
                    // will not break this, since the Phantom will now just keep alive a
                    // PhantomArguments and OSR exit will still do the right things.
                    break;
                    
                case CheckStructure:
                case ForwardCheckStructure:
                case StructureTransitionWatchpoint:
                case ForwardStructureTransitionWatchpoint:
                case CheckArray:
                    // We don't care about these because if we get uses of the relevant
                    // variable then we can safely get rid of these, too. This of course
                    // relies on there not being any information transferred by the CFA
                    // from a CheckStructure on one variable to the information about the
                    // structures of another variable.
                    break;
                    
                default:
                    observeBadArgumentsUses(node);
                    break;
                }
            }
        }

        // Now we know which variables are aliased to arguments. But if any of them are
        // found to have escaped, or were otherwise invalidated, then we need to mark
        // the arguments as requiring creation. This is a property of SetLocals to
        // variables that are neither the correct arguments register nor are marked as
        // being arguments-aliased.
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                NodeIndex nodeIndex = block->at(indexInBlock);
                Node& node = m_graph[nodeIndex];
                if (!node.shouldGenerate())
                    continue;
                if (node.op() != SetLocal)
                    continue;
                Node& source = m_graph[node.child1()];
                if (source.op() != CreateArguments)
                    continue;
                VariableAccessData* variableAccessData = node.variableAccessData();
                if (variableAccessData->isCaptured()) {
                    // The captured case would have already been taken care of in the
                    // previous pass.
                    continue;
                }
                
                ArgumentsAliasingData& data =
                    m_argumentsAliasing.find(variableAccessData)->second;
                if (data.isValid())
                    continue;
                
                m_createsArguments.add(source.codeOrigin.inlineCallFrame);
            }
        }
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("Arguments aliasing states:\n");
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i) {
            VariableAccessData* variableAccessData = &m_graph.m_variableAccessData[i];
            if (!variableAccessData->isRoot())
                continue;
            dataLog("   r%d(%s): ", variableAccessData->local(), m_graph.nameOfVariableAccessData(variableAccessData));
            if (variableAccessData->isCaptured())
                dataLog("Captured");
            else {
                ArgumentsAliasingData& data =
                    m_argumentsAliasing.find(variableAccessData)->second;
                bool first = true;
                if (data.callContextIsValid()) {
                    if (!first)
                        dataLog(", ");
                    dataLog("Have Call Context: %p", data.callContext);
                    first = false;
                    if (!m_createsArguments.contains(data.callContext))
                        dataLog(" (Does Not Create Arguments)");
                }
                if (data.argumentsAssignmentIsValid()) {
                    if (!first)
                        dataLog(", ");
                    dataLog("Arguments Assignment Is Valid");
                    first = false;
                }
                if (!data.escapes) {
                    if (!first)
                        dataLog(", ");
                    dataLog("Does Not Escape");
                    first = false;
                }
                if (!first)
                    dataLog(", ");
                if (data.isValid()) {
                    if (m_createsArguments.contains(data.callContext))
                        dataLog("VALID");
                    else
                        dataLog("INVALID (due to argument creation)");
                } else
                    dataLog("INVALID (due to bad variable use)");
            }
            dataLog("\n");
        }
#endif
        
        InsertionSet<NodeIndex> insertionSet;
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                NodeIndex nodeIndex = block->at(indexInBlock);
                Node& node = m_graph[nodeIndex];
                if (!node.shouldGenerate())
                    continue;
                
                switch (node.op()) {
                case SetLocal: {
                    Node& source = m_graph[node.child1()];
                    if (source.op() != CreateArguments)
                        break;
                    
                    if (m_createsArguments.contains(source.codeOrigin.inlineCallFrame))
                        break;
                    
                    VariableAccessData* variableAccessData = node.variableAccessData();
                    
                    if (m_graph.argumentsRegisterFor(node.codeOrigin) == variableAccessData->local()
                           || unmodifiedArgumentsRegister(m_graph.argumentsRegisterFor(node.codeOrigin)) == variableAccessData->local()) {
                        // The child of this store should really be the empty value.
                        Node emptyJSValue(JSConstant, node.codeOrigin, OpInfo(codeBlock()->addOrFindConstant(JSValue())));
                        emptyJSValue.ref();
                        NodeIndex emptyJSValueIndex = m_graph.size();
                        m_graph.deref(node.child1());
                        node.children.child1() = Edge(emptyJSValueIndex);
                        m_graph.append(emptyJSValue);
                        insertionSet.append(indexInBlock, emptyJSValueIndex);
                        changed = true;
                        break;
                    }
                    ASSERT(!variableAccessData->isCaptured());
                    
                    // If this is a store into a VariableAccessData* that is marked as
                    // arguments aliasing for an InlineCallFrame* that does not create
                    // arguments, then flag the VariableAccessData as being an
                    // arguments-aliased. This'll let the OSR exit machinery do the right
                    // things. Note also that the SetLocal should become dead as soon as
                    // we replace all uses of this variable with GetMyArgumentsLength and
                    // GetMyArgumentByVal.
                    ASSERT(m_argumentsAliasing.find(variableAccessData)->second.isValid());
                    changed |= variableAccessData->mergeIsArgumentsAlias(true);
                    break;
                }
                    
                case Phantom: {
                    // It's highly likely that we will have a Phantom referencing either
                    // CreateArguments, or a local op for the arguments register, or a
                    // local op for an arguments-aliased variable. In any of those cases,
                    // we should remove the phantom reference, since:
                    // 1) Phantoms only exist to aid OSR exit. But arguments simplification
                    //    has its own OSR exit story, which is to inform OSR exit to reify
                    //    the arguments as necessary.
                    // 2) The Phantom may keep the CreateArguments node alive, which is
                    //    precisely what we don't want.
                    for (unsigned i = 0; i < AdjacencyList::Size; ++i)
                        removeArgumentsReferencingPhantomChild(node, i);
                    break;
                }
                    
                case CheckStructure:
                case ForwardCheckStructure:
                case StructureTransitionWatchpoint:
                case ForwardStructureTransitionWatchpoint:
                case CheckArray: {
                    // We can just get rid of this node, if it references a phantom argument.
                    if (!isOKToOptimize(m_graph[node.child1()]))
                        break;
                    m_graph.deref(node.child1());
                    node.setOpAndDefaultFlags(Phantom);
                    node.children.setChild1(Edge());
                    break;
                }
                    
                case GetByVal: {
                    if (node.arrayMode() != Array::Arguments)
                        break;

                    // This can be simplified to GetMyArgumentByVal if we know that
                    // it satisfies either condition (1) or (2):
                    // 1) Its first child is a valid ArgumentsAliasingData and the
                    //    InlineCallFrame* is not marked as creating arguments.
                    // 2) Its first child is CreateArguments and its InlineCallFrame*
                    //    is not marked as creating arguments.
                    
                    if (!isOKToOptimize(m_graph[node.child1()]))
                        break;
                    
                    m_graph.deref(node.child1());
                    node.children.child1() = node.children.child2();
                    node.children.child2() = Edge();
                    node.setOpAndDefaultFlags(GetMyArgumentByVal);
                    changed = true;
                    --indexInBlock; // Force reconsideration of this op now that it's a GetMyArgumentByVal.
                    break;
                }
                    
                case GetArrayLength: {
                    if (node.arrayMode() != Array::Arguments)
                        break;
                    
                    if (!isOKToOptimize(m_graph[node.child1()]))
                        break;
                    
                    m_graph.deref(node.child1());
                    node.children.child1() = Edge();
                    node.setOpAndDefaultFlags(GetMyArgumentsLength);
                    changed = true;
                    --indexInBlock; // Force reconsideration of this op noew that it's a GetMyArgumentsLength.
                    break;
                }
                    
                case GetMyArgumentsLength:
                case GetMyArgumentsLengthSafe: {
                    if (m_createsArguments.contains(node.codeOrigin.inlineCallFrame)) {
                        ASSERT(node.op() == GetMyArgumentsLengthSafe);
                        break;
                    }
                    if (node.op() == GetMyArgumentsLengthSafe) {
                        node.setOp(GetMyArgumentsLength);
                        changed = true;
                    }
                    
                    CodeOrigin codeOrigin = node.codeOrigin;
                    if (!codeOrigin.inlineCallFrame)
                        break;
                    
                    // We know exactly what this will return. But only after we have checked
                    // that nobody has escaped our arguments.
                    Node check(CheckArgumentsNotCreated, codeOrigin);
                    check.ref();
                    NodeIndex checkIndex = m_graph.size();
                    m_graph.append(check);
                    insertionSet.append(indexInBlock, checkIndex);
                    
                    m_graph.convertToConstant(
                        nodeIndex, jsNumber(codeOrigin.inlineCallFrame->arguments.size() - 1));
                    changed = true;
                    break;
                }
                    
                case GetMyArgumentByVal:
                case GetMyArgumentByValSafe: {
                    if (m_createsArguments.contains(node.codeOrigin.inlineCallFrame)) {
                        ASSERT(node.op() == GetMyArgumentByValSafe);
                        break;
                    }
                    if (node.op() == GetMyArgumentByValSafe) {
                        node.setOp(GetMyArgumentByVal);
                        changed = true;
                    }
                    if (!node.codeOrigin.inlineCallFrame)
                        break;
                    if (!m_graph[node.child1()].hasConstant())
                        break;
                    JSValue value = m_graph[node.child1()].valueOfJSConstant(codeBlock());
                    if (!value.isInt32())
                        break;
                    int32_t index = value.asInt32();
                    if (index < 0
                        || static_cast<size_t>(index + 1) >=
                            node.codeOrigin.inlineCallFrame->arguments.size())
                        break;
                    
                    // We know which argument this is accessing. But only after we have checked
                    // that nobody has escaped our arguments. We also need to ensure that the
                    // index is kept alive. That's somewhat pointless since it's a constant, but
                    // it's important because this is one of those invariants that we like to
                    // have in the DFG. Note finally that we use the GetLocalUnlinked opcode
                    // here, since this is being done _after_ the prediction propagation phase
                    // has run - therefore it makes little sense to link the GetLocal operation
                    // into the VariableAccessData and Phi graphs.

                    Node check(CheckArgumentsNotCreated, node.codeOrigin);
                    check.ref();
                    
                    Node phantom(Phantom, node.codeOrigin);
                    phantom.ref();
                    phantom.children = node.children;
                    
                    node.convertToGetLocalUnlinked(
                        static_cast<VirtualRegister>(
                            node.codeOrigin.inlineCallFrame->stackOffset +
                            argumentToOperand(index + 1)));
                    
                    NodeIndex checkNodeIndex = m_graph.size();
                    m_graph.append(check);
                    insertionSet.append(indexInBlock, checkNodeIndex);
                    NodeIndex phantomNodeIndex = m_graph.size();
                    m_graph.append(phantom);
                    insertionSet.append(indexInBlock, phantomNodeIndex);
                    
                    changed = true;
                    break;
                }
                    
                case TearOffArguments: {
                    if (m_createsArguments.contains(node.codeOrigin.inlineCallFrame))
                        continue;
                    
                    node.setOpAndDefaultFlags(Nop);
                    m_graph.clearAndDerefChild1(node);
                    m_graph.clearAndDerefChild2(node);
                    node.setRefCount(0);
                    break;
                }
                    
                default:
                    break;
                }
            }
            insertionSet.execute(*block);
        }
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                NodeIndex nodeIndex = block->at(indexInBlock);
                Node& node = m_graph[nodeIndex];
                if (node.op() != CreateArguments)
                    continue;
                // If this is a CreateArguments for an InlineCallFrame* that does
                // not create arguments, then replace it with a PhantomArguments.
                // PhantomArguments is a non-executing node that just indicates
                // that the node should be reified as an arguments object on OSR
                // exit.
                if (m_createsArguments.contains(node.codeOrigin.inlineCallFrame))
                    continue;
                if (node.shouldGenerate()) {
                    Node phantom(Phantom, node.codeOrigin);
                    phantom.children = node.children;
                    phantom.ref();
                    NodeIndex phantomNodeIndex = m_graph.size();
                    m_graph.append(phantom);
                    insertionSet.append(indexInBlock, phantomNodeIndex);
                }
                node.setOpAndDefaultFlags(PhantomArguments);
                node.children.reset();
                changed = true;
            }
            insertionSet.execute(*block);
        }
        
        if (changed) {
            m_graph.collectGarbage();
            
            // Verify that PhantomArguments nodes are not shouldGenerate().
#if !ASSERT_DISABLED
            for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
                BasicBlock* block = m_graph.m_blocks[blockIndex].get();
                if (!block)
                    continue;
                for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                    NodeIndex nodeIndex = block->at(indexInBlock);
                    Node& node = m_graph[nodeIndex];
                    if (node.op() != PhantomArguments)
                        continue;
                    ASSERT(!node.shouldGenerate());
                }
            }
#endif
        }
        
        return changed;
    }
    
private:
    HashSet<InlineCallFrame*,
            DefaultHash<InlineCallFrame*>::Hash,
            NullableHashTraits<InlineCallFrame*> > m_createsArguments;
    HashMap<VariableAccessData*, ArgumentsAliasingData,
            DefaultHash<VariableAccessData*>::Hash,
            NullableHashTraits<VariableAccessData*> > m_argumentsAliasing;

    void observeBadArgumentsUse(Edge edge)
    {
        if (!edge)
            return;
        
        Node& child = m_graph[edge];
        switch (child.op()) {
        case CreateArguments: {
            m_createsArguments.add(child.codeOrigin.inlineCallFrame);
            break;
        }
            
        case GetLocal: {
            int argumentsRegister = m_graph.uncheckedArgumentsRegisterFor(child.codeOrigin);
            if (argumentsRegister != InvalidVirtualRegister
                && (child.local() == argumentsRegister
                    || child.local() == unmodifiedArgumentsRegister(argumentsRegister))) {
                m_createsArguments.add(child.codeOrigin.inlineCallFrame);
                break;
            }
            
            VariableAccessData* variableAccessData = child.variableAccessData();
            if (variableAccessData->isCaptured())
                break;
            
            ArgumentsAliasingData& data = m_argumentsAliasing.find(variableAccessData)->second;
            data.escapes = true;
            break;
        }
            
        default:
            break;
        }
    }
    
    void observeBadArgumentsUses(Node& node)
    {
        for (unsigned i = m_graph.numChildren(node); i--;)
            observeBadArgumentsUse(m_graph.child(node, i));
    }
    
    void observeProperArgumentsUse(Node& node, Edge edge)
    {
        Node& child = m_graph[edge];
        if (child.op() != GetLocal) {
            // When can this happen? At least two cases that I can think
            // of:
            //
            // 1) Aliased use of arguments in the same basic block,
            //    like:
            //
            //    var a = arguments;
            //    var x = arguments[i];
            //
            // 2) If we're accessing arguments we got from the heap!
                            
            if (child.op() == CreateArguments
                && node.codeOrigin.inlineCallFrame
                   != child.codeOrigin.inlineCallFrame)
                m_createsArguments.add(child.codeOrigin.inlineCallFrame);
            
            return;
        }
                        
        VariableAccessData* variableAccessData = child.variableAccessData();
        if (child.local() == m_graph.uncheckedArgumentsRegisterFor(child.codeOrigin)
            && node.codeOrigin.inlineCallFrame != child.codeOrigin.inlineCallFrame) {
            m_createsArguments.add(child.codeOrigin.inlineCallFrame);
            return;
        }

        if (variableAccessData->isCaptured())
            return;
        
        ArgumentsAliasingData& data = m_argumentsAliasing.find(variableAccessData)->second;
        data.mergeCallContext(node.codeOrigin.inlineCallFrame);
    }
    
    bool isOKToOptimize(Node& source)
    {
        if (m_createsArguments.contains(source.codeOrigin.inlineCallFrame))
            return false;
        
        switch (source.op()) {
        case GetLocal: {
            VariableAccessData* variableAccessData = source.variableAccessData();
            int argumentsRegister = m_graph.uncheckedArgumentsRegisterFor(source.codeOrigin);
            if (argumentsRegister == InvalidVirtualRegister)
                break;
            if (argumentsRegister == variableAccessData->local())
                return true;
            if (unmodifiedArgumentsRegister(argumentsRegister) == variableAccessData->local())
                return true;
            if (variableAccessData->isCaptured())
                break;
            ArgumentsAliasingData& data =
                m_argumentsAliasing.find(variableAccessData)->second;
            if (!data.isValid())
                break;
                            
            return true;
        }
                            
        case CreateArguments: {
            return true;
        }
                            
        default:
            break;
        }
        
        return false;
    }
    
    void removeArgumentsReferencingPhantomChild(Node& node, unsigned edgeIndex)
    {
        Edge edge = node.children.child(edgeIndex);
        if (!edge)
            return;
        
        Node& child = m_graph[edge];
        switch (child.op()) {
        case Phi: // Arises if we had CSE on a GetLocal of the arguments register.
        case GetLocal: // Arises if we had CSE on an arguments access to a variable aliased to the arguments.
        case SetLocal: { // Arises if we had CSE on a GetLocal of the arguments register.
            VariableAccessData* variableAccessData = child.variableAccessData();
            bool isDeadArgumentsRegister =
                variableAccessData->local() ==
                    m_graph.uncheckedArgumentsRegisterFor(child.codeOrigin)
                && !m_createsArguments.contains(child.codeOrigin.inlineCallFrame);
            bool isAliasedArgumentsRegister =
                !variableAccessData->isCaptured()
                && m_argumentsAliasing.find(variableAccessData)->second.isValid()
                && !m_createsArguments.contains(child.codeOrigin.inlineCallFrame);
            if (!isDeadArgumentsRegister && !isAliasedArgumentsRegister)
                break;
            m_graph.deref(edge);
            node.children.removeEdgeFromBag(edgeIndex);
            break;
        }
            
        case CreateArguments: { // Arises if we CSE two GetLocals to the arguments register and then CSE the second use of the GetLocal to the first.
            if (m_createsArguments.contains(child.codeOrigin.inlineCallFrame))
                break;
            m_graph.deref(edge);
            node.children.removeEdgeFromBag(edgeIndex);
            break;
        }
            
        default:
            break;
        }
    }
};

bool performArgumentsSimplification(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Arguments Simplification Phase");
    return runPhase<ArgumentsSimplificationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


