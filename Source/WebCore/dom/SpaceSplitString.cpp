/*
 * Copyright (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2007, 2008, 2011, 2012 Apple Inc. All rights reserved.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "SpaceSplitString.h"

#include "HTMLParserIdioms.h"
#include <wtf/ASCIICType.h>
#include <wtf/HashMap.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/StringBuilder.h>

using namespace WTF;

namespace WebCore {

template <typename CharacterType>
static inline bool hasNonASCIIOrUpper(const CharacterType* characters, unsigned length)
{
    bool hasUpper = false;
    CharacterType ored = 0;
    for (unsigned i = 0; i < length; i++) {
        CharacterType c = characters[i];
        hasUpper |= isASCIIUpper(c);
        ored |= c;
    }
    return hasUpper || (ored & ~0x7F);
}

static inline bool hasNonASCIIOrUpper(const String& string)
{
    unsigned length = string.length();

    if (string.is8Bit())
        return hasNonASCIIOrUpper(string.characters8(), length);
    return hasNonASCIIOrUpper(string.characters16(), length);
}

template <typename CharacterType>
inline void SpaceSplitStringData::createVector(const CharacterType* characters, unsigned length)
{
    unsigned start = 0;
    while (true) {
        while (start < length && isHTMLSpace(characters[start]))
            ++start;
        if (start >= length)
            break;
        unsigned end = start + 1;
        while (end < length && isNotHTMLSpace(characters[end]))
            ++end;

        m_vector.append(AtomicString(characters + start, end - start));

        start = end + 1;
    }
}

void SpaceSplitStringData::createVector(const String& string)
{
    unsigned length = string.length();

    if (string.is8Bit()) {
        createVector(string.characters8(), length);
        return;
    }

    createVector(string.characters16(), length);
}

bool SpaceSplitStringData::containsAll(SpaceSplitStringData& other)
{
    if (this == &other)
        return true;

    size_t thisSize = m_vector.size();
    size_t otherSize = other.m_vector.size();
    for (size_t i = 0; i < otherSize; ++i) {
        const AtomicString& name = other.m_vector[i];
        size_t j;
        for (j = 0; j < thisSize; ++j) {
            if (m_vector[j] == name)
                break;
        }
        if (j == thisSize)
            return false;
    }
    return true;
}

void SpaceSplitStringData::add(const AtomicString& string)
{
    ASSERT(hasOneRef());
    if (contains(string))
        return;

    m_vector.append(string);
}

void SpaceSplitStringData::remove(const AtomicString& string)
{
    ASSERT(hasOneRef());
    size_t position = 0;
    while (position < m_vector.size()) {
        if (m_vector[position] == string)
            m_vector.remove(position);
        else
            ++position;
    }
}

void SpaceSplitString::add(const AtomicString& string)
{
    ensureUnique();
    if (m_data)
        m_data->add(string);
}

void SpaceSplitString::remove(const AtomicString& string)
{
    ensureUnique();
    if (m_data)
        m_data->remove(string);
}

typedef HashMap<AtomicString, SpaceSplitStringData*> SpaceSplitStringDataMap;

static SpaceSplitStringDataMap& sharedDataMap()
{
    DEFINE_STATIC_LOCAL(SpaceSplitStringDataMap, map, ());
    return map;
}

void SpaceSplitString::set(const AtomicString& inputString, bool shouldFoldCase)
{
    if (inputString.isNull()) {
        clear();
        return;
    }

    String string(inputString.string());
    if (shouldFoldCase && hasNonASCIIOrUpper(string))
        string = string.foldCase();

    m_data = SpaceSplitStringData::create(string);
}

SpaceSplitStringData::~SpaceSplitStringData()
{
    if (!m_keyString.isNull())
        sharedDataMap().remove(m_keyString);
}

PassRefPtr<SpaceSplitStringData> SpaceSplitStringData::create(const AtomicString& string)
{
    SpaceSplitStringData*& data = sharedDataMap().add(string, 0).iterator->second;
    if (!data) {
        data = new SpaceSplitStringData(string);
        return adoptRef(data);
    }
    return data;
}

PassRefPtr<SpaceSplitStringData> SpaceSplitStringData::createUnique(const SpaceSplitStringData& other)
{
    return adoptRef(new SpaceSplitStringData(other));
}

SpaceSplitStringData::SpaceSplitStringData(const AtomicString& string)
    : m_keyString(string)
{
    ASSERT(!string.isNull());
    createVector(string);
}

SpaceSplitStringData::SpaceSplitStringData(const SpaceSplitStringData& other)
    : RefCounted<SpaceSplitStringData>()
    , m_vector(other.m_vector)
{
    // Note that we don't copy m_keyString to indicate to the destructor that there's nothing
    // to be removed from the sharedDataMap().
}

} // namespace WebCore
