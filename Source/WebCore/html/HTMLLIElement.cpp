/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "HTMLLIElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "ComposedShadowTreeWalker.h"
#include "HTMLNames.h"
#include "RenderListItem.h"

namespace WebCore {

using namespace HTMLNames;

HTMLLIElement::HTMLLIElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(liTag));
}

PassRefPtr<HTMLLIElement> HTMLLIElement::create(Document* document)
{
    return adoptRef(new HTMLLIElement(liTag, document));
}

PassRefPtr<HTMLLIElement> HTMLLIElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLLIElement(tagName, document));
}

bool HTMLLIElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == typeAttr)
        return true;
    return HTMLElement::isPresentationAttribute(name);
}

void HTMLLIElement::collectStyleForAttribute(const Attribute& attribute, StylePropertySet* style)
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
        else
            addPropertyToAttributeStyle(style, CSSPropertyListStyleType, attribute.value());
    } else
        HTMLElement::collectStyleForAttribute(attribute, style);
}

void HTMLLIElement::parseAttribute(const Attribute& attribute)
{
    if (attribute.name() == valueAttr) {
        if (renderer() && renderer()->isListItem())
            parseValue(attribute.value());
    } else
        HTMLElement::parseAttribute(attribute);
}

void HTMLLIElement::attach()
{
    ASSERT(!attached());

    HTMLElement::attach();

    if (renderer() && renderer()->isListItem()) {
        RenderListItem* render = toRenderListItem(renderer());

        // Find the enclosing list node.
        Node* listNode = 0;
        ComposedShadowTreeParentWalker walker(this);
        while (!listNode) {
            walker.parentIncludingInsertionPointAndShadowRoot();
            if (!walker.get())
                break;
            if (walker.get()->hasTagName(ulTag) || walker.get()->hasTagName(olTag))
                listNode = walker.get();
        }

        // If we are not in a list, tell the renderer so it can position us inside.
        // We don't want to change our style to say "inside" since that would affect nested nodes.
        if (!listNode)
            render->setNotInList(true);

        parseValue(fastGetAttribute(valueAttr));
    }
}

inline void HTMLLIElement::parseValue(const AtomicString& value)
{
    ASSERT(renderer() && renderer()->isListItem());

    bool valueOK;
    int requestedValue = value.toInt(&valueOK);
    if (valueOK)
        toRenderListItem(renderer())->setExplicitValue(requestedValue);
    else
        toRenderListItem(renderer())->clearExplicitValue();
}

}
