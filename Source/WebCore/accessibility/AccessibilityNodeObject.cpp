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
#include "HTMLPlugInImageElement.h"
#include "HTMLSelectElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLTextFormControlElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "LocalizedStrings.h"
#include "MathMLNames.h"
#include "NodeList.h"
#include "Page.h"
#include "ProgressTracker.h"
#include "Text.h"
#include "TextControlInnerElements.h"
#include "TextIterator.h"
#include "Widget.h"
#include "htmlediting.h"
#include "visible_units.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

AccessibilityNodeObject::AccessibilityNodeObject(Node* node)
    : AccessibilityObject()
    , m_ariaRole(UnknownRole)
    , m_childrenDirty(false)
    , m_roleForMSAA(UnknownRole)
    , m_node(node)
{
}

AccessibilityNodeObject::~AccessibilityNodeObject()
{
    ASSERT(isDetached());
}

void AccessibilityNodeObject::init()
{
    m_role = determineAccessibilityRole();
}

PassRefPtr<AccessibilityNodeObject> AccessibilityNodeObject::create(Node* node)
{
    AccessibilityNodeObject* obj = new AccessibilityNodeObject(node);
    obj->init();
    return adoptRef(obj);
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

    axObjectCache()->postNotification(this, document(), AXObjectCache::AXChildrenChanged, true);

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
            axObjectCache()->postNotification(parent, parent->document(), AXObjectCache::AXLiveRegionChanged, true);
        
        // If this element is an ARIA text control, notify the AT of changes.
        if (parent->isARIATextControl() && !parent->isNativeTextControl() && !parent->node()->rendererIsEditable())
            axObjectCache()->postNotification(parent, parent->document(), AXObjectCache::AXValueChanged, true);
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

void AccessibilityNodeObject::setNode(Node* node)
{
    m_node = node;
}

Document* AccessibilityNodeObject::document() const
{
    if (!node())
        return 0;
    return node()->document();
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
    if (node()->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node());
        if (input->isCheckbox())
            return CheckBoxRole;
        if (input->isRadioButton())
            return RadioButtonRole;
        if (input->isTextButton())
            return buttonRoleType();
        if (input->isRangeControl())
            return SliderRole;
        return TextFieldRole;
    }
    if (node()->hasTagName(selectTag)) {
        HTMLSelectElement* selectElement = toHTMLSelectElement(node());
        return selectElement->multiple() ? ListBoxRole : PopUpButtonRole;
    }
    if (node()->hasTagName(textareaTag))
        return TextAreaRole;
    if (headingLevel())
        return HeadingRole;
    if (node()->hasTagName(divTag))
        return DivRole;
    if (node()->hasTagName(pTag))
        return ParagraphRole;
    if (node()->hasTagName(labelTag))
        return LabelRole;
    if (node()->isFocusable())
        return GroupRole;
    
    return UnknownRole;
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
    
    for (Node* child = m_node->firstChild(); child; child = child->nextSibling()) {
        RefPtr<AccessibilityObject> obj = axObjectCache()->getOrCreate(child);
        obj->clearChildren();
        if (obj->accessibilityIsIgnored()) {
            AccessibilityChildrenVector children = obj->children();
            size_t length = children.size();
            for (size_t i = 0; i < length; ++i)
                m_children.append(children[i]);
        } else {
            ASSERT(obj->parentObject() == this);
            m_children.append(obj);
        }
    }
}

bool AccessibilityNodeObject::canHaveChildren() const
{
    // If this is an AccessibilityRenderObject, then it's okay if this object
    // doesn't have a node - there are some renderers that don't have associated
    // nodes, like scroll areas and css-generated text.
    if (!node() && !isAccessibilityRenderObject())
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
        return false;
    default:
        return true;
    }
}

bool AccessibilityNodeObject::accessibilityIsIgnored() const
{
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

    // If it has any children that are elements, we'll assume it might be fallback
    // content. If it has no children or its only children are not elements
    // (e.g. just text nodes), it doesn't have fallback content.
    for (Node* child = node->firstChild(); child; child = child->nextSibling()) {
        if (child->isElementNode())
            return true;
    }

    return false;
}

