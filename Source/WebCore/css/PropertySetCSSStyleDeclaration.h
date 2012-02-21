/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PropertySetCSSStyleDeclaration_h
#define PropertySetCSSStyleDeclaration_h

#include "CSSStyleDeclaration.h"

namespace WebCore {

class CSSRule;
class CSSProperty;
class CSSStyleSheet;
class CSSValue;
class StylePropertySet;
class StyledElement;

class PropertySetCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    PropertySetCSSStyleDeclaration(StylePropertySet* propertySet) : m_propertySet(propertySet) { }
    
    virtual StyledElement* parentElement() const { return 0; }
    virtual void clearParentRule() { ASSERT_NOT_REACHED(); }
    virtual void clearParentElement() { ASSERT_NOT_REACHED(); }
    virtual CSSStyleSheet* contextStyleSheet() const { return 0; }
    
private:
    virtual void ref() OVERRIDE;
    virtual void deref() OVERRIDE;
    
    virtual CSSRule* parentRule() const OVERRIDE { return 0; };
    virtual unsigned length() const OVERRIDE;
    virtual String item(unsigned index) const OVERRIDE;
    virtual PassRefPtr<CSSValue> getPropertyCSSValue(const String& propertyName) OVERRIDE;
    virtual String getPropertyValue(const String& propertyName) OVERRIDE;
    virtual String getPropertyPriority(const String& propertyName) OVERRIDE;
    virtual String getPropertyShorthand(const String& propertyName) OVERRIDE;
    virtual bool isPropertyImplicit(const String& propertyName) OVERRIDE;
    virtual void setProperty(const String& propertyName, const String& value, const String& priority, ExceptionCode&) OVERRIDE;
    virtual String removeProperty(const String& propertyName, ExceptionCode&) OVERRIDE;
    virtual String cssText() const OVERRIDE;
    virtual void setCssText(const String&, ExceptionCode&) OVERRIDE;
    virtual PassRefPtr<CSSValue> getPropertyCSSValueInternal(CSSPropertyID) OVERRIDE;
    virtual String getPropertyValueInternal(CSSPropertyID) OVERRIDE;
    virtual void setPropertyInternal(CSSPropertyID, const String& value, bool important, ExceptionCode&) OVERRIDE;
    
    virtual bool cssPropertyMatches(const CSSProperty*) const OVERRIDE;
    virtual CSSStyleSheet* parentStyleSheet() const OVERRIDE;
    virtual PassRefPtr<StylePropertySet> copy() const OVERRIDE;
    virtual PassRefPtr<StylePropertySet> makeMutable() OVERRIDE;
    virtual void setNeedsStyleRecalc() { }    
    
protected:
    StylePropertySet* m_propertySet;
};

class RuleCSSStyleDeclaration : public PropertySetCSSStyleDeclaration
{
public:
    RuleCSSStyleDeclaration(StylePropertySet* propertySet, CSSRule* parentRule)
        : PropertySetCSSStyleDeclaration(propertySet)
        , m_parentRule(parentRule) 
    {
    }
    
private:    
    virtual CSSRule* parentRule() const { return m_parentRule; };
    virtual void clearParentRule() { m_parentRule = 0; }
    virtual void setNeedsStyleRecalc();
    virtual CSSStyleSheet* contextStyleSheet() const;
    
    CSSRule* m_parentRule;
};

class InlineCSSStyleDeclaration : public PropertySetCSSStyleDeclaration
{
public:
    InlineCSSStyleDeclaration(StylePropertySet* propertySet, StyledElement* parentElement)
        : PropertySetCSSStyleDeclaration(propertySet)
        , m_parentElement(parentElement) 
    {
    }
    
private:
    virtual StyledElement* parentElement() const { return m_parentElement; }
    virtual void clearParentElement() { m_parentElement = 0; }
    virtual void setNeedsStyleRecalc();
    virtual CSSStyleSheet* contextStyleSheet() const;
    
    StyledElement* m_parentElement;
};

} // namespace WebCore

#endif
