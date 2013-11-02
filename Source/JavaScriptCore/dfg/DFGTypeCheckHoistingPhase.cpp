/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#include "DFGTypeCheckHoistingPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGBasicBlock.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "DFGVariableAccessDataDump.h"
#include "Operations.h"
#include <wtf/HashMap.h>

namespace JSC { namespace DFG {

enum CheckBallot { VoteOther, VoteStructureCheck = 1, VoteCheckArray = 1 };

struct ArrayTypeCheck;
struct StructureTypeCheck;

struct CheckData {
    Structure* m_structure;
    ArrayMode m_arrayMode;
    bool m_arrayModeIsValid;
    bool m_arrayModeHoistingOkay;
    
    CheckData()
        : m_structure(0)
        , m_arrayModeIsValid(false)
        , m_arrayModeHoistingOkay(false)
    {
    }
    
    CheckData(Structure* structure)
        : m_structure(structure)
        , m_arrayModeIsValid(false)
        , m_arrayModeHoistingOkay(true)
    {
    }

    CheckData(ArrayMode arrayMode)
        : m_structure(0)
        , m_arrayMode(arrayMode)
        , m_arrayModeIsValid(true)
        , m_arrayModeHoistingOkay(true)
    {
    }

    void disableCheckArrayHoisting()
    {
        m_arrayModeIsValid = false;
        m_arrayModeHoistingOkay = false;
    }
};
    
class TypeCheckHoistingPhase : public Phase {
public:
    TypeCheckHoistingPhase(Graph& graph)
        : Phase(graph, "structure check hoisting")
    {
    }
    
    bool run()
    {
        ASSERT(m_graph.m_form == ThreadedCPS);

        clearVariableVotes();
        identifyRedundantStructureChecks();
        disableHoistingForVariablesWithInsufficientVotes<StructureTypeCheck>();

        clearVariableVotes();
        identifyRedundantArrayChecks();
        disableHoistingForVariablesWithInsufficientVotes<ArrayTypeCheck>();

        disableHoistingAcrossOSREntries<StructureTypeCheck>();
        disableHoistingAcrossOSREntries<ArrayTypeCheck>();

        bool changed = false;

        // Place CheckStructure's at SetLocal sites.
        InsertionSet insertionSet(m_graph);
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                Node* node = block->at(indexInBlock);
                // Be careful not to use 'node' after appending to the graph. In those switch
                // cases where we need to append, we first carefully extract everything we need
                // from the node, before doing any appending.
                switch (node->op()) {
                case SetArgument: {
                    ASSERT(!blockIndex);
                    // Insert a GetLocal and a CheckStructure immediately following this
                    // SetArgument, if the variable was a candidate for structure hoisting.
                    // If the basic block previously only had the SetArgument as its
                    // variable-at-tail, then replace it with this GetLocal.
                    VariableAccessData* variable = node->variableAccessData();
                    HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
                    if (iter == m_map.end())
                        break;
                    if (!iter->value.m_structure && !iter->value.m_arrayModeIsValid)
                        break;

                    CodeOrigin codeOrigin = node->codeOrigin;
                    
                    Node* getLocal = insertionSet.insertNode(
                        indexInBlock + 1, variable->prediction(), GetLocal, codeOrigin,
                        OpInfo(variable), Edge(node));
                    if (iter->value.m_structure) {
                        insertionSet.insertNode(
                            indexInBlock + 1, SpecNone, CheckStructure, codeOrigin,
                            OpInfo(m_graph.addStructureSet(iter->value.m_structure)),
                            Edge(getLocal, CellUse));
                    } else if (iter->value.m_arrayModeIsValid) {
                        ASSERT(iter->value.m_arrayModeHoistingOkay);
                        insertionSet.insertNode(
                            indexInBlock + 1, SpecNone, CheckArray, codeOrigin,
                            OpInfo(iter->value.m_arrayMode.asWord()),
                            Edge(getLocal, CellUse));
                    } else
                        RELEASE_ASSERT_NOT_REACHED();

                    if (block->variablesAtTail.operand(variable->local()) == node)
                        block->variablesAtTail.operand(variable->local()) = getLocal;
                    
                    m_graph.substituteGetLocal(*block, indexInBlock, variable, getLocal);
                    
                    changed = true;
                    break;
                }
                    
                case SetLocal: {
                    VariableAccessData* variable = node->variableAccessData();
                    HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
                    if (iter == m_map.end())
                        break;
                    if (!iter->value.m_structure && !iter->value.m_arrayModeIsValid)
                        break;

                    // First insert a dead SetLocal to tell OSR that the child's value should
                    // be dropped into this bytecode variable if the CheckStructure decides
                    // to exit.
                    
                    CodeOrigin codeOrigin = node->codeOrigin;
                    Edge child1 = node->child1();
                    
                    insertionSet.insertNode(
                        indexInBlock, SpecNone, SetLocal, codeOrigin, OpInfo(variable), child1);

                    // Use NodeExitsForward to indicate that we should exit to the next
                    // bytecode instruction rather than reexecuting the current one.
                    Node* newNode = 0;
                    if (iter->value.m_structure) {
                        newNode = insertionSet.insertNode(
                            indexInBlock, SpecNone, CheckStructure, codeOrigin,
                            OpInfo(m_graph.addStructureSet(iter->value.m_structure)),
                            Edge(child1.node(), CellUse));
                    } else if (iter->value.m_arrayModeIsValid) {
                        ASSERT(iter->value.m_arrayModeHoistingOkay);
                        newNode = insertionSet.insertNode(
                            indexInBlock, SpecNone, CheckArray, codeOrigin,
                            OpInfo(iter->value.m_arrayMode.asWord()),
                            Edge(child1.node(), CellUse));
                    } else
                        RELEASE_ASSERT_NOT_REACHED();
                    newNode->mergeFlags(NodeExitsForward);
                    changed = true;
                    break;
                }
                    
                default:
                    break;
                }
            }
            insertionSet.execute(block);
        }
        
