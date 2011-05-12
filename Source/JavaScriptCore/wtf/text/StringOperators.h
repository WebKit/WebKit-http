/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#ifndef StringOperators_h
#define StringOperators_h

namespace WTF {

template<typename StringType1, typename StringType2>
class StringAppend {
public:
    StringAppend(StringType1 string1, StringType2 string2)
        : m_string1(string1)
        , m_string2(string2)
    {
    }

    operator String() const
    {
        RefPtr<StringImpl> resultImpl = tryMakeString(m_string1, m_string2);
        if (!resultImpl)
            CRASH();
        return resultImpl.release();
    }

    operator AtomicString() const
    {
        return operator String();
    }

    void writeTo(UChar* destination)
    {
        StringTypeAdapter<StringType1> adapter1(m_string1);
        StringTypeAdapter<StringType2> adapter2(m_string2);
        adapter1.writeTo(destination);
        adapter2.writeTo(destination + adapter1.length());
    }

    unsigned length()
    {
        StringTypeAdapter<StringType1> adapter1(m_string1);
        StringTypeAdapter<StringType2> adapter2(m_string2);
        return adapter1.length() + adapter2.length();
    }    

private:
    StringType1 m_string1;
    StringType2 m_string2;
};

template<typename StringType1, typename StringType2>
class StringTypeAdapter<StringAppend<StringType1, StringType2> > {
public:
    StringTypeAdapter<StringAppend<StringType1, StringType2> >(StringAppend<StringType1, StringType2>& buffer)
        : m_buffer(buffer)
    {
    }

    unsigned length() { return m_buffer.length(); }
    void writeTo(UChar* destination) { m_buffer.writeTo(destination); }

private:
    StringAppend<StringType1, StringType2>& m_buffer;
};

inline StringAppend<const char*, String> operator+(const char* string1, const String& string2)
{
    return StringAppend<const char*, String>(string1, string2);
}

inline StringAppend<const char*, AtomicString> operator+(const char* string1, const AtomicString& string2)
{
    return StringAppend<const char*, AtomicString>(string1, string2);
}

template<typename T>
StringAppend<String, T> operator+(const String& string1, T string2)
{
    return StringAppend<String, T>(string1, string2);
}

template<typename U, typename V, typename W>
StringAppend<U, StringAppend<V, W> > operator+(U string1, const StringAppend<V, W>& string2)
{
    return StringAppend<U, StringAppend<V, W> >(string1, string2);
}

} // namespace WTF

#endif // StringOperators_h
