/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
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
#include "MarkupAccumulator.h"

#include "CDATASection.h"
#include "Comment.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Editor.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "HTMLTemplateElement.h"
#include "URL.h"
#include "ProcessingInstruction.h"
#include "XLinkNames.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

using namespace HTMLNames;

struct EntityDescription {
    const char* characters;
    unsigned char length;
    unsigned char mask;
};

static const EntityDescription entitySubstitutionList[] = {
    { "", 0 , 0 },
    { "&amp;", 5 , EntityAmp },
    { "&lt;", 4, EntityLt },
    { "&gt;", 4, EntityGt },
    { "&quot;", 6, EntityQuot },
    { "&nbsp;", 6, EntityNbsp },
};

enum EntitySubstitutionIndex {
    EntitySubstitutionNullIndex = 0,
    EntitySubstitutionAmpIndex = 1,
    EntitySubstitutionLtIndex = 2,
    EntitySubstitutionGtIndex = 3,
    EntitySubstitutionQuotIndex = 4,
    EntitySubstitutionNbspIndex = 5,
};

static const unsigned maximumEscapedentityCharacter = noBreakSpace;
static const uint8_t entityMap[maximumEscapedentityCharacter + 1] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    EntitySubstitutionQuotIndex, // '"'.
    0, 0, 0,
    EntitySubstitutionAmpIndex, // '&'.
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    EntitySubstitutionLtIndex, // '<'.
    0,
    EntitySubstitutionGtIndex, // '>'.
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    EntitySubstitutionNbspIndex // noBreakSpace.
};

template<typename CharacterType>
static inline void appendCharactersReplacingEntitiesInternal(StringBuilder& result, const String& source, unsigned offset, unsigned length, EntityMask entityMask)
{
    const CharacterType* text = source.characters<CharacterType>() + offset;

    size_t positionAfterLastEntity = 0;
    for (size_t i = 0; i < length; ++i) {
        CharacterType character = text[i];
        uint8_t substitution = character < WTF_ARRAY_LENGTH(entityMap) ? entityMap[character] : static_cast<uint8_t>(EntitySubstitutionNullIndex);
        if (UNLIKELY(substitution != EntitySubstitutionNullIndex) && entitySubstitutionList[substitution].mask & entityMask) {
            result.append(text + positionAfterLastEntity, i - positionAfterLastEntity);
            result.append(entitySubstitutionList[substitution].characters, entitySubstitutionList[substitution].length);
            positionAfterLastEntity = i + 1;
        }
    }
    result.append(text + positionAfterLastEntity, length - positionAfterLastEntity);
}

void MarkupAccumulator::appendCharactersReplacingEntities(StringBuilder& result, const String& source, unsigned offset, unsigned length, EntityMask entityMask)
{
    if (!(offset + length))
        return;

    ASSERT(offset + length <= source.length());

    if (source.is8Bit())
        appendCharactersReplacingEntitiesInternal<LChar>(result, source, offset, length, entityMask);
    else
        appendCharactersReplacingEntitiesInternal<UChar>(result, source, offset, length, entityMask);
}

MarkupAccumulator::MarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs resolveUrlsMethod, const Range* range, EFragmentSerialization fragmentSerialization)
    : m_nodes(nodes)
    , m_range(range)
    , m_resolveURLsMethod(resolveUrlsMethod)
    , m_fragmentSerialization(fragmentSerialization)
    , m_prefixLevel(0)
{
}

MarkupAccumulator::~MarkupAccumulator()
{
}

String MarkupAccumulator::serializeNodes(Node& targetNode, Node* nodeToSkip, EChildrenOnly childrenOnly, Vector<QualifiedName>* tagNamesToSkip)
{
    serializeNodesWithNamespaces(targetNode, nodeToSkip, childrenOnly, 0, tagNamesToSkip);
    return m_markup.toString();
}

