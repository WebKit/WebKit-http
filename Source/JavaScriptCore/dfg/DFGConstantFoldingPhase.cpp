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
#include "DFGConstantFoldingPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractInterpreterInlines.h"
#include "DFGBasicBlock.h"
#include "DFGGraph.h"
#include "DFGInPlaceAbstractState.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "GetByIdStatus.h"
#include "Operations.h"
#include "PutByIdStatus.h"

namespace JSC { namespace DFG {

class ConstantFoldingPhase : public Phase {
public:
    ConstantFoldingPhase(Graph& graph)
        : Phase(graph, "constant folding")
        , m_state(graph)
        , m_interpreter(graph, m_state)
        , m_insertionSet(graph)
    {
    }
    
    bool run()
    {
        bool changed = false;
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            if (block->cfaFoundConstants)
                changed |= foldConstants(block);
        }
        
        return changed;
    }

private:
    bool foldConstants(BasicBlock* block)
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("Constant folding considering Block ", *block, ".\n");
#endif
        bool changed = false;
        m_state.beginBasicBlock(block);
        for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
            if (!m_state.isValid())
                break;
            
            Node* node = block->at(indexInBlock);

            bool eliminated = false;
                    
            switch (node->op()) {
            case CheckArgumentsNotCreated: {
                if (!isEmptySpeculation(
                        m_state.variables().operand(
                            m_graph.argumentsRegisterFor(node->codeOrigin)).m_type))
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                    
            case CheckStructure:
            case ArrayifyToStructure: {
                AbstractValue& value = m_state.forNode(node->child1());
                StructureSet set;
                if (node->op() == ArrayifyToStructure)
                    set = node->structure();
                else
                    set = node->structureSet();
                if (value.m_currentKnownStructure.isSubsetOf(set)) {
                    m_interpreter.execute(indexInBlock); // Catch the fact that we may filter on cell.
                    node->convertToPhantom();
                    eliminated = true;
                    break;
                }
                StructureAbstractValue& structureValue = value.m_futurePossibleStructure;
                if (structureValue.isSubsetOf(set)
                    && structureValue.hasSingleton()) {
                    Structure* structure = structureValue.singleton();
                    m_interpreter.execute(indexInBlock); // Catch the fact that we may filter on cell.
                    AdjacencyList children = node->children;
                    children.removeEdge(0);
                    if (!!children.child1()) {
                        Node phantom(Phantom, node->codeOrigin, children);
                        if (node->flags() & NodeExitsForward)
                            phantom.mergeFlags(NodeExitsForward);
                        m_insertionSet.insertNode(indexInBlock, SpecNone, phantom);
                    }
                    node->children.setChild2(Edge());
                    node->children.setChild3(Edge());
                    node->convertToStructureTransitionWatchpoint(structure);
                    eliminated = true;
                    break;
                }
                break;
            }
                
            case CheckArray:
            case Arrayify: {
                if (!node->arrayMode().alreadyChecked(m_graph, node, m_state.forNode(node->child1())))
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                
            case CheckFunction: {
                if (m_state.forNode(node->child1()).value() != node->function())
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                
            case GetById:
            case GetByIdFlush: {
                CodeOrigin codeOrigin = node->codeOrigin;
                Edge childEdge = node->child1();
                Node* child = childEdge.node();
                unsigned identifierNumber = node->identifierNumber();
                
                if (childEdge.useKind() != CellUse)
                    break;
                
                Structure* structure = m_state.forNode(child).bestProvenStructure();
                if (!structure)
                    break;
                
                bool needsWatchpoint = !m_state.forNode(child).m_currentKnownStructure.hasSingleton();
                bool needsCellCheck = m_state.forNode(child).m_type & ~SpecCell;
                
                GetByIdStatus status = GetByIdStatus::computeFor(
                    vm(), structure, m_graph.identifiers()[identifierNumber]);
                
                if (!status.isSimple()) {
                    // FIXME: We could handle prototype cases.
                    // https://bugs.webkit.org/show_bug.cgi?id=110386
                    break;
                }
                
                ASSERT(status.structureSet().size() == 1);
                ASSERT(!status.chain());
                ASSERT(status.structureSet().singletonStructure() == structure);
                
                // Now before we do anything else, push the CFA forward over the GetById
                // and make sure we signal to the loop that it should continue and not
                // do any eliminations.
                m_interpreter.execute(indexInBlock);
                eliminated = true;
                
                if (needsWatchpoint) {
                    ASSERT(m_state.forNode(child).m_futurePossibleStructure.isSubsetOf(StructureSet(structure)));
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, StructureTransitionWatchpoint, codeOrigin,
                        OpInfo(structure), childEdge);
                } else if (needsCellCheck) {
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, Phantom, codeOrigin, childEdge);
                }
                
                childEdge.setUseKind(KnownCellUse);
                
                Edge propertyStorage;
                
                if (isInlineOffset(status.offset()))
                    propertyStorage = childEdge;
                else {
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, GetButterfly, codeOrigin, childEdge));
                }
                
                node->convertToGetByOffset(m_graph.m_storageAccessData.size(), propertyStorage);
                
                StorageAccessData storageAccessData;
                storageAccessData.offset = status.offset();
                storageAccessData.identifierNumber = identifierNumber;
                m_graph.m_storageAccessData.append(storageAccessData);
                break;
            }
                
