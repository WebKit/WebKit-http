/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#include "HTMLFormControlElement.h"

#include "Attribute.h"
#include "ElementShadow.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLFieldSetElement.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLLegendElement.h"
#include "RenderBox.h"
#include "RenderTheme.h"
#include "ScriptEventListener.h"
#include "ValidationMessage.h"
#include "ValidityState.h"
#include <wtf/Vector.h>

namespace WebCore {

using namespace HTMLNames;
using namespace std;

HTMLFormControlElement::HTMLFormControlElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : LabelableElement(tagName, document)
    , m_disabled(false)
    , m_readOnly(false)
    , m_required(false)
    , m_valueMatchesRenderer(false)
    , m_ancestorDisabledState(AncestorDisabledStateUnknown)
    , m_dataListAncestorState(Unknown)
    , m_willValidateInitialized(false)
    , m_willValidate(true)
    , m_isValid(true)
    , m_wasChangedSinceLastFormControlChangeEvent(false)
    , m_hasAutofocused(false)
{
    setForm(form ? form : findFormAncestor());
    setHasCustomCallbacks();
}

HTMLFormControlElement::~HTMLFormControlElement()
{
}

void HTMLFormControlElement::willAddAuthorShadowRoot()
{
    ensureUserAgentShadowRoot();
}

String HTMLFormControlElement::formEnctype() const
{
    return FormSubmission::Attributes::parseEncodingType(fastGetAttribute(formenctypeAttr));
}

void HTMLFormControlElement::setFormEnctype(const String& value)
{
    setAttribute(formenctypeAttr, value);
}

String HTMLFormControlElement::formMethod() const
{
    return FormSubmission::Attributes::methodString(FormSubmission::Attributes::parseMethodType(fastGetAttribute(formmethodAttr)));
}

void HTMLFormControlElement::setFormMethod(const String& value)
{
    setAttribute(formmethodAttr, value);
}

bool HTMLFormControlElement::formNoValidate() const
{
    return fastHasAttribute(formnovalidateAttr);
}

void HTMLFormControlElement::updateAncestorDisabledState() const
{
    HTMLFieldSetElement* fieldSetAncestor = 0;
    ContainerNode* legendAncestor = 0;
    for (ContainerNode* ancestor = parentNode(); ancestor; ancestor = ancestor->parentNode()) {
        if (!legendAncestor && ancestor->hasTagName(legendTag))
            legendAncestor = ancestor;
        if (ancestor->hasTagName(fieldsetTag)) {
            fieldSetAncestor = static_cast<HTMLFieldSetElement*>(ancestor);
            break;
        }
    }
    m_ancestorDisabledState = (fieldSetAncestor && fieldSetAncestor->disabled() && !(legendAncestor && legendAncestor == fieldSetAncestor->legend())) ? AncestorDisabledStateDisabled : AncestorDisabledStateEnabled;
}

void HTMLFormControlElement::ancestorDisabledStateWasChanged()
{
    m_ancestorDisabledState = AncestorDisabledStateUnknown;
    disabledAttributeChanged();
}

void HTMLFormControlElement::parseAttribute(const Attribute& attribute)
{
    if (attribute.name() == formAttr)
        formAttributeChanged();
    else if (attribute.name() == disabledAttr) {
        bool oldDisabled = m_disabled;
        m_disabled = !attribute.isNull();
        if (oldDisabled != m_disabled)
            disabledAttributeChanged();
    } else if (attribute.name() == readonlyAttr) {
        bool oldReadOnly = m_readOnly;
        m_readOnly = !attribute.isNull();
        if (oldReadOnly != m_readOnly) {
            setNeedsWillValidateCheck();
            setNeedsStyleRecalc();
            if (renderer() && renderer()->style()->hasAppearance())
                renderer()->theme()->stateChanged(renderer(), ReadOnlyState);
        }
    } else if (attribute.name() == requiredAttr) {
        bool oldRequired = m_required;
        m_required = !attribute.isNull();
        if (oldRequired != m_required)
            requiredAttributeChanged();
    } else
        HTMLElement::parseAttribute(attribute);
}

void HTMLFormControlElement::disabledAttributeChanged()
{
    setNeedsWillValidateCheck();
    setNeedsStyleRecalc();
    if (renderer() && renderer()->style()->hasAppearance())
        renderer()->theme()->stateChanged(renderer(), EnabledState);
}

void HTMLFormControlElement::requiredAttributeChanged()
{
    setNeedsValidityCheck();
    // Style recalculation is needed because style selectors may include
    // :required and :optional pseudo-classes.
    setNeedsStyleRecalc();
}

static bool shouldAutofocus(HTMLFormControlElement* element)
{
    if (!element->autofocus())
        return false;
    if (!element->renderer())
        return false;
    if (element->document()->ignoreAutofocus())
        return false;
    if (element->document()->isSandboxed(SandboxAutomaticFeatures))
        return false;
    if (element->hasAutofocused())
        return false;

    // FIXME: Should this set of hasTagName checks be replaced by a
    // virtual member function?
    if (element->hasTagName(inputTag))
        return !static_cast<HTMLInputElement*>(element)->isInputTypeHidden();
    if (element->hasTagName(selectTag))
        return true;
    if (element->hasTagName(keygenTag))
        return true;
    if (element->hasTagName(buttonTag))
        return true;
    if (element->hasTagName(textareaTag))
        return true;

    return false;
}

static void focusPostAttach(Node* element, unsigned)
{ 
    static_cast<Element*>(element)->focus(); 
    element->deref(); 
}

void HTMLFormControlElement::attach()
{
    ASSERT(!attached());

    suspendPostAttachCallbacks();

    HTMLElement::attach();

    // The call to updateFromElement() needs to go after the call through
    // to the base class's attach() because that can sometimes do a close
    // on the renderer.
    if (renderer())
        renderer()->updateFromElement();

    if (shouldAutofocus(this)) {
        setAutofocused();
        ref();
        queuePostAttachCallback(focusPostAttach, this);
    }

    resumePostAttachCallbacks();
}

void HTMLFormControlElement::didMoveToNewDocument(Document* oldDocument)
{
    FormAssociatedElement::didMoveToNewDocument(oldDocument);
    HTMLElement::didMoveToNewDocument(oldDocument);
}

Node::InsertionNotificationRequest HTMLFormControlElement::insertedInto(ContainerNode* insertionPoint)
{
    m_ancestorDisabledState = AncestorDisabledStateUnknown;
    m_dataListAncestorState = Unknown;
    setNeedsWillValidateCheck();
    HTMLElement::insertedInto(insertionPoint);
    FormAssociatedElement::insertedInto(insertionPoint);
    return InsertionDone;
}

void HTMLFormControlElement::removedFrom(ContainerNode* insertionPoint)
{
    m_validationMessage = nullptr;
    m_ancestorDisabledState = AncestorDisabledStateUnknown;
    m_dataListAncestorState = Unknown;
    HTMLElement::removedFrom(insertionPoint);
    FormAssociatedElement::removedFrom(insertionPoint);
}

bool HTMLFormControlElement::wasChangedSinceLastFormControlChangeEvent() const
{
    return m_wasChangedSinceLastFormControlChangeEvent;
}

void HTMLFormControlElement::setChangedSinceLastFormControlChangeEvent(bool changed)
{
    m_wasChangedSinceLastFormControlChangeEvent = changed;
}

void HTMLFormControlElement::dispatchFormControlChangeEvent()
{
    HTMLElement::dispatchChangeEvent();
    setChangedSinceLastFormControlChangeEvent(false);
}

void HTMLFormControlElement::dispatchFormControlInputEvent()
{
    setChangedSinceLastFormControlChangeEvent(true);
    HTMLElement::dispatchInputEvent();
}

bool HTMLFormControlElement::disabled() const
{
    if (m_disabled)
        return true;

    if (m_ancestorDisabledState == AncestorDisabledStateUnknown)
        updateAncestorDisabledState();
    return m_ancestorDisabledState == AncestorDisabledStateDisabled;
}

void HTMLFormControlElement::setDisabled(bool b)
{
    setAttribute(disabledAttr, b ? "" : 0);
}

bool HTMLFormControlElement::autofocus() const
{
    return hasAttribute(autofocusAttr);
}

bool HTMLFormControlElement::required() const
{
    return m_required;
}

static void updateFromElementCallback(Node* node, unsigned)
{
    ASSERT_ARG(node, node->isElementNode());
    ASSERT_ARG(node, static_cast<Element*>(node)->isFormControlElement());
    ASSERT(node->renderer());
    if (RenderObject* renderer = node->renderer())
        renderer->updateFromElement();
}

void HTMLFormControlElement::didRecalcStyle(StyleChange)
{
    // updateFromElement() can cause the selection to change, and in turn
    // trigger synchronous layout, so it must not be called during style recalc.
    if (renderer())
        queuePostAttachCallback(updateFromElementCallback, this);
}

bool HTMLFormControlElement::supportsFocus() const
{
    return !disabled();
}

bool HTMLFormControlElement::isFocusable() const
{
    // If there's a renderer, make sure the size isn't empty, but if there's no renderer,
    // it might still be focusable if it's in a canvas subtree (handled in Node::isFocusable).
    if (renderer() && (!renderer()->isBox() || toRenderBox(renderer())->size().isEmpty()))
        return false;
    // HTMLElement::isFocusable handles visibility and calls suportsFocus which
    // will cover the disabled case.
    return HTMLElement::isFocusable();
}

bool HTMLFormControlElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    if (isFocusable())
        if (document()->frame())
            return document()->frame()->eventHandler()->tabsToAllFormControls(event);
    return false;
}

