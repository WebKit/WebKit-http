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

#ifndef DFGGraph_h
#define DFGGraph_h

#if ENABLE(DFG_JIT)

#include "AssemblyHelpers.h"
#include "CodeBlock.h"
#include "DFGArgumentPosition.h"
#include "DFGBasicBlock.h"
#include "DFGDominators.h"
#include "DFGFrozenValue.h"
#include "DFGLongLivedState.h"
#include "DFGNaturalLoops.h"
#include "DFGNode.h"
#include "DFGNodeAllocator.h"
#include "DFGPlan.h"
#include "DFGScannable.h"
#include "JSStack.h"
#include "MethodOfGettingAValueProfile.h"
#include <unordered_map>
#include <wtf/BitVector.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/StdLibExtras.h>

namespace JSC {

class CodeBlock;
class ExecState;

namespace DFG {

struct StorageAccessData {
    PropertyOffset offset;
    unsigned identifierNumber;
};

struct InlineVariableData {
    InlineCallFrame* inlineCallFrame;
    unsigned argumentPositionStart;
    VariableAccessData* calleeVariable;
};

enum AddSpeculationMode {
    DontSpeculateInt32,
    SpeculateInt32AndTruncateConstants,
    SpeculateInt32
};

//
// === Graph ===
//
// The order may be significant for nodes with side-effects (property accesses, value conversions).
// Nodes that are 'dead' remain in the vector with refCount 0.
class Graph : public virtual Scannable {
public:
    Graph(VM&, Plan&, LongLivedState&);
    ~Graph();
    
    void changeChild(Edge& edge, Node* newNode)
    {
        edge.setNode(newNode);
    }
    
    void changeEdge(Edge& edge, Edge newEdge)
    {
        edge = newEdge;
    }
    
    void compareAndSwap(Edge& edge, Node* oldNode, Node* newNode)
    {
        if (edge.node() != oldNode)
            return;
        changeChild(edge, newNode);
    }
    
    void compareAndSwap(Edge& edge, Edge oldEdge, Edge newEdge)
    {
        if (edge != oldEdge)
            return;
        changeEdge(edge, newEdge);
    }
    
    void performSubstitution(Node* node)
    {
        if (node->flags() & NodeHasVarArgs) {
            for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); childIdx++)
                performSubstitutionForEdge(m_varArgChildren[childIdx]);
        } else {
            performSubstitutionForEdge(node->child1());
            performSubstitutionForEdge(node->child2());
            performSubstitutionForEdge(node->child3());
        }
    }
    
    void performSubstitutionForEdge(Edge& child)
    {
        // Check if this operand is actually unused.
        if (!child)
            return;
        
        // Check if there is any replacement.
        Node* replacement = child->misc.replacement;
        if (!replacement)
            return;
        
        child.setNode(replacement);
        
        // There is definitely a replacement. Assert that the replacement does not
        // have a replacement.
        ASSERT(!child->misc.replacement);
    }
    
    template<typename... Params>
    Node* addNode(SpeculatedType type, Params... params)
    {
        Node* node = new (m_allocator) Node(params...);
        node->predict(type);
        return node;
    }

    void dethread();
    
    FrozenValue* freezeFragile(JSValue value);
    FrozenValue* freeze(JSValue value); // We use weak freezing by default. Shorthand for freezeFragile(value)->strengthenTo(WeakValue);
    FrozenValue* freezeStrong(JSValue value); // Shorthand for freezeFragile(value)->strengthenTo(StrongValue).
    
    void convertToConstant(Node* node, FrozenValue* value);
    void convertToConstant(Node* node, JSValue value);
    void convertToStrongConstant(Node* node, JSValue value);
    
    void assertIsWatched(Structure* structure);
    
