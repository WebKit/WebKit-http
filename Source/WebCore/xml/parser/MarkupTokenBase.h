/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#ifndef MarkupTokenBase_h
#define MarkupTokenBase_h

#include "NamedNodeMap.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

namespace WebCore {

class DoctypeDataBase {
    WTF_MAKE_NONCOPYABLE(DoctypeDataBase);
public:
    DoctypeDataBase()
        : m_hasPublicIdentifier(false)
        , m_hasSystemIdentifier(false)
    {
    }

    bool m_hasPublicIdentifier;
    bool m_hasSystemIdentifier;
    WTF::Vector<UChar> m_publicIdentifier;
    WTF::Vector<UChar> m_systemIdentifier;
};
    
template<typename TypeSet, typename DoctypeData = DoctypeDataBase>
class MarkupTokenBase {
    WTF_MAKE_NONCOPYABLE(MarkupTokenBase);
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef typename TypeSet::Type Type;

    class Range {
    public:
        int m_start;
        int m_end;
    };

    class Attribute {
    public:
        Range m_nameRange;
        Range m_valueRange;
        WTF::Vector<UChar, 32> m_name;
        WTF::Vector<UChar, 32> m_value;
    };

    typedef WTF::Vector<Attribute, 10> AttributeList;
    typedef WTF::Vector<UChar, 1024> DataVector;

    MarkupTokenBase() { clear(); }
    virtual ~MarkupTokenBase() { }

    virtual void clear()
    {
        m_type = TypeSet::Uninitialized;
        m_range.m_start = 0;
        m_range.m_end = 0;
        m_baseOffset = 0;
        m_data.clear();
    }

    bool isUninitialized() { return m_type == TypeSet::Uninitialized; }

    int startIndex() const { return m_range.m_start; }
    int endIndex() const { return m_range.m_end; }

    void setBaseOffset(int offset)
    {
        m_baseOffset = offset;
    }

    void end(int endOffset)
    {
        m_range.m_end = endOffset - m_baseOffset;
    }

    void makeEndOfFile()
    {
        ASSERT(m_type == TypeSet::Uninitialized);
        m_type = TypeSet::EndOfFile;
    }

    void beginStartTag(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == TypeSet::Uninitialized);
        m_type = TypeSet::StartTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        m_data.append(character);
    }

    template<typename T>
    void beginEndTag(T characters)
    {
        ASSERT(m_type == TypeSet::Uninitialized);
        m_type = TypeSet::EndTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        m_data.append(characters);
    }

    // Starting a character token works slightly differently than starting
    // other types of tokens because we want to save a per-character branch.
    void ensureIsCharacterToken()
    {
        ASSERT(m_type == TypeSet::Uninitialized || m_type == TypeSet::Character);
        m_type = TypeSet::Character;
    }

    void beginComment()
    {
        ASSERT(m_type == TypeSet::Uninitialized);
        m_type = TypeSet::Comment;
    }

    void beginDOCTYPE()
    {
        ASSERT(m_type == TypeSet::Uninitialized);
        m_type = TypeSet::DOCTYPE;
        m_doctypeData = adoptPtr(new DoctypeData());
    }

    void beginDOCTYPE(UChar character)
    {
        ASSERT(character);
        beginDOCTYPE();
        m_data.append(character);
    }

    template<typename T>
    void appendToCharacter(T characters)
    {
        ASSERT(m_type == TypeSet::Character);
        m_data.append(characters);
    }

