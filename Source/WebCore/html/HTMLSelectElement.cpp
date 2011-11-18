/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "HTMLSelectElement.h"

#include "AXObjectCache.h"
#include "Attribute.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "EventNames.h"
#include "FormDataList.h"
#include "Frame.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLOptionsCollection.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderListBox.h"
#include "RenderMenuList.h"
#include "ScriptEventListener.h"
#include "SpatialNavigation.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/Unicode.h>

using namespace std;
using namespace WTF::Unicode;

namespace WebCore {

using namespace HTMLNames;

// Upper limit agreed upon with representatives of Opera and Mozilla.
static const unsigned maxSelectItems = 10000;

// Configure platform-specific behavior when focused pop-up receives arrow/space/return keystroke.
// (PLATFORM(MAC) and PLATFORM(GTK) are always false in Chromium, hence the extra tests.)
#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))
#define ARROW_KEYS_POP_MENU 1
#define SPACE_OR_RETURN_POP_MENU 0
#elif PLATFORM(GTK) || (PLATFORM(CHROMIUM) && OS(UNIX))
#define ARROW_KEYS_POP_MENU 0
#define SPACE_OR_RETURN_POP_MENU 1
#else
#define ARROW_KEYS_POP_MENU 0
#define SPACE_OR_RETURN_POP_MENU 0
#endif

static const DOMTimeStamp typeAheadTimeout = 1000;

HTMLSelectElement::HTMLSelectElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLFormControlElementWithState(tagName, document, form)
    , m_lastCharTime(0)
    , m_size(0)
    , m_lastOnChangeIndex(-1)
    , m_activeSelectionAnchorIndex(-1)
    , m_activeSelectionEndIndex(-1)
    , m_repeatingChar(0)
    , m_isProcessingUserDrivenChange(false)
    , m_multiple(false)
    , m_activeSelectionState(false)
    , m_shouldRecalcListItems(false)
{
    ASSERT(hasTagName(selectTag));
}

PassRefPtr<HTMLSelectElement> HTMLSelectElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    ASSERT(tagName.matches(selectTag));
    return adoptRef(new HTMLSelectElement(tagName, document, form));
}

const AtomicString& HTMLSelectElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, selectMultiple, ("select-multiple"));
    DEFINE_STATIC_LOCAL(const AtomicString, selectOne, ("select-one"));
    return m_multiple ? selectMultiple : selectOne;
}

void HTMLSelectElement::deselectItems(HTMLOptionElement* excludeElement)
{
    deselectItemsWithoutValidation(excludeElement);
    setNeedsValidityCheck();
}

void HTMLSelectElement::optionSelectedByUser(int optionIndex, bool fireOnChangeNow, bool allowMultipleSelection)
{
    // User interaction such as mousedown events can cause list box select elements to send change events.
    // This produces that same behavior for changes triggered by other code running on behalf of the user.
    if (!usesMenuList()) {
        updateSelectedState(optionIndex, allowMultipleSelection, false);
        setNeedsValidityCheck();
        if (fireOnChangeNow)
            listBoxOnChange();
        return;
    }

    // Bail out if this index is already the selected one, to avoid running unnecessary JavaScript that can mess up
    // autofill when there is no actual change (see https://bugs.webkit.org/show_bug.cgi?id=35256 and <rdar://7467917>).
    // The selectOption function does not behave this way, possibly because other callers need a change event even
    // in cases where the selected option is not change.
    if (optionIndex == selectedIndex())
        return;

    selectOption(optionIndex, DeselectOtherOptions | (fireOnChangeNow ? DispatchChangeEvent : 0) | UserDriven);
}

bool HTMLSelectElement::hasPlaceholderLabelOption() const
{
    // The select element has no placeholder label option if it has an attribute "multiple" specified or a display size of non-1.
    // 
    // The condition "size() > 1" is not compliant with the HTML5 spec as of Dec 3, 2010. "size() != 1" is correct.
    // Using "size() > 1" here because size() may be 0 in WebKit.
    // See the discussion at https://bugs.webkit.org/show_bug.cgi?id=43887
    //
    // "0 size()" happens when an attribute "size" is absent or an invalid size attribute is specified.
    // In this case, the display size should be assumed as the default.
    // The default display size is 1 for non-multiple select elements, and 4 for multiple select elements.
    //
    // Finally, if size() == 0 and non-multiple, the display size can be assumed as 1.
    if (multiple() || size() > 1)
        return false;

    int listIndex = optionToListIndex(0);
    ASSERT(listIndex >= 0);
    if (listIndex < 0)
        return false;
    HTMLOptionElement* option = static_cast<HTMLOptionElement*>(listItems()[listIndex]);
    return !listIndex && option->value().isEmpty();
}

bool HTMLSelectElement::valueMissing() const
{
    if (!isRequiredFormControl())
        return false;

    int firstSelectionIndex = selectedIndex();

    // If a non-placeholer label option is selected (firstSelectionIndex > 0), it's not value-missing.
    return firstSelectionIndex < 0 || (!firstSelectionIndex && hasPlaceholderLabelOption());
}

#if ENABLE(NO_LISTBOX_RENDERING)
void HTMLSelectElement::listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow)
{
    if (!multiple())
        optionSelectedByUser(listToOptionIndex(listIndex), fireOnChangeNow, false);
    else {
        updateSelectedState(listIndex, allowMultiplySelections, shift);
        setNeedsValidityCheck();
        if (fireOnChangeNow)
            listBoxOnChange();
    }
}
#endif

int HTMLSelectElement::activeSelectionStartListIndex() const
{
    if (m_activeSelectionAnchorIndex >= 0)
        return m_activeSelectionAnchorIndex;
    return optionToListIndex(selectedIndex());
}

int HTMLSelectElement::activeSelectionEndListIndex() const
{
    if (m_activeSelectionEndIndex >= 0)
        return m_activeSelectionEndIndex;
    return lastSelectedListIndex();
}

void HTMLSelectElement::add(HTMLElement* element, HTMLElement* before, ExceptionCode& ec)
{
    // Make sure the element is ref'd and deref'd so we don't leak it.
    RefPtr<HTMLElement> protectNewChild(element);

    if (!element || !(element->hasLocalName(optionTag) || element->hasLocalName(hrTag)))
        return;

    insertBefore(element, before, ec);
    setNeedsValidityCheck();
}

