/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "DFGByteCodeParser.h"

#if ENABLE(DFG_JIT)

#include "ArrayConstructor.h"
#include "CallLinkStatus.h"
#include "CodeBlock.h"
#include "DFGArrayMode.h"
#include "DFGByteCodeCache.h"
#include "DFGCapabilities.h"
#include "GetByIdStatus.h"
#include "MethodCallLinkStatus.h"
#include "PutByIdStatus.h"
#include "ResolveGlobalStatus.h"
#include <wtf/HashMap.h>
#include <wtf/MathExtras.h>

namespace JSC { namespace DFG {

// === ByteCodeParser ===
//
// This class is used to compile the dataflow graph from a CodeBlock.
class ByteCodeParser {
public:
    ByteCodeParser(ExecState* exec, Graph& graph)
        : m_exec(exec)
        , m_globalData(&graph.m_globalData)
        , m_codeBlock(graph.m_codeBlock)
        , m_profiledBlock(graph.m_profiledBlock)
        , m_graph(graph)
        , m_currentBlock(0)
        , m_currentIndex(0)
        , m_currentProfilingIndex(0)
        , m_constantUndefined(UINT_MAX)
        , m_constantNull(UINT_MAX)
        , m_constantNaN(UINT_MAX)
        , m_constant1(UINT_MAX)
        , m_constants(m_codeBlock->numberOfConstantRegisters())
        , m_numArguments(m_codeBlock->numParameters())
        , m_numLocals(m_codeBlock->m_numCalleeRegisters)
        , m_preservedVars(m_codeBlock->m_numVars)
        , m_parameterSlots(0)
        , m_numPassedVarArgs(0)
        , m_globalResolveNumber(0)
        , m_inlineStackTop(0)
        , m_haveBuiltOperandMaps(false)
        , m_emptyJSValueIndex(UINT_MAX)
        , m_currentInstruction(0)
    {
        ASSERT(m_profiledBlock);
        
        for (int i = 0; i < m_codeBlock->m_numVars; ++i)
            m_preservedVars.set(i);
    }
    
    // Parse a full CodeBlock of bytecode.
    bool parse();
    
private:
    // Just parse from m_currentIndex to the end of the current CodeBlock.
    void parseCodeBlock();

    // Helper for min and max.
    bool handleMinMax(bool usesResult, int resultOperand, NodeType op, int registerOffset, int argumentCountIncludingThis);
    
    // Handle calls. This resolves issues surrounding inlining and intrinsics.
    void handleCall(Interpreter*, Instruction* currentInstruction, NodeType op, CodeSpecializationKind);
    void emitFunctionCheck(JSFunction* expectedFunction, NodeIndex callTarget, int registerOffset, CodeSpecializationKind);
    // Handle inlining. Return true if it succeeded, false if we need to plant a call.
    bool handleInlining(bool usesResult, int callTarget, NodeIndex callTargetNodeIndex, int resultOperand, bool certainAboutExpectedFunction, JSFunction*, int registerOffset, int argumentCountIncludingThis, unsigned nextOffset, CodeSpecializationKind);
    // Handle setting the result of an intrinsic.
    void setIntrinsicResult(bool usesResult, int resultOperand, NodeIndex);
    // Handle intrinsic functions. Return true if it succeeded, false if we need to plant a call.
    bool handleIntrinsic(bool usesResult, int resultOperand, Intrinsic, int registerOffset, int argumentCountIncludingThis, SpeculatedType prediction);
    bool handleConstantInternalFunction(bool usesResult, int resultOperand, InternalFunction*, int registerOffset, int argumentCountIncludingThis, SpeculatedType prediction, CodeSpecializationKind);
    void handleGetByOffset(
        int destinationOperand, SpeculatedType, NodeIndex base, unsigned identifierNumber,
        PropertyOffset);
    void handleGetById(
        int destinationOperand, SpeculatedType, NodeIndex base, unsigned identifierNumber,
        const GetByIdStatus&);
    // Prepare to parse a block.
    void prepareToParseBlock();
    // Parse a single basic block of bytecode instructions.
    bool parseBlock(unsigned limit);
    // Link block successors.
    void linkBlock(BasicBlock*, Vector<BlockIndex>& possibleTargets);
    void linkBlocks(Vector<UnlinkedBlock>& unlinkedBlocks, Vector<BlockIndex>& possibleTargets);
    // Link GetLocal & SetLocal nodes, to ensure live values are generated.
    enum PhiStackType {
        LocalPhiStack,
        ArgumentPhiStack
    };
    template<PhiStackType stackType>
    void processPhiStack();
    
    void fixVariableAccessPredictions();
    // Add spill locations to nodes.
    void allocateVirtualRegisters();
    
    VariableAccessData* newVariableAccessData(int operand, bool isCaptured)
    {
        ASSERT(operand < FirstConstantRegisterIndex);
        
        m_graph.m_variableAccessData.append(VariableAccessData(static_cast<VirtualRegister>(operand), isCaptured));
        return &m_graph.m_variableAccessData.last();
    }
    
    // Get/Set the operands/result of a bytecode instruction.
    NodeIndex getDirect(int operand)
    {
        // Is this a constant?
        if (operand >= FirstConstantRegisterIndex) {
            unsigned constant = operand - FirstConstantRegisterIndex;
            ASSERT(constant < m_constants.size());
            return getJSConstant(constant);
        }

        if (operand == RegisterFile::Callee)
            return getCallee();

        // Is this an argument?
        if (operandIsArgument(operand))
            return getArgument(operand);

        // Must be a local.
        return getLocal((unsigned)operand);
    }
    NodeIndex get(int operand)
    {
        return getDirect(m_inlineStackTop->remapOperand(operand));
    }
    enum SetMode { NormalSet, SetOnEntry };
    void setDirect(int operand, NodeIndex value, SetMode setMode = NormalSet)
    {
        // Is this an argument?
        if (operandIsArgument(operand)) {
            setArgument(operand, value, setMode);
            return;
        }

        // Must be a local.
        setLocal((unsigned)operand, value, setMode);
    }
    void set(int operand, NodeIndex value, SetMode setMode = NormalSet)
    {
        setDirect(m_inlineStackTop->remapOperand(operand), value, setMode);
    }
    
    NodeIndex injectLazyOperandSpeculation(NodeIndex nodeIndex)
    {
        Node& node = m_graph[nodeIndex];
        ASSERT(node.op() == GetLocal);
        ASSERT(node.codeOrigin.bytecodeIndex == m_currentIndex);
        SpeculatedType prediction = 
            m_inlineStackTop->m_lazyOperands.prediction(
                LazyOperandValueProfileKey(m_currentIndex, node.local()));
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Lazy operand [@%u, bc#%u, r%d] prediction: %s\n",
                nodeIndex, m_currentIndex, node.local(), speculationToString(prediction));
#endif
        node.variableAccessData()->predict(prediction);
        return nodeIndex;
    }

    // Used in implementing get/set, above, where the operand is a local variable.
    NodeIndex getLocal(unsigned operand)
    {
        NodeIndex nodeIndex = m_currentBlock->variablesAtTail.local(operand);
        bool isCaptured = m_codeBlock->isCaptured(operand, m_inlineStackTop->m_inlineCallFrame);
        
        if (nodeIndex != NoNode) {
            Node* nodePtr = &m_graph[nodeIndex];
            if (nodePtr->op() == Flush) {
                // Two possibilities: either the block wants the local to be live
                // but has not loaded its value, or it has loaded its value, in
                // which case we're done.
                nodeIndex = nodePtr->child1().index();
                Node& flushChild = m_graph[nodeIndex];
                if (flushChild.op() == Phi) {
                    VariableAccessData* variableAccessData = flushChild.variableAccessData();
                    variableAccessData->mergeIsCaptured(isCaptured);
                    nodeIndex = injectLazyOperandSpeculation(addToGraph(GetLocal, OpInfo(variableAccessData), nodeIndex));
                    m_currentBlock->variablesAtTail.local(operand) = nodeIndex;
                    return nodeIndex;
                }
                nodePtr = &flushChild;
            }
            
            ASSERT(&m_graph[nodeIndex] == nodePtr);
            ASSERT(nodePtr->op() != Flush);

            nodePtr->variableAccessData()->mergeIsCaptured(isCaptured);
                
            if (isCaptured) {
                // We wish to use the same variable access data as the previous access,
                // but for all other purposes we want to issue a load since for all we
                // know, at this stage of compilation, the local has been clobbered.
                
                // Make sure we link to the Phi node, not to the GetLocal.
                if (nodePtr->op() == GetLocal)
                    nodeIndex = nodePtr->child1().index();
                
                return injectLazyOperandSpeculation(addToGraph(GetLocal, OpInfo(nodePtr->variableAccessData()), nodeIndex));
            }
            
            if (nodePtr->op() == GetLocal)
                return nodeIndex;
            ASSERT(nodePtr->op() == SetLocal);
            return nodePtr->child1().index();
        }

        // Check for reads of temporaries from prior blocks,
        // expand m_preservedVars to cover these.
        m_preservedVars.set(operand);
        
        VariableAccessData* variableAccessData = newVariableAccessData(operand, isCaptured);
        
        NodeIndex phi = addToGraph(Phi, OpInfo(variableAccessData));
        m_localPhiStack.append(PhiStackEntry(m_currentBlock, phi, operand));
        nodeIndex = injectLazyOperandSpeculation(addToGraph(GetLocal, OpInfo(variableAccessData), phi));
        m_currentBlock->variablesAtTail.local(operand) = nodeIndex;
        
        m_currentBlock->variablesAtHead.setLocalFirstTime(operand, nodeIndex);
        
        return nodeIndex;
    }
    void setLocal(unsigned operand, NodeIndex value, SetMode setMode = NormalSet)
    {
        bool isCaptured = m_codeBlock->isCaptured(operand, m_inlineStackTop->m_inlineCallFrame);
        
        if (setMode == NormalSet) {
            ArgumentPosition* argumentPosition = findArgumentPositionForLocal(operand);
            if (isCaptured || argumentPosition)
                flushDirect(operand, argumentPosition);
        }

        VariableAccessData* variableAccessData = newVariableAccessData(operand, isCaptured);
        variableAccessData->mergeStructureCheckHoistingFailed(
            m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, BadCache));
        NodeIndex nodeIndex = addToGraph(SetLocal, OpInfo(variableAccessData), value);
        m_currentBlock->variablesAtTail.local(operand) = nodeIndex;
    }

    // Used in implementing get/set, above, where the operand is an argument.
    NodeIndex getArgument(unsigned operand)
    {
        unsigned argument = operandToArgument(operand);
        ASSERT(argument < m_numArguments);
        
        NodeIndex nodeIndex = m_currentBlock->variablesAtTail.argument(argument);
        bool isCaptured = m_codeBlock->isCaptured(operand);

        if (nodeIndex != NoNode) {
            Node* nodePtr = &m_graph[nodeIndex];
            if (nodePtr->op() == Flush) {
                // Two possibilities: either the block wants the local to be live
                // but has not loaded its value, or it has loaded its value, in
                // which case we're done.
                nodeIndex = nodePtr->child1().index();
                Node& flushChild = m_graph[nodeIndex];
                if (flushChild.op() == Phi) {
                    VariableAccessData* variableAccessData = flushChild.variableAccessData();
                    variableAccessData->mergeIsCaptured(isCaptured);
                    nodeIndex = injectLazyOperandSpeculation(addToGraph(GetLocal, OpInfo(variableAccessData), nodeIndex));
                    m_currentBlock->variablesAtTail.argument(argument) = nodeIndex;
                    return nodeIndex;
                }
                nodePtr = &flushChild;
            }
            
            ASSERT(&m_graph[nodeIndex] == nodePtr);
            ASSERT(nodePtr->op() != Flush);
            
            nodePtr->variableAccessData()->mergeIsCaptured(isCaptured);
            
            if (nodePtr->op() == SetArgument) {
                // We're getting an argument in the first basic block; link
                // the GetLocal to the SetArgument.
                ASSERT(nodePtr->local() == static_cast<VirtualRegister>(operand));
                VariableAccessData* variable = nodePtr->variableAccessData();
                nodeIndex = injectLazyOperandSpeculation(addToGraph(GetLocal, OpInfo(variable), nodeIndex));
                m_currentBlock->variablesAtTail.argument(argument) = nodeIndex;
                return nodeIndex;
            }
            
            if (isCaptured) {
                if (nodePtr->op() == GetLocal)
                    nodeIndex = nodePtr->child1().index();
                return injectLazyOperandSpeculation(addToGraph(GetLocal, OpInfo(nodePtr->variableAccessData()), nodeIndex));
            }
            
            if (nodePtr->op() == GetLocal)
                return nodeIndex;
            
            ASSERT(nodePtr->op() == SetLocal);
            return nodePtr->child1().index();
        }
        
        VariableAccessData* variableAccessData = newVariableAccessData(operand, isCaptured);

        NodeIndex phi = addToGraph(Phi, OpInfo(variableAccessData));
        m_argumentPhiStack.append(PhiStackEntry(m_currentBlock, phi, argument));
        nodeIndex = injectLazyOperandSpeculation(addToGraph(GetLocal, OpInfo(variableAccessData), phi));
        m_currentBlock->variablesAtTail.argument(argument) = nodeIndex;
        
        m_currentBlock->variablesAtHead.setArgumentFirstTime(argument, nodeIndex);
        
        return nodeIndex;
    }
    void setArgument(int operand, NodeIndex value, SetMode setMode = NormalSet)
    {
        unsigned argument = operandToArgument(operand);
        ASSERT(argument < m_numArguments);
        
        bool isCaptured = m_codeBlock->isCaptured(operand);

        // Always flush arguments, except for 'this'.
        if (argument && setMode == NormalSet)
            flushDirect(operand);
        
        VariableAccessData* variableAccessData = newVariableAccessData(operand, isCaptured);
        variableAccessData->mergeStructureCheckHoistingFailed(
            m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, BadCache));
        NodeIndex nodeIndex = addToGraph(SetLocal, OpInfo(variableAccessData), value);
        m_currentBlock->variablesAtTail.argument(argument) = nodeIndex;
    }
    
    ArgumentPosition* findArgumentPositionForArgument(int argument)
    {
        InlineStackEntry* stack = m_inlineStackTop;
        while (stack->m_inlineCallFrame)
            stack = stack->m_caller;
        return stack->m_argumentPositions[argument];
    }
    
    ArgumentPosition* findArgumentPositionForLocal(int operand)
    {
        for (InlineStackEntry* stack = m_inlineStackTop; ; stack = stack->m_caller) {
            InlineCallFrame* inlineCallFrame = stack->m_inlineCallFrame;
            if (!inlineCallFrame)
                break;
            if (operand >= static_cast<int>(inlineCallFrame->stackOffset - RegisterFile::CallFrameHeaderSize))
                continue;
            if (operand == inlineCallFrame->stackOffset + CallFrame::thisArgumentOffset())
                continue;
            if (operand < static_cast<int>(inlineCallFrame->stackOffset - RegisterFile::CallFrameHeaderSize - inlineCallFrame->arguments.size()))
                continue;
            int argument = operandToArgument(operand - inlineCallFrame->stackOffset);
            return stack->m_argumentPositions[argument];
        }
        return 0;
    }
    
    ArgumentPosition* findArgumentPosition(int operand)
    {
        if (operandIsArgument(operand))
            return findArgumentPositionForArgument(operandToArgument(operand));
        return findArgumentPositionForLocal(operand);
    }
    
    void flush(int operand)
    {
        flushDirect(m_inlineStackTop->remapOperand(operand));
    }
    
    void flushDirect(int operand)
    {
        flushDirect(operand, findArgumentPosition(operand));
    }
    
