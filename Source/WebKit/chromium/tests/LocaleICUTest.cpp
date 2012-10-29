/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "LocaleICU.h"

#include <gtest/gtest.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

class LocaleICUTest : public ::testing::Test {
public:
    // Labels class is used for printing results in EXPECT_EQ macro.
    class Labels {
    public:
        Labels(const Vector<String> labels)
            : m_labels(labels)
        {
        }

        // FIXME: We should use Vector<T>::operator==() if it works.
        bool operator==(const Labels& other) const
        {
            if (m_labels.size() != other.m_labels.size())
                return false;
            for (unsigned index = 0; index < m_labels.size(); ++index)
                if (m_labels[index] != other.m_labels[index])
                    return false;
            return true;
        }

        String toString() const
        {
            StringBuilder builder;
            builder.append("labels(");
            for (unsigned index = 0; index < m_labels.size(); ++index) {
                if (index)
                    builder.append(", ");
                builder.append('"');
                builder.append(m_labels[index]);
                builder.append('"');
            }
            builder.append(')');
            return builder.toString();
        }

    private:
        Vector<String> m_labels;
    };

protected:
    Labels labels(const String& element1, const String& element2)
    {
        Vector<String> labels = Vector<String>();
        labels.append(element1);
        labels.append(element2);
        return Labels(labels);
    }

#if ENABLE(INPUT_MULTIPLE_FIELDS_UI)
    String monthFormat(const char* localeString)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return locale->monthFormat();
    }

    String localizedDateFormatText(const char* localeString)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return locale->timeFormat();
    }

    String localizedShortDateFormatText(const char* localeString)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return locale->shortTimeFormat();
    }

    String shortMonthLabel(const char* localeString, unsigned index)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return locale->shortMonthLabels()[index];
    }

    String shortStandAloneMonthLabel(const char* localeString, unsigned index)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return locale->shortStandAloneMonthLabels()[index];
    }

    String standAloneMonthLabel(const char* localeString, unsigned index)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return locale->standAloneMonthLabels()[index];
    }

    Labels timeAMPMLabels(const char* localeString)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return Labels(locale->timeAMPMLabels());
    }

    bool isRTL(const char* localeString)
    {
        OwnPtr<LocaleICU> locale = LocaleICU::create(localeString);
        return locale->isRTL();
    }
#endif
};

std::ostream& operator<<(std::ostream& os, const LocaleICUTest::Labels& labels)
{
    return os << labels.toString().utf8().data();
}

#if ENABLE(INPUT_MULTIPLE_FIELDS_UI)
TEST_F(LocaleICUTest, isRTL)
{
    EXPECT_TRUE(isRTL("ar-EG"));
    EXPECT_FALSE(isRTL("en-us"));
    EXPECT_FALSE(isRTL("ja-jp"));
    EXPECT_FALSE(isRTL("**invalid**"));
}

TEST_F(LocaleICUTest, monthFormat)
{
    EXPECT_STREQ("MMMM yyyy", monthFormat("en_US").utf8().data());
    EXPECT_STREQ("MMMM yyyy", monthFormat("fr").utf8().data());
    EXPECT_STREQ("yyyy\xE5\xB9\xB4M\xE6\x9C\x88", monthFormat("ja").utf8().data());
}

TEST_F(LocaleICUTest, localizedDateFormatText)
{
    // Note: EXPECT_EQ(String, String) doesn't print result as string.
    EXPECT_STREQ("h:mm:ss a", localizedDateFormatText("en_US").utf8().data());
    EXPECT_STREQ("HH:mm:ss", localizedDateFormatText("fr").utf8().data());
    EXPECT_STREQ("H:mm:ss", localizedDateFormatText("ja").utf8().data());
}

TEST_F(LocaleICUTest, localizedShortDateFormatText)
{
    EXPECT_STREQ("h:mm a", localizedShortDateFormatText("en_US").utf8().data());
    EXPECT_STREQ("HH:mm", localizedShortDateFormatText("fr").utf8().data());
    EXPECT_STREQ("H:mm", localizedShortDateFormatText("ja").utf8().data());
}

TEST_F(LocaleICUTest, standAloneMonthLabels)
{
    EXPECT_STREQ("January", standAloneMonthLabel("en_US", 0).utf8().data());
    EXPECT_STREQ("June", standAloneMonthLabel("en_US", 5).utf8().data());
    EXPECT_STREQ("December", standAloneMonthLabel("en_US", 11).utf8().data());

    EXPECT_STREQ("janvier", standAloneMonthLabel("fr_FR", 0).utf8().data());
    EXPECT_STREQ("juin", standAloneMonthLabel("fr_FR", 5).utf8().data());
    EXPECT_STREQ("d\xC3\xA9" "cembre", standAloneMonthLabel("fr_FR", 11).utf8().data());

    EXPECT_STREQ("1\xE6\x9C\x88", standAloneMonthLabel("ja_JP", 0).utf8().data());
    EXPECT_STREQ("6\xE6\x9C\x88", standAloneMonthLabel("ja_JP", 5).utf8().data());
    EXPECT_STREQ("12\xE6\x9C\x88", standAloneMonthLabel("ja_JP", 11).utf8().data());

    EXPECT_STREQ("\xD0\x9C\xD0\xB0\xD1\x80\xD1\x82", standAloneMonthLabel("ru_RU", 2).utf8().data());
    EXPECT_STREQ("\xD0\x9C\xD0\xB0\xD0\xB9", standAloneMonthLabel("ru_RU", 4).utf8().data());
}

