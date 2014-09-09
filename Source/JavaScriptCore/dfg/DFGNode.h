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

#ifndef DFGNode_h
#define DFGNode_h

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "DFGAbstractValue.h"
#include "DFGAdjacencyList.h"
#include "DFGArithMode.h"
#include "DFGArrayMode.h"
#include "DFGCommon.h"
#include "DFGLazyJSValue.h"
#include "DFGNodeFlags.h"
#include "DFGNodeOrigin.h"
#include "DFGNodeType.h"
#include "DFGTransition.h"
#include "DFGUseKind.h"
#include "DFGVariableAccessData.h"
#include "GetByIdVariant.h"
#include "JSCJSValue.h"
#include "Operands.h"
#include "PutByIdVariant.h"
#include "SpeculatedType.h"
#include "StructureSet.h"
#include "ValueProfile.h"
#include <wtf/ListDump.h>

namespace JSC { namespace DFG {

class Graph;
struct BasicBlock;

struct MultiGetByOffsetData {
    unsigned identifierNumber;
    Vector<GetByIdVariant, 2> variants;
};

struct MultiPutByOffsetData {
    unsigned identifierNumber;
    Vector<PutByIdVariant, 2> variants;
    
    bool writesStructures() const;
    bool reallocatesStorage() const;
};

struct NewArrayBufferData {
    unsigned startConstant;
    unsigned numConstants;
    IndexingType indexingType;
};

struct BranchTarget {
    BranchTarget()
        : block(0)
        , count(PNaN)
    {
    }
    
    explicit BranchTarget(BasicBlock* block)
        : block(block)
        , count(PNaN)
    {
    }
    
    void setBytecodeIndex(unsigned bytecodeIndex)
    {
        block = bitwise_cast<BasicBlock*>(static_cast<uintptr_t>(bytecodeIndex));
    }
    unsigned bytecodeIndex() const { return bitwise_cast<uintptr_t>(block); }
    
    void dump(PrintStream&) const;
    
    BasicBlock* block;
    float count;
};

struct BranchData {
    static BranchData withBytecodeIndices(
        unsigned takenBytecodeIndex, unsigned notTakenBytecodeIndex)
    {
        BranchData result;
        result.taken.block = bitwise_cast<BasicBlock*>(static_cast<uintptr_t>(takenBytecodeIndex));
        result.notTaken.block = bitwise_cast<BasicBlock*>(static_cast<uintptr_t>(notTakenBytecodeIndex));
        return result;
    }
    
    unsigned takenBytecodeIndex() const { return taken.bytecodeIndex(); }
    unsigned notTakenBytecodeIndex() const { return notTaken.bytecodeIndex(); }
    
    BasicBlock*& forCondition(bool condition)
    {
        if (condition)
            return taken.block;
        return notTaken.block;
    }
    
    BranchTarget taken;
    BranchTarget notTaken;
};

// The SwitchData and associated data structures duplicate the information in
// JumpTable. The DFG may ultimately end up using the JumpTable, though it may
// instead decide to do something different - this is entirely up to the DFG.
// These data structures give the DFG a higher-level semantic description of
// what is going on, which will allow it to make the right decision.
//
// Note that there will never be multiple SwitchCases in SwitchData::cases that
// have the same SwitchCase::value, since the bytecode's JumpTables never have
// duplicates - since the JumpTable maps a value to a target. It's a
// one-to-many mapping. So we may have duplicate targets, but never duplicate
// values.
struct SwitchCase {
    SwitchCase()
    {
    }
    
    SwitchCase(LazyJSValue value, BasicBlock* target)
        : value(value)
        , target(target)
    {
    }
    
    static SwitchCase withBytecodeIndex(LazyJSValue value, unsigned bytecodeIndex)
    {
        SwitchCase result;
        result.value = value;
        result.target.setBytecodeIndex(bytecodeIndex);
        return result;
    }
    
    LazyJSValue value;
    BranchTarget target;
};

struct SwitchData {
    // Initializes most fields to obviously invalid values. Anyone
    // constructing this should make sure to initialize everything they
    // care about manually.
    SwitchData()
        : kind(static_cast<SwitchKind>(-1))
        , switchTableIndex(UINT_MAX)
        , didUseJumpTable(false)
    {
    }
    
    Vector<SwitchCase> cases;
    BranchTarget fallThrough;
    SwitchKind kind;
    unsigned switchTableIndex;
    bool didUseJumpTable;
};

// This type used in passing an immediate argument to Node constructor;
// distinguishes an immediate value (typically an index into a CodeBlock data structure - 
// a constant index, argument, or identifier) from a Node*.
struct OpInfo {
    OpInfo() : m_value(0) { }
    explicit OpInfo(int32_t value) : m_value(static_cast<uintptr_t>(value)) { }
    explicit OpInfo(uint32_t value) : m_value(static_cast<uintptr_t>(value)) { }
#if OS(DARWIN) || USE(JSVALUE64)
    explicit OpInfo(size_t value) : m_value(static_cast<uintptr_t>(value)) { }
#endif
    explicit OpInfo(void* value) : m_value(reinterpret_cast<uintptr_t>(value)) { }
    uintptr_t m_value;
};

// === Node ===
//
// Node represents a single operation in the data flow graph.
struct Node {
    enum VarArgTag { VarArg };
    
    Node() { }
    