void MarkupAccumulator::serializeNodesWithNamespaces(Node& targetNode, Node* nodeToSkip, EChildrenOnly childrenOnly, const Namespaces* namespaces, Vector<QualifiedName>* tagNamesToSkip)
{
    if (&targetNode == nodeToSkip)
        return;

    if (tagNamesToSkip && targetNode.isElementNode()) {
        for (auto& name : *tagNamesToSkip) {
            if (toElement(targetNode).hasTagName(name))
                return;
        }
    }

    Namespaces namespaceHash;
    if (namespaces)
        namespaceHash = *namespaces;
    else if (inXMLFragmentSerialization()) {
        // Make sure xml prefix and namespace are always known to uphold the constraints listed at http://www.w3.org/TR/xml-names11/#xmlReserved.
        namespaceHash.set(xmlAtom.impl(), XMLNames::xmlNamespaceURI.impl());
        namespaceHash.set(XMLNames::xmlNamespaceURI.impl(), xmlAtom.impl());
    }

    if (!childrenOnly)
        appendStartTag(targetNode, &namespaceHash);

    if (!(targetNode.document().isHTMLDocument() && elementCannotHaveEndTag(targetNode))) {
#if ENABLE(TEMPLATE_ELEMENT)
        Node* current = targetNode.hasTagName(templateTag) ? toHTMLTemplateElement(targetNode).content()->firstChild() : targetNode.firstChild();
#else
        Node* current = targetNode.firstChild();
#endif
        for ( ; current; current = current->nextSibling())
            serializeNodesWithNamespaces(*current, nodeToSkip, IncludeNode, &namespaceHash, tagNamesToSkip);
    }

    if (!childrenOnly)
        appendEndTag(targetNode);
}

String MarkupAccumulator::resolveURLIfNeeded(const Element& element, const String& urlString) const
{
    switch (m_resolveURLsMethod) {
    case ResolveAllURLs:
        return element.document().completeURL(urlString).string();

    case ResolveNonLocalURLs:
        if (!element.document().url().isLocalFile())
            return element.document().completeURL(urlString).string();
        break;

    case DoNotResolveURLs:
        break;
    }
    return urlString;
}

void MarkupAccumulator::appendString(const String& string)
{
    m_markup.append(string);
}

void MarkupAccumulator::appendStartTag(const Node& node, Namespaces* namespaces)
{
    appendStartMarkup(m_markup, node, namespaces);
    if (m_nodes)
        m_nodes->append(const_cast<Node*>(&node));
}

void MarkupAccumulator::appendEndTag(const Node& node)
{
    appendEndMarkup(m_markup, node);
}

size_t MarkupAccumulator::totalLength(const Vector<String>& strings)
{
    size_t length = 0;
    for (size_t i = 0; i < strings.size(); ++i)
        length += strings[i].length();
    return length;
}

void MarkupAccumulator::concatenateMarkup(StringBuilder& result)
{
    result.append(m_markup);
}

void MarkupAccumulator::appendAttributeValue(StringBuilder& result, const String& attribute, bool documentIsHTML)
{
    appendCharactersReplacingEntities(result, attribute, 0, attribute.length(),
        documentIsHTML ? EntityMaskInHTMLAttributeValue : EntityMaskInAttributeValue);
}

void MarkupAccumulator::appendCustomAttributes(StringBuilder&, const Element&, Namespaces*)
{
}

void MarkupAccumulator::appendQuotedURLAttributeValue(StringBuilder& result, const Element& element, const Attribute& attribute)
{
    ASSERT(element.isURLAttribute(attribute));
    const String resolvedURLString = resolveURLIfNeeded(element, attribute.value());
    UChar quoteChar = '"';
    String strippedURLString = resolvedURLString.stripWhiteSpace();
    if (protocolIsJavaScript(strippedURLString)) {
        // minimal escaping for javascript urls
        if (strippedURLString.contains('"')) {
            if (strippedURLString.contains('\''))
                strippedURLString.replaceWithLiteral('"', "&quot;");
            else
                quoteChar = '\'';
        }
        result.append(quoteChar);
        result.append(strippedURLString);
        result.append(quoteChar);
        return;
    }

    // FIXME: This does not fully match other browsers. Firefox percent-escapes non-ASCII characters for innerHTML.
    result.append(quoteChar);
    appendAttributeValue(result, resolvedURLString, false);
    result.append(quoteChar);
}

void MarkupAccumulator::appendNodeValue(StringBuilder& result, const Node& node, const Range* range, EntityMask entityMask)
{
    const String str = node.nodeValue();
    unsigned length = str.length();
    unsigned start = 0;

    if (range) {
        if (&node == range->endContainer())
            length = range->endOffset();
        if (&node == range->startContainer()) {
            start = range->startOffset();
            length -= start;
        }
    }

    appendCharactersReplacingEntities(result, str, start, length, entityMask);
}

bool MarkupAccumulator::shouldAddNamespaceElement(const Element& element)
{
    // Don't add namespace attribute if it is already defined for this elem.
    const AtomicString& prefix = element.prefix();
    if (prefix.isEmpty())
        return !element.hasAttribute(xmlnsAtom);

    DEPRECATED_DEFINE_STATIC_LOCAL(String, xmlnsWithColon, (ASCIILiteral("xmlns:")));
    return !element.hasAttribute(xmlnsWithColon + prefix);
}