            case PutById:
            case PutByIdDirect: {
                CodeOrigin codeOrigin = node->codeOrigin;
                Edge childEdge = node->child1();
                Node* child = childEdge.node();
                unsigned identifierNumber = node->identifierNumber();
                
                ASSERT(childEdge.useKind() == CellUse);
                
                Structure* structure = m_state.forNode(child).bestProvenStructure();
                if (!structure)
                    break;
                
                bool needsWatchpoint = !m_state.forNode(child).m_currentKnownStructure.hasSingleton();
                bool needsCellCheck = m_state.forNode(child).m_type & ~SpecCell;
                
                PutByIdStatus status = PutByIdStatus::computeFor(
                    vm(),
                    m_graph.globalObjectFor(codeOrigin),
                    structure,
                    m_graph.identifiers()[identifierNumber],
                    node->op() == PutByIdDirect);
                
                if (!status.isSimpleReplace() && !status.isSimpleTransition())
                    break;
                
                ASSERT(status.oldStructure() == structure);
                
                // Now before we do anything else, push the CFA forward over the PutById
                // and make sure we signal to the loop that it should continue and not
                // do any eliminations.
                m_interpreter.execute(indexInBlock);
                eliminated = true;
                
                if (needsWatchpoint) {
                    ASSERT(m_state.forNode(child).m_futurePossibleStructure.isSubsetOf(StructureSet(structure)));
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, StructureTransitionWatchpoint, codeOrigin,
                        OpInfo(structure), childEdge);
                } else if (needsCellCheck) {
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, Phantom, codeOrigin, childEdge);
                }
                
                childEdge.setUseKind(KnownCellUse);
                
                StructureTransitionData* transitionData = 0;
                if (status.isSimpleTransition()) {
                    transitionData = m_graph.addStructureTransitionData(
                        StructureTransitionData(structure, status.newStructure()));
                    
                    if (node->op() == PutById) {
                        if (!structure->storedPrototype().isNull()) {
                            addStructureTransitionCheck(
                                codeOrigin, indexInBlock,
                                structure->storedPrototype().asCell());
                        }
                        
                        m_graph.chains().addLazily(status.structureChain());
                        
                        for (unsigned i = 0; i < status.structureChain()->size(); ++i) {
                            JSValue prototype = status.structureChain()->at(i)->storedPrototype();
                            if (prototype.isNull())
                                continue;
                            ASSERT(prototype.isCell());
                            addStructureTransitionCheck(
                                codeOrigin, indexInBlock, prototype.asCell());
                        }
                    }
                }
                
                Edge propertyStorage;
                
