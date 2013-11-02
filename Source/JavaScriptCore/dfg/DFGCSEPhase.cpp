/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#include "DFGCSEPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGEdgeUsesStructure.h"
#include "DFGGraph.h"
#include "DFGPhase.h"
#include "JSCellInlines.h"
#include <wtf/FastBitVector.h>

namespace JSC { namespace DFG {

enum CSEMode { NormalCSE, StoreElimination };

template<CSEMode cseMode>
class CSEPhase : public Phase {
public:
    CSEPhase(Graph& graph)
        : Phase(graph, cseMode == NormalCSE ? "common subexpression elimination" : "store elimination")
    {
    }
    
    bool run()
    {
        ASSERT((cseMode == NormalCSE) == (m_graph.m_fixpointState == FixpointNotConverged));
        ASSERT(m_graph.m_fixpointState != BeforeFixpoint);
        
        m_changed = false;
        
        m_graph.clearReplacements();
        
        for (unsigned blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex)
            performBlockCSE(m_graph.block(blockIndex));
        
        return m_changed;
    }
    
private:
    
    unsigned endIndexForPureCSE()
    {
        unsigned result = m_lastSeen[m_currentNode->op()];
        if (result == UINT_MAX)
            result = 0;
        else
            result++;
        ASSERT(result <= m_indexInBlock);
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("  limit %u: ", result);
#endif
        return result;
    }

    Node* pureCSE(Node* node)
    {
        Edge child1 = node->child1();
        Edge child2 = node->child2();
        Edge child3 = node->child3();
        
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode == child1 || otherNode == child2 || otherNode == child3)
                break;

            if (node->op() != otherNode->op())
                continue;
            
            if (node->arithNodeFlags() != otherNode->arithNodeFlags())
                continue;
            
            Edge otherChild = otherNode->child1();
            if (!otherChild)
                return otherNode;
            if (otherChild != child1)
                continue;
            
            otherChild = otherNode->child2();
            if (!otherChild)
                return otherNode;
            if (otherChild != child2)
                continue;
            
            otherChild = otherNode->child3();
            if (!otherChild)
                return otherNode;
            if (otherChild != child3)
                continue;
            
            return otherNode;
        }
        return 0;
    }
    
