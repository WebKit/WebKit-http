/*
 * Copyright (C) 2008, 2014 Apple Inc. All Rights Reserved.
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
#include "Profile.h"

#include "ProfileNode.h"
#include <wtf/DataLog.h>

namespace JSC {

PassRefPtr<Profile> Profile::create(const String& title, unsigned uid)
{
    return adoptRef(new Profile(title, uid));
}

Profile::Profile(const String& title, unsigned uid)
    : m_title(title)
    , m_uid(uid)
    , m_idleTime(0)
{
    // FIXME: When multi-threading is supported this will be a vector and calls
    // into the profiler will need to know which thread it is executing on.
    m_head = ProfileNode::create(0, CallIdentifier("Thread_1", String(), 0, 0), 0, 0);
}

Profile::~Profile()
{
}

void Profile::forEach(void (ProfileNode::*function)())
{
    ProfileNode* currentNode = m_head->firstChild();
    for (ProfileNode* nextNode = currentNode; nextNode; nextNode = nextNode->firstChild())
        currentNode = nextNode;

    if (!currentNode)
        currentNode = m_head.get();

    ProfileNode* endNode = m_head->traverseNextNodePostOrder();
    while (currentNode && currentNode != endNode) {
        (currentNode->*function)();
        currentNode = currentNode->traverseNextNodePostOrder();
    } 
}

#ifndef NDEBUG
void Profile::debugPrintData() const
{
    dataLogF("Call graph:\n");
    m_head->debugPrintData(0);
}

typedef WTF::KeyValuePair<FunctionCallHashCount::ValueType, unsigned> NameCountPair;

static inline bool functionNameCountPairComparator(const NameCountPair& a, const NameCountPair& b)
{
    return a.value > b.value;
}

void Profile::debugPrintDataSampleStyle() const
{
    typedef Vector<NameCountPair> NameCountPairVector;

    FunctionCallHashCount countedFunctions;
    dataLogF("Call graph:\n");
    m_head->debugPrintDataSampleStyle(0, countedFunctions);

    dataLogF("\nTotal number in stack:\n");
    NameCountPairVector sortedFunctions(countedFunctions.size());
    copyToVector(countedFunctions, sortedFunctions);

    std::sort(sortedFunctions.begin(), sortedFunctions.end(), functionNameCountPairComparator);
    for (NameCountPairVector::iterator it = sortedFunctions.begin(); it != sortedFunctions.end(); ++it)
        dataLogF("        %-12d%s\n", (*it).value, String((*it).key).utf8().data());

    dataLogF("\nSort by top of stack, same collapsed (when >= 5):\n");
}
#endif

} // namespace JSC