bool MarkupAccumulator::shouldAddNamespaceAttribute(const Attribute& attribute, Namespaces& namespaces)
{
    namespaces.checkConsistency();

    // Don't add namespace attributes twice
    // HTML Parser will create xmlns attributes without namespace for HTML elements, allow those as well.
    if (attribute.name().localName() == xmlnsAtom && (attribute.namespaceURI().isEmpty() || attribute.namespaceURI() == XMLNSNames::xmlnsNamespaceURI)) {
        namespaces.set(emptyAtom.impl(), attribute.value().impl());
        return false;
    }

    QualifiedName xmlnsPrefixAttr(xmlnsAtom, attribute.localName(), XMLNSNames::xmlnsNamespaceURI);
    if (attribute.name() == xmlnsPrefixAttr) {
        namespaces.set(attribute.localName().impl(), attribute.value().impl());
        namespaces.set(attribute.value().impl(), attribute.localName().impl());
        return false;
    }

    return true;
}

void MarkupAccumulator::appendNamespace(StringBuilder& result, const AtomicString& prefix, const AtomicString& namespaceURI, Namespaces& namespaces, bool allowEmptyDefaultNS)
{
    namespaces.checkConsistency();
    if (namespaceURI.isEmpty()) {
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-xhtml-syntax.html#xml-fragment-serialization-algorithm
        if (allowEmptyDefaultNS && namespaces.get(emptyAtom.impl())) {
            result.append(' ');
            result.append(xmlnsAtom.string());
            result.appendLiteral("=\"\"");
        }
        return;
    }

    // Use emptyAtoms's impl() for both null and empty strings since the HashMap can't handle 0 as a key
    AtomicStringImpl* pre = prefix.isEmpty() ? emptyAtom.impl() : prefix.impl();
    AtomicStringImpl* foundNS = namespaces.get(pre);
    if (foundNS != namespaceURI.impl()) {
        namespaces.set(pre, namespaceURI.impl());
        // Add namespace to prefix pair so we can do constraint checking later.
        if (inXMLFragmentSerialization() && !prefix.isEmpty())
            namespaces.set(namespaceURI.impl(), pre);
        // Make sure xml prefix and namespace are always known to uphold the constraints listed at http://www.w3.org/TR/xml-names11/#xmlReserved.
        if (namespaceURI.impl() == XMLNames::xmlNamespaceURI.impl())
            return;
        result.append(' ');
        result.append(xmlnsAtom.string());
        if (!prefix.isEmpty()) {
            result.append(':');
            result.append(prefix);
        }

        result.append('=');
        result.append('"');
        appendAttributeValue(result, namespaceURI, false);
        result.append('"');
    }
}

EntityMask MarkupAccumulator::entityMaskForText(const Text& text) const
{
    if (!text.document().isHTMLDocument())
        return EntityMaskInPCDATA;

    const QualifiedName* parentName = 0;
    if (text.parentElement())
        parentName = &text.parentElement()->tagQName();

    if (parentName && (*parentName == scriptTag || *parentName == styleTag || *parentName == xmpTag))
        return EntityMaskInCDATA;
    return EntityMaskInHTMLPCDATA;
}

void MarkupAccumulator::appendText(StringBuilder& result, const Text& text)
{
    appendNodeValue(result, text, m_range, entityMaskForText(text));
}

static void appendComment(StringBuilder& result, const String& comment)
{
    // FIXME: Comment content is not escaped, but XMLSerializer (and possibly other callers) should raise an exception if it includes "-->".
    result.appendLiteral("<!--");
    result.append(comment);
    result.appendLiteral("-->");
}

void MarkupAccumulator::appendXMLDeclaration(StringBuilder& result, const Document& document)
{
    if (!document.hasXMLDeclaration())
        return;

    result.appendLiteral("<?xml version=\"");
    result.append(document.xmlVersion());
    const String& encoding = document.xmlEncoding();
    if (!encoding.isEmpty()) {
        result.appendLiteral("\" encoding=\"");
        result.append(encoding);
    }
    if (document.xmlStandaloneStatus() != Document::StandaloneUnspecified) {
        result.appendLiteral("\" standalone=\"");
        if (document.xmlStandalone())
            result.appendLiteral("yes");
        else
            result.appendLiteral("no");
    }

    result.appendLiteral("\"?>");
}