    // CodeBlock is optional, but may allow additional information to be dumped (e.g. Identifier names).
    void dump(PrintStream& = WTF::dataFile(), DumpContext* = 0);
    enum PhiNodeDumpMode { DumpLivePhisOnly, DumpAllPhis };
    void dumpBlockHeader(PrintStream&, const char* prefix, BasicBlock*, PhiNodeDumpMode, DumpContext*);
    void dump(PrintStream&, Edge);
    void dump(PrintStream&, const char* prefix, Node*, DumpContext* = 0);
    static int amountOfNodeWhiteSpace(Node*);
    static void printNodeWhiteSpace(PrintStream&, Node*);

    // Dump the code origin of the given node as a diff from the code origin of the
    // preceding node. Returns true if anything was printed.
    bool dumpCodeOrigin(PrintStream&, const char* prefix, Node* previousNode, Node* currentNode, DumpContext*);

    AddSpeculationMode addSpeculationMode(Node* add, bool leftShouldSpeculateInt32, bool rightShouldSpeculateInt32, PredictionPass pass)
    {
        ASSERT(add->op() == ValueAdd || add->op() == ArithAdd || add->op() == ArithSub);
        
        RareCaseProfilingSource source = add->sourceFor(pass);
        
        Node* left = add->child1().node();
        Node* right = add->child2().node();
        
        if (left->hasConstant())
            return addImmediateShouldSpeculateInt32(add, rightShouldSpeculateInt32, left, source);
        if (right->hasConstant())
            return addImmediateShouldSpeculateInt32(add, leftShouldSpeculateInt32, right, source);
        
        return (leftShouldSpeculateInt32 && rightShouldSpeculateInt32 && add->canSpeculateInt32(source)) ? SpeculateInt32 : DontSpeculateInt32;
    }
    
    AddSpeculationMode valueAddSpeculationMode(Node* add, PredictionPass pass)
    {
        return addSpeculationMode(
            add,
            add->child1()->shouldSpeculateInt32OrBooleanExpectingDefined(),
            add->child2()->shouldSpeculateInt32OrBooleanExpectingDefined(),
            pass);
    }
    
    AddSpeculationMode arithAddSpeculationMode(Node* add, PredictionPass pass)
    {
        return addSpeculationMode(
            add,
            add->child1()->shouldSpeculateInt32OrBooleanForArithmetic(),
            add->child2()->shouldSpeculateInt32OrBooleanForArithmetic(),
            pass);
    }
    
    AddSpeculationMode addSpeculationMode(Node* add, PredictionPass pass)
    {
        if (add->op() == ValueAdd)
            return valueAddSpeculationMode(add, pass);
        
        return arithAddSpeculationMode(add, pass);
    }
    
    bool addShouldSpeculateInt32(Node* add, PredictionPass pass)
    {
        return addSpeculationMode(add, pass) != DontSpeculateInt32;
    }
    
    bool addShouldSpeculateMachineInt(Node* add)
    {
        if (!enableInt52())
            return false;
        
        Node* left = add->child1().node();
        Node* right = add->child2().node();

        bool speculation;
        if (add->op() == ValueAdd)
            speculation = Node::shouldSpeculateMachineInt(left, right);
        else
            speculation = Node::shouldSpeculateMachineInt(left, right);

        return speculation && !hasExitSite(add, Int52Overflow);
    }
    
    bool mulShouldSpeculateInt32(Node* mul, PredictionPass pass)
    {
        ASSERT(mul->op() == ArithMul);
        
        Node* left = mul->child1().node();
        Node* right = mul->child2().node();
        
        return Node::shouldSpeculateInt32OrBooleanForArithmetic(left, right)
            && mul->canSpeculateInt32(mul->sourceFor(pass));
    }
    
    bool mulShouldSpeculateMachineInt(Node* mul, PredictionPass pass)
    {
        ASSERT(mul->op() == ArithMul);
        
        if (!enableInt52())
            return false;
        
        Node* left = mul->child1().node();
        Node* right = mul->child2().node();

        return Node::shouldSpeculateMachineInt(left, right)
            && mul->canSpeculateInt52(pass)
            && !hasExitSite(mul, Int52Overflow);
    }
    
