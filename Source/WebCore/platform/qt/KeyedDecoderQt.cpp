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

#include "config.h"
#include "KeyedDecoderQt.h"

#include <QDataStream>
#include <QIODevice>
#include <wtf/text/WTFString.h>

namespace WebCore {

std::unique_ptr<KeyedDecoder> KeyedDecoder::decoder(const uint8_t* data, size_t size)
{
    return std::make_unique<KeyedDecoderQt>(data, size);
}

KeyedDecoderQt::KeyedDecoderQt(const uint8_t* data, size_t size)
{
    m_objectStack.append(QVariantMap());

    auto buffer = QByteArray::fromRawData(reinterpret_cast<const char*>(data), size);
    QDataStream stream(&buffer, QIODevice::ReadOnly);
    stream >> m_objectStack.last();
}

KeyedDecoderQt::KeyedDecoderQt(QVariantMap&& data)
{
    m_objectStack.append(data);
}

bool KeyedDecoderQt::decodeVariant(const WTF::String& key, QVariant& result)
{
    auto it = currentObject().find(key);
    if (it == currentObject().end())
        return false;

    result = *it;
    return true;
}

template <typename T>
bool KeyedDecoderQt::decodeSimpleValue(const String& key, T& result)
{
    auto it = currentObject().find(key);
    if (it == currentObject().end())
        return false;

    if (!it->canConvert<T>())
        return false;

    result = it->value<T>();
    return true;
}


template <typename T, typename F>
bool KeyedDecoderQt::decodeNumber(const String& key, T& result, F&& function)
{
    auto it = currentObject().find(key);
    if (it == currentObject().end())
        return false;

    bool ok;
    T t = function(*it, &ok);
    if (ok)
        result = t;
    return ok;
}

bool KeyedDecoderQt::decodeBytes(const String& key, const uint8_t*& bytes, size_t& size)
{
    QByteArray value;
    if (!decodeSimpleValue(key, value))
        return false;

    bytes = reinterpret_cast<const uint8_t*>(value.constData());
    size = value.size();
    return true;
}

bool KeyedDecoderQt::decodeBool(const String& key, bool& value)
{
    return decodeSimpleValue(key, value);
}

bool KeyedDecoderQt::decodeUInt32(const String& key, uint32_t& value)
{
    return decodeNumber(key, value, [](const QVariant& var, bool* ok) {
        return var.toUInt(ok);
    });
}

bool KeyedDecoderQt::decodeInt32(const String& key, int32_t& value)
{
    return decodeNumber(key, value, [](const QVariant& var, bool* ok) {
        return var.toInt(ok);
    });
}

bool KeyedDecoderQt::decodeInt64(const String& key, int64_t& value)
{
    return decodeNumber(key, value, [](const QVariant& var, bool* ok) {
        return var.toLongLong(ok);
    });
}

bool KeyedDecoderQt::decodeFloat(const String& key, float& value)
{
    return decodeNumber(key, value, [](const QVariant& var, bool* ok) {
        return var.toFloat(ok);
    });
}

bool KeyedDecoderQt::decodeDouble(const String& key, double& value)
{
    return decodeNumber(key, value, [](const QVariant& var, bool* ok) {
        return var.toDouble(ok);
    });
}

bool KeyedDecoderQt::decodeString(const String& key, String& string)
{
    QString value;
    if (!decodeSimpleValue(key, value))
        return false;

    string = value;
    return true;
}

bool KeyedDecoderQt::beginObject(const String& key)
{
    QVariantMap value;
    if (!decodeSimpleValue(key, value))
        return false;

    m_objectStack.append(value);
    return true;
}

void KeyedDecoderQt::endObject()
{
    m_objectStack.removeLast();
}

bool KeyedDecoderQt::beginArray(const String& key)
{
    QVariantList value;
    if (!decodeSimpleValue(key, value))
        return false;

    m_arrayStack.append(value);
    m_arrayIndexStack.append(0);
    return true;
}

bool KeyedDecoderQt::beginArrayElement()
{
    if (m_arrayIndexStack.last() >= m_arrayStack.last().size())
        return false;

    QVariant value = m_arrayStack.last().at(m_arrayIndexStack.last()++);
    if (value.type() != QVariant::Map)
        return false;

    m_objectStack.append(value.toMap());
    return true;
}

void KeyedDecoderQt::endArrayElement()
{
    m_objectStack.removeLast();
}

void KeyedDecoderQt::endArray()
{
    m_arrayStack.removeLast();
    m_arrayIndexStack.removeLast();
}

} // namespace WebCore
