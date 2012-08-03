/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef CSSPageRule_h
#define CSSPageRule_h

#include "CSSRule.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSStyleDeclaration;
class CSSStyleSheet;
class StyleRulePage;
class StyleRuleCSSStyleDeclaration;

class CSSPageRule : public CSSRule {
public:
    static PassRefPtr<CSSPageRule> create(StyleRulePage* rule, CSSStyleSheet* sheet) { return adoptRef(new CSSPageRule(rule, sheet)); }

    ~CSSPageRule();

    CSSStyleDeclaration* style() const;

    String selectorText() const;
    void setSelectorText(const String&);

    String cssText() const;

    void reattach(StyleRulePage*);

    void reportDescendantMemoryUsage(MemoryObjectInfo*) const;

private:
    CSSPageRule(StyleRulePage*, CSSStyleSheet*);
    
    RefPtr<StyleRulePage> m_pageRule;

    mutable RefPtr<StyleRuleCSSStyleDeclaration> m_propertiesCSSOMWrapper;
};

} // namespace WebCore

#endif // CSSPageRule_h
