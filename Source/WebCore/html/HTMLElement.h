/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef HTMLElement_h
#define HTMLElement_h

#include "StyledElement.h"

namespace WebCore {

class DocumentFragment;
class HTMLCollection;
class HTMLFormElement;
                       
class HTMLElement : public StyledElement {
public:
    static PassRefPtr<HTMLElement> create(const QualifiedName& tagName, Document*);

    PassRefPtr<HTMLCollection> children();
    
    virtual String title() const;

    virtual short tabIndex() const;
    void setTabIndex(int);

    String innerHTML() const;
    String outerHTML() const;
    // deprecatedCreateContextualFragment logic should be moved into Range::createContextualFragment
    PassRefPtr<DocumentFragment> deprecatedCreateContextualFragment(const String&, FragmentScriptingPermission = FragmentScriptingAllowed);
    void setInnerHTML(const String&, ExceptionCode&);
    void setOuterHTML(const String&, ExceptionCode&);
    void setInnerText(const String&, ExceptionCode&);
    void setOuterText(const String&, ExceptionCode&);

    Element* insertAdjacentElement(const String& where, Element* newChild, ExceptionCode&);
    void insertAdjacentHTML(const String& where, const String& html, ExceptionCode&);
    void insertAdjacentText(const String& where, const String& text, ExceptionCode&);

    virtual bool supportsFocus() const;

    String contentEditable() const;
    void setContentEditable(const String&, ExceptionCode&);

    virtual bool draggable() const;
    void setDraggable(bool);

    bool spellcheck() const;
    void setSpellcheck(bool);

    void click();

    virtual void accessKeyAction(bool sendToAnyElement);

    bool ieForbidsInsertHTML() const;

    virtual bool rendererIsNeeded(const NodeRenderingContext&);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    HTMLFormElement* form() const { return virtualForm(); }

    static void addHTMLAlignmentToStyledElement(StyledElement*, Attribute*);

    HTMLFormElement* findFormAncestor() const;

    TextDirection directionalityIfhasDirAutoAttribute(bool& isAuto) const;

protected:
    HTMLElement(const QualifiedName& tagName, Document*);

    void addHTMLAlignment(Attribute*);

    virtual bool mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const;
    virtual void parseMappedAttribute(Attribute*);

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

private:
    virtual String nodeName() const;

    void setContentEditable(Attribute*);

    virtual HTMLFormElement* virtualForm() const;

    Node* insertAdjacent(const String& where, Node* newChild, ExceptionCode&);
    PassRefPtr<DocumentFragment> textToFragment(const String&, ExceptionCode&);

    void dirAttributeChanged(Attribute*);
    void adjustDirectionalityIfNeededAfterChildAttributeChanged(Element* child);
    void calculateAndAdjustDirectionality();
    void adjustDirectionalityIfNeededAfterChildrenChanged(Node* beforeChange, int childCountDelta);
    TextDirection directionality(Node** strongDirectionalityTextNode= 0) const;
};

inline HTMLElement* toHTMLElement(Node* node)
{
    ASSERT(!node || node->isHTMLElement());
    return static_cast<HTMLElement*>(node);
}

inline const HTMLElement* toHTMLElement(const Node* node)
{
    ASSERT(!node || node->isHTMLElement());
    return static_cast<const HTMLElement*>(node);
}

inline HTMLElement::HTMLElement(const QualifiedName& tagName, Document* document)
    : StyledElement(tagName, document, CreateHTMLElement)
{
    ASSERT(tagName.localName().impl());
}

} // namespace WebCore

#endif // HTMLElement_h
