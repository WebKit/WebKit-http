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

#ifndef DFGNodeType_h
#define DFGNodeType_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGNodeFlags.h"

namespace JSC { namespace DFG {

// This macro defines a set of information about all known node types, used to populate NodeId, NodeType below.
#define FOR_EACH_DFG_OP(macro) \
    /* A constant in the CodeBlock's constant pool. */\
    macro(JSConstant, NodeResultJS | NodeDoesNotExit) \
    \
    /* A constant not in the CodeBlock's constant pool. Uses get patched to jumps that exit the */\
    /* code block. */\
    macro(WeakJSConstant, NodeResultJS | NodeDoesNotExit) \
    \
    /* Marker to indicate that an operation was optimized entirely and all that is left */\
    /* is to make one node alias another. CSE will later usually eliminate this node, */\
    /* though it may choose not to if it would corrupt predictions (very rare). */\
    macro(Identity, NodeResultJS) \
    \
    /* Nodes for handling functions (both as call and as construct). */\
    macro(ToThis, NodeResultJS) \
    macro(CreateThis, NodeResultJS) /* Note this is not MustGenerate since we're returning it anyway. */ \
    macro(GetCallee, NodeResultJS) \
    \
    /* Nodes for local variable access. These nodes are linked together using Phi nodes. */\
    /* Any two nodes that are part of the same Phi graph will share the same */\
    /* VariableAccessData, and thus will share predictions. */\
    macro(GetLocal, NodeResultJS) \
    macro(SetLocal, NodeExitsForward) \
    macro(MovHintAndCheck, NodeMustGenerate | NodeExitsForward) \
    macro(MovHint, NodeDoesNotExit) \
    macro(ZombieHint, NodeDoesNotExit) \
    macro(GetArgument, NodeResultJS | NodeMustGenerate) \
    macro(Phantom, NodeMustGenerate) \
    macro(Upsilon, NodeDoesNotExit | NodeRelevantToOSR) \
    macro(Phi, NodeDoesNotExit | NodeRelevantToOSR) \
    macro(Flush, NodeMustGenerate | NodeDoesNotExit) \
    macro(PhantomLocal, NodeMustGenerate | NodeDoesNotExit) \
    \
    /* Hint that this is where bytecode thinks is a good place to OSR. Note that this */\
    /* will exist even in inlined loops. This has no execution semantics but it must */\
    /* survive all DCE. We treat this as being a can-exit because tier-up to FTL may */\
    /* want all state. */\
    macro(LoopHint, NodeMustGenerate) \
    \
    /* Special node for OSR entry into the FTL. Indicates that we're loading a local */\
    /* variable from the scratch buffer. */\
    macro(ExtractOSREntryLocal, NodeResultJS) \
    \
    /* Tier-up checks from the DFG to the FTL. */\
    macro(CheckTierUpInLoop, NodeMustGenerate) \
    macro(CheckTierUpAndOSREnter, NodeMustGenerate) \
    macro(CheckTierUpAtReturn, NodeMustGenerate) \
    \
    /* Get the value of a local variable, without linking into the VariableAccessData */\
    /* network. This is only valid for variable accesses whose predictions originated */\
    /* as something other than a local access, and thus had their own profiling. */\
    macro(GetLocalUnlinked, NodeResultJS) \
    \
    /* Marker for an argument being set at the prologue of a function. */\
    macro(SetArgument, NodeDoesNotExit) \
    \
    /* Marker of location in the IR where we may possibly perform jump replacement to */\
    /* invalidate this code block. */\
    macro(InvalidationPoint, NodeMustGenerate) \
    \
    /* Nodes for bitwise operations. */\
    macro(BitAnd, NodeResultInt32 | NodeMustGenerate) \
    macro(BitOr, NodeResultInt32 | NodeMustGenerate) \
    macro(BitXor, NodeResultInt32 | NodeMustGenerate) \
    macro(BitLShift, NodeResultInt32 | NodeMustGenerate) \
    macro(BitRShift, NodeResultInt32 | NodeMustGenerate) \
    macro(BitURShift, NodeResultInt32 | NodeMustGenerate) \
    /* Bitwise operators call ToInt32 on their operands. */\
    macro(ValueToInt32, NodeResultInt32) \
    /* Used to box the result of URShift nodes (result has range 0..2^32-1). */\
    macro(UInt32ToNumber, NodeResultNumber | NodeExitsForward) \
    \
    /* Used to cast known integers to doubles, so as to separate the double form */\
    /* of the value from the integer form. */\
    macro(Int32ToDouble, NodeResultNumber) \
    /* Used to speculate that a double value is actually an integer. */\
    macro(DoubleAsInt32, NodeResultInt32 | NodeExitsForward) \
    /* Used to separate representation and register allocation of Int52's represented */\
    /* as values. */\
    macro(Int52ToValue, NodeResultJS) \
    macro(Int52ToDouble, NodeResultNumber) \
    \
    /* Nodes for arithmetic operations. */\
    macro(ArithAdd, NodeResultNumber | NodeMustGenerate) \
    macro(ArithSub, NodeResultNumber | NodeMustGenerate) \
    macro(ArithNegate, NodeResultNumber | NodeMustGenerate) \
    macro(ArithMul, NodeResultNumber | NodeMustGenerate) \
    macro(ArithIMul, NodeResultInt32 | NodeMustGenerate) \
    macro(ArithDiv, NodeResultNumber | NodeMustGenerate) \
    macro(ArithMod, NodeResultNumber | NodeMustGenerate) \
    macro(ArithAbs, NodeResultNumber | NodeMustGenerate) \
    macro(ArithMin, NodeResultNumber | NodeMustGenerate) \
    macro(ArithMax, NodeResultNumber | NodeMustGenerate) \
    macro(ArithSqrt, NodeResultNumber | NodeMustGenerate) \
    macro(ArithSin, NodeResultNumber | NodeMustGenerate) \
    macro(ArithCos, NodeResultNumber | NodeMustGenerate) \
    \
    /* Add of values may either be arithmetic, or result in string concatenation. */\
    macro(ValueAdd, NodeResultJS | NodeMustGenerate | NodeMightClobber) \
    \
    /* Property access. */\
    /* PutByValAlias indicates a 'put' aliases a prior write to the same property. */\
    /* Since a put to 'length' may invalidate optimizations here, */\
    /* this must be the directly subsequent property put. Note that PutByVal */\
    /* opcodes use VarArgs beause they may have up to 4 children. */\
    macro(GetByVal, NodeResultJS | NodeMustGenerate | NodeMightClobber) \
    macro(PutByValDirect, NodeMustGenerate | NodeHasVarArgs | NodeMightClobber) \
    macro(PutByVal, NodeMustGenerate | NodeHasVarArgs | NodeMightClobber) \
    macro(PutByValAlias, NodeMustGenerate | NodeHasVarArgs | NodeMightClobber) \
    macro(GetById, NodeResultJS | NodeMustGenerate | NodeClobbersWorld) \
    macro(GetByIdFlush, NodeResultJS | NodeMustGenerate | NodeClobbersWorld) \
    macro(PutById, NodeMustGenerate | NodeClobbersWorld) \
    macro(PutByIdDirect, NodeMustGenerate | NodeClobbersWorld) \
    macro(CheckStructure, NodeMustGenerate) \
    macro(CheckExecutable, NodeMustGenerate) \
    /* Transition watchpoints are a contract between the party setting the watchpoint */\
    /* and the runtime system, where the party promises that the child object once had */\
    /* the structure being watched, and the runtime system in turn promises that the */\
    /* watchpoint will be turned into an OSR exit if any object with that structure */\
    /* ever transitions to a different structure. Hence, the child object must have */\
    /* previously had a CheckStructure executed on it or we're dealing with an object */\
    /* constant (WeakJSConstant) and the object was known to have that structure at */\
    /* compile-time. In the latter case this means that no structure checks have to be */\
    /* performed for this object by JITted code. In the former case this means that*/\
    /* the object's structure does not need to be rechecked due to side-effecting */\
    /* (clobbering) operations. */\
    macro(StructureTransitionWatchpoint, NodeMustGenerate) \
    macro(PutStructure, NodeMustGenerate) \
    macro(PhantomPutStructure, NodeMustGenerate | NodeDoesNotExit) \
    macro(AllocatePropertyStorage, NodeMustGenerate | NodeDoesNotExit | NodeResultStorage) \
    macro(ReallocatePropertyStorage, NodeMustGenerate | NodeDoesNotExit | NodeResultStorage) \
    macro(GetButterfly, NodeResultStorage) \
    macro(CheckArray, NodeMustGenerate) \
    macro(Arrayify, NodeMustGenerate) \
    macro(ArrayifyToStructure, NodeMustGenerate) \
    macro(GetIndexedPropertyStorage, NodeResultStorage) \
    macro(GetByOffset, NodeResultJS) \
    macro(PutByOffset, NodeMustGenerate) \
    macro(GetArrayLength, NodeResultInt32) \
    macro(GetTypedArrayByteOffset, NodeResultInt32) \
    macro(GetScope, NodeResultJS) \
    macro(GetMyScope, NodeResultJS) \
    macro(SkipTopScope, NodeResultJS) \
    macro(SkipScope, NodeResultJS) \
    macro(GetClosureRegisters, NodeResultStorage) \
    macro(GetClosureVar, NodeResultJS) \
    macro(PutClosureVar, NodeMustGenerate) \
    macro(GetGlobalVar, NodeResultJS) \
    macro(PutGlobalVar, NodeMustGenerate) \
    macro(GlobalVarWatchpoint, NodeMustGenerate) \
    macro(VarInjectionWatchpoint, NodeMustGenerate) \
    macro(CheckFunction, NodeMustGenerate) \
    macro(AllocationProfileWatchpoint, NodeMustGenerate) \
    \
    /* Optimizations for array mutation. */\
    macro(ArrayPush, NodeResultJS | NodeMustGenerate | NodeClobbersWorld) \
    macro(ArrayPop, NodeResultJS | NodeMustGenerate | NodeClobbersWorld) \
    \
    /* Optimizations for regular expression matching. */\
    macro(RegExpExec, NodeResultJS | NodeMustGenerate) \
    macro(RegExpTest, NodeResultJS | NodeMustGenerate) \
    \
    /* Optimizations for string access */ \
    macro(StringCharCodeAt, NodeResultInt32) \
    macro(StringCharAt, NodeResultJS) \
    macro(StringFromCharCode, NodeResultJS) \
    \
    /* Nodes for comparison operations. */\
    macro(CompareLess, NodeResultBoolean | NodeMustGenerate | NodeMightClobber) \
    macro(CompareLessEq, NodeResultBoolean | NodeMustGenerate | NodeMightClobber) \
    macro(CompareGreater, NodeResultBoolean | NodeMustGenerate | NodeMightClobber) \
    macro(CompareGreaterEq, NodeResultBoolean | NodeMustGenerate | NodeMightClobber) \
    macro(CompareEq, NodeResultBoolean | NodeMustGenerate | NodeMightClobber) \
    macro(CompareEqConstant, NodeResultBoolean | NodeMustGenerate) \
    macro(CompareStrictEq, NodeResultBoolean) \
    macro(CompareStrictEqConstant, NodeResultBoolean) \
    \
    /* Calls. */\
    macro(Call, NodeResultJS | NodeMustGenerate | NodeHasVarArgs | NodeClobbersWorld) \
    macro(Construct, NodeResultJS | NodeMustGenerate | NodeHasVarArgs | NodeClobbersWorld) \
    \
    /* Allocations. */\
    macro(NewObject, NodeResultJS) \
    macro(NewArray, NodeResultJS | NodeHasVarArgs) \
    macro(NewArrayWithSize, NodeResultJS) \
    macro(NewArrayBuffer, NodeResultJS) \
    macro(NewTypedArray, NodeResultJS | NodeClobbersWorld | NodeMustGenerate) \
    macro(NewRegexp, NodeResultJS) \
    \
    /* Nodes for misc operations. */\
    macro(Breakpoint, NodeMustGenerate | NodeClobbersWorld) \
    macro(CheckHasInstance, NodeMustGenerate) \
    macro(InstanceOf, NodeResultBoolean) \
    macro(IsUndefined, NodeResultBoolean) \
    macro(IsBoolean, NodeResultBoolean) \
    macro(IsNumber, NodeResultBoolean) \
    macro(IsString, NodeResultBoolean) \
    macro(IsObject, NodeResultBoolean) \
    macro(IsFunction, NodeResultBoolean) \
    macro(TypeOf, NodeResultJS) \
    macro(LogicalNot, NodeResultBoolean) \
    macro(ToPrimitive, NodeResultJS | NodeMustGenerate | NodeClobbersWorld) \
    macro(ToString, NodeResultJS | NodeMustGenerate | NodeMightClobber) \
    macro(NewStringObject, NodeResultJS) \
    macro(MakeRope, NodeResultJS) \
    macro(In, NodeResultBoolean | NodeMustGenerate | NodeClobbersWorld) \
    \
    /* Nodes used for activations. Activation support works by having it anchored at */\
    /* epilgoues via TearOffActivation, and all CreateActivation nodes kept alive by */\
    /* being threaded with each other. */\
    macro(CreateActivation, NodeResultJS) \
    macro(TearOffActivation, NodeMustGenerate) \
    \
    /* Nodes used for arguments. Similar to activation support, only it makes even less */\
    /* sense. */\
    macro(CreateArguments, NodeResultJS) \
    macro(PhantomArguments, NodeResultJS | NodeDoesNotExit) \
    macro(TearOffArguments, NodeMustGenerate) \
    macro(GetMyArgumentsLength, NodeResultJS | NodeMustGenerate) \
    macro(GetMyArgumentByVal, NodeResultJS | NodeMustGenerate) \
    macro(GetMyArgumentsLengthSafe, NodeResultJS | NodeMustGenerate | NodeClobbersWorld) \
    macro(GetMyArgumentByValSafe, NodeResultJS | NodeMustGenerate | NodeClobbersWorld) \
    macro(CheckArgumentsNotCreated, NodeMustGenerate) \
    \
    /* Nodes for creating functions. */\
    macro(NewFunctionNoCheck, NodeResultJS) \
    macro(NewFunction, NodeResultJS) \
    macro(NewFunctionExpression, NodeResultJS) \
    \
    /* These aren't terminals but always exit */ \
    macro(Throw, NodeMustGenerate) \
    macro(ThrowReferenceError, NodeMustGenerate) \
    \
    /* Block terminals. */\
    macro(Jump, NodeMustGenerate) \
    macro(Branch, NodeMustGenerate) \
    macro(Switch, NodeMustGenerate) \
    macro(Return, NodeMustGenerate) \
    macro(Unreachable, NodeMustGenerate) \
    \
    /* Count execution. */\
    macro(CountExecution, NodeMustGenerate) \
    \
    /* This is a pseudo-terminal. It means that execution should fall out of DFG at */\
    /* this point, but execution does continue in the basic block - just in a */\
    /* different compiler. */\
    macro(ForceOSRExit, NodeMustGenerate) \
    \
    /* Checks the watchdog timer. If the timer has fired, we OSR exit to the */ \
    /* baseline JIT to redo the watchdog timer check, and service the timer. */ \
    macro(CheckWatchdogTimer, NodeMustGenerate) \

// This enum generates a monotonically increasing id for all Node types,
// and is used by the subsequent enum to fill out the id (as accessed via the NodeIdMask).
enum NodeType {
#define DFG_OP_ENUM(opcode, flags) opcode,
    FOR_EACH_DFG_OP(DFG_OP_ENUM)
#undef DFG_OP_ENUM
    LastNodeType
};

// Specifies the default flags for each node.
inline NodeFlags defaultFlags(NodeType op)
{
    switch (op) {
#define DFG_OP_ENUM(opcode, flags) case opcode: return flags;
    FOR_EACH_DFG_OP(DFG_OP_ENUM)
#undef DFG_OP_ENUM
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

inline bool permitsOSRBackwardRewiring(NodeType op)
{
    switch (op) {
    case Identity:
        RELEASE_ASSERT_NOT_REACHED();
        return true;
    case UInt32ToNumber:
    case Int52ToValue:
    case Int52ToDouble:
        // These are the only node where we do:
        //
        //     b: UInt32ToNumber(@a)
        //     c: SetLocal(@b)
        //
        // and then also have some uses of @a without Phantom'ing @b.
        return true;
    default:
        return false;
    }
}

// Returns the priority with which we should select the given node for forward
// rewiring. Higher is better. Zero means that the node is not useful for rewiring.
// By convention, we use 100 to mean that the node is totally equivalent to its
// input with no information loss.
inline unsigned forwardRewiringSelectionScore(NodeType op)
{
    switch (op) {
    case Identity:
        // We shouldn't see these by the time we get to OSR even though it clearly
        // is a perfect identity function.
        RELEASE_ASSERT_NOT_REACHED();
        return 100;
        
    case DoubleAsInt32:
        // This speculates that the incoming double is convertible to an int32. So
        // its result is totally equivalent.
        return 100;
        
    case Int32ToDouble:
        // This converts an int32 to a double, but that loses a bit of information.
        // OTOH it's still an equivalent number.
        return 75;
        
    case UInt32ToNumber:
        // It's completely fine to use this for OSR exit, since the uint32 isn't
        // actually representable in bytecode.
        return 100;

    case ValueToInt32:
        // This loses information. Only use it if there are no better alternatives.
        return 25;
        
    case Int52ToValue:
        // Loses no information. It just boxes the value, which is what OSR wants
        // to do anyway.
        return 100;
        
    case Int52ToDouble:
        // This is like Int32ToDouble; we can use it because it gives a semantically
        // equivalent value but that value may be an int32 in a double, so we'd
        // rather not if we can avoid it.
        return 75;
        
    default:
        return 0;
    }
}

inline bool permitsOSRForwardRewiring(NodeType op)
{
    return forwardRewiringSelectionScore(op) > 0;
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGNodeType_h

