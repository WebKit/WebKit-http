/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
 *
 */

#include "config.h"
#include "ElementAttributeData.h"

#include "StyledElement.h"

namespace WebCore {

void AttributeVector::removeAttribute(const QualifiedName& name)
{
    size_t index = getAttributeItemIndex(name);
    if (index == notFound)
        return;

    RefPtr<Attribute> attribute = at(index);
    if (Attr* attr = attribute->attr())
        attr->m_element = 0;
    remove(index);
}

ElementAttributeData::~ElementAttributeData()
{
    detachAttributesFromElement();
}

void ElementAttributeData::setClass(const String& className, bool shouldFoldCase)
{
    m_classNames.set(className, shouldFoldCase);
}
    
StylePropertySet* ElementAttributeData::ensureInlineStyle(StyledElement* element)
{
    if (!m_inlineStyleDecl) {
        ASSERT(element->isStyledElement());
        m_inlineStyleDecl = StylePropertySet::create();
        m_inlineStyleDecl->setStrictParsing(element->isHTMLElement() && !element->document()->inQuirksMode());
    }
    return m_inlineStyleDecl.get();
}

StylePropertySet* ElementAttributeData::ensureMutableInlineStyle(StyledElement* element)
{
    if (m_inlineStyleDecl && !m_inlineStyleDecl->hasCSSOMWrapper()) {
        m_inlineStyleDecl = m_inlineStyleDecl->copy();
        m_inlineStyleDecl->setStrictParsing(element->isHTMLElement() && !element->document()->inQuirksMode());
        return m_inlineStyleDecl.get();
    }
    return ensureInlineStyle(element);
}
    
void ElementAttributeData::updateInlineStyleAvoidingMutation(StyledElement* element, const String& text)
{
    // We reconstruct the property set instead of mutating if there is no CSSOM wrapper.
    // This makes wrapperless property sets immutable and so cacheable.
    if (m_inlineStyleDecl && !m_inlineStyleDecl->hasCSSOMWrapper())
        m_inlineStyleDecl.clear();
    if (!m_inlineStyleDecl) {
        m_inlineStyleDecl = StylePropertySet::create();
        m_inlineStyleDecl->setStrictParsing(element->isHTMLElement() && !element->document()->inQuirksMode());
    }
    m_inlineStyleDecl->parseDeclaration(text, element->document()->elementSheet());
}

void ElementAttributeData::destroyInlineStyle(StyledElement* element)
{
    if (!m_inlineStyleDecl)
        return;
    m_inlineStyleDecl->clearParentElement(element);
    m_inlineStyleDecl = 0;
}

void ElementAttributeData::addAttribute(PassRefPtr<Attribute> prpAttribute, Element* element, EInUpdateStyleAttribute inUpdateStyleAttribute)
{
    RefPtr<Attribute> attribute = prpAttribute;

    if (element && inUpdateStyleAttribute == NotInUpdateStyleAttribute)
        element->willModifyAttribute(attribute->name(), nullAtom, attribute->value());

    m_attributes.append(attribute);
    if (Attr* attr = attribute->attr())
        attr->m_element = element;

    if (element && inUpdateStyleAttribute == NotInUpdateStyleAttribute)
        element->didAddAttribute(attribute.get());
}

void ElementAttributeData::removeAttribute(size_t index, Element* element, EInUpdateStyleAttribute inUpdateStyleAttribute)
{
    ASSERT(index < length());

    RefPtr<Attribute> attribute = m_attributes[index];

    if (element && inUpdateStyleAttribute == NotInUpdateStyleAttribute)
        element->willRemoveAttribute(attribute->name(), attribute->value());

    if (Attr* attr = attribute->attr())
        attr->m_element = 0;
    m_attributes.remove(index);

    if (element && inUpdateStyleAttribute == NotInUpdateStyleAttribute)
        element->didRemoveAttribute(attribute.get());
}

PassRefPtr<Attr> ElementAttributeData::takeAttribute(size_t index, Element* element)
{
    ASSERT(index < length());
    ASSERT(element);

    RefPtr<Attr> attr = m_attributes[index]->createAttrIfNeeded(element);
    removeAttribute(index, element);
    return attr.release();
}

bool ElementAttributeData::isEquivalent(const ElementAttributeData* other) const
{
    if (!other)
        return isEmpty();

    unsigned len = length();
    if (len != other->length())
        return false;

    for (unsigned i = 0; i < len; i++) {
        Attribute* attr = attributeItem(i);
        Attribute* otherAttr = other->getAttributeItem(attr->name());
        if (!otherAttr || attr->value() != otherAttr->value())
            return false;
    }

    return true;
}

void ElementAttributeData::detachAttributesFromElement()
{
    size_t size = m_attributes.size();
    for (size_t i = 0; i < size; i++) {
        if (Attr* attr = m_attributes[i]->attr())
            attr->m_element = 0;
    }
}

void ElementAttributeData::copyAttributesToVector(Vector<RefPtr<Attribute> >& copy)
{
    copy = m_attributes;
}

size_t ElementAttributeData::getAttributeItemIndexSlowCase(const String& name, bool shouldIgnoreAttributeCase) const
{
    unsigned len = length();

    // Continue to checking case-insensitively and/or full namespaced names if necessary:
    for (unsigned i = 0; i < len; ++i) {
        const QualifiedName& attrName = m_attributes[i]->name();
        if (!attrName.hasPrefix()) {
            if (shouldIgnoreAttributeCase && equalIgnoringCase(name, attrName.localName()))
                return i;
        } else {
            // FIXME: Would be faster to do this comparison without calling toString, which
            // generates a temporary string by concatenation. But this branch is only reached
            // if the attribute name has a prefix, which is rare in HTML.
            if (equalPossiblyIgnoringCase(name, attrName.toString(), shouldIgnoreAttributeCase))
                return i;
        }
    }
    return notFound;
}

void ElementAttributeData::setAttributes(const ElementAttributeData& other, Element* element)
{
    ASSERT(element);

    // If assigning the map changes the id attribute, we need to call
    // updateId.
    Attribute* oldId = getAttributeItem(element->document()->idAttributeName());
    Attribute* newId = other.getAttributeItem(element->document()->idAttributeName());

    if (oldId || newId)
        element->updateId(oldId ? oldId->value() : nullAtom, newId ? newId->value() : nullAtom);

    Attribute* oldName = getAttributeItem(HTMLNames::nameAttr);
    Attribute* newName = other.getAttributeItem(HTMLNames::nameAttr);

    if (oldName || newName)
        element->updateName(oldName ? oldName->value() : nullAtom, newName ? newName->value() : nullAtom);

    clearAttributes();
    unsigned newLength = other.length();
    m_attributes.resize(newLength);

    // FIXME: These loops can probably be combined.
    for (unsigned i = 0; i < newLength; i++)
        m_attributes[i] = other.m_attributes[i]->clone();
    for (unsigned i = 0; i < newLength; i++)
        element->attributeChanged(m_attributes[i].get());
}

void ElementAttributeData::clearAttributes()
{
    clearClass();
    detachAttributesFromElement();
    m_attributes.clear();
}

void ElementAttributeData::replaceAttribute(size_t index, PassRefPtr<Attribute> prpAttribute, Element* element)
{
    ASSERT(element);
    ASSERT(index < length());

    RefPtr<Attribute> attribute = prpAttribute;
    Attribute* old = m_attributes[index].get();

    element->willModifyAttribute(attribute->name(), old->value(), attribute->value());

    if (Attr* attr = old->attr())
        attr->m_element = 0;
    m_attributes[index] = attribute;
    if (Attr* attr = attribute->attr())
        attr->m_element = element;

    element->didModifyAttribute(attribute.get());
}

}