void MarkupAccumulator::appendDocumentType(StringBuilder& result, const DocumentType& documentType)
{
    if (documentType.name().isEmpty())
        return;

    result.appendLiteral("<!DOCTYPE ");
    result.append(documentType.name());
    if (!documentType.publicId().isEmpty()) {
        result.appendLiteral(" PUBLIC \"");
        result.append(documentType.publicId());
        result.append('"');
        if (!documentType.systemId().isEmpty()) {
            result.append(' ');
            result.append('"');
            result.append(documentType.systemId());
            result.append('"');
        }
    } else if (!documentType.systemId().isEmpty()) {
        result.appendLiteral(" SYSTEM \"");
        result.append(documentType.systemId());
        result.append('"');
    }
    if (!documentType.internalSubset().isEmpty()) {
        result.append(' ');
        result.append('[');
        result.append(documentType.internalSubset());
        result.append(']');
    }
    result.append('>');
}

void MarkupAccumulator::appendProcessingInstruction(StringBuilder& result, const String& target, const String& data)
{
    // FIXME: PI data is not escaped, but XMLSerializer (and possibly other callers) this should raise an exception if it includes "?>".
    result.append('<');
    result.append('?');
    result.append(target);
    result.append(' ');
    result.append(data);
    result.append('?');
    result.append('>');
}

void MarkupAccumulator::appendElement(StringBuilder& result, const Element& element, Namespaces* namespaces)
{
    appendOpenTag(result, element, namespaces);

    if (element.hasAttributes()) {
        for (const Attribute& attribute : element.attributesIterator())
            appendAttribute(result, element, attribute, namespaces);
    }

    // Give an opportunity to subclasses to add their own attributes.
    appendCustomAttributes(result, element, namespaces);

    appendCloseTag(result, element);
}

void MarkupAccumulator::appendOpenTag(StringBuilder& result, const Element& element, Namespaces* namespaces)
{
    result.append('<');
    if (inXMLFragmentSerialization() && namespaces && element.prefix().isEmpty()) {
        // According to http://www.w3.org/TR/DOM-Level-3-Core/namespaces-algorithms.html#normalizeDocumentAlgo we now should create
        // a default namespace declaration to make this namespace well-formed. However, http://www.w3.org/TR/xml-names11/#xmlReserved states
        // "The prefix xml MUST NOT be declared as the default namespace.", so use the xml prefix explicitly.
        if (element.namespaceURI() == XMLNames::xmlNamespaceURI) {
            result.append(xmlAtom);
            result.append(':');
        }
    }
    result.append(element.nodeNamePreservingCase());
    if ((inXMLFragmentSerialization() || !element.document().isHTMLDocument()) && namespaces && shouldAddNamespaceElement(element))
        appendNamespace(result, element.prefix(), element.namespaceURI(), *namespaces, inXMLFragmentSerialization());
}

void MarkupAccumulator::appendCloseTag(StringBuilder& result, const Element& element)
{
    if (shouldSelfClose(element)) {
        if (element.isHTMLElement())
            result.append(' '); // XHTML 1.0 <-> HTML compatibility.
        result.append('/');
    }
    result.append('>');
}

static inline bool attributeIsInSerializedNamespace(const Attribute& attribute)
{
    return attribute.namespaceURI() == XMLNames::xmlNamespaceURI
        || attribute.namespaceURI() == XLinkNames::xlinkNamespaceURI
        || attribute.namespaceURI() == XMLNSNames::xmlnsNamespaceURI;
}

void MarkupAccumulator::generateUniquePrefix(QualifiedName& prefixedName, const Namespaces& namespaces)
{
    // http://www.w3.org/TR/DOM-Level-3-Core/namespaces-algorithms.html#normalizeDocumentAlgo
    // Find a prefix following the pattern "NS" + index (starting at 1) and make sure this
    // prefix is not declared in the current scope.
    StringBuilder builder;
    do {
        builder.clear();
        builder.append("NS");
        builder.appendNumber(++m_prefixLevel);
        const AtomicString& name = builder.toAtomicString();
        if (!namespaces.get(name.impl())) {
            prefixedName.setPrefix(name);
            return;
        }
    } while (true);
}

