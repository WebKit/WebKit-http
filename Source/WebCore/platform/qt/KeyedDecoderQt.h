/*
 * Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
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

#ifndef KeyedDecoderQt_h
#define KeyedDecoderQt_h

#include "KeyedCoding.h"
#include <QVariantMap>
#include <wtf/Vector.h>

namespace WebCore {

class KeyedDecoderQt final : public KeyedDecoder {
public:
    KeyedDecoderQt(const uint8_t* data, size_t);
    KeyedDecoderQt(QVariantMap&& data);

    bool decodeVariant(const String& key, QVariant&);

public:
    bool decodeBytes(const String& key, const uint8_t*&, size_t&) override;
    bool decodeBool(const String& key, bool&) override;
    bool decodeUInt32(const String& key, uint32_t&) override;
    bool decodeInt32(const String& key, int32_t&) override;
    bool decodeInt64(const String& key, int64_t&) override;
    bool decodeFloat(const String& key, float&) override;
    bool decodeDouble(const String& key, double&) override;
    bool decodeString(const String& key, String&) override;

private:
    bool beginObject(const String& key) override;
    void endObject() override;

    bool beginArray(const String& key) override;
    bool beginArrayElement() override;
    void endArrayElement() override;
    void endArray() override;

    template <typename T> bool decodeSimpleValue(const String& key, T& result);
    template <typename T, typename F> bool decodeNumber(const String& key, T& result, F&& function);
    const QVariantMap& currentObject() { return m_objectStack.last(); }

    Vector<QVariantMap, 16> m_objectStack;
    Vector<QVariantList, 16> m_arrayStack;
    Vector<int> m_arrayIndexStack;
};

} // namespace WebCore

#endif // KeyedDecoderQt_h
