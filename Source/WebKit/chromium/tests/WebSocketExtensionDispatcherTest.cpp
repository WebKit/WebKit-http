/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WebSocketExtensionDispatcher.h"

#include "WebSocketExtensionParser.h"
#include "WebSocketExtensionProcessor.h"

#include <gtest/gtest.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

using namespace WebCore;

namespace {

class WebSocketExtensionDispatcherTest;

class MockWebSocketExtensionProcessor : public WebSocketExtensionProcessor {
public:
    MockWebSocketExtensionProcessor(const String& name, WebSocketExtensionDispatcherTest* test)
        : WebSocketExtensionProcessor(name)
        , m_test(test)
    {
    }
    virtual String handshakeString() OVERRIDE { return extensionToken(); }
    virtual bool processResponse(const HashMap<String, String>&) OVERRIDE;

private:
    WebSocketExtensionDispatcherTest* m_test;
};

class WebSocketExtensionDispatcherTest : public testing::Test {
public:
    WebSocketExtensionDispatcherTest() { }

    void SetUp() { }

    void TearDown() { }

    void addMockProcessor(const String& extensionToken)
    {
        m_extensions.addProcessor(adoptPtr(new MockWebSocketExtensionProcessor(extensionToken, this)));

    }

    void appendResult(const String& extensionToken, const HashMap<String, String>& parameters)
    {
        m_parsedExtensionTokens.append(extensionToken);
        m_parsedParameters.append(parameters);
    }

protected:
    WebSocketExtensionDispatcher m_extensions;
    Vector<String> m_parsedExtensionTokens;
    Vector<HashMap<String, String> > m_parsedParameters;
};

bool MockWebSocketExtensionProcessor::processResponse(const HashMap<String, String>& parameters)
{
    m_test->appendResult(extensionToken(), parameters);
    return true;
}

TEST_F(WebSocketExtensionDispatcherTest, TestSingle)
{
    addMockProcessor("deflate-frame");
    EXPECT_TRUE(m_extensions.processHeaderValue("deflate-frame"));
    EXPECT_EQ(1UL, m_parsedExtensionTokens.size());
    EXPECT_EQ("deflate-frame", m_parsedExtensionTokens[0]);
    EXPECT_EQ("deflate-frame", m_extensions.acceptedExtensions());
    EXPECT_EQ(0, m_parsedParameters[0].size());
}

TEST_F(WebSocketExtensionDispatcherTest, TestParameters)
{
    addMockProcessor("mux");
    EXPECT_TRUE(m_extensions.processHeaderValue("mux; max-channels=4; flow-control  "));
    EXPECT_EQ(1UL, m_parsedExtensionTokens.size());
    EXPECT_EQ("mux", m_parsedExtensionTokens[0]);
    EXPECT_EQ(2, m_parsedParameters[0].size());
    HashMap<String, String>::iterator parameter = m_parsedParameters[0].find("max-channels");
    EXPECT_TRUE(parameter != m_parsedParameters[0].end());
    EXPECT_EQ("4", parameter->value);
    parameter = m_parsedParameters[0].find("flow-control");
    EXPECT_TRUE(parameter != m_parsedParameters[0].end());
    EXPECT_TRUE(parameter->value.isNull());
}

TEST_F(WebSocketExtensionDispatcherTest, TestMultiple)
{
    struct {
        String token;
        HashMap<String, String> parameters;
    } expected[2];
    expected[0].token = "mux";
    expected[0].parameters.add("max-channels", "4");
    expected[0].parameters.add("flow-control", String());
    expected[1].token = "deflate-frame";

    addMockProcessor("mux");
    addMockProcessor("deflate-frame");
    EXPECT_TRUE(m_extensions.processHeaderValue("mux ;  max-channels =4;flow-control, deflate-frame  "));
    EXPECT_TRUE(m_extensions.acceptedExtensions().find("mux") != notFound);
    EXPECT_TRUE(m_extensions.acceptedExtensions().find("deflate-frame") != notFound);
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        EXPECT_EQ(expected[i].token, m_parsedExtensionTokens[i]);
        const HashMap<String, String>& expectedParameters = expected[i].parameters;
        const HashMap<String, String>& parsedParameters = m_parsedParameters[i];
        EXPECT_EQ(expected[i].parameters.size(), m_parsedParameters[i].size());
        for (HashMap<String, String>::const_iterator iterator = expectedParameters.begin(); iterator != expectedParameters.end(); ++iterator) {
            HashMap<String, String>::const_iterator parsed = parsedParameters.find(iterator->key);
            EXPECT_TRUE(parsed != parsedParameters.end());
            if (iterator->value.isNull())
                EXPECT_TRUE(parsed->value.isNull());
            else
                EXPECT_EQ(iterator->value, parsed->value);
        }
    }
}