void HTMLSelectElement::remove(int optionIndex)
{
    int listIndex = optionToListIndex(optionIndex);
    if (listIndex < 0)
        return;

    ExceptionCode ec;
    listItems()[listIndex]->remove(ec);
}

void HTMLSelectElement::remove(HTMLOptionElement* option)
{
    if (option->ownerSelectElement() != this)
        return;

    ExceptionCode ec;
    option->remove(ec);
}

String HTMLSelectElement::value() const
{
    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); i++) {
        if (items[i]->hasLocalName(optionTag) && static_cast<HTMLOptionElement*>(items[i])->selected())
            return static_cast<HTMLOptionElement*>(items[i])->value();
    }
    return "";
}

void HTMLSelectElement::setValue(const String &value)
{
    if (value.isNull())
        return;
    // find the option with value() matching the given parameter
    // and make it the current selection.
    const Vector<HTMLElement*>& items = listItems();
    unsigned optionIndex = 0;
    for (unsigned i = 0; i < items.size(); i++) {
        if (items[i]->hasLocalName(optionTag)) {
            if (static_cast<HTMLOptionElement*>(items[i])->value() == value) {
                setSelectedIndex(optionIndex);
                return;
            }
            optionIndex++;
        }
    }
}

void HTMLSelectElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == sizeAttr) {
        int oldSize = m_size;
        // Set the attribute value to a number.
        // This is important since the style rules for this attribute can determine the appearance property.
        int size = attr->value().toInt();
        String attrSize = String::number(size);
        if (attrSize != attr->value())
            attr->setValue(attrSize);
        size = max(size, 1);

        // Ensure that we've determined selectedness of the items at least once prior to changing the size.
        if (oldSize != size)
            updateListItemSelectedStates();

        m_size = size;
        setNeedsValidityCheck();
        if (m_size != oldSize && attached()) {
            reattach();
            setRecalcListItems();
        }
    } else if (attr->name() == multipleAttr)
        parseMultipleAttribute(attr);
    else if (attr->name() == accesskeyAttr) {
        // FIXME: ignore for the moment.
    } else if (attr->name() == alignAttr) {
        // Don't map 'align' attribute. This matches what Firefox, Opera and IE do.
        // See http://bugs.webkit.org/show_bug.cgi?id=12072
    } else if (attr->name() == onchangeAttr)
        setAttributeEventListener(eventNames().changeEvent, createAttributeEventListener(this, attr));
    else
        HTMLFormControlElementWithState::parseMappedAttribute(attr);
}

bool HTMLSelectElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    if (renderer())
        return isFocusable();
    return HTMLFormControlElementWithState::isKeyboardFocusable(event);
}

bool HTMLSelectElement::isMouseFocusable() const
{
    if (renderer())
        return isFocusable();
    return HTMLFormControlElementWithState::isMouseFocusable();
}

bool HTMLSelectElement::canSelectAll() const
{
    return !usesMenuList();
}

RenderObject* HTMLSelectElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    if (usesMenuList())
        return new (arena) RenderMenuList(this);
    return new (arena) RenderListBox(this);
}

PassRefPtr<HTMLOptionsCollection> HTMLSelectElement::options()
{
    return HTMLOptionsCollection::create(this);
}

void HTMLSelectElement::updateListItemSelectedStates()
{
    if (m_shouldRecalcListItems)
        recalcListItems();
}

void HTMLSelectElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    setRecalcListItems();
    setNeedsValidityCheck();

    HTMLFormControlElementWithState::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    
    if (AXObjectCache::accessibilityEnabled() && renderer())
        renderer()->document()->axObjectCache()->childrenChanged(renderer());
}

void HTMLSelectElement::optionElementChildrenChanged()
{
    setRecalcListItems();
    setNeedsValidityCheck();

    if (AXObjectCache::accessibilityEnabled() && renderer())
        renderer()->document()->axObjectCache()->childrenChanged(renderer());
}

void HTMLSelectElement::accessKeyAction(bool sendMouseEvents)
{
    focus();
    dispatchSimulatedClick(0, sendMouseEvents);
}

void HTMLSelectElement::setMultiple(bool multiple)
{
    bool oldMultiple = this->multiple();
    int oldSelectedIndex = selectedIndex();
    setAttribute(multipleAttr, multiple ? "" : 0);

    // Restore selectedIndex after changing the multiple flag to preserve
    // selection as single-line and multi-line has different defaults.
    if (oldMultiple != this->multiple())
        setSelectedIndex(oldSelectedIndex);
}

void HTMLSelectElement::setSize(int size)
{
    setAttribute(sizeAttr, String::number(size));
}

Node* HTMLSelectElement::namedItem(const AtomicString& name)
{
    return options()->namedItem(name);
}

Node* HTMLSelectElement::item(unsigned index)
{
    return options()->item(index);
}

void HTMLSelectElement::setOption(unsigned index, HTMLOptionElement* option, ExceptionCode& ec)
{
    ec = 0;
    if (index > maxSelectItems - 1)
        index = maxSelectItems - 1;
    int diff = index - length();
    HTMLElement* before = 0;
    // Out of array bounds? First insert empty dummies.
    if (diff > 0) {
        setLength(index, ec);
        // Replace an existing entry?
    } else if (diff < 0) {
        before = toHTMLElement(options()->item(index+1));
        remove(index);
    }
    // Finally add the new element.
    if (!ec) {
        add(option, before, ec);
        if (diff >= 0 && option->selected())
            optionSelectionStateChanged(option, true);
    }
}

