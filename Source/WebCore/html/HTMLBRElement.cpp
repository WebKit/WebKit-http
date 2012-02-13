/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2003, 2006, 2009, 2010 Apple Inc. All rights reserved.
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
#include "HTMLBRElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "HTMLNames.h"
#include "RenderBR.h"

namespace WebCore {

using namespace HTMLNames;

HTMLBRElement::HTMLBRElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(brTag));
}

PassRefPtr<HTMLBRElement> HTMLBRElement::create(Document* document)
{
    return adoptRef(new HTMLBRElement(brTag, document));
}

PassRefPtr<HTMLBRElement> HTMLBRElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLBRElement(tagName, document));
}

bool HTMLBRElement::isPresentationAttribute(Attribute* attr) const
{
    if (attr->name() == clearAttr)
        return true;
    return HTMLElement::isPresentationAttribute(attr);
}

void HTMLBRElement::collectStyleForAttribute(Attribute* attr, StylePropertySet* style)
{
    if (attr->name() == clearAttr) {
        // If the string is empty, then don't add the clear property.
        // <br clear> and <br clear=""> are just treated like <br> by Gecko, Mac IE, etc. -dwh
        if (!attr->isEmpty()) {
            if (equalIgnoringCase(attr->value(), "all"))
                style->setProperty(CSSPropertyClear, "both");
            else
                style->setProperty(CSSPropertyClear, attr->value());
        }
    } else
        HTMLElement::collectStyleForAttribute(attr, style);
}

RenderObject* HTMLBRElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
     if (style->hasContent())
        return RenderObject::createObject(this, style);

     return new (arena) RenderBR(this);
}

}
