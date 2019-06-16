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

#include <QtTest/QtTest>

using WebCore::KeyedEncoder;
using WebCore::KeyedEncoderQt;

class tst_KeyedEncoderQt : public QObject {
    Q_OBJECT
    std::unique_ptr<KeyedEncoderQt> m_encoder { nullptr };

private slots:
    void init()
    {
        m_encoder.reset(new KeyedEncoderQt);
    }

    void cleanup()
    {
        m_encoder = nullptr;
    }

    void simpleValues()
    {
        m_encoder->encodeBool("bool", true);
        const uint8_t bytes[] = { 0, 0, 1, 0, 0 };
        m_encoder->encodeBytes("bytes", bytes, 5);
        m_encoder->encodeString("string", QStringLiteral("привет"));

        auto result = m_encoder->toMap();
        QCOMPARE(result["bool"].type(), QVariant::Bool);
        QCOMPARE(result["bool"].toBool(), true);
        QCOMPARE(result["bytes"].type(), QVariant::ByteArray);
        QCOMPARE(result["bytes"].toByteArray(), QByteArray::fromRawData("\0\0\1\0\0", 5));
        QCOMPARE(result["string"].type(), QVariant::String);
        QCOMPARE(result["string"].toString(), QStringLiteral("привет"));
    }

    void nestedObjects()
    {
        struct dummy {};

        m_encoder->encodeString("begin", "begin");
        m_encoder->encodeObject("foo", dummy(), [](KeyedEncoder& e1, dummy) {
            e1.encodeString("begin", "beginFoo");
            e1.encodeObject("bar", dummy(), [](KeyedEncoder& e2, dummy) {
                e2.encodeFloat("float", 1.5);
            });
            e1.encodeString("end", "endFoo");
        });
        m_encoder->encodeString("end", "end");

        QVariantMap expected = {
            { "begin", "begin" },
            { "foo", QVariantMap {
                { "begin", "beginFoo" },
                { "bar", QVariantMap { { "float", 1.5f } } },
                { "end", "endFoo" },
            } },
            { "end", "end" },
        };

        QCOMPARE(m_encoder->toMap(), expected);
    }

    void array()
    {
        QList<int> values = { 1, 2, 3 };

        m_encoder->encodeString("begin", "begin");
        m_encoder->encodeObjects("array", values.begin(), values.end(),
            [](KeyedEncoder& e, int v) {
            e.encodeUInt32("int", v);
            e.encodeString("string", QString::number(v));
        });
        m_encoder->encodeString("end", "end");

        QVariantMap expected = {
            { "begin", "begin" },
            { "array", QVariantList {
                QVariantMap { { "int", 1 }, { "string", "1" } },
                QVariantMap { { "int", 2 }, { "string", "2" } },
                QVariantMap { { "int", 3 }, { "string", "3" } },
            } },
            { "end", "end" },
        };

        QCOMPARE(m_encoder->toMap(), expected);
    }

    void arrayWithObjects()
    {
        QList<int> values = { 1, 2, 3 };
        struct dummy {};

        m_encoder->encodeString("begin", "begin");
        m_encoder->encodeObjects("array", values.begin(), values.end(),
            [](KeyedEncoder& e1, int v) {
            e1.encodeObject("foo", dummy(), [v](KeyedEncoder& e2, dummy) {
                e2.encodeInt32("int", v);
            });
            e1.encodeString("string", QString::number(v));
        });
        m_encoder->encodeString("end", "end");

        QVariantMap expected = {
            { "begin", "begin" },
            { "array", QVariantList {
                QVariantMap {
                    { "foo", QVariantMap { { "int", 1 } } },
                    { "string", "1" },
                },
                QVariantMap {
                    { "foo", QVariantMap { { "int", 2 } } },
                    { "string", "2" },
                },
                QVariantMap {
                    { "foo", QVariantMap { { "int", 3 } } },
                    { "string", "3" },
                },
            } },
            { "end", "end" },
        };

        QCOMPARE(m_encoder->toMap(), expected);
    }

};

QTEST_GUILESS_MAIN(tst_KeyedEncoderQt)
#include "tst_keyedencoderqt.moc"