void HTMLSelectElement::setLength(unsigned newLen, ExceptionCode& ec)
{
    ec = 0;
    if (newLen > maxSelectItems)
        newLen = maxSelectItems;
    int diff = length() - newLen;

    if (diff < 0) { // Add dummy elements.
        do {
            RefPtr<Element> option = document()->createElement(optionTag, false);
            ASSERT(option);
            add(toHTMLElement(option.get()), 0, ec);
            if (ec)
                break;
        } while (++diff);
    } else {
        const Vector<HTMLElement*>& items = listItems();

        // Removing children fires mutation events, which might mutate the DOM further, so we first copy out a list
        // of elements that we intend to remove then attempt to remove them one at a time.
        Vector<RefPtr<Element> > itemsToRemove;
        size_t optionIndex = 0;
        for (size_t i = 0; i < items.size(); ++i) {
            Element* item = items[i];
            if (item->hasLocalName(optionTag) && optionIndex++ >= newLen) {
                ASSERT(item->parentNode());
                itemsToRemove.append(item);
            }
        }

        for (size_t i = 0; i < itemsToRemove.size(); ++i) {
            Element* item = itemsToRemove[i].get();
            if (item->parentNode())
                item->parentNode()->removeChild(item, ec);
        }
    }
    setNeedsValidityCheck();
}

bool HTMLSelectElement::isRequiredFormControl() const
{
    return required();
}

// Returns the 1st valid item |skip| items from |listIndex| in direction |direction| if there is one.
// Otherwise, it returns the valid item closest to that boundary which is past |listIndex| if there is one.
// Otherwise, it returns |listIndex|.
// Valid means that it is enabled and an option element.
int HTMLSelectElement::nextValidIndex(int listIndex, SkipDirection direction, int skip) const
{
    ASSERT(direction == -1 || direction == 1);
    const Vector<HTMLElement*>& listItems = this->listItems();
    int lastGoodIndex = listIndex;
    int size = listItems.size();
    for (listIndex += direction; listIndex >= 0 && listIndex < size; listIndex += direction) {
        --skip;
        if (!listItems[listIndex]->disabled() && listItems[listIndex]->hasTagName(optionTag)) {
            lastGoodIndex = listIndex;
            if (skip <= 0)
                break;
        }
    }
    return lastGoodIndex;
}

int HTMLSelectElement::nextSelectableListIndex(int startIndex) const
{
    return nextValidIndex(startIndex, SkipForwards, 1);
}

int HTMLSelectElement::previousSelectableListIndex(int startIndex) const
{
    if (startIndex == -1)
        startIndex = listItems().size();
    return nextValidIndex(startIndex, SkipBackwards, 1);
}

int HTMLSelectElement::firstSelectableListIndex() const
{
    const Vector<HTMLElement*>& items = listItems();
    int index = nextValidIndex(items.size(), SkipBackwards, INT_MAX);
    if (static_cast<size_t>(index) == items.size())
        return -1;
    return index;
}

int HTMLSelectElement::lastSelectableListIndex() const
{
    return nextValidIndex(-1, SkipForwards, INT_MAX);
}

// Returns the index of the next valid item one page away from |startIndex| in direction |direction|.
int HTMLSelectElement::nextSelectableListIndexPageAway(int startIndex, SkipDirection direction) const
{
    const Vector<HTMLElement*>& items = listItems();
    // Can't use m_size because renderer forces a minimum size.
    int pageSize = 0;
    if (renderer()->isListBox())
        pageSize = toRenderListBox(renderer())->size() - 1; // -1 so we still show context.

    // One page away, but not outside valid bounds.
    // If there is a valid option item one page away, the index is chosen.
    // If there is no exact one page away valid option, returns startIndex or the most far index.
    int edgeIndex = (direction == SkipForwards) ? 0 : (items.size() - 1);
    int skipAmount = pageSize + ((direction == SkipForwards) ? startIndex : (edgeIndex - startIndex));
    return nextValidIndex(edgeIndex, direction, skipAmount);
}

void HTMLSelectElement::selectAll()
{
    ASSERT(!usesMenuList());
    if (!renderer() || !m_multiple)
        return;

    // Save the selection so it can be compared to the new selectAll selection
    // when dispatching change events.
    saveLastSelection();

    m_activeSelectionState = true;
    setActiveSelectionAnchorIndex(nextSelectableListIndex(-1));
    setActiveSelectionEndIndex(previousSelectableListIndex(-1));

    updateListBoxSelection(false);
    listBoxOnChange();
    setNeedsValidityCheck();
}

void HTMLSelectElement::saveLastSelection()
{
    if (usesMenuList()) {
        m_lastOnChangeIndex = selectedIndex();
        return;
    }

    m_lastOnChangeSelection.clear();
    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        m_lastOnChangeSelection.append(element->hasTagName(optionTag) && toHTMLOptionElement(element)->selected());
    }
}

void HTMLSelectElement::setActiveSelectionAnchorIndex(int index)
{
    m_activeSelectionAnchorIndex = index;

    // Cache the selection state so we can restore the old selection as the new
    // selection pivots around this anchor index.
    m_cachedStateForActiveSelection.clear();

    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        m_cachedStateForActiveSelection.append(element->hasTagName(optionTag) && toHTMLOptionElement(element)->selected());
    }
}

void HTMLSelectElement::setActiveSelectionEndIndex(int index)
{
    m_activeSelectionEndIndex = index;
}

void HTMLSelectElement::updateListBoxSelection(bool deselectOtherOptions)
{
    ASSERT(renderer() && (renderer()->isListBox() || m_multiple));
    ASSERT(!listItems().size() || m_activeSelectionAnchorIndex >= 0);

    unsigned start = min(m_activeSelectionAnchorIndex, m_activeSelectionEndIndex);
    unsigned end = max(m_activeSelectionAnchorIndex, m_activeSelectionEndIndex);

    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        if (!element->hasTagName(optionTag) || toHTMLOptionElement(element)->disabled())
            continue;

        if (i >= start && i <= end)
            toHTMLOptionElement(element)->setSelectedState(m_activeSelectionState);
        else if (deselectOtherOptions || i >= m_cachedStateForActiveSelection.size())
            toHTMLOptionElement(element)->setSelectedState(false);
        else
            toHTMLOptionElement(element)->setSelectedState(m_cachedStateForActiveSelection[i]);
    }

    scrollToSelection();
    setNeedsValidityCheck();
}