    bool negateShouldSpeculateInt32(Node* negate, PredictionPass pass)
    {
        ASSERT(negate->op() == ArithNegate);
        return negate->child1()->shouldSpeculateInt32OrBooleanForArithmetic()
            && negate->canSpeculateInt32(pass);
    }
    
    bool negateShouldSpeculateMachineInt(Node* negate, PredictionPass pass)
    {
        ASSERT(negate->op() == ArithNegate);
        if (!enableInt52())
            return false;
        return negate->child1()->shouldSpeculateMachineInt()
            && !hasExitSite(negate, Int52Overflow)
            && negate->canSpeculateInt52(pass);
    }
    
    VirtualRegister bytecodeRegisterForArgument(CodeOrigin codeOrigin, int argument)
    {
        return VirtualRegister(
            codeOrigin.inlineCallFrame->stackOffset +
            baselineCodeBlockFor(codeOrigin)->argumentIndexAfterCapture(argument));
    }
    
    static const char *opName(NodeType);
    
    StructureSet* addStructureSet(const StructureSet& structureSet)
    {
        ASSERT(structureSet.size());
        m_structureSet.append(structureSet);
        return &m_structureSet.last();
    }
    
    JSGlobalObject* globalObjectFor(CodeOrigin codeOrigin)
    {
        return m_codeBlock->globalObjectFor(codeOrigin);
    }
    
    JSObject* globalThisObjectFor(CodeOrigin codeOrigin)
    {
        JSGlobalObject* object = globalObjectFor(codeOrigin);
        return jsCast<JSObject*>(object->methodTable()->toThis(object, object->globalExec(), NotStrictMode));
    }
    
    ScriptExecutable* executableFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return m_codeBlock->ownerExecutable();
        
