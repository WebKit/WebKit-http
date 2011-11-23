/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ArrayBufferView.h"

#include "ArrayBuffer.h"
#include "ExceptionCode.h"

namespace WTF {

ArrayBufferView::ArrayBufferView(PassRefPtr<ArrayBuffer> buffer,
                       unsigned byteOffset)
        : m_byteOffset(byteOffset)
        , m_buffer(buffer)
{
    m_baseAddress = m_buffer ? (static_cast<char*>(m_buffer->data()) + m_byteOffset) : 0;
    if (m_buffer) 
        m_buffer->addView(this);
}

ArrayBufferView::~ArrayBufferView()
{
    if (m_buffer)
        m_buffer->removeView(this);
}

bool ArrayBufferView::setImpl(ArrayBufferView* array, unsigned byteOffset)
{
    if (byteOffset > byteLength()
        || byteOffset + array->byteLength() > byteLength()
        || byteOffset + array->byteLength() < byteOffset) {
        // Out of range offset or overflow
        return false;
    }

    char* base = static_cast<char*>(baseAddress());
    memmove(base + byteOffset, array->baseAddress(), array->byteLength());
    return true;
}

bool ArrayBufferView::setRangeImpl(const char* data, size_t dataByteLength, unsigned byteOffset)
{
    if (byteOffset > byteLength()
        || byteOffset + dataByteLength > byteLength()
        || byteOffset + dataByteLength < byteOffset) {
        // Out of range offset or overflow
        return false;
    }

    char* base = static_cast<char*>(baseAddress());
    memmove(base + byteOffset, data, dataByteLength);
    return true;
}

bool ArrayBufferView::zeroRangeImpl(unsigned byteOffset, size_t rangeByteLength)
{
    if (byteOffset > byteLength()
        || byteOffset + rangeByteLength > byteLength()
        || byteOffset + rangeByteLength < byteOffset) {
        // Out of range offset or overflow
        return false;
    }

    char* base = static_cast<char*>(baseAddress());
    memset(base + byteOffset, 0, rangeByteLength);
    return true;
}

void ArrayBufferView::calculateOffsetAndLength(int start, int end, unsigned arraySize,
                                          unsigned* offset, unsigned* length)
{
    if (start < 0)
        start += arraySize;
    if (start < 0)
        start = 0;
    if (end < 0)
        end += arraySize;
    if (end < 0)
        end = 0;
    if (end < start)
        end = start;
    *offset = static_cast<unsigned>(start);
    *length = static_cast<unsigned>(end - start);
}

void ArrayBufferView::neuter()
{
    m_buffer = 0;
    m_byteOffset = 0;
}

}
