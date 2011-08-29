/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebVTTToken_h
#define WebVTTToken_h

#if ENABLE(VIDEO_TRACK)

#include "MarkupTokenBase.h"

namespace WebCore {

class WebVTTTokenTypes {
public:
    enum Type {
        Uninitialized,
        Character,
        StartTag,
        EndTag,
        TimestampTag,
        EndOfFile,
    };
};

class WebVTTToken : public MarkupTokenBase<WebVTTTokenTypes> {
public:
    void appendToName(UChar character)
    {
        ASSERT(m_type == WebVTTTokenTypes::StartTag || m_type == WebVTTTokenTypes::EndTag);
        MarkupTokenBase<WebVTTTokenTypes>::appendToName(character);
    }
    
    const DataVector& name() const
    {
        return MarkupTokenBase<WebVTTTokenTypes>::name();
    }
    
    const DataVector& characters() const
    {
        ASSERT(m_type == Type::Character || m_type == Type::TimestampTag);
        return m_data;
    }
    
    void beginEmptyStartTag()
    {
        ASSERT(m_type == Type::Uninitialized);
        m_type = Type::StartTag;
        m_currentAttribute = 0;
        m_attributes.clear();
        m_data.clear();
    }
 
    void beginTimestampTag(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Type::Uninitialized);
        m_type = Type::TimestampTag;
        m_selfClosing = true;
        m_data.append(character);
    }
    
    void appendToTimestamp(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Type::TimestampTag);
        m_data.append(character);
    }

    void appendToClass(UChar character)
    {
        appendToStartType(character);
    }

    void addNewClass()
    {
        ASSERT(m_type == Type::StartTag);
        if (!m_classes.isEmpty())
            m_classes.append(' ');
        m_classes.append(m_currentBuffer);
        m_currentBuffer.clear();
    }
    
    const DataVector& classes() const
    {
        return m_classes;
    }

    void appendToAnnotation(UChar character)
    {
        appendToStartType(character);
    }
        
    void addNewAnnotation()
    {
        ASSERT(m_type == Type::StartTag);
        m_annotation.clear();
        m_annotation.append(m_currentBuffer);
        m_currentBuffer.clear();
    }
    
    const DataVector& annotation() const
    {
        return m_annotation;
    }
    
    void clear()
    {
        m_annotation.clear();
        m_classes.clear();
        m_currentBuffer.clear();
        MarkupTokenBase<WebVTTTokenTypes>::clear();
    }

private:
    void appendToStartType(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Type::StartTag);
        m_currentBuffer.append(character);
    }

    DataVector m_annotation;
    DataVector m_classes;
    DataVector m_currentBuffer;
};

}

#endif
#endif
