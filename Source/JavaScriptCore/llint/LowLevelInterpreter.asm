# Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

# First come the common protocols that both interpreters use. Note that each
# of these must have an ASSERT() in LLIntData.cpp

# These declarations must match interpreter/RegisterFile.h.
const CallFrameHeaderSize = 48
const ArgumentCount = -48
const CallerFrame = -40
const Callee = -32
const ScopeChain = -24
const ReturnPC = -16
const CodeBlock = -8

const ThisArgumentOffset = -CallFrameHeaderSize - 8

# Some register conventions.
if JSVALUE64
    # - Use a pair of registers to represent the PC: one register for the
    #   base of the register file, and one register for the index.
    # - The PC base (or PB for short) should be stored in the csr. It will
    #   get clobbered on calls to other JS code, but will get saved on calls
    #   to C functions.
    # - C calls are still given the Instruction* rather than the PC index.
    #   This requires an add before the call, and a sub after.
    const PC = t4
    const PB = t6
    const tagTypeNumber = csr1
    const tagMask = csr2
else
    const PC = t4
end

# Constants for reasoning about value representation.
if BIG_ENDIAN
    const TagOffset = 0
    const PayloadOffset = 4
else
    const TagOffset = 4
    const PayloadOffset = 0
end

# Constant for reasoning about butterflies.
const IsArray = 1
const HasArrayStorage = 8
const AllArrayTypes = 15

# Type constants.
const StringType = 5
const ObjectType = 13

# Type flags constants.
const MasqueradesAsUndefined = 1
const ImplementsHasInstance = 2
const ImplementsDefaultHasInstance = 8

# Bytecode operand constants.
const FirstConstantRegisterIndex = 0x40000000

# Code type constants.
const GlobalCode = 0
const EvalCode = 1
const FunctionCode = 2

# The interpreter steals the tag word of the argument count.
const LLIntReturnPC = ArgumentCount + TagOffset

# String flags.
const HashFlags8BitBuffer = 64

# Property storage constants
if JSVALUE64
    const InlineStorageCapacity = 6
else
    const InlineStorageCapacity = 7
end

# Allocation constants
if JSVALUE64
    const JSFinalObjectSizeClassIndex = 1
else
    const JSFinalObjectSizeClassIndex = 3
end

# This must match wtf/Vector.h
if JSVALUE64
    const VectorSizeOffset = 0
    const VectorBufferOffset = 8
else
    const VectorSizeOffset = 0
    const VectorBufferOffset = 4
end


# Some common utilities.
macro crash()
    if C_LOOP
        cloopCrash
    else
        storei t0, 0xbbadbeef[]
        move 0, t0
        call t0
    end
end

macro assert(assertion)
    if ASSERT_ENABLED
        assertion(.ok)
        crash()
    .ok:
    end
end

macro preserveReturnAddressAfterCall(destinationRegister)
    if C_LOOP
        # In our case, we're only preserving the bytecode vPC. 
        move lr, destinationRegister
    elsif ARMv7
        move lr, destinationRegister
    elsif X86 or X86_64
        pop destinationRegister
    else
        error
    end
end

macro restoreReturnAddressBeforeReturn(sourceRegister)
    if C_LOOP
        # In our case, we're only restoring the bytecode vPC. 
        move sourceRegister, lr
    elsif ARMv7
        move sourceRegister, lr
    elsif X86 or X86_64
        push sourceRegister
    else
        error
    end
end

macro traceExecution()
    if EXECUTION_TRACING
        callSlowPath(_llint_trace)
    end
end

macro callTargetFunction(callLinkInfo)
    if C_LOOP
        cloopCallJSFunction LLIntCallLinkInfo::machineCodeTarget[callLinkInfo]
    else
        call LLIntCallLinkInfo::machineCodeTarget[callLinkInfo]
        dispatchAfterCall()
    end
end

macro slowPathForCall(advance, slowPath)
    callCallSlowPath(
        advance,
        slowPath,
        macro (callee)
            if C_LOOP
                cloopCallJSFunction callee
            else
                call callee
                dispatchAfterCall()
            end
        end)
end

