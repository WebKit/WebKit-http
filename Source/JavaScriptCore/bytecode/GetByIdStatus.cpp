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
#include "GetByIdStatus.h"

#include "CodeBlock.h"
#include "JSScope.h"
#include "LLIntData.h"
#include "LowLevelInterpreter.h"
#include "Operations.h"

namespace JSC {

GetByIdStatus GetByIdStatus::computeFromLLInt(CodeBlock* profiledBlock, unsigned bytecodeIndex, StringImpl* uid)
{
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(bytecodeIndex);
    UNUSED_PARAM(uid);
#if ENABLE(LLINT)
    Instruction* instruction = profiledBlock->instructions().begin() + bytecodeIndex;
    
    if (instruction[0].u.opcode == LLInt::getOpcode(llint_op_get_array_length))
        return GetByIdStatus(NoInformation, false);

    Structure* structure = instruction[4].u.structure.get();
    if (!structure)
        return GetByIdStatus(NoInformation, false);
    
    unsigned attributesIgnored;
    JSCell* specificValue;
    PropertyOffset offset = structure->getConcurrently(
        *profiledBlock->vm(), uid, attributesIgnored, specificValue);
    if (structure->isDictionary())
        specificValue = 0;
    if (!isValidOffset(offset))
        return GetByIdStatus(NoInformation, false);
    
    return GetByIdStatus(Simple, false, StructureSet(structure), offset, specificValue);
#else
    return GetByIdStatus(NoInformation, false);
#endif
}

void GetByIdStatus::computeForChain(GetByIdStatus& result, CodeBlock* profiledBlock, StringImpl* uid)
{
#if ENABLE(JIT) && ENABLE(VALUE_PROFILER)
    // Validate the chain. If the chain is invalid, then currently the best thing
    // we can do is to assume that TakesSlow is true. In the future, it might be
    // worth exploring reifying the structure chain from the structure we've got
    // instead of using the one from the cache, since that will do the right things
    // if the structure chain has changed. But that may be harder, because we may
    // then end up having a different type of access altogether. And it currently
    // does not appear to be worth it to do so -- effectively, the heuristic we
    // have now is that if the structure chain has changed between when it was
    // cached on in the baseline JIT and when the DFG tried to inline the access,
    // then we fall back on a polymorphic access.
    if (!result.m_chain->isStillValid())
        return;
    
    JSObject* currentObject = result.m_chain->terminalPrototype();
    Structure* currentStructure = result.m_chain->last();
    
    ASSERT_UNUSED(currentObject, currentObject);
        
    unsigned attributesIgnored;
    JSCell* specificValue;
        
    result.m_offset = currentStructure->getConcurrently(
        *profiledBlock->vm(), uid, attributesIgnored, specificValue);
    if (currentStructure->isDictionary())
        specificValue = 0;
    if (!isValidOffset(result.m_offset))
        return;
        
    result.m_structureSet.add(result.m_chain->head());
    result.m_specificValue = JSValue(specificValue);
#else
    UNUSED_PARAM(result);
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(uid);
    UNREACHABLE_FOR_PLATFORM();
#endif
}

GetByIdStatus GetByIdStatus::computeFor(CodeBlock* profiledBlock, StubInfoMap& map, unsigned bytecodeIndex, StringImpl* uid)
{
    ConcurrentJITLocker locker(profiledBlock->m_lock);
    
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(bytecodeIndex);
    UNUSED_PARAM(uid);
#if ENABLE(JIT) && ENABLE(VALUE_PROFILER)
    StructureStubInfo* stubInfo = map.get(CodeOrigin(bytecodeIndex));
    if (!stubInfo || !stubInfo->seen)
        return computeFromLLInt(profiledBlock, bytecodeIndex, uid);
    
    if (stubInfo->resetByGC)
        return GetByIdStatus(TakesSlowPath, true);

    PolymorphicAccessStructureList* list;
    int listSize;
    switch (stubInfo->accessType) {
    case access_get_by_id_self_list:
        list = stubInfo->u.getByIdSelfList.structureList;
        listSize = stubInfo->u.getByIdSelfList.listSize;
        break;
    case access_get_by_id_proto_list:
        list = stubInfo->u.getByIdProtoList.structureList;
        listSize = stubInfo->u.getByIdProtoList.listSize;
        break;
    default:
        list = 0;
        listSize = 0;
        break;
    }
    for (int i = 0; i < listSize; ++i) {
        if (!list->list[i].isDirect)
            return GetByIdStatus(MakesCalls, true);
    }
    
    // Next check if it takes slow case, in which case we want to be kind of careful.
    if (profiledBlock->likelyToTakeSlowCase(bytecodeIndex))
        return GetByIdStatus(TakesSlowPath, true);
    
    // Finally figure out if we can derive an access strategy.
    GetByIdStatus result;
    result.m_wasSeenInJIT = true; // This is interesting for bytecode dumping only.
    switch (stubInfo->accessType) {
    case access_unset:
        return computeFromLLInt(profiledBlock, bytecodeIndex, uid);
        
    case access_get_by_id_self: {
        Structure* structure = stubInfo->u.getByIdSelf.baseObjectStructure.get();
        unsigned attributesIgnored;
        JSCell* specificValue;
        result.m_offset = structure->getConcurrently(
            *profiledBlock->vm(), uid, attributesIgnored, specificValue);
        if (structure->isDictionary())
            specificValue = 0;
        
        if (isValidOffset(result.m_offset)) {
            result.m_structureSet.add(structure);
            result.m_specificValue = JSValue(specificValue);
        }
        
        if (isValidOffset(result.m_offset))
            ASSERT(result.m_structureSet.size());
        break;
    }
        
    case access_get_by_id_self_list: {
        for (int i = 0; i < listSize; ++i) {
            ASSERT(list->list[i].isDirect);
            
            Structure* structure = list->list[i].base.get();
            if (result.m_structureSet.contains(structure))
                continue;
            
            unsigned attributesIgnored;
            JSCell* specificValue;
            PropertyOffset myOffset = structure->getConcurrently(
                *profiledBlock->vm(), uid, attributesIgnored, specificValue);
            if (structure->isDictionary())
                specificValue = 0;
            
            if (!isValidOffset(myOffset)) {
                result.m_offset = invalidOffset;
                break;
            }
                    
            if (!i) {
                result.m_offset = myOffset;
                result.m_specificValue = JSValue(specificValue);
            } else if (result.m_offset != myOffset) {
                result.m_offset = invalidOffset;
                break;
            } else if (result.m_specificValue != JSValue(specificValue))
                result.m_specificValue = JSValue();
            
            result.m_structureSet.add(structure);
        }
                    
        if (isValidOffset(result.m_offset))
            ASSERT(result.m_structureSet.size());
        break;
    }
        
    case access_get_by_id_proto: {
        if (!stubInfo->u.getByIdProto.isDirect)
            return GetByIdStatus(MakesCalls, true);
        result.m_chain = adoptRef(new IntendedStructureChain(
            profiledBlock,
            stubInfo->u.getByIdProto.baseObjectStructure.get(),
            stubInfo->u.getByIdProto.prototypeStructure.get()));
        computeForChain(result, profiledBlock, uid);
        break;
    }
        
    case access_get_by_id_chain: {
        if (!stubInfo->u.getByIdChain.isDirect)
            return GetByIdStatus(MakesCalls, true);
        result.m_chain = adoptRef(new IntendedStructureChain(
            profiledBlock,
            stubInfo->u.getByIdChain.baseObjectStructure.get(),
            stubInfo->u.getByIdChain.chain.get(),
            stubInfo->u.getByIdChain.count));
        computeForChain(result, profiledBlock, uid);
        break;
    }
        
    default:
        ASSERT(!isValidOffset(result.m_offset));
        break;
    }
    
    if (!isValidOffset(result.m_offset)) {
        result.m_state = TakesSlowPath;
        result.m_structureSet.clear();
        result.m_chain.clear();
        result.m_specificValue = JSValue();
    } else
        result.m_state = Simple;
    
    return result;
#else // ENABLE(JIT)
    UNUSED_PARAM(map);
    return GetByIdStatus(NoInformation, false);
#endif // ENABLE(JIT)
}

GetByIdStatus GetByIdStatus::computeFor(VM& vm, Structure* structure, StringImpl* uid)
{
    // For now we only handle the super simple self access case. We could handle the
    // prototype case in the future.
    
    if (!structure)
        return GetByIdStatus(TakesSlowPath);

    if (toUInt32FromStringImpl(uid) != PropertyName::NotAnIndex)
        return GetByIdStatus(TakesSlowPath);
    
    if (structure->typeInfo().overridesGetOwnPropertySlot() && structure->typeInfo().type() != GlobalObjectType)
        return GetByIdStatus(TakesSlowPath);
    
    if (!structure->propertyAccessesAreCacheable())
        return GetByIdStatus(TakesSlowPath);
    
    GetByIdStatus result;
    result.m_wasSeenInJIT = false; // To my knowledge nobody that uses computeFor(VM&, Structure*, StringImpl*) reads this field, but I might as well be honest: no, it wasn't seen in the JIT, since I computed it statically.
    unsigned attributes;
    JSCell* specificValue;
    result.m_offset = structure->getConcurrently(vm, uid, attributes, specificValue);
    if (!isValidOffset(result.m_offset))
        return GetByIdStatus(TakesSlowPath); // It's probably a prototype lookup. Give up on life for now, even though we could totally be way smarter about it.
    if (attributes & Accessor)
        return GetByIdStatus(MakesCalls);
    if (structure->isDictionary())
        specificValue = 0;
    result.m_structureSet.add(structure);
    result.m_specificValue = JSValue(specificValue);
    result.m_state = Simple;
    return result;
}

} // namespace JSC