        return inlineCallFrame->executable.get();
    }
    
    ScriptExecutable* executableFor(const CodeOrigin& codeOrigin)
    {
        return executableFor(codeOrigin.inlineCallFrame);
    }
    
    CodeBlock* baselineCodeBlockFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return m_profiledBlock;
        return baselineCodeBlockForInlineCallFrame(inlineCallFrame);
    }
    
    CodeBlock* baselineCodeBlockFor(const CodeOrigin& codeOrigin)
    {
        return baselineCodeBlockForOriginAndBaselineCodeBlock(codeOrigin, m_profiledBlock);
    }
    
    bool isStrictModeFor(CodeOrigin codeOrigin)
    {
        if (!codeOrigin.inlineCallFrame)
            return m_codeBlock->isStrictMode();
        return jsCast<FunctionExecutable*>(codeOrigin.inlineCallFrame->executable.get())->isStrictMode();
    }
    
    ECMAMode ecmaModeFor(CodeOrigin codeOrigin)
    {
        return isStrictModeFor(codeOrigin) ? StrictMode : NotStrictMode;
    }
    
    bool masqueradesAsUndefinedWatchpointIsStillValid(const CodeOrigin& codeOrigin)
    {
        return globalObjectFor(codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid();
    }
    
    bool hasGlobalExitSite(const CodeOrigin& codeOrigin, ExitKind exitKind)
    {
        return baselineCodeBlockFor(codeOrigin)->hasExitSite(FrequentExitSite(exitKind));
    }
    
    bool hasExitSite(const CodeOrigin& codeOrigin, ExitKind exitKind)
    {
        return baselineCodeBlockFor(codeOrigin)->hasExitSite(FrequentExitSite(codeOrigin.bytecodeIndex, exitKind));
    }
    
    bool hasExitSite(Node* node, ExitKind exitKind)
    {
        return hasExitSite(node->origin.semantic, exitKind);
    }
    
    VirtualRegister argumentsRegisterFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return m_profiledBlock->argumentsRegister();
        
        return VirtualRegister(baselineCodeBlockForInlineCallFrame(
            inlineCallFrame)->argumentsRegister().offset() +
            inlineCallFrame->stackOffset);
    }
    
    VirtualRegister argumentsRegisterFor(const CodeOrigin& codeOrigin)
    {
        return argumentsRegisterFor(codeOrigin.inlineCallFrame);
    }
    
    VirtualRegister machineArgumentsRegisterFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return m_codeBlock->argumentsRegister();
        
        return inlineCallFrame->argumentsRegister;
    }
    
    VirtualRegister machineArgumentsRegisterFor(const CodeOrigin& codeOrigin)
    {
        return machineArgumentsRegisterFor(codeOrigin.inlineCallFrame);
    }
    
    VirtualRegister uncheckedArgumentsRegisterFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return m_profiledBlock->uncheckedArgumentsRegister();
        
        CodeBlock* codeBlock = baselineCodeBlockForInlineCallFrame(inlineCallFrame);
        if (!codeBlock->usesArguments())
            return VirtualRegister();
        
        return VirtualRegister(codeBlock->argumentsRegister().offset() +
            inlineCallFrame->stackOffset);
    }
    
    VirtualRegister uncheckedArgumentsRegisterFor(const CodeOrigin& codeOrigin)
    {
        return uncheckedArgumentsRegisterFor(codeOrigin.inlineCallFrame);
    }
    
    VirtualRegister activationRegister()
    {
        return m_profiledBlock->activationRegister();
    }
    
    VirtualRegister uncheckedActivationRegister()
    {
        return m_profiledBlock->uncheckedActivationRegister();
    }
    
    VirtualRegister machineActivationRegister()
    {
        return m_profiledBlock->activationRegister();
    }
    
    VirtualRegister uncheckedMachineActivationRegister()
    {
        return m_profiledBlock->uncheckedActivationRegister();
    }
    
    ValueProfile* valueProfileFor(Node* node)
    {
        if (!node)
            return 0;
        
        CodeBlock* profiledBlock = baselineCodeBlockFor(node->origin.semantic);
        
        if (node->op() == GetArgument)
            return profiledBlock->valueProfileForArgument(node->local().toArgument());
        
        if (node->hasLocal(*this)) {
            if (m_form == SSA)
                return 0;
            if (!node->local().isArgument())
                return 0;
            int argument = node->local().toArgument();
            if (node->variableAccessData() != m_arguments[argument]->variableAccessData())
                return 0;
            return profiledBlock->valueProfileForArgument(argument);
        }
        
        if (node->hasHeapPrediction())
            return profiledBlock->valueProfileForBytecodeOffset(node->origin.semantic.bytecodeIndex);
        
        return 0;
    }
    
    MethodOfGettingAValueProfile methodOfGettingAValueProfileFor(Node* node)
    {
        if (!node)
            return MethodOfGettingAValueProfile();
        
        CodeBlock* profiledBlock = baselineCodeBlockFor(node->origin.semantic);
        
        if (node->op() == GetLocal) {
            return MethodOfGettingAValueProfile::fromLazyOperand(
                profiledBlock,
                LazyOperandValueProfileKey(
                    node->origin.semantic.bytecodeIndex, node->local()));
        }
        
        return MethodOfGettingAValueProfile(valueProfileFor(node));
    }
    
    bool usesArguments() const
    {
        return m_codeBlock->usesArguments();
    }
    
    BlockIndex numBlocks() const { return m_blocks.size(); }
    BasicBlock* block(BlockIndex blockIndex) const { return m_blocks[blockIndex].get(); }
    BasicBlock* lastBlock() const { return block(numBlocks() - 1); }

    void appendBlock(PassRefPtr<BasicBlock> basicBlock)
    {
        basicBlock->index = m_blocks.size();
        m_blocks.append(basicBlock);
    }
    
    void killBlock(BlockIndex blockIndex)
    {
        m_blocks[blockIndex].clear();
    }
    
    void killBlock(BasicBlock* basicBlock)
    {
        killBlock(basicBlock->index);
    }
    
    void killBlockAndItsContents(BasicBlock*);
    
    void killUnreachableBlocks();
    
    bool isPredictedNumerical(Node* node)
    {
        return isNumerical(node->child1().useKind()) && isNumerical(node->child2().useKind());
    }
    
    // Note that a 'true' return does not actually mean that the ByVal access clobbers nothing.
    // It really means that it will not clobber the entire world. It's still up to you to
    // carefully consider things like:
    // - PutByVal definitely changes the array it stores to, and may even change its length.
    // - PutByOffset definitely changes the object it stores to.
    // - and so on.
    bool byValIsPure(Node* node)
    {
        switch (node->arrayMode().type()) {
        case Array::Generic:
            return false;
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous:
        case Array::ArrayStorage:
            return !node->arrayMode().isOutOfBounds();
        case Array::SlowPutArrayStorage:
            return !node->arrayMode().mayStoreToHole();
        case Array::String:
            return node->op() == GetByVal && node->arrayMode().isInBounds();
#if USE(JSVALUE32_64)
        case Array::Arguments:
            if (node->op() == GetByVal)
                return true;
            return false;
#endif // USE(JSVALUE32_64)
        default:
            return true;
        }
    }
    
    bool clobbersWorld(Node* node)
    {
        if (node->flags() & NodeClobbersWorld)
            return true;
        if (!(node->flags() & NodeMightClobber))
            return false;
        switch (node->op()) {
        case GetByVal:
        case PutByValDirect:
        case PutByVal:
        case PutByValAlias:
            return !byValIsPure(node);
        case ToString:
            switch (node->child1().useKind()) {
            case StringObjectUse:
            case StringOrStringObjectUse:
                return false;
            case CellUse:
            case UntypedUse:
                return true;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                return true;
            }
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return true; // If by some oddity we hit this case in release build it's safer to have CSE assume the worst.
        }
    }
    
    void determineReachability();
    void resetReachability();
    
    unsigned varArgNumChildren(Node* node)
    {
        ASSERT(node->flags() & NodeHasVarArgs);
        return node->numChildren();
    }
    
    unsigned numChildren(Node* node)
    {
        if (node->flags() & NodeHasVarArgs)
            return varArgNumChildren(node);
        return AdjacencyList::Size;
    }
    
    Edge& varArgChild(Node* node, unsigned index)
    {
        ASSERT(node->flags() & NodeHasVarArgs);
        return m_varArgChildren[node->firstChild() + index];
    }
    
    Edge& child(Node* node, unsigned index)
    {
        if (node->flags() & NodeHasVarArgs)
            return varArgChild(node, index);
        return node->children.child(index);
    }
    
    void voteNode(Node* node, unsigned ballot, float weight = 1)
    {
        switch (node->op()) {
        case ValueToInt32:
        case UInt32ToNumber:
            node = node->child1().node();
            break;
        default:
            break;
        }
        
        if (node->op() == GetLocal)
            node->variableAccessData()->vote(ballot, weight);
    }
    
    void voteNode(Edge edge, unsigned ballot, float weight = 1)
    {
        voteNode(edge.node(), ballot, weight);
    }
    
    void voteChildren(Node* node, unsigned ballot, float weight = 1)
    {
        if (node->flags() & NodeHasVarArgs) {
            for (unsigned childIdx = node->firstChild();
                childIdx < node->firstChild() + node->numChildren();
                childIdx++) {
                if (!!m_varArgChildren[childIdx])
                    voteNode(m_varArgChildren[childIdx], ballot, weight);
            }
            return;
        }
        
        if (!node->child1())
            return;
        voteNode(node->child1(), ballot, weight);
        if (!node->child2())
            return;
        voteNode(node->child2(), ballot, weight);
        if (!node->child3())
            return;
        voteNode(node->child3(), ballot, weight);
    }
    
    template<typename T> // T = Node* or Edge
    void substitute(BasicBlock& block, unsigned startIndexInBlock, T oldThing, T newThing)
    {
        for (unsigned indexInBlock = startIndexInBlock; indexInBlock < block.size(); ++indexInBlock) {
            Node* node = block[indexInBlock];
            if (node->flags() & NodeHasVarArgs) {
                for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); ++childIdx) {
                    if (!!m_varArgChildren[childIdx])
                        compareAndSwap(m_varArgChildren[childIdx], oldThing, newThing);
                }
                continue;
            }
            if (!node->child1())
                continue;
            compareAndSwap(node->children.child1(), oldThing, newThing);
            if (!node->child2())
                continue;
            compareAndSwap(node->children.child2(), oldThing, newThing);
            if (!node->child3())
                continue;
            compareAndSwap(node->children.child3(), oldThing, newThing);
        }
    }
    
    // Use this if you introduce a new GetLocal and you know that you introduced it *before*
    // any GetLocals in the basic block.
    // FIXME: it may be appropriate, in the future, to generalize this to handle GetLocals
    // introduced anywhere in the basic block.
    void substituteGetLocal(BasicBlock& block, unsigned startIndexInBlock, VariableAccessData* variableAccessData, Node* newGetLocal);
    
    void invalidateCFG();
    
    void clearFlagsOnAllNodes(NodeFlags);
    
    void clearReplacements();
    void initializeNodeOwners();
    
    void getBlocksInDepthFirstOrder(Vector<BasicBlock*>& result);
    
    Profiler::Compilation* compilation() { return m_plan.compilation.get(); }
    
    DesiredIdentifiers& identifiers() { return m_plan.identifiers; }
    DesiredWatchpoints& watchpoints() { return m_plan.watchpoints; }
    
    FullBytecodeLiveness& livenessFor(CodeBlock*);
    FullBytecodeLiveness& livenessFor(InlineCallFrame*);
    bool isLiveInBytecode(VirtualRegister, CodeOrigin);
    
    unsigned frameRegisterCount();
    unsigned stackPointerOffset();
    unsigned requiredRegisterCountForExit();
    unsigned requiredRegisterCountForExecutionAndExit();
    
    JSActivation* tryGetActivation(Node*);
    WriteBarrierBase<Unknown>* tryGetRegisters(Node*);
    
    JSArrayBufferView* tryGetFoldableView(Node*);
    JSArrayBufferView* tryGetFoldableView(Node*, ArrayMode);
    JSArrayBufferView* tryGetFoldableViewForChild1(Node*);
    
    void registerFrozenValues();
    
    virtual void visitChildren(SlotVisitor&) override;
    
    NO_RETURN_DUE_TO_CRASH void handleAssertionFailure(
        Node* node, const char* file, int line, const char* function,
        const char* assertion);
    
    VM& m_vm;
    Plan& m_plan;
    CodeBlock* m_codeBlock;
    CodeBlock* m_profiledBlock;
    
    NodeAllocator& m_allocator;

    Operands<FrozenValue*> m_mustHandleValues;
    
    Vector< RefPtr<BasicBlock> , 8> m_blocks;
    Vector<Edge, 16> m_varArgChildren;

    HashMap<EncodedJSValue, FrozenValue*, EncodedJSValueHash, EncodedJSValueHashTraits> m_frozenValueMap;
    Bag<FrozenValue> m_frozenValues;
    
    Vector<StorageAccessData> m_storageAccessData;
    Vector<Node*, 8> m_arguments;
    SegmentedVector<VariableAccessData, 16> m_variableAccessData;
    SegmentedVector<ArgumentPosition, 8> m_argumentPositions;
    SegmentedVector<StructureSet, 16> m_structureSet;
    Bag<Transition> m_transitions;
    SegmentedVector<NewArrayBufferData, 4> m_newArrayBufferData;
    Bag<BranchData> m_branchData;
    Bag<SwitchData> m_switchData;
    Bag<MultiGetByOffsetData> m_multiGetByOffsetData;
    Bag<MultiPutByOffsetData> m_multiPutByOffsetData;
    Vector<InlineVariableData, 4> m_inlineVariableData;
    HashMap<CodeBlock*, std::unique_ptr<FullBytecodeLiveness>> m_bytecodeLiveness;
    bool m_hasArguments;
    HashSet<ExecutableBase*> m_executablesWhoseArgumentsEscaped;
    BitVector m_lazyVars;
    Dominators m_dominators;
    NaturalLoops m_naturalLoops;
    unsigned m_localVars;
    unsigned m_nextMachineLocal;
    unsigned m_parameterSlots;
    int m_machineCaptureStart;
    std::unique_ptr<SlowArgument[]> m_slowArguments;

