/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "BitVector.h"

#include <algorithm>
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/FastMalloc.h>
#include <wtf/StdLibExtras.h>

BitVector::BitVector(const BitVector& other)
    : m_bitsOrPointer(makeInlineBits(0))
{
    (*this) = other;
}

BitVector& BitVector::operator=(const BitVector& other)
{
    uintptr_t newBitsOrPointer;
    if (other.isInline())
        newBitsOrPointer = other.m_bitsOrPointer;
    else {
        OutOfLineBits* newOutOfLineBits = OutOfLineBits::create(other.size());
        memcpy(newOutOfLineBits->bits(), other.bits(), byteCount(other.size()));
        newBitsOrPointer = bitwise_cast<uintptr_t>(newOutOfLineBits);
    }
    if (!isInline())
        OutOfLineBits::destroy(outOfLineBits());
    m_bitsOrPointer = newBitsOrPointer;
    return *this;
}

void BitVector::resize(size_t numBits)
{
    if (numBits <= maxInlineBits()) {
        if (isInline())
            return;
    
        OutOfLineBits* myOutOfLineBits = outOfLineBits();
        m_bitsOrPointer = makeInlineBits(*myOutOfLineBits->bits());
        OutOfLineBits::destroy(myOutOfLineBits);
        return;
    }
    
    resizeOutOfLine(numBits);
}

void BitVector::clearAll()
{
    if (isInline())
        m_bitsOrPointer = makeInlineBits(0);
    else
        memset(outOfLineBits()->bits(), 0, byteCount(size()));
}

BitVector::OutOfLineBits* BitVector::OutOfLineBits::create(size_t numBits)
{
    numBits = (numBits + bitsInPointer() - 1) & ~(bitsInPointer() - 1);
    size_t size = sizeof(OutOfLineBits) + sizeof(uintptr_t) * (numBits / bitsInPointer());
    OutOfLineBits* result = new (fastMalloc(size)) OutOfLineBits(numBits);
    return result;
}

void BitVector::OutOfLineBits::destroy(OutOfLineBits* outOfLineBits)
{
    fastFree(outOfLineBits);
}

void BitVector::resizeOutOfLine(size_t numBits)
{
    ASSERT(numBits > maxInlineBits());
    OutOfLineBits* newOutOfLineBits = OutOfLineBits::create(numBits);
    memcpy(newOutOfLineBits->bits(), bits(), byteCount(std::min(size(), numBits)));
    if (!isInline())
        OutOfLineBits::destroy(outOfLineBits());
    m_bitsOrPointer = bitwise_cast<uintptr_t>(newOutOfLineBits);
}

