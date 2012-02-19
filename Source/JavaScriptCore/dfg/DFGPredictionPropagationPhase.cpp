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
#include "DFGPredictionPropagationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "DFGPhase.h"

namespace JSC { namespace DFG {

class PredictionPropagationPhase : public Phase {
public:
    PredictionPropagationPhase(Graph& graph)
        : Phase(graph, "prediction propagation")
    {
    }
    
    void run()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        m_count = 0;
#endif
        // Two stage process: first propagate predictions, then propagate while doing double voting.
        
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
        
        do {
            m_changed = false;
            doRoundOfDoubleVoting();
            propagateForward();
            if (!m_changed)
                break;
            
            m_changed = false;
            doRoundOfDoubleVoting();
            propagateBackward();
        } while (m_changed);
        
        fixup();
    }
    
private:
    bool setPrediction(PredictedType prediction)
    {
        ASSERT(m_graph[m_compileIndex].hasResult());
        
        // setPrediction() is used when we know that there is no way that we can change
        // our minds about what the prediction is going to be. There is no semantic
        // difference between setPrediction() and mergePrediction() other than the
        // increased checking to validate this property.
        ASSERT(m_graph[m_compileIndex].prediction() == PredictNone || m_graph[m_compileIndex].prediction() == prediction);
        
        return m_graph[m_compileIndex].predict(prediction);
    }
    
    bool mergePrediction(PredictedType prediction)
    {
        ASSERT(m_graph[m_compileIndex].hasResult());
        
        return m_graph[m_compileIndex].predict(prediction);
    }
    
