/*
 * Copyright 2010 Google Inc. All rights reserved.
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

#ifndef RawURLBuffer_h
#define RawURLBuffer_h

#if USE(WTFURL)

#include "URLBuffer.h"
#include <string.h>

namespace WTF {

// Simple implementation of the URLBuffer using new[]. This class
// also supports a static buffer so if it is allocated on the stack, most
// URLs can be canonicalized with no heap allocations.
template<typename CharacterType, int inlineCapacity = 1024>
class RawURLBuffer : public URLBuffer<CharacterType> {
public:
    RawURLBuffer() : URLBuffer<CharacterType>()
    {
        this->m_buffer = m_inlineBuffer;
        this->m_capacity = inlineCapacity;
    }

    virtual ~RawURLBuffer()
    {
        if (this->m_buffer != m_inlineBuffer)
            delete[] this->m_buffer;
    }

    virtual void resize(int size)
    {
        CharacterType* newBuffer = new CharacterType[size];
        memcpy(newBuffer, this->m_buffer, sizeof(CharacterType) * (this->m_length < size ? this->m_length : size));
        if (this->m_buffer != m_inlineBuffer)
            delete[] this->m_buffer;
        this->m_buffer = newBuffer;
        this->m_capacity = size;
    }

protected:
    CharacterType m_inlineBuffer[inlineCapacity];
};

} // namespace WTF

#endif // USE(WTFURL)

#endif // RawURLBuffer_h