macro arrayProfile(structureAndIndexingType, profile, scratch)
    const structure = structureAndIndexingType
    const indexingType = structureAndIndexingType
    if VALUE_PROFILER
        storep structure, ArrayProfile::m_lastSeenStructure[profile]
        loadb Structure::m_indexingType[structure], indexingType
        move 1, scratch
        lshifti indexingType, scratch
        ori scratch, ArrayProfile::m_observedArrayModes[profile]
    else
        loadb Structure::m_indexingType[structure], indexingType
    end
end

macro checkSwitchToJIT(increment, action)
    if JIT_ENABLED
        loadp CodeBlock[cfr], t0
        baddis increment, CodeBlock::m_llintExecuteCounter + ExecutionCounter::m_counter[t0], .continue
        action()
    .continue:
    end
end

macro checkSwitchToJITForEpilogue()
    checkSwitchToJIT(
        10,
        macro ()
            callSlowPath(_llint_replace)
        end)
end

macro assertNotConstant(index)
    assert(macro (ok) bilt index, FirstConstantRegisterIndex, ok end)
end

macro functionForCallCodeBlockGetter(targetRegister)
    loadp Callee[cfr], targetRegister
    loadp JSFunction::m_executable[targetRegister], targetRegister
    loadp FunctionExecutable::m_codeBlockForCall[targetRegister], targetRegister
end

macro functionForConstructCodeBlockGetter(targetRegister)
    loadp Callee[cfr], targetRegister
    loadp JSFunction::m_executable[targetRegister], targetRegister
    loadp FunctionExecutable::m_codeBlockForConstruct[targetRegister], targetRegister
end

macro notFunctionCodeBlockGetter(targetRegister)
    loadp CodeBlock[cfr], targetRegister
end

macro functionCodeBlockSetter(sourceRegister)
    storep sourceRegister, CodeBlock[cfr]
end

macro notFunctionCodeBlockSetter(sourceRegister)
    # Nothing to do!
end

# Do the bare minimum required to execute code. Sets up the PC, leave the CodeBlock*
# in t1. May also trigger prologue entry OSR.
macro prologue(codeBlockGetter, codeBlockSetter, osrSlowPath, traceSlowPath)
    preserveReturnAddressAfterCall(t2)
    
    # Set up the call frame and check if we should OSR.
    storep t2, ReturnPC[cfr]
    if EXECUTION_TRACING
        callSlowPath(traceSlowPath)
    end
    codeBlockGetter(t1)
    if JIT_ENABLED
        baddis 5, CodeBlock::m_llintExecuteCounter + ExecutionCounter::m_counter[t1], .continue
        cCall2(osrSlowPath, cfr, PC)
        move t1, cfr
        btpz t0, .recover
        loadp ReturnPC[cfr], t2
        restoreReturnAddressBeforeReturn(t2)
        jmp t0
    .recover:
        codeBlockGetter(t1)
    .continue:
    end
    codeBlockSetter(t1)
    
    # Set up the PC.
    if JSVALUE64
        loadp CodeBlock::m_instructions[t1], PB
        move 0, PC
    else
        loadp CodeBlock::m_instructions[t1], PC
    end
end