void HTMLSelectElement::listBoxOnChange()
{
    ASSERT(!usesMenuList() || m_multiple);

    const Vector<HTMLElement*>& items = listItems();

    // If the cached selection list is empty, or the size has changed, then fire
    // dispatchFormControlChangeEvent, and return early.
    if (m_lastOnChangeSelection.isEmpty() || m_lastOnChangeSelection.size() != items.size()) {
        dispatchFormControlChangeEvent();
        return;
    }

    // Update m_lastOnChangeSelection and fire dispatchFormControlChangeEvent.
    bool fireOnChange = false;
    for (unsigned i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        bool selected = element->hasTagName(optionTag) && toHTMLOptionElement(element)->selected();
        if (selected != m_lastOnChangeSelection[i])
            fireOnChange = true;
        m_lastOnChangeSelection[i] = selected;
    }

    if (fireOnChange)
        dispatchFormControlChangeEvent();
}

void HTMLSelectElement::dispatchChangeEventForMenuList()
{
    ASSERT(usesMenuList());

    int selected = selectedIndex();
    if (m_lastOnChangeIndex != selected && m_isProcessingUserDrivenChange) {
        m_lastOnChangeIndex = selected;
        m_isProcessingUserDrivenChange = false;
        dispatchFormControlChangeEvent();
    }
}

void HTMLSelectElement::scrollToSelection()
{
    if (usesMenuList())
        return;

    if (RenderObject* renderer = this->renderer())
        toRenderListBox(renderer)->selectionChanged();
}

void HTMLSelectElement::setOptionsChangedOnRenderer()
{
    if (RenderObject* renderer = this->renderer()) {
        if (usesMenuList())
            toRenderMenuList(renderer)->setOptionsChanged(true);
        else
            toRenderListBox(renderer)->setOptionsChanged(true);
    }
}

const Vector<HTMLElement*>& HTMLSelectElement::listItems() const
{
    if (m_shouldRecalcListItems)
        recalcListItems();
    else {
#if !ASSERT_DISABLED
        Vector<HTMLElement*> items = m_listItems;
        recalcListItems(false);
        ASSERT(items == m_listItems);
#endif
    }

    return m_listItems;
}

void HTMLSelectElement::setRecalcListItems()
{
    m_shouldRecalcListItems = true;
    // Manual selection anchor is reset when manipulating the select programmatically.
    m_activeSelectionAnchorIndex = -1;
    setOptionsChangedOnRenderer();
    setNeedsStyleRecalc();
    if (!inDocument())
        m_collectionInfo.reset();
}

void HTMLSelectElement::recalcListItems(bool updateSelectedStates) const
{
    m_listItems.clear();

    m_shouldRecalcListItems = false;

    HTMLOptionElement* foundSelected = 0;
    for (Node* currentNode = this->firstChild(); currentNode;) {
        if (!currentNode->isHTMLElement()) {
            currentNode = currentNode->traverseNextSibling(this);
            continue;
        }

        HTMLElement* current = toHTMLElement(currentNode);

        // optgroup tags may not nest. However, both FireFox and IE will
        // flatten the tree automatically, so we follow suit.
        // (http://www.w3.org/TR/html401/interact/forms.html#h-17.6)
        if (current->hasTagName(optgroupTag)) {
            m_listItems.append(current);
            if (current->firstChild()) {
                currentNode = current->firstChild();
                continue;
            }
        }

        if (current->hasTagName(optionTag)) {
            m_listItems.append(current);

            if (updateSelectedStates && !m_multiple) {
                if (!foundSelected && (m_size <= 1 || toHTMLOptionElement(current)->selected())) {
                    foundSelected = toHTMLOptionElement(current);
                    foundSelected->setSelectedState(true);
                } else if (foundSelected && toHTMLOptionElement(current)->selected()) {
                    foundSelected->setSelectedState(false);
                    foundSelected = toHTMLOptionElement(current);
                }
            }
        }

        if (current->hasTagName(hrTag))
            m_listItems.append(current);

        // In conforming HTML code, only <optgroup> and <option> will be found
        // within a <select>. We call traverseNextSibling so that we only step
        // into those tags that we choose to. For web-compat, we should cope
        // with the case where odd tags like a <div> have been added but we
        // handle this because such tags have already been removed from the
        // <select>'s subtree at this point.
        currentNode = currentNode->traverseNextSibling(this);
    }
}

int HTMLSelectElement::selectedIndex() const
{
    unsigned index = 0;

    // Return the number of the first option selected.
    const Vector<HTMLElement*>& items = listItems();
    for (size_t i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        if (element->hasTagName(optionTag)) {
            if (toHTMLOptionElement(element)->selected())
                return index;
            ++index;
        }
    }

    return -1;
}

void HTMLSelectElement::setSelectedIndex(int index)
{
    selectOption(index, DeselectOtherOptions);
}

void HTMLSelectElement::optionSelectionStateChanged(HTMLOptionElement* option, bool optionIsSelected)
{
    ASSERT(option->ownerSelectElement() == this);
    if (optionIsSelected)
        selectOption(option->index());
    else
        selectOption(m_multiple ? -1 : nextSelectableListIndex(-1));
}

void HTMLSelectElement::selectOption(int optionIndex, SelectOptionFlags flags)
{
    bool shouldDeselect = !m_multiple || (flags & DeselectOtherOptions);

    const Vector<HTMLElement*>& items = listItems();
    int listIndex = optionToListIndex(optionIndex);

    HTMLElement* element = 0;
    if (listIndex >= 0) {
        element = items[listIndex];
        if (element->hasTagName(optionTag)) {
            if (m_activeSelectionAnchorIndex < 0 || shouldDeselect)
                setActiveSelectionAnchorIndex(listIndex);
            if (m_activeSelectionEndIndex < 0 || shouldDeselect)
                setActiveSelectionEndIndex(listIndex);
            toHTMLOptionElement(element)->setSelectedState(true);
        }
    }

    if (shouldDeselect)
        deselectItemsWithoutValidation(element);

    // For the menu list case, this is what makes the selected element appear.
    if (RenderObject* renderer = this->renderer())
        renderer->updateFromElement();

    scrollToSelection();

    if (usesMenuList()) {
        m_isProcessingUserDrivenChange = flags & UserDriven;
        if (flags & DispatchChangeEvent)
            dispatchChangeEventForMenuList();
        if (RenderObject* renderer = this->renderer()) {
            if (usesMenuList())
                toRenderMenuList(renderer)->didSetSelectedIndex(listIndex);
            else if (renderer->isListBox())
                toRenderListBox(renderer)->selectionChanged();
        }
    }

    setNeedsValidityCheck();
    if (Frame* frame = document()->frame())
        frame->page()->chrome()->client()->formStateDidChange(this);
}

