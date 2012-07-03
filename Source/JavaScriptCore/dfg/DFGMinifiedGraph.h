/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef DFGMinifiedGraph_h
#define DFGMinifiedGraph_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGMinifiedNode.h"
#include <algorithm>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

class MinifiedGraph {
public:
    MinifiedGraph() { }

    MinifiedNode* at(NodeIndex nodeIndex)
    {
        if (!m_list.size())
            return 0;
        MinifiedNode* entry =
            binarySearch<MinifiedNode, NodeIndex, MinifiedNode::getIndex>(
                m_list.begin(), m_list.size(), nodeIndex, WTF::KeyMustNotBePresentInArray);
        if (entry->index() != nodeIndex)
            return 0;
        return entry;
    }
    
    void append(const MinifiedNode& node)
    {
        m_list.append(node);
    }
    
    void prepareAndShrink()
    {
        std::sort(m_list.begin(), m_list.end(), MinifiedNode::compareByNodeIndex);
        m_list.shrinkToFit();
    }
    
    void setOriginalGraphSize(size_t size) { m_size = size; }
    
    size_t originalGraphSize() const { return m_size; }

private:
    Vector<MinifiedNode> m_list;
    size_t m_size;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGMinifiedGraph_h

