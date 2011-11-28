/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "CSSStyleRule.h"

#include "CSSMutableStyleDeclaration.h"
#include "CSSPageRule.h"
#include "CSSParser.h"
#include "CSSSelector.h"
#include "CSSStyleSheet.h"
#include "Document.h"
#include "StyledElement.h"
#include "StyleSheet.h"

namespace WebCore {

CSSStyleRule::CSSStyleRule(CSSStyleSheet* parent, int sourceLine, CSSRule::Type type)
    : CSSRule(parent, type)
{
    m_sourceLine = sourceLine;

    // m_sourceLine is a bitfield, so let's catch any overflow early in debug mode.
    ASSERT(m_sourceLine == sourceLine);
}

CSSStyleRule::~CSSStyleRule()
{
    if (m_style)
        m_style->setParentRule(0);
}

String CSSStyleRule::selectorText() const
{
    if (isPageRule())
        return static_cast<const CSSPageRule*>(this)->pageSelectorText();

    String str;
    for (CSSSelector* s = selectorList().first(); s; s = CSSSelectorList::next(s)) {
        if (s != selectorList().first())
            str += ", ";
        str += s->selectorText();
    }
    return str;
}

void CSSStyleRule::setSelectorText(const String& selectorText)
{
    Document* doc = 0;

    if (CSSStyleSheet* styleSheet = m_style->parentStyleSheet())
        doc = styleSheet->findDocument();

    if (!doc && m_style->isElementStyleDeclaration()) {
        if (StyledElement* element = static_cast<CSSElementStyleDeclaration*>(m_style.get())->element())
            doc = element->document();
    }

    if (!doc)
        return;

    CSSParser p;
    CSSSelectorList selectorList;
    p.parseSelector(selectorText, doc, selectorList);
    if (!selectorList.first())
        return;

    String oldSelectorText = this->selectorText();
    m_selectorList.adopt(selectorList);
    if (this->selectorText() == oldSelectorText)
        return;

    doc->styleSelectorChanged(DeferRecalcStyle);
}

String CSSStyleRule::cssText() const
{
    String result = selectorText();

    result += " { ";
    result += m_style->cssText();
    result += "}";

    return result;
}

void CSSStyleRule::addSubresourceStyleURLs(ListHashSet<KURL>& urls)
{
    if (m_style)
        m_style->addSubresourceStyleURLs(urls);
}

} // namespace WebCore