bool HTMLFormControlElement::isMouseFocusable() const
{
#if PLATFORM(GTK) || PLATFORM(QT)
    return HTMLElement::isMouseFocusable();
#else
    return false;
#endif
}

short HTMLFormControlElement::tabIndex() const
{
    // Skip the supportsFocus check in HTMLElement.
    return Element::tabIndex();
}

bool HTMLFormControlElement::recalcWillValidate() const
{
    if (m_dataListAncestorState == Unknown) {
        for (ContainerNode* ancestor = parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (ancestor->hasTagName(datalistTag)) {
                m_dataListAncestorState = InsideDataList;
                break;
            }
        }
        if (m_dataListAncestorState == Unknown)
            m_dataListAncestorState = NotInsideDataList;
    }
    return m_dataListAncestorState == NotInsideDataList && !disabled() && !m_readOnly;
}

bool HTMLFormControlElement::willValidate() const
{
    if (!m_willValidateInitialized || m_dataListAncestorState == Unknown) {
        m_willValidateInitialized = true;
        bool newWillValidate = recalcWillValidate();
        if (m_willValidate != newWillValidate) {
            m_willValidate = newWillValidate;
            const_cast<HTMLFormControlElement*>(this)->setNeedsValidityCheck();
        }
    } else {
        // If the following assertion fails, setNeedsWillValidateCheck() is not
        // called correctly when something which changes recalcWillValidate() result
        // is updated.
        ASSERT(m_willValidate == recalcWillValidate());
    }
    return m_willValidate;
}

