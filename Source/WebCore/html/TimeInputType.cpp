/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TimeInputType.h"

#include "DateComponents.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "InputTypeNames.h"
#include <wtf/CurrentTime.h>
#include <wtf/DateMath.h>
#include <wtf/MathExtras.h>
#include <wtf/PassOwnPtr.h>

#if ENABLE(INPUT_TYPE_TIME)
#if ENABLE(INPUT_TYPE_TIME_MULTIPLE_FIELDS)
#include "DateTimeFieldsState.h"
#include "ElementShadow.h"
#include "FormController.h"
#include "KeyboardEvent.h"
#include "ShadowRoot.h"
#endif

namespace WebCore {

using namespace HTMLNames;

static const int timeDefaultStep = 60;
static const int timeDefaultStepBase = 0;
static const int timeStepScaleFactor = 1000;

PassOwnPtr<InputType> TimeInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new TimeInputType(element));
}

const AtomicString& TimeInputType::formControlType() const
{
    return InputTypeNames::time();
}

DateComponents::Type TimeInputType::dateType() const
{
    return DateComponents::Time;
}

Decimal TimeInputType::defaultValueForStepUp() const
{
    double current = currentTimeMS();
    double utcOffset = calculateUTCOffset();
    double dstOffset = calculateDSTOffset(current, utcOffset);
    int offset = static_cast<int>((utcOffset + dstOffset) / msPerMinute);
    current += offset * msPerMinute;

    DateComponents date;
    date.setMillisecondsSinceMidnight(current);
    double milliseconds = date.millisecondsSinceEpoch();
    ASSERT(isfinite(milliseconds));
    return Decimal::fromDouble(milliseconds);
}

StepRange TimeInputType::createStepRange(AnyStepHandling anyStepHandling) const
{
    DEFINE_STATIC_LOCAL(const StepRange::StepDescription, stepDescription, (timeDefaultStep, timeDefaultStepBase, timeStepScaleFactor, StepRange::ScaledStepValueShouldBeInteger));

    const Decimal stepBase = parseToNumber(element()->fastGetAttribute(minAttr), 0);
    const Decimal minimum = parseToNumber(element()->fastGetAttribute(minAttr), Decimal::fromDouble(DateComponents::minimumTime()));
    const Decimal maximum = parseToNumber(element()->fastGetAttribute(maxAttr), Decimal::fromDouble(DateComponents::maximumTime()));
    const Decimal step = StepRange::parseStep(anyStepHandling, stepDescription, element()->fastGetAttribute(stepAttr));
    return StepRange(stepBase, minimum, maximum, step, stepDescription);
}

bool TimeInputType::parseToDateComponentsInternal(const UChar* characters, unsigned length, DateComponents* out) const
{
    ASSERT(out);
    unsigned end;
    return out->parseTime(characters, length, 0, end) && end == length;
}

bool TimeInputType::setMillisecondToDateComponents(double value, DateComponents* date) const
{
    ASSERT(date);
    return date->setMillisecondsSinceMidnight(value);
}

bool TimeInputType::isTimeField() const
{
    return true;
}

#if ENABLE(INPUT_TYPE_TIME_MULTIPLE_FIELDS)

TimeInputType::DateTimeEditControlOwnerImpl::DateTimeEditControlOwnerImpl(TimeInputType& timeInputType)
    : m_timeInputType(timeInputType)
{
}

TimeInputType::DateTimeEditControlOwnerImpl::~DateTimeEditControlOwnerImpl()
{
}

void TimeInputType::DateTimeEditControlOwnerImpl::didBlurFromControl()
{
    // We don't need to call blur(). This function is called when control
    // lost focus.

    // Remove focus ring by CSS "focus" pseudo class.
    m_timeInputType.element()->setFocus(false);
}

void TimeInputType::DateTimeEditControlOwnerImpl::didFocusOnControl()
{
    // We don't need to call focus(). This function is called when control
    // got focus.

    // Add focus ring by CSS "focus" pseudo class.
    m_timeInputType.element()->setFocus(true);
}

void TimeInputType::DateTimeEditControlOwnerImpl::editControlValueChanged()
{
    RefPtr<HTMLInputElement> input(m_timeInputType.element());
    input->setValueInternal(m_timeInputType.serialize(Decimal::fromDouble(m_timeInputType.m_dateTimeEditElement->valueAsDouble())), DispatchNoEvent);
    input->setNeedsStyleRecalc();
    input->dispatchFormControlInputEvent();
    input->dispatchFormControlChangeEvent();
}