int HTMLSelectElement::optionToListIndex(int optionIndex) const
{
    const Vector<HTMLElement*>& items = listItems();
    int listSize = static_cast<int>(items.size());
    if (optionIndex < 0 || optionIndex >= listSize)
        return -1;

    int optionIndex2 = -1;
    for (int listIndex = 0; listIndex < listSize; ++listIndex) {
        if (items[listIndex]->hasTagName(optionTag)) {
            ++optionIndex2;
            if (optionIndex2 == optionIndex)
                return listIndex;
        }
    }

    return -1;
}

int HTMLSelectElement::listToOptionIndex(int listIndex) const
{
    const Vector<HTMLElement*>& items = listItems();
    if (listIndex < 0 || listIndex >= static_cast<int>(items.size()) || !items[listIndex]->hasTagName(optionTag))
        return -1;

    // Actual index of option not counting OPTGROUP entries that may be in list.
    int optionIndex = 0;
    for (int i = 0; i < listIndex; ++i) {
        if (items[i]->hasTagName(optionTag))
            ++optionIndex;
    }

    return optionIndex;
}

void HTMLSelectElement::dispatchFocusEvent(PassRefPtr<Node> oldFocusedNode)
{
    // Save the selection so it can be compared to the new selection when
    // dispatching change events during blur event dispatch.
    if (usesMenuList())
        saveLastSelection();
    HTMLFormControlElementWithState::dispatchFocusEvent(oldFocusedNode);
}

void HTMLSelectElement::dispatchBlurEvent(PassRefPtr<Node> newFocusedNode)
{
    // We only need to fire change events here for menu lists, because we fire
    // change events for list boxes whenever the selection change is actually made.
    // This matches other browsers' behavior.
    if (usesMenuList())
        dispatchChangeEventForMenuList();
    HTMLFormControlElementWithState::dispatchBlurEvent(newFocusedNode);
}

void HTMLSelectElement::deselectItemsWithoutValidation(HTMLElement* excludeElement)
{
    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        if (element != excludeElement && element->hasTagName(optionTag))
            toHTMLOptionElement(element)->setSelectedState(false);
    }
}

bool HTMLSelectElement::saveFormControlState(String& value) const
{
    const Vector<HTMLElement*>& items = listItems();
    size_t length = items.size();
    StringBuilder builder;
    builder.reserveCapacity(length);
    for (unsigned i = 0; i < length; ++i) {
        HTMLElement* element = items[i];
        bool selected = element->hasTagName(optionTag) && toHTMLOptionElement(element)->selected();
        builder.append(selected ? 'X' : '.');
    }
    value = builder.toString();
    return true;
}

void HTMLSelectElement::restoreFormControlState(const String& state)
{
    recalcListItems();

    const Vector<HTMLElement*>& items = listItems();
    size_t length = items.size();

    for (size_t i = 0; i < length; ++i) {
        HTMLElement* element = items[i];
        if (element->hasTagName(optionTag))
            toHTMLOptionElement(element)->setSelectedState(state[i] == 'X');
    }

    setOptionsChangedOnRenderer();
    setNeedsValidityCheck();
}

void HTMLSelectElement::parseMultipleAttribute(const Attribute* attribute)
{
    bool oldUsesMenuList = usesMenuList();
    m_multiple = !attribute->isNull();
    setNeedsValidityCheck();
    if (oldUsesMenuList != usesMenuList())
        reattachIfAttached();
}

bool HTMLSelectElement::appendFormData(FormDataList& list, bool)
{
    const AtomicString& name = formControlName();
    if (name.isEmpty())
        return false;

    bool successful = false;
    const Vector<HTMLElement*>& items = listItems();

    for (unsigned i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        if (element->hasTagName(optionTag) && toHTMLOptionElement(element)->selected() && !toHTMLOptionElement(element)->disabled()) {
            list.appendData(name, toHTMLOptionElement(element)->value());
            successful = true;
        }
    }

    // It's possible that this is a menulist with multiple options and nothing
    // will be submitted (!successful). We won't send a unselected non-disabled
    // option as fallback. This behavior matches to other browsers.
    return successful;
} 

void HTMLSelectElement::reset()
{
    HTMLOptionElement* firstOption = 0;
    HTMLOptionElement* selectedOption = 0;

    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); ++i) {
        HTMLElement* element = items[i];
        if (!element->hasTagName(optionTag))
            continue;

        if (items[i]->fastHasAttribute(selectedAttr)) {
            if (selectedOption && !m_multiple)
                selectedOption->setSelectedState(false);
            toHTMLOptionElement(element)->setSelectedState(true);
            selectedOption = toHTMLOptionElement(element);
        } else
            toHTMLOptionElement(element)->setSelectedState(false);

        if (!firstOption)
            firstOption = toHTMLOptionElement(element);
    }

    if (!selectedOption && firstOption && !m_multiple && m_size <= 1)
        firstOption->setSelectedState(true);

    setOptionsChangedOnRenderer();
    setNeedsStyleRecalc();
    setNeedsValidityCheck();
}

#if !PLATFORM(WIN) || OS(WINCE)
bool HTMLSelectElement::platformHandleKeydownEvent(KeyboardEvent* event)
{
#if ARROW_KEYS_POP_MENU
    if (!isSpatialNavigationEnabled(document()->frame())) {
        if (event->keyIdentifier() == "Down" || event->keyIdentifier() == "Up") {
            focus();
            // Calling focus() may cause us to lose our renderer. Return true so
            // that our caller doesn't process the event further, but don't set
            // the event as handled.
            if (!renderer())
                return true;

            // Save the selection so it can be compared to the new selection
            // when dispatching change events during selectOption, which
            // gets called from RenderMenuList::valueChanged, which gets called
            // after the user makes a selection from the menu.
            saveLastSelection();
            if (RenderMenuList* menuList = toRenderMenuList(renderer()))
                menuList->showPopup();
            event->setDefaultHandled();
        }
        return true;
    }
#else
    UNUSED_PARAM(event);
#endif
    return false;
}
#endif