    void flushDirect(int operand, ArgumentPosition* argumentPosition)
    {
        // FIXME: This should check if the same operand had already been flushed to
        // some other local variable.
        
        bool isCaptured = m_codeBlock->isCaptured(operand, m_inlineStackTop->m_inlineCallFrame);
        
        ASSERT(operand < FirstConstantRegisterIndex);
        
        NodeIndex nodeIndex;
        int index;
        if (operandIsArgument(operand)) {
            index = operandToArgument(operand);
            nodeIndex = m_currentBlock->variablesAtTail.argument(index);
        } else {
            index = operand;
            nodeIndex = m_currentBlock->variablesAtTail.local(index);
            m_preservedVars.set(operand);
        }
        
        if (nodeIndex != NoNode) {
            Node& node = m_graph[nodeIndex];
            switch (node.op()) {
            case Flush:
                nodeIndex = node.child1().index();
                break;
            case GetLocal:
                nodeIndex = node.child1().index();
                break;
            default:
                break;
            }
            
            ASSERT(m_graph[nodeIndex].op() != Flush
                   && m_graph[nodeIndex].op() != GetLocal);
            
            // Emit a Flush regardless of whether we already flushed it.
            // This gives us guidance to see that the variable also needs to be flushed
            // for arguments, even if it already had to be flushed for other reasons.
            VariableAccessData* variableAccessData = node.variableAccessData();
            variableAccessData->mergeIsCaptured(isCaptured);
            addToGraph(Flush, OpInfo(variableAccessData), nodeIndex);
            if (argumentPosition)
                argumentPosition->addVariable(variableAccessData);
            return;
        }
        
        VariableAccessData* variableAccessData = newVariableAccessData(operand, isCaptured);
        NodeIndex phi = addToGraph(Phi, OpInfo(variableAccessData));
        nodeIndex = addToGraph(Flush, OpInfo(variableAccessData), phi);
        if (operandIsArgument(operand)) {
            m_argumentPhiStack.append(PhiStackEntry(m_currentBlock, phi, index));
            m_currentBlock->variablesAtTail.argument(index) = nodeIndex;
            m_currentBlock->variablesAtHead.setArgumentFirstTime(index, nodeIndex);
        } else {
            m_localPhiStack.append(PhiStackEntry(m_currentBlock, phi, index));
            m_currentBlock->variablesAtTail.local(index) = nodeIndex;
            m_currentBlock->variablesAtHead.setLocalFirstTime(index, nodeIndex);
        }
        if (argumentPosition)
            argumentPosition->addVariable(variableAccessData);
    }
    
    void flushArgumentsAndCapturedVariables()
    {
        int numArguments;
        if (m_inlineStackTop->m_inlineCallFrame)
            numArguments = m_inlineStackTop->m_inlineCallFrame->arguments.size();
        else
            numArguments = m_inlineStackTop->m_codeBlock->numParameters();
        for (unsigned argument = numArguments; argument-- > 1;)
            flush(argumentToOperand(argument));
        for (unsigned local = m_inlineStackTop->m_codeBlock->m_numCapturedVars; local--;)
            flush(local);
    }

    // Get an operand, and perform a ToInt32/ToNumber conversion on it.
    NodeIndex getToInt32(int operand)
    {
        return toInt32(get(operand));
    }

    // Perform an ES5 ToInt32 operation - returns a node of type NodeResultInt32.
    NodeIndex toInt32(NodeIndex index)
    {
        Node& node = m_graph[index];

        if (node.hasInt32Result())
            return index;

        if (node.op() == UInt32ToNumber)
            return node.child1().index();

        // Check for numeric constants boxed as JSValues.
        if (node.op() == JSConstant) {
            JSValue v = valueOfJSConstant(index);
            if (v.isInt32())
                return getJSConstant(node.constantNumber());
            if (v.isNumber())
                return getJSConstantForValue(JSValue(JSC::toInt32(v.asNumber())));
        }

        return addToGraph(ValueToInt32, index);
    }

    NodeIndex getJSConstantForValue(JSValue constantValue)
    {
        unsigned constantIndex = m_codeBlock->addOrFindConstant(constantValue);
        if (constantIndex >= m_constants.size())
            m_constants.append(ConstantRecord());
        
        ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        
        return getJSConstant(constantIndex);
    }

    NodeIndex getJSConstant(unsigned constant)
    {
        NodeIndex index = m_constants[constant].asJSValue;
        if (index != NoNode)
            return index;

        NodeIndex resultIndex = addToGraph(JSConstant, OpInfo(constant));
        m_constants[constant].asJSValue = resultIndex;
        return resultIndex;
    }

    NodeIndex getCallee()
    {
        return addToGraph(GetCallee);
    }

    // Helper functions to get/set the this value.
    NodeIndex getThis()
    {
        return get(m_inlineStackTop->m_codeBlock->thisRegister());
    }
    void setThis(NodeIndex value)
    {
        set(m_inlineStackTop->m_codeBlock->thisRegister(), value);
    }

    // Convenience methods for checking nodes for constants.
    bool isJSConstant(NodeIndex index)
    {
        return m_graph[index].op() == JSConstant;
    }
    bool isInt32Constant(NodeIndex nodeIndex)
    {
        return isJSConstant(nodeIndex) && valueOfJSConstant(nodeIndex).isInt32();
    }
    // Convenience methods for getting constant values.
    JSValue valueOfJSConstant(NodeIndex index)
    {
        ASSERT(isJSConstant(index));
        return m_codeBlock->getConstant(FirstConstantRegisterIndex + m_graph[index].constantNumber());
    }
    int32_t valueOfInt32Constant(NodeIndex nodeIndex)
    {
        ASSERT(isInt32Constant(nodeIndex));
        return valueOfJSConstant(nodeIndex).asInt32();
    }
    
    // This method returns a JSConstant with the value 'undefined'.
    NodeIndex constantUndefined()
    {
        // Has m_constantUndefined been set up yet?
        if (m_constantUndefined == UINT_MAX) {
            // Search the constant pool for undefined, if we find it, we can just reuse this!
            unsigned numberOfConstants = m_codeBlock->numberOfConstantRegisters();
            for (m_constantUndefined = 0; m_constantUndefined < numberOfConstants; ++m_constantUndefined) {
                JSValue testMe = m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantUndefined);
                if (testMe.isUndefined())
                    return getJSConstant(m_constantUndefined);
            }

            // Add undefined to the CodeBlock's constants, and add a corresponding slot in m_constants.
            ASSERT(m_constants.size() == numberOfConstants);
            m_codeBlock->addConstant(jsUndefined());
            m_constants.append(ConstantRecord());
            ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        }

        // m_constantUndefined must refer to an entry in the CodeBlock's constant pool that has the value 'undefined'.
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantUndefined).isUndefined());
        return getJSConstant(m_constantUndefined);
    }

    // This method returns a JSConstant with the value 'null'.
    NodeIndex constantNull()
    {
        // Has m_constantNull been set up yet?
        if (m_constantNull == UINT_MAX) {
            // Search the constant pool for null, if we find it, we can just reuse this!
            unsigned numberOfConstants = m_codeBlock->numberOfConstantRegisters();
            for (m_constantNull = 0; m_constantNull < numberOfConstants; ++m_constantNull) {
                JSValue testMe = m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantNull);
                if (testMe.isNull())
                    return getJSConstant(m_constantNull);
            }

            // Add null to the CodeBlock's constants, and add a corresponding slot in m_constants.
            ASSERT(m_constants.size() == numberOfConstants);
            m_codeBlock->addConstant(jsNull());
            m_constants.append(ConstantRecord());
            ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        }

        // m_constantNull must refer to an entry in the CodeBlock's constant pool that has the value 'null'.
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantNull).isNull());
        return getJSConstant(m_constantNull);
    }

    // This method returns a DoubleConstant with the value 1.
    NodeIndex one()
    {
        // Has m_constant1 been set up yet?
        if (m_constant1 == UINT_MAX) {
            // Search the constant pool for the value 1, if we find it, we can just reuse this!
            unsigned numberOfConstants = m_codeBlock->numberOfConstantRegisters();
            for (m_constant1 = 0; m_constant1 < numberOfConstants; ++m_constant1) {
                JSValue testMe = m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constant1);
                if (testMe.isInt32() && testMe.asInt32() == 1)
                    return getJSConstant(m_constant1);
            }

            // Add the value 1 to the CodeBlock's constants, and add a corresponding slot in m_constants.
            ASSERT(m_constants.size() == numberOfConstants);
            m_codeBlock->addConstant(jsNumber(1));
            m_constants.append(ConstantRecord());
            ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        }

        // m_constant1 must refer to an entry in the CodeBlock's constant pool that has the integer value 1.
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constant1).isInt32());
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constant1).asInt32() == 1);
        return getJSConstant(m_constant1);
    }
    
    // This method returns a DoubleConstant with the value NaN.
    NodeIndex constantNaN()
    {
        JSValue nan = jsNaN();
        
        // Has m_constantNaN been set up yet?
        if (m_constantNaN == UINT_MAX) {
            // Search the constant pool for the value NaN, if we find it, we can just reuse this!
            unsigned numberOfConstants = m_codeBlock->numberOfConstantRegisters();
            for (m_constantNaN = 0; m_constantNaN < numberOfConstants; ++m_constantNaN) {
                JSValue testMe = m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantNaN);
                if (JSValue::encode(testMe) == JSValue::encode(nan))
                    return getJSConstant(m_constantNaN);
            }

            // Add the value nan to the CodeBlock's constants, and add a corresponding slot in m_constants.
            ASSERT(m_constants.size() == numberOfConstants);
            m_codeBlock->addConstant(nan);
            m_constants.append(ConstantRecord());
            ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        }

        // m_constantNaN must refer to an entry in the CodeBlock's constant pool that has the value nan.
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantNaN).isDouble());
        ASSERT(isnan(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantNaN).asDouble()));
        return getJSConstant(m_constantNaN);
    }
    
    NodeIndex cellConstant(JSCell* cell)
    {
        HashMap<JSCell*, NodeIndex>::AddResult result = m_cellConstantNodes.add(cell, NoNode);
        if (result.isNewEntry)
            result.iterator->second = addToGraph(WeakJSConstant, OpInfo(cell));
        
        return result.iterator->second;
    }
    
    CodeOrigin currentCodeOrigin()
    {
        return CodeOrigin(m_currentIndex, m_inlineStackTop->m_inlineCallFrame, m_currentProfilingIndex - m_currentIndex);
    }

    // These methods create a node and add it to the graph. If nodes of this type are
    // 'mustGenerate' then the node  will implicitly be ref'ed to ensure generation.
    NodeIndex addToGraph(NodeType op, NodeIndex child1 = NoNode, NodeIndex child2 = NoNode, NodeIndex child3 = NoNode)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(op, currentCodeOrigin(), child1, child2, child3));
        ASSERT(op != Phi);
        m_currentBlock->append(resultIndex);

        if (defaultFlags(op) & NodeMustGenerate)
            m_graph.ref(resultIndex);
        return resultIndex;
    }
    NodeIndex addToGraph(NodeType op, OpInfo info, NodeIndex child1 = NoNode, NodeIndex child2 = NoNode, NodeIndex child3 = NoNode)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(op, currentCodeOrigin(), info, child1, child2, child3));
        if (op == Phi)
            m_currentBlock->phis.append(resultIndex);
        else
            m_currentBlock->append(resultIndex);

        if (defaultFlags(op) & NodeMustGenerate)
            m_graph.ref(resultIndex);
        return resultIndex;
    }
    NodeIndex addToGraph(NodeType op, OpInfo info1, OpInfo info2, NodeIndex child1 = NoNode, NodeIndex child2 = NoNode, NodeIndex child3 = NoNode)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(op, currentCodeOrigin(), info1, info2, child1, child2, child3));
        ASSERT(op != Phi);
        m_currentBlock->append(resultIndex);

        if (defaultFlags(op) & NodeMustGenerate)
            m_graph.ref(resultIndex);
        return resultIndex;
    }
    
    NodeIndex addToGraph(Node::VarArgTag, NodeType op, OpInfo info1, OpInfo info2)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(Node::VarArg, op, currentCodeOrigin(), info1, info2, m_graph.m_varArgChildren.size() - m_numPassedVarArgs, m_numPassedVarArgs));
        ASSERT(op != Phi);
        m_currentBlock->append(resultIndex);
        
        m_numPassedVarArgs = 0;
        
        if (defaultFlags(op) & NodeMustGenerate)
            m_graph.ref(resultIndex);
        return resultIndex;
    }

    NodeIndex insertPhiNode(OpInfo info, BasicBlock* block)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(Phi, currentCodeOrigin(), info));
        block->phis.append(resultIndex);

        return resultIndex;
    }

    void addVarArgChild(NodeIndex child)
    {
        m_graph.m_varArgChildren.append(Edge(child));
        m_numPassedVarArgs++;
    }
    
    NodeIndex addCall(Interpreter* interpreter, Instruction* currentInstruction, NodeType op)
    {
        Instruction* putInstruction = currentInstruction + OPCODE_LENGTH(op_call);

        SpeculatedType prediction = SpecNone;
        if (interpreter->getOpcodeID(putInstruction->u.opcode) == op_call_put_result) {
            m_currentProfilingIndex = m_currentIndex + OPCODE_LENGTH(op_call);
            prediction = getPrediction();
        }
        
        addVarArgChild(get(currentInstruction[1].u.operand));
        int argCount = currentInstruction[2].u.operand;
        if (RegisterFile::CallFrameHeaderSize + (unsigned)argCount > m_parameterSlots)
            m_parameterSlots = RegisterFile::CallFrameHeaderSize + argCount;

        int registerOffset = currentInstruction[3].u.operand;
        int dummyThisArgument = op == Call ? 0 : 1;
        for (int i = 0 + dummyThisArgument; i < argCount; ++i)
            addVarArgChild(get(registerOffset + argumentToOperand(i)));

        NodeIndex call = addToGraph(Node::VarArg, op, OpInfo(0), OpInfo(prediction));
        if (interpreter->getOpcodeID(putInstruction->u.opcode) == op_call_put_result)
            set(putInstruction[1].u.operand, call);
        return call;
    }
    
    NodeIndex addStructureTransitionCheck(JSCell* object, Structure* structure)
    {
        // Add a weak JS constant for the object regardless, since the code should
        // be jettisoned if the object ever dies.
        NodeIndex objectIndex = cellConstant(object);
        
        if (object->structure() == structure && structure->transitionWatchpointSetIsStillValid()) {
            addToGraph(StructureTransitionWatchpoint, OpInfo(structure), objectIndex);
            return objectIndex;
        }
        
        addToGraph(CheckStructure, OpInfo(m_graph.addStructureSet(structure)), objectIndex);
        
        return objectIndex;
    }
    
    NodeIndex addStructureTransitionCheck(JSCell* object)
    {
        return addStructureTransitionCheck(object, object->structure());
    }
    
    SpeculatedType getPredictionWithoutOSRExit(NodeIndex nodeIndex, unsigned bytecodeIndex)
    {
        UNUSED_PARAM(nodeIndex);
        
        SpeculatedType prediction = m_inlineStackTop->m_profiledBlock->valueProfilePredictionForBytecodeOffset(bytecodeIndex);
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Dynamic [@%u, bc#%u] prediction: %s\n", nodeIndex, bytecodeIndex, speculationToString(prediction));
#endif
        
        return prediction;
    }

    SpeculatedType getPrediction(NodeIndex nodeIndex, unsigned bytecodeIndex)
    {
        SpeculatedType prediction = getPredictionWithoutOSRExit(nodeIndex, bytecodeIndex);
        
        if (prediction == SpecNone) {
            // We have no information about what values this node generates. Give up
            // on executing this code, since we're likely to do more damage than good.
            addToGraph(ForceOSRExit);
        }
        
        return prediction;
    }
    
    SpeculatedType getPredictionWithoutOSRExit()
    {
        return getPredictionWithoutOSRExit(m_graph.size(), m_currentProfilingIndex);
    }
    
    SpeculatedType getPrediction()
    {
        return getPrediction(m_graph.size(), m_currentProfilingIndex);
    }
    
    Array::Mode getArrayMode(ArrayProfile* profile)
    {
        profile->computeUpdatedPrediction();
        return fromObserved(profile, Array::Read, false);
    }
    
    Array::Mode getArrayModeAndEmitChecks(ArrayProfile* profile, Array::Action action, NodeIndex base)
    {
        profile->computeUpdatedPrediction();
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        if (m_inlineStackTop->m_profiledBlock->numberOfRareCaseProfiles())
            dataLog("Slow case profile for bc#%u: %u\n", m_currentIndex, m_inlineStackTop->m_profiledBlock->rareCaseProfileForBytecodeOffset(m_currentIndex)->m_counter);
        dataLog("Array profile for bc#%u: %p%s%s, %u\n", m_currentIndex, profile->expectedStructure(), profile->structureIsPolymorphic() ? " (polymorphic)" : "", profile->mayInterceptIndexedAccesses() ? " (may intercept)" : "", profile->observedArrayModes());
#endif
        
        bool makeSafe =
            m_inlineStackTop->m_profiledBlock->couldTakeSlowCase(m_currentIndex)
            || m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, OutOfBounds);
        
        Array::Mode result = fromObserved(profile, action, makeSafe);
        
        if (profile->hasDefiniteStructure() && benefitsFromStructureCheck(result))
            addToGraph(CheckStructure, OpInfo(m_graph.addStructureSet(profile->expectedStructure())), base);
        
        return result;
    }
    
    NodeIndex makeSafe(NodeIndex nodeIndex)
    {
        Node& node = m_graph[nodeIndex];
        
        bool likelyToTakeSlowCase;
        if (!isX86() && node.op() == ArithMod)
            likelyToTakeSlowCase = false;
        else
            likelyToTakeSlowCase = m_inlineStackTop->m_profiledBlock->likelyToTakeSlowCase(m_currentIndex);
        
        if (!likelyToTakeSlowCase
            && !m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, Overflow)
            && !m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, NegativeZero))
            return nodeIndex;
        
        switch (m_graph[nodeIndex].op()) {
        case UInt32ToNumber:
        case ArithAdd:
        case ArithSub:
        case ArithNegate:
        case ValueAdd:
        case ArithMod: // for ArithMod "MayOverflow" means we tried to divide by zero, or we saw double.
            m_graph[nodeIndex].mergeFlags(NodeMayOverflow);
            break;
            
        case ArithMul:
            if (m_inlineStackTop->m_profiledBlock->likelyToTakeDeepestSlowCase(m_currentIndex)
                || m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, Overflow)) {
#if DFG_ENABLE(DEBUG_VERBOSE)
                dataLog("Making ArithMul @%u take deepest slow case.\n", nodeIndex);
#endif
                m_graph[nodeIndex].mergeFlags(NodeMayOverflow | NodeMayNegZero);
            } else if (m_inlineStackTop->m_profiledBlock->likelyToTakeSlowCase(m_currentIndex)
                       || m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, NegativeZero)) {
#if DFG_ENABLE(DEBUG_VERBOSE)
                dataLog("Making ArithMul @%u take faster slow case.\n", nodeIndex);
#endif
                m_graph[nodeIndex].mergeFlags(NodeMayNegZero);
            }
            break;
            
        default:
            ASSERT_NOT_REACHED();
            break;
        }
        
        return nodeIndex;
    }
    
    NodeIndex makeDivSafe(NodeIndex nodeIndex)
    {
        ASSERT(m_graph[nodeIndex].op() == ArithDiv);
        
        // The main slow case counter for op_div in the old JIT counts only when
        // the operands are not numbers. We don't care about that since we already
        // have speculations in place that take care of that separately. We only
        // care about when the outcome of the division is not an integer, which
        // is what the special fast case counter tells us.
        
        if (!m_inlineStackTop->m_profiledBlock->couldTakeSpecialFastCase(m_currentIndex)
            && !m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, Overflow)
            && !m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, NegativeZero))
            return nodeIndex;
        
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Making %s @%u safe at bc#%u because special fast-case counter is at %u and exit profiles say %d, %d\n", Graph::opName(m_graph[nodeIndex].op()), nodeIndex, m_currentIndex, m_inlineStackTop->m_profiledBlock->specialFastCaseProfileForBytecodeOffset(m_currentIndex)->m_counter, m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, Overflow), m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, NegativeZero));
#endif
        
        // FIXME: It might be possible to make this more granular. The DFG certainly can
        // distinguish between negative zero and overflow in its exit profiles.
        m_graph[nodeIndex].mergeFlags(NodeMayOverflow | NodeMayNegZero);
        
        return nodeIndex;
    }
    
    bool willNeedFlush(StructureStubInfo& stubInfo)
    {
        PolymorphicAccessStructureList* list;
        int listSize;
        switch (stubInfo.accessType) {
        case access_get_by_id_self_list:
            list = stubInfo.u.getByIdSelfList.structureList;
            listSize = stubInfo.u.getByIdSelfList.listSize;
            break;
        case access_get_by_id_proto_list:
            list = stubInfo.u.getByIdProtoList.structureList;
            listSize = stubInfo.u.getByIdProtoList.listSize;
            break;
        default:
            return false;
        }
        for (int i = 0; i < listSize; ++i) {
            if (!list->list[i].isDirect)
                return true;
        }
        return false;
    }
    
    bool structureChainIsStillValid(bool direct, Structure* previousStructure, StructureChain* chain)
    {
        if (direct)
            return true;
        
        if (!previousStructure->storedPrototype().isNull() && previousStructure->storedPrototype().asCell()->structure() != chain->head()->get())
            return false;
        
        for (WriteBarrier<Structure>* it = chain->head(); *it; ++it) {
            if (!(*it)->storedPrototype().isNull() && (*it)->storedPrototype().asCell()->structure() != it[1].get())
                return false;
        }
        
        return true;
    }
    
    void buildOperandMapsIfNecessary();
    
    ExecState* m_exec;
    JSGlobalData* m_globalData;
    CodeBlock* m_codeBlock;
    CodeBlock* m_profiledBlock;
    Graph& m_graph;

    // The current block being generated.
    BasicBlock* m_currentBlock;
    // The bytecode index of the current instruction being generated.
    unsigned m_currentIndex;
    // The bytecode index of the value profile of the current instruction being generated.
    unsigned m_currentProfilingIndex;

    // We use these values during code generation, and to avoid the need for
    // special handling we make sure they are available as constants in the
    // CodeBlock's constant pool. These variables are initialized to
    // UINT_MAX, and lazily updated to hold an index into the CodeBlock's
    // constant pool, as necessary.
    unsigned m_constantUndefined;
    unsigned m_constantNull;
    unsigned m_constantNaN;
    unsigned m_constant1;
    HashMap<JSCell*, unsigned> m_cellConstants;
    HashMap<JSCell*, NodeIndex> m_cellConstantNodes;

    // A constant in the constant pool may be represented by more than one
    // node in the graph, depending on the context in which it is being used.
    struct ConstantRecord {
        ConstantRecord()
            : asInt32(NoNode)
            , asNumeric(NoNode)
            , asJSValue(NoNode)
        {
        }

        NodeIndex asInt32;
        NodeIndex asNumeric;
        NodeIndex asJSValue;
    };

    // Track the index of the node whose result is the current value for every
    // register value in the bytecode - argument, local, and temporary.
    Vector<ConstantRecord, 16> m_constants;

    // The number of arguments passed to the function.
    unsigned m_numArguments;
    // The number of locals (vars + temporaries) used in the function.
    unsigned m_numLocals;
    // The set of registers we need to preserve across BasicBlock boundaries;
    // typically equal to the set of vars, but we expand this to cover all
    // temporaries that persist across blocks (dues to ?:, &&, ||, etc).
    BitVector m_preservedVars;
    // The number of slots (in units of sizeof(Register)) that we need to
    // preallocate for calls emanating from this frame. This includes the
    // size of the CallFrame, only if this is not a leaf function.  (I.e.
    // this is 0 if and only if this function is a leaf.)
    unsigned m_parameterSlots;
    // The number of var args passed to the next var arg node.
    unsigned m_numPassedVarArgs;
    // The index in the global resolve info.
    unsigned m_globalResolveNumber;

    struct PhiStackEntry {
        PhiStackEntry(BasicBlock* block, NodeIndex phi, unsigned varNo)
            : m_block(block)
            , m_phi(phi)
            , m_varNo(varNo)
        {
        }

        BasicBlock* m_block;
        NodeIndex m_phi;
        unsigned m_varNo;
    };
    Vector<PhiStackEntry, 16> m_argumentPhiStack;
    Vector<PhiStackEntry, 16> m_localPhiStack;
    
    struct InlineStackEntry {
        ByteCodeParser* m_byteCodeParser;
        
        CodeBlock* m_codeBlock;
        CodeBlock* m_profiledBlock;
        InlineCallFrame* m_inlineCallFrame;
        VirtualRegister m_calleeVR; // absolute virtual register, not relative to call frame
        
        ScriptExecutable* executable() { return m_codeBlock->ownerExecutable(); }
        
        QueryableExitProfile m_exitProfile;
        
        // Remapping of identifier and constant numbers from the code block being
        // inlined (inline callee) to the code block that we're inlining into
        // (the machine code block, which is the transitive, though not necessarily
        // direct, caller).
        Vector<unsigned> m_identifierRemap;
        Vector<unsigned> m_constantRemap;
        
        // Blocks introduced by this code block, which need successor linking.
        // May include up to one basic block that includes the continuation after
        // the callsite in the caller. These must be appended in the order that they
        // are created, but their bytecodeBegin values need not be in order as they
        // are ignored.
        Vector<UnlinkedBlock> m_unlinkedBlocks;
        
        // Potential block linking targets. Must be sorted by bytecodeBegin, and
        // cannot have two blocks that have the same bytecodeBegin. For this very
        // reason, this is not equivalent to 
        Vector<BlockIndex> m_blockLinkingTargets;
        
        // If the callsite's basic block was split into two, then this will be
        // the head of the callsite block. It needs its successors linked to the
        // m_unlinkedBlocks, but not the other way around: there's no way for
        // any blocks in m_unlinkedBlocks to jump back into this block.
        BlockIndex m_callsiteBlockHead;
        
        // Does the callsite block head need linking? This is typically true
        // but will be false for the machine code block's inline stack entry
        // (since that one is not inlined) and for cases where an inline callee
        // did the linking for us.
        bool m_callsiteBlockHeadNeedsLinking;
        
        VirtualRegister m_returnValue;
        
        // Speculations about variable types collected from the profiled code block,
        // which are based on OSR exit profiles that past DFG compilatins of this
        // code block had gathered.
        LazyOperandValueProfileParser m_lazyOperands;
        
        // Did we see any returns? We need to handle the (uncommon but necessary)
        // case where a procedure that does not return was inlined.
        bool m_didReturn;
        
        // Did we have any early returns?
        bool m_didEarlyReturn;
        
        // Pointers to the argument position trackers for this slice of code.
        Vector<ArgumentPosition*> m_argumentPositions;
        
        InlineStackEntry* m_caller;
        
        InlineStackEntry(
            ByteCodeParser*,
            CodeBlock*,
            CodeBlock* profiledBlock,
            BlockIndex callsiteBlockHead,
            VirtualRegister calleeVR,
            JSFunction* callee,
            VirtualRegister returnValueVR,
            VirtualRegister inlineCallFrameStart,
            int argumentCountIncludingThis,
            CodeSpecializationKind);
        
        ~InlineStackEntry()
        {
            m_byteCodeParser->m_inlineStackTop = m_caller;
        }
        
        int remapOperand(int operand) const
        {
            if (!m_inlineCallFrame)
                return operand;
            
            if (operand >= FirstConstantRegisterIndex) {
                int result = m_constantRemap[operand - FirstConstantRegisterIndex];
                ASSERT(result >= FirstConstantRegisterIndex);
                return result;
            }

            if (operand == RegisterFile::Callee)
                return m_calleeVR;

            return operand + m_inlineCallFrame->stackOffset;
        }
    };
    
    InlineStackEntry* m_inlineStackTop;

    // Have we built operand maps? We initialize them lazily, and only when doing
    // inlining.
    bool m_haveBuiltOperandMaps;
    // Mapping between identifier names and numbers.
    IdentifierMap m_identifierMap;
    // Mapping between values and constant numbers.
    JSValueMap m_jsValueMap;
    // Index of the empty value, or UINT_MAX if there is no mapping. This is a horrible
    // work-around for the fact that JSValueMap can't handle "empty" values.
    unsigned m_emptyJSValueIndex;
    
    // Cache of code blocks that we've generated bytecode for.
    ByteCodeCache<canInlineFunctionFor> m_codeBlockCache;
    
    Instruction* m_currentInstruction;
};