    void propagate(Node& node)
    {
        if (!node.shouldGenerate())
            return;
        
        NodeType op = node.op;

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("   %s @%u: ", Graph::opName(op), m_compileIndex);
#endif
        
        bool changed = false;
        
        switch (op) {
        case JSConstant:
        case WeakJSConstant: {
            changed |= setPrediction(predictionFromValue(m_graph.valueOfJSConstant(m_compileIndex)));
            break;
        }
            
        case GetLocal: {
            PredictedType prediction = node.variableAccessData()->prediction();
            if (prediction)
                changed |= mergePrediction(prediction);
            break;
        }
            
        case SetLocal: {
            changed |= node.variableAccessData()->predict(m_graph[node.child1()].prediction());
            break;
        }
            
        case BitAnd:
        case BitOr:
        case BitXor:
        case BitRShift:
        case BitLShift:
        case BitURShift:
        case ValueToInt32: {
            changed |= setPrediction(PredictInt32);
            break;
        }
            
        case ArrayPop:
        case ArrayPush: {
            if (node.getHeapPrediction())
                changed |= mergePrediction(node.getHeapPrediction());
            break;
        }

        case StringCharCodeAt: {
            changed |= mergePrediction(PredictInt32);
            break;
        }

        case ArithMod: {
            PredictedType left = m_graph[node.child1()].prediction();
            PredictedType right = m_graph[node.child2()].prediction();
            
            if (left && right) {
                if (isInt32Prediction(mergePredictions(left, right)) && nodeCanSpeculateInteger(node.arithNodeFlags()))
                    changed |= mergePrediction(PredictInt32);
                else
                    changed |= mergePrediction(PredictDouble);
            }
            break;
        }
            
        case UInt32ToNumber: {
            if (nodeCanSpeculateInteger(node.arithNodeFlags()))
                changed |= setPrediction(PredictInt32);
            else
                changed |= setPrediction(PredictNumber);
            break;
        }

        case ValueAdd: {
            PredictedType left = m_graph[node.child1()].prediction();
            PredictedType right = m_graph[node.child2()].prediction();
            
            if (left && right) {
                if (isNumberPrediction(left) && isNumberPrediction(right)) {
                    if (m_graph.addShouldSpeculateInteger(node))
                        changed |= mergePrediction(PredictInt32);
                    else
                        changed |= mergePrediction(PredictDouble);
                } else if (!(left & PredictNumber) || !(right & PredictNumber)) {
                    // left or right is definitely something other than a number.
                    changed |= mergePrediction(PredictString);
                } else
                    changed |= mergePrediction(PredictString | PredictInt32 | PredictDouble);
            }
            break;
        }
            
        case ArithAdd:
        case ArithSub: {
            PredictedType left = m_graph[node.child1()].prediction();
            PredictedType right = m_graph[node.child2()].prediction();
            
            if (left && right) {
                if (m_graph.addShouldSpeculateInteger(node))
                    changed |= mergePrediction(PredictInt32);
                else
                    changed |= mergePrediction(PredictDouble);
            }
            break;
        }
            
        case ArithMul:
        case ArithMin:
        case ArithMax:
        case ArithDiv: {
            PredictedType left = m_graph[node.child1()].prediction();
            PredictedType right = m_graph[node.child2()].prediction();
            
            if (left && right) {
                if (isInt32Prediction(mergePredictions(left, right)) && nodeCanSpeculateInteger(node.arithNodeFlags()))
                    changed |= mergePrediction(PredictInt32);
                else
                    changed |= mergePrediction(PredictDouble);
            }
            break;
        }
            
        case ArithSqrt: {
            changed |= setPrediction(PredictDouble);
            break;
        }
            
        case ArithAbs: {
            PredictedType child = m_graph[node.child1()].prediction();
            if (child) {
                if (nodeCanSpeculateInteger(node.arithNodeFlags()))
                    changed |= mergePrediction(child);
                else
                    changed |= setPrediction(PredictDouble);
            }
            break;
        }
            
        case LogicalNot:
        case CompareLess:
        case CompareLessEq:
        case CompareGreater:
        case CompareGreaterEq:
        case CompareEq:
        case CompareStrictEq:
        case InstanceOf: {
            changed |= setPrediction(PredictBoolean);
            break;
        }
            
        case GetById: {
            if (node.getHeapPrediction())
                changed |= mergePrediction(node.getHeapPrediction());
            else if (codeBlock()->identifier(node.identifierNumber()) == globalData().propertyNames->length) {
                // If there is no prediction from value profiles, check if we might be
                // able to infer the type ourselves.
                bool isArray = isArrayPrediction(m_graph[node.child1()].prediction());
                bool isString = isStringPrediction(m_graph[node.child1()].prediction());
                bool isByteArray = m_graph[node.child1()].shouldSpeculateByteArray();
                bool isInt8Array = m_graph[node.child1()].shouldSpeculateInt8Array();
                bool isInt16Array = m_graph[node.child1()].shouldSpeculateInt16Array();
                bool isInt32Array = m_graph[node.child1()].shouldSpeculateInt32Array();
                bool isUint8Array = m_graph[node.child1()].shouldSpeculateUint8Array();
                bool isUint8ClampedArray = m_graph[node.child1()].shouldSpeculateUint8ClampedArray();
                bool isUint16Array = m_graph[node.child1()].shouldSpeculateUint16Array();
                bool isUint32Array = m_graph[node.child1()].shouldSpeculateUint32Array();
                bool isFloat32Array = m_graph[node.child1()].shouldSpeculateFloat32Array();
                bool isFloat64Array = m_graph[node.child1()].shouldSpeculateFloat64Array();
                if (isArray || isString || isByteArray || isInt8Array || isInt16Array || isInt32Array || isUint8Array || isUint8ClampedArray || isUint16Array || isUint32Array || isFloat32Array || isFloat64Array)
                    changed |= mergePrediction(PredictInt32);
            }
            break;
        }
            
        case GetByIdFlush:
            if (node.getHeapPrediction())
                changed |= mergePrediction(node.getHeapPrediction());
            break;
            
        case GetByVal: {
            if (m_graph[node.child1()].shouldSpeculateUint32Array() || m_graph[node.child1()].shouldSpeculateFloat32Array() || m_graph[node.child1()].shouldSpeculateFloat64Array())
                changed |= mergePrediction(PredictDouble);
            else if (node.getHeapPrediction())
                changed |= mergePrediction(node.getHeapPrediction());
            break;
        }
            
        case GetPropertyStorage: 
        case GetIndexedPropertyStorage: {
            changed |= setPrediction(PredictOther);
            break;
        }

        case GetByOffset: {
            if (node.getHeapPrediction())
                changed |= mergePrediction(node.getHeapPrediction());
            break;
        }
            
        case Call:
        case Construct: {
            if (node.getHeapPrediction())
                changed |= mergePrediction(node.getHeapPrediction());
            break;
        }
            
        case ConvertThis: {
            PredictedType prediction = m_graph[node.child1()].prediction();
            if (prediction) {
                if (prediction & ~PredictObjectMask) {
                    prediction &= PredictObjectMask;
                    prediction = mergePredictions(prediction, PredictObjectOther);
                }
                changed |= mergePrediction(prediction);
            }
            break;
        }
            
        case GetGlobalVar: {
            PredictedType prediction = m_graph.getGlobalVarPrediction(node.varNumber());
            if (prediction)
                changed |= mergePrediction(prediction);
            break;
        }
            
        case PutGlobalVar: {
            changed |= m_graph.predictGlobalVar(node.varNumber(), m_graph[node.child1()].prediction());
            break;
        }
            
        case GetScopedVar:
        case Resolve:
        case ResolveBase:
        case ResolveBaseStrictPut:
        case ResolveGlobal: {
            PredictedType prediction = node.getHeapPrediction();
            if (prediction)
                changed |= mergePrediction(prediction);
            break;
        }
            
        case GetScopeChain: {
            changed |= setPrediction(PredictCellOther);
            break;
        }
            
        case GetCallee: {
            changed |= setPrediction(PredictFunction);
            break;
        }
            
        case CreateThis:
        case NewObject: {
            changed |= setPrediction(PredictFinalObject);
            break;
        }
            
        case NewArray:
        case NewArrayBuffer: {
            changed |= setPrediction(PredictArray);
            break;
        }
            
        case NewRegexp: {
            changed |= setPrediction(PredictObjectOther);
            break;
        }
        
        case StringCharAt:
        case StrCat: {
            changed |= setPrediction(PredictString);
            break;
        }
            
        case ToPrimitive: {
            PredictedType child = m_graph[node.child1()].prediction();
            if (child) {
                if (isObjectPrediction(child)) {
                    // I'd love to fold this case into the case below, but I can't, because
                    // removing PredictObjectMask from something that only has an object
                    // prediction and nothing else means we have an ill-formed PredictedType
                    // (strong predict-none). This should be killed once we remove all traces
                    // of static (aka weak) predictions.
                    changed |= mergePrediction(PredictString);
                } else if (child & PredictObjectMask) {
                    // Objects get turned into strings. So if the input has hints of objectness,
                    // the output will have hinsts of stringiness.
                    changed |= mergePrediction(mergePredictions(child & ~PredictObjectMask, PredictString));
                } else
                    changed |= mergePrediction(child);
            }
            break;
        }
            
        case GetArrayLength:
        case GetByteArrayLength:
        case GetInt8ArrayLength:
        case GetInt16ArrayLength:
        case GetInt32ArrayLength:
        case GetUint8ArrayLength:
        case GetUint8ClampedArrayLength:
        case GetUint16ArrayLength:
        case GetUint32ArrayLength:
        case GetFloat32ArrayLength:
        case GetFloat64ArrayLength:
        case GetStringLength: {
            // This node should never be visible at this stage of compilation. It is
            // inserted by fixup(), which follows this phase.
            ASSERT_NOT_REACHED();
            break;
        }
        
#ifndef NDEBUG
        // These get ignored because they don't return anything.
        case PutScopedVar:
        case DFG::Jump:
        case Branch:
        case Breakpoint:
        case Return:
        case CheckHasInstance:
        case Phi:
        case Flush:
        case Throw:
        case ThrowReferenceError:
        case ForceOSRExit:
        case SetArgument:
        case PutByVal:
        case PutByValAlias:
        case PutById:
        case PutByIdDirect:
        case CheckStructure:
        case CheckFunction:
        case PutStructure:
        case PutByOffset:
            break;
            
        // These gets ignored because it doesn't do anything.
        case Phantom:
        case InlineStart:
        case Nop:
            break;
#else
        default:
            break;
#endif
        }

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("%s\n", predictionToString(m_graph[m_compileIndex].prediction()));
#endif
        
        m_changed |= changed;
    }
    