void HTMLSelectElement::menuListDefaultEventHandler(Event* event)
{
    if (event->type() == eventNames().keydownEvent) {
        if (!renderer() || !event->isKeyboardEvent())
            return;

        if (platformHandleKeydownEvent(static_cast<KeyboardEvent*>(event)))
            return;

        // When using spatial navigation, we want to be able to navigate away
        // from the select element when the user hits any of the arrow keys,
        // instead of changing the selection.
        if (isSpatialNavigationEnabled(document()->frame())) {
            if (!m_activeSelectionState)
                return;
        }

        const String& keyIdentifier = static_cast<KeyboardEvent*>(event)->keyIdentifier();
        bool handled = true;
        const Vector<HTMLElement*>& listItems = this->listItems();
        int listIndex = optionToListIndex(selectedIndex());

        if (keyIdentifier == "Down" || keyIdentifier == "Right")
            listIndex = nextValidIndex(listIndex, SkipForwards, 1);
        else if (keyIdentifier == "Up" || keyIdentifier == "Left")
            listIndex = nextValidIndex(listIndex, SkipBackwards, 1);
        else if (keyIdentifier == "PageDown")
            listIndex = nextValidIndex(listIndex, SkipForwards, 3);
        else if (keyIdentifier == "PageUp")
            listIndex = nextValidIndex(listIndex, SkipBackwards, 3);
        else if (keyIdentifier == "Home")
            listIndex = nextValidIndex(-1, SkipForwards, 1);
        else if (keyIdentifier == "End")
            listIndex = nextValidIndex(listItems.size(), SkipBackwards, 1);
        else
            handled = false;

        if (handled && static_cast<size_t>(listIndex) < listItems.size())
            selectOption(listToOptionIndex(listIndex), DeselectOtherOptions | UserDriven);

        if (handled)
            event->setDefaultHandled();
    }

    // Use key press event here since sending simulated mouse events
    // on key down blocks the proper sending of the key press event.
    if (event->type() == eventNames().keypressEvent) {
        if (!renderer() || !event->isKeyboardEvent())
            return;

        int keyCode = static_cast<KeyboardEvent*>(event)->keyCode();
        bool handled = false;

        if (keyCode == ' ' && isSpatialNavigationEnabled(document()->frame())) {
            // Use space to toggle arrow key handling for selection change or spatial navigation.
            m_activeSelectionState = !m_activeSelectionState;
            event->setDefaultHandled();
            return;
        }

#if SPACE_OR_RETURN_POP_MENU
        if (keyCode == ' ' || keyCode == '\r') {
            focus();

            // Calling focus() may cause us to lose our renderer, in which case
            // do not want to handle the event.
            if (!renderer())
                return;

            // Save the selection so it can be compared to the new selection
            // when dispatching change events during selectOption, which
            // gets called from RenderMenuList::valueChanged, which gets called
            // after the user makes a selection from the menu.
            saveLastSelection();
            if (RenderMenuList* menuList = toRenderMenuList(renderer()))
                menuList->showPopup();
            handled = true;
        }
#elif ARROW_KEYS_POP_MENU
        if (keyCode == ' ') {
            focus();

            // Calling focus() may cause us to lose our renderer, in which case
            // do not want to handle the event.
            if (!renderer())
                return;

            // Save the selection so it can be compared to the new selection
            // when dispatching change events during selectOption, which
            // gets called from RenderMenuList::valueChanged, which gets called
            // after the user makes a selection from the menu.
            saveLastSelection();
            if (RenderMenuList* menuList = toRenderMenuList(renderer()))
                menuList->showPopup();
            handled = true;
        } else if (keyCode == '\r') {
            if (form())
                form()->submitImplicitly(event, false);
            dispatchChangeEventForMenuList();
            handled = true;
        }
#else
        int listIndex = optionToListIndex(selectedIndex());
        if (keyCode == '\r') {
            // listIndex should already be selected, but this will fire the onchange handler.
            selectOption(listToOptionIndex(listIndex), DeselectOtherOptions | DispatchChangeEvent | UserDriven);
            handled = true;
        }
#endif
        if (handled)
            event->setDefaultHandled();
    }

    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        focus();
        if (renderer() && renderer()->isMenuList()) {
            if (RenderMenuList* menuList = toRenderMenuList(renderer())) {
                if (menuList->popupIsVisible())
                    menuList->hidePopup();
                else {
                    // Save the selection so it can be compared to the new
                    // selection when we call onChange during selectOption,
                    // which gets called from RenderMenuList::valueChanged,
                    // which gets called after the user makes a selection from
                    // the menu.
                    saveLastSelection();
                    menuList->showPopup();
                }
            }
        }
        event->setDefaultHandled();
    }
}

void HTMLSelectElement::updateSelectedState(int listIndex, bool multi, bool shift)
{
    ASSERT(listIndex >= 0);

    // Save the selection so it can be compared to the new selection when
    // dispatching change events during mouseup, or after autoscroll finishes.
    saveLastSelection();

    m_activeSelectionState = true;

    bool shiftSelect = m_multiple && shift;
    bool multiSelect = m_multiple && multi && !shift;

    HTMLElement* clickedElement = listItems()[listIndex];
    if (clickedElement->hasTagName(optionTag)) {
        // Keep track of whether an active selection (like during drag
        // selection), should select or deselect.
        if (toHTMLOptionElement(clickedElement)->selected() && multi)
            m_activeSelectionState = false;
        if (!m_activeSelectionState)
            toHTMLOptionElement(clickedElement)->setSelectedState(false);
    }

    // If we're not in any special multiple selection mode, then deselect all
    // other items, excluding the clicked option. If no option was clicked, then
    // this will deselect all items in the list.
    if (!shiftSelect && !multiSelect)
        deselectItemsWithoutValidation(clickedElement);

    // If the anchor hasn't been set, and we're doing a single selection or a
    // shift selection, then initialize the anchor to the first selected index.
    if (m_activeSelectionAnchorIndex < 0 && !multiSelect)
        setActiveSelectionAnchorIndex(selectedIndex());

    // Set the selection state of the clicked option.
    if (clickedElement->hasTagName(optionTag) && !toHTMLOptionElement(clickedElement)->disabled())
        toHTMLOptionElement(clickedElement)->setSelectedState(true);

    // If there was no selectedIndex() for the previous initialization, or If
    // we're doing a single selection, or a multiple selection (using cmd or
    // ctrl), then initialize the anchor index to the listIndex that just got
    // clicked.
    if (m_activeSelectionAnchorIndex < 0 || !shiftSelect)
        setActiveSelectionAnchorIndex(listIndex);

    setActiveSelectionEndIndex(listIndex);
    updateListBoxSelection(!multiSelect);
}