#define NEXT_OPCODE(name) \
    m_currentIndex += OPCODE_LENGTH(name); \
    continue

#define LAST_OPCODE(name) \
    m_currentIndex += OPCODE_LENGTH(name); \
    return shouldContinueParsing


void ByteCodeParser::handleCall(Interpreter* interpreter, Instruction* currentInstruction, NodeType op, CodeSpecializationKind kind)
{
    ASSERT(OPCODE_LENGTH(op_call) == OPCODE_LENGTH(op_construct));
    
    NodeIndex callTarget = get(currentInstruction[1].u.operand);
    enum {
        ConstantFunction,
        ConstantInternalFunction,
        LinkedFunction,
        UnknownFunction
    } callType;
            
    CallLinkStatus callLinkStatus = CallLinkStatus::computeFor(
        m_inlineStackTop->m_profiledBlock, m_currentIndex);
    
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("For call at @%lu bc#%u: ", m_graph.size(), m_currentIndex);
    if (callLinkStatus.isSet()) {
        if (callLinkStatus.couldTakeSlowPath())
            dataLog("could take slow path, ");
        dataLog("target = %p\n", callLinkStatus.callTarget());
    } else
        dataLog("not set.\n");
#endif
    
    if (m_graph.isFunctionConstant(callTarget)) {
        callType = ConstantFunction;
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Call at [@%lu, bc#%u] has a function constant: %p, exec %p.\n",
                m_graph.size(), m_currentIndex,
                m_graph.valueOfFunctionConstant(callTarget),
                m_graph.valueOfFunctionConstant(callTarget)->executable());
#endif
    } else if (m_graph.isInternalFunctionConstant(callTarget)) {
        callType = ConstantInternalFunction;
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Call at [@%lu, bc#%u] has an internal function constant: %p.\n",
                m_graph.size(), m_currentIndex,
                m_graph.valueOfInternalFunctionConstant(callTarget));
#endif
    } else if (callLinkStatus.isSet() && !callLinkStatus.couldTakeSlowPath()
               && !m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, BadCache)) {
        callType = LinkedFunction;
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Call at [@%lu, bc#%u] is linked to: %p, exec %p.\n",
                m_graph.size(), m_currentIndex, callLinkStatus.callTarget(),
                callLinkStatus.callTarget()->executable());
#endif
    } else {
        callType = UnknownFunction;
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Call at [@%lu, bc#%u] is has an unknown or ambiguous target.\n",
                m_graph.size(), m_currentIndex);
#endif
    }
    if (callType != UnknownFunction) {
        int argumentCountIncludingThis = currentInstruction[2].u.operand;
        int registerOffset = currentInstruction[3].u.operand;

        // Do we have a result?
        bool usesResult = false;
        int resultOperand = 0; // make compiler happy
        unsigned nextOffset = m_currentIndex + OPCODE_LENGTH(op_call);
        Instruction* putInstruction = currentInstruction + OPCODE_LENGTH(op_call);
        SpeculatedType prediction = SpecNone;
        if (interpreter->getOpcodeID(putInstruction->u.opcode) == op_call_put_result) {
            resultOperand = putInstruction[1].u.operand;
            usesResult = true;
            m_currentProfilingIndex = nextOffset;
            prediction = getPrediction();
            nextOffset += OPCODE_LENGTH(op_call_put_result);
        }

        if (callType == ConstantInternalFunction) {
            if (handleConstantInternalFunction(usesResult, resultOperand, m_graph.valueOfInternalFunctionConstant(callTarget), registerOffset, argumentCountIncludingThis, prediction, kind))
                return;
            
            // Can only handle this using the generic call handler.
            addCall(interpreter, currentInstruction, op);
            return;
        }
        
        JSFunction* expectedFunction;
        Intrinsic intrinsic;
        bool certainAboutExpectedFunction;
        if (callType == ConstantFunction) {
            expectedFunction = m_graph.valueOfFunctionConstant(callTarget);
            intrinsic = expectedFunction->executable()->intrinsicFor(kind);
            certainAboutExpectedFunction = true;
        } else {
            ASSERT(callType == LinkedFunction);
            expectedFunction = callLinkStatus.callTarget();
            intrinsic = expectedFunction->executable()->intrinsicFor(kind);
            certainAboutExpectedFunction = false;
        }
                
        if (intrinsic != NoIntrinsic) {
            if (!certainAboutExpectedFunction)
                emitFunctionCheck(expectedFunction, callTarget, registerOffset, kind);
            
            if (handleIntrinsic(usesResult, resultOperand, intrinsic, registerOffset, argumentCountIncludingThis, prediction)) {
                if (!certainAboutExpectedFunction) {
                    // Need to keep the call target alive for OSR. We could easily optimize this out if we wanted
                    // to, since at this point we know that the call target is a constant. It's just that OSR isn't
                    // smart enough to figure that out, since it doesn't understand CheckFunction.
                    addToGraph(Phantom, callTarget);
                }
                
                return;
            }
        } else if (handleInlining(usesResult, currentInstruction[1].u.operand, callTarget, resultOperand, certainAboutExpectedFunction, expectedFunction, registerOffset, argumentCountIncludingThis, nextOffset, kind))
            return;
    }
    
    addCall(interpreter, currentInstruction, op);
}

void ByteCodeParser::emitFunctionCheck(JSFunction* expectedFunction, NodeIndex callTarget, int registerOffset, CodeSpecializationKind kind)
{
    NodeIndex thisArgument;
    if (kind == CodeForCall)
        thisArgument = get(registerOffset + argumentToOperand(0));
    else
        thisArgument = NoNode;
    addToGraph(CheckFunction, OpInfo(expectedFunction), callTarget, thisArgument);
}

