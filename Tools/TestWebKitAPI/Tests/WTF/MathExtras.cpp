/*
 * Copyright (C) 2012 Intel Corporation
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

#include <wtf/MathExtras.h>

namespace TestWebKitAPI {

TEST(WTF, Lrint)
{
    EXPECT_EQ(lrint(-7.5), -8);
    EXPECT_EQ(lrint(-8.5), -8);
    EXPECT_EQ(lrint(-0.5), 0);
    EXPECT_EQ(lrint(0.5), 0);
    EXPECT_EQ(lrint(-0.5), 0);
    EXPECT_EQ(lrint(1.3), 1);
    EXPECT_EQ(lrint(1.7), 2);
    EXPECT_EQ(lrint(0), 0);
    EXPECT_EQ(lrint(-0), 0);
    if (sizeof(long int) == 8) {
        // Largest double number with 0.5 precision and one halfway rounding case below.
        EXPECT_EQ(lrint(pow(2.0, 52) - 0.5), pow(2.0, 52));
        EXPECT_EQ(lrint(pow(2.0, 52) - 1.5), pow(2.0, 52) - 2);
        // Smallest double number with 0.5 precision and one halfway rounding case above.
        EXPECT_EQ(lrint(-pow(2.0, 52) + 0.5), -pow(2.0, 52));
        EXPECT_EQ(lrint(-pow(2.0, 52) + 1.5), -pow(2.0, 52) + 2);
    }
}

TEST(WTF, clampToIntLong)
{
    if (sizeof(long) == sizeof(int))
        return;

    long maxInt = std::numeric_limits<int>::max();
    long minInt = std::numeric_limits<int>::min();
    long overflowInt = maxInt + 1;
    long underflowInt = minInt - 1;

    EXPECT_GT(overflowInt, maxInt);
    EXPECT_LT(underflowInt, minInt);

    EXPECT_EQ(clampTo<int>(maxInt), maxInt);
    EXPECT_EQ(clampTo<int>(minInt), minInt);

    EXPECT_EQ(clampTo<int>(overflowInt), maxInt);
    EXPECT_EQ(clampTo<int>(underflowInt), minInt);
}

TEST(WTF, clampToIntLongLong)
{
    long long maxInt = std::numeric_limits<int>::max();
    long long minInt = std::numeric_limits<int>::min();
    long long overflowInt = maxInt + 1;
    long long underflowInt = minInt - 1;

    EXPECT_GT(overflowInt, maxInt);
    EXPECT_LT(underflowInt, minInt);

    EXPECT_EQ(clampTo<int>(maxInt), maxInt);
    EXPECT_EQ(clampTo<int>(minInt), minInt);

    EXPECT_EQ(clampTo<int>(overflowInt), maxInt);
    EXPECT_EQ(clampTo<int>(underflowInt), minInt);
}

TEST(WTF, clampToIntegerFloat)
{
    // This test is inaccurate as floats will round the min / max integer
    // due to the narrow mantissa. However it will properly checks within
    // (close to the extreme) and outside the integer range.
    float maxInt = std::numeric_limits<int>::max();
    float minInt = std::numeric_limits<int>::min();
    float overflowInt = maxInt * 1.1;
    float underflowInt = minInt * 1.1;

    EXPECT_GT(overflowInt, maxInt);
    EXPECT_LT(underflowInt, minInt);

    // If maxInt == 2^31 - 1 (ie on I32 architecture), the closest float used to represent it is 2^31.
    EXPECT_NEAR(clampToInteger(maxInt), maxInt, 1);
    EXPECT_EQ(clampToInteger(minInt), minInt);

    EXPECT_NEAR(clampToInteger(overflowInt), maxInt, 1);
    EXPECT_EQ(clampToInteger(underflowInt), minInt);
}

TEST(WTF, clampToIntegerDouble)
{
    double maxInt = std::numeric_limits<int>::max();
    double minInt = std::numeric_limits<int>::min();
    double overflowInt = maxInt + 1;
    double underflowInt = minInt - 1;

    EXPECT_GT(overflowInt, maxInt);
    EXPECT_LT(underflowInt, minInt);

    EXPECT_EQ(clampToInteger(maxInt), maxInt);
    EXPECT_EQ(clampToInteger(minInt), minInt);

    EXPECT_EQ(clampToInteger(overflowInt), maxInt);
    EXPECT_EQ(clampToInteger(underflowInt), minInt);
}

TEST(WTF, clampToFloat)
{
    double maxFloat = std::numeric_limits<float>::max();
    double minFloat = -maxFloat;
    double overflowFloat = maxFloat * 1.1;
    double underflowFloat = minFloat * 1.1;

    EXPECT_GT(overflowFloat, maxFloat);
    EXPECT_LT(underflowFloat, minFloat);

    EXPECT_EQ(clampToFloat(maxFloat), maxFloat);
    EXPECT_EQ(clampToFloat(minFloat), minFloat);

    EXPECT_EQ(clampToFloat(overflowFloat), maxFloat);
    EXPECT_EQ(clampToFloat(underflowFloat), minFloat);

    EXPECT_EQ(clampToFloat(std::numeric_limits<float>::infinity()), maxFloat);
    EXPECT_EQ(clampToFloat(-std::numeric_limits<float>::infinity()), minFloat);
}

TEST(WTF, clampToUnsignedLong)
{
    if (sizeof(unsigned long) == sizeof(unsigned))
        return;

    unsigned long maxUnsigned = std::numeric_limits<unsigned>::max();
    unsigned long overflowUnsigned = maxUnsigned + 1;

    EXPECT_GT(overflowUnsigned, maxUnsigned);

    EXPECT_EQ(clampTo<unsigned>(maxUnsigned), maxUnsigned);

    EXPECT_EQ(clampTo<unsigned>(overflowUnsigned), maxUnsigned);
    EXPECT_EQ(clampTo<unsigned>(-1), 0u);
}

TEST(WTF, clampToUnsignedLongLong)
{
    unsigned long long maxUnsigned = std::numeric_limits<unsigned>::max();
    unsigned long long overflowUnsigned = maxUnsigned + 1;

    EXPECT_GT(overflowUnsigned, maxUnsigned);

    EXPECT_EQ(clampTo<unsigned>(maxUnsigned), maxUnsigned);

    EXPECT_EQ(clampTo<unsigned>(overflowUnsigned), maxUnsigned);
    EXPECT_EQ(clampTo<unsigned>(-1), 0u);
}

} // namespace TestWebKitAPI