void HTMLSelectElement::listBoxDefaultEventHandler(Event* event)
{
    const Vector<HTMLElement*>& listItems = this->listItems();

    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        focus();
        // Calling focus() may cause us to lose our renderer, in which case do not want to handle the event.
        if (!renderer())
            return;

        // Convert to coords relative to the list box if needed.
        MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
        IntPoint localOffset = roundedIntPoint(renderer()->absoluteToLocal(mouseEvent->absoluteLocation(), false, true));
        int listIndex = toRenderListBox(renderer())->listIndexAtOffset(toSize(localOffset));
        if (listIndex >= 0) {
#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))
            updateSelectedState(listIndex, mouseEvent->metaKey(), mouseEvent->shiftKey());
#else
            updateSelectedState(listIndex, mouseEvent->ctrlKey(), mouseEvent->shiftKey());
#endif
            if (Frame* frame = document()->frame())
                frame->eventHandler()->setMouseDownMayStartAutoscroll();

            event->setDefaultHandled();
        }
    } else if (event->type() == eventNames().mousemoveEvent && event->isMouseEvent() && !toRenderBox(renderer())->canBeScrolledAndHasScrollableArea()) {
        MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
        if (mouseEvent->button() != LeftButton || !mouseEvent->buttonDown())
            return;

        IntPoint localOffset = roundedIntPoint(renderer()->absoluteToLocal(mouseEvent->absoluteLocation(), false, true));
        int listIndex = toRenderListBox(renderer())->listIndexAtOffset(toSize(localOffset));
        if (listIndex >= 0) {
            if (m_multiple) {
                setActiveSelectionEndIndex(listIndex);
                updateListBoxSelection(false);
            } else
                updateSelectedState(listIndex, false, false);
            event->setDefaultHandled();
        }
    } else if (event->type() == eventNames().mouseupEvent && event->isMouseEvent() && static_cast<MouseEvent*>(event)->button() == LeftButton && document()->frame()->eventHandler()->autoscrollRenderer() != renderer()) {
        // This makes sure we fire dispatchFormControlChangeEvent for a single
        // click. For drag selection, onChange will fire when the autoscroll
        // timer stops.
        listBoxOnChange();
    } else if (event->type() == eventNames().keydownEvent) {
        if (!event->isKeyboardEvent())
            return;
        const String& keyIdentifier = static_cast<KeyboardEvent*>(event)->keyIdentifier();

        bool handled = false;
        int endIndex = 0;
        if (m_activeSelectionEndIndex < 0) {
            // Initialize the end index
            if (keyIdentifier == "Down" || keyIdentifier == "PageDown") {
                int startIndex = lastSelectedListIndex();
                handled = true;
                if (keyIdentifier == "Down")
                    endIndex = nextSelectableListIndex(startIndex);
                else
                    endIndex = nextSelectableListIndexPageAway(startIndex, SkipForwards);
            } else if (keyIdentifier == "Up" || keyIdentifier == "PageUp") {
                int startIndex = optionToListIndex(selectedIndex());
                handled = true;
                if (keyIdentifier == "Up")
                    endIndex = previousSelectableListIndex(startIndex);
                else
                    endIndex = nextSelectableListIndexPageAway(startIndex, SkipBackwards);
            }
        } else {
            // Set the end index based on the current end index.
            if (keyIdentifier == "Down") {
                endIndex = nextSelectableListIndex(m_activeSelectionEndIndex);
                handled = true;
            } else if (keyIdentifier == "Up") {
                endIndex = previousSelectableListIndex(m_activeSelectionEndIndex);
                handled = true;
            } else if (keyIdentifier == "PageDown") {
                endIndex = nextSelectableListIndexPageAway(m_activeSelectionEndIndex, SkipForwards);
                handled = true;
            } else if (keyIdentifier == "PageUp") {
                endIndex = nextSelectableListIndexPageAway(m_activeSelectionEndIndex, SkipBackwards);
                handled = true;
            }
        }
        if (keyIdentifier == "Home") {
            endIndex = firstSelectableListIndex();
            handled = true;
        } else if (keyIdentifier == "End") {
            endIndex = lastSelectableListIndex();
            handled = true;
        }

        if (isSpatialNavigationEnabled(document()->frame()))
            // Check if the selection moves to the boundary.
            if (keyIdentifier == "Left" || keyIdentifier == "Right" || ((keyIdentifier == "Down" || keyIdentifier == "Up") && endIndex == m_activeSelectionEndIndex))
                return;

        if (endIndex >= 0 && handled) {
            // Save the selection so it can be compared to the new selection
            // when dispatching change events immediately after making the new
            // selection.
            saveLastSelection();

            ASSERT_UNUSED(listItems, !listItems.size() || static_cast<size_t>(endIndex) < listItems.size());
            setActiveSelectionEndIndex(endIndex);

            bool selectNewItem = !m_multiple || static_cast<KeyboardEvent*>(event)->shiftKey() || !isSpatialNavigationEnabled(document()->frame());
            if (selectNewItem)
                m_activeSelectionState = true;
            // If the anchor is unitialized, or if we're going to deselect all
            // other options, then set the anchor index equal to the end index.
            bool deselectOthers = !m_multiple || (!static_cast<KeyboardEvent*>(event)->shiftKey() && selectNewItem);
            if (m_activeSelectionAnchorIndex < 0 || deselectOthers) {
                if (deselectOthers)
                    deselectItemsWithoutValidation();
                setActiveSelectionAnchorIndex(m_activeSelectionEndIndex);
            }

            toRenderListBox(renderer())->scrollToRevealElementAtListIndex(endIndex);
            if (selectNewItem) {
                updateListBoxSelection(deselectOthers);
                listBoxOnChange();
            } else
                scrollToSelection();

            event->setDefaultHandled();
        }
    } else if (event->type() == eventNames().keypressEvent) {
        if (!event->isKeyboardEvent())
            return;
        int keyCode = static_cast<KeyboardEvent*>(event)->keyCode();

        if (keyCode == '\r') {
            if (form())
                form()->submitImplicitly(event, false);
            event->setDefaultHandled();
        } else if (m_multiple && keyCode == ' ' && isSpatialNavigationEnabled(document()->frame())) {
            // Use space to toggle selection change.
            m_activeSelectionState = !m_activeSelectionState;
            updateSelectedState(listToOptionIndex(m_activeSelectionEndIndex), true /*multi*/, false /*shift*/);
            listBoxOnChange();
            event->setDefaultHandled();
        }
    }
}

