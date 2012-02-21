/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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
#include "PropertySetCSSStyleDeclaration.h"

#include "CSSParser.h"
#include "HTMLNames.h"
#include "InspectorInstrumentation.h"
#include "MutationObserverInterestGroup.h"
#include "MutationRecord.h"
#include "StylePropertySet.h"
#include "StyledElement.h"

using namespace std;

namespace WebCore {

namespace {

class StyleAttributeMutationScope {
    WTF_MAKE_NONCOPYABLE(StyleAttributeMutationScope);
public:
    StyleAttributeMutationScope(PropertySetCSSStyleDeclaration* decl)
    {
        ++s_scopeCount;

        if (s_scopeCount != 1) {
            ASSERT(s_currentDecl == decl);
            return;
        }

        ASSERT(!s_currentDecl);
        s_currentDecl = decl;

#if ENABLE(MUTATION_OBSERVERS)
        if (!s_currentDecl->parentElement())
            return;
        m_mutationRecipients = MutationObserverInterestGroup::createForAttributesMutation(s_currentDecl->parentElement(), HTMLNames::styleAttr);
        if (!m_mutationRecipients)
            return;

        AtomicString oldValue = m_mutationRecipients->isOldValueRequested() ? s_currentDecl->parentElement()->getAttribute(HTMLNames::styleAttr) : nullAtom;
        m_mutation = MutationRecord::createAttributes(s_currentDecl->parentElement(), HTMLNames::styleAttr, oldValue);
#endif
    }

    ~StyleAttributeMutationScope()
    {
        --s_scopeCount;
        if (s_scopeCount)
            return;

#if ENABLE(MUTATION_OBSERVERS)
        if (m_mutation && s_shouldDeliver)
            m_mutationRecipients->enqueueMutationRecord(m_mutation);
        s_shouldDeliver = false;
#endif
        if (!s_shouldNotifyInspector) {
            s_currentDecl = 0;
            return;
        }
        // We have to clear internal state before calling Inspector's code.
        PropertySetCSSStyleDeclaration* localCopyStyleDecl = s_currentDecl;
        s_currentDecl = 0;
        s_shouldNotifyInspector = false;
        if (localCopyStyleDecl->parentElement() && localCopyStyleDecl->parentElement()->document())
            InspectorInstrumentation::didInvalidateStyleAttr(localCopyStyleDecl->parentElement()->document(), localCopyStyleDecl->parentElement());
    }

#if ENABLE(MUTATION_OBSERVERS)
    void enqueueMutationRecord()
    {
        s_shouldDeliver = true;
    }
#endif

    void didInvalidateStyleAttr()
    {
        s_shouldNotifyInspector = true;
    }

private:
    static unsigned s_scopeCount;
    static PropertySetCSSStyleDeclaration* s_currentDecl;
    static bool s_shouldNotifyInspector;
#if ENABLE(MUTATION_OBSERVERS)
    static bool s_shouldDeliver;

