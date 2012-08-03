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

#ifndef CSSFontFaceRule_h
#define CSSFontFaceRule_h

#include "CSSRule.h"
#include "PropertySetCSSStyleDeclaration.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSStyleDeclaration;
class StyleRuleFontFace;
class StyleRuleCSSStyleDeclaration;

class CSSFontFaceRule : public CSSRule {
public:
    static PassRefPtr<CSSFontFaceRule> create(StyleRuleFontFace* rule, CSSStyleSheet* sheet) { return adoptRef(new CSSFontFaceRule(rule, sheet)); }

    ~CSSFontFaceRule();

    CSSStyleDeclaration* style() const;

    String cssText() const;

    void reattach(StyleRuleFontFace*);

    void reportDescendantMemoryUsage(MemoryObjectInfo*) const;

private:
    CSSFontFaceRule(StyleRuleFontFace*, CSSStyleSheet* parent);

    RefPtr<StyleRuleFontFace> m_fontFaceRule;
    
    mutable RefPtr<StyleRuleCSSStyleDeclaration> m_propertiesCSSOMWrapper;
};

} // namespace WebCore

#endif // CSSFontFaceRule_h
