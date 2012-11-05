/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010, 2012 Apple Inc. All rights reserved.
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

#include "Attr.h"
#include "CSSParser.h"
#include "CSSStyleSheet.h"
#include "StyledElement.h"
#include "WebCoreMemoryInstrumentation.h"
#include <wtf/MemoryInstrumentationVector.h>

namespace WebCore {

static size_t sizeForImmutableElementAttributeDataWithAttributeCount(unsigned count)
{
    return sizeof(ImmutableElementAttributeData) - sizeof(void*) + sizeof(Attribute) * count;
}

PassRefPtr<ElementAttributeData> ElementAttributeData::createImmutable(const Vector<Attribute>& attributes)
{
    void* slot = WTF::fastMalloc(sizeForImmutableElementAttributeDataWithAttributeCount(attributes.size()));
    return adoptRef(new (slot) ImmutableElementAttributeData(attributes));
}

PassRefPtr<ElementAttributeData> ElementAttributeData::create()
{
    return adoptRef(new MutableElementAttributeData);
}

ImmutableElementAttributeData::ImmutableElementAttributeData(const Vector<Attribute>& attributes)
    : ElementAttributeData(attributes.size())
{
    for (unsigned i = 0; i < m_arraySize; ++i)
        new (&reinterpret_cast<Attribute*>(&m_attributeArray)[i]) Attribute(attributes[i]);
}

MutableElementAttributeData::MutableElementAttributeData(const ImmutableElementAttributeData& other)
{
    const ElementAttributeData& baseOther = static_cast<const ElementAttributeData&>(other);

    m_inlineStyleDecl = baseOther.m_inlineStyleDecl;
    m_attributeStyle = baseOther.m_attributeStyle;
    m_classNames = baseOther.m_classNames;
    m_idForStyleResolution = baseOther.m_idForStyleResolution;

    // An ImmutableElementAttributeData should never have a mutable inline StylePropertySet attached.
    ASSERT(!baseOther.m_inlineStyleDecl || !baseOther.m_inlineStyleDecl->isMutable());

    m_attributeVector.reserveCapacity(baseOther.m_arraySize);
    for (unsigned i = 0; i < baseOther.m_arraySize; ++i)
        m_attributeVector.uncheckedAppend(other.immutableAttributeArray()[i]);
}

ImmutableElementAttributeData::~ImmutableElementAttributeData()
{
    for (unsigned i = 0; i < m_arraySize; ++i)
        (reinterpret_cast<Attribute*>(&m_attributeArray)[i]).~Attribute();
}

PassRefPtr<ElementAttributeData> ElementAttributeData::makeMutableCopy() const
{
    ASSERT(!isMutable());
    return adoptRef(new MutableElementAttributeData(static_cast<const ImmutableElementAttributeData&>(*this)));
}

StylePropertySet* ElementAttributeData::ensureInlineStyle(StyledElement* element)
{
    ASSERT(isMutable());
    if (!m_inlineStyleDecl) {
        ASSERT(element->isStyledElement());
        m_inlineStyleDecl = StylePropertySet::create(strictToCSSParserMode(element->isHTMLElement() && !element->document()->inQuirksMode()));
    }
    return m_inlineStyleDecl.get();
}

StylePropertySet* ElementAttributeData::ensureMutableInlineStyle(StyledElement* element)
{
    ASSERT(isMutable());
    if (m_inlineStyleDecl && !m_inlineStyleDecl->isMutable()) {
        m_inlineStyleDecl = m_inlineStyleDecl->copy();
        return m_inlineStyleDecl.get();
    }
    return ensureInlineStyle(element);
}
    
void ElementAttributeData::updateInlineStyleAvoidingMutation(StyledElement* element, const String& text) const
{
    // We reconstruct the property set instead of mutating if there is no CSSOM wrapper.
    // This makes wrapperless property sets immutable and so cacheable.
    if (m_inlineStyleDecl && !m_inlineStyleDecl->isMutable())
        m_inlineStyleDecl.clear();
    if (!m_inlineStyleDecl)
        m_inlineStyleDecl = CSSParser::parseInlineStyleDeclaration(text, element);
    else
        m_inlineStyleDecl->parseDeclaration(text, element->document()->elementSheet()->contents());
}

void ElementAttributeData::detachCSSOMWrapperIfNeeded(StyledElement* element)
{
    if (m_inlineStyleDecl)
        m_inlineStyleDecl->clearParentElement(element);
}

void ElementAttributeData::destroyInlineStyle(StyledElement* element)
{
    detachCSSOMWrapperIfNeeded(element);
    m_inlineStyleDecl = 0;
}

void ElementAttributeData::addAttribute(const Attribute& attribute)
{
    ASSERT(isMutable());
    mutableAttributeVector().append(attribute);
}

void ElementAttributeData::removeAttribute(size_t index)
{
    ASSERT(isMutable());
    ASSERT(index < length());
    mutableAttributeVector().remove(index);
}

bool ElementAttributeData::isEquivalent(const ElementAttributeData* other) const
{
    if (!other)
        return isEmpty();

    unsigned len = length();
    if (len != other->length())
        return false;

    for (unsigned i = 0; i < len; i++) {
        const Attribute* attribute = attributeItem(i);
        const Attribute* otherAttr = other->getAttributeItem(attribute->name());
        if (!otherAttr || attribute->value() != otherAttr->value())
            return false;
    }

    return true;
}

void ElementAttributeData::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    size_t actualSize = m_isMutable ? sizeof(ElementAttributeData) : sizeForImmutableElementAttributeDataWithAttributeCount(m_arraySize);
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::DOM, actualSize);
    info.addMember(m_inlineStyleDecl);
    info.addMember(m_attributeStyle);
    info.addMember(m_classNames);
    info.addMember(m_idForStyleResolution);
    if (m_isMutable)
        info.addMember(mutableAttributeVector());
    for (unsigned i = 0, len = length(); i < len; i++)
        info.addMember(*attributeItem(i));
}

