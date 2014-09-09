/*
 * Copyright (C) 2006, 2008, 2010, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextControlInnerElements.h"

#include "Document.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "MouseEvent.h"
#include "RenderSearchField.h"
#include "RenderTextControl.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "SpeechInput.h"
#include "SpeechInputEvent.h"
#include "TextEvent.h"
#include "TextEventInputType.h"
#include <wtf/Ref.h>

namespace WebCore {

using namespace HTMLNames;

TextControlInnerContainer::TextControlInnerContainer(Document& document)
    : HTMLDivElement(divTag, document)
{
}

PassRefPtr<TextControlInnerContainer> TextControlInnerContainer::create(Document& document)
{
    return adoptRef(new TextControlInnerContainer(document));
}
    
RenderPtr<RenderElement> TextControlInnerContainer::createElementRenderer(PassRef<RenderStyle> style)
{
    return createRenderer<RenderTextControlInnerContainer>(*this, WTF::move(style));
}

TextControlInnerElement::TextControlInnerElement(Document& document)
    : HTMLDivElement(divTag, document)
{
    setHasCustomStyleResolveCallbacks();
}

PassRefPtr<TextControlInnerElement> TextControlInnerElement::create(Document& document)
{
    return adoptRef(new TextControlInnerElement(document));
}

PassRefPtr<RenderStyle> TextControlInnerElement::customStyleForRenderer(RenderStyle&)
{
    RenderTextControlSingleLine* parentRenderer = toRenderTextControlSingleLine(shadowHost()->renderer());
    return parentRenderer->createInnerBlockStyle(&parentRenderer->style());
}

// ---------------------------

inline TextControlInnerTextElement::TextControlInnerTextElement(Document& document)
    : HTMLDivElement(divTag, document)
{
    setHasCustomStyleResolveCallbacks();
}

PassRefPtr<TextControlInnerTextElement> TextControlInnerTextElement::create(Document& document)
{
    return adoptRef(new TextControlInnerTextElement(document));
}

void TextControlInnerTextElement::defaultEventHandler(Event* event)
{
    // FIXME: In the future, we should add a way to have default event listeners.
    // Then we would add one to the text field's inner div, and we wouldn't need this subclass.
    // Or possibly we could just use a normal event listener.
    if (event->isBeforeTextInsertedEvent() || event->type() == eventNames().webkitEditableContentChangedEvent) {
        Element* shadowAncestor = shadowHost();
        // A TextControlInnerTextElement can have no host if its been detached,
        // but kept alive by an EditCommand. In this case, an undo/redo can
        // cause events to be sent to the TextControlInnerTextElement. To
        // prevent an infinite loop, we must check for this case before sending
        // the event up the chain.
        if (shadowAncestor)
            shadowAncestor->defaultEventHandler(event);
    }
    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

RenderPtr<RenderElement> TextControlInnerTextElement::createElementRenderer(PassRef<RenderStyle> style)
{
    return createRenderer<RenderTextControlInnerBlock>(*this, WTF::move(style));
}

RenderTextControlInnerBlock* TextControlInnerTextElement::renderer() const
{
    return toRenderTextControlInnerBlock(HTMLDivElement::renderer());
}

PassRefPtr<RenderStyle> TextControlInnerTextElement::customStyleForRenderer(RenderStyle&)
{
    RenderTextControl* parentRenderer = toRenderTextControl(shadowHost()->renderer());
    return parentRenderer->createInnerTextStyle(&parentRenderer->style());
}

// ----------------------------

inline SearchFieldResultsButtonElement::SearchFieldResultsButtonElement(Document& document)
    : HTMLDivElement(divTag, document)
{
}

PassRefPtr<SearchFieldResultsButtonElement> SearchFieldResultsButtonElement::create(Document& document)
{
    return adoptRef(new SearchFieldResultsButtonElement(document));
}

void SearchFieldResultsButtonElement::defaultEventHandler(Event* event)
{
    // On mousedown, bring up a menu, if needed
    HTMLInputElement* input = toHTMLInputElement(shadowHost());
    if (input && event->type() == eventNames().mousedownEvent && event->isMouseEvent() && toMouseEvent(event)->button() == LeftButton) {
        input->focus();
        input->select();
#if !PLATFORM(IOS)
        RenderSearchField* renderer = toRenderSearchField(input->renderer());
        if (renderer->popupIsVisible())
            renderer->hidePopup();
        else if (input->maxResults() > 0)
            renderer->showPopup();
#endif
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

#if !PLATFORM(IOS)
bool SearchFieldResultsButtonElement::willRespondToMouseClickEvents()
{
    return true;
}
#endif

// ----------------------------

inline SearchFieldCancelButtonElement::SearchFieldCancelButtonElement(Document& document)
    : HTMLDivElement(divTag, document)
    , m_capturing(false)
{
    setPseudo(AtomicString("-webkit-search-cancel-button", AtomicString::ConstructFromLiteral));
    setHasCustomStyleResolveCallbacks();
}

PassRefPtr<SearchFieldCancelButtonElement> SearchFieldCancelButtonElement::create(Document& document)
{
    return adoptRef(new SearchFieldCancelButtonElement(document));
}

void SearchFieldCancelButtonElement::willDetachRenderers()
{
    if (m_capturing) {
        if (Frame* frame = document().frame())
            frame->eventHandler().setCapturingMouseEventsElement(nullptr);
    }
}

void SearchFieldCancelButtonElement::defaultEventHandler(Event* event)
{
    // If the element is visible, on mouseup, clear the value, and set selection
    RefPtr<HTMLInputElement> input(toHTMLInputElement(shadowHost()));
    if (!input || input->isDisabledOrReadOnly()) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }

    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && toMouseEvent(event)->button() == LeftButton) {
        if (renderer() && renderer()->visibleToHitTesting()) {
            if (Frame* frame = document().frame()) {
                frame->eventHandler().setCapturingMouseEventsElement(this);
                m_capturing = true;
            }
        }
        input->focus();
        input->select();
        event->setDefaultHandled();
    }
    if (event->type() == eventNames().mouseupEvent && event->isMouseEvent() && toMouseEvent(event)->button() == LeftButton) {
        if (m_capturing) {
            if (Frame* frame = document().frame()) {
                frame->eventHandler().setCapturingMouseEventsElement(nullptr);
                m_capturing = false;
            }
            if (hovered()) {
                String oldValue = input->value();
                input->setValueForUser("");
                input->onSearch();
                event->setDefaultHandled();
            }
        }
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

#if !PLATFORM(IOS)
bool SearchFieldCancelButtonElement::willRespondToMouseClickEvents()
{
    const HTMLInputElement* input = toHTMLInputElement(shadowHost());
    if (input && !input->isDisabledOrReadOnly())
        return true;

    return HTMLDivElement::willRespondToMouseClickEvents();
}
#endif

// ----------------------------

#if ENABLE(INPUT_SPEECH)

inline InputFieldSpeechButtonElement::InputFieldSpeechButtonElement(Document& document)
    : HTMLDivElement(divTag, document)
    , m_capturing(false)
    , m_state(Idle)
    , m_listenerId(0)
{
    setPseudo(AtomicString("-webkit-input-speech-button", AtomicString::ConstructFromLiteral));
    setHasCustomStyleResolveCallbacks();
}

InputFieldSpeechButtonElement::~InputFieldSpeechButtonElement()
{
    SpeechInput* speech = speechInput();
    if (speech && m_listenerId)  { // Could be null when page is unloading.
        if (m_state != Idle)
            speech->cancelRecognition(m_listenerId);
        speech->unregisterListener(m_listenerId);
    }
}

PassRefPtr<InputFieldSpeechButtonElement> InputFieldSpeechButtonElement::create(Document& document)
{
    return adoptRef(new InputFieldSpeechButtonElement(document));
}

void InputFieldSpeechButtonElement::defaultEventHandler(Event* event)
{
    // For privacy reasons, only allow clicks directly coming from the user.
    if (!ScriptController::processingUserGesture()) {
        HTMLDivElement::defaultEventHandler(event);
        return;
    }

    // The call to focus() below dispatches a focus event, and an event handler in the page might
    // remove the input element from DOM. To make sure it remains valid until we finish our work
    // here, we take a temporary reference.
    RefPtr<HTMLInputElement> input(toHTMLInputElement(shadowHost()));

    if (!input || input->isDisabledOrReadOnly()) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }

    // On mouse down, select the text and set focus.
    if (event->type() == eventNames().mousedownEvent && event->isMouseEvent() && toMouseEvent(event)->button() == LeftButton) {
        if (renderer() && renderer()->visibleToHitTesting()) {
            if (Frame* frame = document().frame()) {
                frame->eventHandler().setCapturingMouseEventsElement(this);
                m_capturing = true;
            }
        }
        Ref<InputFieldSpeechButtonElement> protect(*this);
        input->focus();
        input->select();
        event->setDefaultHandled();
    }
    // On mouse up, release capture cleanly.
    if (event->type() == eventNames().mouseupEvent && event->isMouseEvent() && toMouseEvent(event)->button() == LeftButton) {
        if (m_capturing && renderer() && renderer()->visibleToHitTesting()) {
            if (Frame* frame = document().frame()) {
                frame->eventHandler().setCapturingMouseEventsElement(nullptr);
                m_capturing = false;
            }
        }
    }

    if (event->type() == eventNames().clickEvent && m_listenerId) {
        switch (m_state) {
        case Idle:
            startSpeechInput();
            break;
        case Recording:
            stopSpeechInput();
            break;
        case Recognizing:
            // Nothing to do here, we will continue to wait for results.
            break;
        }
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

#if !PLATFORM(IOS)
bool InputFieldSpeechButtonElement::willRespondToMouseClickEvents()
{
    const HTMLInputElement* input = toHTMLInputElement(shadowHost());
    if (input && !input->isDisabledOrReadOnly())
        return true;

    return HTMLDivElement::willRespondToMouseClickEvents();
}
#endif

void InputFieldSpeechButtonElement::setState(SpeechInputState state)
{
    if (m_state != state) {
        m_state = state;
        shadowHost()->renderer()->repaint();
    }
}

SpeechInput* InputFieldSpeechButtonElement::speechInput()
{
    return SpeechInput::from(document().page());
}

void InputFieldSpeechButtonElement::didCompleteRecording(int)
{
    setState(Recognizing);
}

void InputFieldSpeechButtonElement::didCompleteRecognition(int)
{
    setState(Idle);
}

void InputFieldSpeechButtonElement::setRecognitionResult(int, const SpeechInputResultArray& results)
{
    m_results = results;

    // The call to setValue() below dispatches an event, and an event handler in the page might
    // remove the input element from DOM. To make sure it remains valid until we finish our work
    // here, we take a temporary reference.
    RefPtr<HTMLInputElement> input(toHTMLInputElement(shadowHost()));
    if (!input || input->isDisabledOrReadOnly())
        return;

    Ref<InputFieldSpeechButtonElement> protect(*this);
    if (document().domWindow()) {
        // Call selectionChanged, causing the element to cache the selection,
        // so that the text event inserts the text in this element even if
        // focus has moved away from it.
        input->selectionChanged(false);
        input->dispatchEvent(TextEvent::create(document().domWindow(), results.isEmpty() ? "" : results[0]->utterance(), TextEventInputOther));
    }

    // This event is sent after the text event so the website can perform actions using the input field content immediately.
    // It provides alternative recognition hypotheses and notifies that the results come from speech input.
    input->dispatchEvent(SpeechInputEvent::create(eventNames().webkitspeechchangeEvent, results));

    // Check before accessing the renderer as the above event could have potentially turned off
    // speech in the input element, hence removing this button and renderer from the hierarchy.
    if (renderer())
        renderer()->repaint();
}

void InputFieldSpeechButtonElement::willAttachRenderers()
{
    ASSERT(!m_listenerId);
    if (SpeechInput* input = SpeechInput::from(document().page()))
        m_listenerId = input->registerListener(this);
}

void InputFieldSpeechButtonElement::willDetachRenderers()
{
    if (m_capturing) {
        if (Frame* frame = document().frame())
            frame->eventHandler().setCapturingMouseEventsElement(nullptr);
    }

    if (m_listenerId) {
        if (m_state != Idle)
            speechInput()->cancelRecognition(m_listenerId);
        speechInput()->unregisterListener(m_listenerId);
        m_listenerId = 0;
    }
}

void InputFieldSpeechButtonElement::startSpeechInput()
{
    if (m_state != Idle)
        return;

    RefPtr<HTMLInputElement> input = toHTMLInputElement(shadowHost());
    AtomicString language = input->computeInheritedLanguage();
    String grammar = input->getAttribute(webkitgrammarAttr);
    IntRect rect = document().view()->contentsToRootView(pixelSnappedBoundingBox());
    if (speechInput()->startRecognition(m_listenerId, rect, language, grammar, document().securityOrigin()))
        setState(Recording);
}

void InputFieldSpeechButtonElement::stopSpeechInput()
{
    if (m_state == Recording)
        speechInput()->stopRecording(m_listenerId);
}

#endif // ENABLE(INPUT_SPEECH)

}