# Expects that CodeBlock is in t1, which is what prologue() leaves behind.
# Must call dispatch(0) after calling this.
macro functionInitialization(profileArgSkip)
    if VALUE_PROFILER
        # Profile the arguments. Unfortunately, we have no choice but to do this. This
        # code is pretty horrendous because of the difference in ordering between
        # arguments and value profiles, the desire to have a simple loop-down-to-zero
        # loop, and the desire to use only three registers so as to preserve the PC and
        # the code block. It is likely that this code should be rewritten in a more
        # optimal way for architectures that have more than five registers available
        # for arbitrary use in the interpreter.
        loadi CodeBlock::m_numParameters[t1], t0
        addp -profileArgSkip, t0 # Use addi because that's what has the peephole
        assert(macro (ok) bpgteq t0, 0, ok end)
        btpz t0, .argumentProfileDone
        loadp CodeBlock::m_argumentValueProfiles + VectorBufferOffset[t1], t3
        mulp sizeof ValueProfile, t0, t2 # Aaaaahhhh! Need strength reduction!
        negp t0
        lshiftp 3, t0
        addp t2, t3
    .argumentProfileLoop:
        if JSVALUE64
            loadp ThisArgumentOffset + 8 - profileArgSkip * 8[cfr, t0], t2
            subp sizeof ValueProfile, t3
            storep t2, profileArgSkip * sizeof ValueProfile + ValueProfile::m_buckets[t3]
        else
            loadi ThisArgumentOffset + TagOffset + 8 - profileArgSkip * 8[cfr, t0], t2
            subp sizeof ValueProfile, t3
            storei t2, profileArgSkip * sizeof ValueProfile + ValueProfile::m_buckets + TagOffset[t3]
            loadi ThisArgumentOffset + PayloadOffset + 8 - profileArgSkip * 8[cfr, t0], t2
            storei t2, profileArgSkip * sizeof ValueProfile + ValueProfile::m_buckets + PayloadOffset[t3]
        end
        baddpnz 8, t0, .argumentProfileLoop
    .argumentProfileDone:
    end
        
    # Check stack height.
    loadi CodeBlock::m_numCalleeRegisters[t1], t0
    loadp CodeBlock::m_globalData[t1], t2
    loadp JSGlobalData::interpreter[t2], t2   # FIXME: Can get to the RegisterFile from the JITStackFrame
    lshifti 3, t0
    addp t0, cfr, t0
    bpaeq Interpreter::m_registerFile + RegisterFile::m_end[t2], t0, .stackHeightOK

    # Stack height check failed - need to call a slow_path.
    callSlowPath(_llint_register_file_check)
.stackHeightOK:
end

macro allocateBasicJSObject(sizeClassIndex, structure, result, scratch1, scratch2, slowCase)
    if ALWAYS_ALLOCATE_SLOW
        jmp slowCase
    else
        const offsetOfMySizeClass =
            JSGlobalData::heap +
            Heap::m_objectSpace +
            MarkedSpace::m_normalSpace +
            MarkedSpace::Subspace::preciseAllocators +
            sizeClassIndex * sizeof MarkedAllocator
        
        const offsetOfFirstFreeCell = 
            MarkedAllocator::m_freeList + 
            MarkedBlock::FreeList::head

        # FIXME: we can get the global data in one load from the stack.
        loadp CodeBlock[cfr], scratch1
        loadp CodeBlock::m_globalData[scratch1], scratch1
        
        # Get the object from the free list.   
        loadp offsetOfMySizeClass + offsetOfFirstFreeCell[scratch1], result
        btpz result, slowCase
        
        # Remove the object from the free list.
        loadp [result], scratch2
        storep scratch2, offsetOfMySizeClass + offsetOfFirstFreeCell[scratch1]
    
        # Initialize the object.
        storep structure, JSCell::m_structure[result]
        storep 0, JSObject::m_butterfly[result]
    end
end

macro doReturn()
    loadp ReturnPC[cfr], t2
    loadp CallerFrame[cfr], cfr
    restoreReturnAddressBeforeReturn(t2)
    ret
end


# Indicate the beginning of LLInt.
_llint_begin:
    crash()


_llint_program_prologue:
    prologue(notFunctionCodeBlockGetter, notFunctionCodeBlockSetter, _llint_entry_osr, _llint_trace_prologue)
    dispatch(0)


_llint_eval_prologue:
    prologue(notFunctionCodeBlockGetter, notFunctionCodeBlockSetter, _llint_entry_osr, _llint_trace_prologue)
    dispatch(0)


_llint_function_for_call_prologue:
    prologue(functionForCallCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_call, _llint_trace_prologue_function_for_call)
.functionForCallBegin:
    functionInitialization(0)
    dispatch(0)
    

_llint_function_for_construct_prologue:
    prologue(functionForConstructCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_construct, _llint_trace_prologue_function_for_construct)
.functionForConstructBegin:
    functionInitialization(1)
    dispatch(0)
    

_llint_function_for_call_arity_check:
    prologue(functionForCallCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_call_arityCheck, _llint_trace_arityCheck_for_call)
    functionArityCheck(.functionForCallBegin, _llint_slow_path_call_arityCheck)