    void propagateForward()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("Propagating predictions forward [%u]\n", ++m_count);
#endif
        for (m_compileIndex = 0; m_compileIndex < m_graph.size(); ++m_compileIndex)
            propagate(m_graph[m_compileIndex]);
    }
    
    void propagateBackward()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("Propagating predictions backward [%u]\n", ++m_count);
#endif
        for (m_compileIndex = m_graph.size(); m_compileIndex-- > 0;)
            propagate(m_graph[m_compileIndex]);
    }

    void vote(NodeUse nodeUse, VariableAccessData::Ballot ballot)
    {
        switch (m_graph[nodeUse].op) {
        case ValueToInt32:
        case UInt32ToNumber:
            nodeUse = m_graph[nodeUse].child1();
            break;
        default:
            break;
        }
        
        if (m_graph[nodeUse].op == GetLocal)
            m_graph[nodeUse].variableAccessData()->vote(ballot);
    }
    
    void vote(Node& node, VariableAccessData::Ballot ballot)
    {
        if (node.op & NodeHasVarArgs) {
            for (unsigned childIdx = node.firstChild(); childIdx < node.firstChild() + node.numChildren(); childIdx++)
                vote(m_graph.m_varArgChildren[childIdx], ballot);
            return;
        }
        
        if (!node.child1())
            return;
        vote(node.child1(), ballot);
        if (!node.child2())
            return;
        vote(node.child2(), ballot);
        if (!node.child3())
            return;
        vote(node.child3(), ballot);
    }
    
    void doRoundOfDoubleVoting()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("Voting on double uses of locals [%u]\n", m_count);