TEST_F(WebSocketExtensionDispatcherTest, TestQuotedString)
{
    addMockProcessor("x-foo");
    EXPECT_TRUE(m_extensions.processHeaderValue("x-foo; param1=\"quoted string\"; param2=\"\\\"quoted\\\" string\\\\\""));
    EXPECT_EQ(2, m_parsedParameters[0].size());
    EXPECT_EQ("quoted string", m_parsedParameters[0].get("param1"));
    EXPECT_EQ("\"quoted\" string\\", m_parsedParameters[0].get("param2"));
}

TEST_F(WebSocketExtensionDispatcherTest, TestInvalid)
{
    const char* inputs[] = {
        "\"x-foo\"",
        "x-baz",
        "x-foo\\",
        "x-(foo)",
        "x-foo; ",
        "x-foo; bar=",
        "x-foo; bar=x y",
        "x-foo; bar=\"mismatch quote",
        "x-foo; bar=\"\\\"",
        "x-foo; \"bar\"=baz",
        "x-foo x-bar",
        "x-foo, x-baz"
        "x-foo, ",
    };
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        m_extensions.reset();
        addMockProcessor("x-foo");
        addMockProcessor("x-bar");
        EXPECT_FALSE(m_extensions.processHeaderValue(inputs[i]));
        EXPECT_TRUE(m_extensions.acceptedExtensions().isNull());
    }
}

// Tests for the most complex example at http://tools.ietf.org/html/draft-ietf-hybi-permessage-compression-01#section-3.1
TEST_F(WebSocketExtensionDispatcherTest, TestPerMessageCompressExample)
{
    addMockProcessor("permessage-compress");
    addMockProcessor("bar");
    EXPECT_TRUE(m_extensions.processHeaderValue("permessage-compress; method=\"foo; x=\\\"Hello World\\\", bar\""));
    EXPECT_EQ(1U, m_parsedExtensionTokens.size());
    EXPECT_EQ("permessage-compress", m_parsedExtensionTokens[0]);
    String methodParameter = m_parsedParameters[0].find("method")->value;
    EXPECT_EQ("foo; x=\"Hello World\", bar", methodParameter);

    CString methodValue = methodParameter.ascii();
    WebSocketExtensionParser parser(methodValue.data(), methodValue.data() + methodValue.length());

    String token1;
    HashMap<String, String> parameters1;
    EXPECT_TRUE(parser.parseExtension(token1, parameters1));
    EXPECT_EQ("foo", token1);
    EXPECT_EQ(1, parameters1.size());
    HashMap<String, String>::iterator xparameter = parameters1.find("x");
    EXPECT_EQ("x", xparameter->key);
    EXPECT_EQ("Hello World", xparameter->value);

    String token2;
    HashMap<String, String> parameters2;
    EXPECT_TRUE(parser.parseExtension(token2, parameters2));
    EXPECT_EQ("bar", token2);
    EXPECT_EQ(0, parameters2.size());
}

}