bool AccessibilityNodeObject::isWebArea() const
{
    return roleValue() == WebAreaRole;
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

    if (node->hasTagName(textareaTag))
        return true;

    if (node->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
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

    if (node->hasTagName(imgTag))
        return true;

    if (node->hasTagName(appletTag) || node->hasTagName(embedTag) || node->hasTagName(objectTag))
        return true;

    if (node->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
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
 
    if (roleValue() == ButtonRole && node->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
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
    return roleValue() == MenuItemRole;
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
    if (equalIgnoringCase(getAttribute(aria_disabledAttr), "true"))
        return false;

    Node* node = this->node();
    if (!node || !node->isElementNode())
        return true;

    return toElement(node)->isEnabledFormControl();
}

bool AccessibilityNodeObject::isIndeterminate() const
{
    Node* node = this->node();
    if (!node)
        return false;

    HTMLInputElement* inputElement = node->toInputElement();
    if (!inputElement)
        return false;

    return inputElement->isIndeterminate();
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

    return node->active();
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
    AccessibilityRole ariaRole = ariaRoleAttribute();
    if (ariaRole == RadioButtonRole || ariaRole == CheckBoxRole) {
        if (equalIgnoringCase(getAttribute(aria_checkedAttr), "true"))
            return true;
        return false;
    }

    // Otherwise it's not checked
    return false;
}

bool AccessibilityNodeObject::isHovered() const
{
    Node* node = this->node();
    if (!node)
        return false;

    return node->hovered();
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

    if (node->hasTagName(textareaTag))
        return static_cast<HTMLTextAreaElement*>(node)->readOnly();

    if (node->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
        if (input->isTextField())
            return input->readOnly();
    }

    return !node->rendererIsEditable();
}

bool AccessibilityNodeObject::isRequired() const
{
    if (equalIgnoringCase(getAttribute(aria_requiredAttr), "true"))
        return true;

    Node* n = this->node();
    if (n && (n->isElementNode() && toElement(n)->isFormControlElement()))
        return static_cast<HTMLFormControlElement*>(n)->required();

    return false;
}

int AccessibilityNodeObject::headingLevel() const
{
    // headings can be in block flow and non-block flow
    Node* node = this->node();
    if (!node)
        return false;

    if (ariaRoleAttribute() == HeadingRole)
        return getAttribute(aria_levelAttr).toInt();

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
    if (!isARIARange())
        return String();

    return getAttribute(aria_valuetextAttr).string();
}

bool AccessibilityNodeObject::isARIARange() const
{
    switch (m_ariaRole) {
    case ProgressIndicatorRole:
    case SliderRole:
    case ScrollBarRole:
    case SpinButtonRole:
        return true;
    default:
        return false;
    }
}

float AccessibilityNodeObject::valueForRange() const
{
    if (node() && node()->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node());
        if (input->isRangeControl())
            return input->valueAsNumber();
    }

    if (!isARIARange())
        return 0.0f;

    return getAttribute(aria_valuenowAttr).toFloat();
}

float AccessibilityNodeObject::maxValueForRange() const
{
    if (node() && node()->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node());
        if (input->isRangeControl())
            return input->maximum();
    }

    if (!isARIARange())
        return 0.0f;

    return getAttribute(aria_valuemaxAttr).toFloat();
}

float AccessibilityNodeObject::minValueForRange() const
{
    if (node() && node()->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node());
        if (input->isRangeControl())
            return input->minimum();
    }

    if (!isARIARange())
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
        if (node->hasTagName(aTag) || (node->renderer() && cache->getOrCreate(node->renderer())->isAnchor()))
            return toElement(node);
    }

    return 0;
}

