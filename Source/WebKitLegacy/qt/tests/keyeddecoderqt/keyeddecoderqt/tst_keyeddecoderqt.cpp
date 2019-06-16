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
#include "KeyedEncoderQt.h"

#include <QtTest/QtTest>

using WebCore::KeyedDecoder;
using WebCore::KeyedDecoderQt;

static QVariantMap testData()
{
    return {
        { "string", QStringLiteral("привет") },
        { "array", QVariantList {
            QVariantMap {
                { "baz", QVariantMap { { "int", 1 } } },
                { "string", "1" },
            },
            QVariantMap {
                { "baz", QVariantMap { { "int", 2 } } },
                { "string", "2" },
            },
            QVariantMap {
                { "baz", QVariantMap { { "int", 3 } } },
                { "string", "3" },
            },
        } },
        { "foo", QVariantMap {
            { "begin", "beginFoo" },
            { "bar", QVariantMap { { "float", 2.5f } } },
            { "end", "endFoo" },
        } },
        { "bool", true },
        { "float", 1.5 },
        { "bytes", QByteArray::fromRawData("\0\0\1\0\0", 5) },
        { "longlong", 1234567890123456789ll }
    };
}

static std::unique_ptr<KeyedDecoder> makeDecoder()
{
    return std::make_unique<KeyedDecoderQt>(testData());
}

class tst_KeyedDecoderQt : public QObject {
    Q_OBJECT

private slots:
    void stringValue()
    {
        WTF::String s;
        KeyedDecoderQt decoder(testData());
        QVERIFY(decoder.decodeString("string", s));
        QCOMPARE(s, WTF::String::fromUTF8("привет"));
    }

    void boolValue()
    {
        bool b;
        KeyedDecoderQt decoder(testData());
        QVERIFY(decoder.decodeBool("bool", b));
        QCOMPARE(b, true);
    }

    void floatValue()
    {
        float f;
        KeyedDecoderQt decoder(testData());
        QVERIFY(decoder.decodeFloat("float", f));
        QCOMPARE(f, 1.5f);
    }

    void bytesValue()
    {
        const uint8_t* bytes;
        size_t size;
        KeyedDecoderQt decoder(testData());
        QVERIFY(decoder.decodeBytes("bytes", bytes, size));

        const uint8_t expected[] = { 0, 0, 1, 0, 0 };
        QCOMPARE(size, (size_t)5);
        QCOMPARE(bytes[0], (uint8_t)0);
        QCOMPARE(bytes[1], (uint8_t)0);
        QCOMPARE(bytes[2], (uint8_t)1);
        QCOMPARE(bytes[3], (uint8_t)0);
        QCOMPARE(bytes[4], (uint8_t)0);
    }

    void int64Value()
    {
        int64_t i;
        KeyedDecoderQt decoder(testData());
        QVERIFY(decoder.decodeInt64("longlong", i));
        QCOMPARE(i, (int64_t)1234567890123456789ll);
    }

    void missingValue()
    {
        WTF::String s;
        KeyedDecoderQt decoder(testData());
        QVERIFY(!decoder.decodeString("foobar", s));
        QCOMPARE(s, WTF::String());
    }

    void wrongType1()
    {
        float f = 1.0;
        KeyedDecoderQt decoder(testData());
        QVERIFY(!decoder.decodeFloat("string", f));
        QCOMPARE(f, 1.0);
    }

    void wrongType2()
    {
        WTF::String s;
        KeyedDecoderQt decoder(testData());
        QVERIFY(!decoder.decodeString("array", s));
        QVERIFY(!decoder.decodeString("foo", s));
    }

    void object()
    {
        struct Foo {
            WTF::String begin;
            WTF::String end;
            float f;
        } foo;

        KeyedDecoderQt decoder(testData());
        decoder.decodeObject("foo", foo, [&](KeyedDecoder& d1, Foo& foo) -> bool {
            if (!d1.decodeString("begin", foo.begin))
                return false;
            if (!d1.decodeString("end", foo.end))
                return false;
            return d1.decodeObject("bar", foo, [&](KeyedDecoder& d2, Foo& foo) {
                return d2.decodeFloat("float", foo.f);
            });
        });
        QCOMPARE(foo.begin, WTF::String("beginFoo"));
        QCOMPARE(foo.end, WTF::String("endFoo"));
        QCOMPARE(foo.f, 2.5f);
    }

    void array()
    {
        struct Foo {
            WTF::String s;
            int i;
        };

        WTF::Vector<Foo> v;

        KeyedDecoderQt decoder(testData());
        decoder.decodeObjects("array", v, [&](KeyedDecoder& d1, Foo& foo) -> bool {
            if (!d1.decodeString("string", foo.s))
                return false;
            return d1.decodeObject("baz", foo, [&](KeyedDecoder& d2, Foo& foo) {
                return d2.decodeInt32("int", foo.i);
            });
        });
        QCOMPARE(v.size(), (size_t)3);
        QCOMPARE(v[0].s, WTF::String("1"));
        QCOMPARE(v[0].i, 1);
        QCOMPARE(v[1].s, WTF::String("2"));
        QCOMPARE(v[1].i, 2);
        QCOMPARE(v[2].s, WTF::String("3"));
        QCOMPARE(v[2].i, 3);
    }
};

QTEST_GUILESS_MAIN(tst_KeyedDecoderQt)
#include "tst_keyeddecoderqt.moc"