    Node* int32ToDoubleCSE(Node* node)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode == node->child1())
                return 0;
            switch (otherNode->op()) {
            case Int32ToDouble:
                if (otherNode->child1() == node->child1())
                    return otherNode;
                break;
            default:
                break;
            }
        }
        return 0;
    }
    
    Node* constantCSE(Node* node)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode->op() != JSConstant)
                continue;
            
            if (otherNode->constantNumber() != node->constantNumber())
                continue;
            
            return otherNode;
        }
        return 0;
    }
    
    Node* weakConstantCSE(Node* node)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode->op() != WeakJSConstant)
                continue;
            
            if (otherNode->weakConstant() != node->weakConstant())
                continue;
            
            return otherNode;
        }
        return 0;
    }
    
    Node* getCalleeLoadElimination()
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetCallee:
                return node;
            default:
                break;
            }
        }
        return 0;
    }
    
    Node* getArrayLengthElimination(Node* array)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetArrayLength:
                if (node->child1() == array)
                    return node;
                break;
                
            case PutByValDirect:
            case PutByVal:
                if (!m_graph.byValIsPure(node))
                    return 0;
                if (node->arrayMode().mayStoreToHole())
                    return 0;
                break;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
            }
        }
        return 0;
    }
    
    Node* globalVarLoadElimination(WriteBarrier<Unknown>* registerPointer)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return node;
                break;
            case PutGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return node->child1().node();
                break;
            default:
                break;
            }
            if (m_graph.clobbersWorld(node))
                break;
        }
        return 0;
    }
    
    Node* scopedVarLoadElimination(Node* registers, unsigned varNumber)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetClosureVar: {
                if (node->child1() == registers && node->varNumber() == varNumber)
                    return node;
                break;
            } 
            case PutClosureVar: {
                if (node->varNumber() != varNumber)
                    break;
                if (node->child2() == registers)
                    return node->child3().node();
                return 0;
            }
            case SetLocal: {
                VariableAccessData* variableAccessData = node->variableAccessData();
                if (variableAccessData->isCaptured()
                    && variableAccessData->local() == static_cast<VirtualRegister>(varNumber))
                    return 0;
                break;
            }
            default:
                break;
            }
            if (m_graph.clobbersWorld(node))
                break;
        }
        return 0;
    }
    
    bool globalVarWatchpointElimination(WriteBarrier<Unknown>* registerPointer)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GlobalVarWatchpoint:
                if (node->registerPointer() == registerPointer)
                    return true;
                break;
            case PutGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return false;
                break;
            default:
                break;
            }
            if (m_graph.clobbersWorld(node))
                break;
        }
        return false;
    }

    bool varInjectionWatchpointElimination()
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node->op() == VarInjectionWatchpoint)
                return true;
            if (m_graph.clobbersWorld(node))
                break;
        }
        return false;
    }
    
    Node* globalVarStoreElimination(WriteBarrier<Unknown>* registerPointer)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case PutGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return node;
                break;
                
            case GetGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return 0;
                break;
                
            default:
                break;
            }
            if (m_graph.clobbersWorld(node) || node->canExit())
                return 0;
        }
        return 0;
    }
    
    Node* scopedVarStoreElimination(Node* scope, Node* registers, unsigned varNumber)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case PutClosureVar: {
                if (node->varNumber() != varNumber)
                    break;
                if (node->child1() == scope && node->child2() == registers)
                    return node;
                return 0;
            }
                
            case GetClosureVar: {
                // Let's be conservative.
                if (node->varNumber() == varNumber)
                    return 0;
                break;
            }
                
            case GetLocal: {
                VariableAccessData* variableAccessData = node->variableAccessData();
                if (variableAccessData->isCaptured()
                    && variableAccessData->local() == static_cast<VirtualRegister>(varNumber))
                    return 0;
                break;
            }

            default:
                break;
            }
            if (m_graph.clobbersWorld(node) || node->canExit())
                return 0;
        }
        return 0;
    }
    
    Node* getByValLoadElimination(Node* child1, Node* child2)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1 || node == child2) 
                break;

            switch (node->op()) {
            case GetByVal:
                if (!m_graph.byValIsPure(node))
                    return 0;
                if (node->child1() == child1 && node->child2() == child2)
                    return node;
                break;
                    
            case PutByValDirect:
            case PutByVal:
            case PutByValAlias: {
                if (!m_graph.byValIsPure(node))
                    return 0;
                if (m_graph.varArgChild(node, 0) == child1 && m_graph.varArgChild(node, 1) == child2)
                    return m_graph.varArgChild(node, 2).node();
                // We must assume that the PutByVal will clobber the location we're getting from.
                // FIXME: We can do better; if we know that the PutByVal is accessing an array of a
                // different type than the GetByVal, then we know that they won't clobber each other.
                // ... except of course for typed arrays, where all typed arrays clobber all other
                // typed arrays!  An Int32Array can alias a Float64Array for example, and so on.
                return 0;
            }
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }

    bool checkFunctionElimination(JSCell* function, Node* child1)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            if (node->op() == CheckFunction && node->child1() == child1 && node->function() == function)
                return true;
        }
        return false;
    }
    
    bool checkExecutableElimination(ExecutableBase* executable, Node* child1)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;

            if (node->op() == CheckExecutable && node->child1() == child1 && node->executable() == executable)
                return true;
        }
        return false;
    }

    bool checkStructureElimination(const StructureSet& structureSet, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case CheckStructure:
                if (node->child1() == child1
                    && structureSet.isSupersetOf(node->structureSet()))
                    return true;
                break;
                
            case StructureTransitionWatchpoint:
                if (node->child1() == child1
                    && structureSet.contains(node->structure()))
                    return true;
                break;
                
            case PutStructure:
                if (node->child1() == child1
                    && structureSet.contains(node->structureTransitionData().newStructure))
                    return true;
                if (structureSet.contains(node->structureTransitionData().previousStructure))
                    return false;
                break;
                
            case PutByOffset:
                // Setting a property cannot change the structure.
                break;
                    
            case PutByValDirect:
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return false;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our structures.
                // But that seems like it would take Effort.
                return false;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return false;
                break;
            }
        }
        return false;
    }
    
    bool structureTransitionWatchpointElimination(Structure* structure, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case CheckStructure:
                if (node->child1() == child1
                    && node->structureSet().containsOnly(structure))
                    return true;
                break;
                
            case PutStructure:
                ASSERT(node->structureTransitionData().previousStructure != structure);
                break;
                
            case PutByOffset:
                // Setting a property cannot change the structure.
                break;
                    
            case PutByValDirect:
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return false;
                
            case StructureTransitionWatchpoint:
                if (node->structure() == structure && node->child1() == child1)
                    return true;
                break;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our structures.
                // But that seems like it would take Effort.
                return false;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return false;
                break;
            }
        }
        return false;
    }
    
    Node* putStructureStoreElimination(Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;
            switch (node->op()) {
            case CheckStructure:
                return 0;
                
            case PhantomPutStructure:
                if (node->child1() == child1) // No need to retrace our steps.
                    return 0;
                break;
                
            case PutStructure:
                if (node->child1() == child1)
                    return node;
                break;
                
            // PutStructure needs to execute if we GC. Hence this needs to
            // be careful with respect to nodes that GC.
            case CreateArguments:
            case TearOffArguments:
            case NewFunctionNoCheck:
            case NewFunction:
            case NewFunctionExpression:
            case CreateActivation:
            case TearOffActivation:
            case ToPrimitive:
            case NewRegexp:
            case NewArrayBuffer:
            case NewArray:
            case NewObject:
            case CreateThis:
            case AllocatePropertyStorage:
            case ReallocatePropertyStorage:
            case TypeOf:
            case ToString:
            case NewStringObject:
            case MakeRope:
            case NewTypedArray:
                return 0;
                
            // This either exits, causes a GC (lazy string allocation), or clobbers
            // the world. The chances of it not doing any of those things are so
            // slim that we might as well not even try to reason about it.
            case GetByVal:
                return 0;
                
            case GetIndexedPropertyStorage:
                if (node->arrayMode().getIndexedPropertyStorageMayTriggerGC())
                    return 0;
                break;
                
            default:
                break;
            }
            if (m_graph.clobbersWorld(node) || node->canExit())
                return 0;
            if (edgesUseStructure(m_graph, node))
                return 0;
        }
        return 0;
    }
    
    Node* getByOffsetLoadElimination(unsigned identifierNumber, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;

            switch (node->op()) {
            case GetByOffset:
                if (node->child1() == child1
                    && m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber)
                    return node;
                break;
                
            case PutByOffset:
                if (m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber) {
                    if (node->child1() == child1) // Must be same property storage.
                        return node->child3().node();
                    return 0;
                }
                break;
                    
            case PutByValDirect:
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return 0;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    Node* putByOffsetStoreElimination(unsigned identifierNumber, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;

            switch (node->op()) {
            case GetByOffset:
                if (m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber)
                    return 0;
                break;
                
            case PutByOffset:
                if (m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber) {
                    if (node->child1() == child1) // Must be same property storage.
                        return node;
                    return 0;
                }
                break;
                    
            case PutByValDirect:
            case PutByVal:
            case PutByValAlias:
            case GetByVal:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return 0;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
            if (node->canExit())
                return 0;
        }
        return 0;
    }
    
    Node* getPropertyStorageLoadElimination(Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case GetButterfly:
                if (node->child1() == child1)
                    return node;
                break;

            case AllocatePropertyStorage:
            case ReallocatePropertyStorage:
                // If we can cheaply prove this is a change to our object's storage, we
                // can optimize and use its result.
                if (node->child1() == child1)
                    return node;
                // Otherwise, we currently can't prove that this doesn't change our object's
                // storage, so we conservatively assume that it may change the storage
                // pointer of any object, including ours.
                return 0;
                    
            case PutByValDirect:
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return 0;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our butterfly.
                // But that seems like it would take Effort.
                return 0;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    bool checkArrayElimination(Node* child1, ArrayMode arrayMode)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case CheckArray:
                if (node->child1() == child1 && node->arrayMode() == arrayMode)
                    return true;
                break;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our array.
                // But that seems like it would take Effort.
                return false;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return false;
                break;
            }
        }
        return false;
    }

    Node* getIndexedPropertyStorageLoadElimination(Node* child1, ArrayMode arrayMode)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case GetIndexedPropertyStorage: {
                if (node->child1() == child1 && node->arrayMode() == arrayMode)
                    return node;
                break;
            }

            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    Node* getTypedArrayByteOffsetLoadElimination(Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case GetTypedArrayByteOffset: {
                if (node->child1() == child1)
                    return node;
                break;
            }

            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    Node* getMyScopeLoadElimination()
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case CreateActivation:
                // This may cause us to return a different scope.
                return 0;
            case GetMyScope:
                return node;
            default:
                break;
            }
        }
        return 0;
    }
    
    Node* getLocalLoadElimination(VirtualRegister local, Node*& relevantLocalOp, bool careAboutClobbering)
    {
        relevantLocalOp = 0;
        
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetLocal:
                if (node->local() == local) {
                    relevantLocalOp = node;
                    return node;
                }
                break;
                
            case GetLocalUnlinked:
                if (node->unlinkedLocal() == local) {
                    relevantLocalOp = node;
                    return node;
                }
                break;
                
            case SetLocal:
                if (node->local() == local) {
                    relevantLocalOp = node;
                    return node->child1().node();
                }
                break;
                
            case PutClosureVar:
                if (static_cast<VirtualRegister>(node->varNumber()) == local)
                    return 0;
                break;
                
            default:
                if (careAboutClobbering && m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    struct SetLocalStoreEliminationResult {
        SetLocalStoreEliminationResult()
            : mayBeAccessed(false)
            , mayExit(false)
            , mayClobberWorld(false)
        {
        }
        
        bool mayBeAccessed;
        bool mayExit;
        bool mayClobberWorld;
    };
    SetLocalStoreEliminationResult setLocalStoreElimination(
        VirtualRegister local, Node* expectedNode)
    {
        SetLocalStoreEliminationResult result;
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetLocal:
            case Flush:
                if (node->local() == local)
                    result.mayBeAccessed = true;
                break;
                
            case GetLocalUnlinked:
                if (node->unlinkedLocal() == local)
                    result.mayBeAccessed = true;
                break;
                
            case SetLocal: {
                if (node->local() != local)
                    break;
                if (node != expectedNode)
                    result.mayBeAccessed = true;
                return result;
            }
                
            case GetClosureVar:
                if (static_cast<VirtualRegister>(node->varNumber()) == local)
                    result.mayBeAccessed = true;
                break;
                
            case GetMyScope:
            case SkipTopScope:
                if (node->codeOrigin.inlineCallFrame)
                    break;
                if (m_graph.uncheckedActivationRegister() == local)
                    result.mayBeAccessed = true;
                break;
                
            case CheckArgumentsNotCreated:
            case GetMyArgumentsLength:
            case GetMyArgumentsLengthSafe:
                if (m_graph.uncheckedArgumentsRegisterFor(node->codeOrigin) == local)
                    result.mayBeAccessed = true;
                break;
                
            case GetMyArgumentByVal:
            case GetMyArgumentByValSafe:
                result.mayBeAccessed = true;
                break;
                
            case GetByVal:
                // If this is accessing arguments then it's potentially accessing locals.
                if (node->arrayMode().type() == Array::Arguments)
                    result.mayBeAccessed = true;
                break;
                
            case CreateArguments:
            case TearOffActivation:
            case TearOffArguments:
                // If an activation is being torn off then it means that captured variables
                // are live. We could be clever here and check if the local qualifies as an
                // argument register. But that seems like it would buy us very little since
                // any kind of tear offs are rare to begin with.
                result.mayBeAccessed = true;
                break;
                
            default:
                break;
            }
            result.mayExit |= node->canExit();
            result.mayClobberWorld |= m_graph.clobbersWorld(node);
        }
        RELEASE_ASSERT_NOT_REACHED();
        // Be safe in release mode.
        result.mayBeAccessed = true;
        return result;
    }
    
    void eliminateIrrelevantPhantomChildren(Node* node)
    {
        ASSERT(node->op() == Phantom);
        for (unsigned i = 0; i < AdjacencyList::Size; ++i) {
            Edge edge = node->children.child(i);
            if (!edge)
                continue;
            if (edge.useKind() != UntypedUse)
                continue; // Keep the type check.
            if (edge->flags() & NodeRelevantToOSR)
                continue;
            
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("   Eliminating edge @", m_currentNode->index(), " -> @", edge->index());
#endif
            node->children.removeEdge(i--);
            m_changed = true;
        }
    }
    
    bool setReplacement(Node* replacement)
    {
        if (!replacement)
            return false;
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("   Replacing @%u -> @%u", m_currentNode->index(), replacement->index());
#endif
        
        m_currentNode->convertToPhantom();
        eliminateIrrelevantPhantomChildren(m_currentNode);
        
        // At this point we will eliminate all references to this node.
        m_currentNode->misc.replacement = replacement;
        
        m_changed = true;
        
        return true;
    }
    
    void eliminate()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("   Eliminating @%u", m_currentNode->index());