void HTMLFormControlElement::setNeedsWillValidateCheck()
{
    // We need to recalculate willValidate immediately because willValidate change can causes style change.
    bool newWillValidate = recalcWillValidate();
    if (m_willValidateInitialized && m_willValidate == newWillValidate)
        return;
    m_willValidateInitialized = true;
    m_willValidate = newWillValidate;
    setNeedsValidityCheck();
    setNeedsStyleRecalc();
    if (!m_willValidate)
        hideVisibleValidationMessage();
}

void HTMLFormControlElement::updateVisibleValidationMessage()
{
    Page* page = document()->page();
    if (!page)
        return;
    String message;
    if (renderer() && willValidate())
        message = validationMessage().stripWhiteSpace();
    if (!m_validationMessage)
        m_validationMessage = ValidationMessage::create(this);
    m_validationMessage->updateValidationMessage(message);
}

void HTMLFormControlElement::hideVisibleValidationMessage()
{
    if (m_validationMessage)
        m_validationMessage->requestToHideMessage();
}

bool HTMLFormControlElement::checkValidity(Vector<RefPtr<FormAssociatedElement> >* unhandledInvalidControls)
{
    if (!willValidate() || isValidFormControlElement())
        return true;
    // An event handler can deref this object.
    RefPtr<HTMLFormControlElement> protector(this);
    RefPtr<Document> originalDocument(document());
    bool needsDefaultAction = dispatchEvent(Event::create(eventNames().invalidEvent, false, true));
    if (needsDefaultAction && unhandledInvalidControls && inDocument() && originalDocument == document())
        unhandledInvalidControls->append(this);
    return false;
}

bool HTMLFormControlElement::isValidFormControlElement()
{
    // If the following assertion fails, setNeedsValidityCheck() is not called
    // correctly when something which changes validity is updated.
    ASSERT(m_isValid == validity()->valid());
    return m_isValid;
}

void HTMLFormControlElement::setNeedsValidityCheck()
{
    bool newIsValid = validity()->valid();
    if (willValidate() && newIsValid != m_isValid) {
        // Update style for pseudo classes such as :valid :invalid.
        setNeedsStyleRecalc();
    }
    m_isValid = newIsValid;

    // Updates only if this control already has a validtion message.
    if (m_validationMessage && m_validationMessage->isVisible()) {
        // Calls updateVisibleValidationMessage() even if m_isValid is not
        // changed because a validation message can be chagned.
        updateVisibleValidationMessage();
    }
}

void HTMLFormControlElement::setCustomValidity(const String& error)
{
    FormAssociatedElement::setCustomValidity(error);
    setNeedsValidityCheck();
}

bool HTMLFormControlElement::shouldMatchReadOnlySelector() const
{
    return readOnly();
}

bool HTMLFormControlElement::shouldMatchReadWriteSelector() const
{
    return !readOnly();
}

bool HTMLFormControlElement::validationMessageShadowTreeContains(Node* node) const
{
    return m_validationMessage && m_validationMessage->shadowTreeContains(node);
}

void HTMLFormControlElement::dispatchBlurEvent(PassRefPtr<Node> newFocusedNode)
{
    HTMLElement::dispatchBlurEvent(newFocusedNode);
    hideVisibleValidationMessage();
}

HTMLFormElement* HTMLFormControlElement::virtualForm() const
{
    return FormAssociatedElement::form();
}

bool HTMLFormControlElement::isDefaultButtonForForm() const
{
    return isSuccessfulSubmitButton() && form() && form()->defaultButton() == this;
}

HTMLFormControlElement* HTMLFormControlElement::enclosingFormControlElement(Node* node)
{
    for (; node; node = node->parentNode()) {
        if (node->isElementNode() && toElement(node)->isFormControlElement())
            return static_cast<HTMLFormControlElement*>(node);
    }
    return 0;
}

} // namespace Webcore
