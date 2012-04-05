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

#ifndef JSStringJoiner_h
#define JSStringJoiner_h

#include "JSValue.h"
#include "UString.h"
#include <wtf/Vector.h>

namespace JSC {

class ExecState;


class JSStringJoiner {
public:
    JSStringJoiner(const UString& separator, size_t stringCount);

    void append(const UString&);
    JSValue build(ExecState*);

private:
    UString m_separator;
    Vector<UString> m_strings;

    unsigned m_cumulatedStringsLength;
    bool m_isValid;
    bool m_is8Bits;
};

inline JSStringJoiner::JSStringJoiner(const UString& separator, size_t stringCount)
    : m_separator(separator)
    , m_cumulatedStringsLength(0)
    , m_isValid(true)
    , m_is8Bits(m_separator.is8Bit())
{
    ASSERT(!m_separator.isNull());
    m_isValid = m_strings.tryReserveCapacity(stringCount);
}

inline void JSStringJoiner::append(const UString& str)
{
    if (!m_isValid)
        return;

    m_strings.uncheckedAppend(str);
    if (!str.isNull()) {
        m_cumulatedStringsLength += str.length();
        m_is8Bits = m_is8Bits && str.is8Bit();
    }
}

}

#endif
