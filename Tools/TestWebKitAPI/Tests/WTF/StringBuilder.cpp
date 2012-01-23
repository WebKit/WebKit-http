/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "WTFStringUtilities.h"

namespace TestWebKitAPI {

void expectBuilderContent(const char* expected, const StringBuilder& builder)
{
    // Not using builder.toString() or builder.toStringPreserveCapacity() because they all
    // change internal state of builder.
    EXPECT_EQ(String(expected), String(builder.characters(), builder.length()));
}

void expectEmpty(const StringBuilder& builder)
{
    EXPECT_EQ(0U, builder.length());
    EXPECT_TRUE(builder.isEmpty());
    EXPECT_EQ(0, builder.characters());
}

TEST(StringBuilderTest, DefaultConstructor)
{
    StringBuilder builder;
    expectEmpty(builder);
}

TEST(StringBuilderTest, Append)
{
    StringBuilder builder;
    builder.append(String("0123456789"));
    expectBuilderContent("0123456789", builder);
    builder.append("abcd");
    expectBuilderContent("0123456789abcd", builder);
    builder.append("efgh", 3);
    expectBuilderContent("0123456789abcdefg", builder);
    builder.append("");
    expectBuilderContent("0123456789abcdefg", builder);
    builder.append('#');
    expectBuilderContent("0123456789abcdefg#", builder);

    builder.toString(); // Test after reifyString().
    StringBuilder builder1;
    builder.append("", 0);
    expectBuilderContent("0123456789abcdefg#", builder);
    builder1.append(builder.characters(), builder.length());
    builder1.append("XYZ");
    builder.append(builder1.characters(), builder1.length());
    expectBuilderContent("0123456789abcdefg#0123456789abcdefg#XYZ", builder);

    StringBuilder builder2;
    builder2.reserveCapacity(100);
    builder2.append("xyz");
    const UChar* characters = builder2.characters();
    builder2.append("0123456789");
    ASSERT_EQ(characters, builder2.characters());
    builder2.toStringPreserveCapacity(); // Test after reifyString with buffer preserved.
    builder2.append("abcd");
    ASSERT_EQ(characters, builder2.characters());
}

TEST(StringBuilderTest, ToString)
{
    StringBuilder builder;
    builder.append("0123456789");
    String string = builder.toString();
    ASSERT_EQ(String("0123456789"), string);
    ASSERT_EQ(string.impl(), builder.toString().impl());

    // Changing the StringBuilder should not affect the original result of toString().
    builder.append("abcdefghijklmnopqrstuvwxyz");
    ASSERT_EQ(String("0123456789"), string);

    // Changing the StringBuilder should not affect the original result of toString() in case the capacity is not changed.
    builder.reserveCapacity(200);
    string = builder.toString();
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyz"), string);
    builder.append("ABC");
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyz"), string);

    // Changing the original result of toString() should not affect the content of the StringBuilder.
    String string1 = builder.toString();
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABC"), string1);
    string1.append("DEF");
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABC"), builder.toString());
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABCDEF"), string1);

    // Resizing the StringBuilder should not affect the original result of toString().
    string1 = builder.toString();
    builder.resize(10);
    builder.append("###");
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABC"), string1);
}

TEST(StringBuilderTest, ToStringPreserveCapacity)
{
    StringBuilder builder;
    builder.append("0123456789");
    unsigned capacity = builder.capacity();
    String string = builder.toStringPreserveCapacity();
    ASSERT_EQ(capacity, builder.capacity());
    ASSERT_EQ(String("0123456789"), string);
    ASSERT_EQ(string.impl(), builder.toStringPreserveCapacity().impl());
    ASSERT_EQ(string.characters(), builder.characters());

    // Changing the StringBuilder should not affect the original result of toStringPreserveCapacity().
    builder.append("abcdefghijklmnopqrstuvwxyz");
    ASSERT_EQ(String("0123456789"), string);

    // Changing the StringBuilder should not affect the original result of toStringPreserveCapacity() in case the capacity is not changed.
    builder.reserveCapacity(200);
    capacity = builder.capacity();
    string = builder.toStringPreserveCapacity();
    ASSERT_EQ(capacity, builder.capacity());
    ASSERT_EQ(string.characters(), builder.characters());
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyz"), string);
    builder.append("ABC");
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyz"), string);

    // Changing the original result of toStringPreserveCapacity() should not affect the content of the StringBuilder.
    capacity = builder.capacity();
    String string1 = builder.toStringPreserveCapacity();
    ASSERT_EQ(capacity, builder.capacity());
    ASSERT_EQ(string1.characters(), builder.characters());
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABC"), string1);
    string1.append("DEF");
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABC"), builder.toStringPreserveCapacity());
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABCDEF"), string1);

    // Resizing the StringBuilder should not affect the original result of toStringPreserveCapacity().
    capacity = builder.capacity();
    string1 = builder.toStringPreserveCapacity();
    ASSERT_EQ(capacity, builder.capacity());
    ASSERT_EQ(string.characters(), builder.characters());
    builder.resize(10);
    builder.append("###");
    ASSERT_EQ(String("0123456789abcdefghijklmnopqrstuvwxyzABC"), string1);
}

TEST(StringBuilderTest, Clear)
{
    StringBuilder builder;
    builder.append("0123456789");
    builder.clear();
    expectEmpty(builder);
}

TEST(StringBuilderTest, Array)
{
    StringBuilder builder;
    builder.append("0123456789");
    EXPECT_EQ('0', static_cast<char>(builder[0]));
    EXPECT_EQ('9', static_cast<char>(builder[9]));
    builder.toString(); // Test after reifyString().
    EXPECT_EQ('0', static_cast<char>(builder[0]));
    EXPECT_EQ('9', static_cast<char>(builder[9]));
}

TEST(StringBuilderTest, Resize)
{
    StringBuilder builder;
    builder.append("0123456789");
    builder.resize(10);
    EXPECT_EQ(10U, builder.length());
    expectBuilderContent("0123456789", builder);
    builder.resize(8);
    EXPECT_EQ(8U, builder.length());
    expectBuilderContent("01234567", builder);

    builder.toString();
    builder.resize(7);
    EXPECT_EQ(7U, builder.length());
    expectBuilderContent("0123456", builder);
    builder.resize(0);
    expectEmpty(builder);
}

TEST(StringBuilderTest, Equal)
{
    StringBuilder builder1;
    StringBuilder builder2;
    ASSERT_TRUE(builder1 == builder2);
    ASSERT_TRUE(equal(builder1, static_cast<LChar*>(0), 0));
    ASSERT_TRUE(builder1 == String());
    ASSERT_TRUE(String() == builder1);
    ASSERT_TRUE(builder1 != String("abc"));

    builder1.append("123");
    builder1.reserveCapacity(32);
    builder2.append("123");
    builder1.reserveCapacity(64);
    ASSERT_TRUE(builder1 == builder2);
    ASSERT_TRUE(builder1 == String("123"));
    ASSERT_TRUE(String("123") == builder1);

    builder2.append("456");
    ASSERT_TRUE(builder1 != builder2);
    ASSERT_TRUE(builder2 != builder1);
    ASSERT_TRUE(String("123") != builder2);
    ASSERT_TRUE(builder2 != String("123"));
    builder2.toString(); // Test after reifyString().
    ASSERT_TRUE(builder1 != builder2);

    builder2.resize(3);
    ASSERT_TRUE(builder1 == builder2);

    builder1.toString(); // Test after reifyString().
    ASSERT_TRUE(builder1 == builder2);
}

TEST(StringBuilderTest, CanShrink)
{
    StringBuilder builder;
    builder.reserveCapacity(256);
    ASSERT_TRUE(builder.canShrink());
    for (int i = 0; i < 256; i++)
        builder.append('x');
    ASSERT_EQ(builder.length(), builder.capacity());
    ASSERT_FALSE(builder.canShrink());
}

TEST(StringBuilderTest, ToAtomicString)
{
    StringBuilder builder;
    builder.append("123");
    AtomicString atomicString = builder.toAtomicString();
    ASSERT_EQ(String("123"), atomicString);

    builder.reserveCapacity(256);
    ASSERT_TRUE(builder.canShrink());
    for (int i = builder.length(); i < 128; i++)
        builder.append('x');
    AtomicString atomicString1 = builder.toAtomicString();
    ASSERT_EQ(128u, atomicString1.length());
    ASSERT_EQ('x', atomicString1[127]);

    // Later change of builder should not affect the atomic string.
    for (int i = builder.length(); i < 256; i++)
        builder.append('x');
    ASSERT_EQ(128u, atomicString1.length());

    ASSERT_FALSE(builder.canShrink());
    String string = builder.toString();
    AtomicString atomicString2 = builder.toAtomicString();
    // They should share the same StringImpl.
    ASSERT_EQ(atomicString2.impl(), string.impl());
}

} // namespace
