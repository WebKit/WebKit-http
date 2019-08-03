/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <wtf/OSAllocator.h>

#include <errno.h>
#include <sys/mman.h>
#include <wtf/Assertions.h>
#include <wtf/PageAllocation.h>

#include <OS.h>
#include <syscalls.h>

namespace WTF {

void* OSAllocator::reserveUncommitted(size_t bytes, Usage usage, bool writable, bool executable, bool includesGuardPages)
{
    UNUSED_PARAM(usage);
    UNUSED_PARAM(writable);
    UNUSED_PARAM(executable);

	addr_t result = 0;
	if (_kern_reserve_address_range(&result, B_ANY_ADDRESS, bytes) != B_OK) {
		return NULL;
	}
    if (result && includesGuardPages) {
		// Mark the guard pages as not accessible (not really needed since they
		// are reserved and not mapped already
        mmap((char*)(result), pageSize(), PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
        mmap((char*)(result) + bytes - pageSize(), pageSize(), PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
    }
    return (void*)result;
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage usage, bool writable, bool executable, bool includesGuardPages)
{
    // All POSIX reservations start out logically committed.
    int protection = PROT_READ;
    if (writable)
        protection |= PROT_WRITE;
    if (executable)
        protection |= PROT_EXEC;

    int flags = MAP_PRIVATE | MAP_ANON;
    UNUSED_PARAM(usage);
    int fd = -1;

    void* result = 0;

    result = mmap(result, bytes, protection, flags, fd, 0);
    if (result == MAP_FAILED) {
        if (executable)
            result = 0;
        else
            CRASH();
    }
    if (result && includesGuardPages) {
        // We use mmap to remap the guardpages rather than using mprotect as
        // mprotect results in multiple references to the code region. This
        // breaks the madvise based mechanism we use to return physical memory
        // to the OS.
        mmap(result, pageSize(), PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, fd, 0);
        mmap(static_cast<char*>(result) + bytes - pageSize(), pageSize(), PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, fd, 0);
    }
    return result;
}

void OSAllocator::commit(void* address, size_t bytes, bool writable, bool executable)
{
    int protection = PROT_READ;
    if (writable)
        protection |= PROT_WRITE;
    if (executable)
        protection |= PROT_EXEC;

    int flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;

	void* effective = mmap(address, bytes, protection, flags, -1, 0);
	if (effective != address)
		CRASH();
}

void OSAllocator::decommit(void* address, size_t bytes)
{
	int result = munmap(address, bytes);
	if (result == -1)
		CRASH();
}

void OSAllocator::hintMemoryNotNeededSoon(void* address, size_t bytes)
{
#if HAVE(MADV_DONTNEED)
    while (madvise(address, bytes, MADV_DONTNEED) == -1 && errno == EAGAIN) { }
#else
    UNUSED_PARAM(address);
    UNUSED_PARAM(bytes);
#endif
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
    int result = _kern_unreserve_address_range((addr_t)address, bytes);
    if (result != B_OK)
        CRASH();
}

} // namespace WTF
