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

#ifndef RangeInputType_h
#define RangeInputType_h

#include "InputType.h"

namespace WebCore {

class SliderThumbElement;

class RangeInputType : public InputType {
public:
    static PassOwnPtr<InputType> create(HTMLInputElement*);

private:
    RangeInputType(HTMLInputElement* element) : InputType(element) { }
    virtual bool isRangeControl() const OVERRIDE;
    virtual const AtomicString& formControlType() const OVERRIDE;
    virtual double valueAsNumber() const OVERRIDE;
    virtual void setValueAsNumber(double, TextFieldEventBehavior, ExceptionCode&) const OVERRIDE;
    virtual bool supportsRequired() const OVERRIDE;
    virtual bool rangeUnderflow(const String&) const OVERRIDE;
    virtual bool rangeOverflow(const String&) const OVERRIDE;
    virtual bool supportsRangeLimitation() const OVERRIDE;
    virtual double minimum() const OVERRIDE;
    virtual double maximum() const OVERRIDE;
    virtual bool isSteppable() const OVERRIDE;
    virtual bool stepMismatch(const String&, double) const OVERRIDE;
    virtual double stepBase() const OVERRIDE;
    virtual double defaultStep() const OVERRIDE;
    virtual double stepScaleFactor() const OVERRIDE;
    virtual void handleMouseDownEvent(MouseEvent*) OVERRIDE;
    virtual void handleKeydownEvent(KeyboardEvent*) OVERRIDE;
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const OVERRIDE;
    virtual void createShadowSubtree() OVERRIDE;
    virtual double parseToDouble(const String&, double) const OVERRIDE;
    virtual String serialize(double) const OVERRIDE;
    virtual void accessKeyAction(bool sendMouseEvents) OVERRIDE;
    virtual void minOrMaxAttributeChanged() OVERRIDE;
    virtual void setValue(const String&, bool valueChanged, TextFieldEventBehavior) OVERRIDE;
    virtual String fallbackValue() const OVERRIDE;
    virtual String sanitizeValue(const String& proposedValue) const OVERRIDE;
    virtual bool shouldRespectListAttribute() OVERRIDE;
};

} // namespace WebCore

#endif // RangeInputType_h