bool ByteCodeParser::handleInlining(bool usesResult, int callTarget, NodeIndex callTargetNodeIndex, int resultOperand, bool certainAboutExpectedFunction, JSFunction* expectedFunction, int registerOffset, int argumentCountIncludingThis, unsigned nextOffset, CodeSpecializationKind kind)
{
    // First, the really simple checks: do we have an actual JS function?
    if (!expectedFunction)
        return false;
    if (expectedFunction->isHostFunction())
        return false;
    
    FunctionExecutable* executable = expectedFunction->jsExecutable();
    
    // Does the number of arguments we're passing match the arity of the target? We currently
    // inline only if the number of arguments passed is greater than or equal to the number
    // arguments expected.
    if (static_cast<int>(executable->parameterCount()) + 1 > argumentCountIncludingThis)
        return false;
    
    // Have we exceeded inline stack depth, or are we trying to inline a recursive call?
    // If either of these are detected, then don't inline.
    unsigned depth = 0;
    for (InlineStackEntry* entry = m_inlineStackTop; entry; entry = entry->m_caller) {
        ++depth;
        if (depth >= Options::maximumInliningDepth())
            return false; // Depth exceeded.
        
        if (entry->executable() == executable)
            return false; // Recursion detected.
    }
    
    // Does the code block's size match the heuristics/requirements for being
    // an inline candidate?
    CodeBlock* profiledBlock = executable->profiledCodeBlockFor(kind);
    if (!profiledBlock)
        return false;
    
    if (!mightInlineFunctionFor(profiledBlock, kind))
        return false;
    
    // If we get here then it looks like we should definitely inline this code. Proceed
    // with parsing the code to get bytecode, so that we can then parse the bytecode.
    // Note that if LLInt is enabled, the bytecode will always be available. Also note
    // that if LLInt is enabled, we may inline a code block that has never been JITted
    // before!
    CodeBlock* codeBlock = m_codeBlockCache.get(CodeBlockKey(executable, kind), expectedFunction->scope());
    if (!codeBlock)
        return false;
    
    ASSERT(canInlineFunctionFor(codeBlock, kind));

#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("Inlining executable %p.\n", executable);
#endif
    
    // Now we know without a doubt that we are committed to inlining. So begin the process
    // by checking the callee (if necessary) and making sure that arguments and the callee
    // are flushed.
    if (!certainAboutExpectedFunction)
        emitFunctionCheck(expectedFunction, callTargetNodeIndex, registerOffset, kind);
    
    // FIXME: Don't flush constants!
    
    int inlineCallFrameStart = m_inlineStackTop->remapOperand(registerOffset) - RegisterFile::CallFrameHeaderSize;
    
    // Make sure that the area used by the call frame is reserved.
    for (int arg = inlineCallFrameStart + RegisterFile::CallFrameHeaderSize + codeBlock->m_numVars; arg-- > inlineCallFrameStart;)
        m_preservedVars.set(arg);
    
    // Make sure that we have enough locals.
    unsigned newNumLocals = inlineCallFrameStart + RegisterFile::CallFrameHeaderSize + codeBlock->m_numCalleeRegisters;
    if (newNumLocals > m_numLocals) {
        m_numLocals = newNumLocals;
        for (size_t i = 0; i < m_graph.m_blocks.size(); ++i)
            m_graph.m_blocks[i]->ensureLocals(newNumLocals);
    }
    
    size_t argumentPositionStart = m_graph.m_argumentPositions.size();

    InlineStackEntry inlineStackEntry(
        this, codeBlock, profiledBlock, m_graph.m_blocks.size() - 1,
        (VirtualRegister)m_inlineStackTop->remapOperand(callTarget), expectedFunction,
        (VirtualRegister)m_inlineStackTop->remapOperand(
            usesResult ? resultOperand : InvalidVirtualRegister),
        (VirtualRegister)inlineCallFrameStart, argumentCountIncludingThis, kind);
    
    // This is where the actual inlining really happens.
    unsigned oldIndex = m_currentIndex;
    unsigned oldProfilingIndex = m_currentProfilingIndex;
    m_currentIndex = 0;
    m_currentProfilingIndex = 0;

    addToGraph(InlineStart, OpInfo(argumentPositionStart));
    
    parseCodeBlock();
    
    m_currentIndex = oldIndex;
    m_currentProfilingIndex = oldProfilingIndex;
    
    // If the inlined code created some new basic blocks, then we have linking to do.
    if (inlineStackEntry.m_callsiteBlockHead != m_graph.m_blocks.size() - 1) {
        
        ASSERT(!inlineStackEntry.m_unlinkedBlocks.isEmpty());
        if (inlineStackEntry.m_callsiteBlockHeadNeedsLinking)
            linkBlock(m_graph.m_blocks[inlineStackEntry.m_callsiteBlockHead].get(), inlineStackEntry.m_blockLinkingTargets);
        else
            ASSERT(m_graph.m_blocks[inlineStackEntry.m_callsiteBlockHead]->isLinked);
        
        // It's possible that the callsite block head is not owned by the caller.
        if (!inlineStackEntry.m_caller->m_unlinkedBlocks.isEmpty()) {
            // It's definitely owned by the caller, because the caller created new blocks.
            // Assert that this all adds up.
            ASSERT(inlineStackEntry.m_caller->m_unlinkedBlocks.last().m_blockIndex == inlineStackEntry.m_callsiteBlockHead);
            ASSERT(inlineStackEntry.m_caller->m_unlinkedBlocks.last().m_needsNormalLinking);
            inlineStackEntry.m_caller->m_unlinkedBlocks.last().m_needsNormalLinking = false;
        } else {
            // It's definitely not owned by the caller. Tell the caller that he does not
            // need to link his callsite block head, because we did it for him.
            ASSERT(inlineStackEntry.m_caller->m_callsiteBlockHeadNeedsLinking);
            ASSERT(inlineStackEntry.m_caller->m_callsiteBlockHead == inlineStackEntry.m_callsiteBlockHead);
            inlineStackEntry.m_caller->m_callsiteBlockHeadNeedsLinking = false;
        }
        
        linkBlocks(inlineStackEntry.m_unlinkedBlocks, inlineStackEntry.m_blockLinkingTargets);
    } else
        ASSERT(inlineStackEntry.m_unlinkedBlocks.isEmpty());
    
    // If there was a return, but no early returns, then we're done. We allow parsing of
    // the caller to continue in whatever basic block we're in right now.
    if (!inlineStackEntry.m_didEarlyReturn && inlineStackEntry.m_didReturn) {
        BasicBlock* lastBlock = m_graph.m_blocks.last().get();
        ASSERT(lastBlock->isEmpty() || !m_graph.last().isTerminal());
        
        // If we created new blocks then the last block needs linking, but in the
        // caller. It doesn't need to be linked to, but it needs outgoing links.
        if (!inlineStackEntry.m_unlinkedBlocks.isEmpty()) {
#if DFG_ENABLE(DEBUG_VERBOSE)
            dataLog("Reascribing bytecode index of block %p from bc#%u to bc#%u (inline return case).\n", lastBlock, lastBlock->bytecodeBegin, m_currentIndex);
#endif
            // For debugging purposes, set the bytecodeBegin. Note that this doesn't matter
            // for release builds because this block will never serve as a potential target
            // in the linker's binary search.
            lastBlock->bytecodeBegin = m_currentIndex;
            m_inlineStackTop->m_caller->m_unlinkedBlocks.append(UnlinkedBlock(m_graph.m_blocks.size() - 1));
        }
        
        m_currentBlock = m_graph.m_blocks.last().get();

#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Done inlining executable %p, continuing code generation at epilogue.\n", executable);
#endif
        return true;
    }
    
    // If we get to this point then all blocks must end in some sort of terminals.
    ASSERT(m_graph.last().isTerminal());
    
    // Link the early returns to the basic block we're about to create.
    for (size_t i = 0; i < inlineStackEntry.m_unlinkedBlocks.size(); ++i) {
        if (!inlineStackEntry.m_unlinkedBlocks[i].m_needsEarlyReturnLinking)
            continue;
        BasicBlock* block = m_graph.m_blocks[inlineStackEntry.m_unlinkedBlocks[i].m_blockIndex].get();
        ASSERT(!block->isLinked);
        Node& node = m_graph[block->last()];
        ASSERT(node.op() == Jump);
        ASSERT(node.takenBlockIndex() == NoBlock);
        node.setTakenBlockIndex(m_graph.m_blocks.size());
        inlineStackEntry.m_unlinkedBlocks[i].m_needsEarlyReturnLinking = false;
#if !ASSERT_DISABLED
        block->isLinked = true;
#endif
    }
    
    // Need to create a new basic block for the continuation at the caller.
    OwnPtr<BasicBlock> block = adoptPtr(new BasicBlock(nextOffset, m_numArguments, m_numLocals));
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("Creating inline epilogue basic block %p, #%zu for %p bc#%u at inline depth %u.\n", block.get(), m_graph.m_blocks.size(), m_inlineStackTop->executable(), m_currentIndex, CodeOrigin::inlineDepthForCallFrame(m_inlineStackTop->m_inlineCallFrame));
#endif
    m_currentBlock = block.get();
    ASSERT(m_inlineStackTop->m_caller->m_blockLinkingTargets.isEmpty() || m_graph.m_blocks[m_inlineStackTop->m_caller->m_blockLinkingTargets.last()]->bytecodeBegin < nextOffset);
    m_inlineStackTop->m_caller->m_unlinkedBlocks.append(UnlinkedBlock(m_graph.m_blocks.size()));
    m_inlineStackTop->m_caller->m_blockLinkingTargets.append(m_graph.m_blocks.size());
    m_graph.m_blocks.append(block.release());
    prepareToParseBlock();
    
    // At this point we return and continue to generate code for the caller, but
    // in the new basic block.
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("Done inlining executable %p, continuing code generation in new block.\n", executable);
#endif
    return true;
}

void ByteCodeParser::setIntrinsicResult(bool usesResult, int resultOperand, NodeIndex nodeIndex)
{
    if (!usesResult)
        return;
    set(resultOperand, nodeIndex);
}

bool ByteCodeParser::handleMinMax(bool usesResult, int resultOperand, NodeType op, int registerOffset, int argumentCountIncludingThis)
{
    if (argumentCountIncludingThis == 1) { // Math.min()
        setIntrinsicResult(usesResult, resultOperand, constantNaN());
        return true;
    }
     
    if (argumentCountIncludingThis == 2) { // Math.min(x)
        // FIXME: what we'd really like is a ValueToNumber, except we don't support that right now. Oh well.
        NodeIndex result = get(registerOffset + argumentToOperand(1));
        addToGraph(CheckNumber, result);
        setIntrinsicResult(usesResult, resultOperand, result);
        return true;
    }
    
    if (argumentCountIncludingThis == 3) { // Math.min(x, y)
        setIntrinsicResult(usesResult, resultOperand, addToGraph(op, get(registerOffset + argumentToOperand(1)), get(registerOffset + argumentToOperand(2))));
        return true;
    }
    
    // Don't handle >=3 arguments for now.
    return false;
}

// FIXME: We dead-code-eliminate unused Math intrinsics, but that's invalid because
// they need to perform the ToNumber conversion, which can have side-effects.
bool ByteCodeParser::handleIntrinsic(bool usesResult, int resultOperand, Intrinsic intrinsic, int registerOffset, int argumentCountIncludingThis, SpeculatedType prediction)
{
    switch (intrinsic) {
    case AbsIntrinsic: {
        if (argumentCountIncludingThis == 1) { // Math.abs()
            setIntrinsicResult(usesResult, resultOperand, constantNaN());
            return true;
        }

        if (!MacroAssembler::supportsFloatingPointAbs())
            return false;

        NodeIndex nodeIndex = addToGraph(ArithAbs, get(registerOffset + argumentToOperand(1)));
        if (m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, Overflow))
            m_graph[nodeIndex].mergeFlags(NodeMayOverflow);
        setIntrinsicResult(usesResult, resultOperand, nodeIndex);
        return true;
    }

    case MinIntrinsic:
        return handleMinMax(usesResult, resultOperand, ArithMin, registerOffset, argumentCountIncludingThis);
        
    case MaxIntrinsic:
        return handleMinMax(usesResult, resultOperand, ArithMax, registerOffset, argumentCountIncludingThis);
        
    case SqrtIntrinsic: {
        if (argumentCountIncludingThis == 1) { // Math.sqrt()
            setIntrinsicResult(usesResult, resultOperand, constantNaN());
            return true;
        }
        
        if (!MacroAssembler::supportsFloatingPointSqrt())
            return false;
        
        setIntrinsicResult(usesResult, resultOperand, addToGraph(ArithSqrt, get(registerOffset + argumentToOperand(1))));
        return true;
    }
        
    case ArrayPushIntrinsic: {
        if (argumentCountIncludingThis != 2)
            return false;
        
        Array::Mode arrayMode = getArrayMode(m_currentInstruction[5].u.arrayProfile);
        if (!modeIsJSArray(arrayMode))
            return false;
        NodeIndex arrayPush = addToGraph(ArrayPush, OpInfo(arrayMode), OpInfo(prediction), get(registerOffset + argumentToOperand(0)), get(registerOffset + argumentToOperand(1)));
        if (usesResult)
            set(resultOperand, arrayPush);
        
        return true;
    }
        
    case ArrayPopIntrinsic: {
        if (argumentCountIncludingThis != 1)
            return false;
        
        Array::Mode arrayMode = getArrayMode(m_currentInstruction[5].u.arrayProfile);
        if (!modeIsJSArray(arrayMode))
            return false;
        NodeIndex arrayPop = addToGraph(ArrayPop, OpInfo(arrayMode), OpInfo(prediction), get(registerOffset + argumentToOperand(0)));
        if (usesResult)
            set(resultOperand, arrayPop);
        return true;
    }

    case CharCodeAtIntrinsic: {
        if (argumentCountIncludingThis != 2)
            return false;

        int thisOperand = registerOffset + argumentToOperand(0);
        if (!(m_graph[get(thisOperand)].prediction() & SpecString))
            return false;
        
        int indexOperand = registerOffset + argumentToOperand(1);
        NodeIndex charCode = addToGraph(StringCharCodeAt, OpInfo(Array::String), get(thisOperand), getToInt32(indexOperand));

        if (usesResult)
            set(resultOperand, charCode);
        return true;
    }

    case CharAtIntrinsic: {
        if (argumentCountIncludingThis != 2)
            return false;

        int thisOperand = registerOffset + argumentToOperand(0);
        if (!(m_graph[get(thisOperand)].prediction() & SpecString))
            return false;

        int indexOperand = registerOffset + argumentToOperand(1);
        NodeIndex charCode = addToGraph(StringCharAt, OpInfo(Array::String), get(thisOperand), getToInt32(indexOperand));

        if (usesResult)
            set(resultOperand, charCode);
        return true;
    }

    case RegExpExecIntrinsic: {
        if (argumentCountIncludingThis != 2)
            return false;
        
        NodeIndex regExpExec = addToGraph(RegExpExec, OpInfo(0), OpInfo(prediction), get(registerOffset + argumentToOperand(0)), get(registerOffset + argumentToOperand(1)));
        if (usesResult)
            set(resultOperand, regExpExec);
        
        return true;
    }
        
    case RegExpTestIntrinsic: {
        if (argumentCountIncludingThis != 2)
            return false;
        
        NodeIndex regExpExec = addToGraph(RegExpTest, OpInfo(0), OpInfo(prediction), get(registerOffset + argumentToOperand(0)), get(registerOffset + argumentToOperand(1)));
        if (usesResult)
            set(resultOperand, regExpExec);
        
        return true;
    }
        
    default:
        return false;
    }
}

bool ByteCodeParser::handleConstantInternalFunction(
    bool usesResult, int resultOperand, InternalFunction* function, int registerOffset,
    int argumentCountIncludingThis, SpeculatedType prediction, CodeSpecializationKind kind)
{
    // If we ever find that we have a lot of internal functions that we specialize for,
    // then we should probably have some sort of hashtable dispatch, or maybe even
    // dispatch straight through the MethodTable of the InternalFunction. But for now,
    // it seems that this case is hit infrequently enough, and the number of functions
    // we know about is small enough, that having just a linear cascade of if statements
    // is good enough.
    
    UNUSED_PARAM(prediction); // Remove this once we do more things.
    UNUSED_PARAM(kind); // Remove this once we do more things.
    
    if (function->classInfo() == &ArrayConstructor::s_info) {
        if (argumentCountIncludingThis == 2) {
            setIntrinsicResult(
                usesResult, resultOperand,
                addToGraph(NewArrayWithSize, get(registerOffset + argumentToOperand(1))));
            return true;
        }
        
        for (int i = 1; i < argumentCountIncludingThis; ++i)
            addVarArgChild(get(registerOffset + argumentToOperand(i)));
        setIntrinsicResult(
            usesResult, resultOperand,
            addToGraph(Node::VarArg, NewArray, OpInfo(0), OpInfo(0)));
        return true;
    }
    
    return false;
}