#if USE(JSVALUE32_64)
    std::unordered_map<int64_t, double*> m_doubleConstantsMap;
    std::unique_ptr<Bag<double>> m_doubleConstants;
#endif
    
    OptimizationFixpointState m_fixpointState;
    StructureWatchpointState m_structureWatchpointState;
    GraphForm m_form;
    UnificationState m_unificationState;
    RefCountState m_refCountState;
private:
    
    void handleSuccessor(Vector<BasicBlock*, 16>& worklist, BasicBlock*, BasicBlock* successor);
    void addForDepthFirstSort(Vector<BasicBlock*>& result, Vector<BasicBlock*, 16>& worklist, HashSet<BasicBlock*>& seen, BasicBlock*);
    
    AddSpeculationMode addImmediateShouldSpeculateInt32(Node* add, bool variableShouldSpeculateInt32, Node* immediate, RareCaseProfilingSource source)
    {
        ASSERT(immediate->hasConstant());
        
        JSValue immediateValue = immediate->asJSValue();
        if (!immediateValue.isNumber() && !immediateValue.isBoolean())
            return DontSpeculateInt32;
        
        if (!variableShouldSpeculateInt32)
            return DontSpeculateInt32;
        
        if (immediateValue.isInt32() || immediateValue.isBoolean())
            return add->canSpeculateInt32(source) ? SpeculateInt32 : DontSpeculateInt32;
        
        double doubleImmediate = immediateValue.asDouble();
        const double twoToThe48 = 281474976710656.0;
        if (doubleImmediate < -twoToThe48 || doubleImmediate > twoToThe48)
            return DontSpeculateInt32;
        
        return bytecodeCanTruncateInteger(add->arithNodeFlags()) ? SpeculateInt32AndTruncateConstants : DontSpeculateInt32;
    }
};