#endif
        
        ASSERT(m_currentNode->mustGenerate());
        m_currentNode->convertToPhantom();
        eliminateIrrelevantPhantomChildren(m_currentNode);
        
        m_changed = true;
    }
    
    void eliminate(Node* node, NodeType phantomType = Phantom)
    {
        if (!node)
            return;
        ASSERT(node->mustGenerate());
        node->setOpAndDefaultNonExitFlags(phantomType);
        if (phantomType == Phantom)
            eliminateIrrelevantPhantomChildren(node);
        
        m_changed = true;
    }
    
    void performNodeCSE(Node* node)
    {
        if (cseMode == NormalCSE)
            m_graph.performSubstitution(node);
        
        if (node->op() == SetLocal)
            node->child1()->mergeFlags(NodeRelevantToOSR);
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("   %s @%u: ", Graph::opName(node->op()), node->index());
#endif
        
        switch (node->op()) {
        
        case Identity:
            if (cseMode == StoreElimination)
                break;
            setReplacement(node->child1().node());
            break;
            
        // Handle the pure nodes. These nodes never have any side-effects.
        case BitAnd:
        case BitOr:
        case BitXor:
        case BitRShift:
        case BitLShift:
        case BitURShift:
        case ArithAdd:
        case ArithSub:
        case ArithNegate:
        case ArithMul:
        case ArithIMul:
        case ArithMod:
        case ArithDiv:
        case ArithAbs:
        case ArithMin:
        case ArithMax:
        case ArithSqrt:
        case ArithSin:
        case ArithCos:
        case StringCharAt:
        case StringCharCodeAt:
        case IsUndefined:
        case IsBoolean:
        case IsNumber:
        case IsString:
        case IsObject:
        case IsFunction:
        case DoubleAsInt32:
        case LogicalNot:
        case SkipTopScope:
        case SkipScope:
        case GetClosureRegisters:
        case GetScope:
        case TypeOf:
        case CompareEqConstant:
        case ValueToInt32:
        case MakeRope:
        case Int52ToDouble:
        case Int52ToValue:
            if (cseMode == StoreElimination)
                break;
            setReplacement(pureCSE(node));
            break;
            
        case Int32ToDouble:
            if (cseMode == StoreElimination)
                break;
            setReplacement(int32ToDoubleCSE(node));
            break;
            
        case GetCallee:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getCalleeLoadElimination());
            break;

        case GetLocal: {
            if (cseMode == StoreElimination)
                break;
            VariableAccessData* variableAccessData = node->variableAccessData();
            if (!variableAccessData->isCaptured())
                break;
            Node* relevantLocalOp;
            Node* possibleReplacement = getLocalLoadElimination(variableAccessData->local(), relevantLocalOp, variableAccessData->isCaptured());
            if (!relevantLocalOp)
                break;
            if (relevantLocalOp->op() != GetLocalUnlinked
                && relevantLocalOp->variableAccessData() != variableAccessData)
                break;
            Node* phi = node->child1().node();
            if (!setReplacement(possibleReplacement))
                break;
            
            m_graph.dethread();
            
            // If we replace a GetLocal with a GetLocalUnlinked, then turn the GetLocalUnlinked
            // into a GetLocal.
            if (relevantLocalOp->op() == GetLocalUnlinked)
                relevantLocalOp->convertToGetLocal(variableAccessData, phi);

            m_changed = true;
            break;
        }
            
        case GetLocalUnlinked: {
            if (cseMode == StoreElimination)
                break;
            Node* relevantLocalOpIgnored;
            setReplacement(getLocalLoadElimination(node->unlinkedLocal(), relevantLocalOpIgnored, true));
            break;
        }
            
        case Flush: {
            VariableAccessData* variableAccessData = node->variableAccessData();
            VirtualRegister local = variableAccessData->local();
            Node* replacement = node->child1().node();
            if (replacement->op() != SetLocal)
                break;
            ASSERT(replacement->variableAccessData() == variableAccessData);
            // FIXME: We should be able to remove SetLocals that can exit; we just need
            // to replace them with appropriate type checks.
            if (cseMode == NormalCSE) {
                // Need to be conservative at this time; if the SetLocal has any chance of performing
                // any speculations then we cannot do anything.
                FlushFormat format = variableAccessData->flushFormat();
                ASSERT(format != DeadFlush);
                if (format != FlushedJSValue)
                    break;
            } else {
                if (replacement->canExit())
                    break;
            }
            SetLocalStoreEliminationResult result =
                setLocalStoreElimination(local, replacement);
            if (result.mayBeAccessed || result.mayClobberWorld)
                break;
            ASSERT(replacement->op() == SetLocal);
            // FIXME: Investigate using mayExit as a further optimization.
            node->convertToPhantom();
            Node* dataNode = replacement->child1().node();
            ASSERT(dataNode->hasResult());
            node->child1() = Edge(dataNode);
            m_graph.dethread();
            m_changed = true;
            break;
        }
            
        case JSConstant:
            if (cseMode == StoreElimination)
                break;
            // This is strange, but necessary. Some phases will convert nodes to constants,
            // which may result in duplicated constants. We use CSE to clean this up.
            setReplacement(constantCSE(node));
            break;
            
        case WeakJSConstant:
            if (cseMode == StoreElimination)
                break;
            // FIXME: have CSE for weak constants against strong constants and vice-versa.
            setReplacement(weakConstantCSE(node));
            break;
            
        case GetArrayLength:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getArrayLengthElimination(node->child1().node()));
            break;

        case GetMyScope:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getMyScopeLoadElimination());
            break;
            
        // Handle nodes that are conditionally pure: these are pure, and can
        // be CSE'd, so long as the prediction is the one we want.
        case ValueAdd:
        case CompareLess:
        case CompareLessEq:
        case CompareGreater:
        case CompareGreaterEq:
        case CompareEq: {
            if (cseMode == StoreElimination)
                break;
            if (m_graph.isPredictedNumerical(node)) {
                Node* replacement = pureCSE(node);
                if (replacement && m_graph.isPredictedNumerical(replacement))
                    setReplacement(replacement);
            }
            break;
        }
            
        // Finally handle heap accesses. These are not quite pure, but we can still
        // optimize them provided that some subtle conditions are met.
        case GetGlobalVar:
            if (cseMode == StoreElimination)
                break;
            setReplacement(globalVarLoadElimination(node->registerPointer()));
            break;

        case GetClosureVar: {
            if (cseMode == StoreElimination)
                break;
            setReplacement(scopedVarLoadElimination(node->child1().node(), node->varNumber()));
            break;
        }

        case GlobalVarWatchpoint:
            if (cseMode == StoreElimination)
                break;
            if (globalVarWatchpointElimination(node->registerPointer()))
                eliminate();
            break;
            
        case VarInjectionWatchpoint:
            if (cseMode == StoreElimination)
                break;
            if (varInjectionWatchpointElimination())
                eliminate();
            break;
            
        case PutGlobalVar:
            if (cseMode == NormalCSE)
                break;
            eliminate(globalVarStoreElimination(node->registerPointer()));
            break;
            
        case PutClosureVar: {
            if (cseMode == NormalCSE)
                break;
            eliminate(scopedVarStoreElimination(node->child1().node(), node->child2().node(), node->varNumber()));
            break;
        }

        case GetByVal:
            if (cseMode == StoreElimination)
                break;
            if (m_graph.byValIsPure(node))
                setReplacement(getByValLoadElimination(node->child1().node(), node->child2().node()));
            break;
                
        case PutByValDirect:
        case PutByVal: {
            if (cseMode == StoreElimination)
                break;
            Edge child1 = m_graph.varArgChild(node, 0);
            Edge child2 = m_graph.varArgChild(node, 1);
            if (node->arrayMode().canCSEStorage()) {
                Node* replacement = getByValLoadElimination(child1.node(), child2.node());
                if (!replacement)
                    break;
                node->setOp(PutByValAlias);
            }
            break;
        }
            
        case CheckStructure:
            if (cseMode == StoreElimination)
                break;
            if (checkStructureElimination(node->structureSet(), node->child1().node()))
                eliminate();
            break;
            
        case StructureTransitionWatchpoint:
            if (cseMode == StoreElimination)
                break;
            if (structureTransitionWatchpointElimination(node->structure(), node->child1().node()))
                eliminate();
            break;
            
        case PutStructure:
            if (cseMode == NormalCSE)
                break;
            eliminate(putStructureStoreElimination(node->child1().node()), PhantomPutStructure);
            break;

        case CheckFunction:
            if (cseMode == StoreElimination)
                break;
            if (checkFunctionElimination(node->function(), node->child1().node()))
                eliminate();
            break;
                
        case CheckExecutable:
            if (cseMode == StoreElimination)
                break;
            if (checkExecutableElimination(node->executable(), node->child1().node()))
                eliminate();
            break;
                
        case CheckArray:
            if (cseMode == StoreElimination)
                break;
            if (checkArrayElimination(node->child1().node(), node->arrayMode()))
                eliminate();
            break;
            
        case GetIndexedPropertyStorage: {
            if (cseMode == StoreElimination)
                break;
            setReplacement(getIndexedPropertyStorageLoadElimination(node->child1().node(), node->arrayMode()));
            break;
        }
            
        case GetTypedArrayByteOffset: {
            if (cseMode == StoreElimination)
                break;
            setReplacement(getTypedArrayByteOffsetLoadElimination(node->child1().node()));
            break;
        }

        case GetButterfly:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getPropertyStorageLoadElimination(node->child1().node()));
            break;

        case GetByOffset:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getByOffsetLoadElimination(m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber, node->child1().node()));
            break;
            
        case PutByOffset:
            if (cseMode == NormalCSE)
                break;
            eliminate(putByOffsetStoreElimination(m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber, node->child1().node()));
            break;
            
        case Phantom:
            // FIXME: we ought to remove Phantom's that have no children.
            
            eliminateIrrelevantPhantomChildren(node);
            break;
            
        default:
            // do nothing.
            break;
        }
        
        m_lastSeen[node->op()] = m_indexInBlock;
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("\n");
#endif
    }
    
    void performBlockCSE(BasicBlock* block)
    {
        if (!block)
            return;
        if (!block->isReachable)
            return;
        
        m_currentBlock = block;
        for (unsigned i = 0; i < LastNodeType; ++i)
            m_lastSeen[i] = UINT_MAX;
        
        // All Phis need to already be marked as relevant to OSR.
        if (!ASSERT_DISABLED) {
            for (unsigned i = 0; i < block->phis.size(); ++i)
                ASSERT(block->phis[i]->flags() & NodeRelevantToOSR);
        }
        
        // Make all of my SetLocal and GetLocal nodes relevant to OSR, and do some other
        // necessary bookkeeping.
        for (unsigned i = 0; i < block->size(); ++i) {
            Node* node = block->at(i);
            
            switch (node->op()) {
            case SetLocal:
            case GetLocal: // FIXME: The GetLocal case is only necessary until we do https://bugs.webkit.org/show_bug.cgi?id=106707.
                node->mergeFlags(NodeRelevantToOSR);
                break;
            default:
                node->clearFlags(NodeRelevantToOSR);
                break;
            }
        }

        for (m_indexInBlock = 0; m_indexInBlock < block->size(); ++m_indexInBlock) {
            m_currentNode = block->at(m_indexInBlock);
            performNodeCSE(m_currentNode);
        }
        
        if (!ASSERT_DISABLED && cseMode == StoreElimination) {
            // Nobody should have replacements set.
            for (unsigned i = 0; i < block->size(); ++i)
                ASSERT(!block->at(i)->misc.replacement);
        }
    }
    
    BasicBlock* m_currentBlock;
    Node* m_currentNode;
    unsigned m_indexInBlock;
    FixedArray<unsigned, LastNodeType> m_lastSeen;
    bool m_changed; // Only tracks changes that have a substantive effect on other optimizations.
};

bool performCSE(Graph& graph)
{
    SamplingRegion samplingRegion("DFG CSE Phase");
    return runPhase<CSEPhase<NormalCSE>>(graph);
}

bool performStoreElimination(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Store Elimination Phase");
    return runPhase<CSEPhase<StoreElimination>>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