void ByteCodeParser::handleGetByOffset(
    int destinationOperand, SpeculatedType prediction, NodeIndex base, unsigned identifierNumber,
    PropertyOffset offset)
{
    NodeIndex propertyStorage;
    if (isInlineOffset(offset))
        propertyStorage = base;
    else
        propertyStorage = addToGraph(GetButterfly, base);
    set(destinationOperand,
        addToGraph(
            GetByOffset, OpInfo(m_graph.m_storageAccessData.size()), OpInfo(prediction),
            propertyStorage));
        
    StorageAccessData storageAccessData;
    storageAccessData.offset = indexRelativeToBase(offset);
    storageAccessData.identifierNumber = identifierNumber;
    m_graph.m_storageAccessData.append(storageAccessData);
}

void ByteCodeParser::handleGetById(
    int destinationOperand, SpeculatedType prediction, NodeIndex base, unsigned identifierNumber,
    const GetByIdStatus& getByIdStatus)
{
    if (!getByIdStatus.isSimple()
        || m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, BadCache)) {
        set(destinationOperand,
            addToGraph(
                getByIdStatus.makesCalls() ? GetByIdFlush : GetById,
                OpInfo(identifierNumber), OpInfo(prediction), base));
        return;
    }
    
    ASSERT(getByIdStatus.structureSet().size());
                
    // The implementation of GetByOffset does not know to terminate speculative
    // execution if it doesn't have a prediction, so we do it manually.
    if (prediction == SpecNone)
        addToGraph(ForceOSRExit);
    
    NodeIndex originalBaseForBaselineJIT = base;
                
    addToGraph(CheckStructure, OpInfo(m_graph.addStructureSet(getByIdStatus.structureSet())), base);
    
    if (!getByIdStatus.chain().isEmpty()) {
        Structure* currentStructure = getByIdStatus.structureSet().singletonStructure();
        JSObject* currentObject = 0;
        for (unsigned i = 0; i < getByIdStatus.chain().size(); ++i) {
            currentObject = asObject(currentStructure->prototypeForLookup(m_inlineStackTop->m_codeBlock));
            currentStructure = getByIdStatus.chain()[i];
            base = addStructureTransitionCheck(currentObject, currentStructure);
        }
    }
    
    // Unless we want bugs like https://bugs.webkit.org/show_bug.cgi?id=88783, we need to
    // ensure that the base of the original get_by_id is kept alive until we're done with
    // all of the speculations. We only insert the Phantom if there had been a CheckStructure
    // on something other than the base following the CheckStructure on base, or if the
    // access was compiled to a WeakJSConstant specific value, in which case we might not
    // have any explicit use of the base at all.
    if (getByIdStatus.specificValue() || originalBaseForBaselineJIT != base)
        addToGraph(Phantom, originalBaseForBaselineJIT);
    
    if (getByIdStatus.specificValue()) {
        ASSERT(getByIdStatus.specificValue().isCell());
        
        set(destinationOperand, cellConstant(getByIdStatus.specificValue().asCell()));
        return;
    }
    
    handleGetByOffset(
        destinationOperand, prediction, base, identifierNumber, getByIdStatus.offset());
}

void ByteCodeParser::prepareToParseBlock()
{
    for (unsigned i = 0; i < m_constants.size(); ++i)
        m_constants[i] = ConstantRecord();
    m_cellConstantNodes.clear();
}

bool ByteCodeParser::parseBlock(unsigned limit)
{
    bool shouldContinueParsing = true;
    
    Interpreter* interpreter = m_globalData->interpreter;
    Instruction* instructionsBegin = m_inlineStackTop->m_codeBlock->instructions().begin();
    unsigned blockBegin = m_currentIndex;
    
    // If we are the first basic block, introduce markers for arguments. This allows
    // us to track if a use of an argument may use the actual argument passed, as
    // opposed to using a value we set explicitly.
    if (m_currentBlock == m_graph.m_blocks[0].get() && !m_inlineStackTop->m_inlineCallFrame) {
        m_graph.m_arguments.resize(m_numArguments);
        for (unsigned argument = 0; argument < m_numArguments; ++argument) {
            VariableAccessData* variable = newVariableAccessData(
                argumentToOperand(argument), m_codeBlock->isCaptured(argumentToOperand(argument)));
            variable->mergeStructureCheckHoistingFailed(
                m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, BadCache));
            NodeIndex setArgument = addToGraph(SetArgument, OpInfo(variable));
            m_graph.m_arguments[argument] = setArgument;
            m_currentBlock->variablesAtHead.setArgumentFirstTime(argument, setArgument);
            m_currentBlock->variablesAtTail.setArgumentFirstTime(argument, setArgument);
        }
    }

    while (true) {
        m_currentProfilingIndex = m_currentIndex;

        // Don't extend over jump destinations.
        if (m_currentIndex == limit) {
            // Ordinarily we want to plant a jump. But refuse to do this if the block is
            // empty. This is a special case for inlining, which might otherwise create
            // some empty blocks in some cases. When parseBlock() returns with an empty
            // block, it will get repurposed instead of creating a new one. Note that this
            // logic relies on every bytecode resulting in one or more nodes, which would
            // be true anyway except for op_loop_hint, which emits a Phantom to force this
            // to be true.
            if (!m_currentBlock->isEmpty())
                addToGraph(Jump, OpInfo(m_currentIndex));
            else {
#if DFG_ENABLE(DEBUG_VERBOSE)
                dataLog("Refusing to plant jump at limit %u because block %p is empty.\n", limit, m_currentBlock);
#endif
            }
            return shouldContinueParsing;
        }
        
        // Switch on the current bytecode opcode.
        Instruction* currentInstruction = instructionsBegin + m_currentIndex;
        m_currentInstruction = currentInstruction; // Some methods want to use this, and we'd rather not thread it through calls.
        OpcodeID opcodeID = interpreter->getOpcodeID(currentInstruction->u.opcode);
        switch (opcodeID) {

        // === Function entry opcodes ===

        case op_enter:
            // Initialize all locals to undefined.
            for (int i = 0; i < m_inlineStackTop->m_codeBlock->m_numVars; ++i)
                set(i, constantUndefined(), SetOnEntry);
            NEXT_OPCODE(op_enter);

        case op_convert_this: {
            NodeIndex op1 = getThis();
            if (m_graph[op1].op() != ConvertThis) {
                ValueProfile* profile =
                    m_inlineStackTop->m_profiledBlock->valueProfileForBytecodeOffset(m_currentProfilingIndex);
                profile->computeUpdatedPrediction();
#if DFG_ENABLE(DEBUG_VERBOSE)
                dataLog("[@%lu bc#%u]: profile %p: ", m_graph.size(), m_currentProfilingIndex, profile);
                profile->dump(WTF::dataFile());
                dataLog("\n");
#endif
                if (profile->m_singletonValueIsTop
                    || !profile->m_singletonValue
                    || !profile->m_singletonValue.isCell()
                    || profile->m_singletonValue.asCell()->classInfo() != &Structure::s_info)
                    setThis(addToGraph(ConvertThis, op1));
                else {
                    addToGraph(
                        CheckStructure,
                        OpInfo(m_graph.addStructureSet(jsCast<Structure*>(profile->m_singletonValue.asCell()))),
                        op1);
                }
            }
            NEXT_OPCODE(op_convert_this);
        }

        case op_create_this: {
            set(currentInstruction[1].u.operand, addToGraph(CreateThis, get(RegisterFile::Callee)));
            NEXT_OPCODE(op_create_this);
        }
            
        case op_new_object: {
            set(currentInstruction[1].u.operand, addToGraph(NewObject));
            NEXT_OPCODE(op_new_object);
        }
            
        case op_new_array: {
            int startOperand = currentInstruction[2].u.operand;
            int numOperands = currentInstruction[3].u.operand;
            for (int operandIdx = startOperand; operandIdx < startOperand + numOperands; ++operandIdx)
                addVarArgChild(get(operandIdx));
            set(currentInstruction[1].u.operand, addToGraph(Node::VarArg, NewArray, OpInfo(0), OpInfo(0)));
            NEXT_OPCODE(op_new_array);
        }
            
        case op_new_array_buffer: {
            int startConstant = currentInstruction[2].u.operand;
            int numConstants = currentInstruction[3].u.operand;
            set(currentInstruction[1].u.operand, addToGraph(NewArrayBuffer, OpInfo(startConstant), OpInfo(numConstants)));
            NEXT_OPCODE(op_new_array_buffer);
        }
            
        case op_new_regexp: {
            set(currentInstruction[1].u.operand, addToGraph(NewRegexp, OpInfo(currentInstruction[2].u.operand)));
            NEXT_OPCODE(op_new_regexp);
        }
            
        // === Bitwise operations ===

        case op_bitand: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(BitAnd, op1, op2));
            NEXT_OPCODE(op_bitand);
        }

        case op_bitor: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(BitOr, op1, op2));
            NEXT_OPCODE(op_bitor);
        }

        case op_bitxor: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(BitXor, op1, op2));
            NEXT_OPCODE(op_bitxor);
        }

        case op_rshift: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            NodeIndex result;
            // Optimize out shifts by zero.
            if (isInt32Constant(op2) && !(valueOfInt32Constant(op2) & 0x1f))
                result = op1;
            else
                result = addToGraph(BitRShift, op1, op2);
            set(currentInstruction[1].u.operand, result);
            NEXT_OPCODE(op_rshift);
        }

        case op_lshift: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            NodeIndex result;
            // Optimize out shifts by zero.
            if (isInt32Constant(op2) && !(valueOfInt32Constant(op2) & 0x1f))
                result = op1;
            else
                result = addToGraph(BitLShift, op1, op2);
            set(currentInstruction[1].u.operand, result);
            NEXT_OPCODE(op_lshift);
        }

        case op_urshift: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            NodeIndex result;
            // The result of a zero-extending right shift is treated as an unsigned value.
            // This means that if the top bit is set, the result is not in the int32 range,
            // and as such must be stored as a double. If the shift amount is a constant,
            // we may be able to optimize.
            if (isInt32Constant(op2)) {
                // If we know we are shifting by a non-zero amount, then since the operation
                // zero fills we know the top bit of the result must be zero, and as such the
                // result must be within the int32 range. Conversely, if this is a shift by
                // zero, then the result may be changed by the conversion to unsigned, but it
                // is not necessary to perform the shift!
                if (valueOfInt32Constant(op2) & 0x1f)
                    result = addToGraph(BitURShift, op1, op2);
                else
                    result = makeSafe(addToGraph(UInt32ToNumber, op1));
            }  else {
                // Cannot optimize at this stage; shift & potentially rebox as a double.
                result = addToGraph(BitURShift, op1, op2);
                result = makeSafe(addToGraph(UInt32ToNumber, result));
            }
            set(currentInstruction[1].u.operand, result);
            NEXT_OPCODE(op_urshift);
        }

        // === Increment/Decrement opcodes ===

        case op_pre_inc: {
            unsigned srcDst = currentInstruction[1].u.operand;
            NodeIndex op = get(srcDst);
            set(srcDst, makeSafe(addToGraph(ArithAdd, op, one())));
            NEXT_OPCODE(op_pre_inc);
        }

        case op_post_inc: {
            unsigned result = currentInstruction[1].u.operand;
            unsigned srcDst = currentInstruction[2].u.operand;
            ASSERT(result != srcDst); // Required for assumptions we make during OSR.
            NodeIndex op = get(srcDst);
            set(result, op);
            set(srcDst, makeSafe(addToGraph(ArithAdd, op, one())));
            NEXT_OPCODE(op_post_inc);
        }

        case op_pre_dec: {
            unsigned srcDst = currentInstruction[1].u.operand;
            NodeIndex op = get(srcDst);
            set(srcDst, makeSafe(addToGraph(ArithSub, op, one())));
            NEXT_OPCODE(op_pre_dec);
        }

        case op_post_dec: {
            unsigned result = currentInstruction[1].u.operand;
            unsigned srcDst = currentInstruction[2].u.operand;
            NodeIndex op = get(srcDst);
            set(result, op);
            set(srcDst, makeSafe(addToGraph(ArithSub, op, one())));
            NEXT_OPCODE(op_post_dec);
        }

        // === Arithmetic operations ===

        case op_add: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            if (m_graph[op1].hasNumberResult() && m_graph[op2].hasNumberResult())
                set(currentInstruction[1].u.operand, makeSafe(addToGraph(ArithAdd, op1, op2)));
            else
                set(currentInstruction[1].u.operand, makeSafe(addToGraph(ValueAdd, op1, op2)));
            NEXT_OPCODE(op_add);
        }

        case op_sub: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, makeSafe(addToGraph(ArithSub, op1, op2)));
            NEXT_OPCODE(op_sub);
        }

        case op_negate: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, makeSafe(addToGraph(ArithNegate, op1)));
            NEXT_OPCODE(op_negate);
        }

        case op_mul: {
            // Multiply requires that the inputs are not truncated, unfortunately.
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, makeSafe(addToGraph(ArithMul, op1, op2)));
            NEXT_OPCODE(op_mul);
        }

        case op_mod: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, makeSafe(addToGraph(ArithMod, op1, op2)));
            NEXT_OPCODE(op_mod);
        }

        case op_div: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, makeDivSafe(addToGraph(ArithDiv, op1, op2)));
            NEXT_OPCODE(op_div);
        }

        // === Misc operations ===

#if ENABLE(DEBUG_WITH_BREAKPOINT)
        case op_debug:
            addToGraph(Breakpoint);
            NEXT_OPCODE(op_debug);