_llint_function_for_construct_arity_check:
    prologue(functionForConstructCodeBlockGetter, functionCodeBlockSetter, _llint_entry_osr_function_for_construct_arityCheck, _llint_trace_arityCheck_for_construct)
    functionArityCheck(.functionForConstructBegin, _llint_slow_path_construct_arityCheck)


# Value-representation-specific code.
if JSVALUE64
    include LowLevelInterpreter64
else
    include LowLevelInterpreter32_64
end


# Value-representation-agnostic code.
_llint_op_new_array:
    traceExecution()
    callSlowPath(_llint_slow_path_new_array)
    dispatch(4)


_llint_op_new_array_buffer:
    traceExecution()
    callSlowPath(_llint_slow_path_new_array_buffer)
    dispatch(4)


_llint_op_new_regexp:
    traceExecution()
    callSlowPath(_llint_slow_path_new_regexp)
    dispatch(3)


_llint_op_less:
    traceExecution()
    callSlowPath(_llint_slow_path_less)
    dispatch(4)


_llint_op_lesseq:
    traceExecution()
    callSlowPath(_llint_slow_path_lesseq)
    dispatch(4)


_llint_op_greater:
    traceExecution()
    callSlowPath(_llint_slow_path_greater)
    dispatch(4)


_llint_op_greatereq:
    traceExecution()
    callSlowPath(_llint_slow_path_greatereq)
    dispatch(4)


_llint_op_mod:
    traceExecution()
    callSlowPath(_llint_slow_path_mod)
    dispatch(4)


_llint_op_typeof:
    traceExecution()
    callSlowPath(_llint_slow_path_typeof)
    dispatch(3)


_llint_op_is_object:
    traceExecution()
    callSlowPath(_llint_slow_path_is_object)
    dispatch(3)


_llint_op_is_function:
    traceExecution()
    callSlowPath(_llint_slow_path_is_function)
    dispatch(3)


_llint_op_in:
    traceExecution()
    callSlowPath(_llint_slow_path_in)
    dispatch(4)


_llint_op_resolve:
    traceExecution()
    callSlowPath(_llint_slow_path_resolve)
    dispatch(4)


_llint_op_resolve_skip:
    traceExecution()
    callSlowPath(_llint_slow_path_resolve_skip)
    dispatch(5)


_llint_op_resolve_base:
    traceExecution()
    callSlowPath(_llint_slow_path_resolve_base)
    dispatch(5)


_llint_op_ensure_property_exists:
    traceExecution()
    callSlowPath(_llint_slow_path_ensure_property_exists)
    dispatch(3)


_llint_op_resolve_with_base:
    traceExecution()
    callSlowPath(_llint_slow_path_resolve_with_base)
    dispatch(5)


_llint_op_resolve_with_this:
    traceExecution()
    callSlowPath(_llint_slow_path_resolve_with_this)
    dispatch(5)


macro withInlineStorage(object, propertyStorage, continuation)
    # Indicate that the object is the property storage, and that the
    # property storage register is unused.
    continuation(object, propertyStorage)
end

macro withOutOfLineStorage(object, propertyStorage, continuation)
    loadp JSObject::m_butterfly[object], propertyStorage
    # Indicate that the propertyStorage register now points to the
    # property storage, and that the object register may be reused
    # if the object pointer is not needed anymore.
    continuation(propertyStorage, object)
end


_llint_op_del_by_id:
    traceExecution()
    callSlowPath(_llint_slow_path_del_by_id)
    dispatch(4)


_llint_op_del_by_val:
    traceExecution()
    callSlowPath(_llint_slow_path_del_by_val)
    dispatch(4)


_llint_op_put_by_index:
    traceExecution()
    callSlowPath(_llint_slow_path_put_by_index)
    dispatch(4)


_llint_op_put_getter_setter:
    traceExecution()
    callSlowPath(_llint_slow_path_put_getter_setter)
    dispatch(5)


_llint_op_jmp_scopes:
    traceExecution()
    callSlowPath(_llint_slow_path_jmp_scopes)
    dispatch(0)


_llint_op_loop_if_true:
    traceExecution()
    jumpTrueOrFalse(
        macro (value, target) btinz value, target end,
        _llint_slow_path_jtrue)


_llint_op_jtrue:
    traceExecution()
    jumpTrueOrFalse(
        macro (value, target) btinz value, target end,
        _llint_slow_path_jtrue)


