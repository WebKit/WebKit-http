/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTFString_h
#define WTFString_h

// This file would be called String.h, but that conflicts with <string.h>
// on systems without case-sensitive file systems.

#include "StringImpl.h"

#ifdef __OBJC__
#include <objc/objc.h>
#endif

#if USE(CF)
typedef const struct __CFString * CFStringRef;
#endif

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE
#include <QDataStream>
#endif

#if PLATFORM(WX)
class wxString;
#endif

namespace WTF {

class CString;
struct StringHash;

// Declarations of string operations

bool charactersAreAllASCII(const UChar*, size_t);
bool charactersAreAllLatin1(const UChar*, size_t);
WTF_EXPORT_PRIVATE int charactersToIntStrict(const UChar*, size_t, bool* ok = 0, int base = 10);
WTF_EXPORT_PRIVATE unsigned charactersToUIntStrict(const UChar*, size_t, bool* ok = 0, int base = 10);
int64_t charactersToInt64Strict(const UChar*, size_t, bool* ok = 0, int base = 10);
uint64_t charactersToUInt64Strict(const UChar*, size_t, bool* ok = 0, int base = 10);
intptr_t charactersToIntPtrStrict(const UChar*, size_t, bool* ok = 0, int base = 10);

int charactersToInt(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
unsigned charactersToUInt(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
int64_t charactersToInt64(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
uint64_t charactersToUInt64(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage
intptr_t charactersToIntPtr(const UChar*, size_t, bool* ok = 0); // ignores trailing garbage

WTF_EXPORT_PRIVATE double charactersToDouble(const UChar*, size_t, bool* ok = 0, bool* didReadNumber = 0);
float charactersToFloat(const UChar*, size_t, bool* ok = 0, bool* didReadNumber = 0);

template<bool isSpecialCharacter(UChar)> bool isAllSpecialCharacters(const UChar*, size_t);

class String {
public:
    // Construct a null string, distinguishable from an empty string.
    WTF_EXPORT_PRIVATE String() { }

    // Construct a string with UTF-16 data.
    WTF_EXPORT_PRIVATE String(const UChar* characters, unsigned length);

    // Construct a string by copying the contents of a vector.  To avoid
    // copying, consider using String::adopt instead.
    template<size_t inlineCapacity>
    explicit String(const Vector<UChar, inlineCapacity>&);

    // Construct a string with UTF-16 data, from a null-terminated source.
    WTF_EXPORT_PRIVATE String(const UChar*);

    // Construct a string with latin1 data.
    WTF_EXPORT_PRIVATE String(const char* characters, unsigned length);

    // Construct a string with latin1 data, from a null-terminated source.
    WTF_EXPORT_PRIVATE String(const char* characters);

    // Construct a string referencing an existing StringImpl.
    WTF_EXPORT_PRIVATE String(StringImpl* impl) : m_impl(impl) { }
    WTF_EXPORT_PRIVATE String(PassRefPtr<StringImpl> impl) : m_impl(impl) { }
    WTF_EXPORT_PRIVATE String(RefPtr<StringImpl> impl) : m_impl(impl) { }

    // Inline the destructor.
    ALWAYS_INLINE ~String() { }

    void swap(String& o) { m_impl.swap(o.m_impl); }

    static String adopt(StringBuffer& buffer) { return StringImpl::adopt(buffer); }
    template<size_t inlineCapacity>
    static String adopt(Vector<UChar, inlineCapacity>& vector) { return StringImpl::adopt(vector); }

    bool isNull() const { return !m_impl; }
    bool isEmpty() const { return !m_impl || !m_impl->length(); }

    StringImpl* impl() const { return m_impl.get(); }

    unsigned length() const
    {
        if (!m_impl)
            return 0;
        return m_impl->length();
    }

    const UChar* characters() const
    {
        if (!m_impl)
            return 0;
        return m_impl->characters();
    }

    WTF_EXPORT_PRIVATE CString ascii() const;
    WTF_EXPORT_PRIVATE CString latin1() const;
    WTF_EXPORT_PRIVATE CString utf8(bool strict = false) const;

    UChar operator[](unsigned index) const
    {
        if (!m_impl || index >= m_impl->length())
            return 0;
        return m_impl->characters()[index];
    }

    WTF_EXPORT_PRIVATE static String number(short);
    WTF_EXPORT_PRIVATE static String number(unsigned short);
    WTF_EXPORT_PRIVATE static String number(int);
    WTF_EXPORT_PRIVATE static String number(unsigned);
    WTF_EXPORT_PRIVATE static String number(long);
    WTF_EXPORT_PRIVATE static String number(unsigned long);
    WTF_EXPORT_PRIVATE static String number(long long);
    WTF_EXPORT_PRIVATE static String number(unsigned long long);
    WTF_EXPORT_PRIVATE static String number(double);

    // Find a single character or string, also with match function & latin1 forms.
    size_t find(UChar c, unsigned start = 0) const
        { return m_impl ? m_impl->find(c, start) : notFound; }
    size_t find(const String& str, unsigned start = 0) const
        { return m_impl ? m_impl->find(str.impl(), start) : notFound; }
    size_t find(CharacterMatchFunctionPtr matchFunction, unsigned start = 0) const
        { return m_impl ? m_impl->find(matchFunction, start) : notFound; }
    size_t find(const char* str, unsigned start = 0) const
        { return m_impl ? m_impl->find(str, start) : notFound; }

    // Find the last instance of a single character or string.
    size_t reverseFind(UChar c, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFind(c, start) : notFound; }
    size_t reverseFind(const String& str, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFind(str.impl(), start) : notFound; }

    // Case insensitive string matching.
    WTF_EXPORT_PRIVATE size_t findIgnoringCase(const char* str, unsigned start = 0) const
        { return m_impl ? m_impl->findIgnoringCase(str, start) : notFound; }
    WTF_EXPORT_PRIVATE size_t findIgnoringCase(const String& str, unsigned start = 0) const
        { return m_impl ? m_impl->findIgnoringCase(str.impl(), start) : notFound; }
    size_t reverseFindIgnoringCase(const String& str, unsigned start = UINT_MAX) const
        { return m_impl ? m_impl->reverseFindIgnoringCase(str.impl(), start) : notFound; }

    // Wrappers for find & reverseFind adding dynamic sensitivity check.
    size_t find(const char* str, unsigned start, bool caseSensitive) const
        { return caseSensitive ? find(str, start) : findIgnoringCase(str, start); }
    size_t find(const String& str, unsigned start, bool caseSensitive) const
        { return caseSensitive ? find(str, start) : findIgnoringCase(str, start); }
    size_t reverseFind(const String& str, unsigned start, bool caseSensitive) const
        { return caseSensitive ? reverseFind(str, start) : reverseFindIgnoringCase(str, start); }

    WTF_EXPORT_PRIVATE const UChar* charactersWithNullTermination();
    
    WTF_EXPORT_PRIVATE UChar32 characterStartingAt(unsigned) const; // Ditto.
    
    bool contains(UChar c) const { return find(c) != notFound; }
    bool contains(const char* str, bool caseSensitive = true) const { return find(str, 0, caseSensitive) != notFound; }
    bool contains(const String& str, bool caseSensitive = true) const { return find(str, 0, caseSensitive) != notFound; }

    bool startsWith(const String& s, bool caseSensitive = true) const
        { return m_impl ? m_impl->startsWith(s.impl(), caseSensitive) : s.isEmpty(); }
    bool endsWith(const String& s, bool caseSensitive = true) const
        { return m_impl ? m_impl->endsWith(s.impl(), caseSensitive) : s.isEmpty(); }

    WTF_EXPORT_PRIVATE void append(const String&);
    WTF_EXPORT_PRIVATE void append(char);
    WTF_EXPORT_PRIVATE void append(UChar);
    WTF_EXPORT_PRIVATE void append(const UChar*, unsigned length);
    WTF_EXPORT_PRIVATE void insert(const String&, unsigned pos);
    void insert(const UChar*, unsigned length, unsigned pos);

    String& replace(UChar a, UChar b) { if (m_impl) m_impl = m_impl->replace(a, b); return *this; }
    String& replace(UChar a, const String& b) { if (m_impl) m_impl = m_impl->replace(a, b.impl()); return *this; }
    String& replace(const String& a, const String& b) { if (m_impl) m_impl = m_impl->replace(a.impl(), b.impl()); return *this; }
    String& replace(unsigned index, unsigned len, const String& b) { if (m_impl) m_impl = m_impl->replace(index, len, b.impl()); return *this; }

    void makeLower() { if (m_impl) m_impl = m_impl->lower(); }
    void makeUpper() { if (m_impl) m_impl = m_impl->upper(); }
    void fill(UChar c) { if (m_impl) m_impl = m_impl->fill(c); }

    WTF_EXPORT_PRIVATE void truncate(unsigned len);
    WTF_EXPORT_PRIVATE void remove(unsigned pos, int len = 1);

    WTF_EXPORT_PRIVATE String substring(unsigned pos, unsigned len = UINT_MAX) const;
    String substringSharingImpl(unsigned pos, unsigned len = UINT_MAX) const;
    String left(unsigned len) const { return substring(0, len); }
    String right(unsigned len) const { return substring(length() - len, len); }

    // Returns a lowercase/uppercase version of the string
    WTF_EXPORT_PRIVATE String lower() const;
    WTF_EXPORT_PRIVATE String upper() const;

    WTF_EXPORT_PRIVATE String stripWhiteSpace() const;
    WTF_EXPORT_PRIVATE String stripWhiteSpace(IsWhiteSpaceFunctionPtr) const;
    WTF_EXPORT_PRIVATE String simplifyWhiteSpace() const;
    WTF_EXPORT_PRIVATE String simplifyWhiteSpace(IsWhiteSpaceFunctionPtr) const;

    WTF_EXPORT_PRIVATE String removeCharacters(CharacterMatchFunctionPtr) const;
    template<bool isSpecialCharacter(UChar)> bool isAllSpecialCharacters() const;

    // Return the string with case folded for case insensitive comparison.
    WTF_EXPORT_PRIVATE String foldCase() const;

#if !PLATFORM(QT)
    WTF_EXPORT_PRIVATE static String format(const char *, ...) WTF_ATTRIBUTE_PRINTF(1, 2);
#else
    WTF_EXPORT_PRIVATE static String format(const char *, ...);
#endif

    // Returns an uninitialized string. The characters needs to be written
    // into the buffer returned in data before the returned string is used.
    // Failure to do this will have unpredictable results.
    static String createUninitialized(unsigned length, UChar*& data) { return StringImpl::createUninitialized(length, data); }

    WTF_EXPORT_PRIVATE void split(const String& separator, Vector<String>& result) const;
    WTF_EXPORT_PRIVATE void split(const String& separator, bool allowEmptyEntries, Vector<String>& result) const;
    WTF_EXPORT_PRIVATE void split(UChar separator, Vector<String>& result) const;
    WTF_EXPORT_PRIVATE void split(UChar separator, bool allowEmptyEntries, Vector<String>& result) const;

    WTF_EXPORT_PRIVATE int toIntStrict(bool* ok = 0, int base = 10) const;
    WTF_EXPORT_PRIVATE unsigned toUIntStrict(bool* ok = 0, int base = 10) const;
    WTF_EXPORT_PRIVATE int64_t toInt64Strict(bool* ok = 0, int base = 10) const;
    WTF_EXPORT_PRIVATE uint64_t toUInt64Strict(bool* ok = 0, int base = 10) const;
    WTF_EXPORT_PRIVATE intptr_t toIntPtrStrict(bool* ok = 0, int base = 10) const;

    WTF_EXPORT_PRIVATE int toInt(bool* ok = 0) const;
    WTF_EXPORT_PRIVATE unsigned toUInt(bool* ok = 0) const;
    int64_t toInt64(bool* ok = 0) const;
    WTF_EXPORT_PRIVATE uint64_t toUInt64(bool* ok = 0) const;
    WTF_EXPORT_PRIVATE intptr_t toIntPtr(bool* ok = 0) const;
    WTF_EXPORT_PRIVATE double toDouble(bool* ok = 0, bool* didReadNumber = 0) const;
    WTF_EXPORT_PRIVATE float toFloat(bool* ok = 0, bool* didReadNumber = 0) const;

    bool percentage(int& percentage) const;

    // Returns a StringImpl suitable for use on another thread.
    WTF_EXPORT_PRIVATE String crossThreadString() const;
    // Makes a deep copy. Helpful only if you need to use a String on another thread
    // (use crossThreadString if the method call doesn't need to be threadsafe).
    // Since the underlying StringImpl objects are immutable, there's no other reason
    // to ever prefer copy() over plain old assignment.
    WTF_EXPORT_PRIVATE String threadsafeCopy() const;

    // Prevent Strings from being implicitly convertable to bool as it will be ambiguous on any platform that
    // allows implicit conversion to another pointer type (e.g., Mac allows implicit conversion to NSString*).
    typedef struct ImplicitConversionFromWTFStringToBoolDisallowedA* (String::*UnspecifiedBoolTypeA);
    typedef struct ImplicitConversionFromWTFStringToBoolDisallowedB* (String::*UnspecifiedBoolTypeB);
    operator UnspecifiedBoolTypeA() const;
    operator UnspecifiedBoolTypeB() const;

#if USE(CF)
    String(CFStringRef);
    CFStringRef createCFString() const;
#endif

#ifdef __OBJC__
    String(NSString*);
    
    // This conversion maps NULL to "", which loses the meaning of NULL, but we 
    // need this mapping because AppKit crashes when passed nil NSStrings.
    operator NSString*() const { if (!m_impl) return @""; return *m_impl; }
#endif

#if PLATFORM(QT)
    String(const QString&);
    String(const QStringRef&);
    operator QString() const;
#endif

#if PLATFORM(WX)
    WTF_EXPORT_PRIVATE String(const wxString&);
    WTF_EXPORT_PRIVATE operator wxString() const;
#endif

    // String::fromUTF8 will return a null string if
    // the input data contains invalid UTF-8 sequences.
    WTF_EXPORT_PRIVATE static String fromUTF8(const char*, size_t);
    WTF_EXPORT_PRIVATE static String fromUTF8(const char*);

    // Tries to convert the passed in string to UTF-8, but will fall back to Latin-1 if the string is not valid UTF-8.
    WTF_EXPORT_PRIVATE static String fromUTF8WithLatin1Fallback(const char*, size_t);
    
    // Determines the writing direction using the Unicode Bidi Algorithm rules P2 and P3.
    WTF::Unicode::Direction defaultWritingDirection(bool* hasStrongDirectionality = 0) const
    {
        if (m_impl)
            return m_impl->defaultWritingDirection(hasStrongDirectionality);
        if (hasStrongDirectionality)
            *hasStrongDirectionality = false;
        return WTF::Unicode::LeftToRight;
    }

    bool containsOnlyASCII() const { return charactersAreAllASCII(characters(), length()); }
    bool containsOnlyLatin1() const { return charactersAreAllLatin1(characters(), length()); }

    // Hash table deleted values, which are only constructed and never copied or destroyed.
    String(WTF::HashTableDeletedValueType) : m_impl(WTF::HashTableDeletedValue) { }
    WTF_EXPORT_PRIVATE bool isHashTableDeletedValue() const { return m_impl.isHashTableDeletedValue(); }

#ifndef NDEBUG
    void show() const;
#endif

private:
    RefPtr<StringImpl> m_impl;
};

#if PLATFORM(QT)
QDataStream& operator<<(QDataStream& stream, const String& str);
QDataStream& operator>>(QDataStream& stream, String& str);
#endif

inline String& operator+=(String& a, const String& b) { a.append(b); return a; }

inline bool operator==(const String& a, const String& b) { return equal(a.impl(), b.impl()); }
inline bool operator==(const String& a, const char* b) { return equal(a.impl(), b); }
inline bool operator==(const char* a, const String& b) { return equal(a, b.impl()); }

inline bool operator!=(const String& a, const String& b) { return !equal(a.impl(), b.impl()); }
inline bool operator!=(const String& a, const char* b) { return !equal(a.impl(), b); }
inline bool operator!=(const char* a, const String& b) { return !equal(a, b.impl()); }

inline bool equalIgnoringCase(const String& a, const String& b) { return equalIgnoringCase(a.impl(), b.impl()); }
inline bool equalIgnoringCase(const String& a, const char* b) { return equalIgnoringCase(a.impl(), b); }
inline bool equalIgnoringCase(const char* a, const String& b) { return equalIgnoringCase(a, b.impl()); }

inline bool equalPossiblyIgnoringCase(const String& a, const String& b, bool ignoreCase) 
{
    return ignoreCase ? equalIgnoringCase(a, b) : (a == b);
}

inline bool equalIgnoringNullity(const String& a, const String& b) { return equalIgnoringNullity(a.impl(), b.impl()); }

template<size_t inlineCapacity>
inline bool equalIgnoringNullity(const Vector<UChar, inlineCapacity>& a, const String& b) { return equalIgnoringNullity(a, b.impl()); }

inline bool operator!(const String& str) { return str.isNull(); }

inline void swap(String& a, String& b) { a.swap(b); }

// Definitions of string operations

template<size_t inlineCapacity>
String::String(const Vector<UChar, inlineCapacity>& vector)
    : m_impl(vector.size() ? StringImpl::create(vector.data(), vector.size()) : 0)
{
}

#ifdef __OBJC__
// This is for situations in WebKit where the long standing behavior has been
// "nil if empty", so we try to maintain longstanding behavior for the sake of
// entrenched clients
inline NSString* nsStringNilIfEmpty(const String& str) {  return str.isEmpty() ? nil : (NSString*)str; }
#endif

inline bool charactersAreAllASCII(const UChar* characters, size_t length)
{
    UChar ored = 0;
    for (size_t i = 0; i < length; ++i)
        ored |= characters[i];
    return !(ored & 0xFF80);
}

inline bool charactersAreAllLatin1(const UChar* characters, size_t length)
{
    UChar ored = 0;
    for (size_t i = 0; i < length; ++i)
        ored |= characters[i];
    return !(ored & 0xFF00);
}

WTF_EXPORT_PRIVATE int codePointCompare(const String&, const String&);

inline size_t find(const UChar* characters, unsigned length, UChar matchCharacter, unsigned index = 0)
{
    while (index < length) {
        if (characters[index] == matchCharacter)
            return index;
        ++index;
    }
    return notFound;
}

inline size_t find(const UChar* characters, unsigned length, CharacterMatchFunctionPtr matchFunction, unsigned index = 0)
{
    while (index < length) {
        if (matchFunction(characters[index]))
            return index;
        ++index;
    }
    return notFound;
}

inline size_t reverseFind(const UChar* characters, unsigned length, UChar matchCharacter, unsigned index = UINT_MAX)
{
    if (!length)
        return notFound;
    if (index >= length)
        index = length - 1;
    while (characters[index] != matchCharacter) {
        if (!index--)
            return notFound;
    }
    return index;
}

inline void append(Vector<UChar>& vector, const String& string)
{
    vector.append(string.characters(), string.length());
}

inline void appendNumber(Vector<UChar>& vector, unsigned char number)
{
    int numberLength = number > 99 ? 3 : (number > 9 ? 2 : 1);
    size_t vectorSize = vector.size();
    vector.grow(vectorSize + numberLength);

    switch (numberLength) {
    case 3:
        vector[vectorSize + 2] = number % 10 + '0';
        number /= 10;

    case 2:
        vector[vectorSize + 1] = number % 10 + '0';
        number /= 10;

    case 1:
        vector[vectorSize] = number % 10 + '0';
    }
}

template<bool isSpecialCharacter(UChar)> inline bool isAllSpecialCharacters(const UChar* characters, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        if (!isSpecialCharacter(characters[i]))
            return false;
    }
    return true;
}

template<bool isSpecialCharacter(UChar)> inline bool String::isAllSpecialCharacters() const
{
    return WTF::isAllSpecialCharacters<isSpecialCharacter>(characters(), length());
}

// StringHash is the default hash for String
template<typename T> struct DefaultHash;
template<> struct DefaultHash<String> {
    typedef StringHash Hash;
};

template <> struct VectorTraits<String> : SimpleClassVectorTraits { };

// Shared global empty string.
WTF_EXPORT_PRIVATE const String& emptyString();

}

using WTF::CString;
using WTF::String;
using WTF::emptyString;
using WTF::append;
using WTF::appendNumber;
using WTF::charactersAreAllASCII;
using WTF::charactersAreAllLatin1;
using WTF::charactersToIntStrict;
using WTF::charactersToUIntStrict;
using WTF::charactersToInt64Strict;
using WTF::charactersToUInt64Strict;
using WTF::charactersToIntPtrStrict;
using WTF::charactersToInt;
using WTF::charactersToUInt;
using WTF::charactersToInt64;
using WTF::charactersToUInt64;
using WTF::charactersToIntPtr;
using WTF::charactersToDouble;
using WTF::charactersToFloat;
using WTF::equal;
using WTF::equalIgnoringCase;
using WTF::find;
using WTF::isAllSpecialCharacters;
using WTF::isSpaceOrNewline;
using WTF::reverseFind;

#include "AtomicString.h"
#endif