        return changed;
    }

private:
    void clearVariableVotes()
    { 
        for (unsigned i = m_graph.m_variableAccessData.size(); i--;) {
            VariableAccessData* variable = &m_graph.m_variableAccessData[i];
            if (!variable->isRoot())
                continue;
            variable->clearVotes();
        }
    }
        
    // Identify the set of variables that are always subject to the same structure
    // checks. For now, only consider monomorphic structure checks (one structure).
    void identifyRedundantStructureChecks()
    {    
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                Node* node = block->at(indexInBlock);
                switch (node->op()) {
                case CheckStructure:
                case StructureTransitionWatchpoint: {
                    // We currently rely on the fact that we're the only ones who would
                    // insert these nodes with NodeExitsForward.
                    RELEASE_ASSERT(!(node->flags() & NodeExitsForward));
                    Node* child = node->child1().node();
                    if (child->op() != GetLocal)
                        break;
                    VariableAccessData* variable = child->variableAccessData();
                    variable->vote(VoteStructureCheck);
                    if (!shouldConsiderForHoisting<StructureTypeCheck>(variable))
                        break;
                    noticeStructureCheck(variable, node->structureSet());
                    break;
                }
                    
                case GetByOffset:
                case PutByOffset:
                case PutStructure:
                case AllocatePropertyStorage:
                case ReallocatePropertyStorage:
                case GetButterfly:
                case GetByVal:
                case PutByValDirect:
                case PutByVal:
                case PutByValAlias:
                case GetArrayLength:
                case CheckArray:
                case GetIndexedPropertyStorage:
                case GetTypedArrayByteOffset:
                case Phantom:
                    // Don't count these uses.
                    break;
                    
                case ArrayifyToStructure:
                case Arrayify:
                    if (node->arrayMode().conversion() == Array::RageConvert) {
                        // Rage conversion changes structures. We should avoid tying to do
                        // any kind of hoisting when rage conversion is in play.
                        Node* child = node->child1().node();
                        if (child->op() != GetLocal)
                            break;
                        VariableAccessData* variable = child->variableAccessData();
                        variable->vote(VoteOther);
                        if (!shouldConsiderForHoisting<StructureTypeCheck>(variable))
                            break;
                        noticeStructureCheck(variable, 0);
                    }
                    break;
                    
                case SetLocal: {
                    // Find all uses of the source of the SetLocal. If any of them are a
                    // kind of CheckStructure, then we should notice them to ensure that
                    // we're not hoisting a check that would contravene checks that are
                    // already being performed.
                    VariableAccessData* variable = node->variableAccessData();
                    if (!shouldConsiderForHoisting<StructureTypeCheck>(variable))
                        break;
                    Node* source = node->child1().node();
                    for (unsigned subIndexInBlock = 0; subIndexInBlock < block->size(); ++subIndexInBlock) {
                        Node* subNode = block->at(subIndexInBlock);
                        switch (subNode->op()) {
                        case CheckStructure: {
                            if (subNode->child1() != source)
                                break;
                            
                            noticeStructureCheck(variable, subNode->structureSet());
                            break;
                        }
                        case StructureTransitionWatchpoint: {
                            if (subNode->child1() != source)
                                break;
                            
                            noticeStructureCheck(variable, subNode->structure());
                            break;
                        }
                        default:
                            break;
                        }
                    }
                    
                    m_graph.voteChildren(node, VoteOther);
                    break;
                }
                    
                default:
                    m_graph.voteChildren(node, VoteOther);
                    break;
                }
            }
        }
    }
        
    void identifyRedundantArrayChecks()
    {
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                Node* node = block->at(indexInBlock);
                switch (node->op()) {
                case CheckArray: {
                    // We currently rely on the fact that we're the only ones who would
                    // insert these nodes with NodeExitsForward.
                    RELEASE_ASSERT(!(node->flags() & NodeExitsForward));
                    Node* child = node->child1().node();
                    if (child->op() != GetLocal)
                        break;
                    VariableAccessData* variable = child->variableAccessData();
                    variable->vote(VoteCheckArray);
                    if (!shouldConsiderForHoisting<ArrayTypeCheck>(variable))
                        break;
                    noticeCheckArray(variable, node->arrayMode());
                    break;
                }

                case CheckStructure:
                case StructureTransitionWatchpoint:
                case GetByOffset:
                case PutByOffset:
                case PutStructure:
                case ReallocatePropertyStorage:
                case GetButterfly:
                case GetByVal:
                case PutByValDirect:
                case PutByVal:
                case PutByValAlias:
                case GetArrayLength:
                case GetIndexedPropertyStorage:
                case Phantom:
                    // Don't count these uses.
                    break;
                    
                case AllocatePropertyStorage:
                case ArrayifyToStructure:
                case Arrayify: {
                    // Any Arrayify could change our indexing type, so disable
                    // all CheckArray hoisting.
                    Node* child = node->child1().node();
                    if (child->op() != GetLocal)
                        break;
                    VariableAccessData* variable = child->variableAccessData();
                    variable->vote(VoteOther);
                    if (!shouldConsiderForHoisting<ArrayTypeCheck>(variable))
                        break;
                    disableCheckArrayHoisting(variable);
                    break;
                }
                    
                case SetLocal: {
                    // Find all uses of the source of the SetLocal. If any of them are a
                    // kind of CheckStructure, then we should notice them to ensure that
                    // we're not hoisting a check that would contravene checks that are
                    // already being performed.
                    VariableAccessData* variable = node->variableAccessData();
                    if (!shouldConsiderForHoisting<ArrayTypeCheck>(variable))
                        break;
                    Node* source = node->child1().node();
                    for (unsigned subIndexInBlock = 0; subIndexInBlock < block->size(); ++subIndexInBlock) {
                        Node* subNode = block->at(subIndexInBlock);
                        switch (subNode->op()) {
                        case CheckStructure: {
                            if (subNode->child1() != source)
                                break;
                            
                            noticeStructureCheckAccountingForArrayMode(variable, subNode->structureSet());
                            break;
                        }
                        case StructureTransitionWatchpoint: {
                            if (subNode->child1() != source)
                                break;
                            
                            noticeStructureCheckAccountingForArrayMode(variable, subNode->structure());
                            break;
                        }
                        case CheckArray: {
                            if (subNode->child1() != source)
                                break;
                            noticeCheckArray(variable, subNode->arrayMode());
                            break;
                        }
                        default:
                            break;
                        }
                    }
                    
                    m_graph.voteChildren(node, VoteOther);
                    break;
                }
                    
                default:
                    m_graph.voteChildren(node, VoteOther);
                    break;
                }
            }
        }
    }

    // Disable hoisting on variables that appear to mostly be used in
    // contexts where it doesn't make sense.
    template <typename TypeCheck>
    void disableHoistingForVariablesWithInsufficientVotes()
    {    
        for (unsigned i = m_graph.m_variableAccessData.size(); i--;) {
            VariableAccessData* variable = &m_graph.m_variableAccessData[i];
            if (!variable->isRoot())
                continue;
            if (TypeCheck::hasEnoughVotesToHoist(variable))
                continue;
            HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
            if (iter == m_map.end())
                continue;
            TypeCheck::disableHoisting(iter->value);
        }
    }

    // Disable check hoisting for variables that cross the OSR entry that
    // we're currently taking, and where the value currently does not have the
    // particular form we want (e.g. a contradictory ArrayMode or Struture).
    template <typename TypeCheck>
    void disableHoistingAcrossOSREntries()
    {
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            ASSERT(block->isReachable);
            if (!block->isOSRTarget)
                continue;
            if (block->bytecodeBegin != m_graph.m_plan.osrEntryBytecodeIndex)
                continue;
            for (size_t i = 0; i < m_graph.m_plan.mustHandleValues.size(); ++i) {
                int operand = m_graph.m_plan.mustHandleValues.operandForIndex(i);
                Node* node = block->variablesAtHead.operand(operand);
                if (!node)
                    continue;
                VariableAccessData* variable = node->variableAccessData();
                HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
                if (iter == m_map.end())
                    continue;
                if (!TypeCheck::isValidToHoist(iter->value))
                    continue;
                JSValue value = m_graph.m_plan.mustHandleValues[i];
                if (!value || !value.isCell() || TypeCheck::isContravenedByValue(iter->value, value)) {
                    TypeCheck::disableHoisting(iter->value);
                    continue;
                }
            }
        }
    }

    void disableCheckArrayHoisting(VariableAccessData* variable)
    {
        HashMap<VariableAccessData*, CheckData>::AddResult result = m_map.add(variable, CheckData());
        result.iterator->value.disableCheckArrayHoisting();
    }

    template <typename TypeCheck>
    bool shouldConsiderForHoisting(VariableAccessData* variable)
    {
        if (!variable->shouldUnboxIfPossible())
            return false;
        if (TypeCheck::hoistingPreviouslyFailed(variable))
            return false;
        if (!isCellSpeculation(variable->prediction()))
            return false;
        return true;
    }
    
    void noticeStructureCheck(VariableAccessData* variable, Structure* structure)
    {
        HashMap<VariableAccessData*, CheckData>::AddResult result = m_map.add(variable, CheckData(structure));
        if (result.isNewEntry)
            return;
        if (result.iterator->value.m_structure == structure)
            return;
        result.iterator->value.m_structure = 0;
    }
    
    void noticeStructureCheck(VariableAccessData* variable, const StructureSet& set)
    {
        if (set.size() != 1) {
            noticeStructureCheck(variable, 0);
            return;
        }
        noticeStructureCheck(variable, set.singletonStructure());
    }

    void noticeCheckArray(VariableAccessData* variable, ArrayMode arrayMode)
    {
        HashMap<VariableAccessData*, CheckData>::AddResult result = m_map.add(variable, CheckData(arrayMode));
        if (result.isNewEntry)
            return;
        if (!result.iterator->value.m_arrayModeHoistingOkay)
            return;
        if (result.iterator->value.m_arrayMode == arrayMode)
            return;
        if (!result.iterator->value.m_arrayModeIsValid) {
            result.iterator->value.m_arrayMode = arrayMode;
            result.iterator->value.m_arrayModeIsValid = true;
            return;
        }
        result.iterator->value.disableCheckArrayHoisting();
    }

    void noticeStructureCheckAccountingForArrayMode(VariableAccessData* variable, Structure* structure)
    {
        HashMap<VariableAccessData*, CheckData>::iterator result = m_map.find(variable);
        if (result == m_map.end())
            return;
        if (!result->value.m_arrayModeHoistingOkay || !result->value.m_arrayModeIsValid)
            return;
        if (result->value.m_arrayMode.structureWouldPassArrayModeFiltering(structure))
            return;
        result->value.disableCheckArrayHoisting();
    }

    void noticeStructureCheckAccountingForArrayMode(VariableAccessData* variable, const StructureSet& set)
    {
        for (unsigned i = 0; i < set.size(); i++)
            noticeStructureCheckAccountingForArrayMode(variable, set[i]);
    }

    HashMap<VariableAccessData*, CheckData> m_map;
};