void HTMLSelectElement::defaultEventHandler(Event* event)
{
    if (!renderer())
        return;

    if (usesMenuList())
        menuListDefaultEventHandler(event);
    else 
        listBoxDefaultEventHandler(event);
    if (event->defaultHandled())
        return;

    if (event->type() == eventNames().keypressEvent && event->isKeyboardEvent()) {
        KeyboardEvent* keyboardEvent = static_cast<KeyboardEvent*>(event);
        if (!keyboardEvent->ctrlKey() && !keyboardEvent->altKey() && !keyboardEvent->metaKey() && isPrintableChar(keyboardEvent->charCode())) {
            typeAheadFind(keyboardEvent);
            event->setDefaultHandled();
            return;
        }
    }
    HTMLFormControlElementWithState::defaultEventHandler(event);
}

int HTMLSelectElement::lastSelectedListIndex() const
{
    const Vector<HTMLElement*>& items = listItems();
    for (size_t i = items.size(); i;) {
        HTMLElement* element = items[--i];
        if (element->hasTagName(optionTag) && toHTMLOptionElement(element)->selected())
            return i;
    }
    return -1;
}

static String stripLeadingWhiteSpace(const String& string)
{
    int length = string.length();

    int i;
    for (i = 0; i < length; ++i) {
        if (string[i] != noBreakSpace && (string[i] <= 0x7F ? !isASCIISpace(string[i]) : (direction(string[i]) != WhiteSpaceNeutral)))
            break;
    }

    return string.substring(i, length - i);
}

void HTMLSelectElement::typeAheadFind(KeyboardEvent* event)
{
    if (event->timeStamp() < m_lastCharTime)
        return;

    DOMTimeStamp delta = event->timeStamp() - m_lastCharTime;
    m_lastCharTime = event->timeStamp();

    UChar c = event->charCode();

    String prefix;
    int searchStartOffset = 1;
    if (delta > typeAheadTimeout) {
        prefix = String(&c, 1);
        m_typedString = prefix;
        m_repeatingChar = c;
    } else {
        m_typedString.append(c);

        if (c == m_repeatingChar) {
            // The user is likely trying to cycle through all the items starting
            // with this character, so just search on the character.
            prefix = String(&c, 1);
        } else {
            m_repeatingChar = 0;
            prefix = m_typedString;
            searchStartOffset = 0;
        }
    }

    const Vector<HTMLElement*>& items = listItems();
    int itemCount = items.size();
    if (itemCount < 1)
        return;

    int selected = selectedIndex();
    int index = (optionToListIndex(selected >= 0 ? selected : 0) + searchStartOffset) % itemCount;
    ASSERT(index >= 0);

    // Compute a case-folded copy of the prefix string before beginning the search for
    // a matching element. This code uses foldCase to work around the fact that
    // String::startWith does not fold non-ASCII characters. This code can be changed
    // to use startWith once that is fixed.
    String prefixWithCaseFolded(prefix.foldCase());
    for (int i = 0; i < itemCount; ++i, index = (index + 1) % itemCount) {
        HTMLElement* element = items[index];
        if (!element->hasTagName(optionTag) || toHTMLOptionElement(element)->disabled())
            continue;

        // Fold the option string and check if its prefix is equal to the folded prefix.
        String text = toHTMLOptionElement(element)->textIndentedToRespectGroupLabel();
        if (stripLeadingWhiteSpace(text).foldCase().startsWith(prefixWithCaseFolded)) {
            selectOption(listToOptionIndex(index), DeselectOtherOptions | UserDriven);
            if (!usesMenuList())
                listBoxOnChange();

            setOptionsChangedOnRenderer();
            setNeedsStyleRecalc();
            return;
        }
    }
}

void HTMLSelectElement::insertedIntoTree(bool deep)
{
    // When the element is created during document parsing, it won't have any
    // items yet - but for innerHTML and related methods, this method is called
    // after the whole subtree is constructed.
    recalcListItems();
    HTMLFormControlElementWithState::insertedIntoTree(deep);
}

void HTMLSelectElement::accessKeySetSelectedIndex(int index)
{    
    // First bring into focus the list box.
    if (!focused())
        accessKeyAction(false);
    
    // If this index is already selected, unselect. otherwise update the selected index.
    const Vector<HTMLElement*>& items = listItems();
    int listIndex = optionToListIndex(index);
    if (listIndex >= 0) {
        HTMLElement* element = items[listIndex];
        if (element->hasTagName(optionTag)) {
            if (toHTMLOptionElement(element)->selected())
                toHTMLOptionElement(element)->setSelectedState(false);
            else
                selectOption(index, DispatchChangeEvent | UserDriven);
        }
    }

    if (usesMenuList())
        dispatchChangeEventForMenuList();
    else
        listBoxOnChange();

    scrollToSelection();
}

unsigned HTMLSelectElement::length() const
{
    unsigned options = 0;

    const Vector<HTMLElement*>& items = listItems();
    for (unsigned i = 0; i < items.size(); ++i) {
        if (items[i]->hasTagName(optionTag))
            ++options;
    }

    return options;
}

#ifndef NDEBUG

HTMLSelectElement* toHTMLSelectElement(Node* node)
{
    ASSERT(!node || node->hasTagName(selectTag));
    return static_cast<HTMLSelectElement*>(node);
}

const HTMLSelectElement* toHTMLSelectElement(const Node* node)
{
    ASSERT(!node || node->hasTagName(selectTag));
    return static_cast<const HTMLSelectElement*>(node);
}

#endif

} // namespace
