/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RegisterFile.h"

#include "ConservativeRoots.h"
#include "Interpreter.h"

namespace JSC {

static size_t committedBytesCount = 0;

static Mutex& registerFileStatisticsMutex()
{
    DEFINE_STATIC_LOCAL(Mutex, staticMutex, ());
    return staticMutex;
}    
    
RegisterFile::~RegisterFile()
{
    void* base = m_reservation.base();
    m_reservation.decommit(base, reinterpret_cast<intptr_t>(m_commitEnd) - reinterpret_cast<intptr_t>(base));
    addToCommittedByteCount(-(reinterpret_cast<intptr_t>(m_commitEnd) - reinterpret_cast<intptr_t>(base)));
    m_reservation.deallocate();
}

void RegisterFile::gatherConservativeRoots(ConservativeRoots& conservativeRoots)
{
    conservativeRoots.add(begin(), end());
}

void RegisterFile::gatherConservativeRoots(ConservativeRoots& conservativeRoots, JettisonedCodeBlocks& jettisonedCodeBlocks)
{
    conservativeRoots.add(begin(), end(), jettisonedCodeBlocks);
}

void RegisterFile::releaseExcessCapacity()
{
    ptrdiff_t delta = reinterpret_cast<uintptr_t>(m_commitEnd) - reinterpret_cast<uintptr_t>(m_reservation.base());
    m_reservation.decommit(m_reservation.base(), delta);
    addToCommittedByteCount(-delta);
    m_commitEnd = static_cast<Register*>(m_reservation.base());
}

void RegisterFile::initializeThreading()
{
    registerFileStatisticsMutex();
}

size_t RegisterFile::committedByteCount()
{
    MutexLocker locker(registerFileStatisticsMutex());
    return committedBytesCount;
}

void RegisterFile::addToCommittedByteCount(long byteCount)
{
    MutexLocker locker(registerFileStatisticsMutex());
    ASSERT(static_cast<long>(committedBytesCount) + byteCount > -1);
    committedBytesCount += byteCount;
}

} // namespace JSC
