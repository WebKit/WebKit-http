/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#ifndef SmallStrings_h
#define SmallStrings_h

#include "UString.h"
#include <wtf/FixedArray.h>
#include <wtf/OwnPtr.h>

#define JSC_COMMON_STRINGS_EACH_NAME(macro) \
    macro(boolean) \
    macro(false) \
    macro(function) \
    macro(number) \
    macro(null) \
    macro(object) \
    macro(undefined) \
    macro(string) \
    macro(true)

namespace JSC {

    class HeapRootVisitor;
    class JSGlobalData;
    class JSString;
    class SmallStringsStorage;
    class SlotVisitor;

    static const unsigned maxSingleCharacterString = 0xFF;

    class SmallStrings {
        WTF_MAKE_NONCOPYABLE(SmallStrings);
    public:
        SmallStrings();
        ~SmallStrings();

        JSString* emptyString(JSGlobalData* globalData)
        {
            if (!m_emptyString)
                createEmptyString(globalData);
            return m_emptyString;
        }

        JSString* singleCharacterString(JSGlobalData* globalData, unsigned char character)
        {
            if (!m_singleCharacterStrings[character])
                createSingleCharacterString(globalData, character);
            return m_singleCharacterStrings[character];
        }

        JS_EXPORT_PRIVATE StringImpl* singleCharacterStringRep(unsigned char character);

        void finalizeSmallStrings();

        JSString** singleCharacterStrings() { return &m_singleCharacterStrings[0]; }

#define JSC_COMMON_STRINGS_ACCESSOR_DEFINITION(name) \
        JSString* name##String(JSGlobalData* globalData) const \
        { \
            if (!m_##name) \
                initialize(globalData, m_##name, #name); \
            return m_##name; \
        }
        JSC_COMMON_STRINGS_EACH_NAME(JSC_COMMON_STRINGS_ACCESSOR_DEFINITION)
#undef JSC_COMMON_STRINGS_ACCESSOR_DEFINITION

    private:
        static const unsigned singleCharacterStringCount = maxSingleCharacterString + 1;

        JS_EXPORT_PRIVATE void createEmptyString(JSGlobalData*);
        JS_EXPORT_PRIVATE void createSingleCharacterString(JSGlobalData*, unsigned char);

        void initialize(JSGlobalData* globalData, JSString*& string, const char* value) const;

        JSString* m_emptyString;
#define JSC_COMMON_STRINGS_ATTRIBUTE_DECLARATION(name) mutable JSString* m_##name;
        JSC_COMMON_STRINGS_EACH_NAME(JSC_COMMON_STRINGS_ATTRIBUTE_DECLARATION)
#undef JSC_COMMON_STRINGS_ATTRIBUTE_DECLARATION
        JSString* m_singleCharacterStrings[singleCharacterStringCount];
        OwnPtr<SmallStringsStorage> m_storage;
    };

} // namespace JSC

#endif // SmallStrings_h
