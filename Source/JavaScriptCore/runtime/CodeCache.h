/*
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
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

#ifndef CodeCache_h
#define CodeCache_h

#include "CodeSpecializationKind.h"
#include "ParserModes.h"
#include "Strong.h"
#include "WeakRandom.h"

#include <wtf/FixedArray.h>
#include <wtf/Forward.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RandomNumber.h>
#include <wtf/text/WTFString.h>

namespace JSC {

class EvalExecutable;
class Identifier;
class ProgramExecutable;
class UnlinkedCodeBlock;
class UnlinkedEvalCodeBlock;
class UnlinkedFunctionCodeBlock;
class UnlinkedFunctionExecutable;
class UnlinkedProgramCodeBlock;
class JSGlobalData;
struct ParserError;
class SourceCode;
class SourceProvider;

template <typename KeyType, typename EntryType, int CacheSize> class CacheMap {
    typedef typename HashMap<KeyType, unsigned>::iterator iterator;
public:
    CacheMap()
        : m_randomGenerator((static_cast<uint32_t>(randomNumber() * std::numeric_limits<uint32_t>::max())))
    {
    }
    const EntryType* find(const KeyType& key)
    {
        iterator result = m_map.find(key);
        if (result == m_map.end())
            return 0;
        return &m_data[result->value].second;
    }
    void add(const KeyType& key, const EntryType& value)
    {
        iterator result = m_map.find(key);
        if (result != m_map.end()) {
            m_data[result->value].second = value;
            return;
        }
        size_t newIndex = m_randomGenerator.getUint32() % CacheSize;
        if (m_data[newIndex].second)
            m_map.remove(m_data[newIndex].first);
        m_map.add(key, newIndex);
        m_data[newIndex].first = key;
        m_data[newIndex].second = value;
        ASSERT(m_map.size() <= CacheSize);
    }
private:
    HashMap<KeyType, unsigned> m_map;
    FixedArray<std::pair<KeyType, EntryType>, CacheSize> m_data;
    WeakRandom m_randomGenerator;
};

class CodeCache {
public:
    static PassOwnPtr<CodeCache> create() { return adoptPtr(new CodeCache); }

    UnlinkedProgramCodeBlock* getProgramCodeBlock(JSGlobalData&, ProgramExecutable*, const SourceCode&, JSParserStrictness, DebuggerMode, ProfilerMode, ParserError&);
    UnlinkedEvalCodeBlock* getEvalCodeBlock(JSGlobalData&, EvalExecutable*, const SourceCode&, JSParserStrictness, DebuggerMode, ProfilerMode, ParserError&);
    UnlinkedFunctionCodeBlock* getFunctionCodeBlock(JSGlobalData&, UnlinkedFunctionExecutable*, const SourceCode&, CodeSpecializationKind, DebuggerMode, ProfilerMode, ParserError&);
    UnlinkedFunctionExecutable* getFunctionExecutableFromGlobalCode(JSGlobalData&, const Identifier&, const SourceCode&, ParserError&);
    void usedFunctionCode(JSGlobalData&, UnlinkedFunctionCodeBlock*);
    ~CodeCache();

    enum CodeType { EvalType, ProgramType, FunctionType };
    typedef std::pair<String, unsigned> CodeBlockKey;
    typedef std::pair<String, String> GlobalFunctionKey;

private:
    CodeCache();

    UnlinkedFunctionCodeBlock* generateFunctionCodeBlock(JSGlobalData&, UnlinkedFunctionExecutable*, const SourceCode&, CodeSpecializationKind, DebuggerMode, ProfilerMode, ParserError&);

    template <class UnlinkedCodeBlockType, class ExecutableType> inline UnlinkedCodeBlockType* getCodeBlock(JSGlobalData&, ExecutableType*, const SourceCode&, JSParserStrictness, DebuggerMode, ProfilerMode, ParserError&);
    CodeBlockKey makeCodeBlockKey(const SourceCode&, CodeType, JSParserStrictness);
    GlobalFunctionKey makeGlobalFunctionKey(const SourceCode&, const String&);

    enum {
        kMaxCodeBlockEntries = 1024,
        kMaxGlobalFunctionEntries = 1024,
        kMaxFunctionCodeBlocks = 1024
    };

    CacheMap<CodeBlockKey, Strong<UnlinkedCodeBlock>, kMaxCodeBlockEntries> m_cachedCodeBlocks;
    CacheMap<GlobalFunctionKey, Strong<UnlinkedFunctionExecutable>, kMaxGlobalFunctionEntries> m_cachedGlobalFunctions;
    CacheMap<UnlinkedFunctionCodeBlock*, Strong<UnlinkedFunctionCodeBlock>, kMaxFunctionCodeBlocks> m_cachedFunctionCode;
};

}

#endif