TEST_F(LocaleICUTest, shortMonthLabels)
{
    EXPECT_STREQ("Jan", shortMonthLabel("en_US", 0).utf8().data());
    EXPECT_STREQ("Jan", shortStandAloneMonthLabel("en_US", 0).utf8().data());
    EXPECT_STREQ("Dec", shortMonthLabel("en_US", 11).utf8().data());
    EXPECT_STREQ("Dec", shortStandAloneMonthLabel("en_US", 11).utf8().data());

    EXPECT_STREQ("janv.", shortMonthLabel("fr_FR", 0).utf8().data());
    EXPECT_STREQ("janv.", shortStandAloneMonthLabel("fr_FR", 0).utf8().data());
    EXPECT_STREQ("d\xC3\xA9" "c.", shortMonthLabel("fr_FR", 11).utf8().data());
    EXPECT_STREQ("d\xC3\xA9" "c.", shortStandAloneMonthLabel("fr_FR", 11).utf8().data());

    EXPECT_STREQ("1\xE6\x9C\x88", shortMonthLabel("ja_JP", 0).utf8().data());
    EXPECT_STREQ("1\xE6\x9C\x88", shortStandAloneMonthLabel("ja_JP", 0).utf8().data());
    EXPECT_STREQ("12\xE6\x9C\x88", shortMonthLabel("ja_JP", 11).utf8().data());
    EXPECT_STREQ("12\xE6\x9C\x88", shortStandAloneMonthLabel("ja_JP", 11).utf8().data());

    EXPECT_STREQ("\xD0\xBC\xD0\xB0\xD1\x80\xD1\x82\xD0\xB0", shortMonthLabel("ru_RU", 2).utf8().data());
    EXPECT_STREQ("\xD0\xBC\xD0\xB0\xD1\x80\xD1\x82", shortStandAloneMonthLabel("ru_RU", 2).utf8().data());
    EXPECT_STREQ("\xD0\xBC\xD0\xB0\xD1\x8F", shortMonthLabel("ru_RU", 4).utf8().data());
    EXPECT_STREQ("\xD0\xBC\xD0\xB0\xD0\xB9", shortStandAloneMonthLabel("ru_RU", 4).utf8().data());
}

TEST_F(LocaleICUTest, timeAMPMLabels)
{
    EXPECT_EQ(labels("AM", "PM"), timeAMPMLabels("en_US"));
    EXPECT_EQ(labels("AM", "PM"), timeAMPMLabels("fr"));

    UChar jaAM[3] = { 0x5348, 0x524d, 0 };
    UChar jaPM[3] = { 0x5348, 0x5F8C, 0 };
    EXPECT_EQ(labels(String(jaAM), String(jaPM)), timeAMPMLabels("ja"));
}

static String testDecimalSeparator(const AtomicString& localeIdentifier)
{
    OwnPtr<Locale> locale = Locale::create(localeIdentifier);
    return locale->localizedDecimalSeparator();
}

TEST_F(LocaleICUTest, localizedDecimalSeparator)
{
    EXPECT_EQ(String("."), testDecimalSeparator("en_US"));
    EXPECT_EQ(String(","), testDecimalSeparator("fr"));
}
#endif

void testNumberIsReversible(const AtomicString& localeIdentifier, const char* original, const char* shouldHave = 0)
{
    OwnPtr<Locale> locale = Locale::create(localeIdentifier);
    String localized = locale->convertToLocalizedNumber(original);
    if (shouldHave)
        EXPECT_TRUE(localized.contains(shouldHave));
    String converted = locale->convertFromLocalizedNumber(localized);
    EXPECT_EQ(original, converted);
}

void testNumbers(const char* localeString)
{
    testNumberIsReversible(localeString, "123456789012345678901234567890");
    testNumberIsReversible(localeString, "-123.456");
    testNumberIsReversible(localeString, ".456");
    testNumberIsReversible(localeString, "-0.456");
}

TEST_F(LocaleICUTest, reversible)
{
    testNumberIsReversible("en_US", "123456789012345678901234567890");
    testNumberIsReversible("en_US", "-123.456", ".");
    testNumberIsReversible("en_US", ".456", ".");
    testNumberIsReversible("en_US", "-0.456", ".");

    testNumberIsReversible("fr", "123456789012345678901234567890");
    testNumberIsReversible("fr", "-123.456", ",");
    testNumberIsReversible("fr", ".456", ",");
    testNumberIsReversible("fr", "-0.456", ",");

    // Persian locale has a negative prefix and a negative suffix.
    testNumbers("fa");

    // Test some of major locales.
    testNumbers("ar");
    testNumbers("de_DE");
    testNumbers("es_ES");
    testNumbers("ja_JP");
    testNumbers("ko_KR");
    testNumbers("zh_CN");
    testNumbers("zh_HK");
    testNumbers("zh_TW");
}
