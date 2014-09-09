/*
 * Copyright (C) 2011, 2012, 2013, 2014 Apple Inc. All rights reserved.
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
#include "DFGPredictionPropagationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "DFGPhase.h"
#include "JSCInlines.h"

namespace JSC { namespace DFG {

SpeculatedType resultOfToPrimitive(SpeculatedType type)
{
    if (type & SpecObject) {
        // Objects get turned into strings. So if the input has hints of objectness,
        // the output will have hinsts of stringiness.
        return mergeSpeculations(type & ~SpecObject, SpecString);
    }
    
    return type;
}

class PredictionPropagationPhase : public Phase {
public:
    PredictionPropagationPhase(Graph& graph)
        : Phase(graph, "prediction propagation")
    {
    }
    
    bool run()
    {
        ASSERT(m_graph.m_form == ThreadedCPS);
        ASSERT(m_graph.m_unificationState == GloballyUnified);

        m_pass = PrimaryPass;
        propagateToFixpoint();
        
        m_pass = RareCasePass;
        propagateToFixpoint();
        
        m_pass = DoubleVotingPass;
        do {
            m_changed = false;
            doRoundOfDoubleVoting();
            if (!m_changed)
                break;
            m_changed = false;
            propagateForward();
        } while (m_changed);
        
        return true;
    }
    
private:
    void propagateToFixpoint()
    {
        do {
            m_changed = false;
            
            // Forward propagation is near-optimal for both topologically-sorted and
            // DFS-sorted code.
            propagateForward();
            if (!m_changed)
                break;
            
            // Backward propagation reduces the likelihood that pathological code will
            // cause slowness. Loops (especially nested ones) resemble backward flow.
            // This pass captures two cases: (1) it detects if the forward fixpoint
            // found a sound solution and (2) short-circuits backward flow.
            m_changed = false;
            propagateBackward();
        } while (m_changed);
    }
    
    bool setPrediction(SpeculatedType prediction)
    {
        ASSERT(m_currentNode->hasResult());
        
        // setPrediction() is used when we know that there is no way that we can change
        // our minds about what the prediction is going to be. There is no semantic
        // difference between setPrediction() and mergeSpeculation() other than the
        // increased checking to validate this property.
        ASSERT(m_currentNode->prediction() == SpecNone || m_currentNode->prediction() == prediction);
        
        return m_currentNode->predict(prediction);
    }
    
    bool mergePrediction(SpeculatedType prediction)
    {
        ASSERT(m_currentNode->hasResult());
        
        return m_currentNode->predict(prediction);
    }
    
    SpeculatedType speculatedDoubleTypeForPrediction(SpeculatedType value)
    {
        SpeculatedType result = SpecDoubleReal;
        if (value & SpecDoubleImpureNaN)
            result |= SpecDoubleImpureNaN;
        if (value & SpecDoublePureNaN)
            result |= SpecDoublePureNaN;
        if (!isFullNumberOrBooleanSpeculation(value))
            result |= SpecDoublePureNaN;
        return result;
    }

    SpeculatedType speculatedDoubleTypeForPredictions(SpeculatedType left, SpeculatedType right)
    {
        return speculatedDoubleTypeForPrediction(mergeSpeculations(left, right));
    }

    void propagate(Node* node)
    {
        NodeType op = node->op();

        bool changed = false;
        
        switch (op) {
        case JSConstant: {
            SpeculatedType type = speculationFromValue(node->asJSValue());
            if (type == SpecInt52AsDouble && enableInt52())
                type = SpecInt52;
            changed |= setPrediction(type);
            break;
        }
            
        case GetLocal: {
            VariableAccessData* variable = node->variableAccessData();
            SpeculatedType prediction = variable->prediction();
            if (!variable->couldRepresentInt52() && (prediction & SpecInt52))
                prediction = (prediction | SpecInt52AsDouble) & ~SpecInt52;
            if (prediction)
                changed |= mergePrediction(prediction);
            break;
        }
            
        case SetLocal: {
            VariableAccessData* variableAccessData = node->variableAccessData();
            changed |= variableAccessData->predict(node->child1()->prediction());
            break;
        }
            
        case BitAnd:
        case BitOr:
        case BitXor:
        case BitRShift:
        case BitLShift:
        case BitURShift:
        case ArithIMul: {
            changed |= setPrediction(SpecInt32);
            break;
        }
            
        case ArrayPop:
        case ArrayPush:
        case RegExpExec:
        case RegExpTest:
        case GetById:
        case GetByIdFlush:
        case GetMyArgumentByValSafe:
        case GetByOffset:
        case MultiGetByOffset:
        case Call:
        case Construct:
        case NativeCall:
        case NativeConstruct:
        case GetGlobalVar:
        case GetClosureVar: {
            changed |= setPrediction(node->getHeapPrediction());
            break;
        }
            
        case GetGetterSetterByOffset: {
            changed |= setPrediction(SpecCellOther);
            break;
        }

        case GetGetter:
        case GetSetter:
        case GetCallee:
        case NewFunctionNoCheck:
        case NewFunctionExpression: {
            changed |= setPrediction(SpecFunction);
            break;
        }

        case StringCharCodeAt: {
            changed |= setPrediction(SpecInt32);
            break;
        }

        case UInt32ToNumber: {
            // FIXME: Support Int52.
            // https://bugs.webkit.org/show_bug.cgi?id=125704
            if (node->canSpeculateInt32(m_pass))
                changed |= mergePrediction(SpecInt32);
            else
                changed |= mergePrediction(SpecBytecodeNumber);
            break;
        }

        case ValueAdd: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (isFullNumberOrBooleanSpeculationExpectingDefined(left)
                    && isFullNumberOrBooleanSpeculationExpectingDefined(right)) {
                    if (m_graph.addSpeculationMode(node, m_pass) != DontSpeculateInt32)
                        changed |= mergePrediction(SpecInt32);
                    else if (m_graph.addShouldSpeculateMachineInt(node))
                        changed |= mergePrediction(SpecInt52);
                    else
                        changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
                } else if (
                    !(left & (SpecFullNumber | SpecBoolean))
                    || !(right & (SpecFullNumber | SpecBoolean))) {
                    // left or right is definitely something other than a number.
                    changed |= mergePrediction(SpecString);
                } else
                    changed |= mergePrediction(SpecString | SpecInt32 | SpecBytecodeDouble);
            }
            break;
        }
            
        case ArithAdd: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (m_graph.addSpeculationMode(node, m_pass) != DontSpeculateInt32)
                    changed |= mergePrediction(SpecInt32);
                else if (m_graph.addShouldSpeculateMachineInt(node))
                    changed |= mergePrediction(SpecInt52);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }
            
        case ArithSub: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (m_graph.addSpeculationMode(node, m_pass) != DontSpeculateInt32)
                    changed |= mergePrediction(SpecInt32);
                else if (m_graph.addShouldSpeculateMachineInt(node))
                    changed |= mergePrediction(SpecInt52);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }
            
        case ArithNegate:
            if (node->child1()->prediction()) {
                if (m_graph.negateShouldSpeculateInt32(node, m_pass))
                    changed |= mergePrediction(SpecInt32);
                else if (m_graph.negateShouldSpeculateMachineInt(node, m_pass))
                    changed |= mergePrediction(SpecInt52);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPrediction(node->child1()->prediction()));
            }
            break;
            
        case ArithMin:
        case ArithMax: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (Node::shouldSpeculateInt32OrBooleanForArithmetic(node->child1().node(), node->child2().node())
                    && node->canSpeculateInt32(m_pass))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }

        case ArithMul: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (m_graph.mulShouldSpeculateInt32(node, m_pass))
                    changed |= mergePrediction(SpecInt32);
                else if (m_graph.mulShouldSpeculateMachineInt(node, m_pass))
                    changed |= mergePrediction(SpecInt52);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }
            
        case ArithDiv: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (Node::shouldSpeculateInt32OrBooleanForArithmetic(node->child1().node(), node->child2().node())
                    && node->canSpeculateInt32(m_pass))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(SpecBytecodeDouble);
            }
            break;
        }
            
        case ArithMod: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (Node::shouldSpeculateInt32OrBooleanForArithmetic(node->child1().node(), node->child2().node())
                    && node->canSpeculateInt32(m_pass))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(SpecBytecodeDouble);
            }
            break;
        }
            
        case ArithSqrt:
        case ArithFRound:
        case ArithSin:
        case ArithCos: {
            changed |= setPrediction(SpecBytecodeDouble);
            break;
        }
            
        case ArithAbs: {
            SpeculatedType child = node->child1()->prediction();
            if (isInt32OrBooleanSpeculationForArithmetic(child)
                && node->canSpeculateInt32(m_pass))
                changed |= mergePrediction(SpecInt32);
            else
                changed |= mergePrediction(speculatedDoubleTypeForPrediction(child));
            break;
        }
            
        case LogicalNot:
        case CompareLess:
        case CompareLessEq:
        case CompareGreater:
        case CompareGreaterEq:
        case CompareEq:
        case CompareEqConstant:
        case CompareStrictEq:
        case InstanceOf:
        case IsUndefined:
        case IsBoolean:
        case IsNumber:
        case IsString:
        case IsObject:
        case IsFunction: {
            changed |= setPrediction(SpecBoolean);
            break;
        }

        case TypeOf: {
            changed |= setPrediction(SpecString);
            break;
        }

        case GetByVal: {
            if (!node->child1()->prediction())
                break;
            
            ArrayMode arrayMode = node->arrayMode().refine(
                m_graph, node,
                node->child1()->prediction(),
                node->child2()->prediction(),
                SpecNone, node->flags());
            
            switch (arrayMode.type()) {
            case Array::Double:
                if (arrayMode.isOutOfBounds())
                    changed |= mergePrediction(node->getHeapPrediction() | SpecDoubleReal);
                else
                    changed |= mergePrediction(SpecDoubleReal);
                break;
            case Array::Float32Array:
            case Array::Float64Array:
                changed |= mergePrediction(SpecFullDouble);
                break;
            case Array::Uint32Array:
                if (isInt32SpeculationForArithmetic(node->getHeapPrediction()))
                    changed |= mergePrediction(SpecInt32);
                else if (enableInt52())
                    changed |= mergePrediction(SpecMachineInt);
                else
                    changed |= mergePrediction(SpecInt32 | SpecInt52AsDouble);
                break;
            case Array::Int8Array:
            case Array::Uint8Array:
            case Array::Int16Array:
            case Array::Uint16Array:
            case Array::Int32Array:
                changed |= mergePrediction(SpecInt32);
                break;
            default:
                changed |= mergePrediction(node->getHeapPrediction());
                break;
            }
            break;
        }
            
        case GetMyArgumentsLengthSafe: {
            changed |= setPrediction(SpecInt32);
            break;
        }

        case GetClosureRegisters:            
        case GetButterfly: 
        case GetIndexedPropertyStorage:
        case AllocatePropertyStorage:
        case ReallocatePropertyStorage: {
            changed |= setPrediction(SpecOther);
            break;
        }

        case ToThis: {
            SpeculatedType prediction = node->child1()->prediction();
            if (prediction) {
                if (prediction & ~SpecObject) {
                    prediction &= SpecObject;
                    prediction = mergeSpeculations(prediction, SpecObjectOther);
                }
                changed |= mergePrediction(prediction);
            }
            break;
        }
            
        case GetMyScope:
        case SkipTopScope:
        case SkipScope: {
            changed |= setPrediction(SpecObjectOther);
            break;
        }
            
        case CreateThis:
        case NewObject: {
            changed |= setPrediction(SpecFinalObject);
            break;
        }
            
        case NewArray:
        case NewArrayWithSize:
        case NewArrayBuffer: {
            changed |= setPrediction(SpecArray);
            break;
        }
            
        case NewTypedArray: {
            changed |= setPrediction(speculationFromTypedArrayType(node->typedArrayType()));
            break;
        }
            
        case NewRegexp:
        case CreateActivation: {
            changed |= setPrediction(SpecObjectOther);
            break;
        }
        
        case StringFromCharCode: {
            changed |= setPrediction(SpecString);
            changed |= node->child1()->mergeFlags(NodeBytecodeUsesAsNumber | NodeBytecodeUsesAsInt);            
            break;
        }
        case StringCharAt:
        case ToString:
        case MakeRope: {
            changed |= setPrediction(SpecString);
            break;
        }
            
        case ToPrimitive: {
            SpeculatedType child = node->child1()->prediction();
            if (child)
                changed |= mergePrediction(resultOfToPrimitive(child));
            break;
        }
            
        case NewStringObject: {
            changed |= setPrediction(SpecStringObject);
            break;
        }
            
        case CreateArguments: {
            changed |= setPrediction(SpecArguments);
            break;
        }
            
        case NewFunction: {
            SpeculatedType child = node->child1()->prediction();
            if (child & SpecEmpty)
                changed |= mergePrediction((child & ~SpecEmpty) | SpecFunction);
            else
                changed |= mergePrediction(child);
            break;
        }
            
        case FiatInt52: {
            RELEASE_ASSERT(enableInt52());
            changed |= setPrediction(SpecMachineInt);
            break;
        }

        case PutByValAlias:
        case GetArrayLength:
        case GetTypedArrayByteOffset:
        case DoubleAsInt32:
        case GetLocalUnlinked:
        case GetMyArgumentsLength:
        case GetMyArgumentByVal:
        case PhantomArguments:
        case CheckArray:
        case Arrayify:
        case ArrayifyToStructure:
        case CheckTierUpInLoop:
        case CheckTierUpAtReturn:
        case CheckTierUpAndOSREnter:
        case InvalidationPoint:
        case CheckInBounds:
        case ValueToInt32:
        case HardPhantom:
        case DoubleRep:
        case ValueRep:
        case Int52Rep:
        case DoubleConstant:
        case Int52Constant:
        case Identity:
        case BooleanToNumber: {
            // This node should never be visible at this stage of compilation. It is
            // inserted by fixup(), which follows this phase.
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        
        case Phi:
            // Phis should not be visible here since we're iterating the all-but-Phi's
            // part of basic blocks.
            RELEASE_ASSERT_NOT_REACHED();
            break;
            
        case Upsilon:
        case GetArgument:
            // These don't get inserted until we go into SSA.
            RELEASE_ASSERT_NOT_REACHED();
            break;

        case GetScope:
            changed |= setPrediction(SpecObjectOther);
            break;
            
        case In:
            changed |= setPrediction(SpecBoolean);
            break;

#ifndef NDEBUG
        // These get ignored because they don't return anything.
        case StoreBarrier:
        case StoreBarrierWithNullCheck:
        case PutByValDirect:
        case PutByVal:
        case PutClosureVar:
        case Return:
        case Throw:
        case PutById:
        case PutByIdFlush:
        case PutByIdDirect:
        case PutByOffset:
        case MultiPutByOffset:
        case DFG::Jump:
        case Branch:
        case Switch:
        case Breakpoint:
        case ProfileWillCall:
        case ProfileDidCall:
        case CheckHasInstance:
        case ThrowReferenceError:
        case ForceOSRExit:
        case SetArgument:
        case CheckStructure:
        case CheckExecutable:
        case CheckFunction:
        case PutStructure:
        case TearOffActivation:
        case TearOffArguments:
        case CheckArgumentsNotCreated:
        case VariableWatchpoint:
        case VarInjectionWatchpoint:
        case AllocationProfileWatchpoint:
        case Phantom:
        case Check:
        case PutGlobalVar:
        case CheckWatchdogTimer:
        case Unreachable:
        case LoopHint:
        case NotifyWrite:
        case FunctionReentryWatchpoint:
        case TypedArrayWatchpoint:
        case ConstantStoragePointer:
        case MovHint:
        case ZombieHint:
            break;
            
        // This gets ignored because it already has a prediction.
        case ExtractOSREntryLocal:
            break;
            
        // These gets ignored because it doesn't do anything.
        case CountExecution:
        case PhantomLocal:
        case Flush:
            break;
            
        case LastNodeType:
            RELEASE_ASSERT_NOT_REACHED();
            break;
#else
        default:
            break;
#endif
        }

        m_changed |= changed;
    }
        
    void propagateForward()
    {
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            ASSERT(block->isReachable);
            for (unsigned i = 0; i < block->size(); ++i) {
                m_currentNode = block->at(i);
                propagate(m_currentNode);
            }
        }
    }
    
    void propagateBackward()
    {
        for (BlockIndex blockIndex = m_graph.numBlocks(); blockIndex--;) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            ASSERT(block->isReachable);
            for (unsigned i = block->size(); i--;) {
                m_currentNode = block->at(i);
                propagate(m_currentNode);
            }
        }
    }
    
    void doDoubleVoting(Node* node, float weight)
    {
        // Loop pre-headers created by OSR entrypoint creation may have NaN weight to indicate
        // that we actually don't know they weight. Assume that they execute once. This turns
        // out to be an OK assumption since the pre-header doesn't have any meaningful code.
        if (weight != weight)
            weight = 1;
        
        switch (node->op()) {
        case ValueAdd:
        case ArithAdd:
        case ArithSub: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
                
            DoubleBallot ballot;
                
            if (isFullNumberSpeculation(left)
                && isFullNumberSpeculation(right)
                && !m_graph.addShouldSpeculateInt32(node, m_pass)
                && !m_graph.addShouldSpeculateMachineInt(node))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot, weight);
            m_graph.voteNode(node->child2(), ballot, weight);
            break;
        }
                
        case ArithMul: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
                
            DoubleBallot ballot;
                
            if (isFullNumberSpeculation(left)
                && isFullNumberSpeculation(right)
                && !m_graph.mulShouldSpeculateInt32(node, m_pass)
                && !m_graph.mulShouldSpeculateMachineInt(node, m_pass))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot, weight);
            m_graph.voteNode(node->child2(), ballot, weight);
            break;
        }

        case ArithMin:
        case ArithMax:
        case ArithMod:
        case ArithDiv: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
                
            DoubleBallot ballot;
                
            if (isFullNumberSpeculation(left)
                && isFullNumberSpeculation(right)
                && !(Node::shouldSpeculateInt32OrBooleanForArithmetic(node->child1().node(), node->child2().node()) && node->canSpeculateInt32(m_pass)))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot, weight);
            m_graph.voteNode(node->child2(), ballot, weight);
            break;
        }
                
        case ArithAbs:
            DoubleBallot ballot;
            if (node->child1()->shouldSpeculateNumber()
                && !(node->child1()->shouldSpeculateInt32OrBooleanForArithmetic() && node->canSpeculateInt32(m_pass)))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot, weight);
            break;
                
        case ArithSqrt:
        case ArithCos:
        case ArithSin:
            if (node->child1()->shouldSpeculateNumber())
                m_graph.voteNode(node->child1(), VoteDouble, weight);
            else
                m_graph.voteNode(node->child1(), VoteValue, weight);
            break;
                
        case SetLocal: {
            SpeculatedType prediction = node->child1()->prediction();
            if (isDoubleSpeculation(prediction))
                node->variableAccessData()->vote(VoteDouble, weight);
            else if (
                !isFullNumberSpeculation(prediction)
                || isInt32Speculation(prediction) || isMachineIntSpeculation(prediction))
                node->variableAccessData()->vote(VoteValue, weight);
            break;
        }

        case PutByValDirect:
        case PutByVal:
        case PutByValAlias: {
            Edge child1 = m_graph.varArgChild(node, 0);
            Edge child2 = m_graph.varArgChild(node, 1);
            Edge child3 = m_graph.varArgChild(node, 2);
            m_graph.voteNode(child1, VoteValue, weight);
            m_graph.voteNode(child2, VoteValue, weight);
            switch (node->arrayMode().type()) {
            case Array::Double:
                m_graph.voteNode(child3, VoteDouble, weight);
                break;
            default:
                m_graph.voteNode(child3, VoteValue, weight);
                break;
            }
            break;
        }
            
        case MovHint:
            // Ignore these since they have no effect on in-DFG execution.
            break;
            
        default:
            m_graph.voteChildren(node, VoteValue, weight);
            break;
        }
    }
    
    void doRoundOfDoubleVoting()
    {
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i)
            m_graph.m_variableAccessData[i].find()->clearVotes();
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            ASSERT(block->isReachable);
            for (unsigned i = 0; i < block->size(); ++i) {
                m_currentNode = block->at(i);
                doDoubleVoting(m_currentNode, block->executionCount);
            }
        }
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i) {
            VariableAccessData* variableAccessData = &m_graph.m_variableAccessData[i];
            if (!variableAccessData->isRoot())
                continue;
            m_changed |= variableAccessData->tallyVotesForShouldUseDoubleFormat();
        }
        for (unsigned i = 0; i < m_graph.m_argumentPositions.size(); ++i)
            m_changed |= m_graph.m_argumentPositions[i].mergeArgumentPredictionAwareness();
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i) {
            VariableAccessData* variableAccessData = &m_graph.m_variableAccessData[i];
            if (!variableAccessData->isRoot())
                continue;
            m_changed |= variableAccessData->makePredictionForDoubleFormat();
        }
    }
    
    Node* m_currentNode;
    bool m_changed;
    PredictionPass m_pass; // We use different logic for considering predictions depending on how far along we are in propagation.
};
    
bool performPredictionPropagation(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Prediction Propagation Phase");
    return runPhase<PredictionPropagationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

