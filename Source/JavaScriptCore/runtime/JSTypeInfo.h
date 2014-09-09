// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef JSTypeInfo_h
#define JSTypeInfo_h

// This file would be called TypeInfo.h, but that conflicts with <typeinfo.h>
// in the STL on systems without case-sensitive file systems. 

#include "JSType.h"

namespace JSC {

class LLIntOffsetsExtractor;

static const unsigned MasqueradesAsUndefined = 1; // WebCore uses MasqueradesAsUndefined to make document.all undetectable.
static const unsigned ImplementsHasInstance = 1 << 1;
static const unsigned OverridesHasInstance = 1 << 2;
static const unsigned ImplementsDefaultHasInstance = 1 << 3;
static const unsigned IsEnvironmentRecord = 1 << 4;
static const unsigned OverridesGetOwnPropertySlot = 1 << 5;
static const unsigned InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero = 1 << 6;

static const unsigned OverridesGetPropertyNames = 1 << 8;
static const unsigned ProhibitsPropertyCaching = 1 << 9;
static const unsigned HasImpureGetOwnPropertySlot = 1 << 10;
static const unsigned NewImpurePropertyFiresWatchpoints = 1 << 11;
static const unsigned StructureIsImmortal = 1 << 12;

class TypeInfo {
public:
    typedef uint8_t InlineTypeFlags;
    typedef uint8_t OutOfLineTypeFlags;

    TypeInfo(JSType type, unsigned flags = 0)
        : TypeInfo(type, flags & 0xff, flags >> 8)
    {
    }

    TypeInfo(JSType type, InlineTypeFlags inlineTypeFlags, OutOfLineTypeFlags outOfLineTypeFlags)
        : m_type(type)
        , m_flags(inlineTypeFlags)
        , m_flags2(outOfLineTypeFlags)
    {
        // No object that doesn't ImplementsHasInstance should override it!
        ASSERT((m_flags & (ImplementsHasInstance | OverridesHasInstance)) != OverridesHasInstance);
        // ImplementsDefaultHasInstance means (ImplementsHasInstance & !OverridesHasInstance)
        if ((m_flags & (ImplementsHasInstance | OverridesHasInstance)) == ImplementsHasInstance)
            m_flags |= ImplementsDefaultHasInstance;
    }

    JSType type() const { return static_cast<JSType>(m_type); }
    bool isObject() const { return isObject(type()); }
    static bool isObject(JSType type) { return type >= ObjectType; }
    bool isFinalObject() const { return type() == FinalObjectType; }
    bool isNumberObject() const { return type() == NumberObjectType; }
    bool isName() const { return type() == NameInstanceType; }

    unsigned flags() const { return (static_cast<unsigned>(m_flags2) << 8) | static_cast<unsigned>(m_flags); }
    bool masqueradesAsUndefined() const { return isSetOnFlags1(MasqueradesAsUndefined); }
    bool implementsHasInstance() const { return isSetOnFlags1(ImplementsHasInstance); }
    bool isEnvironmentRecord() const { return isSetOnFlags1(IsEnvironmentRecord); }
    bool overridesHasInstance() const { return isSetOnFlags1(OverridesHasInstance); }
    bool implementsDefaultHasInstance() const { return isSetOnFlags1(ImplementsDefaultHasInstance); }
    bool overridesGetOwnPropertySlot() const { return overridesGetOwnPropertySlot(inlineTypeFlags()); }
    static bool overridesGetOwnPropertySlot(InlineTypeFlags flags) { return flags & OverridesGetOwnPropertySlot; }
    bool interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero() const { return isSetOnFlags1(InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero); }
    bool overridesGetPropertyNames() const { return isSetOnFlags2(OverridesGetPropertyNames); }
    bool prohibitsPropertyCaching() const { return isSetOnFlags2(ProhibitsPropertyCaching); }
    bool hasImpureGetOwnPropertySlot() const { return isSetOnFlags2(HasImpureGetOwnPropertySlot); }
    bool newImpurePropertyFiresWatchpoints() const { return isSetOnFlags2(NewImpurePropertyFiresWatchpoints); }
    bool structureIsImmortal() const { return isSetOnFlags2(StructureIsImmortal); }

    static ptrdiff_t flagsOffset()
    {
        return OBJECT_OFFSETOF(TypeInfo, m_flags);
    }

    static ptrdiff_t typeOffset()
    {
        return OBJECT_OFFSETOF(TypeInfo, m_type);
    }

    InlineTypeFlags inlineTypeFlags() const { return m_flags; }
    OutOfLineTypeFlags outOfLineTypeFlags() const { return m_flags2; }

private:
    friend class LLIntOffsetsExtractor;

    bool isSetOnFlags1(unsigned flag) const { ASSERT(flag <= (1 << 7)); return m_flags & flag; }
    bool isSetOnFlags2(unsigned flag) const { ASSERT(flag >= (1 << 8)); return m_flags2 & (flag >> 8); }

    unsigned char m_type;
    unsigned char m_flags;
    unsigned char m_flags2;
};

}

#endif // JSTypeInfo_h