                if (isInlineOffset(status.offset()))
                    propertyStorage = childEdge;
                else if (status.isSimpleReplace() || structure->outOfLineCapacity() == status.newStructure()->outOfLineCapacity()) {
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, GetButterfly, codeOrigin, childEdge));
                } else if (!structure->outOfLineCapacity()) {
                    ASSERT(status.newStructure()->outOfLineCapacity());
                    ASSERT(!isInlineOffset(status.offset()));
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, AllocatePropertyStorage,
                        codeOrigin, OpInfo(transitionData), childEdge));
                } else {
                    ASSERT(structure->outOfLineCapacity());
                    ASSERT(status.newStructure()->outOfLineCapacity() > structure->outOfLineCapacity());
                    ASSERT(!isInlineOffset(status.offset()));
                    
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, ReallocatePropertyStorage, codeOrigin,
                        OpInfo(transitionData), childEdge,
                        Edge(m_insertionSet.insertNode(
                            indexInBlock, SpecNone, GetButterfly, codeOrigin, childEdge))));
                }
                
                if (status.isSimpleTransition()) {
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, PutStructure, codeOrigin, 
                        OpInfo(transitionData), childEdge);
                }
                
                node->convertToPutByOffset(m_graph.m_storageAccessData.size(), propertyStorage);
                
                StorageAccessData storageAccessData;
                storageAccessData.offset = status.offset();
                storageAccessData.identifierNumber = identifierNumber;
                m_graph.m_storageAccessData.append(storageAccessData);
                break;
            }
                
            default:
                break;
            }
                
            if (eliminated) {
                changed = true;
                continue;
            }
                
            m_interpreter.execute(indexInBlock);
            if (!node->shouldGenerate() || m_state.didClobber() || node->hasConstant())
                continue;
            JSValue value = m_state.forNode(node).value();
            if (!value)
                continue;
            
            // Check if merging the abstract value of the constant into the abstract value
            // we've proven for this node wouldn't widen the proof. If it widens the proof
            // (i.e. says that the set contains more things in it than it previously did)
            // then we refuse to fold.
            AbstractValue oldValue = m_state.forNode(node);
            AbstractValue constantValue;
            constantValue.set(m_graph, value);
            if (oldValue.merge(constantValue))
                continue;
                
            CodeOrigin codeOrigin = node->codeOrigin;
            AdjacencyList children = node->children;
            
            if (node->op() == GetLocal) {
                // GetLocals without a Phi child are guaranteed dead. We don't have to
                // do anything about them.
                if (!node->child1())
                    continue;
                
                if (m_graph.m_form != LoadStore) {
                    VariableAccessData* variable = node->variableAccessData();
                    Node* phi = node->child1().node();
                    if (phi->op() == Phi
                        && block->variablesAtHead.operand(variable->local()) == phi
                        && block->variablesAtTail.operand(variable->local()) == node) {
                        
                        // Keep the graph threaded for easy cases. This is improves compile
                        // times. It would be correct to just dethread here.
                        
                        m_graph.convertToConstant(node, value);
                        Node* phantom = m_insertionSet.insertNode(
                            indexInBlock, SpecNone, PhantomLocal,  codeOrigin,
                            OpInfo(variable), Edge(phi));
                        block->variablesAtHead.operand(variable->local()) = phantom;
                        block->variablesAtTail.operand(variable->local()) = phantom;
                        
                        changed = true;
                        
                        continue;
                    }
                    
                    m_graph.dethread();
                }
            } else
                ASSERT(!node->hasVariableAccessData(m_graph));
            
            m_graph.convertToConstant(node, value);
            m_insertionSet.insertNode(
                indexInBlock, SpecNone, Phantom, codeOrigin, children);
            
            changed = true;
        }
        m_state.reset();
        m_insertionSet.execute(block);
        
        return changed;
    }

    void addStructureTransitionCheck(CodeOrigin codeOrigin, unsigned indexInBlock, JSCell* cell)
    {
        Node* weakConstant = m_insertionSet.insertNode(
            indexInBlock, speculationFromValue(cell), WeakJSConstant, codeOrigin, OpInfo(cell));
        
        if (m_graph.watchpoints().isStillValid(cell->structure()->transitionWatchpointSet())) {
            m_insertionSet.insertNode(
                indexInBlock, SpecNone, StructureTransitionWatchpoint, codeOrigin,
                OpInfo(cell->structure()), Edge(weakConstant, CellUse));
            return;
        }

        m_insertionSet.insertNode(
            indexInBlock, SpecNone, CheckStructure, codeOrigin,
            OpInfo(m_graph.addStructureSet(cell->structure())), Edge(weakConstant, CellUse));
    }
    
    InPlaceAbstractState m_state;
    AbstractInterpreter<InPlaceAbstractState> m_interpreter;
    InsertionSet m_insertionSet;
};

bool performConstantFolding(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Constant Folding Phase");
    return runPhase<ConstantFoldingPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


