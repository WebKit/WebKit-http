/*
 * Copyright (C) 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XPathFunctions.h"

#include "Element.h"
#include "ProcessingInstruction.h"
#include "TreeScope.h"
#include "XMLNames.h"
#include "XPathUtil.h"
#include "XPathValue.h"
#include <wtf/MathExtras.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {
namespace XPath {

static inline bool isWhitespace(UChar c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

#define DEFINE_FUNCTION_CREATOR(Suffix) static std::unique_ptr<Function> createFunction##Suffix() { return std::make_unique<Fun##Suffix>(); }

class Interval {
public:
    static const int Inf = -1;

    Interval();
    Interval(int value);
    Interval(int min, int max);

    bool contains(int value) const;

private:
    int m_min;
    int m_max;
};

class FunLast FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
public:
    FunLast() { setIsContextSizeSensitive(true); }
};

class FunPosition FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
public:
    FunPosition() { setIsContextPositionSensitive(true); }
};

class FunCount FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
};

class FunId FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NodeSetValue; }
};

class FunLocalName FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
public:
    FunLocalName() { setIsContextNodeSensitive(true); } // local-name() with no arguments uses context node. 
};

class FunNamespaceURI FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
public:
    FunNamespaceURI() { setIsContextNodeSensitive(true); } // namespace-uri() with no arguments uses context node. 
};

class FunName FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
public:
    FunName() { setIsContextNodeSensitive(true); } // name() with no arguments uses context node. 
};

class FunString FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
public:
    FunString() { setIsContextNodeSensitive(true); } // string() with no arguments uses context node. 
};

class FunConcat FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
};

class FunStartsWith FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
};

class FunContains FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
};

class FunSubstringBefore FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
};

class FunSubstringAfter FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
};

class FunSubstring FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
};

class FunStringLength FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
public:
    FunStringLength() { setIsContextNodeSensitive(true); } // string-length() with no arguments uses context node. 
};

class FunNormalizeSpace FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
public:
    FunNormalizeSpace() { setIsContextNodeSensitive(true); } // normalize-space() with no arguments uses context node. 
};

class FunTranslate FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }
};

class FunBoolean FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
};

class FunNot : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
};

class FunTrue FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
};

class FunFalse FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
};

class FunLang FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
public:
    FunLang() { setIsContextNodeSensitive(true); } // lang() always works on context node. 
};

class FunNumber FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
public:
    FunNumber() { setIsContextNodeSensitive(true); } // number() with no arguments uses context node. 
};

class FunSum FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
};

class FunFloor FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
};

class FunCeiling FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
};

class FunRound FINAL : public Function {
    virtual Value evaluate() const OVERRIDE;
    virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
public:
    static double round(double);
};

DEFINE_FUNCTION_CREATOR(Last)
DEFINE_FUNCTION_CREATOR(Position)
DEFINE_FUNCTION_CREATOR(Count)
DEFINE_FUNCTION_CREATOR(Id)
DEFINE_FUNCTION_CREATOR(LocalName)
DEFINE_FUNCTION_CREATOR(NamespaceURI)
DEFINE_FUNCTION_CREATOR(Name)

DEFINE_FUNCTION_CREATOR(String)
DEFINE_FUNCTION_CREATOR(Concat)
DEFINE_FUNCTION_CREATOR(StartsWith)
DEFINE_FUNCTION_CREATOR(Contains)
DEFINE_FUNCTION_CREATOR(SubstringBefore)
DEFINE_FUNCTION_CREATOR(SubstringAfter)
DEFINE_FUNCTION_CREATOR(Substring)
DEFINE_FUNCTION_CREATOR(StringLength)
DEFINE_FUNCTION_CREATOR(NormalizeSpace)
DEFINE_FUNCTION_CREATOR(Translate)

DEFINE_FUNCTION_CREATOR(Boolean)
DEFINE_FUNCTION_CREATOR(Not)
DEFINE_FUNCTION_CREATOR(True)
DEFINE_FUNCTION_CREATOR(False)
DEFINE_FUNCTION_CREATOR(Lang)

DEFINE_FUNCTION_CREATOR(Number)
DEFINE_FUNCTION_CREATOR(Sum)
DEFINE_FUNCTION_CREATOR(Floor)
DEFINE_FUNCTION_CREATOR(Ceiling)
DEFINE_FUNCTION_CREATOR(Round)

#undef DEFINE_FUNCTION_CREATOR

inline Interval::Interval()
    : m_min(Inf), m_max(Inf)
{
}

inline Interval::Interval(int value)
    : m_min(value), m_max(value)
{
}

inline Interval::Interval(int min, int max)
    : m_min(min), m_max(max)
{
}

inline bool Interval::contains(int value) const
{
    if (m_min == Inf && m_max == Inf)
        return true;

    if (m_min == Inf)
        return value <= m_max;

    if (m_max == Inf)
        return value >= m_min;

    return value >= m_min && value <= m_max;
}

void Function::setArguments(const String& name, Vector<std::unique_ptr<Expression>> arguments)
{
    ASSERT(!subexpressionCount());

    // Functions that use the context node as an implicit argument are context node sensitive when they
    // have no arguments, but when explicit arguments are added, they are no longer context node sensitive.
    // As of this writing, the only exception to this is the "lang" function.
    if (name != "lang" && !arguments.isEmpty())
        setIsContextNodeSensitive(false);

    setSubexpressions(std::move(arguments));
}

Value FunLast::evaluate() const
{
    return Expression::evaluationContext().size;
}

Value FunPosition::evaluate() const
{
    return Expression::evaluationContext().position;
}

Value FunId::evaluate() const
{
    Value a = argument(0).evaluate();
    StringBuilder idList; // A whitespace-separated list of IDs

    if (a.isNodeSet()) {
        const NodeSet& nodes = a.toNodeSet();
        for (size_t i = 0; i < nodes.size(); ++i) {
            String str = stringValue(nodes[i]);
            idList.append(str);
            idList.append(' ');
        }
    } else {
        String str = a.toString();
        idList.append(str);
    }
    
    TreeScope& contextScope = evaluationContext().node->treeScope();
    NodeSet result;
    HashSet<Node*> resultSet;

    unsigned startPos = 0;
    unsigned length = idList.length();
    while (true) {
        while (startPos < length && isWhitespace(idList[startPos]))
            ++startPos;
        
        if (startPos == length)
            break;

        size_t endPos = startPos;
        while (endPos < length && !isWhitespace(idList[endPos]))
            ++endPos;

        // If there are several nodes with the same id, id() should return the first one.
        // In WebKit, getElementById behaves so, too, although its behavior in this case is formally undefined.
        Node* node = contextScope.getElementById(String(idList.characters() + startPos, endPos - startPos));
        if (node && resultSet.add(node).isNewEntry)
            result.append(node);
        
        startPos = endPos;
    }
    
    result.markSorted(false);
    
    return Value(std::move(result));
}

static inline String expandedNameLocalPart(Node* node)
{
    // The local part of an XPath expanded-name matches DOM local name for most node types, except for namespace nodes and processing instruction nodes.
    ASSERT(node->nodeType() != Node::XPATH_NAMESPACE_NODE); // Not supported yet.
    if (node->nodeType() == Node::PROCESSING_INSTRUCTION_NODE)
        return toProcessingInstruction(node)->target();
    return node->localName().string();
}

static inline String expandedName(Node* node)
{
    const AtomicString& prefix = node->prefix();
    return prefix.isEmpty() ? expandedNameLocalPart(node) : prefix + ":" + expandedNameLocalPart(node);
}

Value FunLocalName::evaluate() const
{
    if (argumentCount() > 0) {
        Value a = argument(0).evaluate();
        if (!a.isNodeSet())
            return emptyString();

        Node* node = a.toNodeSet().firstNode();
        return node ? expandedNameLocalPart(node) : emptyString();
    }

    return expandedNameLocalPart(evaluationContext().node.get());
}

Value FunNamespaceURI::evaluate() const
{
    if (argumentCount() > 0) {
        Value a = argument(0).evaluate();
        if (!a.isNodeSet())
            return emptyString();

        Node* node = a.toNodeSet().firstNode();
        return node ? node->namespaceURI().string() : emptyString();
    }

    return evaluationContext().node->namespaceURI().string();
}

Value FunName::evaluate() const
{
    if (argumentCount() > 0) {
        Value a = argument(0).evaluate();
        if (!a.isNodeSet())
            return emptyString();

        Node* node = a.toNodeSet().firstNode();
        return node ? expandedName(node) : emptyString();
    }

    return expandedName(evaluationContext().node.get());
}

Value FunCount::evaluate() const
{
    Value a = argument(0).evaluate();
    
    return double(a.toNodeSet().size());
}

Value FunString::evaluate() const
{
    if (!argumentCount())
        return Value(Expression::evaluationContext().node.get()).toString();
    return argument(0).evaluate().toString();
}

Value FunConcat::evaluate() const
{
    StringBuilder result;
    result.reserveCapacity(1024);

    for (unsigned i = 0, count = argumentCount(); i < count; ++i) {
        String str(argument(i).evaluate().toString());
        result.append(str);
    }

    return result.toString();
}

Value FunStartsWith::evaluate() const
{
    String s1 = argument(0).evaluate().toString();
    String s2 = argument(1).evaluate().toString();

    if (s2.isEmpty())
        return true;

    return s1.startsWith(s2);
}

Value FunContains::evaluate() const
{
    String s1 = argument(0).evaluate().toString();
    String s2 = argument(1).evaluate().toString();

    if (s2.isEmpty()) 
        return true;

    return s1.contains(s2) != 0;
}

Value FunSubstringBefore::evaluate() const
{
    String s1 = argument(0).evaluate().toString();
    String s2 = argument(1).evaluate().toString();

    if (s2.isEmpty())
        return emptyString();

    size_t i = s1.find(s2);

    if (i == notFound)
        return emptyString();

    return s1.left(i);
}

Value FunSubstringAfter::evaluate() const
{
    String s1 = argument(0).evaluate().toString();
    String s2 = argument(1).evaluate().toString();

    size_t i = s1.find(s2);
    if (i == notFound)
        return emptyString();

    return s1.substring(i + s2.length());
}

Value FunSubstring::evaluate() const
{
    String s = argument(0).evaluate().toString();
    double doublePos = argument(1).evaluate().toNumber();
    if (std::isnan(doublePos))
        return emptyString();
    long pos = static_cast<long>(FunRound::round(doublePos));
    bool haveLength = argumentCount() == 3;
    long len = -1;
    if (haveLength) {
        double doubleLen = argument(2).evaluate().toNumber();
        if (std::isnan(doubleLen))
            return emptyString();
        len = static_cast<long>(FunRound::round(doubleLen));
    }

    if (pos > long(s.length())) 
        return emptyString();

    if (pos < 1) {
        if (haveLength) {
            len -= 1 - pos;
            if (len < 1)
                return emptyString();
        }
        pos = 1;
    }

    return s.substring(pos - 1, len);
}

Value FunStringLength::evaluate() const
{
    if (!argumentCount())
        return Value(Expression::evaluationContext().node.get()).toString().length();
    return argument(0).evaluate().toString().length();
}

Value FunNormalizeSpace::evaluate() const
{
    if (!argumentCount()) {
        String s = Value(Expression::evaluationContext().node.get()).toString();
        return s.simplifyWhiteSpace();
    }

    String s = argument(0).evaluate().toString();
    return s.simplifyWhiteSpace();
}

Value FunTranslate::evaluate() const
{
    String s1 = argument(0).evaluate().toString();
    String s2 = argument(1).evaluate().toString();
    String s3 = argument(2).evaluate().toString();
    StringBuilder result;

    for (unsigned i1 = 0; i1 < s1.length(); ++i1) {
        UChar ch = s1[i1];
        size_t i2 = s2.find(ch);
        
        if (i2 == notFound)
            result.append(ch);
        else if (i2 < s3.length())
            result.append(s3[i2]);
    }

    return result.toString();
}

Value FunBoolean::evaluate() const
{
    return argument(0).evaluate().toBoolean();
}

Value FunNot::evaluate() const
{
    return !argument(0).evaluate().toBoolean();
}

Value FunTrue::evaluate() const
{
    return true;
}

Value FunLang::evaluate() const
{
    String lang = argument(0).evaluate().toString();

    const Attribute* languageAttribute = 0;
    Node* node = evaluationContext().node.get();
    while (node) {
        if (node->isElementNode()) {
            Element* element = toElement(node);
            if (element->hasAttributes())
                languageAttribute = element->findAttributeByName(XMLNames::langAttr);
        }
        if (languageAttribute)
            break;
        node = node->parentNode();
    }

    if (!languageAttribute)
        return false;

    String langValue = languageAttribute->value();
    while (true) {
        if (equalIgnoringCase(langValue, lang))
            return true;

        // Remove suffixes one by one.
        size_t index = langValue.reverseFind('-');
        if (index == notFound)
            break;
        langValue = langValue.left(index);
    }

    return false;
}

Value FunFalse::evaluate() const
{
    return false;
}

Value FunNumber::evaluate() const
{
    if (!argumentCount())
        return Value(Expression::evaluationContext().node.get()).toNumber();
    return argument(0).evaluate().toNumber();
}

Value FunSum::evaluate() const
{
    Value a = argument(0).evaluate();
    if (!a.isNodeSet())
        return 0.0;

    double sum = 0.0;
    const NodeSet& nodes = a.toNodeSet();
    // To be really compliant, we should sort the node-set, as floating point addition is not associative.
    // However, this is unlikely to ever become a practical issue, and sorting is slow.

    for (unsigned i = 0; i < nodes.size(); i++)
        sum += Value(stringValue(nodes[i])).toNumber();
    
    return sum;
}

Value FunFloor::evaluate() const
{
    return floor(argument(0).evaluate().toNumber());
}

Value FunCeiling::evaluate() const
{
    return ceil(argument(0).evaluate().toNumber());
}

double FunRound::round(double val)
{
    if (!std::isnan(val) && !std::isinf(val)) {
        if (std::signbit(val) && val >= -0.5)
            val *= 0; // negative zero
        else
            val = floor(val + 0.5);
    }
    return val;
}

Value FunRound::evaluate() const
{
    return round(argument(0).evaluate().toNumber());
}

struct FunctionMapValue {
    std::unique_ptr<Function> (*creationFunction)();
    Interval argumentCountInterval;
};

static void populateFunctionMap(HashMap<String, FunctionMapValue>& functionMap)
{
    struct FunctionMapping {
        const char* name;
        FunctionMapValue function;
    };

    static const FunctionMapping functions[] = {
        { "boolean", { createFunctionBoolean, 1 } },
        { "ceiling", { createFunctionCeiling, 1 } },
        { "concat", { createFunctionConcat, Interval(2, Interval::Inf) } },
        { "contains", { createFunctionContains, 2 } },
        { "count", { createFunctionCount, 1 } },
        { "false", { createFunctionFalse, 0 } },
        { "floor", { createFunctionFloor, 1 } },
        { "id", { createFunctionId, 1 } },
        { "lang", { createFunctionLang, 1 } },
        { "last", { createFunctionLast, 0 } },
        { "local-name", { createFunctionLocalName, Interval(0, 1) } },
        { "name", { createFunctionName, Interval(0, 1) } },
        { "namespace-uri", { createFunctionNamespaceURI, Interval(0, 1) } },
        { "normalize-space", { createFunctionNormalizeSpace, Interval(0, 1) } },
        { "not", { createFunctionNot, 1 } },
        { "number", { createFunctionNumber, Interval(0, 1) } },
        { "position", { createFunctionPosition, 0 } },
        { "round", { createFunctionRound, 1 } },
        { "starts-with", { createFunctionStartsWith, 2 } },
        { "string", { createFunctionString, Interval(0, 1) } },
        { "string-length", { createFunctionStringLength, Interval(0, 1) } },
        { "substring", { createFunctionSubstring, Interval(2, 3) } },
        { "substring-after", { createFunctionSubstringAfter, 2 } },
        { "substring-before", { createFunctionSubstringBefore, 2 } },
        { "sum", { createFunctionSum, 1 } },
        { "translate", { createFunctionTranslate, 3 } },
        { "true", { createFunctionTrue, 0 } },
    };

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(functions); ++i)
        functionMap.add(functions[i].name, functions[i].function);
}

std::unique_ptr<Function> Function::create(const String& name, unsigned numArguments)
{
    static NeverDestroyed<HashMap<String, FunctionMapValue>> functionMap;
    if (functionMap.get().isEmpty())
        populateFunctionMap(functionMap);

    auto it = functionMap.get().find(name);
    if (it == functionMap.get().end())
        return nullptr;

    if (!it->value.argumentCountInterval.contains(numArguments))
        return nullptr;

    return it->value.creationFunction();
}

std::unique_ptr<Function> Function::create(const String& name)
{
    return create(name, 0);
}

std::unique_ptr<Function> Function::create(const String& name, Vector<std::unique_ptr<Expression>> arguments)
{
    std::unique_ptr<Function> function = create(name, arguments.size());
    if (function)
        function->setArguments(name, std::move(arguments));
    return function;
}

}
}
