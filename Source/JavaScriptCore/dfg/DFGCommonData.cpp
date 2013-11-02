/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "DFGCommonData.h"

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "DFGNode.h"
#include "DFGPlan.h"
#include "Operations.h"
#include "VM.h"

namespace JSC { namespace DFG {

void CommonData::notifyCompilingStructureTransition(Plan& plan, CodeBlock* codeBlock, Node* node)
{
    plan.transitions.addLazily(
        codeBlock,
        node->codeOrigin.codeOriginOwner(),
        node->structureTransitionData().previousStructure,
        node->structureTransitionData().newStructure);
}

unsigned CommonData::addCodeOrigin(CodeOrigin codeOrigin)
{
    if (codeOrigins.isEmpty()
        || codeOrigins.last() != codeOrigin)
        codeOrigins.append(codeOrigin);
    unsigned index = codeOrigins.size() - 1;
    ASSERT(codeOrigins[index] == codeOrigin);
    return index;
}

void CommonData::shrinkToFit()
{
    codeOrigins.shrinkToFit();
    weakReferences.shrinkToFit();
    transitions.shrinkToFit();
}

bool CommonData::invalidate()
{
    if (!isStillValid)
        return false;
    for (unsigned i = jumpReplacements.size(); i--;)
        jumpReplacements[i].fire();
    isStillValid = false;
    return true;
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

