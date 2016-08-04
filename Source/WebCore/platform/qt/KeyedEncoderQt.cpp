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
#include "KeyedEncoderQt.h"

#include "SharedBuffer.h"

#include <QDataStream>
#include <QIODevice>

namespace WebCore {

std::unique_ptr<KeyedEncoder> KeyedEncoder::encoder()
{
    return std::make_unique<KeyedEncoderQt>();
}

KeyedEncoderQt::KeyedEncoderQt()
{
    m_objectStack.constructAndAppend(QString(), QVariantMap());
}

void KeyedEncoderQt::encodeBytes(const String& key, const uint8_t* bytes, size_t size)
{
    currentObject().insert(key, QByteArray::fromRawData(reinterpret_cast<const char*>(bytes), size));
}

void KeyedEncoderQt::encodeBool(const String& key, bool value)
{
    currentObject().insert(key, value);
}

void KeyedEncoderQt::encodeUInt32(const String& key, uint32_t value)
{
    currentObject().insert(key, value);
}

void KeyedEncoderQt::encodeInt32(const String& key, int32_t value)
{
    currentObject().insert(key, value);
}

void KeyedEncoderQt::encodeInt64(const String& key, int64_t value)
{
    currentObject().insert(key, (qint64)value);
}

void KeyedEncoderQt::encodeFloat(const String& key, float value)
{
    currentObject().insert(key, value);
}

void KeyedEncoderQt::encodeDouble(const String& key, double value)
{
    currentObject().insert(key, value);
}

void KeyedEncoderQt::encodeString(const String& key, const String& value)
{
    currentObject().insert(key, QString(value));
}

void KeyedEncoderQt::encodeVariant(const String& key, const QVariant& value)
{
    currentObject().insert(key, value);
}

void KeyedEncoderQt::beginObject(const String& key)
{
    m_objectStack.constructAndAppend(key, QVariantMap());
}

void KeyedEncoderQt::endObject()
{
    auto objectKV = m_objectStack.takeLast();
    currentObject().insert(objectKV.first, objectKV.second);
}

void KeyedEncoderQt::beginArray(const String& key)
{
    m_arrayStack.constructAndAppend(key, QVariantList());
}

void KeyedEncoderQt::beginArrayElement()
{
    m_objectStack.constructAndAppend(QString(), QVariantMap());
}

void KeyedEncoderQt::endArrayElement()
{
    auto objectKV = m_objectStack.takeLast();
    currentArray().append(objectKV.second);
}

void KeyedEncoderQt::endArray()
{
    auto arrayKV = m_arrayStack.takeLast();
    currentObject().insert(arrayKV.first, arrayKV.second);
}

PassRefPtr<SharedBuffer> KeyedEncoderQt::finishEncoding()
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << toMap();
    // TODO: Wrap SharedBuffer around QByteArray when it's possible
    return SharedBuffer::create(data.data(), data.size());
}

QVariantMap& KeyedEncoderQt::toMap()
{
    return m_objectStack.first().second;
}

} // namespace WebCore