#endif
        case op_mov: {
            NodeIndex op = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, op);
            NEXT_OPCODE(op_mov);
        }

        case op_check_has_instance:
            addToGraph(CheckHasInstance, get(currentInstruction[1].u.operand));
            NEXT_OPCODE(op_check_has_instance);

        case op_instanceof: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            NodeIndex baseValue = get(currentInstruction[3].u.operand);
            NodeIndex prototype = get(currentInstruction[4].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(InstanceOf, value, baseValue, prototype));
            NEXT_OPCODE(op_instanceof);
        }
            
        case op_is_undefined: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(IsUndefined, value));
            NEXT_OPCODE(op_is_undefined);
        }

        case op_is_boolean: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(IsBoolean, value));
            NEXT_OPCODE(op_is_boolean);
        }

        case op_is_number: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(IsNumber, value));
            NEXT_OPCODE(op_is_number);
        }

        case op_is_string: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(IsString, value));
            NEXT_OPCODE(op_is_string);
        }

        case op_is_object: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(IsObject, value));
            NEXT_OPCODE(op_is_object);
        }

        case op_is_function: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(IsFunction, value));
            NEXT_OPCODE(op_is_function);
        }

        case op_not: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, value));
            NEXT_OPCODE(op_not);
        }
            
        case op_to_primitive: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(ToPrimitive, value));
            NEXT_OPCODE(op_to_primitive);
        }
            
        case op_strcat: {
            int startOperand = currentInstruction[2].u.operand;
            int numOperands = currentInstruction[3].u.operand;
            for (int operandIdx = startOperand; operandIdx < startOperand + numOperands; ++operandIdx)
                addVarArgChild(get(operandIdx));
            set(currentInstruction[1].u.operand, addToGraph(Node::VarArg, StrCat, OpInfo(0), OpInfo(0)));
            NEXT_OPCODE(op_strcat);
        }

        case op_less: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareLess, op1, op2));
            NEXT_OPCODE(op_less);
        }

        case op_lesseq: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareLessEq, op1, op2));
            NEXT_OPCODE(op_lesseq);
        }

        case op_greater: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareGreater, op1, op2));
            NEXT_OPCODE(op_greater);
        }

        case op_greatereq: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareGreaterEq, op1, op2));
            NEXT_OPCODE(op_greatereq);
        }

        case op_eq: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareEq, op1, op2));
            NEXT_OPCODE(op_eq);
        }

        case op_eq_null: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareEq, value, constantNull()));
            NEXT_OPCODE(op_eq_null);
        }

        case op_stricteq: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareStrictEq, op1, op2));
            NEXT_OPCODE(op_stricteq);
        }

        case op_neq: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, addToGraph(CompareEq, op1, op2)));
            NEXT_OPCODE(op_neq);
        }

        case op_neq_null: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, addToGraph(CompareEq, value, constantNull())));
            NEXT_OPCODE(op_neq_null);
        }

        case op_nstricteq: {
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, addToGraph(CompareStrictEq, op1, op2)));
            NEXT_OPCODE(op_nstricteq);
        }

        // === Property access operations ===

        case op_get_by_val: {
            SpeculatedType prediction = getPrediction();
            
            NodeIndex base = get(currentInstruction[2].u.operand);
            Array::Mode arrayMode = getArrayModeAndEmitChecks(currentInstruction[4].u.arrayProfile, Array::Read, base);
            NodeIndex property = get(currentInstruction[3].u.operand);
            NodeIndex getByVal = addToGraph(GetByVal, OpInfo(arrayMode), OpInfo(prediction), base, property);
            set(currentInstruction[1].u.operand, getByVal);

            NEXT_OPCODE(op_get_by_val);
        }

        case op_put_by_val: {
            NodeIndex base = get(currentInstruction[1].u.operand);

            Array::Mode arrayMode = getArrayModeAndEmitChecks(currentInstruction[4].u.arrayProfile, Array::Write, base);
            
            NodeIndex property = get(currentInstruction[2].u.operand);
            NodeIndex value = get(currentInstruction[3].u.operand);
            
            addVarArgChild(base);
            addVarArgChild(property);
            addVarArgChild(value);
            addVarArgChild(NoNode); // Leave room for property storage.
            addToGraph(Node::VarArg, PutByVal, OpInfo(arrayMode), OpInfo(0));

            NEXT_OPCODE(op_put_by_val);
        }
            
        case op_method_check: {
            m_currentProfilingIndex += OPCODE_LENGTH(op_method_check);
            Instruction* getInstruction = currentInstruction + OPCODE_LENGTH(op_method_check);
            
            SpeculatedType prediction = getPrediction();
            
            ASSERT(interpreter->getOpcodeID(getInstruction->u.opcode) == op_get_by_id
                   || interpreter->getOpcodeID(getInstruction->u.opcode) == op_get_by_id_out_of_line);
            
            NodeIndex base = get(getInstruction[2].u.operand);
            unsigned identifier = m_inlineStackTop->m_identifierRemap[getInstruction[3].u.operand];
                
            // Check if the method_check was monomorphic. If so, emit a CheckXYZMethod
            // node, which is a lot more efficient.
            GetByIdStatus getByIdStatus = GetByIdStatus::computeFor(
                m_inlineStackTop->m_profiledBlock,
                m_currentIndex,
                m_codeBlock->identifier(identifier));
            MethodCallLinkStatus methodCallStatus = MethodCallLinkStatus::computeFor(
                m_inlineStackTop->m_profiledBlock, m_currentIndex);
            
            if (methodCallStatus.isSet()
                && !getByIdStatus.wasSeenInJIT()
                && !m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, BadCache)) {
                // It's monomorphic as far as we can tell, since the method_check was linked
                // but the slow path (i.e. the normal get_by_id) never fired.

                addToGraph(CheckStructure, OpInfo(m_graph.addStructureSet(methodCallStatus.structure())), base);
                if (methodCallStatus.needsPrototypeCheck()) {
                    addStructureTransitionCheck(
                        methodCallStatus.prototype(), methodCallStatus.prototypeStructure());
                    addToGraph(Phantom, base);
                }
                set(getInstruction[1].u.operand, cellConstant(methodCallStatus.function()));
            } else {
                handleGetById(
                    getInstruction[1].u.operand, prediction, base, identifier, getByIdStatus);
            }
            
            m_currentIndex += OPCODE_LENGTH(op_method_check) + OPCODE_LENGTH(op_get_by_id);
            continue;
        }
        case op_get_scoped_var: {
            SpeculatedType prediction = getPrediction();
            int dst = currentInstruction[1].u.operand;
            int slot = currentInstruction[2].u.operand;
            int depth = currentInstruction[3].u.operand;
            NodeIndex getScopeChain = addToGraph(GetScopeChain, OpInfo(depth));
            NodeIndex getScopedVar = addToGraph(GetScopedVar, OpInfo(slot), OpInfo(prediction), getScopeChain);
            set(dst, getScopedVar);
            NEXT_OPCODE(op_get_scoped_var);
        }
        case op_put_scoped_var: {
            int slot = currentInstruction[1].u.operand;
            int depth = currentInstruction[2].u.operand;
            int source = currentInstruction[3].u.operand;
            NodeIndex getScopeChain = addToGraph(GetScopeChain, OpInfo(depth));
            addToGraph(PutScopedVar, OpInfo(slot), getScopeChain, get(source));
            NEXT_OPCODE(op_put_scoped_var);
        }
        case op_get_by_id:
        case op_get_by_id_out_of_line:
        case op_get_array_length: {
            SpeculatedType prediction = getPrediction();
            
            NodeIndex base = get(currentInstruction[2].u.operand);
            unsigned identifierNumber = m_inlineStackTop->m_identifierRemap[currentInstruction[3].u.operand];
            
            Identifier identifier = m_codeBlock->identifier(identifierNumber);
            GetByIdStatus getByIdStatus = GetByIdStatus::computeFor(
                m_inlineStackTop->m_profiledBlock, m_currentIndex, identifier);
            
            handleGetById(
                currentInstruction[1].u.operand, prediction, base, identifierNumber, getByIdStatus);

            NEXT_OPCODE(op_get_by_id);
        }
        case op_put_by_id:
        case op_put_by_id_out_of_line:
        case op_put_by_id_transition_direct:
        case op_put_by_id_transition_normal:
        case op_put_by_id_transition_direct_out_of_line:
        case op_put_by_id_transition_normal_out_of_line: {
            NodeIndex value = get(currentInstruction[3].u.operand);
            NodeIndex base = get(currentInstruction[1].u.operand);
            unsigned identifierNumber = m_inlineStackTop->m_identifierRemap[currentInstruction[2].u.operand];
            bool direct = currentInstruction[8].u.operand;

            PutByIdStatus putByIdStatus = PutByIdStatus::computeFor(
                m_inlineStackTop->m_profiledBlock,
                m_currentIndex,
                m_codeBlock->identifier(identifierNumber));
            if (!putByIdStatus.isSet())
                addToGraph(ForceOSRExit);
            
            bool hasExitSite = m_inlineStackTop->m_exitProfile.hasExitSite(m_currentIndex, BadCache);
            
            if (!hasExitSite && putByIdStatus.isSimpleReplace()) {
                addToGraph(CheckStructure, OpInfo(m_graph.addStructureSet(putByIdStatus.oldStructure())), base);
                NodeIndex propertyStorage;
                if (isInlineOffset(putByIdStatus.offset()))
                    propertyStorage = base;
                else
                    propertyStorage = addToGraph(GetButterfly, base);
                addToGraph(PutByOffset, OpInfo(m_graph.m_storageAccessData.size()), propertyStorage, base, value);
                
                StorageAccessData storageAccessData;
                storageAccessData.offset = indexRelativeToBase(putByIdStatus.offset());
                storageAccessData.identifierNumber = identifierNumber;
                m_graph.m_storageAccessData.append(storageAccessData);
            } else if (!hasExitSite
                       && putByIdStatus.isSimpleTransition()
                       && structureChainIsStillValid(
                           direct,
                           putByIdStatus.oldStructure(),
                           putByIdStatus.structureChain())) {

                addToGraph(CheckStructure, OpInfo(m_graph.addStructureSet(putByIdStatus.oldStructure())), base);
                if (!direct) {
                    if (!putByIdStatus.oldStructure()->storedPrototype().isNull()) {
                        addStructureTransitionCheck(
                            putByIdStatus.oldStructure()->storedPrototype().asCell());
                    }
                    
                    for (WriteBarrier<Structure>* it = putByIdStatus.structureChain()->head(); *it; ++it) {
                        JSValue prototype = (*it)->storedPrototype();
                        if (prototype.isNull())
                            continue;
                        ASSERT(prototype.isCell());
                        addStructureTransitionCheck(prototype.asCell());
                    }
                }
                ASSERT(putByIdStatus.oldStructure()->transitionWatchpointSetHasBeenInvalidated());
                
                NodeIndex propertyStorage;
                StructureTransitionData* transitionData =
                    m_graph.addStructureTransitionData(
                        StructureTransitionData(
                            putByIdStatus.oldStructure(),
                            putByIdStatus.newStructure()));

                if (putByIdStatus.oldStructure()->outOfLineCapacity()
                    != putByIdStatus.newStructure()->outOfLineCapacity()) {
                    
                    // If we're growing the property storage then it must be because we're
                    // storing into the out-of-line storage.
                    ASSERT(!isInlineOffset(putByIdStatus.offset()));
                    
                    if (!putByIdStatus.oldStructure()->outOfLineCapacity()) {
                        propertyStorage = addToGraph(
                            AllocatePropertyStorage, OpInfo(transitionData), base);
                    } else {
                        propertyStorage = addToGraph(
                            ReallocatePropertyStorage, OpInfo(transitionData),
                            base, addToGraph(GetButterfly, base));
                    }
                } else {
                    if (isInlineOffset(putByIdStatus.offset()))
                        propertyStorage = base;
                    else
                        propertyStorage = addToGraph(GetButterfly, base);
                }
                
                addToGraph(PutStructure, OpInfo(transitionData), base);
                
                addToGraph(
                    PutByOffset,
                    OpInfo(m_graph.m_storageAccessData.size()),
                    propertyStorage,
                    base,
                    value);
                
                StorageAccessData storageAccessData;
                storageAccessData.offset = indexRelativeToBase(putByIdStatus.offset());
                storageAccessData.identifierNumber = identifierNumber;
                m_graph.m_storageAccessData.append(storageAccessData);
            } else {
                if (direct)
                    addToGraph(PutByIdDirect, OpInfo(identifierNumber), base, value);
                else
                    addToGraph(PutById, OpInfo(identifierNumber), base, value);
            }

            NEXT_OPCODE(op_put_by_id);
        }

        case op_get_global_var: {
            SpeculatedType prediction = getPrediction();
            
            JSGlobalObject* globalObject = m_inlineStackTop->m_codeBlock->globalObject();

            NodeIndex getGlobalVar = addToGraph(
                GetGlobalVar,
                OpInfo(globalObject->assertRegisterIsInThisObject(currentInstruction[2].u.registerPointer)),
                OpInfo(prediction));
            set(currentInstruction[1].u.operand, getGlobalVar);
            NEXT_OPCODE(op_get_global_var);
        }
                    
        case op_get_global_var_watchable: {
            SpeculatedType prediction = getPrediction();
            
            JSGlobalObject* globalObject = m_inlineStackTop->m_codeBlock->globalObject();
            
            unsigned identifierNumber = m_inlineStackTop->m_identifierRemap[currentInstruction[3].u.operand];
            Identifier identifier = m_codeBlock->identifier(identifierNumber);
            SymbolTableEntry entry = globalObject->symbolTable()->get(identifier.impl());
            if (!entry.couldBeWatched()) {
                NodeIndex getGlobalVar = addToGraph(
                    GetGlobalVar,
                    OpInfo(globalObject->assertRegisterIsInThisObject(currentInstruction[2].u.registerPointer)),
                    OpInfo(prediction));
                set(currentInstruction[1].u.operand, getGlobalVar);
                NEXT_OPCODE(op_get_global_var_watchable);
            }
            
            // The watchpoint is still intact! This means that we will get notified if the
            // current value in the global variable changes. So, we can inline that value.
            // Moreover, currently we can assume that this value is a JSFunction*, which
            // implies that it's a cell. This simplifies things, since in general we'd have
            // to use a JSConstant for non-cells and a WeakJSConstant for cells. So instead
            // of having both cases we just assert that the value is a cell.
            
            // NB. If it wasn't for CSE, GlobalVarWatchpoint would have no need for the
            // register pointer. But CSE tracks effects on global variables by comparing
            // register pointers. Because CSE executes multiple times while the backend
            // executes once, we use the following performance trade-off:
            // - The node refers directly to the register pointer to make CSE super cheap.
            // - To perform backend code generation, the node only contains the identifier
            //   number, from which it is possible to get (via a few average-time O(1)
            //   lookups) to the WatchpointSet.
            
            addToGraph(
                GlobalVarWatchpoint,
                OpInfo(globalObject->assertRegisterIsInThisObject(currentInstruction[2].u.registerPointer)),
                OpInfo(identifierNumber));
            
            JSValue specificValue = globalObject->registerAt(entry.getIndex()).get();
            ASSERT(specificValue.isCell());
            set(currentInstruction[1].u.operand, cellConstant(specificValue.asCell()));
            
            NEXT_OPCODE(op_get_global_var_watchable);
        }

        case op_put_global_var:
        case op_init_global_const: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            addToGraph(
                PutGlobalVar,
                OpInfo(m_inlineStackTop->m_codeBlock->globalObject()->assertRegisterIsInThisObject(currentInstruction[1].u.registerPointer)),
                value);
            NEXT_OPCODE(op_put_global_var);
        }

        case op_put_global_var_check:
        case op_init_global_const_check: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            CodeBlock* codeBlock = m_inlineStackTop->m_codeBlock;
            JSGlobalObject* globalObject = codeBlock->globalObject();
            unsigned identifierNumber = m_inlineStackTop->m_identifierRemap[currentInstruction[4].u.operand];
            Identifier identifier = m_codeBlock->identifier(identifierNumber);
            SymbolTableEntry entry = globalObject->symbolTable()->get(identifier.impl());
            if (!entry.couldBeWatched()) {
                addToGraph(
                    PutGlobalVar,
                    OpInfo(globalObject->assertRegisterIsInThisObject(currentInstruction[1].u.registerPointer)),
                    value);
                NEXT_OPCODE(op_put_global_var_check);
            }
            addToGraph(
                PutGlobalVarCheck,
                OpInfo(codeBlock->globalObject()->assertRegisterIsInThisObject(currentInstruction[1].u.registerPointer)),
                OpInfo(identifierNumber),
                value);
            NEXT_OPCODE(op_put_global_var_check);
        }

        // === Block terminators. ===

        case op_jmp: {
            unsigned relativeOffset = currentInstruction[1].u.operand;
            addToGraph(Jump, OpInfo(m_currentIndex + relativeOffset));
            LAST_OPCODE(op_jmp);
        }

        case op_loop: {
            unsigned relativeOffset = currentInstruction[1].u.operand;
            addToGraph(Jump, OpInfo(m_currentIndex + relativeOffset));
            LAST_OPCODE(op_loop);
        }

        case op_jtrue: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jtrue)), condition);
            LAST_OPCODE(op_jtrue);
        }

        case op_jfalse: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jfalse)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jfalse);
        }

        case op_loop_if_true: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_true)), condition);
            LAST_OPCODE(op_loop_if_true);
        }

        case op_loop_if_false: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_false)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_loop_if_false);
        }

        case op_jeq_null: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex value = get(currentInstruction[1].u.operand);
            NodeIndex condition = addToGraph(CompareEq, value, constantNull());
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jeq_null)), condition);
            LAST_OPCODE(op_jeq_null);
        }

        case op_jneq_null: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex value = get(currentInstruction[1].u.operand);
            NodeIndex condition = addToGraph(CompareEq, value, constantNull());
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jneq_null)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jneq_null);
        }

        case op_jless: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLess, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jless)), condition);
            LAST_OPCODE(op_jless);
        }

        case op_jlesseq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLessEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jlesseq)), condition);
            LAST_OPCODE(op_jlesseq);
        }

        case op_jgreater: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareGreater, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jgreater)), condition);
            LAST_OPCODE(op_jgreater);
        }

        case op_jgreatereq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareGreaterEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jgreatereq)), condition);
            LAST_OPCODE(op_jgreatereq);
        }

        case op_jnless: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLess, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jnless)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jnless);
        }

        case op_jnlesseq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLessEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jnlesseq)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jnlesseq);
        }

        case op_jngreater: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareGreater, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jngreater)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jngreater);
        }

        case op_jngreatereq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareGreaterEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jngreatereq)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jngreatereq);
        }

        case op_loop_if_less: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLess, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_less)), condition);
            LAST_OPCODE(op_loop_if_less);
        }

        case op_loop_if_lesseq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLessEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_lesseq)), condition);
            LAST_OPCODE(op_loop_if_lesseq);
        }

        case op_loop_if_greater: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareGreater, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_greater)), condition);
            LAST_OPCODE(op_loop_if_greater);
        }

        case op_loop_if_greatereq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareGreaterEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_greatereq)), condition);
            LAST_OPCODE(op_loop_if_greatereq);
        }

        case op_ret:
            flushArgumentsAndCapturedVariables();
            if (m_inlineStackTop->m_inlineCallFrame) {
                if (m_inlineStackTop->m_returnValue != InvalidVirtualRegister)
                    setDirect(m_inlineStackTop->m_returnValue, get(currentInstruction[1].u.operand));
                m_inlineStackTop->m_didReturn = true;
                if (m_inlineStackTop->m_unlinkedBlocks.isEmpty()) {
                    // If we're returning from the first block, then we're done parsing.
                    ASSERT(m_inlineStackTop->m_callsiteBlockHead == m_graph.m_blocks.size() - 1);
                    shouldContinueParsing = false;
                    LAST_OPCODE(op_ret);
                } else {
                    // If inlining created blocks, and we're doing a return, then we need some
                    // special linking.
                    ASSERT(m_inlineStackTop->m_unlinkedBlocks.last().m_blockIndex == m_graph.m_blocks.size() - 1);
                    m_inlineStackTop->m_unlinkedBlocks.last().m_needsNormalLinking = false;
                }
                if (m_currentIndex + OPCODE_LENGTH(op_ret) != m_inlineStackTop->m_codeBlock->instructions().size() || m_inlineStackTop->m_didEarlyReturn) {
                    ASSERT(m_currentIndex + OPCODE_LENGTH(op_ret) <= m_inlineStackTop->m_codeBlock->instructions().size());
                    addToGraph(Jump, OpInfo(NoBlock));
                    m_inlineStackTop->m_unlinkedBlocks.last().m_needsEarlyReturnLinking = true;
                    m_inlineStackTop->m_didEarlyReturn = true;
                }
                LAST_OPCODE(op_ret);
            }
            addToGraph(Return, get(currentInstruction[1].u.operand));
            LAST_OPCODE(op_ret);
            
        case op_end:
            flushArgumentsAndCapturedVariables();
            ASSERT(!m_inlineStackTop->m_inlineCallFrame);
            addToGraph(Return, get(currentInstruction[1].u.operand));
            LAST_OPCODE(op_end);

        case op_throw:
            flushArgumentsAndCapturedVariables();
            addToGraph(Throw, get(currentInstruction[1].u.operand));
            LAST_OPCODE(op_throw);
            
        case op_throw_reference_error:
            flushArgumentsAndCapturedVariables();
            addToGraph(ThrowReferenceError);
            LAST_OPCODE(op_throw_reference_error);
            
        case op_call:
            handleCall(interpreter, currentInstruction, Call, CodeForCall);
            NEXT_OPCODE(op_call);
            
        case op_construct:
            handleCall(interpreter, currentInstruction, Construct, CodeForConstruct);
            NEXT_OPCODE(op_construct);
            
        case op_call_varargs: {
            ASSERT(m_inlineStackTop->m_inlineCallFrame);
            ASSERT(currentInstruction[3].u.operand == m_inlineStackTop->m_codeBlock->argumentsRegister());
            // It would be cool to funnel this into handleCall() so that it can handle
            // inlining. But currently that won't be profitable anyway, since none of the
            // uses of call_varargs will be inlineable. So we set this up manually and
            // without inline/intrinsic detection.
            
            Instruction* putInstruction = currentInstruction + OPCODE_LENGTH(op_call_varargs);
            
            SpeculatedType prediction = SpecNone;
            if (interpreter->getOpcodeID(putInstruction->u.opcode) == op_call_put_result) {
                m_currentProfilingIndex = m_currentIndex + OPCODE_LENGTH(op_call_varargs);
                prediction = getPrediction();
            }
            
            addToGraph(CheckArgumentsNotCreated);
            
            unsigned argCount = m_inlineStackTop->m_inlineCallFrame->arguments.size();
            if (RegisterFile::CallFrameHeaderSize + argCount > m_parameterSlots)
                m_parameterSlots = RegisterFile::CallFrameHeaderSize + argCount;
            
            addVarArgChild(get(currentInstruction[1].u.operand)); // callee
            addVarArgChild(get(currentInstruction[2].u.operand)); // this
            for (unsigned argument = 1; argument < argCount; ++argument)
                addVarArgChild(get(argumentToOperand(argument)));
            
            NodeIndex call = addToGraph(Node::VarArg, Call, OpInfo(0), OpInfo(prediction));
            if (interpreter->getOpcodeID(putInstruction->u.opcode) == op_call_put_result)
                set(putInstruction[1].u.operand, call);
            
            NEXT_OPCODE(op_call_varargs);
        }
            
        case op_call_put_result:
            NEXT_OPCODE(op_call_put_result);
            
        case op_jneq_ptr:
            // Statically speculate for now. It makes sense to let speculate-only jneq_ptr
            // support simmer for a while before making it more general, since it's
            // already gnarly enough as it is.
            addToGraph(
                CheckFunction, OpInfo(currentInstruction[2].u.jsCell.get()),
                get(currentInstruction[1].u.operand));
            addToGraph(Jump, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jneq_ptr)));
            LAST_OPCODE(op_jneq_ptr);

        case op_resolve: {
            SpeculatedType prediction = getPrediction();
            
            unsigned identifier = m_inlineStackTop->m_identifierRemap[currentInstruction[2].u.operand];

            NodeIndex resolve = addToGraph(Resolve, OpInfo(identifier), OpInfo(prediction));
            set(currentInstruction[1].u.operand, resolve);

            NEXT_OPCODE(op_resolve);
        }

        case op_resolve_base: {
            SpeculatedType prediction = getPrediction();
            
            unsigned identifier = m_inlineStackTop->m_identifierRemap[currentInstruction[2].u.operand];

            NodeIndex resolve = addToGraph(currentInstruction[3].u.operand ? ResolveBaseStrictPut : ResolveBase, OpInfo(identifier), OpInfo(prediction));
            set(currentInstruction[1].u.operand, resolve);

            NEXT_OPCODE(op_resolve_base);
        }
            
        case op_resolve_global: {
            SpeculatedType prediction = getPrediction();
            
            unsigned identifierNumber = m_inlineStackTop->m_identifierRemap[
                currentInstruction[2].u.operand];
            
            ResolveGlobalStatus status = ResolveGlobalStatus::computeFor(
                m_inlineStackTop->m_profiledBlock, m_currentIndex,
                m_codeBlock->identifier(identifierNumber));
            if (status.isSimple()) {
                ASSERT(status.structure());
                
                NodeIndex globalObject = addStructureTransitionCheck(
                    m_inlineStackTop->m_codeBlock->globalObject(), status.structure());
                
                if (status.specificValue()) {
                    ASSERT(status.specificValue().isCell());
                    
                    set(currentInstruction[1].u.operand,
                        cellConstant(status.specificValue().asCell()));
                } else {
                    handleGetByOffset(
                        currentInstruction[1].u.operand, prediction, globalObject,
                        identifierNumber, status.offset());
                }
                
                m_globalResolveNumber++; // Skip over the unused global resolve info.
                
                NEXT_OPCODE(op_resolve_global);
            }
            
            NodeIndex resolve = addToGraph(ResolveGlobal, OpInfo(m_graph.m_resolveGlobalData.size()), OpInfo(prediction));
            m_graph.m_resolveGlobalData.append(ResolveGlobalData());
            ResolveGlobalData& data = m_graph.m_resolveGlobalData.last();
            data.identifierNumber = identifierNumber;
            data.resolveInfoIndex = m_globalResolveNumber++;
            set(currentInstruction[1].u.operand, resolve);

            NEXT_OPCODE(op_resolve_global);
        }

        case op_loop_hint: {
            // Baseline->DFG OSR jumps between loop hints. The DFG assumes that Baseline->DFG
            // OSR can only happen at basic block boundaries. Assert that these two statements
            // are compatible.
            ASSERT_UNUSED(blockBegin, m_currentIndex == blockBegin);
            
            // We never do OSR into an inlined code block. That could not happen, since OSR
            // looks up the code block that is the replacement for the baseline JIT code
            // block. Hence, machine code block = true code block = not inline code block.
            if (!m_inlineStackTop->m_caller)
                m_currentBlock->isOSRTarget = true;
            
            // Emit a phantom node to ensure that there is a placeholder node for this bytecode
            // op.
            addToGraph(Phantom);
            
            NEXT_OPCODE(op_loop_hint);
        }
            
        case op_init_lazy_reg: {
            set(currentInstruction[1].u.operand, getJSConstantForValue(JSValue()));
            NEXT_OPCODE(op_init_lazy_reg);
        }
            
        case op_create_activation: {
            set(currentInstruction[1].u.operand, addToGraph(CreateActivation, get(currentInstruction[1].u.operand)));
            NEXT_OPCODE(op_create_activation);
        }
            
        case op_create_arguments: {
            m_graph.m_hasArguments = true;
            NodeIndex createArguments = addToGraph(CreateArguments, get(currentInstruction[1].u.operand));
            set(currentInstruction[1].u.operand, createArguments);
            set(unmodifiedArgumentsRegister(currentInstruction[1].u.operand), createArguments);
            NEXT_OPCODE(op_create_arguments);
        }
            
        case op_tear_off_activation: {
            addToGraph(TearOffActivation, get(currentInstruction[1].u.operand));
            NEXT_OPCODE(op_tear_off_activation);
        }

        case op_tear_off_arguments: {
            m_graph.m_hasArguments = true;
            addToGraph(TearOffArguments, get(unmodifiedArgumentsRegister(currentInstruction[1].u.operand)), get(currentInstruction[2].u.operand));
            NEXT_OPCODE(op_tear_off_arguments);
        }
            
        case op_get_arguments_length: {
            m_graph.m_hasArguments = true;
            set(currentInstruction[1].u.operand, addToGraph(GetMyArgumentsLengthSafe));
            NEXT_OPCODE(op_get_arguments_length);
        }
            
        case op_get_argument_by_val: {
            m_graph.m_hasArguments = true;
            set(currentInstruction[1].u.operand,
                addToGraph(
                    GetMyArgumentByValSafe, OpInfo(0), OpInfo(getPrediction()),
                    get(currentInstruction[3].u.operand)));
            NEXT_OPCODE(op_get_argument_by_val);
        }
            
        case op_new_func: {
            if (!currentInstruction[3].u.operand) {
                set(currentInstruction[1].u.operand,
                    addToGraph(NewFunctionNoCheck, OpInfo(currentInstruction[2].u.operand)));
            } else {
                set(currentInstruction[1].u.operand,
                    addToGraph(
                        NewFunction,
                        OpInfo(currentInstruction[2].u.operand),
                        get(currentInstruction[1].u.operand)));
            }
            NEXT_OPCODE(op_new_func);
        }
            
        case op_new_func_exp: {
            set(currentInstruction[1].u.operand,
                addToGraph(NewFunctionExpression, OpInfo(currentInstruction[2].u.operand)));
            NEXT_OPCODE(op_new_func_exp);
        }

        default:
            // Parse failed! This should not happen because the capabilities checker
            // should have caught it.
            ASSERT_NOT_REACHED();
            return false;
        }
    }
}

