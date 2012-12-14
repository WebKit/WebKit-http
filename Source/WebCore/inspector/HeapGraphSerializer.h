/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HeapGraphSerializer_h
#define HeapGraphSerializer_h

#if ENABLE(INSPECTOR)

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/MemoryInstrumentation.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class HeapGraphEdge;
class HeapGraphNode;
class InspectorObject;

class HeapGraphSerializer {
    WTF_MAKE_NONCOPYABLE(HeapGraphSerializer);
public:
    HeapGraphSerializer();
    ~HeapGraphSerializer();
    void reportNode(const WTF::MemoryObjectInfo&);
    void reportEdge(const void*, const void*, const char*);
    void reportLeaf(const void*, const WTF::MemoryObjectInfo&, const char*);
    void reportBaseAddress(const void*, const void*);

    PassRefPtr<InspectorObject> serialize();

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    int addString(const String&);
    void adjutEdgeTargets();

    typedef HashMap<String, int> StringMap;
    StringMap m_stringToIndex;
    Vector<String> m_strings;
    int m_lastReportedEdgeIndex;

    typedef HashMap<const void*, int> ObjectToNodeIndex;
    ObjectToNodeIndex m_objectToNodeIndex;

    typedef HashMap<const void*, const void*> BaseToRealAddress;
    BaseToRealAddress m_baseToRealAddress;

    Vector<HeapGraphNode> m_nodes;
    Vector<HeapGraphEdge> m_edges;
};

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
#endif // !defined(HeapGraphSerializer_h)