size_t ElementAttributeData::getAttributeItemIndexSlowCase(const AtomicString& name, bool shouldIgnoreAttributeCase) const
{
    // Continue to checking case-insensitively and/or full namespaced names if necessary:
    for (unsigned i = 0; i < length(); ++i) {
        const Attribute* attribute = attributeItem(i);
        if (!attribute->name().hasPrefix()) {
            if (shouldIgnoreAttributeCase && equalIgnoringCase(name, attribute->localName()))
                return i;
        } else {
            // FIXME: Would be faster to do this comparison without calling toString, which
            // generates a temporary string by concatenation. But this branch is only reached
            // if the attribute name has a prefix, which is rare in HTML.
            if (equalPossiblyIgnoringCase(name, attribute->name().toString(), shouldIgnoreAttributeCase))
                return i;
        }
    }
    return notFound;
}

void ElementAttributeData::cloneDataFrom(const ElementAttributeData& sourceData, const Element& sourceElement, Element& targetElement)
{
    // FIXME: Cloned elements could start out with immutable attribute data.
    ASSERT(isMutable());

    const AtomicString& oldID = targetElement.getIdAttribute();
    const AtomicString& newID = sourceElement.getIdAttribute();

    if (!oldID.isNull() || !newID.isNull())
        targetElement.updateId(oldID, newID);

    const AtomicString& oldName = targetElement.getNameAttribute();
    const AtomicString& newName = sourceElement.getNameAttribute();

    if (!oldName.isNull() || !newName.isNull())
        targetElement.updateName(oldName, newName);

    clearAttributes();

    if (sourceData.isMutable())
        mutableAttributeVector() = sourceData.mutableAttributeVector();
    else {
        mutableAttributeVector().reserveInitialCapacity(sourceData.m_arraySize);
        for (unsigned i = 0; i < sourceData.m_arraySize; ++i)
            mutableAttributeVector().uncheckedAppend(sourceData.immutableAttributeArray()[i]);
    }

    for (unsigned i = 0; i < length(); ++i) {
        const Attribute& attribute = mutableAttributeVector().at(i);
        if (targetElement.isStyledElement() && attribute.name() == HTMLNames::styleAttr) {
            static_cast<StyledElement&>(targetElement).styleAttributeChanged(attribute.value(), StyledElement::DoNotReparseStyleAttribute);
            continue;
        }
        targetElement.attributeChanged(attribute.name(), attribute.value());
    }

    if (targetElement.isStyledElement() && sourceData.m_inlineStyleDecl) {
        m_inlineStyleDecl = sourceData.m_inlineStyleDecl->immutableCopyIfNeeded();
        targetElement.setIsStyleAttributeValid(sourceElement.isStyleAttributeValid());
    }
}

void ElementAttributeData::clearAttributes()
{
    ASSERT(isMutable());
    clearClass();
    mutableAttributeVector().clear();
}

}
