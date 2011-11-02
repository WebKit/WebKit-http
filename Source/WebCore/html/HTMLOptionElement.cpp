/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Motorola Mobility, Inc.  All rights reserved.
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
#include "HTMLOptionElement.h"

#include "Attribute.h"
#include "CSSStyleSelector.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLSelectElement.h"
#include "NodeRenderStyle.h"
#include "NodeRenderingContext.h"
#include "RenderMenuList.h"
#include "ScriptElement.h"
#include "Text.h"
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

HTMLOptionElement::HTMLOptionElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLFormControlElement(tagName, document, form)
    , m_isSelected(false)
{
    ASSERT(hasTagName(optionTag));
}

PassRefPtr<HTMLOptionElement> HTMLOptionElement::create(Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLOptionElement(optionTag, document, form));
}

PassRefPtr<HTMLOptionElement> HTMLOptionElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLOptionElement(tagName, document, form));
}

PassRefPtr<HTMLOptionElement> HTMLOptionElement::createForJSConstructor(Document* document, const String& data, const String& value,
        bool defaultSelected, bool selected, ExceptionCode& ec)
{
    RefPtr<HTMLOptionElement> element = adoptRef(new HTMLOptionElement(optionTag, document));

    RefPtr<Text> text = Text::create(document, data.isNull() ? "" : data);

    ec = 0;
    element->appendChild(text.release(), ec);
    if (ec)
        return 0;

    if (!value.isNull())
        element->setValue(value);
    if (defaultSelected)
        element->setAttribute(selectedAttr, emptyAtom);
    element->setSelected(selected);

    return element.release();
}

void HTMLOptionElement::attach()
{
    if (parentNode()->renderStyle())
        setRenderStyle(styleForRenderer());
    HTMLFormControlElement::attach();
}

void HTMLOptionElement::detach()
{
    m_style.clear();
    HTMLFormControlElement::detach();
}

bool HTMLOptionElement::supportsFocus() const
{
    return HTMLElement::supportsFocus();
}

bool HTMLOptionElement::isFocusable() const
{
    // Option elements do not have a renderer so we check the renderStyle instead.
    return supportsFocus() && renderStyle() && renderStyle()->display() != NONE;
}

const AtomicString& HTMLOptionElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, option, ("option"));
    return option;
}

String HTMLOptionElement::text() const
{
    Document* document = this->document();
    String text;

    // WinIE does not use the label attribute, so as a quirk, we ignore it.
    if (!document->inQuirksMode())
        text = fastGetAttribute(labelAttr);

    // FIXME: The following treats an element with the label attribute set to
    // the empty string the same as an element with no label attribute at all.
    // Is that correct? If it is, then should the label function work the same way?
    if (text.isEmpty())
        text = collectOptionInnerText();

    // FIXME: Is displayStringModifiedByEncoding helpful here?
    // If it's correct here, then isn't it needed in the value and label functions too?
    return document->displayStringModifiedByEncoding(text).stripWhiteSpace(isHTMLSpace).simplifyWhiteSpace(isHTMLSpace);
}

void HTMLOptionElement::setText(const String &text, ExceptionCode& ec)
{
    // Changing the text causes a recalc of a select's items, which will reset the selected
    // index to the first item if the select is single selection with a menu list. We attempt to
    // preserve the selected item.
    HTMLSelectElement* select = ownerSelectElement();
    bool selectIsMenuList = select && select->usesMenuList();
    int oldSelectedIndex = selectIsMenuList ? select->selectedIndex() : -1;

    // Handle the common special case where there's exactly 1 child node, and it's a text node.
    Node* child = firstChild();
    if (child && child->isTextNode() && !child->nextSibling())
        static_cast<Text *>(child)->setData(text, ec);
    else {
        removeChildren();
        appendChild(Text::create(document(), text), ec);
    }
    
    if (selectIsMenuList && select->selectedIndex() != oldSelectedIndex)
        select->setSelectedIndex(oldSelectedIndex);
}

void HTMLOptionElement::accessKeyAction(bool)
{
    HTMLSelectElement* select = ownerSelectElement();
    if (select)
        select->accessKeySetSelectedIndex(index());
}

int HTMLOptionElement::index() const
{
    // It would be faster to cache the index, but harder to get it right in all cases.

    HTMLSelectElement* selectElement = ownerSelectElement();
    if (!selectElement)
        return 0;

    int optionIndex = 0;

    const Vector<HTMLElement*>& items = selectElement->listItems();
    size_t length = items.size();
    for (size_t i = 0; i < length; ++i) {
        if (!items[i]->hasTagName(optionTag))
            continue;
        if (items[i] == this)
            return optionIndex;
        ++optionIndex;
    }

    return 0;
}

void HTMLOptionElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == selectedAttr) {
        // FIXME: This doesn't match what the HTML specification says.
        // The specification implies that removing the selected attribute or
        // changing the value of a selected attribute that is already present
        // has no effect on whether the element is selected. Further, it seems
        // that we need to do more than just set m_isSelected to select in that
        // case; we'd need to do the other work from the setSelected function.
        m_isSelected = !attr->isNull();
    } else
        HTMLFormControlElement::parseMappedAttribute(attr);
}

String HTMLOptionElement::value() const
{
    const AtomicString& value = fastGetAttribute(valueAttr);
    if (!value.isNull())
        return value;
    return collectOptionInnerText().stripWhiteSpace(isHTMLSpace).simplifyWhiteSpace(isHTMLSpace);
}

void HTMLOptionElement::setValue(const String& value)
{
    setAttribute(valueAttr, value);
}

bool HTMLOptionElement::selected()
{
    if (HTMLSelectElement* select = ownerSelectElement())
        select->updateListItemSelectedStates();
    return m_isSelected;
}

void HTMLOptionElement::setSelected(bool selected)
{
    if (m_isSelected == selected)
        return;

    setSelectedState(selected);

    if (HTMLSelectElement* select = ownerSelectElement())
        select->optionSelectionStateChanged(this, selected);
}

void HTMLOptionElement::setSelectedState(bool selected)
{
    if (m_isSelected == selected)
        return;

    m_isSelected = selected;
    setNeedsStyleRecalc();
}

void HTMLOptionElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    if (HTMLSelectElement* select = ownerSelectElement())
        select->optionElementChildrenChanged();
    HTMLFormControlElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}

HTMLSelectElement* HTMLOptionElement::ownerSelectElement() const
{
    ContainerNode* select = parentNode();
    while (select && !select->hasTagName(selectTag))
        select = select->parentNode();

    if (!select)
        return 0;

    return toHTMLSelectElement(select);
}

String HTMLOptionElement::label() const
{
    const AtomicString& label = fastGetAttribute(labelAttr);
    if (!label.isNull())
        return label; 
    return collectOptionInnerText().stripWhiteSpace(isHTMLSpace).simplifyWhiteSpace(isHTMLSpace);
}

void HTMLOptionElement::setLabel(const String& label)
{
    setAttribute(labelAttr, label);
}

void HTMLOptionElement::setRenderStyle(PassRefPtr<RenderStyle> newStyle)
{
    m_style = newStyle;
    if (HTMLSelectElement* select = ownerSelectElement()) {
        if (RenderObject* renderer = select->renderer())
            renderer->repaint();
    }
}

RenderStyle* HTMLOptionElement::nonRendererRenderStyle() const
{
    return m_style.get();
}

String HTMLOptionElement::textIndentedToRespectGroupLabel() const
{
    ContainerNode* parent = parentNode();
    if (parent && parent->hasTagName(optgroupTag))
        return "    " + text();
    return text();
}

bool HTMLOptionElement::disabled() const
{
    return ownElementDisabled() || (parentNode() && static_cast<HTMLFormControlElement*>(parentNode())->disabled());
}

void HTMLOptionElement::insertedIntoTree(bool deep)
{
    if (HTMLSelectElement* select = ownerSelectElement()) {
        select->setRecalcListItems();
        // Do not call selected() since calling updateListItemSelectedStates()
        // at this time won't do the right thing. (Why, exactly?)
        // FIXME: Might be better to call this unconditionally, always passing m_isSelected,
        // rather than only calling it if we are selected.
        if (m_isSelected)
            select->optionSelectionStateChanged(this, true);
        select->scrollToSelection();
    }

    HTMLFormControlElement::insertedIntoTree(deep);
}

String HTMLOptionElement::collectOptionInnerText() const
{
    StringBuilder text;
    for (Node* node = firstChild(); node; ) {
        if (node->isTextNode())
            text.append(node->nodeValue());
        // Text nodes inside script elements are not part of the option text.
        if (node->isElementNode() && toScriptElement(toElement(node)))
            node = node->traverseNextSibling(this);
        else
            node = node->traverseNextNode(this);
    }
    return text.toString();
}

#ifndef NDEBUG

HTMLOptionElement* toHTMLOptionElement(Node* node)
{
    ASSERT(!node || node->hasTagName(optionTag));
    return static_cast<HTMLOptionElement*>(node);
}

const HTMLOptionElement* toHTMLOptionElement(const Node* node)
{
    ASSERT(!node || node->hasTagName(optionTag));
    return static_cast<const HTMLOptionElement*>(node);
}

#endif

} // namespace