template<ByteCodeParser::PhiStackType stackType>
void ByteCodeParser::processPhiStack()
{
    Vector<PhiStackEntry, 16>& phiStack = (stackType == ArgumentPhiStack) ? m_argumentPhiStack : m_localPhiStack;
    
    while (!phiStack.isEmpty()) {
        PhiStackEntry entry = phiStack.last();
        phiStack.removeLast();
        
        if (!entry.m_block->isReachable)
            continue;
        
        if (!entry.m_block->isReachable)
            continue;
        
        PredecessorList& predecessors = entry.m_block->m_predecessors;
        unsigned varNo = entry.m_varNo;
        VariableAccessData* dataForPhi = m_graph[entry.m_phi].variableAccessData();

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("   Handling phi entry for var %u, phi @%u.\n", entry.m_varNo, entry.m_phi);
#endif
        
        for (size_t i = 0; i < predecessors.size(); ++i) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("     Dealing with predecessor block %u.\n", predecessors[i]);
#endif
            
            BasicBlock* predecessorBlock = m_graph.m_blocks[predecessors[i]].get();

            NodeIndex& var = (stackType == ArgumentPhiStack) ? predecessorBlock->variablesAtTail.argument(varNo) : predecessorBlock->variablesAtTail.local(varNo);
            
            NodeIndex valueInPredecessor = var;
            if (valueInPredecessor == NoNode) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Did not find node, adding phi.\n");
#endif

                valueInPredecessor = insertPhiNode(OpInfo(newVariableAccessData(stackType == ArgumentPhiStack ? argumentToOperand(varNo) : static_cast<int>(varNo), false)), predecessorBlock);
                var = valueInPredecessor;
                if (stackType == ArgumentPhiStack)
                    predecessorBlock->variablesAtHead.setArgumentFirstTime(varNo, valueInPredecessor);
                else
                    predecessorBlock->variablesAtHead.setLocalFirstTime(varNo, valueInPredecessor);
                phiStack.append(PhiStackEntry(predecessorBlock, valueInPredecessor, varNo));
            } else if (m_graph[valueInPredecessor].op() == GetLocal) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Found GetLocal @%u.\n", valueInPredecessor);
#endif

                // We want to ensure that the VariableAccessDatas are identical between the
                // GetLocal and its block-local Phi. Strictly speaking we only need the two
                // to be unified. But for efficiency, we want the code that creates GetLocals
                // and Phis to try to reuse VariableAccessDatas as much as possible.
                ASSERT(m_graph[valueInPredecessor].variableAccessData() == m_graph[m_graph[valueInPredecessor].child1().index()].variableAccessData());
                
                valueInPredecessor = m_graph[valueInPredecessor].child1().index();
            } else {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Found @%u.\n", valueInPredecessor);
#endif
            }
            ASSERT(m_graph[valueInPredecessor].op() == SetLocal
                   || m_graph[valueInPredecessor].op() == Phi
                   || m_graph[valueInPredecessor].op() == Flush
                   || (m_graph[valueInPredecessor].op() == SetArgument
                       && stackType == ArgumentPhiStack));
            
            VariableAccessData* dataForPredecessor = m_graph[valueInPredecessor].variableAccessData();
            
            dataForPredecessor->unify(dataForPhi);

            Node* phiNode = &m_graph[entry.m_phi];
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("      Ref count of @%u = %u.\n", entry.m_phi, phiNode->refCount());
#endif
            if (phiNode->refCount()) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Reffing @%u.\n", valueInPredecessor);
#endif
                m_graph.ref(valueInPredecessor);
            }

            if (!phiNode->child1()) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Setting @%u->child1 = @%u.\n", entry.m_phi, valueInPredecessor);
#endif
                phiNode->children.setChild1(Edge(valueInPredecessor));
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Children of @%u: ", entry.m_phi);
                phiNode->dumpChildren(WTF::dataFile());
                dataLog(".\n");
#endif
                continue;
            }
            if (!phiNode->child2()) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Setting @%u->child2 = @%u.\n", entry.m_phi, valueInPredecessor);
#endif
                phiNode->children.setChild2(Edge(valueInPredecessor));
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Children of @%u: ", entry.m_phi);
                phiNode->dumpChildren(WTF::dataFile());
                dataLog(".\n");
#endif
                continue;
            }
            if (!phiNode->child3()) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Setting @%u->child3 = @%u.\n", entry.m_phi, valueInPredecessor);
#endif
                phiNode->children.setChild3(Edge(valueInPredecessor));
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLog("      Children of @%u: ", entry.m_phi);
                phiNode->dumpChildren(WTF::dataFile());
                dataLog(".\n");
#endif
                continue;
            }
            
            NodeIndex newPhi = insertPhiNode(OpInfo(dataForPhi), entry.m_block);
            
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("      Splitting @%u, created @%u.\n", entry.m_phi, newPhi);
#endif

            phiNode = &m_graph[entry.m_phi]; // reload after vector resize
            Node& newPhiNode = m_graph[newPhi];
            if (phiNode->refCount())
                m_graph.ref(newPhi);

            newPhiNode.children = phiNode->children;

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("      Children of @%u: ", newPhi);
            newPhiNode.dumpChildren(WTF::dataFile());
            dataLog(".\n");
#endif

            phiNode->children.initialize(newPhi, valueInPredecessor, NoNode);

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("      Children of @%u: ", entry.m_phi);
            phiNode->dumpChildren(WTF::dataFile());
            dataLog(".\n");
#endif
        }
    }
}

void ByteCodeParser::fixVariableAccessPredictions()
{
    for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i) {
        VariableAccessData* data = &m_graph.m_variableAccessData[i];
        data->find()->predict(data->nonUnifiedPrediction());
        data->find()->mergeIsCaptured(data->isCaptured());
        data->find()->mergeStructureCheckHoistingFailed(data->structureCheckHoistingFailed());
    }
}

void ByteCodeParser::linkBlock(BasicBlock* block, Vector<BlockIndex>& possibleTargets)
{
    ASSERT(!block->isLinked);
    ASSERT(!block->isEmpty());
    Node& node = m_graph[block->last()];
    ASSERT(node.isTerminal());
    
    switch (node.op()) {
    case Jump:
        node.setTakenBlockIndex(m_graph.blockIndexForBytecodeOffset(possibleTargets, node.takenBytecodeOffsetDuringParsing()));
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Linked basic block %p to %p, #%u.\n", block, m_graph.m_blocks[node.takenBlockIndex()].get(), node.takenBlockIndex());
#endif
        break;
        
    case Branch:
        node.setTakenBlockIndex(m_graph.blockIndexForBytecodeOffset(possibleTargets, node.takenBytecodeOffsetDuringParsing()));
        node.setNotTakenBlockIndex(m_graph.blockIndexForBytecodeOffset(possibleTargets, node.notTakenBytecodeOffsetDuringParsing()));
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Linked basic block %p to %p, #%u and %p, #%u.\n", block, m_graph.m_blocks[node.takenBlockIndex()].get(), node.takenBlockIndex(), m_graph.m_blocks[node.notTakenBlockIndex()].get(), node.notTakenBlockIndex());
#endif
        break;
        
    default:
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Marking basic block %p as linked.\n", block);
#endif
        break;
    }
    
#if !ASSERT_DISABLED
    block->isLinked = true;
#endif
}

