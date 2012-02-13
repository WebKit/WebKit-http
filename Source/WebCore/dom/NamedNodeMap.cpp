/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *           (C) 2007 Eric Seidel (eric@webkit.org)
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
#include "NamedNodeMap.h"

#include "Attr.h"
#include "Document.h"
#include "Element.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

static inline bool shouldIgnoreAttributeCase(const Element* e)
{
    return e && e->document()->isHTMLDocument() && e->isHTMLElement();
}

void NamedNodeMap::ref()
{
    ASSERT(m_element);
    m_element->ref();
}

void NamedNodeMap::deref()
{
    ASSERT(m_element);
    m_element->deref();
}

PassRefPtr<Node> NamedNodeMap::getNamedItem(const String& name) const
{
    Attribute* a = m_attributeData.getAttributeItem(name, shouldIgnoreAttributeCase(m_element));
    if (!a)
        return 0;
    
    return a->createAttrIfNeeded(m_element);
}

PassRefPtr<Node> NamedNodeMap::getNamedItemNS(const String& namespaceURI, const String& localName) const
{
    return getNamedItem(QualifiedName(nullAtom, localName, namespaceURI));
}

PassRefPtr<Node> NamedNodeMap::removeNamedItem(const String& name, ExceptionCode& ec)
{
    Attribute* a = m_attributeData.getAttributeItem(name, shouldIgnoreAttributeCase(m_element));
    if (!a) {
        ec = NOT_FOUND_ERR;
        return 0;
    }
    
    return removeNamedItem(a->name(), ec);
}

PassRefPtr<Node> NamedNodeMap::removeNamedItemNS(const String& namespaceURI, const String& localName, ExceptionCode& ec)
{
    return removeNamedItem(QualifiedName(nullAtom, localName, namespaceURI), ec);
}

PassRefPtr<Node> NamedNodeMap::getNamedItem(const QualifiedName& name) const
{
    Attribute* a = getAttributeItem(name);
    if (!a)
        return 0;

    return a->createAttrIfNeeded(m_element);
}

PassRefPtr<Node> NamedNodeMap::setNamedItem(Node* node, ExceptionCode& ec)
{
    if (!m_element || !node) {
        ec = NOT_FOUND_ERR;
        return 0;
    }

    // Not mentioned in spec: throw a HIERARCHY_REQUEST_ERROR if the user passes in a non-attribute node
    if (!node->isAttributeNode()) {
        ec = HIERARCHY_REQUEST_ERR;
        return 0;
    }
    Attr* attr = static_cast<Attr*>(node);

    Attribute* attribute = attr->attr();
    size_t index = getAttributeItemIndex(attribute->name());
    Attribute* oldAttribute = index != notFound ? attributeItem(index) : 0;
    if (oldAttribute == attribute)
        return node; // we know about it already

    // INUSE_ATTRIBUTE_ERR: Raised if node is an Attr that is already an attribute of another Element object.
    // The DOM user must explicitly clone Attr nodes to re-use them in other elements.
    if (attr->ownerElement()) {
        ec = INUSE_ATTRIBUTE_ERR;
        return 0;
    }

    RefPtr<Attr> oldAttr;
    if (oldAttribute) {
        oldAttr = oldAttribute->createAttrIfNeeded(m_element);
        m_attributeData.replaceAttribute(index, attribute, m_element);
    } else
        m_attributeData.addAttribute(attribute, m_element);

    return oldAttr.release();
}

PassRefPtr<Node> NamedNodeMap::setNamedItemNS(Node* node, ExceptionCode& ec)
{
    return setNamedItem(node, ec);
}

PassRefPtr<Node> NamedNodeMap::removeNamedItem(const QualifiedName& name, ExceptionCode& ec)
{
    ASSERT(m_element);

    size_t index = getAttributeItemIndex(name);
    if (index == notFound) {
        ec = NOT_FOUND_ERR;
        return 0;
    }

    RefPtr<Attr> attr = m_attributeData.m_attributes[index]->createAttrIfNeeded(m_element);

    removeAttribute(index);

    return attr.release();
}

PassRefPtr<Node> NamedNodeMap::item(unsigned index) const
{
    if (index >= length())
        return 0;

    return m_attributeData.m_attributes[index]->createAttrIfNeeded(m_element);
}

void NamedNodeMap::detachFromElement()
{
    // This can't happen if the holder of the map is JavaScript, because we mark the
    // element if the map is alive. So it has no impact on web page behavior. Because
    // of that, we can simply clear all the attributes to avoid accessing stale
    // pointers to do things like create Attr objects.
    m_element = 0;
    m_attributeData.clearAttributes();
}

bool NamedNodeMap::mapsEquivalent(const NamedNodeMap* otherMap) const
{
    if (!otherMap)
        return isEmpty();
    
    unsigned len = length();
    if (len != otherMap->length())
        return false;
    
    for (unsigned i = 0; i < len; i++) {
        Attribute* attr = attributeItem(i);
        Attribute* otherAttr = otherMap->getAttributeItem(attr->name());
        if (!otherAttr || attr->value() != otherAttr->value())
            return false;
    }
    
    return true;
}

} // namespace WebCore