bool performTypeCheckHoisting(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Type Check Hoisting Phase");
    return runPhase<TypeCheckHoistingPhase>(graph);
}

struct ArrayTypeCheck {
    static bool isValidToHoist(CheckData& checkData)
    {
        return checkData.m_arrayModeIsValid;
    }

    static void disableHoisting(CheckData& checkData)
    {
        checkData.disableCheckArrayHoisting();
    }

    static bool isContravenedByValue(CheckData& checkData, JSValue value)
    {
        ASSERT(value.isCell());
        return !checkData.m_arrayMode.structureWouldPassArrayModeFiltering(value.asCell()->structure());
    }

    static bool hasEnoughVotesToHoist(VariableAccessData* variable)
    {
        return variable->voteRatio() >= Options::checkArrayVoteRatioForHoisting();
    }

    static bool hoistingPreviouslyFailed(VariableAccessData* variable)
    {
        return variable->checkArrayHoistingFailed();
    }
};

struct StructureTypeCheck {
    static bool isValidToHoist(CheckData& checkData)
    {
        return checkData.m_structure;
    }

    static void disableHoisting(CheckData& checkData)
    {
        checkData.m_structure = 0;
    }

    static bool isContravenedByValue(CheckData& checkData, JSValue value)
    {
        ASSERT(value.isCell());
        return checkData.m_structure != value.asCell()->structure();
    }

    static bool hasEnoughVotesToHoist(VariableAccessData* variable)
    {
        return variable->voteRatio() >= Options::structureCheckVoteRatioForHoisting();
    }

    static bool hoistingPreviouslyFailed(VariableAccessData* variable)
    {
        return variable->structureCheckHoistingFailed();
    }
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


