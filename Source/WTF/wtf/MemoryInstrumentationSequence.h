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

#ifndef MemoryInstrumentationSequence_h
#define MemoryInstrumentationSequence_h

#include <wtf/MemoryInstrumentation.h>
#include <wtf/TypeTraits.h>

namespace WTF {

template<typename ValueType>
struct SequenceMemoryInstrumentationTraits {
    template <typename I> static void reportMemoryUsage(I iterator, I end, MemoryClassInfo& info)
    {
        while (iterator != end) {
            info.addMember(*iterator);
            ++iterator;
        }
    }
};

template<> struct SequenceMemoryInstrumentationTraits<int> {
    template <typename I> static void reportMemoryUsage(I, I, MemoryClassInfo&) { }
};

template<> struct SequenceMemoryInstrumentationTraits<void*> {
    template <typename I> static void reportMemoryUsage(I, I, MemoryClassInfo&) { }
};

template<> struct SequenceMemoryInstrumentationTraits<char*> {
    template <typename I> static void reportMemoryUsage(I, I, MemoryClassInfo&) { }
};

template<> struct SequenceMemoryInstrumentationTraits<const char*> {
    template <typename I> static void reportMemoryUsage(I, I, MemoryClassInfo&) { }
};

template<> struct SequenceMemoryInstrumentationTraits<const void*> {
    template <typename I> static void reportMemoryUsage(I, I, MemoryClassInfo&) { }
};

template<typename ValueType, typename I> void reportSequenceMemoryUsage(I begin, I end, MemoryClassInfo& info)
{
    // Check if type is convertible to integer to handle iteration through enum values.
    SequenceMemoryInstrumentationTraits<typename Conditional<IsConvertibleToInteger<ValueType>::value, int, ValueType>::Type>::reportMemoryUsage(begin, end, info);
}

}

#endif // !defined(MemoryInstrumentationSequence_h)