bool TimeInputType::hasCustomFocusLogic() const
{
    return false;
}

bool TimeInputType::DateTimeEditControlOwnerImpl::isEditControlOwnerDisabled() const
{
    return m_timeInputType.element()->readOnly();
}

bool TimeInputType::DateTimeEditControlOwnerImpl::isEditControlOwnerReadOnly() const
{
    return m_timeInputType.element()->disabled();
}

TimeInputType::TimeInputType(HTMLInputElement* element)
    : BaseDateAndTimeInputType(element)
    , m_dateTimeEditElement(0)
    , m_dateTimeEditControlOwnerImpl(*this)
{
}

TimeInputType::~TimeInputType()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->removeEditControlOwner();
}

void TimeInputType::blur()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->blurByOwner();
}

RenderObject* TimeInputType::createRenderer(RenderArena* arena, RenderStyle* style) const
{
    return InputType::createRenderer(arena, style);
}

void TimeInputType::createShadowSubtree()
{
    ASSERT(element()->shadow());

    RefPtr<DateTimeEditElement> dateTimeEditElement(DateTimeEditElement::create(element()->document(), m_dateTimeEditControlOwnerImpl));
    m_dateTimeEditElement = dateTimeEditElement.get();
    element()->userAgentShadowRoot()->appendChild(m_dateTimeEditElement);
    updateInnerTextValue();
}

void TimeInputType::destroyShadowSubtree()
{
    if (m_dateTimeEditElement) {
        m_dateTimeEditElement->removeEditControlOwner();
        m_dateTimeEditElement = 0;
    }
    BaseDateAndTimeInputType::destroyShadowSubtree();
}

void TimeInputType::focus(bool)
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->focusByOwner();
}

void TimeInputType::forwardEvent(Event* event)
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->defaultEventHandler(event);
}

void TimeInputType::disabledAttributeChanged()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->disabledStateChanged();
}

void TimeInputType::handleKeydownEvent(KeyboardEvent* event)
{
    forwardEvent(event);
}

bool TimeInputType::isKeyboardFocusable(KeyboardEvent*) const
{
    return false;
}

bool TimeInputType::isMouseFocusable() const
{
    return false;
}

void TimeInputType::minOrMaxAttributeChanged()
{
    updateInnerTextValue();
}

void TimeInputType::readonlyAttributeChanged()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->readOnlyStateChanged();
}

bool TimeInputType::isTextField() const
{
    return false;
}

void TimeInputType::restoreFormControlState(const FormControlState& state)
{
    if (!m_dateTimeEditElement)
        return;
    DateComponents date;
    setMillisecondToDateComponents(createStepRange(AnyIsDefaultStep).minimum().toDouble(), &date);
    DateTimeFieldsState dateTimeFieldsState = DateTimeFieldsState::restoreFormControlState(state);
    m_dateTimeEditElement->setValueAsDateTimeFieldsState(dateTimeFieldsState, date);
    element()->setValueInternal(serialize(Decimal::fromDouble(m_dateTimeEditElement->valueAsDouble())), DispatchNoEvent);
}

FormControlState TimeInputType::saveFormControlState() const
{
    if (!m_dateTimeEditElement)
        return FormControlState();

    return m_dateTimeEditElement->valueAsDateTimeFieldsState().saveFormControlState();
}

void TimeInputType::setValue(const String& sanitizedValue, bool valueChanged, TextFieldEventBehavior eventBehavior)
{
    InputType::setValue(sanitizedValue, valueChanged, eventBehavior);
    if (valueChanged)
        updateInnerTextValue();
}

bool TimeInputType::shouldUseInputMethod() const
{
    return false;
}

void TimeInputType::stepAttributeChanged()
{
    updateInnerTextValue();
}

void TimeInputType::updateInnerTextValue()
{
    if (!m_dateTimeEditElement)
        return;

    Localizer& localizer = element()->document()->getLocalizer(element()->computeInheritedLanguage());
    const StepRange stepRange(createStepRange(AnyIsDefaultStep));
    DateComponents date;
    if (parseToDateComponents(element()->value(), &date))
        m_dateTimeEditElement->setValueAsDate(stepRange, date, localizer);
    else {
        setMillisecondToDateComponents(stepRange.minimum().toDouble(), &date);
        m_dateTimeEditElement->setEmptyValue(stepRange, date, localizer);
    }
}
#else
TimeInputType::TimeInputType(HTMLInputElement* element)
    : BaseDateAndTimeInputType(element)
{
}
#endif

} // namespace WebCore

#endif
