/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "CSSStyleSheet.h"

#include "CSSFontFaceRule.h"
#include "CSSImportRule.h"
#include "CSSNamespace.h"
#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "Node.h"
#include "SVGNames.h"
#include "SecurityOrigin.h"
#include "TextEncoding.h"
#include <wtf/Deque.h>

namespace WebCore {

#if !ASSERT_DISABLED
static bool isAcceptableCSSStyleSheetParent(Node* parentNode)
{
    // Only these nodes can be parents of StyleSheets, and they need to call clearOwnerNode() when moved out of document.
    return !parentNode
        || parentNode->isDocumentNode()
        || parentNode->hasTagName(HTMLNames::linkTag)
        || parentNode->hasTagName(HTMLNames::styleTag)
#if ENABLE(SVG)
        || parentNode->hasTagName(SVGNames::styleTag)
#endif
        || parentNode->nodeType() == Node::PROCESSING_INSTRUCTION_NODE;
}
#endif

CSSStyleSheet::CSSStyleSheet(Node* parentNode, const String& href, const KURL& baseURL, const String& charset)
    : StyleSheet(parentNode, href, baseURL)
    , m_charset(charset)
    , m_loadCompleted(false)
    , m_strictParsing(false)
    , m_isUserStyleSheet(false)
    , m_hasSyntacticallyValidCSSHeader(true)
{
    ASSERT(isAcceptableCSSStyleSheetParent(parentNode));
}

CSSStyleSheet::CSSStyleSheet(CSSRule* ownerRule, const String& href, const KURL& baseURL, const String& charset)
    : StyleSheet(ownerRule, href, baseURL)
    , m_charset(charset)
    , m_loadCompleted(false)
    , m_strictParsing(!ownerRule || ownerRule->useStrictParsing())
    , m_hasSyntacticallyValidCSSHeader(true)
{
    CSSStyleSheet* parentSheet = ownerRule ? ownerRule->parentStyleSheet() : 0;
    m_isUserStyleSheet = parentSheet ? parentSheet->isUserStyleSheet() : false;
}

CSSStyleSheet::~CSSStyleSheet()
{
    // For style rules outside the document, .parentStyleSheet can become null even if the style rule
    // is still observable from JavaScript. This matches the behavior of .parentNode for nodes, but
    // it's not ideal because it makes the CSSOM's behavior depend on the timing of garbage collection.
    for (unsigned i = 0; i < m_children.size(); ++i) {
        ASSERT(m_children.at(i)->parentStyleSheet() == this);
        m_children.at(i)->setParentStyleSheet(0);
    }
}

void CSSStyleSheet::append(PassRefPtr<CSSRule> child)
{
    CSSRule* c = child.get();
    m_children.append(child);
    if (c->isImportRule())
        static_cast<CSSImportRule*>(c)->requestStyleSheet();
}

void CSSStyleSheet::remove(unsigned index)
{
    m_children.remove(index);
}

unsigned CSSStyleSheet::insertRule(const String& rule, unsigned index, ExceptionCode& ec)
{
    ec = 0;
    if (index > m_children.size()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }
    CSSParser p(useStrictParsing());
    RefPtr<CSSRule> r = p.parseRule(this, rule);

    if (!r) {
        ec = SYNTAX_ERR;
        return 0;
    }

    // Throw a HIERARCHY_REQUEST_ERR exception if the rule cannot be inserted at the specified index.  The best
    // example of this is an @import rule inserted after regular rules.
    if (index > 0) {
        if (r->isImportRule()) {
            // Check all the rules that come before this one to make sure they are only @charset and @import rules.
            for (unsigned i = 0; i < index; ++i) {
                if (!m_children.at(i)->isCharsetRule() && !m_children.at(i)->isImportRule()) {
                    ec = HIERARCHY_REQUEST_ERR;
                    return 0;
                }
            }
        } else if (r->isCharsetRule()) {
            // The @charset rule has to come first and there can be only one.
            ec = HIERARCHY_REQUEST_ERR;
            return 0;
        }
    }

    CSSRule* c = r.get();
    m_children.insert(index, r.release());
    if (c->isImportRule())
        static_cast<CSSImportRule*>(c)->requestStyleSheet();

    styleSheetChanged();

    return index;
}

int CSSStyleSheet::addRule(const String& selector, const String& style, int index, ExceptionCode& ec)
{
    insertRule(selector + " { " + style + " }", index, ec);

    // As per Microsoft documentation, always return -1.
    return -1;
}

int CSSStyleSheet::addRule(const String& selector, const String& style, ExceptionCode& ec)
{
    return addRule(selector, style, m_children.size(), ec);
}

PassRefPtr<CSSRuleList> CSSStyleSheet::cssRules(bool omitCharsetRules)
{
    KURL url = finalURL();
    Document* document = findDocument();
    if (!url.isEmpty() && document && !document->securityOrigin()->canRequest(url))
        return 0;
    return CSSRuleList::create(this, omitCharsetRules);
}

void CSSStyleSheet::deleteRule(unsigned index, ExceptionCode& ec)
{
    if (index >= m_children.size()) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    ec = 0;
    m_children.at(index)->setParentStyleSheet(0);
    m_children.remove(index);
    styleSheetChanged();
}

void CSSStyleSheet::addNamespace(CSSParser* p, const AtomicString& prefix, const AtomicString& uri)
{
    if (uri.isNull())
        return;

    m_namespaces = adoptPtr(new CSSNamespace(prefix, uri, m_namespaces.release()));

    if (prefix.isEmpty())
        // Set the default namespace on the parser so that selectors that omit namespace info will
        // be able to pick it up easily.
        p->m_defaultNamespace = uri;
}

const AtomicString& CSSStyleSheet::determineNamespace(const AtomicString& prefix)
{
    if (prefix.isNull())
        return nullAtom; // No namespace. If an element/attribute has a namespace, we won't match it.
    if (prefix == starAtom)
        return starAtom; // We'll match any namespace.
    if (m_namespaces) {
        if (CSSNamespace* namespaceForPrefix = m_namespaces->namespaceForPrefix(prefix))
            return namespaceForPrefix->uri;
    }
    return nullAtom; // Assume we won't match any namespaces.
}

bool CSSStyleSheet::parseString(const String &string, bool strict)
{
    return parseStringAtLine(string, strict, 0);
}

bool CSSStyleSheet::parseStringAtLine(const String& string, bool strict, int startLineNumber)
{
    setStrictParsing(strict);
    CSSParser p(strict);
    p.parseSheet(this, string, startLineNumber);
    return true;
}

bool CSSStyleSheet::isLoading()
{
    for (unsigned i = 0; i < m_children.size(); ++i) {
        CSSRule* rule = m_children.at(i).get();
        if (rule->isImportRule() && static_cast<CSSImportRule*>(rule)->isLoading())
            return true;
    }
    return false;
}

void CSSStyleSheet::checkLoaded()
{
    if (isLoading())
        return;

    // Avoid |this| being deleted by scripts that run via
    // ScriptableDocumentParser::executeScriptsWaitingForStylesheets().
    // See <rdar://problem/6622300>.
    RefPtr<CSSStyleSheet> protector(this);
    if (CSSStyleSheet* styleSheet = parentStyleSheet())
        styleSheet->checkLoaded();
    m_loadCompleted = ownerNode() ? ownerNode()->sheetLoaded() : true;
}

void CSSStyleSheet::startLoadingDynamicSheet()
{
    if (Node* owner = ownerNode())
        owner->startLoadingDynamicSheet();
}

Node* CSSStyleSheet::findStyleSheetOwnerNode() const
{
    for (const CSSStyleSheet* sheet = this; sheet; sheet = sheet->parentStyleSheet()) {
        if (Node* ownerNode = sheet->ownerNode())
            return ownerNode;
    }
    return 0;
}

Document* CSSStyleSheet::findDocument()
{
    Node* ownerNode = findStyleSheetOwnerNode();

    return ownerNode ? ownerNode->document() : 0;
}

void CSSStyleSheet::styleSheetChanged()
{
    CSSStyleSheet* rootSheet = this;
    while (CSSStyleSheet* parent = rootSheet->parentStyleSheet())
        rootSheet = parent;

    /* FIXME: We don't need to do everything updateStyleSelector does,
     * basically we just need to recreate the document's selector with the
     * already existing style sheets.
     */
    if (Document* documentToUpdate = rootSheet->findDocument())
        documentToUpdate->styleSelectorChanged(DeferRecalcStyle);
}

KURL CSSStyleSheet::completeURL(const String& url) const
{
    // Always return a null URL when passed a null string.
    // FIXME: Should we change the KURL constructor to have this behavior?
    // See also Document::completeURL(const String&)
    if (url.isNull())
        return KURL();
    if (m_charset.isEmpty())
        return KURL(baseURL(), url);
    const TextEncoding encoding = TextEncoding(m_charset);
    return KURL(baseURL(), url, encoding);
}

void CSSStyleSheet::addSubresourceStyleURLs(ListHashSet<KURL>& urls)
{
    Deque<CSSStyleSheet*> styleSheetQueue;
    styleSheetQueue.append(this);

    while (!styleSheetQueue.isEmpty()) {
        CSSStyleSheet* styleSheet = styleSheetQueue.takeFirst();

        for (unsigned i = 0; i < styleSheet->m_children.size(); ++i) {
            CSSRule* rule = styleSheet->m_children.at(i).get();
            if (rule->isImportRule()) {
                if (CSSStyleSheet* ruleStyleSheet = static_cast<CSSImportRule*>(rule)->styleSheet())
                    styleSheetQueue.append(ruleStyleSheet);
                static_cast<CSSImportRule*>(rule)->addSubresourceStyleURLs(urls);
            } else if (rule->isFontFaceRule())
                static_cast<CSSFontFaceRule*>(rule)->addSubresourceStyleURLs(urls);
            else if (rule->isStyleRule() || rule->isPageRule())
                static_cast<CSSStyleRule*>(rule)->addSubresourceStyleURLs(urls);
        }
    }
}

}