Element* AccessibilityNodeObject::actionElement() const
{
    Node* node = this->node();
    if (!node)
        return 0;

    if (node->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
        if (!input->disabled() && (isCheckboxOrRadio() || input->isTextButton()))
            return input;
    } else if (node->hasTagName(buttonTag))
        return toElement(node);

    if (isFileUploadButton())
        return toElement(node);
            
    if (AccessibilityObject::isARIAInput(ariaRoleAttribute()))
        return toElement(node);

    if (isImageButton())
        return toElement(node);
    
    if (node->hasTagName(selectTag))
        return toElement(node);

    switch (roleValue()) {
    case ButtonRole:
    case PopUpButtonRole:
    case ToggleButtonRole:
    case TabRole:
    case MenuItemRole:
    case ListItemRole:
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
    while (node && !node->isElementNode())
        node = node->parentNode();

    if (!node)
        return 0;

    // FIXME: Do the continuation search like anchorElement does
    for (Element* element = toElement(node); element; element = element->parentElement()) {
        if (element->getAttributeEventListener(eventNames().clickEvent) || element->getAttributeEventListener(eventNames().mousedownEvent) || element->getAttributeEventListener(eventNames().mouseupEvent))
            return element;
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
    alterSliderValue(true);
}

void AccessibilityNodeObject::decrement()
{
    alterSliderValue(false);
}

void AccessibilityNodeObject::changeValueByStep(bool increase)
{
    float step = stepValueForRange();
    float value = valueForRange();

    value += increase ? step : -step;

    setValue(String::number(value));

    axObjectCache()->postNotification(node(), AXObjectCache::AXValueChanged, true);
}

void AccessibilityNodeObject::changeValueByPercent(float percentChange)
{
    float range = maxValueForRange() - minValueForRange();
    float value = valueForRange();

    value += range * (percentChange / 100);
    setValue(String::number(value));

    axObjectCache()->postNotification(node(), AXObjectCache::AXValueChanged, true);
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

    return true;
}

HTMLLabelElement* AccessibilityNodeObject::labelForElement(Element* element) const
{
    RefPtr<NodeList> list = element->document()->getElementsByTagName("label");
    unsigned len = list->length();
    for (unsigned i = 0; i < len; i++) {
        if (list->item(i)->hasTagName(labelTag)) {
            HTMLLabelElement* label = static_cast<HTMLLabelElement*>(list->item(i));
            if (label->control() == element)
                return label;
        }
    }

    return 0;
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
    for (Node* sibling = node->parentNode()->firstChild(); sibling; sibling = sibling->nextSibling()) {
        if (sibling->isElementNode()) {
            const AtomicString& siblingAriaRole = toElement(sibling)->getAttribute(roleAttr);
            if (equalIgnoringCase(siblingAriaRole, role))
                return toElement(sibling);
        }
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
        if (menuItemAX->isMenuButton())
            return menuItemAX;
    }
    return 0;
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

#if ENABLE(MATHML)
    Node* node = this->node();
    if (node && node->isElementNode() && toElement(node)->isMathMLElement())
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
    
    // Hierarchy leveling starts at 0.
    // We measure tree hierarchy by the number of groups that the item is within.
    unsigned level = 0;
    for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) {
        AccessibilityRole parentRole = parent->roleValue();
        if (parentRole == GroupRole)
            level++;
        else if (parentRole == TreeRole)
            break;
    }
    
    return level;
}

String AccessibilityNodeObject::textUnderElement() const
{
    Node* node = this->node();
    if (!node)
        return String();

    // Note: TextIterator doesn't return any text for nodes that don't have renderers.
    // If this could be fixed, it'd be more accurate use TextIterator here.
    if (node->isElementNode())
        return toElement(node)->innerText();

    return String();
}

String AccessibilityNodeObject::title() const
{
    Node* node = this->node();
    if (!node)
        return String();

    bool isInputTag = node->hasTagName(inputTag);
    if (isInputTag) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
        if (input->isTextButton())
            return input->valueWithDefault();
    }

    if (isInputTag || AccessibilityObject::isARIAInput(ariaRoleAttribute()) || isControl()) {
        HTMLLabelElement* label = labelForElement(toElement(node));
        if (label && !exposesTitleUIElement())
            return label->innerText();
    }

    // If this node isn't rendered, there's no inner text we can extract from a select element.
    if (!isAccessibilityRenderObject() && node->hasTagName(selectTag))
        return String();

    switch (roleValue()) {
    case PopUpButtonRole:
    case ButtonRole:
    case ToggleButtonRole:
    case CheckBoxRole:
    case ListBoxOptionRole:
    case MenuButtonRole:
    case MenuItemRole:
    case RadioButtonRole:
    case TabRole:
        return textUnderElement();
    default:
        break;
    }

    if (isHeading() || isLink())
        return textUnderElement();

    // If it's focusable but it's not content editable or a known control type, then it will appear to                  
    // the user as a single atomic object, so we should use its text as the default title.                              
    if (isGenericFocusableElement())
        return textUnderElement();

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

    if (isNativeTextControl()) {
        if (node->hasTagName(textareaTag))
            return static_cast<HTMLTextAreaElement*>(node)->value();
        if (node->hasTagName(inputTag))
            return node->toInputElement()->value();
    }

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

// This function implements the ARIA accessible name as described by the Mozilla                                        
// ARIA Implementer's Guide.                                                                                            
static String accessibleNameForNode(Node* node)
{
    if (node->isTextNode())
        return toText(node)->data();

    if (node->hasTagName(inputTag))
        return static_cast<HTMLInputElement*>(node)->value();

    if (node->isHTMLElement()) {
        const AtomicString& alt = toHTMLElement(node)->getAttribute(altAttr);
        if (!alt.isEmpty())
            return alt;
    }

    return String();
}

String AccessibilityNodeObject::accessibilityDescriptionForElements(Vector<Element*> &elements) const
{
    StringBuilder builder;
    unsigned size = elements.size();
    for (unsigned i = 0; i < size; ++i) {
        Element* idElement = elements[i];

        builder.append(accessibleNameForNode(idElement));
        for (Node* n = idElement->firstChild(); n; n = n->traverseNextNode(idElement))
            builder.append(accessibleNameForNode(n));

        if (i != size - 1)
            builder.append(' ');
    }
    return builder.toString();
}

void AccessibilityNodeObject::elementsFromAttribute(Vector<Element*>& elements, const QualifiedName& attribute) const
{
    Node* node = this->node();
    if (!node || !node->isElementNode())
        return;

    TreeScope* scope = node->treeScope();
    if (!scope)
        return;

    String idList = getAttribute(attribute).string();
    if (idList.isEmpty())
        return;

    idList.replace('\n', ' ');
    Vector<String> idVector;
    idList.split(' ', idVector);

    unsigned size = idVector.size();
    for (unsigned i = 0; i < size; ++i) {
        AtomicString idName(idVector[i]);
        Element* idElement = scope->getElementById(idName);
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
    
    // NOTE: It would be more accurate to ask the document whether setFocusedNode() would
    // do anything. For example, setFocusedNode() will do nothing if the current focused
    // node will not relinquish the focus.
    if (!node)
        return false;

    if (node->isElementNode() && !toElement(node)->isEnabledFormControl())
        return false;

    return node->supportsFocus();
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

} // namespace WebCore