_llint_op_loop_if_false:
    traceExecution()
    jumpTrueOrFalse(
        macro (value, target) btiz value, target end,
        _llint_slow_path_jfalse)


_llint_op_jfalse:
    traceExecution()
    jumpTrueOrFalse(
        macro (value, target) btiz value, target end,
        _llint_slow_path_jfalse)


_llint_op_loop_if_less:
    traceExecution()
    compare(
        macro (left, right, target) bilt left, right, target end,
        macro (left, right, target) bdlt left, right, target end,
        _llint_slow_path_jless)


_llint_op_jless:
    traceExecution()
    compare(
        macro (left, right, target) bilt left, right, target end,
        macro (left, right, target) bdlt left, right, target end,
        _llint_slow_path_jless)


_llint_op_jnless:
    traceExecution()
    compare(
        macro (left, right, target) bigteq left, right, target end,
        macro (left, right, target) bdgtequn left, right, target end,
        _llint_slow_path_jnless)


_llint_op_loop_if_greater:
    traceExecution()
    compare(
        macro (left, right, target) bigt left, right, target end,
        macro (left, right, target) bdgt left, right, target end,
        _llint_slow_path_jgreater)


_llint_op_jgreater:
    traceExecution()
    compare(
        macro (left, right, target) bigt left, right, target end,
        macro (left, right, target) bdgt left, right, target end,
        _llint_slow_path_jgreater)


_llint_op_jngreater:
    traceExecution()
    compare(
        macro (left, right, target) bilteq left, right, target end,
        macro (left, right, target) bdltequn left, right, target end,
        _llint_slow_path_jngreater)


_llint_op_loop_if_lesseq:
    traceExecution()
    compare(
        macro (left, right, target) bilteq left, right, target end,
        macro (left, right, target) bdlteq left, right, target end,
        _llint_slow_path_jlesseq)


_llint_op_jlesseq:
    traceExecution()
    compare(
        macro (left, right, target) bilteq left, right, target end,
        macro (left, right, target) bdlteq left, right, target end,
        _llint_slow_path_jlesseq)


_llint_op_jnlesseq:
    traceExecution()
    compare(
        macro (left, right, target) bigt left, right, target end,
        macro (left, right, target) bdgtun left, right, target end,
        _llint_slow_path_jnlesseq)


_llint_op_loop_if_greatereq:
    traceExecution()
    compare(
        macro (left, right, target) bigteq left, right, target end,
        macro (left, right, target) bdgteq left, right, target end,
        _llint_slow_path_jgreatereq)


_llint_op_jgreatereq:
    traceExecution()
    compare(
        macro (left, right, target) bigteq left, right, target end,
        macro (left, right, target) bdgteq left, right, target end,
        _llint_slow_path_jgreatereq)


_llint_op_jngreatereq:
    traceExecution()
    compare(
        macro (left, right, target) bilt left, right, target end,
        macro (left, right, target) bdltun left, right, target end,
        _llint_slow_path_jngreatereq)


_llint_op_loop_hint:
    traceExecution()
    checkSwitchToJITForLoop()
    dispatch(1)


_llint_op_switch_string:
    traceExecution()
    callSlowPath(_llint_slow_path_switch_string)
    dispatch(0)


_llint_op_new_func_exp:
    traceExecution()
    callSlowPath(_llint_slow_path_new_func_exp)
    dispatch(3)


_llint_op_call:
    traceExecution()
    arrayProfileForCall()
    doCall(_llint_slow_path_call)


_llint_op_construct:
    traceExecution()
    doCall(_llint_slow_path_construct)


_llint_op_call_varargs:
    traceExecution()
    slowPathForCall(6, _llint_slow_path_call_varargs)