#endif
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i)
            m_graph.m_variableAccessData[i].find()->clearVotes();
        for (m_compileIndex = 0; m_compileIndex < m_graph.size(); ++m_compileIndex) {
            Node& node = m_graph[m_compileIndex];
            switch (node.op) {
            case ValueAdd:
            case ArithAdd:
            case ArithSub: {
                PredictedType left = m_graph[node.child1()].prediction();
                PredictedType right = m_graph[node.child2()].prediction();
                
                VariableAccessData::Ballot ballot;
                
                if (isNumberPrediction(left) && isNumberPrediction(right)
                    && !m_graph.addShouldSpeculateInteger(node))
                    ballot = VariableAccessData::VoteDouble;
                else
                    ballot = VariableAccessData::VoteValue;
                
                vote(node.child1(), ballot);
                vote(node.child2(), ballot);
                break;
            }
                
            case ArithMul:
            case ArithMin:
            case ArithMax:
            case ArithMod:
            case ArithDiv: {
                PredictedType left = m_graph[node.child1()].prediction();
                PredictedType right = m_graph[node.child2()].prediction();
                
                VariableAccessData::Ballot ballot;
                
                if (isNumberPrediction(left) && isNumberPrediction(right) && !(Node::shouldSpeculateInteger(m_graph[node.child1()], m_graph[node.child1()]) && node.canSpeculateInteger()))
                    ballot = VariableAccessData::VoteDouble;
                else
                    ballot = VariableAccessData::VoteValue;
                
                vote(node.child1(), ballot);
                vote(node.child2(), ballot);
                break;
            }
                
            case ArithAbs:
                VariableAccessData::Ballot ballot;
                if (!(m_graph[node.child1()].shouldSpeculateInteger() && node.canSpeculateInteger()))
                    ballot = VariableAccessData::VoteDouble;
                else
                    ballot = VariableAccessData::VoteValue;
                
                vote(node.child1(), ballot);
                break;
                
            case ArithSqrt:
                vote(node.child1(), VariableAccessData::VoteDouble);
                break;
                
            case SetLocal: {
                PredictedType prediction = m_graph[node.child1()].prediction();
                if (isDoublePrediction(prediction))
                    node.variableAccessData()->vote(VariableAccessData::VoteDouble);
                else if (!isNumberPrediction(prediction) || isInt32Prediction(prediction))
                    node.variableAccessData()->vote(VariableAccessData::VoteValue);
                break;
            }
                
            default:
                vote(node, VariableAccessData::VoteValue);
                break;
            }
        }
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i)
            m_changed |= m_graph.m_variableAccessData[i].find()->tallyVotesForShouldUseDoubleFormat();
    }
    
    void fixupNode(Node& node)
    {
        if (!node.shouldGenerate())
            return;
        
        NodeType op = node.op;

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("   %s @%u: ", Graph::opName(op), m_compileIndex);
#endif
        
        switch (op) {
        case GetById: {
            if (!isInt32Prediction(m_graph[m_compileIndex].prediction()))
                break;
            if (codeBlock()->identifier(node.identifierNumber()) != globalData().propertyNames->length)
                break;
            bool isArray = isArrayPrediction(m_graph[node.child1()].prediction());
            bool isString = isStringPrediction(m_graph[node.child1()].prediction());
            bool isByteArray = m_graph[node.child1()].shouldSpeculateByteArray();
            bool isInt8Array = m_graph[node.child1()].shouldSpeculateInt8Array();
            bool isInt16Array = m_graph[node.child1()].shouldSpeculateInt16Array();
            bool isInt32Array = m_graph[node.child1()].shouldSpeculateInt32Array();
            bool isUint8Array = m_graph[node.child1()].shouldSpeculateUint8Array();
            bool isUint8ClampedArray = m_graph[node.child1()].shouldSpeculateUint8ClampedArray();
            bool isUint16Array = m_graph[node.child1()].shouldSpeculateUint16Array();
            bool isUint32Array = m_graph[node.child1()].shouldSpeculateUint32Array();
            bool isFloat32Array = m_graph[node.child1()].shouldSpeculateFloat32Array();
            bool isFloat64Array = m_graph[node.child1()].shouldSpeculateFloat64Array();
            if (!isArray && !isString && !isByteArray && !isInt8Array && !isInt16Array && !isInt32Array && !isUint8Array && !isUint8ClampedArray && !isUint16Array && !isUint32Array && !isFloat32Array && !isFloat64Array)
                break;
            
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("  @%u -> %s", m_compileIndex, isArray ? "GetArrayLength" : "GetStringLength");
#endif
            if (isArray)
                node.op = GetArrayLength;
            else if (isString)
                node.op = GetStringLength;
            else if (isByteArray)
                node.op = GetByteArrayLength;
            else if (isInt8Array)
                node.op = GetInt8ArrayLength;
            else if (isInt16Array)
                node.op = GetInt16ArrayLength;
            else if (isInt32Array)
                node.op = GetInt32ArrayLength;
            else if (isUint8Array)
                node.op = GetUint8ArrayLength;
            else if (isUint8ClampedArray)
                node.op = GetUint8ClampedArrayLength;
            else if (isUint16Array)
                node.op = GetUint16ArrayLength;
            else if (isUint32Array)
                node.op = GetUint32ArrayLength;
            else if (isFloat32Array)
                node.op = GetFloat32ArrayLength;
            else if (isFloat64Array)
                node.op = GetFloat64ArrayLength;
            else
                ASSERT_NOT_REACHED();
            m_graph.deref(m_compileIndex); // No longer MustGenerate
            break;
        }
        case GetIndexedPropertyStorage: {
            PredictedType basePrediction = m_graph[node.child2()].prediction();
            if (!(basePrediction & PredictInt32) && basePrediction) {
                node.op = Nop;
                m_graph.clearAndDerefChild1(node);
                m_graph.clearAndDerefChild2(node);
                m_graph.clearAndDerefChild3(node);
                node.setRefCount(0);
            }
            break;
        }
        case GetByVal:
        case StringCharAt:
        case StringCharCodeAt: {
            if (!!node.child3() && m_graph[node.child3()].op == Nop)
                node.children.child3() = NodeUse();
            break;
        }
        default:
            break;
        }

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("\n");
#endif
    }
    
    void fixup()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("Performing Fixup\n");
#endif
        for (m_compileIndex = 0; m_compileIndex < m_graph.size(); ++m_compileIndex)
            fixupNode(m_graph[m_compileIndex]);
    }
    
    NodeIndex m_compileIndex;
    bool m_changed;

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
    unsigned m_count;
#endif
};
    
void performPredictionPropagation(Graph& graph)
{
    runPhase<PredictionPropagationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

