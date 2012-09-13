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

#ifndef PropertyOffset_h
#define PropertyOffset_h

#include "JSType.h"
#include <wtf/Platform.h>
#include <wtf/StdLibExtras.h>
#include <wtf/UnusedParam.h>

namespace JSC {

#if USE(JSVALUE32_64)
#define INLINE_STORAGE_CAPACITY 7
#else
#define INLINE_STORAGE_CAPACITY 6
#endif

typedef int PropertyOffset;

static const PropertyOffset invalidOffset = -1;
static const PropertyOffset inlineStorageCapacity = INLINE_STORAGE_CAPACITY;
static const PropertyOffset firstOutOfLineOffset = inlineStorageCapacity;

// Declare all of the functions because they tend to do forward calls.
inline void checkOffset(PropertyOffset);
inline void checkOffset(PropertyOffset, JSType);
inline void validateOffset(PropertyOffset);
inline void validateOffset(PropertyOffset, JSType);
inline bool isValidOffset(PropertyOffset);
inline bool isInlineOffset(PropertyOffset);
inline bool isOutOfLineOffset(PropertyOffset);
inline size_t offsetInInlineStorage(PropertyOffset);
inline size_t offsetInOutOfLineStorage(PropertyOffset);
inline size_t offsetInRespectiveStorage(PropertyOffset);
inline size_t numberOfOutOfLineSlotsForLastOffset(PropertyOffset);
inline size_t numberOfSlotsForLastOffset(PropertyOffset, JSType);
inline PropertyOffset nextPropertyOffsetFor(PropertyOffset, JSType);
inline PropertyOffset firstPropertyOffsetFor(JSType);

inline void checkOffset(PropertyOffset offset)
{
    UNUSED_PARAM(offset);
    ASSERT(offset >= invalidOffset);
}

inline void checkOffset(PropertyOffset offset, JSType type)
{
    UNUSED_PARAM(offset);
    UNUSED_PARAM(type);
    ASSERT(offset >= invalidOffset);
    ASSERT(offset == invalidOffset
           || type == FinalObjectType
           || isOutOfLineOffset(offset));
}

inline void validateOffset(PropertyOffset offset)
{
    checkOffset(offset);
    ASSERT(isValidOffset(offset));
}

inline void validateOffset(PropertyOffset offset, JSType type)
{
    checkOffset(offset, type);
    ASSERT(isValidOffset(offset));
}

inline bool isValidOffset(PropertyOffset offset)
{
    checkOffset(offset);
    return offset != invalidOffset;
}

inline bool isInlineOffset(PropertyOffset offset)
{
    checkOffset(offset);
    return offset < inlineStorageCapacity;
}

inline bool isOutOfLineOffset(PropertyOffset offset)
{
    checkOffset(offset);
    return !isInlineOffset(offset);
}

inline size_t offsetInInlineStorage(PropertyOffset offset)
{
    validateOffset(offset);
    ASSERT(isInlineOffset(offset));
    return offset;
}

inline size_t offsetInOutOfLineStorage(PropertyOffset offset)
{
    validateOffset(offset);
    ASSERT(isOutOfLineOffset(offset));
    return -static_cast<ptrdiff_t>(offset - firstOutOfLineOffset) - 1;
}

inline size_t offsetInRespectiveStorage(PropertyOffset offset)
{
    if (isInlineOffset(offset))
        return offsetInInlineStorage(offset);
    return offsetInOutOfLineStorage(offset);
}

inline size_t numberOfOutOfLineSlotsForLastOffset(PropertyOffset offset)
{
    checkOffset(offset);
    if (offset < firstOutOfLineOffset)
        return 0;
    return offset - firstOutOfLineOffset + 1;
}

inline size_t numberOfSlotsForLastOffset(PropertyOffset offset, JSType type)
{
    checkOffset(offset, type);
    if (type == FinalObjectType)
        return offset + 1;
    return numberOfOutOfLineSlotsForLastOffset(offset);
}

inline PropertyOffset nextPropertyOffsetFor(PropertyOffset offset, JSType type)
{
    checkOffset(offset, type);
    if (type != FinalObjectType && offset == invalidOffset)
        return firstOutOfLineOffset;
    return offset + 1;
}

inline PropertyOffset firstPropertyOffsetFor(JSType type)
{
    return nextPropertyOffsetFor(invalidOffset, type);
}

} // namespace JSC

#endif // PropertyOffset_h

