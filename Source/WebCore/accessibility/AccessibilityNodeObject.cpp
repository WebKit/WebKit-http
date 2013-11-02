/*
* Copyright (C) 2012, Google Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1.  Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
* 2.  Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
* 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
*     its contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "AccessibilityNodeObject.h"

#include "AXObjectCache.h"
#include "AccessibilityImageMapLink.h"
#include "AccessibilityListBox.h"
#include "AccessibilitySpinButton.h"
#include "AccessibilityTable.h"
#include "ElementIterator.h"
#include "EventNames.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLAreaElement.h"
#include "HTMLFieldSetElement.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLLabelElement.h"
#include "HTMLLegendElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLOptionsCollection.h"
#include "HTMLParserIdioms.h"
#include "HTMLPlugInImageElement.h"
#include "HTMLSelectElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLTextFormControlElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "LabelableElement.h"
#include "LocalizedStrings.h"
#include "MathMLNames.h"
#include "NodeList.h"
#include "NodeTraversal.h"
#include "Page.h"
#include "ProgressTracker.h"
#include "RenderView.h"
#include "SVGElement.h"
#include "SVGNames.h"
#include "Text.h"
#include "TextControlInnerElements.h"
#include "TextIterator.h"
#include "UserGestureIndicator.h"
#include "VisibleUnits.h"
#include "Widget.h"
#include "htmlediting.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

using namespace HTMLNames;

static String accessibleNameForNode(Node*);

AccessibilityNodeObject::AccessibilityNodeObject(Node* node)
    : AccessibilityObject()
    , m_ariaRole(UnknownRole)
    , m_childrenDirty(false)
    , m_roleForMSAA(UnknownRole)
#ifndef NDEBUG
    , m_initialized(false)
#endif
    , m_node(node)
{
}

AccessibilityNodeObject::~AccessibilityNodeObject()
{
    ASSERT(isDetached());
}

void AccessibilityNodeObject::init()
{
#ifndef NDEBUG
    ASSERT(!m_initialized);
    m_initialized = true;
#endif
    m_role = determineAccessibilityRole();
}

PassRefPtr<AccessibilityNodeObject> AccessibilityNodeObject::create(Node* node)
{
    return adoptRef(new AccessibilityNodeObject(node));
}

void AccessibilityNodeObject::detach()
{
    clearChildren();
    AccessibilityObject::detach();
    m_node = 0;
}

void AccessibilityNodeObject::childrenChanged()
{
    // This method is meant as a quick way of marking a portion of the accessibility tree dirty.
    if (!node() && !renderer())
        return;

    axObjectCache()->postNotification(this, document(), AXObjectCache::AXChildrenChanged);

    // Go up the accessibility parent chain, but only if the element already exists. This method is
    // called during render layouts, minimal work should be done. 
    // If AX elements are created now, they could interrogate the render tree while it's in a funky state.
    // At the same time, process ARIA live region changes.
    for (AccessibilityObject* parent = this; parent; parent = parent->parentObjectIfExists()) {
        parent->setNeedsToUpdateChildren();

        // These notifications always need to be sent because screenreaders are reliant on them to perform. 
        // In other words, they need to be sent even when the screen reader has not accessed this live region since the last update.

        // If this element supports ARIA live regions, then notify the AT of changes.
        if (parent->supportsARIALiveRegion())
            axObjectCache()->postNotification(parent, parent->document(), AXObjectCache::AXLiveRegionChanged);
        
        // If this element is an ARIA text control, notify the AT of changes.
        if (parent->isARIATextControl() && !parent->isNativeTextControl() && !parent->node()->rendererIsEditable())
            axObjectCache()->postNotification(parent, parent->document(), AXObjectCache::AXValueChanged);
    }
}

void AccessibilityNodeObject::updateAccessibilityRole()
{
    bool ignoredStatus = accessibilityIsIgnored();
    m_role = determineAccessibilityRole();
    
    // The AX hierarchy only needs to be updated if the ignored status of an element has changed.
    if (ignoredStatus != accessibilityIsIgnored())
        childrenChanged();
}
    
AccessibilityObject* AccessibilityNodeObject::firstChild() const
{
    if (!node())
        return 0;
    
    Node* firstChild = node()->firstChild();

    if (!firstChild)
        return 0;
    
    return axObjectCache()->getOrCreate(firstChild);
}

AccessibilityObject* AccessibilityNodeObject::lastChild() const
{
    if (!node())
        return 0;
    
    Node* lastChild = node()->lastChild();
    if (!lastChild)
        return 0;
    
    return axObjectCache()->getOrCreate(lastChild);
}

AccessibilityObject* AccessibilityNodeObject::previousSibling() const
{
    if (!node())
        return 0;

    Node* previousSibling = node()->previousSibling();
    if (!previousSibling)
        return 0;

    return axObjectCache()->getOrCreate(previousSibling);
}

AccessibilityObject* AccessibilityNodeObject::nextSibling() const
{
    if (!node())
        return 0;

    Node* nextSibling = node()->nextSibling();
    if (!nextSibling)
        return 0;

    return axObjectCache()->getOrCreate(nextSibling);
}
    
AccessibilityObject* AccessibilityNodeObject::parentObjectIfExists() const
{
    return parentObject();
}
    
AccessibilityObject* AccessibilityNodeObject::parentObject() const
{
    if (!node())
        return 0;

    Node* parentObj = node()->parentNode();
    if (parentObj)
        return axObjectCache()->getOrCreate(parentObj);
    
    return 0;
}

LayoutRect AccessibilityNodeObject::elementRect() const
{
    return boundingBoxRect();
}
    
LayoutRect AccessibilityNodeObject::boundingBoxRect() const
{
    // AccessibilityNodeObjects have no mechanism yet to return a size or position.
    // For now, let's return the position of the ancestor that does have a position,
    // and make it the width of that parent, and about the height of a line of text, so that it's clear the object is a child of the parent.
    
    LayoutRect boundingBox;
    
    for (AccessibilityObject* positionProvider = parentObject(); positionProvider; positionProvider = positionProvider->parentObject()) {
        if (positionProvider->isAccessibilityRenderObject()) {
            LayoutRect parentRect = positionProvider->elementRect();
            boundingBox.setSize(LayoutSize(parentRect.width(), LayoutUnit(std::min(10.0f, parentRect.height().toFloat()))));
            boundingBox.setLocation(parentRect.location());
            break;
        }
    }
    
    return boundingBox;
}

void AccessibilityNodeObject::setNode(Node* node)
{
    m_node = node;
}

Document* AccessibilityNodeObject::document() const
{
    if (!node())
        return 0;
    return &node()->document();
}

AccessibilityRole AccessibilityNodeObject::determineAccessibilityRole()
{
    if (!node())
        return UnknownRole;

    m_ariaRole = determineAriaRoleAttribute();
    
    AccessibilityRole ariaRole = ariaRoleAttribute();
    if (ariaRole != UnknownRole)
        return ariaRole;

    if (node()->isLink())
        return WebCoreLinkRole;
    if (node()->isTextNode())
        return StaticTextRole;
    if (node()->hasTagName(buttonTag))
        return buttonRoleType();
    if (isHTMLInputElement(node())) {
        HTMLInputElement* input = toHTMLInputElement(node());
        if (input->isCheckbox())
            return CheckBoxRole;
        if (input->isRadioButton())
            return RadioButtonRole;
        if (input->isTextButton())
            return buttonRoleType();
        if (input->isRangeControl())
            return SliderRole;

#if ENABLE(INPUT_TYPE_COLOR)
        const AtomicString& type = input->getAttribute(typeAttr);
        if (equalIgnoringCase(type, "color"))
            return ColorWellRole;
#endif

        return TextFieldRole;
    }
    if (node()->hasTagName(selectTag)) {
        HTMLSelectElement* selectElement = toHTMLSelectElement(node());
        return selectElement->multiple() ? ListBoxRole : PopUpButtonRole;
    }
    if (isHTMLTextAreaElement(node()))
        return TextAreaRole;
    if (headingLevel())
        return HeadingRole;
    if (node()->hasTagName(divTag))
        return DivRole;
    if (node()->hasTagName(pTag))
        return ParagraphRole;
    if (isHTMLLabelElement(node()))
        return LabelRole;
    if (node()->isElementNode() && toElement(node())->isFocusable())
        return GroupRole;
    
    return UnknownRole;
}

void AccessibilityNodeObject::insertChild(AccessibilityObject* child, unsigned index)
{
    if (!child)
        return;
    
    // If the parent is asking for this child's children, then either it's the first time (and clearing is a no-op),
    // or its visibility has changed. In the latter case, this child may have a stale child cached.
    // This can prevent aria-hidden changes from working correctly. Hence, whenever a parent is getting children, ensure data is not stale.
    child->clearChildren();
    
    if (child->accessibilityIsIgnored()) {
        AccessibilityChildrenVector children = child->children();
        size_t length = children.size();
        for (size_t i = 0; i < length; ++i)
            m_children.insert(index + i, children[i]);
    } else {
        ASSERT(child->parentObject() == this);
        m_children.insert(index, child);
    }
}

void AccessibilityNodeObject::addChild(AccessibilityObject* child)
{
    insertChild(child, m_children.size());
}

void AccessibilityNodeObject::addChildren()
{
    // If the need to add more children in addition to existing children arises, 
    // childrenChanged should have been called, leaving the object with no children.
    ASSERT(!m_haveChildren); 
    
    if (!m_node)
        return;

    m_haveChildren = true;

    // The only time we add children from the DOM tree to a node with a renderer is when it's a canvas.
    if (renderer() && !m_node->hasTagName(canvasTag))
        return;
    
    for (Node* child = m_node->firstChild(); child; child = child->nextSibling())
        addChild(axObjectCache()->getOrCreate(child));
}

bool AccessibilityNodeObject::canHaveChildren() const
{
    // If this is an AccessibilityRenderObject, then it's okay if this object
    // doesn't have a node - there are some renderers that don't have associated
    // nodes, like scroll areas and css-generated text.
    if (!node() && !isAccessibilityRenderObject())
        return false;

    // When <noscript> is not being used (its renderer() == 0), ignore its children.
    if (node() && !renderer() && node()->hasTagName(noscriptTag))
        return false;
    
    // Elements that should not have children
    switch (roleValue()) {
    case ImageRole:
    case ButtonRole:
    case PopUpButtonRole:
    case CheckBoxRole:
    case RadioButtonRole:
    case TabRole:
    case ToggleButtonRole:
    case StaticTextRole:
    case ListBoxOptionRole:
    case ScrollBarRole:
    case ProgressIndicatorRole:
        return false;
    case LegendRole:
        if (Element* element = this->element())
            return !ancestorsOfType<HTMLFieldSetElement>(*element).first();
    default:
        return true;
    }
}

bool AccessibilityNodeObject::computeAccessibilityIsIgnored() const
{
#ifndef NDEBUG
    // Double-check that an AccessibilityObject is never accessed before
    // it's been initialized.
    ASSERT(m_initialized);
#endif

    // If this element is within a parent that cannot have children, it should not be exposed.
    if (isDescendantOfBarrenParent())
        return true;

    return m_role == UnknownRole;
}

bool AccessibilityNodeObject::canvasHasFallbackContent() const
{
    Node* node = this->node();
    if (!node || !node->hasTagName(canvasTag))
        return false;
    Element& canvasElement = toElement(*node);
    // If it has any children that are elements, we'll assume it might be fallback
    // content. If it has no children or its only children are not elements
    // (e.g. just text nodes), it doesn't have fallback content.
    return elementChildren(canvasElement).first();
}

bool AccessibilityNodeObject::isImageButton() const
{
    return isNativeImage() && isButton();
}

bool AccessibilityNodeObject::isAnchor() const
{
    return !isNativeImage() && isLink();
}

bool AccessibilityNodeObject::isNativeTextControl() const
{
    Node* node = this->node();
    if (!node)
        return false;

    if (isHTMLTextAreaElement(node))
        return true;

    if (isHTMLInputElement(node)) {
        HTMLInputElement* input = toHTMLInputElement(node);
        return input->isText() || input->isNumberField();
    }

    return false;
}

bool AccessibilityNodeObject::isSearchField() const
{
    Node* node = this->node();
    if (!node)
        return false;

    HTMLInputElement* inputElement = node->toInputElement();
    if (!inputElement)
        return false;

    if (inputElement->isSearchField())
        return true;

    // Some websites don't label their search fields as such. However, they will
    // use the word "search" in either the form or input type. This won't catch every case,
    // but it will catch google.com for example.

    // Check the node name of the input type, sometimes it's "search".
    const AtomicString& nameAttribute = getAttribute(nameAttr);
    if (nameAttribute.contains("search", false))
        return true;

    // Check the form action and the name, which will sometimes be "search".
    HTMLFormElement* form = inputElement->form();
    if (form && (form->name().contains("search", false) || form->action().contains("search", false)))
        return true;

    return false;
}

bool AccessibilityNodeObject::isNativeImage() const
{
    Node* node = this->node();
    if (!node)
        return false;

    if (isHTMLImageElement(node))
        return true;

    if (node->hasTagName(appletTag) || node->hasTagName(embedTag) || node->hasTagName(objectTag))
        return true;

    if (isHTMLInputElement(node)) {
        HTMLInputElement* input = toHTMLInputElement(node);
        return input->isImageButton();
    }

    return false;
}

bool AccessibilityNodeObject::isImage() const
{
    return roleValue() == ImageRole;
}

bool AccessibilityNodeObject::isPasswordField() const
{
    Node* node = this->node();
    if (!node || !node->isHTMLElement())
        return false;

    if (ariaRoleAttribute() != UnknownRole)
        return false;

    HTMLInputElement* inputElement = node->toInputElement();
    if (!inputElement)
        return false;

    return inputElement->isPasswordField();
}

bool AccessibilityNodeObject::isInputImage() const
{
    Node* node = this->node();
    if (!node)
        return false;
 
    if (roleValue() == ButtonRole && isHTMLInputElement(node)) {
        HTMLInputElement* input = toHTMLInputElement(node);
        return input->isImageButton();
    }

    return false;
}

bool AccessibilityNodeObject::isProgressIndicator() const
{
    return roleValue() == ProgressIndicatorRole;
}

bool AccessibilityNodeObject::isSlider() const
{
    return roleValue() == SliderRole;
}

bool AccessibilityNodeObject::isMenuRelated() const
{
    switch (roleValue()) {
    case MenuRole:
    case MenuBarRole:
    case MenuButtonRole:
    case MenuItemRole:
    case MenuItemCheckboxRole:
    case MenuItemRadioRole:
        return true;
    default:
        return false;
    }
}

bool AccessibilityNodeObject::isMenu() const
{
    return roleValue() == MenuRole;
}

bool AccessibilityNodeObject::isMenuBar() const
{
    return roleValue() == MenuBarRole;
}

bool AccessibilityNodeObject::isMenuButton() const
{
    return roleValue() == MenuButtonRole;
}

bool AccessibilityNodeObject::isMenuItem() const
{
    switch (roleValue()) {
    case MenuItemRole:
    case MenuItemRadioRole:
    case MenuItemCheckboxRole:
        return true;
    default:
        return false;
    }
}

bool AccessibilityNodeObject::isNativeCheckboxOrRadio() const
{
    Node* node = this->node();
    if (!node)
        return false;

    HTMLInputElement* input = node->toInputElement();
    if (input)
        return input->isCheckbox() || input->isRadioButton();

    return false;
}

bool AccessibilityNodeObject::isEnabled() const
{
    // ARIA says that the disabled status applies to the current element and all descendant elements.
    for (AccessibilityObject* object = const_cast<AccessibilityNodeObject*>(this); object; object = object->parentObject()) {
        const AtomicString& disabledStatus = object->getAttribute(aria_disabledAttr);
        if (equalIgnoringCase(disabledStatus, "true"))
            return false;
        if (equalIgnoringCase(disabledStatus, "false"))
            break;
    }

    Node* node = this->node();
    if (!node || !node->isElementNode())
        return true;

    return !toElement(node)->isDisabledFormControl();
}

bool AccessibilityNodeObject::isIndeterminate() const
{
    Node* node = this->node();
    if (!node)
        return false;

    HTMLInputElement* inputElement = node->toInputElement();
    if (!inputElement)
        return false;

    return inputElement->shouldAppearIndeterminate();
}

bool AccessibilityNodeObject::isPressed() const
{
    if (!isButton())
        return false;

    Node* node = this->node();
    if (!node)
        return false;

    // If this is an ARIA button, check the aria-pressed attribute rather than node()->active()
    if (ariaRoleAttribute() == ButtonRole) {
        if (equalIgnoringCase(getAttribute(aria_pressedAttr), "true"))
            return true;
        return false;
    }

    if (!node->isElementNode())
        return false;
    return toElement(node)->active();
}

bool AccessibilityNodeObject::isChecked() const
{
    Node* node = this->node();
    if (!node)
        return false;

    // First test for native checkedness semantics
    HTMLInputElement* inputElement = node->toInputElement();
    if (inputElement)
        return inputElement->shouldAppearChecked();

    // Else, if this is an ARIA checkbox or radio, respect the aria-checked attribute
    bool validRole = false;
    switch (ariaRoleAttribute()) {
    case RadioButtonRole:
    case CheckBoxRole:
    case MenuItemRole:
    case MenuItemCheckboxRole:
    case MenuItemRadioRole:
        validRole = true;
        break;
    default:
        break;
    }
    
    if (validRole && equalIgnoringCase(getAttribute(aria_checkedAttr), "true"))
        return true;

    return false;
}

bool AccessibilityNodeObject::isHovered() const
{
    Node* node = this->node();
    if (!node)
        return false;

    return node->isElementNode() && toElement(node)->hovered();
}

bool AccessibilityNodeObject::isMultiSelectable() const
{
    const AtomicString& ariaMultiSelectable = getAttribute(aria_multiselectableAttr);
    if (equalIgnoringCase(ariaMultiSelectable, "true"))
        return true;
    if (equalIgnoringCase(ariaMultiSelectable, "false"))
        return false;
    
    return node() && node()->hasTagName(selectTag) && toHTMLSelectElement(node())->multiple();
}

bool AccessibilityNodeObject::isReadOnly() const
{
    Node* node = this->node();
    if (!node)
        return true;

    if (isHTMLTextAreaElement(node))
        return toHTMLFormControlElement(node)->isReadOnly();

    if (isHTMLInputElement(node)) {
        HTMLInputElement* input = toHTMLInputElement(node);
        if (input->isTextField())
            return input->isReadOnly();
    }

    return !node->rendererIsEditable();
}

bool AccessibilityNodeObject::isRequired() const
{
    if (equalIgnoringCase(getAttribute(aria_requiredAttr), "true"))
        return true;

    Node* n = this->node();
    if (n && (n->isElementNode() && toElement(n)->isFormControlElement()))
        return static_cast<HTMLFormControlElement*>(n)->isRequired();

    return false;
}

bool AccessibilityNodeObject::supportsRequiredAttribute() const
{
    switch (roleValue()) {
    case ButtonRole:
        return isFileUploadButton();
    case CellRole:
    case CheckBoxRole:
    case ComboBoxRole:
    case GridRole:
    case IncrementorRole:
    case ListBoxRole:
    case PopUpButtonRole:
    case RadioButtonRole:
    case RadioGroupRole:
    case RowHeaderRole:
    case SliderRole:
    case SpinButtonRole:
    case TableHeaderContainerRole:
    case TextAreaRole:
    case TextFieldRole:
    case ToggleButtonRole:
        return true;
    default:
        return false;
    }
}

int AccessibilityNodeObject::headingLevel() const
{
    // headings can be in block flow and non-block flow
    Node* node = this->node();
    if (!node)
        return false;

    if (isHeading()) {
        int ariaLevel = getAttribute(aria_levelAttr).toInt();
        if (ariaLevel > 0)
            return ariaLevel;
    }

    if (node->hasTagName(h1Tag))
        return 1;

    if (node->hasTagName(h2Tag))
        return 2;

    if (node->hasTagName(h3Tag))
        return 3;

    if (node->hasTagName(h4Tag))
        return 4;

    if (node->hasTagName(h5Tag))
        return 5;

    if (node->hasTagName(h6Tag))
        return 6;

    return 0;
}

String AccessibilityNodeObject::valueDescription() const
{
    if (!isRangeControl())
        return String();

    return getAttribute(aria_valuetextAttr).string();
}

float AccessibilityNodeObject::valueForRange() const
{
    if (node() && isHTMLInputElement(node())) {
        HTMLInputElement* input = toHTMLInputElement(node());
        if (input->isRangeControl())
            return input->valueAsNumber();
    }

    if (!isRangeControl())
        return 0.0f;

    return getAttribute(aria_valuenowAttr).toFloat();
}

float AccessibilityNodeObject::maxValueForRange() const
{
    if (node() && isHTMLInputElement(node())) {
        HTMLInputElement* input = toHTMLInputElement(node());
        if (input->isRangeControl())
            return input->maximum();
    }

    if (!isRangeControl())
        return 0.0f;

    return getAttribute(aria_valuemaxAttr).toFloat();
}

float AccessibilityNodeObject::minValueForRange() const
{
    if (node() && isHTMLInputElement(node())) {
        HTMLInputElement* input = toHTMLInputElement(node());
        if (input->isRangeControl())
            return input->minimum();
    }

    if (!isRangeControl())
        return 0.0f;

    return getAttribute(aria_valueminAttr).toFloat();
}

float AccessibilityNodeObject::stepValueForRange() const
{
    return getAttribute(stepAttr).toFloat();
}

bool AccessibilityNodeObject::isHeading() const
{
    return roleValue() == HeadingRole;
}

bool AccessibilityNodeObject::isLink() const
{
    return roleValue() == WebCoreLinkRole;
}

bool AccessibilityNodeObject::isControl() const
{
    Node* node = this->node();
    if (!node)
        return false;

    return ((node->isElementNode() && toElement(node)->isFormControlElement())
        || AccessibilityObject::isARIAControl(ariaRoleAttribute()));
}

bool AccessibilityNodeObject::isFieldset() const
{
    Node* node = this->node();
    if (!node)
        return false;

    return node->hasTagName(fieldsetTag);
}

bool AccessibilityNodeObject::isGroup() const
{
    return roleValue() == GroupRole;
}

AccessibilityObject* AccessibilityNodeObject::selectedRadioButton()
{
    if (!isRadioGroup())
        return 0;

    AccessibilityObject::AccessibilityChildrenVector children = this->children();

    // Find the child radio button that is selected (ie. the intValue == 1).
    size_t size = children.size();
    for (size_t i = 0; i < size; ++i) {
        AccessibilityObject* object = children[i].get();
        if (object->roleValue() == RadioButtonRole && object->checkboxOrRadioValue() == ButtonStateOn)
            return object;
    }
    return 0;
}

AccessibilityObject* AccessibilityNodeObject::selectedTabItem()
{
    if (!isTabList())
        return 0;

    // Find the child tab item that is selected (ie. the intValue == 1).
    AccessibilityObject::AccessibilityChildrenVector tabs;
    tabChildren(tabs);

    AccessibilityObject::AccessibilityChildrenVector children = this->children();
    size_t size = tabs.size();
    for (size_t i = 0; i < size; ++i) {
        AccessibilityObject* object = children[i].get();
        if (object->isTabItem() && object->isChecked())
            return object;
    }
    return 0;
}

AccessibilityButtonState AccessibilityNodeObject::checkboxOrRadioValue() const
{
    if (isNativeCheckboxOrRadio())
        return isChecked() ? ButtonStateOn : ButtonStateOff;

    return AccessibilityObject::checkboxOrRadioValue();
}

Element* AccessibilityNodeObject::anchorElement() const
{
    Node* node = this->node();
    if (!node)
        return 0;

    AXObjectCache* cache = axObjectCache();

    // search up the DOM tree for an anchor element
    // NOTE: this assumes that any non-image with an anchor is an HTMLAnchorElement
    for ( ; node; node = node->parentNode()) {
        if (isHTMLAnchorElement(node) || (node->renderer() && cache->getOrCreate(node->renderer())->isAnchor()))
            return toElement(node);
    }

    return 0;
}

static bool isNodeActionElement(Node* node)
{
    if (isHTMLInputElement(node)) {
        HTMLInputElement* input = toHTMLInputElement(node);
        if (!input->isDisabledFormControl() && (input->isRadioButton() || input->isCheckbox() || input->isTextButton() || input->isFileUpload() || input->isImageButton()))
            return true;
    } else if (node->hasTagName(buttonTag) || node->hasTagName(selectTag))
        return true;

    return false;
}
    
static Element* nativeActionElement(Node* start)
{
    if (!start)
        return 0;
    
    // Do a deep-dive to see if any nodes should be used as the action element.
    // We have to look at Nodes, since this method should only be called on objects that do not have children (like buttons).
    // It solves the problem when authors put role="button" on a group and leave the actual button inside the group.
    
    for (Node* child = start->firstChild(); child; child = child->nextSibling()) {
        if (isNodeActionElement(child))
            return toElement(child);

        if (Element* subChild = nativeActionElement(child))
            return subChild;
    }
    return 0;
}
    
Element* AccessibilityNodeObject::actionElement() const
{
    Node* node = this->node();
    if (!node)
        return 0;

    if (isNodeActionElement(node))
        return toElement(node);
    
    if (AccessibilityObject::isARIAInput(ariaRoleAttribute()))
        return toElement(node);

    switch (roleValue()) {
    case ButtonRole:
    case PopUpButtonRole:
    case ToggleButtonRole:
    case TabRole:
    case MenuItemRole:
    case MenuItemCheckboxRole:
    case MenuItemRadioRole:
    case ListItemRole:
        // Check if the author is hiding the real control element inside the ARIA element.
        if (Element* nativeElement = nativeActionElement(node))
            return nativeElement;
        return toElement(node);
    default:
        break;
    }
    
    Element* elt = anchorElement();
    if (!elt)
        elt = mouseButtonListener();
    return elt;
}

Element* AccessibilityNodeObject::mouseButtonListener() const
{
    Node* node = this->node();
    if (!node)
        return 0;

    // check if our parent is a mouse button listener
    // FIXME: Do the continuation search like anchorElement does
    auto lineage = elementLineage(node->isElementNode() ? toElement(node) : node->parentElement());
    for (auto element = lineage.begin(), end = lineage.end(); element != end; ++element) {
        // If we've reached the body and this is not a control element, do not expose press action for this element.
        // It can cause false positives, where every piece of text is labeled as accepting press actions. 
        if (element->hasTagName(bodyTag) && isStaticText())
            break;
        
        if (element->hasEventListeners(eventNames().clickEvent) || element->hasEventListeners(eventNames().mousedownEvent) || element->hasEventListeners(eventNames().mouseupEvent))
            return &*element;
    }

    return 0;
}

bool AccessibilityNodeObject::isDescendantOfBarrenParent() const
{
    for (AccessibilityObject* object = parentObject(); object; object = object->parentObject()) {
        if (!object->canHaveChildren())
            return true;
    }

    return false;
}

void AccessibilityNodeObject::alterSliderValue(bool increase)
{
    if (roleValue() != SliderRole)
        return;

    if (!getAttribute(stepAttr).isEmpty())
        changeValueByStep(increase);
    else
        changeValueByPercent(increase ? 5 : -5);
}
    
void AccessibilityNodeObject::increment()
{
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    alterSliderValue(true);
}

void AccessibilityNodeObject::decrement()
{
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    alterSliderValue(false);
}

void AccessibilityNodeObject::changeValueByStep(bool increase)
{
    float step = stepValueForRange();
    float value = valueForRange();

    value += increase ? step : -step;

    setValue(String::number(value));

    axObjectCache()->postNotification(node(), AXObjectCache::AXValueChanged);
}

void AccessibilityNodeObject::changeValueByPercent(float percentChange)
{
    float range = maxValueForRange() - minValueForRange();
    float value = valueForRange();

    value += range * (percentChange / 100);
    setValue(String::number(value));

    axObjectCache()->postNotification(node(), AXObjectCache::AXValueChanged);
}

bool AccessibilityNodeObject::isGenericFocusableElement() const
{
    if (!canSetFocusAttribute())
        return false;

     // If it's a control, it's not generic.
     if (isControl())
        return false;

    // If it has an aria role, it's not generic.
    if (m_ariaRole != UnknownRole)
        return false;

    // If the content editable attribute is set on this element, that's the reason
    // it's focusable, and existing logic should handle this case already - so it's not a
    // generic focusable element.

    if (hasContentEditableAttributeSet())
        return false;

    // The web area and body element are both focusable, but existing logic handles these
    // cases already, so we don't need to include them here.
    if (roleValue() == WebAreaRole)
        return false;
    if (node() && node()->hasTagName(bodyTag))
        return false;

    // An SVG root is focusable by default, but it's probably not interactive, so don't
    // include it. It can still be made accessible by giving it an ARIA role.
    if (roleValue() == SVGRootRole)
        return false;

    return true;
}

HTMLLabelElement* AccessibilityNodeObject::labelForElement(Element* element) const
{
    if (!element->isHTMLElement() || !toHTMLElement(element)->isLabelable())
        return 0;

    const AtomicString& id = element->getIdAttribute();
    if (!id.isEmpty()) {
        if (HTMLLabelElement* label = element->treeScope().labelElementForId(id))
            return label;
    }

    return ancestorsOfType<HTMLLabelElement>(*element).first();
}

String AccessibilityNodeObject::ariaAccessibilityDescription() const
{
    String ariaLabeledBy = ariaLabeledByAttribute();
    if (!ariaLabeledBy.isEmpty())
        return ariaLabeledBy;

    const AtomicString& ariaLabel = getAttribute(aria_labelAttr);
    if (!ariaLabel.isEmpty())
        return ariaLabel;

    return String();
}

static Element* siblingWithAriaRole(String role, Node* node)
{
    ContainerNode* parent = node->parentNode();
    if (!parent)
        return 0;
    auto children = elementChildren(*parent);
    for (auto sibling = children.begin(), end = children.end(); sibling != end; ++sibling) {
        const AtomicString& siblingAriaRole = sibling->fastGetAttribute(roleAttr);
        if (equalIgnoringCase(siblingAriaRole, role))
            return &*sibling;
    }

    return 0;
}

Element* AccessibilityNodeObject::menuElementForMenuButton() const
{
    if (ariaRoleAttribute() != MenuButtonRole)
        return 0;

    return siblingWithAriaRole("menu", node());
}

AccessibilityObject* AccessibilityNodeObject::menuForMenuButton() const
{
    return axObjectCache()->getOrCreate(menuElementForMenuButton());
}

Element* AccessibilityNodeObject::menuItemElementForMenu() const
{
    if (ariaRoleAttribute() != MenuRole)
        return 0;
    
    return siblingWithAriaRole("menuitem", node());    
}

AccessibilityObject* AccessibilityNodeObject::menuButtonForMenu() const
{
    Element* menuItem = menuItemElementForMenu();

    if (menuItem) {
        // ARIA just has generic menu items. AppKit needs to know if this is a top level items like MenuBarButton or MenuBarItem
        AccessibilityObject* menuItemAX = axObjectCache()->getOrCreate(menuItem);
        if (menuItemAX && menuItemAX->isMenuButton())
            return menuItemAX;
    }
    return 0;
}

void AccessibilityNodeObject::titleElementText(Vector<AccessibilityText>& textOrder) const
{
    Node* node = this->node();
    if (!node)
        return;
    
    bool isInputTag = isHTMLInputElement(node);
    if (isInputTag || AccessibilityObject::isARIAInput(ariaRoleAttribute()) || isControl()) {
        HTMLLabelElement* label = labelForElement(toElement(node));
        if (label) {
            AccessibilityObject* labelObject = axObjectCache()->getOrCreate(label);
            String innerText = label->innerText();
            // Only use the <label> text if there's no ARIA override.
            if (!innerText.isEmpty() && !ariaAccessibilityDescription())
                textOrder.append(AccessibilityText(innerText, LabelByElementText, labelObject));
            return;
        }
    }
    
    AccessibilityObject* titleUIElement = this->titleUIElement();
    if (titleUIElement)
        textOrder.append(AccessibilityText(String(), LabelByElementText, titleUIElement));
}

void AccessibilityNodeObject::alternativeText(Vector<AccessibilityText>& textOrder) const
{
    if (isWebArea()) {
        String webAreaText = alternativeTextForWebArea();
        if (!webAreaText.isEmpty())
            textOrder.append(AccessibilityText(webAreaText, AlternativeText));
        return;
    }
    
    ariaLabeledByText(textOrder);
    
    const AtomicString& ariaLabel = getAttribute(aria_labelAttr);
    if (!ariaLabel.isEmpty())
        textOrder.append(AccessibilityText(ariaLabel, AlternativeText));
    
    if (isImage() || isInputImage() || isNativeImage() || isCanvas()) {
        // Images should use alt as long as the attribute is present, even if empty.
        // Otherwise, it should fallback to other methods, like the title attribute.
        const AtomicString& alt = getAttribute(altAttr);
        if (!alt.isNull())
            textOrder.append(AccessibilityText(alt, AlternativeText));
    }
    
    Node* node = this->node();
    if (!node)
        return;
    
    // The fieldset element derives its alternative text from the first associated legend element if one is available.
    if (isHTMLFieldSetElement(node)) {
        AccessibilityObject* object = axObjectCache()->getOrCreate(toHTMLFieldSetElement(node)->legend());
        if (object && !object->isHidden())
            textOrder.append(AccessibilityText(accessibleNameForNode(object->node()), AlternativeText));
    }
    
#if ENABLE(SVG)
    // SVG elements all can have a <svg:title> element inside which should act as the descriptive text.
    if (node->isSVGElement())
        textOrder.append(AccessibilityText(toSVGElement(node)->title(), AlternativeText));
#endif
    
#if ENABLE(MATHML)
    if (node->isElementNode() && toElement(node)->isMathMLElement())
        textOrder.append(AccessibilityText(getAttribute(MathMLNames::alttextAttr), AlternativeText));
#endif
}

void AccessibilityNodeObject::visibleText(Vector<AccessibilityText>& textOrder) const
{
    Node* node = this->node();
    if (!node)
        return;
    
    bool isInputTag = isHTMLInputElement(node);
    if (isInputTag) {
        HTMLInputElement* input = toHTMLInputElement(node);
        if (input->isTextButton()) {
            textOrder.append(AccessibilityText(input->valueWithDefault(), VisibleText));
            return;
        }
    }
    
    // If this node isn't rendered, there's no inner text we can extract from a select element.
    if (!isAccessibilityRenderObject() && node->hasTagName(selectTag))
        return;
    
    bool useTextUnderElement = false;
    
    switch (roleValue()) {
    case PopUpButtonRole:
        // Native popup buttons should not use their button children's text as a title. That value is retrieved through stringValue().
        if (node->hasTagName(selectTag))
            break;
    case ButtonRole:
    case ToggleButtonRole:
    case CheckBoxRole:
    case ListBoxOptionRole:
    // MacOS does not expect native <li> elements to expose label information, it only expects leaf node elements to do that.
#if !PLATFORM(MAC)
    case ListItemRole:
#endif
    case MenuButtonRole:
    case MenuItemRole:
    case MenuItemCheckboxRole:
    case MenuItemRadioRole:
    case RadioButtonRole:
    case TabRole:
    case ProgressIndicatorRole:
        useTextUnderElement = true;
        break;
    default:
        break;
    }
    
    // If it's focusable but it's not content editable or a known control type, then it will appear to
    // the user as a single atomic object, so we should use its text as the default title.
    if (isHeading() || isLink())
        useTextUnderElement = true;
    else if (isGenericFocusableElement()) {
        // If a node uses a negative tabindex, do not expose it as a generic focusable element, because keyboard focus management
        // will never land on this specific element.
        useTextUnderElement = !(node && node->isElementNode() && toElement(node)->tabIndex() < 0);
    }
    
    if (useTextUnderElement) {
        AccessibilityTextUnderElementMode mode;
        
        // Headings often include links as direct children. Those links need to be included in text under element.
        if (isHeading())
            mode.includeFocusableContent = true;

        String text = textUnderElement(mode);
        if (!text.isEmpty())
            textOrder.append(AccessibilityText(text, ChildrenText));
    }
}

void AccessibilityNodeObject::helpText(Vector<AccessibilityText>& textOrder) const
{
    const AtomicString& ariaHelp = getAttribute(aria_helpAttr);
    if (!ariaHelp.isEmpty())
        textOrder.append(AccessibilityText(ariaHelp, HelpText));
    
    String describedBy = ariaDescribedByAttribute();
    if (!describedBy.isEmpty())
        textOrder.append(AccessibilityText(describedBy, SummaryText));
    
    // Add help type text that is derived from ancestors.
    for (Node* curr = node(); curr; curr = curr->parentNode()) {
        const AtomicString& summary = getAttribute(summaryAttr);
        if (!summary.isEmpty())
            textOrder.append(AccessibilityText(summary, SummaryText));
        
        // The title attribute should be used as help text unless it is already being used as descriptive text.
        const AtomicString& title = getAttribute(titleAttr);
        if (!title.isEmpty())
            textOrder.append(AccessibilityText(title, TitleTagText));
        
        // Only take help text from an ancestor element if its a group or an unknown role. If help was
        // added to those kinds of elements, it is likely it was meant for a child element.
        AccessibilityObject* axObj = axObjectCache()->getOrCreate(curr);
        if (!axObj)
            return;
        
        AccessibilityRole role = axObj->roleValue();
        if (role != GroupRole && role != UnknownRole)
            break;
    }
}

void AccessibilityNodeObject::accessibilityText(Vector<AccessibilityText>& textOrder)
{
    titleElementText(textOrder);
    alternativeText(textOrder);
    visibleText(textOrder);
    helpText(textOrder);
    
    String placeholder = placeholderValue();
    if (!placeholder.isEmpty())
        textOrder.append(AccessibilityText(placeholder, PlaceholderText));
}
    
void AccessibilityNodeObject::ariaLabeledByText(Vector<AccessibilityText>& textOrder) const
{
    String ariaLabeledBy = ariaLabeledByAttribute();
    if (!ariaLabeledBy.isEmpty()) {
        Vector<Element*> elements;
        ariaLabeledByElements(elements);
        
        Vector<RefPtr<AccessibilityObject>> axElements;
        unsigned length = elements.size();
        for (unsigned k = 0; k < length; k++) {
            RefPtr<AccessibilityObject> axElement = axObjectCache()->getOrCreate(elements[k]);
            axElements.append(axElement);
        }
        
        textOrder.append(AccessibilityText(ariaLabeledBy, AlternativeText, axElements));
    }
}
    
String AccessibilityNodeObject::alternativeTextForWebArea() const
{
    // The WebArea description should follow this order:
    //     aria-label on the <html>
    //     title on the <html>
    //     <title> inside the <head> (of it was set through JS)
    //     name on the <html>
    // For iframes:
    //     aria-label on the <iframe>
    //     title on the <iframe>
    //     name on the <iframe>
    
    Document* document = this->document();
    if (!document)
        return String();
    
    // Check if the HTML element has an aria-label for the webpage.
    if (Element* documentElement = document->documentElement()) {
        const AtomicString& ariaLabel = documentElement->getAttribute(aria_labelAttr);
        if (!ariaLabel.isEmpty())
            return ariaLabel;
    }
    
    Node* owner = document->ownerElement();
    if (owner) {
        if (owner->hasTagName(frameTag) || owner->hasTagName(iframeTag)) {
            const AtomicString& title = toElement(owner)->getAttribute(titleAttr);
            if (!title.isEmpty())
                return title;
            return toElement(owner)->getNameAttribute();
        }
        if (owner->isHTMLElement())
            return toHTMLElement(owner)->getNameAttribute();
    }
    
    String documentTitle = document->title();
    if (!documentTitle.isEmpty())
        return documentTitle;
    
    owner = document->body();
    if (owner && owner->isHTMLElement())
        return toHTMLElement(owner)->getNameAttribute();
    
    return String();
}
    
String AccessibilityNodeObject::accessibilityDescription() const
{
    // Static text should not have a description, it should only have a stringValue.
    if (roleValue() == StaticTextRole)
        return String();

    String ariaDescription = ariaAccessibilityDescription();
    if (!ariaDescription.isEmpty())
        return ariaDescription;

    if (isImage() || isInputImage() || isNativeImage() || isCanvas()) {
        // Images should use alt as long as the attribute is present, even if empty.                    
        // Otherwise, it should fallback to other methods, like the title attribute.                    
        const AtomicString& alt = getAttribute(altAttr);
        if (!alt.isNull())
            return alt;
    }

#if ENABLE(SVG)
    // SVG elements all can have a <svg:title> element inside which should act as the descriptive text.
    if (m_node && m_node->isSVGElement())
        return toSVGElement(m_node)->title();
#endif
    
#if ENABLE(MATHML)
    if (m_node && m_node->isElementNode() && toElement(m_node)->isMathMLElement())
        return getAttribute(MathMLNames::alttextAttr);
#endif

    // An element's descriptive text is comprised of title() (what's visible on the screen) and accessibilityDescription() (other descriptive text).
    // Both are used to generate what a screen reader speaks.                                                           
    // If this point is reached (i.e. there's no accessibilityDescription) and there's no title(), we should fallback to using the title attribute.
    // The title attribute is normally used as help text (because it is a tooltip), but if there is nothing else available, this should be used (according to ARIA).
    if (title().isEmpty())
        return getAttribute(titleAttr);

    return String();
}

String AccessibilityNodeObject::helpText() const
{
    Node* node = this->node();
    if (!node)
        return String();
    
    const AtomicString& ariaHelp = getAttribute(aria_helpAttr);
    if (!ariaHelp.isEmpty())
        return ariaHelp;
    
    String describedBy = ariaDescribedByAttribute();
    if (!describedBy.isEmpty())
        return describedBy;
    
    String description = accessibilityDescription();
    for (Node* curr = node; curr; curr = curr->parentNode()) {
        if (curr->isHTMLElement()) {
            const AtomicString& summary = toElement(curr)->getAttribute(summaryAttr);
            if (!summary.isEmpty())
                return summary;
            
            // The title attribute should be used as help text unless it is already being used as descriptive text.
            const AtomicString& title = toElement(curr)->getAttribute(titleAttr);
            if (!title.isEmpty() && description != title)
                return title;
        }
        
        // Only take help text from an ancestor element if its a group or an unknown role. If help was 
        // added to those kinds of elements, it is likely it was meant for a child element.
        AccessibilityObject* axObj = axObjectCache()->getOrCreate(curr);
        if (axObj) {
            AccessibilityRole role = axObj->roleValue();
            if (role != GroupRole && role != UnknownRole)
                break;
        }
    }
    
    return String();
}
    
unsigned AccessibilityNodeObject::hierarchicalLevel() const
{
    Node* node = this->node();
    if (!node || !node->isElementNode())
        return 0;
    Element* element = toElement(node);
    String ariaLevel = element->getAttribute(aria_levelAttr);
    if (!ariaLevel.isEmpty())
        return ariaLevel.toInt();
    
    // Only tree item will calculate its level through the DOM currently.
    if (roleValue() != TreeItemRole)
        return 0;
    
    // Hierarchy leveling starts at 1, to match the aria-level spec.
    // We measure tree hierarchy by the number of groups that the item is within.
    unsigned level = 1;
    for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) {
        AccessibilityRole parentRole = parent->roleValue();
        if (parentRole == GroupRole)
            level++;
        else if (parentRole == TreeRole)
            break;
    }
    
    return level;
}

// When building the textUnderElement for an object, determine whether or not
// we should include the inner text of this given descendant object or skip it.
static bool shouldUseAccessiblityObjectInnerText(AccessibilityObject* obj, AccessibilityTextUnderElementMode mode)
{
    // Do not use any heuristic if we are explicitly asking to include all the children.
    if (mode.childrenInclusion == AccessibilityTextUnderElementMode::TextUnderElementModeIncludeAllChildren)
        return true;

    // Consider this hypothetical example:
    // <div tabindex=0>
    //   <h2>
    //     Table of contents
    //   </h2>
    //   <a href="#start">Jump to start of book</a>
    //   <ul>
    //     <li><a href="#1">Chapter 1</a></li>
    //     <li><a href="#1">Chapter 2</a></li>
    //   </ul>
    // </div>
    //
    // The goal is to return a reasonable title for the outer container div, because
    // it's focusable - but without making its title be the full inner text, which is
    // quite long. As a heuristic, skip links, controls, and elements that are usually
    // containers with lots of children.

    if (equalIgnoringCase(obj->getAttribute(aria_hiddenAttr), "true"))
        return false;
    
    // If something doesn't expose any children, then we can always take the inner text content.
    // This is what we want when someone puts an <a> inside a <button> for example.
    if (obj->isDescendantOfBarrenParent())
        return true;
    
    // Skip focusable children, so we don't include the text of links and controls.
    if (obj->canSetFocusAttribute() && !mode.includeFocusableContent)
        return false;

    // Skip big container elements like lists, tables, etc.
    if (obj->isList() || obj->isAccessibilityTable() || obj->isTree() || obj->isCanvas())
        return false;

    return true;
}

static bool shouldAddSpaceBeforeAppendingNextElement(StringBuilder& builder, String& childText)
{
    if (!builder.length() || !childText.length())
        return false;

    // We don't need to add an additional space before or after a line break.
    return !(isHTMLLineBreak(childText[0]) || isHTMLLineBreak(builder[builder.length() - 1]));
}

String AccessibilityNodeObject::textUnderElement(AccessibilityTextUnderElementMode mode) const
{
    Node* node = this->node();
    if (node && node->isTextNode())
        return toText(node)->wholeText();

    // The render tree should be stable before going ahead. Otherwise, further uses of the
    // TextIterator will force a layout update, potentially altering the accessibility tree
    // and leading to crashes in the loop that computes the result text from the children.
    ASSERT(!document()->renderView()->layoutState());
    ASSERT(!document()->childNeedsStyleRecalc());

    StringBuilder builder;
    for (AccessibilityObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!shouldUseAccessiblityObjectInnerText(child, mode))
            continue;

        if (child->isAccessibilityNodeObject()) {
            Vector<AccessibilityText> textOrder;
            toAccessibilityNodeObject(child)->alternativeText(textOrder);
            if (textOrder.size() > 0 && textOrder[0].text.length()) {
                if (shouldAddSpaceBeforeAppendingNextElement(builder, textOrder[0].text))
                    builder.append(' ');
                builder.append(textOrder[0].text);
                continue;
            }
        }

        String childText = child->textUnderElement(mode);
        if (childText.length()) {
            if (shouldAddSpaceBeforeAppendingNextElement(builder, childText))
                builder.append(' ');
            builder.append(childText);
        }
    }

    return builder.toString().stripWhiteSpace().simplifyWhiteSpace(isHTMLSpaceButNotLineBreak);
}

String AccessibilityNodeObject::title() const
{
    Node* node = this->node();
    if (!node)
        return String();

    bool isInputTag = isHTMLInputElement(node);
    if (isInputTag) {
        HTMLInputElement* input = toHTMLInputElement(node);
        if (input->isTextButton())
            return input->valueWithDefault();
    }

    if (isInputTag || AccessibilityObject::isARIAInput(ariaRoleAttribute()) || isControl()) {
        HTMLLabelElement* label = labelForElement(toElement(node));
        // Use the label text as the title if 1) the title element is NOT an exposed element and 2) there's no ARIA override.
        if (label && !exposesTitleUIElement() && !ariaAccessibilityDescription().length())
            return label->innerText();
    }

    // If this node isn't rendered, there's no inner text we can extract from a select element.
    if (!isAccessibilityRenderObject() && node->hasTagName(selectTag))
        return String();

    switch (roleValue()) {
    case PopUpButtonRole:
        // Native popup buttons should not use their button children's text as a title. That value is retrieved through stringValue().
        if (node->hasTagName(selectTag))
            return String();
    case ButtonRole:
    case ToggleButtonRole:
    case CheckBoxRole:
    case ListBoxOptionRole:
    case ListItemRole:
    case MenuButtonRole:
    case MenuItemRole:
    case MenuItemCheckboxRole:
    case MenuItemRadioRole:
    case RadioButtonRole:
    case TabRole:
        return textUnderElement();
    // SVGRoots should not use the text under itself as a title. That could include the text of objects like <text>.
    case SVGRootRole:
        return String();
    default:
        break;
    }

    if (isLink())
        return textUnderElement();
    if (isHeading())
        return textUnderElement(AccessibilityTextUnderElementMode(AccessibilityTextUnderElementMode::TextUnderElementModeSkipIgnoredChildren, true));

    // If it's focusable but it's not content editable or a known control type, then it will appear to
    // the user as a single atomic object, so we should use its text as the default title.                              
    if (isGenericFocusableElement()) {
        // If a node uses a negative tabindex, do not expose it as a generic focusable element, because keyboard focus management
        // will never land on this specific element.
        Node* node = this->node();
        if (node && node->isElementNode() && toElement(node)->tabIndex() < 0)
            return String();
        
        return textUnderElement();
    }

    return String();
}

String AccessibilityNodeObject::text() const
{
    // If this is a user defined static text, use the accessible name computation.                                      
    if (ariaRoleAttribute() == StaticTextRole)
        return ariaAccessibilityDescription();

    if (!isTextControl())
        return String();

    Node* node = this->node();
    if (!node)
        return String();

    if (isNativeTextControl() && (isHTMLTextAreaElement(node) || isHTMLInputElement(node)))
        return toHTMLTextFormControlElement(node)->value();

    if (!node->isElementNode())
        return String();

    return toElement(node)->innerText();
}

String AccessibilityNodeObject::stringValue() const
{
    Node* node = this->node();
    if (!node)
        return String();

    if (ariaRoleAttribute() == StaticTextRole) {
        String staticText = text();
        if (!staticText.length())
            staticText = textUnderElement();
        return staticText;
    }

    if (node->isTextNode())
        return textUnderElement();

    if (node->hasTagName(selectTag)) {
        HTMLSelectElement* selectElement = toHTMLSelectElement(node);
        int selectedIndex = selectElement->selectedIndex();
        const Vector<HTMLElement*> listItems = selectElement->listItems();
        if (selectedIndex >= 0 && static_cast<size_t>(selectedIndex) < listItems.size()) {
            const AtomicString& overriddenDescription = listItems[selectedIndex]->fastGetAttribute(aria_labelAttr);
            if (!overriddenDescription.isNull())
                return overriddenDescription;
        }
        if (!selectElement->multiple())
            return selectElement->value();
        return String();
    }

    if (isTextControl())
        return text();

    // FIXME: We might need to implement a value here for more types
    // FIXME: It would be better not to advertise a value at all for the types for which we don't implement one;
    // this would require subclassing or making accessibilityAttributeNames do something other than return a
    // single static array.
    return String();
}

void AccessibilityNodeObject::colorValue(int& r, int& g, int& b) const
{
    r = 0;
    g = 0;
    b = 0;

    if (!isColorWell())
        return;

    if (!node() || !isHTMLInputElement(node()))
        return;

    HTMLInputElement* input = toHTMLInputElement(node());
    const AtomicString& type = input->getAttribute(typeAttr);
    if (!equalIgnoringCase(type, "color"))
        return;

    // HTMLInputElement::value always returns a string parseable by Color().
    Color color(input->value());
    r = color.red();
    g = color.green();
    b = color.blue();
}

// This function implements the ARIA accessible name as described by the Mozilla                                        
// ARIA Implementer's Guide.                                                                                            
static String accessibleNameForNode(Node* node)
{
    if (!node->isHTMLElement())
        return String();
    
    HTMLElement* element = toHTMLElement(node);
    
    const AtomicString& ariaLabel = element->fastGetAttribute(aria_labelAttr);
    if (!ariaLabel.isEmpty())
        return ariaLabel;
    
    const AtomicString& alt = element->fastGetAttribute(altAttr);
    if (!alt.isEmpty())
        return alt;
    
    if (isHTMLInputElement(node))
        return toHTMLInputElement(node)->value();
    
    // If the node can be turned into an AX object, we can use standard name computation rules.
    // If however, the node cannot (because there's no renderer e.g.) fallback to using the basic text underneath.
    AccessibilityObject* axObject = node->document().axObjectCache()->getOrCreate(node);
    String text;
    if (axObject)
        text = axObject->textUnderElement();
    else if (node->isElementNode())
        text = toElement(node)->innerText();
    
    if (!text.isEmpty())
        return text;
    
    const AtomicString& title = element->fastGetAttribute(titleAttr);
    if (!title.isEmpty())
        return title;
    
    return String();
}

String AccessibilityNodeObject::accessibilityDescriptionForElements(Vector<Element*> &elements) const
{
    StringBuilder builder;
    unsigned size = elements.size();
    for (unsigned i = 0; i < size; ++i) {
        if (i)
            builder.append(' ');
        
        builder.append(accessibleNameForNode(elements[i]));
    }
    return builder.toString();
}

String AccessibilityNodeObject::ariaDescribedByAttribute() const
{
    Vector<Element*> elements;
    elementsFromAttribute(elements, aria_describedbyAttr);
    
    return accessibilityDescriptionForElements(elements);
}

void AccessibilityNodeObject::elementsFromAttribute(Vector<Element*>& elements, const QualifiedName& attribute) const
{
    Node* node = this->node();
    if (!node || !node->isElementNode())
        return;

    TreeScope& treeScope = node->treeScope();

    String idList = getAttribute(attribute).string();
    if (idList.isEmpty())
        return;

    idList.replace('\n', ' ');
    Vector<String> idVector;
    idList.split(' ', idVector);

    unsigned size = idVector.size();
    for (unsigned i = 0; i < size; ++i) {
        AtomicString idName(idVector[i]);
        Element* idElement = treeScope.getElementById(idName);
        if (idElement)
            elements.append(idElement);
    }
}


void AccessibilityNodeObject::ariaLabeledByElements(Vector<Element*>& elements) const
{
    elementsFromAttribute(elements, aria_labeledbyAttr);
    if (!elements.size())
        elementsFromAttribute(elements, aria_labelledbyAttr);
}


String AccessibilityNodeObject::ariaLabeledByAttribute() const
{
    Vector<Element*> elements;
    ariaLabeledByElements(elements);

    return accessibilityDescriptionForElements(elements);
}

bool AccessibilityNodeObject::canSetFocusAttribute() const
{
    Node* node = this->node();
    if (!node)
        return false;

    if (isWebArea())
        return true;
    
    // NOTE: It would be more accurate to ask the document whether setFocusedElement() would
    // do anything. For example, setFocusedElement() will do nothing if the current focused
    // node will not relinquish the focus.
    if (!node)
        return false;

    if (!node->isElementNode())
        return false;

    Element* element = toElement(node);

    if (element->isDisabledFormControl())
        return false;

    return element->supportsFocus();
}

AccessibilityRole AccessibilityNodeObject::determineAriaRoleAttribute() const
{
    const AtomicString& ariaRole = getAttribute(roleAttr);
    if (ariaRole.isNull() || ariaRole.isEmpty())
        return UnknownRole;
    
    AccessibilityRole role = ariaRoleToWebCoreRole(ariaRole);

    // ARIA states if an item can get focus, it should not be presentational.
    if (role == PresentationalRole && canSetFocusAttribute())
        return UnknownRole;

    if (role == ButtonRole)
        role = buttonRoleType();

    if (role == TextAreaRole && !ariaIsMultiline())
        role = TextFieldRole;

    role = remapAriaRoleDueToParent(role);
    
    // Presentational roles are invalidated by the presence of ARIA attributes.
    if (role == PresentationalRole && supportsARIAAttributes())
        role = UnknownRole;
    
    if (role)
        return role;

    return UnknownRole;
}

AccessibilityRole AccessibilityNodeObject::ariaRoleAttribute() const
{
    return m_ariaRole;
}

AccessibilityRole AccessibilityNodeObject::remapAriaRoleDueToParent(AccessibilityRole role) const
{
    // Some objects change their role based on their parent.
    // However, asking for the unignoredParent calls accessibilityIsIgnored(), which can trigger a loop. 
    // While inside the call stack of creating an element, we need to avoid accessibilityIsIgnored().
    // https://bugs.webkit.org/show_bug.cgi?id=65174

    if (role != ListBoxOptionRole && role != MenuItemRole)
        return role;
    
    for (AccessibilityObject* parent = parentObject(); parent && !parent->accessibilityIsIgnored(); parent = parent->parentObject()) {
        AccessibilityRole parentAriaRole = parent->ariaRoleAttribute();

        // Selects and listboxes both have options as child roles, but they map to different roles within WebCore.
        if (role == ListBoxOptionRole && parentAriaRole == MenuRole)
            return MenuItemRole;
        // An aria "menuitem" may map to MenuButton or MenuItem depending on its parent.
        if (role == MenuItemRole && parentAriaRole == GroupRole)
            return MenuButtonRole;
        
        // If the parent had a different role, then we don't need to continue searching up the chain.
        if (parentAriaRole)
            break;
    }
    
    return role;
}   

// If you call node->rendererIsEditable() since that will return true if an ancestor is editable.
// This only returns true if this is the element that actually has the contentEditable attribute set.
bool AccessibilityNodeObject::hasContentEditableAttributeSet() const
{
    if (!hasAttribute(contenteditableAttr))
        return false;
    const AtomicString& contentEditableValue = getAttribute(contenteditableAttr);
    // Both "true" (case-insensitive) and the empty string count as true.
    return contentEditableValue.isEmpty() || equalIgnoringCase(contentEditableValue, "true");
}

bool AccessibilityNodeObject::canSetSelectedAttribute() const
{
    // Elements that can be selected
    switch (roleValue()) {
    case CellRole:
    case RadioButtonRole:
    case RowHeaderRole:
    case RowRole:
    case TabListRole:
    case TabRole:
    case TreeGridRole:
    case TreeItemRole:
    case TreeRole:
        return isEnabled();
    default:
        return false;
    }
}

} // namespace WebCore