    OwnPtr<MutationObserverInterestGroup> m_mutationRecipients;
    RefPtr<MutationRecord> m_mutation;
#endif
};

unsigned StyleAttributeMutationScope::s_scopeCount = 0;
PropertySetCSSStyleDeclaration* StyleAttributeMutationScope::s_currentDecl = 0;
bool StyleAttributeMutationScope::s_shouldNotifyInspector = false;
#if ENABLE(MUTATION_OBSERVERS)
bool StyleAttributeMutationScope::s_shouldDeliver = false;
#endif

} // namespace
    
void PropertySetCSSStyleDeclaration::ref()
{ 
    m_propertySet->ref();
}

void PropertySetCSSStyleDeclaration::deref()
{ 
    m_propertySet->deref(); 
}

unsigned PropertySetCSSStyleDeclaration::length() const
{
    return m_propertySet->propertyCount();
}

String PropertySetCSSStyleDeclaration::item(unsigned i) const
{
    if (i >= m_propertySet->propertyCount())
        return "";
    return getPropertyName(static_cast<CSSPropertyID>(m_propertySet->propertyAt(i).id()));
}

String PropertySetCSSStyleDeclaration::cssText() const
{
    return m_propertySet->asText();
}
    
void PropertySetCSSStyleDeclaration::setCssText(const String& text, ExceptionCode& ec)
{
#if ENABLE(MUTATION_OBSERVERS)
    StyleAttributeMutationScope mutationScope(this);
#endif
    ec = 0;
    // FIXME: Detect syntax errors and set ec.
    m_propertySet->parseDeclaration(text, contextStyleSheet());

    setNeedsStyleRecalc();
#if ENABLE(MUTATION_OBSERVERS)
    mutationScope.enqueueMutationRecord();    
#endif
}

PassRefPtr<CSSValue> PropertySetCSSStyleDeclaration::getPropertyCSSValue(const String& propertyName)
{
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return 0;
    return m_propertySet->getPropertyCSSValue(propertyID);
}

String PropertySetCSSStyleDeclaration::getPropertyValue(const String &propertyName)
{
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();
    return m_propertySet->getPropertyValue(propertyID);
}

String PropertySetCSSStyleDeclaration::getPropertyPriority(const String& propertyName)
{
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();
    return m_propertySet->propertyIsImportant(propertyID) ? "important" : "";
}

String PropertySetCSSStyleDeclaration::getPropertyShorthand(const String& propertyName)
{
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();
    int shorthandID = m_propertySet->getPropertyShorthand(propertyID);
    if (!shorthandID)
        return String();
    return getPropertyName(static_cast<CSSPropertyID>(shorthandID));
}

bool PropertySetCSSStyleDeclaration::isPropertyImplicit(const String& propertyName)
{
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return false;
    return m_propertySet->isPropertyImplicit(propertyID);
}

void PropertySetCSSStyleDeclaration::setProperty(const String& propertyName, const String& value, const String& priority, ExceptionCode& ec)
{
#if ENABLE(MUTATION_OBSERVERS)
    StyleAttributeMutationScope mutationScope(this);
#endif
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return;
    bool important = priority.find("important", 0, false) != notFound;
    ec = 0;
    bool changed = m_propertySet->setProperty(propertyID, value, important, contextStyleSheet());
    if (changed) {
        // CSS DOM requires raising SYNTAX_ERR of parsing failed, but this is too dangerous for compatibility,
        // see <http://bugs.webkit.org/show_bug.cgi?id=7296>.
        setNeedsStyleRecalc();
#if ENABLE(MUTATION_OBSERVERS)
        mutationScope.enqueueMutationRecord();
#endif
    }
}

String PropertySetCSSStyleDeclaration::removeProperty(const String& propertyName, ExceptionCode& ec)
{
#if ENABLE(MUTATION_OBSERVERS)
    StyleAttributeMutationScope mutationScope(this);
#endif
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return String();
    ec = 0;
    String result;
    bool changes = m_propertySet->removeProperty(propertyID, &result);
    if (changes) {
        setNeedsStyleRecalc();
#if ENABLE(MUTATION_OBSERVERS)
        mutationScope.enqueueMutationRecord();
#endif
    }
    return result;
}

PassRefPtr<CSSValue> PropertySetCSSStyleDeclaration::getPropertyCSSValueInternal(CSSPropertyID propertyID)
{ 
    return m_propertySet->getPropertyCSSValue(propertyID); 
}

String PropertySetCSSStyleDeclaration::getPropertyValueInternal(CSSPropertyID propertyID)
{ 
    return m_propertySet->getPropertyValue(propertyID); 
}

void PropertySetCSSStyleDeclaration::setPropertyInternal(CSSPropertyID propertyID, const String& value, bool important, ExceptionCode& ec)
{ 
#if ENABLE(MUTATION_OBSERVERS)
    StyleAttributeMutationScope mutationScope(this);
#endif
    ec = 0;
    bool changed = m_propertySet->setProperty(propertyID, value, important, contextStyleSheet());
    if (changed) {
        setNeedsStyleRecalc();
#if ENABLE(MUTATION_OBSERVERS)
        mutationScope.enqueueMutationRecord();
#endif
    }
}

CSSStyleSheet* PropertySetCSSStyleDeclaration::parentStyleSheet() const
{ 
    return contextStyleSheet(); 
}

PassRefPtr<StylePropertySet> PropertySetCSSStyleDeclaration::copy() const
{
    return m_propertySet->copy();
}

PassRefPtr<StylePropertySet> PropertySetCSSStyleDeclaration::makeMutable()
{
    return m_propertySet;
}

bool PropertySetCSSStyleDeclaration::cssPropertyMatches(const CSSProperty* property) const
{
    return m_propertySet->propertyMatches(property);
}

void RuleCSSStyleDeclaration::setNeedsStyleRecalc()
{
    if (CSSStyleSheet* styleSheet = contextStyleSheet()) {
        if (Document* document = styleSheet->findDocument())
            document->styleSelectorChanged(DeferRecalcStyle);
    }
}
    
CSSStyleSheet* RuleCSSStyleDeclaration::contextStyleSheet() const
{
    return m_parentRule ? m_parentRule->parentStyleSheet() : 0;
}

void InlineCSSStyleDeclaration::setNeedsStyleRecalc()
{
    if (!m_parentElement)
        return;
    m_parentElement->setNeedsStyleRecalc(InlineStyleChange);
    m_parentElement->invalidateStyleAttribute();
    StyleAttributeMutationScope(this).didInvalidateStyleAttr();
    return;
}

CSSStyleSheet* InlineCSSStyleDeclaration::contextStyleSheet() const
{
    return m_parentElement ? m_parentElement->document()->elementSheet() : 0;
}

} // namespace WebCore
