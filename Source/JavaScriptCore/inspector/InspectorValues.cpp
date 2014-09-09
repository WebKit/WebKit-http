/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "config.h"
#include "InspectorValues.h"

#include <wtf/DecimalNumber.h>
#include <wtf/dtoa.h>
#include <wtf/text/StringBuilder.h>

namespace Inspector {

namespace {

static const int stackLimit = 1000;

enum Token {
    OBJECT_BEGIN,
    OBJECT_END,
    ARRAY_BEGIN,
    ARRAY_END,
    STRING,
    NUMBER,
    BOOL_TRUE,
    BOOL_FALSE,
    NULL_TOKEN,
    LIST_SEPARATOR,
    OBJECT_PAIR_SEPARATOR,
    INVALID_TOKEN,
};

const char* const nullString = "null";
const char* const trueString = "true";
const char* const falseString = "false";

bool parseConstToken(const UChar* start, const UChar* end, const UChar** tokenEnd, const char* token)
{
    while (start < end && *token != '\0' && *start++ == *token++) { }
    if (*token != '\0')
        return false;
    *tokenEnd = start;
    return true;
}

bool readInt(const UChar* start, const UChar* end, const UChar** tokenEnd, bool canHaveLeadingZeros)
{
    if (start == end)
        return false;
    bool haveLeadingZero = '0' == *start;
    int length = 0;
    while (start < end && '0' <= *start && *start <= '9') {
        ++start;
        ++length;
    }
    if (!length)
        return false;
    if (!canHaveLeadingZeros && length > 1 && haveLeadingZero)
        return false;
    *tokenEnd = start;
    return true;
}

bool parseNumberToken(const UChar* start, const UChar* end, const UChar** tokenEnd)
{
    // We just grab the number here.  We validate the size in DecodeNumber.
    // According   to RFC4627, a valid number is: [minus] int [frac] [exp]
    if (start == end)
        return false;
    UChar c = *start;
    if ('-' == c)
        ++start;

    if (!readInt(start, end, &start, false))
        return false;
    if (start == end) {
        *tokenEnd = start;
        return true;
    }

    // Optional fraction part
    c = *start;
    if ('.' == c) {
        ++start;
        if (!readInt(start, end, &start, true))
            return false;
        if (start == end) {
            *tokenEnd = start;
            return true;
        }
        c = *start;
    }

    // Optional exponent part
    if ('e' == c || 'E' == c) {
        ++start;
        if (start == end)
            return false;
        c = *start;
        if ('-' == c || '+' == c) {
            ++start;
            if (start == end)
                return false;
        }
        if (!readInt(start, end, &start, true))
            return false;
    }

    *tokenEnd = start;
    return true;
}

bool readHexDigits(const UChar* start, const UChar* end, const UChar** tokenEnd, int digits)
{
    if (end - start < digits)
        return false;
    for (int i = 0; i < digits; ++i) {
        UChar c = *start++;
        if (!(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))
            return false;
    }
    *tokenEnd = start;
    return true;
}

bool parseStringToken(const UChar* start, const UChar* end, const UChar** tokenEnd)
{
    while (start < end) {
        UChar c = *start++;
        if ('\\' == c) {
            c = *start++;
            // Make sure the escaped char is valid.
            switch (c) {
            case 'x':
                if (!readHexDigits(start, end, &start, 2))
                    return false;
                break;
            case 'u':
                if (!readHexDigits(start, end, &start, 4))
                    return false;
                break;
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'v':
            case '"':
                break;
            default:
                return false;
            }
        } else if ('"' == c) {
            *tokenEnd = start;
            return true;
        }
    }
    return false;
}

Token parseToken(const UChar* start, const UChar* end, const UChar** tokenStart, const UChar** tokenEnd)
{
    while (start < end && isSpaceOrNewline(*start))
        ++start;

    if (start == end)
        return INVALID_TOKEN;

    *tokenStart = start;

    switch (*start) {
    case 'n':
        if (parseConstToken(start, end, tokenEnd, nullString))
            return NULL_TOKEN;
        break;
    case 't':
        if (parseConstToken(start, end, tokenEnd, trueString))
            return BOOL_TRUE;
        break;
    case 'f':
        if (parseConstToken(start, end, tokenEnd, falseString))
            return BOOL_FALSE;
        break;
    case '[':
        *tokenEnd = start + 1;
        return ARRAY_BEGIN;
    case ']':
        *tokenEnd = start + 1;
        return ARRAY_END;
    case ',':
        *tokenEnd = start + 1;
        return LIST_SEPARATOR;
    case '{':
        *tokenEnd = start + 1;
        return OBJECT_BEGIN;
    case '}':
        *tokenEnd = start + 1;
        return OBJECT_END;
    case ':':
        *tokenEnd = start + 1;
        return OBJECT_PAIR_SEPARATOR;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
        if (parseNumberToken(start, end, tokenEnd))
            return NUMBER;
        break;
    case '"':
        if (parseStringToken(start + 1, end, tokenEnd))
            return STRING;
        break;
    }
    return INVALID_TOKEN;
}

inline int hexToInt(UChar c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    ASSERT_NOT_REACHED();
    return 0;
}

bool decodeString(const UChar* start, const UChar* end, StringBuilder* output)
{
    while (start < end) {
        UChar c = *start++;
        if ('\\' != c) {
            output->append(c);
            continue;
        }
        c = *start++;
        switch (c) {
        case '"':
        case '/':
        case '\\':
            break;
        case 'b':
            c = '\b';
            break;
        case 'f':
            c = '\f';
            break;
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        case 'v':
            c = '\v';
            break;
        case 'x':
            c = (hexToInt(*start) << 4) +
                hexToInt(*(start + 1));
            start += 2;
            break;
        case 'u':
            c = (hexToInt(*start) << 12) +
                (hexToInt(*(start + 1)) << 8) +
                (hexToInt(*(start + 2)) << 4) +
                hexToInt(*(start + 3));
            start += 4;
            break;
        default:
            return false;
        }
        output->append(c);
    }
    return true;
}

bool decodeString(const UChar* start, const UChar* end, String* output)
{
    if (start == end) {
        *output = "";
        return true;
    }
    if (start > end)
        return false;
    StringBuilder buffer;
    buffer.reserveCapacity(end - start);
    if (!decodeString(start, end, &buffer))
        return false;
    *output = buffer.toString();
    return true;
}

PassRefPtr<InspectorValue> buildValue(const UChar* start, const UChar* end, const UChar** valueTokenEnd, int depth)
{
    if (depth > stackLimit)
        return nullptr;

    RefPtr<InspectorValue> result;
    const UChar* tokenStart;
    const UChar* tokenEnd;
    Token token = parseToken(start, end, &tokenStart, &tokenEnd);
    switch (token) {
    case INVALID_TOKEN:
        return nullptr;
    case NULL_TOKEN:
        result = InspectorValue::null();
        break;
    case BOOL_TRUE:
        result = InspectorBasicValue::create(true);
        break;
    case BOOL_FALSE:
        result = InspectorBasicValue::create(false);
        break;
    case NUMBER: {
        bool ok;
        double value = charactersToDouble(tokenStart, tokenEnd - tokenStart, &ok);
        if (!ok)
            return nullptr;
        result = InspectorBasicValue::create(value);
        break;
    }
    case STRING: {
        String value;
        bool ok = decodeString(tokenStart + 1, tokenEnd - 1, &value);
        if (!ok)
            return nullptr;
        result = InspectorString::create(value);
        break;
    }
    case ARRAY_BEGIN: {
        RefPtr<InspectorArray> array = InspectorArray::create();
        start = tokenEnd;
        token = parseToken(start, end, &tokenStart, &tokenEnd);
        while (token != ARRAY_END) {
            RefPtr<InspectorValue> arrayNode = buildValue(start, end, &tokenEnd, depth + 1);
            if (!arrayNode)
                return nullptr;
            array->pushValue(arrayNode);

            // After a list value, we expect a comma or the end of the list.
            start = tokenEnd;
            token = parseToken(start, end, &tokenStart, &tokenEnd);
            if (token == LIST_SEPARATOR) {
                start = tokenEnd;
                token = parseToken(start, end, &tokenStart, &tokenEnd);
                if (token == ARRAY_END)
                    return nullptr;
            } else if (token != ARRAY_END) {
                // Unexpected value after list value.  Bail out.
                return nullptr;
            }
        }
        if (token != ARRAY_END)
            return nullptr;
        result = array.release();
        break;
    }
    case OBJECT_BEGIN: {
        RefPtr<InspectorObject> object = InspectorObject::create();
        start = tokenEnd;
        token = parseToken(start, end, &tokenStart, &tokenEnd);
        while (token != OBJECT_END) {
            if (token != STRING)
                return nullptr;
            String key;
            if (!decodeString(tokenStart + 1, tokenEnd - 1, &key))
                return nullptr;
            start = tokenEnd;

            token = parseToken(start, end, &tokenStart, &tokenEnd);
            if (token != OBJECT_PAIR_SEPARATOR)
                return nullptr;
            start = tokenEnd;

            RefPtr<InspectorValue> value = buildValue(start, end, &tokenEnd, depth + 1);
            if (!value)
                return nullptr;
            object->setValue(key, value);
            start = tokenEnd;

            // After a key/value pair, we expect a comma or the end of the
            // object.
            token = parseToken(start, end, &tokenStart, &tokenEnd);
            if (token == LIST_SEPARATOR) {
                start = tokenEnd;
                token = parseToken(start, end, &tokenStart, &tokenEnd);
                 if (token == OBJECT_END)
                    return nullptr;
            } else if (token != OBJECT_END) {
                // Unexpected value after last object value.  Bail out.
                return nullptr;
            }
        }
        if (token != OBJECT_END)
            return nullptr;
        result = object.release();
        break;
    }

    default:
        // We got a token that's not a value.
        return nullptr;
    }
    *valueTokenEnd = tokenEnd;
    return result.release();
}

inline bool escapeChar(UChar c, StringBuilder* dst)
{
    switch (c) {
    case '\b': dst->append("\\b", 2); break;
    case '\f': dst->append("\\f", 2); break;
    case '\n': dst->append("\\n", 2); break;
    case '\r': dst->append("\\r", 2); break;
    case '\t': dst->append("\\t", 2); break;
    case '\\': dst->append("\\\\", 2); break;
    case '"': dst->append("\\\"", 2); break;
    default:
        return false;
    }
    return true;
}

inline void doubleQuoteString(const String& str, StringBuilder* dst)
{
    dst->append('"');
    for (unsigned i = 0; i < str.length(); ++i) {
        UChar c = str[i];
        if (!escapeChar(c, dst)) {
            if (c < 32 || c > 126 || c == '<' || c == '>') {
                // 1. Escaping <, > to prevent script execution.
                // 2. Technically, we could also pass through c > 126 as UTF8, but this
                //    is also optional.  It would also be a pain to implement here.
                dst->append(String::format("\\u%04X", c));
            } else
                dst->append(c);
        }
    }
    dst->append('"');
}

} // anonymous namespace

bool InspectorValue::asBoolean(bool*) const
{
    return false;
}

bool InspectorValue::asNumber(double*) const
{
    return false;
}

bool InspectorValue::asNumber(float*) const
{
    return false;
}

bool InspectorValue::asNumber(int*) const
{
    return false;
}

bool InspectorValue::asNumber(unsigned*) const
{
    return false;
}

bool InspectorValue::asNumber(long*) const
{
    return false;
}

bool InspectorValue::asNumber(long long*) const
{
    return false;
}

bool InspectorValue::asNumber(unsigned long*) const
{
    return false;
}

bool InspectorValue::asNumber(unsigned long long*) const
{
    return false;
}

bool InspectorValue::asString(String*) const
{
    return false;
}

bool InspectorValue::asValue(RefPtr<InspectorValue>* output)
{
    *output = this;
    return true;
}

bool InspectorValue::asObject(RefPtr<InspectorObject>*)
{
    return false;
}

bool InspectorValue::asArray(RefPtr<InspectorArray>*)
{
    return false;
}

PassRefPtr<InspectorObject> InspectorValue::asObject()
{
    return nullptr;
}

PassRefPtr<InspectorArray> InspectorValue::asArray()
{
    return nullptr;
}

PassRefPtr<InspectorValue> InspectorValue::parseJSON(const String& json)
{
    // FIXME: This whole file should just use StringView instead of UChar/length and avoid upconverting.
    auto characters = StringView(json).upconvertedCharacters();
    const UChar* start = characters;
    const UChar* end = start + json.length();
    const UChar* tokenEnd;
    RefPtr<InspectorValue> value = buildValue(start, end, &tokenEnd, 0);
    if (!value || tokenEnd != end)
        return nullptr;
    return value.release();
}

String InspectorValue::toJSONString() const
{
    StringBuilder result;
    result.reserveCapacity(512);
    writeJSON(&result);
    return result.toString();
}

void InspectorValue::writeJSON(StringBuilder* output) const
{
    ASSERT(m_type == TypeNull);
    output->append(nullString, 4);
}

bool InspectorBasicValue::asBoolean(bool* output) const
{
    if (type() != TypeBoolean)
        return false;
    *output = m_boolValue;
    return true;
}

bool InspectorBasicValue::asNumber(double* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = m_doubleValue;
    return true;
}

bool InspectorBasicValue::asNumber(float* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = static_cast<float>(m_doubleValue);
    return true;
}

bool InspectorBasicValue::asNumber(int* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = static_cast<int>(m_doubleValue);
    return true;
}

bool InspectorBasicValue::asNumber(unsigned* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = static_cast<unsigned>(m_doubleValue);
    return true;
}

bool InspectorBasicValue::asNumber(long* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = static_cast<long>(m_doubleValue);
    return true;
}

bool InspectorBasicValue::asNumber(long long* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = static_cast<long long>(m_doubleValue);
    return true;
}

bool InspectorBasicValue::asNumber(unsigned long* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = static_cast<unsigned long>(m_doubleValue);
    return true;
}

bool InspectorBasicValue::asNumber(unsigned long long* output) const
{
    if (type() != TypeNumber)
        return false;
    *output = static_cast<unsigned long long>(m_doubleValue);
    return true;
}

void InspectorBasicValue::writeJSON(StringBuilder* output) const
{
    ASSERT(type() == TypeBoolean || type() == TypeNumber);
    if (type() == TypeBoolean) {
        if (m_boolValue)
            output->append(trueString, 4);
        else
            output->append(falseString, 5);
    } else if (type() == TypeNumber) {
        NumberToLStringBuffer buffer;
        if (!std::isfinite(m_doubleValue)) {
            output->append(nullString, 4);
            return;
        }
        DecimalNumber decimal = m_doubleValue;
        unsigned length = 0;
        if (decimal.bufferLengthForStringDecimal() > WTF::NumberToStringBufferLength) {
            // Not enough room for decimal. Use exponential format.
            if (decimal.bufferLengthForStringExponential() > WTF::NumberToStringBufferLength) {
                // Fallback for an abnormal case if it's too little even for exponential.
                output->append("NaN", 3);
                return;
            }
            length = decimal.toStringExponential(buffer, WTF::NumberToStringBufferLength);
        } else
            length = decimal.toStringDecimal(buffer, WTF::NumberToStringBufferLength);
        output->append(buffer, length);
    }
}

bool InspectorString::asString(String* output) const
{
    *output = m_stringValue;
    return true;
}

void InspectorString::writeJSON(StringBuilder* output) const
{
    ASSERT(type() == TypeString);
    doubleQuoteString(m_stringValue, output);
}

InspectorObjectBase::~InspectorObjectBase()
{
}

bool InspectorObjectBase::asObject(RefPtr<InspectorObject>* output)
{
    COMPILE_ASSERT(sizeof(InspectorObject) == sizeof(InspectorObjectBase), cannot_cast);
    *output = static_cast<InspectorObject*>(this);
    return true;
}

PassRefPtr<InspectorObject> InspectorObjectBase::asObject()
{
    return openAccessors();
}

InspectorObject* InspectorObjectBase::openAccessors()
{
    COMPILE_ASSERT(sizeof(InspectorObject) == sizeof(InspectorObjectBase), cannot_cast);
    return static_cast<InspectorObject*>(this);
}

bool InspectorObjectBase::getBoolean(const String& name, bool* output) const
{
    RefPtr<InspectorValue> value = get(name);
    if (!value)
        return false;
    return value->asBoolean(output);
}

bool InspectorObjectBase::getString(const String& name, String* output) const
{
    RefPtr<InspectorValue> value = get(name);
    if (!value)
        return false;
    return value->asString(output);
}

PassRefPtr<InspectorObject> InspectorObjectBase::getObject(const String& name) const
{
    PassRefPtr<InspectorValue> value = get(name);
    if (!value)
        return nullptr;
    return value->asObject();
}

PassRefPtr<InspectorArray> InspectorObjectBase::getArray(const String& name) const
{
    PassRefPtr<InspectorValue> value = get(name);
    if (!value)
        return nullptr;
    return value->asArray();
}

PassRefPtr<InspectorValue> InspectorObjectBase::get(const String& name) const
{
    Dictionary::const_iterator it = m_data.find(name);
    if (it == m_data.end())
        return nullptr;
    return it->value;
}

void InspectorObjectBase::remove(const String& name)
{
    m_data.remove(name);
    for (size_t i = 0; i < m_order.size(); ++i) {
        if (m_order[i] == name) {
            m_order.remove(i);
            break;
        }
    }
}

void InspectorObjectBase::writeJSON(StringBuilder* output) const
{
    output->append('{');
    for (size_t i = 0; i < m_order.size(); ++i) {
        Dictionary::const_iterator it = m_data.find(m_order[i]);
        ASSERT(it != m_data.end());
        if (i)
            output->append(',');
        doubleQuoteString(it->key, output);
        output->append(':');
        it->value->writeJSON(output);
    }
    output->append('}');
}

InspectorObjectBase::InspectorObjectBase()
    : InspectorValue(TypeObject)
    , m_data()
    , m_order()
{
}

InspectorArrayBase::~InspectorArrayBase()
{
}

bool InspectorArrayBase::asArray(RefPtr<InspectorArray>* output)
{
    COMPILE_ASSERT(sizeof(InspectorArrayBase) == sizeof(InspectorArray), cannot_cast);
    *output = static_cast<InspectorArray*>(this);
    return true;
}

PassRefPtr<InspectorArray> InspectorArrayBase::asArray()
{
    COMPILE_ASSERT(sizeof(InspectorArrayBase) == sizeof(InspectorArray), cannot_cast);
    return static_cast<InspectorArray*>(this);
}

void InspectorArrayBase::writeJSON(StringBuilder* output) const
{
    output->append('[');
    for (Vector<RefPtr<InspectorValue>>::const_iterator it = m_data.begin(); it != m_data.end(); ++it) {
        if (it != m_data.begin())
            output->append(',');
        (*it)->writeJSON(output);
    }
    output->append(']');
}

InspectorArrayBase::InspectorArrayBase()
    : InspectorValue(TypeArray)
    , m_data()
{
}

PassRefPtr<InspectorValue> InspectorArrayBase::get(size_t index)
{
    ASSERT_WITH_SECURITY_IMPLICATION(index < m_data.size());
    return m_data[index];
}

PassRefPtr<InspectorObject> InspectorObject::create()
{
    return adoptRef(new InspectorObject);
}

PassRefPtr<InspectorArray> InspectorArray::create()
{
    return adoptRef(new InspectorArray);
}

PassRefPtr<InspectorValue> InspectorValue::null()
{
    return adoptRef(new InspectorValue);
}

PassRefPtr<InspectorString> InspectorString::create(const String& value)
{
    return adoptRef(new InspectorString(value));
}

PassRefPtr<InspectorString> InspectorString::create(const char* value)
{
    return adoptRef(new InspectorString(value));
}

PassRefPtr<InspectorBasicValue> InspectorBasicValue::create(bool value)
{
    return adoptRef(new InspectorBasicValue(value));
}

PassRefPtr<InspectorBasicValue> InspectorBasicValue::create(int value)
{
    return adoptRef(new InspectorBasicValue(value));
}

PassRefPtr<InspectorBasicValue> InspectorBasicValue::create(double value)
{
    return adoptRef(new InspectorBasicValue(value));
}

} // namespace Inspector