#define DFG_NODE_DO_TO_CHILDREN(graph, node, thingToDo) do {            \
        Node* _node = (node);                                           \
        if (_node->flags() & NodeHasVarArgs) {                          \
            for (unsigned _childIdx = _node->firstChild();              \
                _childIdx < _node->firstChild() + _node->numChildren(); \
                _childIdx++) {                                          \
                if (!!(graph).m_varArgChildren[_childIdx])              \
                    thingToDo(_node, (graph).m_varArgChildren[_childIdx]); \
            }                                                           \
        } else {                                                        \
            if (!_node->child1()) {                                     \
                ASSERT(                                                 \
                    !_node->child2()                                    \
                    && !_node->child3());                               \
                break;                                                  \
            }                                                           \
            thingToDo(_node, _node->child1());                          \
                                                                        \
            if (!_node->child2()) {                                     \
                ASSERT(!_node->child3());                               \
                break;                                                  \
            }                                                           \
            thingToDo(_node, _node->child2());                          \
                                                                        \
            if (!_node->child3())                                       \
                break;                                                  \
            thingToDo(_node, _node->child3());                          \
        }                                                               \
    } while (false)

#define DFG_ASSERT(graph, node, assertion) do {                         \
        if (!!(assertion))                                              \
            break;                                                      \
        (graph).handleAssertionFailure(                                 \
            (node), __FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion); \
    } while (false)

#define DFG_CRASH(graph, node, reason)                                  \
    (graph).handleAssertionFailure(                                     \
        (node), __FILE__, __LINE__, WTF_PRETTY_FUNCTION, (reason));

} } // namespace JSC::DFG

#endif
#endif