_llint_op_call_eval:
    traceExecution()
    
    # Eval is executed in one of two modes:
    #
    # 1) We find that we're really invoking eval() in which case the
    #    execution is perfomed entirely inside the slow_path, and it
    #    returns the PC of a function that just returns the return value
    #    that the eval returned.
    #
    # 2) We find that we're invoking something called eval() that is not
    #    the real eval. Then the slow_path returns the PC of the thing to
    #    call, and we call it.
    #
    # This allows us to handle two cases, which would require a total of
    # up to four pieces of state that cannot be easily packed into two
    # registers (C functions can return up to two registers, easily):
    #
    # - The call frame register. This may or may not have been modified
    #   by the slow_path, but the convention is that it returns it. It's not
    #   totally clear if that's necessary, since the cfr is callee save.
    #   But that's our style in this here interpreter so we stick with it.
    #
    # - A bit to say if the slow_path successfully executed the eval and has
    #   the return value, or did not execute the eval but has a PC for us
    #   to call.
    #
    # - Either:
    #   - The JS return value (two registers), or
    #
    #   - The PC to call.
    #
    # It turns out to be easier to just always have this return the cfr
    # and a PC to call, and that PC may be a dummy thunk that just
    # returns the JS value that the eval returned.
    
    slowPathForCall(4, _llint_slow_path_call_eval)


_llint_generic_return_point:
    dispatchAfterCall()


_llint_op_strcat:
    traceExecution()
    callSlowPath(_llint_slow_path_strcat)
    dispatch(4)


_llint_op_method_check:
    traceExecution()
    # We ignore method checks and use normal get_by_id optimizations.
    dispatch(1)


_llint_op_get_pnames:
    traceExecution()
    callSlowPath(_llint_slow_path_get_pnames)
    dispatch(0) # The slow_path either advances the PC or jumps us to somewhere else.


_llint_op_push_with_scope:
    traceExecution()
    callSlowPath(_llint_slow_path_push_with_scope)
    dispatch(2)


_llint_op_pop_scope:
    traceExecution()
    callSlowPath(_llint_slow_path_pop_scope)
    dispatch(1)


_llint_op_push_name_scope:
    traceExecution()
    callSlowPath(_llint_slow_path_push_name_scope)
    dispatch(4)


_llint_op_throw:
    traceExecution()
    callSlowPath(_llint_slow_path_throw)
    dispatch(2)


_llint_op_throw_reference_error:
    traceExecution()
    callSlowPath(_llint_slow_path_throw_reference_error)
    dispatch(2)


_llint_op_profile_will_call:
    traceExecution()
    callSlowPath(_llint_slow_path_profile_will_call)
    dispatch(2)


_llint_op_profile_did_call:
    traceExecution()
    callSlowPath(_llint_slow_path_profile_did_call)
    dispatch(2)


_llint_op_debug:
    traceExecution()
    callSlowPath(_llint_slow_path_debug)
    dispatch(5)


_llint_native_call_trampoline:
    nativeCallTrampoline(NativeExecutable::m_function)


_llint_native_construct_trampoline:
    nativeCallTrampoline(NativeExecutable::m_constructor)


# Lastly, make sure that we can link even though we don't support all opcodes.
# These opcodes should never arise when using LLInt or either JIT. We assert
# as much.

macro notSupported()
    if ASSERT_ENABLED
        crash()
    else
        # We should use whatever the smallest possible instruction is, just to
        # ensure that there is a gap between instruction labels. If multiple
        # smallest instructions exist, we should pick the one that is most
        # likely result in execution being halted. Currently that is the break
        # instruction on all architectures we're interested in. (Break is int3
        # on Intel, which is 1 byte, and bkpt on ARMv7, which is 2 bytes.)
        break
    end
end

_llint_op_get_by_id_chain:
    notSupported()

_llint_op_get_by_id_custom_chain:
    notSupported()

_llint_op_get_by_id_custom_proto:
    notSupported()

_llint_op_get_by_id_custom_self:
    notSupported()

_llint_op_get_by_id_generic:
    notSupported()

_llint_op_get_by_id_getter_chain:
    notSupported()

_llint_op_get_by_id_getter_proto:
    notSupported()

_llint_op_get_by_id_getter_self:
    notSupported()

_llint_op_get_by_id_proto:
    notSupported()

_llint_op_get_by_id_self:
    notSupported()

_llint_op_get_string_length:
    notSupported()

_llint_op_put_by_id_generic:
    notSupported()

_llint_op_put_by_id_replace:
    notSupported()

_llint_op_put_by_id_transition:
    notSupported()


# Indicate the end of LLInt.
_llint_end:
    crash()
