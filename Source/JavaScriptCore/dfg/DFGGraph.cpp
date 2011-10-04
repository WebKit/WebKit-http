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
#include "DFGGraph.h"

#include "CodeBlock.h"

#if ENABLE(DFG_JIT)

namespace JSC { namespace DFG {

#ifndef NDEBUG

// Creates an array of stringized names.
static const char* dfgOpNames[] = {
#define STRINGIZE_DFG_OP_ENUM(opcode, flags) #opcode ,
    FOR_EACH_DFG_OP(STRINGIZE_DFG_OP_ENUM)
#undef STRINGIZE_DFG_OP_ENUM
};

const char *Graph::opName(NodeType op)
{
    return dfgOpNames[op & NodeIdMask];
}

const char* Graph::nameOfVariableAccessData(VariableAccessData* variableAccessData)
{
    // Variables are already numbered. For readability of IR dumps, this returns
    // an alphabetic name for the variable access data, so that you don't have to
    // reason about two numbers (variable number and live range number), but instead
    // a number and a letter.
    
    unsigned index = std::numeric_limits<unsigned>::max();
    for (unsigned i = 0; i < m_variableAccessData.size(); ++i) {
        if (&m_variableAccessData[i] == variableAccessData) {
            index = i;
            break;
        }
    }
    
    ASSERT(index != std::numeric_limits<unsigned>::max());
    
    if (!index)
        return "A";

    static char buf[10];
    BoundsCheckedPointer<char> ptr(buf, sizeof(buf));
    
    while (index) {
        *ptr++ = 'A' + (index % 26);
        index /= 26;
    }
    
    *ptr++ = 0;
    
    return buf;
}

void Graph::dump(NodeIndex nodeIndex, CodeBlock* codeBlock)
{
    Node& node = at(nodeIndex);
    NodeType op = node.op;

    unsigned refCount = node.refCount();
    bool skipped = !refCount;
    bool mustGenerate = node.mustGenerate();
    if (mustGenerate) {
        ASSERT(refCount);
        --refCount;
    }

    // Example/explanation of dataflow dump output
    //
    //   14:   <!2:7>  GetByVal(@3, @13)
    //   ^1     ^2 ^3     ^4       ^5
    //
    // (1) The nodeIndex of this operation.
    // (2) The reference count. The number printed is the 'real' count,
    //     not including the 'mustGenerate' ref. If the node is
    //     'mustGenerate' then the count it prefixed with '!'.
    // (3) The virtual register slot assigned to this node.
    // (4) The name of the operation.
    // (5) The arguments to the operation. The may be of the form:
    //         @#   - a NodeIndex referencing a prior node in the graph.
    //         arg# - an argument number.
    //         $#   - the index in the CodeBlock of a constant { for numeric constants the value is displayed | for integers, in both decimal and hex }.
    //         id#  - the index in the CodeBlock of an identifier { if codeBlock is passed to dump(), the string representation is displayed }.
    //         var# - the index of a var on the global object, used by GetGlobalVar/PutGlobalVar operations.
    printf("% 4d:%s<%c%u:", (int)nodeIndex, skipped ? "  skipped  " : "           ", mustGenerate ? '!' : ' ', refCount);
    if (node.hasResult() && !skipped && node.hasVirtualRegister())
        printf("%u", node.virtualRegister());
    else
        printf("-");
    printf(">\t%s(", opName(op));
    bool hasPrinted = false;
    if (op & NodeHasVarArgs) {
        for (unsigned childIdx = node.firstChild(); childIdx < node.firstChild() + node.numChildren(); childIdx++) {
            if (hasPrinted)
                printf(", ");
            else
                hasPrinted = true;
            printf("@%u", m_varArgChildren[childIdx]);
        }
    } else {
        if (node.child1() != NoNode)
            printf("@%u", node.child1());
        if (node.child2() != NoNode)
            printf(", @%u", node.child2());
        if (node.child3() != NoNode)
            printf(", @%u", node.child3());
        hasPrinted = node.child1() != NoNode;
    }

    if (node.hasArithNodeFlags()) {
        printf("%s%s", hasPrinted ? ", " : "", arithNodeFlagsAsString(node.rawArithNodeFlags()));
        hasPrinted = true;
    }
    if (node.hasVarNumber()) {
        printf("%svar%u", hasPrinted ? ", " : "", node.varNumber());
        hasPrinted = true;
    }
    if (node.hasIdentifier()) {
        if (codeBlock)
            printf("%sid%u{%s}", hasPrinted ? ", " : "", node.identifierNumber(), codeBlock->identifier(node.identifierNumber()).ustring().utf8().data());
        else
            printf("%sid%u", hasPrinted ? ", " : "", node.identifierNumber());
        hasPrinted = true;
    }
    if (node.hasStructureSet()) {
        for (size_t i = 0; i < node.structureSet().size(); ++i) {
            printf("%sstruct(%p)", hasPrinted ? ", " : "", node.structureSet()[i]);
            hasPrinted = true;
        }
    }
    if (node.hasStructureTransitionData()) {
        printf("%sstruct(%p -> %p)", hasPrinted ? ", " : "", node.structureTransitionData().previousStructure, node.structureTransitionData().newStructure);
        hasPrinted = true;
    }
    if (node.hasStorageAccessData()) {
        StorageAccessData& storageAccessData = m_storageAccessData[node.storageAccessDataIndex()];
        if (codeBlock)
            printf("%sid%u{%s}", hasPrinted ? ", " : "", storageAccessData.identifierNumber, codeBlock->identifier(storageAccessData.identifierNumber).ustring().utf8().data());
        else
            printf("%sid%u", hasPrinted ? ", " : "", storageAccessData.identifierNumber);
        
        printf(", %lu", storageAccessData.offset);
        hasPrinted = true;
    }
    ASSERT(node.hasVariableAccessData() == node.hasLocal());
    if (node.hasVariableAccessData()) {
        VariableAccessData* variableAccessData = node.variableAccessData();
        int operand = variableAccessData->operand();
        if (operandIsArgument(operand))
            printf("%sarg%u(%s)", hasPrinted ? ", " : "", operand - codeBlock->thisRegister(), nameOfVariableAccessData(variableAccessData));
        else
            printf("%sr%u(%s)", hasPrinted ? ", " : "", operand, nameOfVariableAccessData(variableAccessData));
        hasPrinted = true;
    }
    if (op == JSConstant) {
        printf("%s$%u", hasPrinted ? ", " : "", node.constantNumber());
        if (codeBlock) {
            JSValue value = valueOfJSConstant(codeBlock, nodeIndex);
            printf(" = %s", value.description());
        }
        hasPrinted = true;
    }
    if  (node.isBranch() || node.isJump()) {
        printf("%sT:#%u", hasPrinted ? ", " : "", blockIndexForBytecodeOffset(node.takenBytecodeOffset()));
        hasPrinted = true;
    }
    if  (node.isBranch()) {
        printf("%sF:#%u", hasPrinted ? ", " : "", blockIndexForBytecodeOffset(node.notTakenBytecodeOffset()));
        hasPrinted = true;
    }
    (void)hasPrinted;
    
    printf(")");

    if (!skipped) {
        if (node.hasVariableAccessData())
            printf("  predicting %s", predictionToString(node.variableAccessData()->prediction()));
        else if (node.hasVarNumber())
            printf("  predicting %s", predictionToString(getGlobalVarPrediction(node.varNumber())));
        else if (node.hasHeapPrediction())
            printf("  predicting %s", predictionToString(node.getHeapPrediction()));
        else if (node.hasMethodCheckData()) {
            MethodCheckData& methodCheckData = m_methodCheckData[node.methodCheckDataIndex()];
            JSCell* functionCell = getJSFunction(methodCheckData.function);
            ExecutableBase* executable = 0;
            CodeBlock* primaryForCall = 0;
            CodeBlock* secondaryForCall = 0;
            CodeBlock* primaryForConstruct = 0;
            CodeBlock* secondaryForConstruct = 0;
            if (functionCell) {
                JSFunction* function = asFunction(functionCell);
                executable = function->executable();
                if (!executable->isHostFunction()) {
                    FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
                    if (functionExecutable->isGeneratedForCall()) {
                        primaryForCall = &functionExecutable->generatedBytecodeForCall();
                        secondaryForCall = primaryForCall->alternative();
                    }
                    if (functionExecutable->isGeneratedForConstruct()) {
                        primaryForConstruct = &functionExecutable->generatedBytecodeForConstruct();
                        secondaryForConstruct = primaryForConstruct->alternative();
                    }
                }
            }
            printf("  predicting function %p(%p(%p(%p) %p(%p)))", methodCheckData.function, executable, primaryForCall, secondaryForCall, primaryForConstruct, secondaryForConstruct);
        }
    }
    
    printf("\n");
}

void Graph::dump(CodeBlock* codeBlock)
{
    for (size_t b = 0; b < m_blocks.size(); ++b) {
        printf("Block #%u (bc#%u):  %s\n", (int)b, m_blocks[b]->bytecodeBegin, m_blocks[b]->isOSRTarget ? " (OSR target)" : "");
        for (size_t i = m_blocks[b]->begin; i < m_blocks[b]->end; ++i)
            dump(i, codeBlock);
    }
    printf("Phi Nodes:\n");
    for (size_t i = m_blocks.last()->end; i < size(); ++i)
        dump(i, codeBlock);
}

#endif

// FIXME: Convert this method to be iterative, not recursive.
void Graph::refChildren(NodeIndex op)
{
    Node& node = at(op);

    if (node.op & NodeHasVarArgs) {
        for (unsigned childIdx = node.firstChild(); childIdx < node.firstChild() + node.numChildren(); childIdx++)
            ref(m_varArgChildren[childIdx]);
    } else {
        if (node.child1() == NoNode) {
            ASSERT(node.child2() == NoNode && node.child3() == NoNode);
            return;
        }
        ref(node.child1());

        if (node.child2() == NoNode) {
            ASSERT(node.child3() == NoNode);
            return;
        }
        ref(node.child2());

        if (node.child3() == NoNode)
            return;
        ref(node.child3());
    }
}

void Graph::predictArgumentTypes(ExecState* exec, CodeBlock* codeBlock)
{
    if (exec) {
        size_t numberOfArguments = std::min(exec->argumentCountIncludingThis(), m_arguments.size());
        
        for (size_t arg = 1; arg < numberOfArguments; ++arg)
            at(m_arguments[arg]).variableAccessData()->predict(predictionFromValue(exec->argument(arg - 1)));
    }
    
    ASSERT(codeBlock);
    ASSERT(codeBlock->alternative());

    CodeBlock* profiledCodeBlock = codeBlock->alternative();
    ASSERT(codeBlock->m_numParameters >= 1);
    for (size_t arg = 0; arg < static_cast<size_t>(codeBlock->m_numParameters); ++arg) {
        ValueProfile* profile = profiledCodeBlock->valueProfileForArgument(arg);
        if (!profile)
            continue;
        
        at(m_arguments[arg]).variableAccessData()->predict(profile->computeUpdatedPrediction());
        
#if ENABLE(DFG_DEBUG_VERBOSE)
        printf("Argument [%lu] prediction: %s\n", arg, predictionToString(at(m_arguments[arg]).variableAccessData()->prediction()));
#endif
    }
}

} } // namespace JSC::DFG

#endif