void ByteCodeParser::linkBlocks(Vector<UnlinkedBlock>& unlinkedBlocks, Vector<BlockIndex>& possibleTargets)
{
    for (size_t i = 0; i < unlinkedBlocks.size(); ++i) {
        if (unlinkedBlocks[i].m_needsNormalLinking) {
            linkBlock(m_graph.m_blocks[unlinkedBlocks[i].m_blockIndex].get(), possibleTargets);
            unlinkedBlocks[i].m_needsNormalLinking = false;
        }
    }
}

void ByteCodeParser::buildOperandMapsIfNecessary()
{
    if (m_haveBuiltOperandMaps)
        return;
    
    for (size_t i = 0; i < m_codeBlock->numberOfIdentifiers(); ++i)
        m_identifierMap.add(m_codeBlock->identifier(i).impl(), i);
    for (size_t i = 0; i < m_codeBlock->numberOfConstantRegisters(); ++i) {
        JSValue value = m_codeBlock->getConstant(i + FirstConstantRegisterIndex);
        if (!value)
            m_emptyJSValueIndex = i + FirstConstantRegisterIndex;
        else
            m_jsValueMap.add(JSValue::encode(value), i + FirstConstantRegisterIndex);
    }
    
    m_haveBuiltOperandMaps = true;
}

ByteCodeParser::InlineStackEntry::InlineStackEntry(
    ByteCodeParser* byteCodeParser,
    CodeBlock* codeBlock,
    CodeBlock* profiledBlock,
    BlockIndex callsiteBlockHead,
    VirtualRegister calleeVR,
    JSFunction* callee,
    VirtualRegister returnValueVR,
    VirtualRegister inlineCallFrameStart,
    int argumentCountIncludingThis,
    CodeSpecializationKind kind)
    : m_byteCodeParser(byteCodeParser)
    , m_codeBlock(codeBlock)
    , m_profiledBlock(profiledBlock)
    , m_calleeVR(calleeVR)
    , m_exitProfile(profiledBlock->exitProfile())
    , m_callsiteBlockHead(callsiteBlockHead)
    , m_returnValue(returnValueVR)
    , m_lazyOperands(profiledBlock->lazyOperandValueProfiles())
    , m_didReturn(false)
    , m_didEarlyReturn(false)
    , m_caller(byteCodeParser->m_inlineStackTop)
{
    m_argumentPositions.resize(argumentCountIncludingThis);
    for (int i = 0; i < argumentCountIncludingThis; ++i) {
        byteCodeParser->m_graph.m_argumentPositions.append(ArgumentPosition());
        ArgumentPosition* argumentPosition = &byteCodeParser->m_graph.m_argumentPositions.last();
        m_argumentPositions[i] = argumentPosition;
    }
    
    // Track the code-block-global exit sites.
    if (m_exitProfile.hasExitSite(ArgumentsEscaped)) {
        byteCodeParser->m_graph.m_executablesWhoseArgumentsEscaped.add(
            codeBlock->ownerExecutable());
    }
        
    if (m_caller) {
        // Inline case.
        ASSERT(codeBlock != byteCodeParser->m_codeBlock);
        ASSERT(callee);
        ASSERT(calleeVR != InvalidVirtualRegister);
        ASSERT(inlineCallFrameStart != InvalidVirtualRegister);
        ASSERT(callsiteBlockHead != NoBlock);
        
        InlineCallFrame inlineCallFrame;
        inlineCallFrame.executable.set(*byteCodeParser->m_globalData, byteCodeParser->m_codeBlock->ownerExecutable(), codeBlock->ownerExecutable());
        inlineCallFrame.stackOffset = inlineCallFrameStart + RegisterFile::CallFrameHeaderSize;
        inlineCallFrame.callee.set(*byteCodeParser->m_globalData, byteCodeParser->m_codeBlock->ownerExecutable(), callee);
        inlineCallFrame.caller = byteCodeParser->currentCodeOrigin();
        inlineCallFrame.arguments.resize(argumentCountIncludingThis); // Set the number of arguments including this, but don't configure the value recoveries, yet.
        inlineCallFrame.isCall = isCall(kind);
        
        if (inlineCallFrame.caller.inlineCallFrame)
            inlineCallFrame.capturedVars = inlineCallFrame.caller.inlineCallFrame->capturedVars;
        else {
            for (int i = byteCodeParser->m_codeBlock->m_numVars; i--;) {
                if (byteCodeParser->m_codeBlock->isCaptured(i))
                    inlineCallFrame.capturedVars.set(i);
            }
        }

        for (int i = argumentCountIncludingThis; i--;) {
            if (codeBlock->isCaptured(argumentToOperand(i)))
                inlineCallFrame.capturedVars.set(argumentToOperand(i) + inlineCallFrame.stackOffset);
        }
        for (size_t i = codeBlock->m_numVars; i--;) {
            if (codeBlock->isCaptured(i))
                inlineCallFrame.capturedVars.set(i + inlineCallFrame.stackOffset);
        }

#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Current captured variables: ");
        inlineCallFrame.capturedVars.dump(WTF::dataFile());
        dataLog("\n");
#endif
        
        byteCodeParser->m_codeBlock->inlineCallFrames().append(inlineCallFrame);
        m_inlineCallFrame = &byteCodeParser->m_codeBlock->inlineCallFrames().last();
        
        byteCodeParser->buildOperandMapsIfNecessary();
        
        m_identifierRemap.resize(codeBlock->numberOfIdentifiers());
        m_constantRemap.resize(codeBlock->numberOfConstantRegisters());

        for (size_t i = 0; i < codeBlock->numberOfIdentifiers(); ++i) {
            StringImpl* rep = codeBlock->identifier(i).impl();
            IdentifierMap::AddResult result = byteCodeParser->m_identifierMap.add(rep, byteCodeParser->m_codeBlock->numberOfIdentifiers());
            if (result.isNewEntry)
                byteCodeParser->m_codeBlock->addIdentifier(Identifier(byteCodeParser->m_globalData, rep));
            m_identifierRemap[i] = result.iterator->second;
        }
        for (size_t i = 0; i < codeBlock->numberOfConstantRegisters(); ++i) {
            JSValue value = codeBlock->getConstant(i + FirstConstantRegisterIndex);
            if (!value) {
                if (byteCodeParser->m_emptyJSValueIndex == UINT_MAX) {
                    byteCodeParser->m_emptyJSValueIndex = byteCodeParser->m_codeBlock->numberOfConstantRegisters() + FirstConstantRegisterIndex;
                    byteCodeParser->m_codeBlock->addConstant(JSValue());
                    byteCodeParser->m_constants.append(ConstantRecord());
                }
                m_constantRemap[i] = byteCodeParser->m_emptyJSValueIndex;
                continue;
            }
            JSValueMap::AddResult result = byteCodeParser->m_jsValueMap.add(JSValue::encode(value), byteCodeParser->m_codeBlock->numberOfConstantRegisters() + FirstConstantRegisterIndex);
            if (result.isNewEntry) {
                byteCodeParser->m_codeBlock->addConstant(value);
                byteCodeParser->m_constants.append(ConstantRecord());
            }
            m_constantRemap[i] = result.iterator->second;
        }
        for (unsigned i = 0; i < codeBlock->numberOfGlobalResolveInfos(); ++i)
            byteCodeParser->m_codeBlock->addGlobalResolveInfo(std::numeric_limits<unsigned>::max());
        
        m_callsiteBlockHeadNeedsLinking = true;
    } else {
        // Machine code block case.
        ASSERT(codeBlock == byteCodeParser->m_codeBlock);
        ASSERT(!callee);
        ASSERT(calleeVR == InvalidVirtualRegister);
        ASSERT(returnValueVR == InvalidVirtualRegister);
        ASSERT(inlineCallFrameStart == InvalidVirtualRegister);
        ASSERT(callsiteBlockHead == NoBlock);

        m_inlineCallFrame = 0;

        m_identifierRemap.resize(codeBlock->numberOfIdentifiers());
        m_constantRemap.resize(codeBlock->numberOfConstantRegisters());

        for (size_t i = 0; i < codeBlock->numberOfIdentifiers(); ++i)
            m_identifierRemap[i] = i;
        for (size_t i = 0; i < codeBlock->numberOfConstantRegisters(); ++i)
            m_constantRemap[i] = i + FirstConstantRegisterIndex;

        m_callsiteBlockHeadNeedsLinking = false;
    }
    
    for (size_t i = 0; i < m_constantRemap.size(); ++i)
        ASSERT(m_constantRemap[i] >= static_cast<unsigned>(FirstConstantRegisterIndex));
    
    byteCodeParser->m_inlineStackTop = this;
}

void ByteCodeParser::parseCodeBlock()
{
    CodeBlock* codeBlock = m_inlineStackTop->m_codeBlock;
    
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("Parsing code block %p. codeType = %s, numCapturedVars = %u, needsFullScopeChain = %s, needsActivation = %s, isStrictMode = %s\n",
            codeBlock,
            codeTypeToString(codeBlock->codeType()),
            codeBlock->m_numCapturedVars,
            codeBlock->needsFullScopeChain()?"true":"false",
            codeBlock->ownerExecutable()->needsActivation()?"true":"false",
            codeBlock->ownerExecutable()->isStrictMode()?"true":"false");
    codeBlock->baselineVersion()->dump(m_exec);
#endif
    
    for (unsigned jumpTargetIndex = 0; jumpTargetIndex <= codeBlock->numberOfJumpTargets(); ++jumpTargetIndex) {
        // The maximum bytecode offset to go into the current basicblock is either the next jump target, or the end of the instructions.
        unsigned limit = jumpTargetIndex < codeBlock->numberOfJumpTargets() ? codeBlock->jumpTarget(jumpTargetIndex) : codeBlock->instructions().size();
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("Parsing bytecode with limit %p bc#%u at inline depth %u.\n", m_inlineStackTop->executable(), limit, CodeOrigin::inlineDepthForCallFrame(m_inlineStackTop->m_inlineCallFrame));
#endif
        ASSERT(m_currentIndex < limit);

        // Loop until we reach the current limit (i.e. next jump target).
        do {
            if (!m_currentBlock) {
                // Check if we can use the last block.
                if (!m_graph.m_blocks.isEmpty() && m_graph.m_blocks.last()->isEmpty()) {
                    // This must be a block belonging to us.
                    ASSERT(m_inlineStackTop->m_unlinkedBlocks.last().m_blockIndex == m_graph.m_blocks.size() - 1);
                    // Either the block is linkable or it isn't. If it's linkable then it's the last
                    // block in the blockLinkingTargets list. If it's not then the last block will
                    // have a lower bytecode index that the one we're about to give to this block.
                    if (m_inlineStackTop->m_blockLinkingTargets.isEmpty() || m_graph.m_blocks[m_inlineStackTop->m_blockLinkingTargets.last()]->bytecodeBegin != m_currentIndex) {
                        // Make the block linkable.
                        ASSERT(m_inlineStackTop->m_blockLinkingTargets.isEmpty() || m_graph.m_blocks[m_inlineStackTop->m_blockLinkingTargets.last()]->bytecodeBegin < m_currentIndex);
                        m_inlineStackTop->m_blockLinkingTargets.append(m_graph.m_blocks.size() - 1);
                    }
                    // Change its bytecode begin and continue.
                    m_currentBlock = m_graph.m_blocks.last().get();
#if DFG_ENABLE(DEBUG_VERBOSE)
                    dataLog("Reascribing bytecode index of block %p from bc#%u to bc#%u (peephole case).\n", m_currentBlock, m_currentBlock->bytecodeBegin, m_currentIndex);
#endif
                    m_currentBlock->bytecodeBegin = m_currentIndex;
                } else {
                    OwnPtr<BasicBlock> block = adoptPtr(new BasicBlock(m_currentIndex, m_numArguments, m_numLocals));
#if DFG_ENABLE(DEBUG_VERBOSE)
                    dataLog("Creating basic block %p, #%zu for %p bc#%u at inline depth %u.\n", block.get(), m_graph.m_blocks.size(), m_inlineStackTop->executable(), m_currentIndex, CodeOrigin::inlineDepthForCallFrame(m_inlineStackTop->m_inlineCallFrame));
#endif
                    m_currentBlock = block.get();
                    ASSERT(m_inlineStackTop->m_unlinkedBlocks.isEmpty() || m_graph.m_blocks[m_inlineStackTop->m_unlinkedBlocks.last().m_blockIndex]->bytecodeBegin < m_currentIndex);
                    m_inlineStackTop->m_unlinkedBlocks.append(UnlinkedBlock(m_graph.m_blocks.size()));
                    m_inlineStackTop->m_blockLinkingTargets.append(m_graph.m_blocks.size());
                    // The first block is definitely an OSR target.
                    if (!m_graph.m_blocks.size())
                        block->isOSRTarget = true;
                    m_graph.m_blocks.append(block.release());
                    prepareToParseBlock();
                }
            }

            bool shouldContinueParsing = parseBlock(limit);

            // We should not have gone beyond the limit.
            ASSERT(m_currentIndex <= limit);
            
            // We should have planted a terminal, or we just gave up because
            // we realized that the jump target information is imprecise, or we
            // are at the end of an inline function, or we realized that we
            // should stop parsing because there was a return in the first
            // basic block.
            ASSERT(m_currentBlock->isEmpty() || m_graph.last().isTerminal() || (m_currentIndex == codeBlock->instructions().size() && m_inlineStackTop->m_inlineCallFrame) || !shouldContinueParsing);

            if (!shouldContinueParsing)
                return;
            
            m_currentBlock = 0;
        } while (m_currentIndex < limit);
    }

    // Should have reached the end of the instructions.
    ASSERT(m_currentIndex == codeBlock->instructions().size());
}

bool ByteCodeParser::parse()
{
    // Set during construction.
    ASSERT(!m_currentIndex);
    
#if DFG_ENABLE(ALL_VARIABLES_CAPTURED)
    // We should be pretending that the code has an activation.
    ASSERT(m_graph.needsActivation());
#endif
    
    InlineStackEntry inlineStackEntry(
        this, m_codeBlock, m_profiledBlock, NoBlock, InvalidVirtualRegister, 0,
        InvalidVirtualRegister, InvalidVirtualRegister, m_codeBlock->numParameters(),
        CodeForCall);
    
    parseCodeBlock();

    linkBlocks(inlineStackEntry.m_unlinkedBlocks, inlineStackEntry.m_blockLinkingTargets);
    m_graph.determineReachability();
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
    dataLog("Processing local variable phis.\n");
#endif
    
    m_currentProfilingIndex = m_currentIndex;
    
    processPhiStack<LocalPhiStack>();
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
    dataLog("Processing argument phis.\n");
#endif
    processPhiStack<ArgumentPhiStack>();

    for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        ASSERT(block);
        if (!block->isReachable)
            m_graph.m_blocks[blockIndex].clear();
    }
    
    fixVariableAccessPredictions();
    
    for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        if (!block)
            continue;
        if (!block->isOSRTarget)
            continue;
        if (block->bytecodeBegin != m_graph.m_osrEntryBytecodeIndex)
            continue;
        for (size_t i = 0; i < m_graph.m_mustHandleValues.size(); ++i) {
            NodeIndex nodeIndex = block->variablesAtHead.operand(
                m_graph.m_mustHandleValues.operandForIndex(i));
            if (nodeIndex == NoNode)
                continue;
            Node& node = m_graph[nodeIndex];
            ASSERT(node.hasLocal());
            node.variableAccessData()->predict(
                speculationFromValue(m_graph.m_mustHandleValues[i]));
        }
    }
    
    m_graph.m_preservedVars = m_preservedVars;
    m_graph.m_localVars = m_numLocals;
    m_graph.m_parameterSlots = m_parameterSlots;

    return true;
}

bool parse(ExecState* exec, Graph& graph)
{
    SamplingRegion samplingRegion("DFG Parsing");
#if DFG_DEBUG_LOCAL_DISBALE
    UNUSED_PARAM(exec);
    UNUSED_PARAM(graph);
    return false;
#else
    return ByteCodeParser(exec, graph).parse();
#endif
}

} } // namespace JSC::DFG

#endif