void MarkupAccumulator::appendAttribute(StringBuilder& result, const Element& element, const Attribute& attribute, Namespaces* namespaces)
{
    bool documentIsHTML = element.document().isHTMLDocument();

    result.append(' ');

    QualifiedName prefixedName = attribute.name();
    if (documentIsHTML && !attributeIsInSerializedNamespace(attribute))
        result.append(attribute.name().localName());
    else {
        if (!attribute.namespaceURI().isEmpty()) {
            AtomicStringImpl* foundNS = namespaces && attribute.prefix().impl() ? namespaces->get(attribute.prefix().impl()) : 0;
            bool prefixIsAlreadyMappedToOtherNS = foundNS && foundNS != attribute.namespaceURI().impl();
            if (attribute.prefix().isEmpty() || !foundNS || prefixIsAlreadyMappedToOtherNS) {
                if (AtomicStringImpl* prefix = namespaces ? namespaces->get(attribute.namespaceURI().impl()) : 0)
                    prefixedName.setPrefix(AtomicString(prefix));
                else {
                    bool shouldBeDeclaredUsingAppendNamespace = !attribute.prefix().isEmpty() && !foundNS;
                    if (!shouldBeDeclaredUsingAppendNamespace && attribute.localName() != xmlnsAtom && namespaces)
                        generateUniquePrefix(prefixedName, *namespaces);
                }
            }
        }
        result.append(prefixedName.toString());
    }

    result.append('=');

    if (element.isURLAttribute(attribute))
        appendQuotedURLAttributeValue(result, element, attribute);
    else {
        result.append('"');
        appendAttributeValue(result, attribute.value(), documentIsHTML);
        result.append('"');
    }

    if ((inXMLFragmentSerialization() || !documentIsHTML) && namespaces && shouldAddNamespaceAttribute(attribute, *namespaces))
        appendNamespace(result, prefixedName.prefix(), prefixedName.namespaceURI(), *namespaces);
}

void MarkupAccumulator::appendCDATASection(StringBuilder& result, const String& section)
{
    // FIXME: CDATA content is not escaped, but XMLSerializer (and possibly other callers) should raise an exception if it includes "]]>".
    result.appendLiteral("<![CDATA[");
    result.append(section);
    result.appendLiteral("]]>");
}

void MarkupAccumulator::appendStartMarkup(StringBuilder& result, const Node& node, Namespaces* namespaces)
{
    if (namespaces)
        namespaces->checkConsistency();

    switch (node.nodeType()) {
    case Node::TEXT_NODE:
        appendText(result, toText(node));
        break;
    case Node::COMMENT_NODE:
        appendComment(result, toComment(node).data());
        break;
    case Node::DOCUMENT_NODE:
        appendXMLDeclaration(result, toDocument(node));
        break;
    case Node::DOCUMENT_FRAGMENT_NODE:
        break;
    case Node::DOCUMENT_TYPE_NODE:
        appendDocumentType(result, toDocumentType(node));
        break;
    case Node::PROCESSING_INSTRUCTION_NODE:
        appendProcessingInstruction(result, toProcessingInstruction(node).target(), toProcessingInstruction(node).data());
        break;
    case Node::ELEMENT_NODE:
        appendElement(result, toElement(node), namespaces);
        break;
    case Node::CDATA_SECTION_NODE:
        appendCDATASection(result, toCDATASection(node).data());
        break;
    case Node::ATTRIBUTE_NODE:
    case Node::ENTITY_NODE:
    case Node::ENTITY_REFERENCE_NODE:
    case Node::NOTATION_NODE:
    case Node::XPATH_NAMESPACE_NODE:
        ASSERT_NOT_REACHED();
        break;
    }
}

// Rules of self-closure
// 1. No elements in HTML documents use the self-closing syntax.
// 2. Elements w/ children never self-close because they use a separate end tag.
// 3. HTML elements which do not have a "forbidden" end tag will close with a separate end tag.
// 4. Other elements self-close.
bool MarkupAccumulator::shouldSelfClose(const Node& node)
{
    if (!inXMLFragmentSerialization() && node.document().isHTMLDocument())
        return false;
    if (node.hasChildNodes())
        return false;
    if (node.isHTMLElement() && !elementCannotHaveEndTag(node))
        return false;
    return true;
}

bool MarkupAccumulator::elementCannotHaveEndTag(const Node& node)
{
    if (!node.isHTMLElement())
        return false;

    // FIXME: ieForbidsInsertHTML may not be the right function to call here
    // ieForbidsInsertHTML is used to disallow setting innerHTML/outerHTML
    // or createContextualFragment.  It does not necessarily align with
    // which elements should be serialized w/o end tags.
    return toHTMLElement(node).ieForbidsInsertHTML();
}

void MarkupAccumulator::appendEndMarkup(StringBuilder& result, const Node& node)
{
    if (!node.isElementNode() || shouldSelfClose(node) || (!node.hasChildNodes() && elementCannotHaveEndTag(node)))
        return;

    result.append('<');
    result.append('/');
    result.append(toElement(node).nodeNamePreservingCase());
    result.append('>');
}

}
