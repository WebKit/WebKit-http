/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "HTMLOListElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "HTMLNames.h"
#include "RenderListItem.h"

namespace WebCore {

using namespace HTMLNames;

HTMLOListElement::HTMLOListElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_start(0xBADBEEF)
    , m_itemCount(0)
    , m_hasExplicitStart(false)
    , m_isReversed(false)
    , m_shouldRecalculateItemCount(false)
{
    ASSERT(hasTagName(olTag));
}

PassRefPtr<HTMLOListElement> HTMLOListElement::create(Document* document)
{
    return adoptRef(new HTMLOListElement(olTag, document));
}

PassRefPtr<HTMLOListElement> HTMLOListElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLOListElement(tagName, document));
}

bool HTMLOListElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == typeAttr)
        return true;
    return HTMLElement::isPresentationAttribute(name);
}

void HTMLOListElement::collectStyleForAttribute(const Attribute& attribute, StylePropertySet* style)
{
    if (attribute.name() == typeAttr) {
        if (attribute.value() == "a")
            addPropertyToAttributeStyle(style, CSSPropertyListStyleType, CSSValueLowerAlpha);
        else if (attribute.value() == "A")
            addPropertyToAttributeStyle(style, CSSPropertyListStyleType, CSSValueUpperAlpha);
        else if (attribute.value() == "i")
            addPropertyToAttributeStyle(style, CSSPropertyListStyleType, CSSValueLowerRoman);
        else if (attribute.value() == "I")
            addPropertyToAttributeStyle(style, CSSPropertyListStyleType, CSSValueUpperRoman);
        else if (attribute.value() == "1")
            addPropertyToAttributeStyle(style, CSSPropertyListStyleType, CSSValueDecimal);
    } else
        HTMLElement::collectStyleForAttribute(attribute, style);
}

void HTMLOListElement::parseAttribute(const Attribute& attribute)
{
    if (attribute.name() == startAttr) {
        int oldStart = start();
        bool canParse;
        int parsedStart = attribute.value().toInt(&canParse);
        m_hasExplicitStart = canParse;
        m_start = canParse ? parsedStart : 0xBADBEEF;
        if (oldStart == start())
            return;
        updateItemValues();
    } else if (attribute.name() == reversedAttr) {
        bool reversed = !attribute.isNull();
        if (reversed == m_isReversed)
            return;
        m_isReversed = reversed;
        updateItemValues();
    } else
        HTMLElement::parseAttribute(attribute);
}

void HTMLOListElement::setStart(int start)
{
    setAttribute(startAttr, String::number(start));
}

void HTMLOListElement::updateItemValues()
{
    for (RenderListItem* listItem = RenderListItem::nextListItem(renderer()); listItem; listItem = RenderListItem::nextListItem(renderer(), listItem))
        listItem->updateValue();
}

void HTMLOListElement::recalculateItemCount()
{
    m_itemCount = 0;

    for (RenderListItem* listItem = RenderListItem::nextListItem(renderer()); listItem; listItem = RenderListItem::nextListItem(renderer(), listItem))
        m_itemCount++;

    m_shouldRecalculateItemCount = false;
}

}
