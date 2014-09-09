/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef IDBKeyData_h
#define IDBKeyData_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBKey.h"

namespace WebCore {

class KeyedDecoder;
class KeyedEncoder;

struct IDBKeyData {
    IDBKeyData()
        : type(IDBKey::InvalidType)
        , numberValue(0)
        , isNull(true)
    {
    }

    WEBCORE_EXPORT IDBKeyData(const IDBKey*);

    static IDBKeyData minimum()
    {
        IDBKeyData result;
        result.type = IDBKey::MinType;
        result.isNull = false;
        return result;
    }

    static IDBKeyData maximum()
    {
        IDBKeyData result;
        result.type = IDBKey::MaxType;
        result.isNull = false;
        return result;
    }

    WEBCORE_EXPORT PassRefPtr<IDBKey> maybeCreateIDBKey() const;

    IDBKeyData isolatedCopy() const;

    WEBCORE_EXPORT void encode(KeyedEncoder&) const;
    WEBCORE_EXPORT static bool decode(KeyedDecoder&, IDBKeyData&);

    // compare() has the same semantics as strcmp().
    //   - Returns negative if this IDBKeyData is less than other.
    //   - Returns positive if this IDBKeyData is greater than other.
    //   - Returns zero if this IDBKeyData is equal to other.
    WEBCORE_EXPORT int compare(const IDBKeyData& other) const;

    void setArrayValue(const Vector<IDBKeyData>&);
    void setStringValue(const String&);
    void setDateValue(double);
    WEBCORE_EXPORT void setNumberValue(double);

#ifndef NDEBUG
    WEBCORE_EXPORT String loggingString() const;
#endif

    IDBKey::Type type;
    Vector<IDBKeyData> arrayValue;
    String stringValue;
    double numberValue;
    bool isNull;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
#endif // IDBKeyData_h
