/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "Test.h"
#include "WTFStringUtilities.h"
#include <wtf/JSONValues.h>

namespace TestWebKitAPI {

TEST(JSONValue, Construct)
{
    {
        Ref<JSON::Value> value = JSON::Value::null();
        EXPECT_TRUE(value->type() == JSON::Value::Type::Null);
        EXPECT_TRUE(value->isNull());
    }

    {
        Ref<JSON::Value> value = JSON::Value::create(true);
        EXPECT_TRUE(value->type() == JSON::Value::Type::Boolean);
        bool booleanValue;
        EXPECT_TRUE(value->asBoolean(booleanValue));
        EXPECT_EQ(booleanValue, true);

        value = JSON::Value::create(false);
        EXPECT_TRUE(value->type() == JSON::Value::Type::Boolean);
        EXPECT_TRUE(value->asBoolean(booleanValue));
        EXPECT_EQ(booleanValue, false);
    }

    {
        Ref<JSON::Value> value = JSON::Value::create(1);
        EXPECT_TRUE(value->type() == JSON::Value::Type::Integer);
        int integerValue;
        EXPECT_TRUE(value->asInteger(integerValue));
        EXPECT_EQ(integerValue, 1);

        // Doubles can be get as integers, but not the other way around.
        float floatValue;
        EXPECT_FALSE(value->asDouble(floatValue));

        value = JSON::Value::create(std::numeric_limits<int>::max());
        EXPECT_TRUE(value->asInteger(integerValue));
        EXPECT_EQ(integerValue, std::numeric_limits<int>::max());
        unsigned unsignedValue;
        EXPECT_TRUE(value->asInteger(unsignedValue));
        long longValue;
        EXPECT_TRUE(value->asInteger(longValue));
        long long longLongValue;
        EXPECT_TRUE(value->asInteger(longLongValue));
        unsigned long unsignedLongValue;
        EXPECT_TRUE(value->asInteger(unsignedLongValue));
        unsigned long long unsignedLongLongValue;
        EXPECT_TRUE(value->asInteger(unsignedLongLongValue));
    }

    {
        Ref<JSON::Value> value = JSON::Value::create(1.5);
        EXPECT_TRUE(value->type() == JSON::Value::Type::Double);
        double doubleValue;
        EXPECT_TRUE(value->asDouble(doubleValue));
        EXPECT_EQ(doubleValue, 1.5);

        float floatValue;
        EXPECT_TRUE(value->asDouble(floatValue));

        int integerValue;
        EXPECT_TRUE(value->asInteger(integerValue));
        EXPECT_EQ(integerValue, static_cast<int>(1.5));

        unsigned unsignedValue;
        EXPECT_TRUE(value->asInteger(unsignedValue));
        long longValue;
        EXPECT_TRUE(value->asInteger(longValue));
        long long longLongValue;
        EXPECT_TRUE(value->asInteger(longLongValue));
        unsigned long unsignedLongValue;
        EXPECT_TRUE(value->asInteger(unsignedLongValue));
        unsigned long long unsignedLongLongValue;
        EXPECT_TRUE(value->asInteger(unsignedLongLongValue));
    }

    {
        Ref<JSON::Value> value = JSON::Value::create("webkit");
        EXPECT_TRUE(value->type() == JSON::Value::Type::String);
        String stringValue;
        EXPECT_TRUE(value->asString(stringValue));
        EXPECT_EQ(stringValue, "webkit");

        String nullString;
        value = JSON::Value::create(nullString);
        EXPECT_TRUE(value->asString(stringValue));
        EXPECT_TRUE(stringValue.isNull());

        value = JSON::Value::create(emptyString());
        EXPECT_TRUE(value->asString(stringValue));
        EXPECT_FALSE(stringValue.isNull());
        EXPECT_TRUE(stringValue.isEmpty());
    }

    {
        Ref<JSON::Array> array = JSON::Array::create();
        EXPECT_TRUE(array->type() == JSON::Value::Type::Array);
        EXPECT_EQ(array->length(), 0U);
    }

    {
        Ref<JSON::Object> object = JSON::Object::create();
        EXPECT_TRUE(object->type() == JSON::Value::Type::Object);
        EXPECT_EQ(object->size(), 0);
    }
}

TEST(JSONValue, ParseJSON)
{
    {
        RefPtr<JSON::Value> value;
        EXPECT_TRUE(JSON::Value::parseJSON("null", value));
        EXPECT_TRUE(value->isNull());
    }

    {
        RefPtr<JSON::Value> value;
        EXPECT_TRUE(JSON::Value::parseJSON("true", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::Boolean);
        bool booleanValue;
        EXPECT_TRUE(value->asBoolean(booleanValue));
        EXPECT_EQ(booleanValue, true);

        EXPECT_TRUE(JSON::Value::parseJSON("false", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::Boolean);
        EXPECT_TRUE(value->asBoolean(booleanValue));
        EXPECT_EQ(booleanValue, false);
    }

    {
        RefPtr<JSON::Value> value;
        EXPECT_TRUE(JSON::Value::parseJSON("1", value));
        // Numbers are always parsed as double.
        EXPECT_TRUE(value->type() == JSON::Value::Type::Double);
        double doubleValue;
        EXPECT_TRUE(value->asDouble(doubleValue));
        EXPECT_EQ(doubleValue, 1.0);

        EXPECT_TRUE(JSON::Value::parseJSON("1.5", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::Double);
        EXPECT_TRUE(value->asDouble(doubleValue));
        EXPECT_EQ(doubleValue, 1.5);
    }

    {
        RefPtr<JSON::Value> value;
        EXPECT_TRUE(JSON::Value::parseJSON("\"string\"", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::String);
        String stringValue;
        EXPECT_TRUE(value->asString(stringValue));
        EXPECT_EQ(stringValue, "string");
    }

    {
        RefPtr<JSON::Value> value;
        EXPECT_TRUE(JSON::Value::parseJSON("[]", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::Array);
        RefPtr<JSON::Array> arrayValue;
        EXPECT_TRUE(value->asArray(arrayValue));
        EXPECT_EQ(arrayValue->length(), 0U);

        EXPECT_TRUE(JSON::Value::parseJSON("[null, 1 ,2.5,[{\"foo\":\"bar\"},{\"baz\":false}],\"webkit\"]", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::Array);
        EXPECT_TRUE(value->asArray(arrayValue));
        EXPECT_EQ(arrayValue->length(), 5U);
        auto it = arrayValue->begin();
        RefPtr<JSON::Value> itemValue = it->get();
        EXPECT_TRUE(itemValue->type() == JSON::Value::Type::Null);
        ++it;
        EXPECT_FALSE(it == arrayValue->end());

        itemValue = it->get();
        EXPECT_TRUE(itemValue->type() == JSON::Value::Type::Double);
        int integerValue;
        EXPECT_TRUE(itemValue->asInteger(integerValue));
        EXPECT_EQ(integerValue, 1);
        ++it;
        EXPECT_FALSE(it == arrayValue->end());

        itemValue = it->get();
        EXPECT_TRUE(itemValue->type() == JSON::Value::Type::Double);
        double doubleValue;
        EXPECT_TRUE(itemValue->asDouble(doubleValue));
        EXPECT_EQ(doubleValue, 2.5);
        ++it;
        EXPECT_FALSE(it == arrayValue->end());

        itemValue = it->get();
        EXPECT_TRUE(itemValue->type() == JSON::Value::Type::Array);
        RefPtr<JSON::Array> subArrayValue;
        EXPECT_TRUE(itemValue->asArray(subArrayValue));
        EXPECT_EQ(subArrayValue->length(), 2U);
        RefPtr<JSON::Value> objectValue = subArrayValue->get(0);
        EXPECT_TRUE(objectValue->type() == JSON::Value::Type::Object);
        RefPtr<JSON::Object> object;
        EXPECT_TRUE(objectValue->asObject(object));
        EXPECT_EQ(object->size(), 1);
        String stringValue;
        EXPECT_TRUE(object->getString("foo", stringValue));
        EXPECT_EQ(stringValue, "bar");
        objectValue = subArrayValue->get(1);
        EXPECT_TRUE(objectValue->type() == JSON::Value::Type::Object);
        EXPECT_TRUE(objectValue->asObject(object));
        EXPECT_EQ(object->size(), 1);
        bool booleanValue;
        EXPECT_TRUE(object->getBoolean("baz", booleanValue));
        EXPECT_EQ(booleanValue, false);
        ++it;
        EXPECT_FALSE(it == arrayValue->end());

        itemValue = it->get();
        EXPECT_TRUE(itemValue->type() == JSON::Value::Type::String);
        EXPECT_TRUE(itemValue->asString(stringValue));
        EXPECT_EQ(stringValue, "webkit");
        ++it;
        EXPECT_TRUE(it == arrayValue->end());
    }

    {
        RefPtr<JSON::Value> value;
        EXPECT_TRUE(JSON::Value::parseJSON("{}", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::Object);
        RefPtr<JSON::Object> objectValue;
        EXPECT_TRUE(value->asObject(objectValue));
        EXPECT_EQ(objectValue->size(), 0);

        EXPECT_TRUE(JSON::Value::parseJSON("{\"foo\": \"bar\", \"baz\": {\"sub\":[null,false]}}", value));
        EXPECT_TRUE(value->type() == JSON::Value::Type::Object);
        EXPECT_TRUE(value->asObject(objectValue));
        EXPECT_EQ(objectValue->size(), 2);

        String stringValue;
        EXPECT_TRUE(objectValue->getString("foo", stringValue));
        EXPECT_EQ(stringValue, "bar");

        RefPtr<JSON::Object> object;
        EXPECT_TRUE(objectValue->getObject("baz", object));
        EXPECT_EQ(object->size(), 1);
        RefPtr<JSON::Array> array;
        EXPECT_TRUE(object->getArray("sub", array));
        EXPECT_EQ(array->length(), 2U);
    }

    {
        RefPtr<JSON::Value> value;
        EXPECT_FALSE(JSON::Value::parseJSON(",", value));
        EXPECT_FALSE(JSON::Value::parseJSON("\"foo", value));
        EXPECT_FALSE(JSON::Value::parseJSON("foo\"", value));
        EXPECT_FALSE(JSON::Value::parseJSON("TrUe", value));
        EXPECT_FALSE(JSON::Value::parseJSON("False", value));
        EXPECT_FALSE(JSON::Value::parseJSON("1.d", value));
        EXPECT_FALSE(JSON::Value::parseJSON("[1,]", value));
        EXPECT_FALSE(JSON::Value::parseJSON("1,2]", value));
        EXPECT_FALSE(JSON::Value::parseJSON("[1,2", value));
        EXPECT_FALSE(JSON::Value::parseJSON("[1 2]", value));
        EXPECT_FALSE(JSON::Value::parseJSON("[1,2]]", value));
        EXPECT_FALSE(JSON::Value::parseJSON("[1,2],", value));
        EXPECT_FALSE(JSON::Value::parseJSON("{foo:\"bar\"}", value));
        EXPECT_FALSE(JSON::Value::parseJSON("{\"foo\":bar}", value));
        EXPECT_FALSE(JSON::Value::parseJSON("{\"foo:\"bar\"}", value));
        EXPECT_FALSE(JSON::Value::parseJSON("{foo\":\"bar\"}", value));
        EXPECT_FALSE(JSON::Value::parseJSON("{{\"foo\":\"bar\"}}", value));
        EXPECT_FALSE(JSON::Value::parseJSON("{\"foo\":\"bar\"},", value));
        EXPECT_FALSE(JSON::Value::parseJSON("[{\"foo\":\"bar\"},{\"baz\":false},]", value));
        EXPECT_FALSE(JSON::Value::parseJSON("[{\"foo\":{\"baz\":false}]", value));
    }
}

TEST(JSONValue, MemoryCost)
{
    {
        Ref<JSON::Object> value = JSON::Object::create();
        value->setValue("test", JSON::Value::null());
        size_t memoryCost = value->memoryCost();
        EXPECT_GT(memoryCost, 0U);
        EXPECT_LE(memoryCost, 20U);
    }

    {
        Ref<JSON::Object> value = JSON::Object::create();
        size_t memoryCost = value->memoryCost();
        EXPECT_GT(memoryCost, 0U);
        EXPECT_LE(memoryCost, 8U);
    }
}

} // namespace TestWebKitAPI