    void appendToComment(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == TypeSet::Comment);
        m_data.append(character);
    }

    void addNewAttribute()
    {
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        m_attributes.grow(m_attributes.size() + 1);
        m_currentAttribute = &m_attributes.last();
#ifndef NDEBUG
        m_currentAttribute->m_nameRange.m_start = 0;
        m_currentAttribute->m_nameRange.m_end = 0;
        m_currentAttribute->m_valueRange.m_start = 0;
        m_currentAttribute->m_valueRange.m_end = 0;
#endif
    }

    void beginAttributeName(int offset)
    {
        m_currentAttribute->m_nameRange.m_start = offset - m_baseOffset;
    }

    void endAttributeName(int offset)
    {
        int index = offset - m_baseOffset;
        m_currentAttribute->m_nameRange.m_end = index;
        m_currentAttribute->m_valueRange.m_start = index;
        m_currentAttribute->m_valueRange.m_end = index;
    }

    void beginAttributeValue(int offset)
    {
        m_currentAttribute->m_valueRange.m_start = offset - m_baseOffset;
#ifndef NDEBUG
        m_currentAttribute->m_valueRange.m_end = 0;
#endif
    }

    void endAttributeValue(int offset)
    {
        m_currentAttribute->m_valueRange.m_end = offset - m_baseOffset;
    }

    void appendToAttributeName(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        // FIXME: We should be able to add the following ASSERT once we fix
        // https://bugs.webkit.org/show_bug.cgi?id=62971
        //   ASSERT(m_currentAttribute->m_nameRange.m_start);
        m_currentAttribute->m_name.append(character);
    }

    void appendToAttributeValue(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        ASSERT(m_currentAttribute->m_valueRange.m_start);
        m_currentAttribute->m_value.append(character);
    }

    void appendToAttributeValue(size_t i, const String& value)
    {
        ASSERT(!value.isEmpty());
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        m_attributes[i].m_value.append(value.characters(), value.length());
    }

    Type type() const { return m_type; }

    bool selfClosing() const
    {
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        return m_selfClosing;
    }

    void setSelfClosing()
    {
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        m_selfClosing = true;
    }

    const AttributeList& attributes() const
    {
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        return m_attributes;
    }

    void eraseCharacters()
    {
        ASSERT(m_type == TypeSet::Character);
        m_data.clear();
    }

    void eraseValueOfAttribute(size_t i)
    {
        ASSERT(m_type == TypeSet::StartTag || m_type == TypeSet::EndTag);
        m_attributes[i].m_value.clear();
    }

    const DataVector& characters() const
    {
        ASSERT(m_type == TypeSet::Character);
        return m_data;
    }

    const DataVector& comment() const
    {
        ASSERT(m_type == TypeSet::Comment);
        return m_data;
    }

    // FIXME: Distinguish between a missing public identifer and an empty one.
    const WTF::Vector<UChar>& publicIdentifier() const
    {
        ASSERT(m_type == TypeSet::DOCTYPE);
        return m_doctypeData->m_publicIdentifier;
    }

    // FIXME: Distinguish between a missing system identifer and an empty one.
    const WTF::Vector<UChar>& systemIdentifier() const
    {
        ASSERT(m_type == TypeSet::DOCTYPE);
        return m_doctypeData->m_systemIdentifier;
    }

    void setPublicIdentifierToEmptyString()
    {
        ASSERT(m_type == TypeSet::DOCTYPE);
        m_doctypeData->m_hasPublicIdentifier = true;
        m_doctypeData->m_publicIdentifier.clear();
    }

    void setSystemIdentifierToEmptyString()
    {
        ASSERT(m_type == TypeSet::DOCTYPE);
        m_doctypeData->m_hasSystemIdentifier = true;
        m_doctypeData->m_systemIdentifier.clear();
    }

    void appendToPublicIdentifier(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == TypeSet::DOCTYPE);
        ASSERT(m_doctypeData->m_hasPublicIdentifier);
        m_doctypeData->m_publicIdentifier.append(character);
    }

    void appendToSystemIdentifier(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == TypeSet::DOCTYPE);
        ASSERT(m_doctypeData->m_hasSystemIdentifier);
        m_doctypeData->m_systemIdentifier.append(character);
    }

protected:

#ifndef NDEBUG
    void printString(const DataVector& string) const
    {
        DataVector::const_iterator iter = string.begin();
        for (; iter != string.end(); ++iter)
            printf("%lc", wchar_t(*iter));
    }
    
    void printAttrs() const
    {
        typename AttributeList::const_iterator iter = m_attributes.begin();
        for (; iter != m_attributes.end(); ++iter) {
            printf(" ");
            printString(iter->m_name);
            printf("=\"");
            printString(iter->m_value);
            printf("\"");
        }
    }
#endif // NDEBUG

    inline void appendToName(UChar character)
    {
        ASSERT(character);
        m_data.append(character);
    }
    
    inline const DataVector& name() const
    {
        return m_data;
    }

    // FIXME: I'm not sure what the final relationship between HTMLToken and
    // AtomicHTMLToken will be. I'm marking this a friend for now, but we'll
    // want to end up with a cleaner interface between the two classes.
    friend class AtomicHTMLToken;

    Type m_type;
    Range m_range; // Always starts at zero.
    int m_baseOffset;
    DataVector m_data;

    // For DOCTYPE
    OwnPtr<DoctypeData> m_doctypeData;

    // For StartTag and EndTag
    bool m_selfClosing;
    AttributeList m_attributes;

    // A pointer into m_attributes used during lexing.
    Attribute* m_currentAttribute;
};

}

#endif // MarkupTokenBase_h