    Node(NodeType op, NodeOrigin nodeOrigin, const AdjacencyList& children)
        : origin(nodeOrigin)
        , children(children)
        , m_virtualRegister(VirtualRegister())
        , m_refCount(1)
        , m_prediction(SpecNone)
        , replacement(nullptr)
        , owner(nullptr)
    {
        setOpAndDefaultFlags(op);
    }
    
    // Construct a node with up to 3 children, no immediate value.
    Node(NodeType op, NodeOrigin nodeOrigin, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : origin(nodeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(VirtualRegister())
        , m_refCount(1)
        , m_prediction(SpecNone)
        , m_opInfo(0)
        , m_opInfo2(0)
        , replacement(nullptr)
        , owner(nullptr)
    {
        setOpAndDefaultFlags(op);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }

    // Construct a node with up to 3 children, no immediate value.
    Node(NodeFlags result, NodeType op, NodeOrigin nodeOrigin, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : origin(nodeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(VirtualRegister())
        , m_refCount(1)
        , m_prediction(SpecNone)
        , m_opInfo(0)
        , m_opInfo2(0)
        , replacement(nullptr)
        , owner(nullptr)
    {
        setOpAndDefaultFlags(op);
        setResult(result);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }

    // Construct a node with up to 3 children and an immediate value.
    Node(NodeType op, NodeOrigin nodeOrigin, OpInfo imm, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : origin(nodeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(VirtualRegister())
        , m_refCount(1)
        , m_prediction(SpecNone)
        , m_opInfo(imm.m_value)
        , m_opInfo2(0)
        , replacement(nullptr)
        , owner(nullptr)
    {
        setOpAndDefaultFlags(op);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }

    // Construct a node with up to 3 children and an immediate value.
    Node(NodeFlags result, NodeType op, NodeOrigin nodeOrigin, OpInfo imm, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : origin(nodeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(VirtualRegister())
        , m_refCount(1)
        , m_prediction(SpecNone)
        , m_opInfo(imm.m_value)
        , m_opInfo2(0)
        , replacement(nullptr)
        , owner(nullptr)
    {
        setOpAndDefaultFlags(op);
        setResult(result);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }

    // Construct a node with up to 3 children and two immediate values.
    Node(NodeType op, NodeOrigin nodeOrigin, OpInfo imm1, OpInfo imm2, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : origin(nodeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(VirtualRegister())
        , m_refCount(1)
        , m_prediction(SpecNone)
        , m_opInfo(imm1.m_value)
        , m_opInfo2(imm2.m_value)
        , replacement(nullptr)
        , owner(nullptr)
    {
        setOpAndDefaultFlags(op);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }
    
    // Construct a node with a variable number of children and two immediate values.
    Node(VarArgTag, NodeType op, NodeOrigin nodeOrigin, OpInfo imm1, OpInfo imm2, unsigned firstChild, unsigned numChildren)
        : origin(nodeOrigin)
        , children(AdjacencyList::Variable, firstChild, numChildren)
        , m_virtualRegister(VirtualRegister())
        , m_refCount(1)
        , m_prediction(SpecNone)
        , m_opInfo(imm1.m_value)
        , m_opInfo2(imm2.m_value)
        , replacement(nullptr)
        , owner(nullptr)
    {
        setOpAndDefaultFlags(op);
        ASSERT(m_flags & NodeHasVarArgs);
    }
    
    NodeType op() const { return static_cast<NodeType>(m_op); }
    NodeFlags flags() const { return m_flags; }
    
    // This is not a fast method.
    unsigned index() const;
    
    void setOp(NodeType op)
    {
        m_op = op;
    }
    
    void setFlags(NodeFlags flags)
    {
        m_flags = flags;
    }
    
    bool mergeFlags(NodeFlags flags)
    {
        NodeFlags newFlags = m_flags | flags;
        if (newFlags == m_flags)
            return false;
        m_flags = newFlags;
        return true;
    }
    
    bool filterFlags(NodeFlags flags)
    {
        NodeFlags newFlags = m_flags & flags;
        if (newFlags == m_flags)
            return false;
        m_flags = newFlags;
        return true;
    }
    
    bool clearFlags(NodeFlags flags)
    {
        return filterFlags(~flags);
    }
    
    void setResult(NodeFlags result)
    {
        ASSERT(!(result & ~NodeResultMask));
        clearFlags(NodeResultMask);
        mergeFlags(result);
    }
    
    NodeFlags result() const
    {
        return flags() & NodeResultMask;
    }
    
    void setOpAndDefaultFlags(NodeType op)
    {
        m_op = op;
        m_flags = defaultFlags(op);
    }

    void convertToPhantom()
    {
        setOpAndDefaultFlags(Phantom);
    }
    
    void convertToCheck()
    {
        setOpAndDefaultFlags(Check);
    }
    
    void replaceWith(Node* other)
    {
        convertToPhantom();
        replacement = other;
    }

    void convertToIdentity();

    bool mustGenerate()
    {
        return m_flags & NodeMustGenerate;
    }
    
    bool isConstant()
    {
        switch (op()) {
        case JSConstant:
        case DoubleConstant:
        case Int52Constant:
            return true;
        default:
            return false;
        }
    }
    
    bool isPhantomArguments()
    {
        return op() == PhantomArguments;
    }
    
    bool hasConstant()
    {
        switch (op()) {
        case JSConstant:
        case DoubleConstant:
        case Int52Constant:
        case PhantomArguments:
            return true;
        default:
            return false;
        }
    }

    FrozenValue* constant()
    {
        ASSERT(hasConstant());
        if (op() == PhantomArguments)
            return FrozenValue::emptySingleton();
        return bitwise_cast<FrozenValue*>(m_opInfo);
    }
    
    // Don't call this directly - use Graph::convertToConstant() instead!
    void convertToConstant(FrozenValue* value)
    {
        if (hasDoubleResult())
            m_op = DoubleConstant;
        else if (hasInt52Result())
            m_op = Int52Constant;
        else
            m_op = JSConstant;
        m_flags &= ~(NodeMustGenerate | NodeMightClobber | NodeClobbersWorld);
        m_opInfo = bitwise_cast<uintptr_t>(value);
        children.reset();
    }
    
    void convertToConstantStoragePointer(void* pointer)
    {
        ASSERT(op() == GetIndexedPropertyStorage);
        m_op = ConstantStoragePointer;
        m_opInfo = bitwise_cast<uintptr_t>(pointer);
        children.reset();
    }
    
    void convertToGetLocalUnlinked(VirtualRegister local)
    {
        m_op = GetLocalUnlinked;
        m_flags &= ~(NodeMustGenerate | NodeMightClobber | NodeClobbersWorld);
        m_opInfo = local.offset();
        m_opInfo2 = VirtualRegister().offset();
        children.reset();
    }
    
    void convertToGetByOffset(unsigned storageAccessDataIndex, Edge storage)
    {
        ASSERT(m_op == GetById || m_op == GetByIdFlush || m_op == MultiGetByOffset);
        m_opInfo = storageAccessDataIndex;
        children.setChild2(children.child1());
        children.child2().setUseKind(KnownCellUse);
        children.setChild1(storage);
        m_op = GetByOffset;
        m_flags &= ~NodeClobbersWorld;
    }
    
    void convertToMultiGetByOffset(MultiGetByOffsetData* data)
    {
        ASSERT(m_op == GetById || m_op == GetByIdFlush);
        m_opInfo = bitwise_cast<intptr_t>(data);
        child1().setUseKind(CellUse);
        m_op = MultiGetByOffset;
        m_flags &= ~NodeClobbersWorld;
    }
    
    void convertToPutByOffset(unsigned storageAccessDataIndex, Edge storage)
    {
        ASSERT(m_op == PutById || m_op == PutByIdDirect || m_op == PutByIdFlush || m_op == MultiPutByOffset);
        m_opInfo = storageAccessDataIndex;
        children.setChild3(children.child2());
        children.setChild2(children.child1());
        children.setChild1(storage);
        m_op = PutByOffset;
        m_flags &= ~NodeClobbersWorld;
    }
    
    void convertToMultiPutByOffset(MultiPutByOffsetData* data)
    {
        ASSERT(m_op == PutById || m_op == PutByIdDirect || m_op == PutByIdFlush);
        m_opInfo = bitwise_cast<intptr_t>(data);
        m_op = MultiPutByOffset;
        m_flags &= ~NodeClobbersWorld;
    }
    
    void convertToPhantomLocal()
    {
        ASSERT(m_op == Phantom && (child1()->op() == Phi || child1()->op() == SetLocal || child1()->op() == SetArgument));
        m_op = PhantomLocal;
        m_opInfo = child1()->m_opInfo; // Copy the variableAccessData.
        children.setChild1(Edge());
    }
    
    void convertToGetLocal(VariableAccessData* variable, Node* phi)
    {
        ASSERT(m_op == GetLocalUnlinked);
        m_op = GetLocal;
        m_opInfo = bitwise_cast<uintptr_t>(variable);
        m_opInfo2 = 0;
        children.setChild1(Edge(phi));
    }
    
    void convertToToString()
    {
        ASSERT(m_op == ToPrimitive);
        m_op = ToString;
    }
    
    JSValue asJSValue()
    {
        return constant()->value();
    }
     
    bool isInt32Constant()
    {
        return isConstant() && constant()->value().isInt32();
    }
     
    int32_t asInt32()
    {
        return asJSValue().asInt32();
    }
     
    uint32_t asUInt32()
    {
        return asInt32();
    }
     
    bool isDoubleConstant()
    {
        return isConstant() && constant()->value().isDouble();
    }
     
    bool isNumberConstant()
    {
        return isConstant() && constant()->value().isNumber();
    }
    
    double asNumber()
    {
        return asJSValue().asNumber();
    }
     
    bool isMachineIntConstant()
    {
        return isConstant() && constant()->value().isMachineInt();
    }
     
    int64_t asMachineInt()
    {
        return asJSValue().asMachineInt();
    }
     
    bool isBooleanConstant()
    {
        return isConstant() && constant()->value().isBoolean();
    }
     
    bool asBoolean()
    {
        return constant()->value().asBoolean();
    }
     
    bool isCellConstant()
    {
        return isConstant() && constant()->value().isCell();
    }
     
    JSCell* asCell()
    {
        return constant()->value().asCell();
    }
     
    template<typename T>
    T dynamicCastConstant()
    {
        if (!isCellConstant())
            return nullptr;
        return jsDynamicCast<T>(asCell());
    }
     
    bool containsMovHint()
    {
        switch (op()) {
        case MovHint:
        case ZombieHint:
            return true;
        default:
            return false;
        }
    }
    
    bool hasVariableAccessData(Graph&);
    bool hasLocal(Graph& graph)
    {
        return hasVariableAccessData(graph);
    }
    
    // This is useful for debugging code, where a node that should have a variable
    // access data doesn't have one because it hasn't been initialized yet.
    VariableAccessData* tryGetVariableAccessData()
    {
        VariableAccessData* result = reinterpret_cast<VariableAccessData*>(m_opInfo);
        if (!result)
            return 0;
        return result->find();
    }
    
    VariableAccessData* variableAccessData()
    {
        return reinterpret_cast<VariableAccessData*>(m_opInfo)->find();
    }
    
    VirtualRegister local()
    {
        return variableAccessData()->local();
    }
    
    VirtualRegister machineLocal()
    {
        return variableAccessData()->machineLocal();
    }
    
    bool hasUnlinkedLocal()
    {
        switch (op()) {
        case GetLocalUnlinked:
        case ExtractOSREntryLocal:
        case MovHint:
        case ZombieHint:
            return true;
        default:
            return false;
        }
    }
    
    VirtualRegister unlinkedLocal()
    {
        ASSERT(hasUnlinkedLocal());
        return static_cast<VirtualRegister>(m_opInfo);
    }
    
    bool hasUnlinkedMachineLocal()
    {
        return op() == GetLocalUnlinked;
    }
    
    void setUnlinkedMachineLocal(VirtualRegister reg)
    {
        ASSERT(hasUnlinkedMachineLocal());
        m_opInfo2 = reg.offset();
    }
    
    VirtualRegister unlinkedMachineLocal()
    {
        ASSERT(hasUnlinkedMachineLocal());
        return VirtualRegister(m_opInfo2);
    }
    
    bool hasPhi()
    {
        return op() == Upsilon;
    }
    
    Node* phi()
    {
        ASSERT(hasPhi());
        return bitwise_cast<Node*>(m_opInfo);
    }

    bool isStoreBarrier()
    {
        switch (op()) {
        case StoreBarrier:
        case StoreBarrierWithNullCheck:
            return true;
        default:
            return false;
        }
    }

    bool hasIdentifier()
    {
        switch (op()) {
        case GetById:
        case GetByIdFlush:
        case PutById:
        case PutByIdFlush:
        case PutByIdDirect:
            return true;
        default:
            return false;
        }
    }

    unsigned identifierNumber()
    {
        ASSERT(hasIdentifier());
        return m_opInfo;
    }
    
    bool hasArithNodeFlags()
    {
        switch (op()) {
        case UInt32ToNumber:
        case ArithAdd:
        case ArithSub:
        case ArithNegate:
        case ArithMul:
        case ArithAbs:
        case ArithMin:
        case ArithMax:
        case ArithMod:
        case ArithDiv:
        case ValueAdd:
            return true;
        default:
            return false;
        }
    }
    
    // This corrects the arithmetic node flags, so that irrelevant bits are
    // ignored. In particular, anything other than ArithMul does not need
    // to know if it can speculate on negative zero.
    NodeFlags arithNodeFlags()
    {
        NodeFlags result = m_flags & NodeArithFlagsMask;
        if (op() == ArithMul || op() == ArithDiv || op() == ArithMod || op() == ArithNegate || op() == DoubleAsInt32)
            return result;
        return result & ~NodeBytecodeNeedsNegZero;
    }
    
    bool hasConstantBuffer()
    {
        return op() == NewArrayBuffer;
    }
    
    NewArrayBufferData* newArrayBufferData()
    {
        ASSERT(hasConstantBuffer());
        return reinterpret_cast<NewArrayBufferData*>(m_opInfo);
    }
    
    unsigned startConstant()
    {
        return newArrayBufferData()->startConstant;
    }
    
    unsigned numConstants()
    {
        return newArrayBufferData()->numConstants;
    }
    
    bool hasIndexingType()
    {
        switch (op()) {
        case NewArray:
        case NewArrayWithSize:
        case NewArrayBuffer:
            return true;
        default:
            return false;
        }
    }
    
    IndexingType indexingType()
    {
        ASSERT(hasIndexingType());
        if (op() == NewArrayBuffer)
            return newArrayBufferData()->indexingType;
        return m_opInfo;
    }
    
    bool hasTypedArrayType()
    {
        switch (op()) {
        case NewTypedArray:
            return true;
        default:
            return false;
        }
    }
    
    TypedArrayType typedArrayType()
    {
        ASSERT(hasTypedArrayType());
        TypedArrayType result = static_cast<TypedArrayType>(m_opInfo);
        ASSERT(isTypedView(result));
        return result;
    }
    
    bool hasInlineCapacity()
    {
        return op() == CreateThis;
    }

    unsigned inlineCapacity()
    {
        ASSERT(hasInlineCapacity());
        return m_opInfo;
    }

    void setIndexingType(IndexingType indexingType)
    {
        ASSERT(hasIndexingType());
        m_opInfo = indexingType;
    }
    
    bool hasRegexpIndex()
    {
        return op() == NewRegexp;
    }
    
    unsigned regexpIndex()
    {
        ASSERT(hasRegexpIndex());
        return m_opInfo;
    }
    
    bool hasVarNumber()
    {
        return op() == GetClosureVar || op() == PutClosureVar;
    }

    int varNumber()
    {
        ASSERT(hasVarNumber());
        return m_opInfo;
    }
    
    bool hasRegisterPointer()
    {
        return op() == GetGlobalVar || op() == PutGlobalVar;
    }
    
    WriteBarrier<Unknown>* registerPointer()
    {
        return bitwise_cast<WriteBarrier<Unknown>*>(m_opInfo);
    }
    
    bool hasResult()
    {
        return !!result();
    }

    bool hasInt32Result()
    {
        return result() == NodeResultInt32;
    }
    
    bool hasInt52Result()
    {
        return result() == NodeResultInt52;
    }
    
    bool hasNumberResult()
    {
        return result() == NodeResultNumber;
    }
    
    bool hasDoubleResult()
    {
        return result() == NodeResultDouble;
    }
    
    bool hasJSResult()
    {
        return result() == NodeResultJS;
    }
    
    bool hasBooleanResult()
    {
        return result() == NodeResultBoolean;
    }

    bool hasStorageResult()
    {
        return result() == NodeResultStorage;
    }
    
    UseKind defaultUseKind()
    {
        return useKindForResult(result());
    }
    
    Edge defaultEdge()
    {
        return Edge(this, defaultUseKind());
    }

    bool isJump()
    {
        return op() == Jump;
    }

    bool isBranch()
    {
        return op() == Branch;
    }
    
    bool isSwitch()
    {
        return op() == Switch;
    }

    bool isTerminal()
    {
        switch (op()) {
        case Jump:
        case Branch:
        case Switch:
        case Return:
        case Unreachable:
            return true;
        default:
            return false;
        }
    }

    unsigned targetBytecodeOffsetDuringParsing()
    {
        ASSERT(isJump());
        return m_opInfo;
    }

    BasicBlock*& targetBlock()
    {
        ASSERT(isJump());
        return *bitwise_cast<BasicBlock**>(&m_opInfo);
    }
    
    BranchData* branchData()
    {
        ASSERT(isBranch());
        return bitwise_cast<BranchData*>(m_opInfo);
    }
    
    SwitchData* switchData()
    {
        ASSERT(isSwitch());
        return bitwise_cast<SwitchData*>(m_opInfo);
    }
    
    unsigned numSuccessors()
    {
        switch (op()) {
        case Jump:
            return 1;
        case Branch:
            return 2;
        case Switch:
            return switchData()->cases.size() + 1;
        default:
            return 0;
        }
    }
    
    BasicBlock*& successor(unsigned index)
    {
        if (isSwitch()) {
            if (index < switchData()->cases.size())
                return switchData()->cases[index].target.block;
            RELEASE_ASSERT(index == switchData()->cases.size());
            return switchData()->fallThrough.block;
        }
        switch (index) {
        case 0:
            if (isJump())
                return targetBlock();
            return branchData()->taken.block;
        case 1:
            return branchData()->notTaken.block;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return targetBlock();
        }
    }
    
    BasicBlock*& successorForCondition(bool condition)
    {
        return branchData()->forCondition(condition);
    }
    
    bool hasHeapPrediction()
    {
        switch (op()) {
        case GetDirectPname:
        case GetById:
        case GetByIdFlush:
        case GetByVal:
        case GetMyArgumentByVal:
        case GetMyArgumentByValSafe:
        case Call:
        case Construct:
        case ProfiledCall:
        case ProfiledConstruct:
        case NativeCall:
        case NativeConstruct:
        case GetByOffset:
        case MultiGetByOffset:
        case GetClosureVar:
        case ArrayPop:
        case ArrayPush:
        case RegExpExec:
        case RegExpTest:
        case GetGlobalVar:
            return true;
        default:
            return false;
        }
    }
    
    SpeculatedType getHeapPrediction()
    {
        ASSERT(hasHeapPrediction());
        return static_cast<SpeculatedType>(m_opInfo2);
    }
    
    bool predictHeap(SpeculatedType prediction)
    {
        ASSERT(hasHeapPrediction());
        
        return mergeSpeculation(m_opInfo2, prediction);
    }
    
    void setHeapPrediction(SpeculatedType prediction)
    {
        ASSERT(hasHeapPrediction());
        m_opInfo2 = prediction;
    }
    
    bool hasCellOperand()
    {
        switch (op()) {
        case AllocationProfileWatchpoint:
        case CheckCell:
        case NativeConstruct:
        case NativeCall:
            return true;
        default:
            return false;
        }
    }

    FrozenValue* cellOperand()
    {
        ASSERT(hasCellOperand());
        return reinterpret_cast<FrozenValue*>(m_opInfo);
    }
    
    void setCellOperand(FrozenValue* value)
    {
        ASSERT(hasCellOperand());
        m_opInfo = bitwise_cast<uintptr_t>(value);
    }
    
    bool hasVariableWatchpointSet()
    {
        return op() == NotifyWrite || op() == VariableWatchpoint;
    }
    
    VariableWatchpointSet* variableWatchpointSet()
    {
        return reinterpret_cast<VariableWatchpointSet*>(m_opInfo);
    }
    
    bool hasTypedArray()
    {
        return op() == TypedArrayWatchpoint;
    }
    
    JSArrayBufferView* typedArray()
    {
        return reinterpret_cast<JSArrayBufferView*>(m_opInfo);
    }
    
    bool hasStoragePointer()
    {
        return op() == ConstantStoragePointer;
    }
    
    void* storagePointer()
    {
        return reinterpret_cast<void*>(m_opInfo);
    }

    bool hasTransition()
    {
        switch (op()) {
        case PutStructure:
        case AllocatePropertyStorage:
        case ReallocatePropertyStorage:
            return true;
        default:
            return false;
        }
    }
    
    Transition* transition()
    {
        ASSERT(hasTransition());
        return reinterpret_cast<Transition*>(m_opInfo);
    }
    
    bool hasStructureSet()
    {
        switch (op()) {
        case CheckStructure:
            return true;
        default:
            return false;
        }
    }
    
    StructureSet& structureSet()
    {
        ASSERT(hasStructureSet());
        return *reinterpret_cast<StructureSet*>(m_opInfo);
    }
    
    bool hasStructure()
    {
        switch (op()) {
        case ArrayifyToStructure:
        case NewObject:
        case NewStringObject:
            return true;
        default:
            return false;
        }
    }
    
    Structure* structure()
    {
        ASSERT(hasStructure());
        return reinterpret_cast<Structure*>(m_opInfo);
    }
    
    bool hasStorageAccessData()
    {
        return op() == GetByOffset || op() == GetGetterSetterByOffset || op() == PutByOffset;
    }
    
    unsigned storageAccessDataIndex()
    {
        ASSERT(hasStorageAccessData());
        return m_opInfo;
    }
    
    bool hasMultiGetByOffsetData()
    {
        return op() == MultiGetByOffset;
    }
    
    MultiGetByOffsetData& multiGetByOffsetData()
    {
        return *reinterpret_cast<MultiGetByOffsetData*>(m_opInfo);
    }
    
    bool hasMultiPutByOffsetData()
    {
        return op() == MultiPutByOffset;
    }
    
    MultiPutByOffsetData& multiPutByOffsetData()
    {
        return *reinterpret_cast<MultiPutByOffsetData*>(m_opInfo);
    }
    
    bool hasFunctionDeclIndex()
    {
        return op() == NewFunction
            || op() == NewFunctionNoCheck;
    }
    
    unsigned functionDeclIndex()
    {
        ASSERT(hasFunctionDeclIndex());
        return m_opInfo;
    }
    
    bool hasFunctionExprIndex()
    {
        return op() == NewFunctionExpression;
    }
    
    unsigned functionExprIndex()
    {
        ASSERT(hasFunctionExprIndex());
        return m_opInfo;
    }
    
    bool hasSymbolTable()
    {
        return op() == FunctionReentryWatchpoint;
    }
    
    SymbolTable* symbolTable()
    {
        ASSERT(hasSymbolTable());
        return reinterpret_cast<SymbolTable*>(m_opInfo);
    }
    
    bool hasArrayMode()
    {
        switch (op()) {
        case GetIndexedPropertyStorage:
        case GetArrayLength:
        case PutByValDirect:
        case PutByVal:
        case PutByValAlias:
        case GetByVal:
        case StringCharAt:
        case StringCharCodeAt:
        case CheckArray:
        case Arrayify:
        case ArrayifyToStructure:
        case ArrayPush:
        case ArrayPop:
        case HasIndexedProperty:
            return true;
        default:
            return false;
        }
    }
    
    ArrayMode arrayMode()
    {
        ASSERT(hasArrayMode());
        if (op() == ArrayifyToStructure)
            return ArrayMode::fromWord(m_opInfo2);
        return ArrayMode::fromWord(m_opInfo);
    }
    
    bool setArrayMode(ArrayMode arrayMode)
    {
        ASSERT(hasArrayMode());
        if (this->arrayMode() == arrayMode)
            return false;
        m_opInfo = arrayMode.asWord();
        return true;
    }
    
    bool hasArithMode()
    {
        switch (op()) {
        case ArithAdd:
        case ArithSub:
        case ArithNegate:
        case ArithMul:
        case ArithDiv:
        case ArithMod:
        case UInt32ToNumber:
        case DoubleAsInt32:
            return true;
        default:
            return false;
        }
    }

    Arith::Mode arithMode()
    {
        ASSERT(hasArithMode());
        return static_cast<Arith::Mode>(m_opInfo);
    }
    
    void setArithMode(Arith::Mode mode)
    {
        m_opInfo = mode;
    }
    
    bool hasVirtualRegister()
    {
        return m_virtualRegister.isValid();
    }
    
    VirtualRegister virtualRegister()
    {
        ASSERT(hasResult());
        ASSERT(m_virtualRegister.isValid());
        return m_virtualRegister;
    }
    
    void setVirtualRegister(VirtualRegister virtualRegister)
    {
        ASSERT(hasResult());
        ASSERT(!m_virtualRegister.isValid());
        m_virtualRegister = virtualRegister;
    }
    
    bool hasExecutionCounter()
    {
        return op() == CountExecution;
    }
    
    Profiler::ExecutionCounter* executionCounter()
    {
        return bitwise_cast<Profiler::ExecutionCounter*>(m_opInfo);
    }

    bool shouldGenerate()
    {
        return m_refCount;
    }
    
    bool willHaveCodeGenOrOSR()
    {
        switch (op()) {
        case SetLocal:
        case MovHint:
        case ZombieHint:
        case PhantomArguments:
            return true;
        case Phantom:
        case HardPhantom:
            return child1().useKindUnchecked() != UntypedUse || child2().useKindUnchecked() != UntypedUse || child3().useKindUnchecked() != UntypedUse;
        default:
            return shouldGenerate();
        }
    }
    
    bool isSemanticallySkippable()
    {
        return op() == CountExecution;
    }

    unsigned refCount()
    {
        return m_refCount;
    }

    unsigned postfixRef()
    {
        return m_refCount++;
    }

    unsigned adjustedRefCount()
    {
        return mustGenerate() ? m_refCount - 1 : m_refCount;
    }
    
    void setRefCount(unsigned refCount)
    {
        m_refCount = refCount;
    }
    
    Edge& child1()
    {
        ASSERT(!(m_flags & NodeHasVarArgs));
        return children.child1();
    }
    
    // This is useful if you want to do a fast check on the first child
    // before also doing a check on the opcode. Use this with care and
    // avoid it if possible.
    Edge child1Unchecked()
    {
        return children.child1Unchecked();
    }

    Edge& child2()
    {
        ASSERT(!(m_flags & NodeHasVarArgs));
        return children.child2();
    }

    Edge& child3()
    {
        ASSERT(!(m_flags & NodeHasVarArgs));
        return children.child3();
    }
    
    unsigned firstChild()
    {
        ASSERT(m_flags & NodeHasVarArgs);
        return children.firstChild();
    }
    
    unsigned numChildren()
    {
        ASSERT(m_flags & NodeHasVarArgs);
        return children.numChildren();
    }
    
    UseKind binaryUseKind()
    {
        ASSERT(child1().useKind() == child2().useKind());
        return child1().useKind();
    }
    
    bool isBinaryUseKind(UseKind left, UseKind right)
    {
        return child1().useKind() == left && child2().useKind() == right;
    }
    
    bool isBinaryUseKind(UseKind useKind)
    {
        return isBinaryUseKind(useKind, useKind);
    }
    
    Edge childFor(UseKind useKind)
    {
        if (child1().useKind() == useKind)
            return child1();
        if (child2().useKind() == useKind)
            return child2();
        if (child3().useKind() == useKind)
            return child3();
        return Edge();
    }
    
    SpeculatedType prediction()
    {
        return m_prediction;
    }
    
    bool predict(SpeculatedType prediction)
    {
        return mergeSpeculation(m_prediction, prediction);
    }
    
    bool shouldSpeculateInt32()
    {
        return isInt32Speculation(prediction());
    }
    
    bool sawBooleans()
    {
        return !!(prediction() & SpecBoolean);
    }
    
    bool shouldSpeculateInt32OrBoolean()
    {
        return isInt32OrBooleanSpeculation(prediction());
    }
    
    bool shouldSpeculateInt32ForArithmetic()
    {
        return isInt32SpeculationForArithmetic(prediction());
    }
    
    bool shouldSpeculateInt32OrBooleanForArithmetic()
    {
        return isInt32OrBooleanSpeculationForArithmetic(prediction());
    }
    
    bool shouldSpeculateInt32OrBooleanExpectingDefined()
    {
        return isInt32OrBooleanSpeculationExpectingDefined(prediction());
    }
    
    bool shouldSpeculateMachineInt()
    {
        return isMachineIntSpeculation(prediction());
    }
    
    bool shouldSpeculateDouble()
    {
        return isDoubleSpeculation(prediction());
    }
    
    bool shouldSpeculateNumber()
    {
        return isFullNumberSpeculation(prediction());
    }
    
    bool shouldSpeculateNumberOrBoolean()
    {
        return isFullNumberOrBooleanSpeculation(prediction());
    }
    
    bool shouldSpeculateNumberOrBooleanExpectingDefined()
    {
        return isFullNumberOrBooleanSpeculationExpectingDefined(prediction());
    }
    
    bool shouldSpeculateBoolean()
    {
        return isBooleanSpeculation(prediction());
    }
    
    bool shouldSpeculateOther()
    {
        return isOtherSpeculation(prediction());
    }
    
    bool shouldSpeculateMisc()
    {
        return isMiscSpeculation(prediction());
    }
   
    bool shouldSpeculateStringIdent()
    {
        return isStringIdentSpeculation(prediction());
    }
    
    bool shouldSpeculateNotStringVar()
    {
        return isNotStringVarSpeculation(prediction());
    }
 
    bool shouldSpeculateString()
    {
        return isStringSpeculation(prediction());
    }
 
    bool shouldSpeculateStringObject()
    {
        return isStringObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateStringOrStringObject()
    {
        return isStringOrStringObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateFinalObject()
    {
        return isFinalObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateFinalObjectOrOther()
    {
        return isFinalObjectOrOtherSpeculation(prediction());
    }
    
    bool shouldSpeculateArray()
    {
        return isArraySpeculation(prediction());
    }
    
    bool shouldSpeculateArguments()
    {
        return isArgumentsSpeculation(prediction());
    }
    
    bool shouldSpeculateInt8Array()
    {
        return isInt8ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateInt16Array()
    {
        return isInt16ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateInt32Array()
    {
        return isInt32ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateUint8Array()
    {
        return isUint8ArraySpeculation(prediction());
    }

    bool shouldSpeculateUint8ClampedArray()
    {
        return isUint8ClampedArraySpeculation(prediction());
    }
    
    bool shouldSpeculateUint16Array()
    {
        return isUint16ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateUint32Array()
    {
        return isUint32ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateFloat32Array()
    {
        return isFloat32ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateFloat64Array()
    {
        return isFloat64ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateArrayOrOther()
    {
        return isArrayOrOtherSpeculation(prediction());
    }
    
    bool shouldSpeculateObject()
    {
        return isObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateObjectOrOther()
    {
        return isObjectOrOtherSpeculation(prediction());
    }

    bool shouldSpeculateCell()
    {
        return isCellSpeculation(prediction());
    }
    
    static bool shouldSpeculateBoolean(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateBoolean() && op2->shouldSpeculateBoolean();
    }
    
    static bool shouldSpeculateInt32(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateInt32() && op2->shouldSpeculateInt32();
    }
    
    static bool shouldSpeculateInt32OrBoolean(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateInt32OrBoolean()
            && op2->shouldSpeculateInt32OrBoolean();
    }
    
    static bool shouldSpeculateInt32OrBooleanForArithmetic(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateInt32OrBooleanForArithmetic()
            && op2->shouldSpeculateInt32OrBooleanForArithmetic();
    }
    
    static bool shouldSpeculateInt32OrBooleanExpectingDefined(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateInt32OrBooleanExpectingDefined()
            && op2->shouldSpeculateInt32OrBooleanExpectingDefined();
    }
    
    static bool shouldSpeculateMachineInt(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateMachineInt() && op2->shouldSpeculateMachineInt();
    }
    
    static bool shouldSpeculateNumber(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateNumber() && op2->shouldSpeculateNumber();
    }
    
    static bool shouldSpeculateNumberOrBoolean(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateNumberOrBoolean()
            && op2->shouldSpeculateNumberOrBoolean();
    }
    
    static bool shouldSpeculateNumberOrBooleanExpectingDefined(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateNumberOrBooleanExpectingDefined()
            && op2->shouldSpeculateNumberOrBooleanExpectingDefined();
    }
    
    static bool shouldSpeculateFinalObject(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateFinalObject() && op2->shouldSpeculateFinalObject();
    }

    static bool shouldSpeculateArray(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateArray() && op2->shouldSpeculateArray();
    }
    
    bool canSpeculateInt32(RareCaseProfilingSource source)
    {
        return nodeCanSpeculateInt32(arithNodeFlags(), source);
    }
    
    bool canSpeculateInt52(RareCaseProfilingSource source)
    {
        return nodeCanSpeculateInt52(arithNodeFlags(), source);
    }
    
    RareCaseProfilingSource sourceFor(PredictionPass pass)
    {
        if (pass == PrimaryPass || child1()->sawBooleans() || (child2() && child2()->sawBooleans()))
            return DFGRareCase;
        return AllRareCases;
    }
    
    bool canSpeculateInt32(PredictionPass pass)
    {
        return canSpeculateInt32(sourceFor(pass));
    }
    
    bool canSpeculateInt52(PredictionPass pass)
    {
        return canSpeculateInt52(sourceFor(pass));
    }
    
    void dumpChildren(PrintStream& out)
    {
        if (!child1())
            return;
        out.printf("@%u", child1()->index());
        if (!child2())
            return;
        out.printf(", @%u", child2()->index());
        if (!child3())
            return;
        out.printf(", @%u", child3()->index());
    }
    
    // NB. This class must have a trivial destructor.

    NodeOrigin origin;

    // References to up to 3 children, or links to a variable length set of children.
    AdjacencyList children;

private:
    unsigned m_op : 10; // real type is NodeType
    unsigned m_flags : 22;
    // The virtual register number (spill location) associated with this .
    VirtualRegister m_virtualRegister;
    // The number of uses of the result of this operation (+1 for 'must generate' nodes, which have side-effects).
    unsigned m_refCount;
    // The prediction ascribed to this node after propagation.
    SpeculatedType m_prediction;
    // Immediate values, accesses type-checked via accessors above. The first one is
    // big enough to store a pointer.
    uintptr_t m_opInfo;
    uintptr_t m_opInfo2;

public:
    // Fields used by various analyses.
    AbstractValue value;
    
    // Miscellaneous data that is usually meaningless, but can hold some analysis results
    // if you ask right. For example, if you do Graph::initializeNodeOwners(), Node::owner
    // will tell you which basic block a node belongs to. You cannot rely on this persisting
    // across transformations unless you do the maintenance work yourself. Other phases use
    // Node::replacement, but they do so manually: first you do Graph::clearReplacements()
    // and then you set, and use, replacement's yourself.
    //
    // Bottom line: don't use these fields unless you initialize them yourself, or by
    // calling some appropriate methods that initialize them the way you want. Otherwise,
    // these fields are meaningless.
    Node* replacement;
    BasicBlock* owner;
};

inline bool nodeComparator(Node* a, Node* b)
{
    return a->index() < b->index();
}

template<typename T>
CString nodeListDump(const T& nodeList)
{
    return sortedListDump(nodeList, nodeComparator);
}

template<typename T>
CString nodeMapDump(const T& nodeMap, DumpContext* context = 0)
{
    Vector<typename T::KeyType> keys;
    for (
        typename T::const_iterator iter = nodeMap.begin();
        iter != nodeMap.end(); ++iter)
        keys.append(iter->key);
    std::sort(keys.begin(), keys.end(), nodeComparator);
    StringPrintStream out;
    CommaPrinter comma;
    for(unsigned i = 0; i < keys.size(); ++i)
        out.print(comma, keys[i], "=>", inContext(nodeMap.get(keys[i]), context));
    return out.toCString();
}

} } // namespace JSC::DFG

namespace WTF {

void printInternal(PrintStream&, JSC::DFG::SwitchKind);
void printInternal(PrintStream&, JSC::DFG::Node*);

inline JSC::DFG::Node* inContext(JSC::DFG::Node* node, JSC::DumpContext*) { return node; }

} // namespace WTF

using WTF::inContext;

#endif
#endif
