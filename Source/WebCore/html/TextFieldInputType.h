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

#ifndef TextFieldInputType_h
#define TextFieldInputType_h

#include "InputType.h"

namespace WebCore {

class SpinButtonElement;

// The class represents types of which UI contain text fields.
// It supports not only the types for BaseTextInputType but also type=number.
class TextFieldInputType : public InputType {
protected:
    TextFieldInputType(HTMLInputElement*);
    virtual ~TextFieldInputType();
    virtual bool canSetSuggestedValue();
    virtual void handleKeydownEvent(KeyboardEvent*);
    void handleKeydownEventForSpinButton(KeyboardEvent*);
    void handleWheelEventForSpinButton(WheelEvent*);

    virtual HTMLElement* containerElement() const;
    virtual HTMLElement* innerBlockElement() const;
    virtual HTMLElement* innerTextElement() const;
    virtual HTMLElement* innerSpinButtonElement() const;
#if ENABLE(INPUT_SPEECH)
    virtual HTMLElement* speechButtonElement() const;
#endif

protected:
    virtual bool needsContainer() const;
    virtual void createShadowSubtree();
    virtual void destroyShadowSubtree();
    virtual void disabledAttributeChanged();
    virtual void readonlyAttributeChanged();

private:
    virtual bool isTextField() const;
    virtual bool valueMissing(const String&) const;
    virtual void handleBeforeTextInsertedEvent(BeforeTextInsertedEvent*);
    virtual void forwardEvent(Event*);
    virtual bool shouldSubmitImplicitly(Event*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const;
    virtual bool shouldUseInputMethod() const;
    virtual void setValue(const String&, bool valueChanged, bool sendChangeEvent);
    virtual void dispatchChangeEventInResponseToSetValue();
    virtual String sanitizeValue(const String&);
    virtual bool shouldRespectListAttribute();
    virtual HTMLElement* placeholderElement() const;
    virtual void updatePlaceholderText();

    RefPtr<HTMLElement> m_container;
    RefPtr<HTMLElement> m_innerBlock;
    RefPtr<HTMLElement> m_innerText;
    RefPtr<HTMLElement> m_placeholder;
    RefPtr<SpinButtonElement> m_innerSpinButton;
#if ENABLE(INPUT_SPEECH)
    RefPtr<HTMLElement> m_speechButton;
#endif
};

} // namespace WebCore

#endif // TextFieldInputType_h
