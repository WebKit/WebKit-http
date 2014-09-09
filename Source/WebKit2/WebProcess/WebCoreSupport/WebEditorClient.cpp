/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebEditorClient.h"

#include "EditorState.h"
#include "ServicesOverlayController.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebPageProxy.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include "WebUndoStep.h"
#include <WebCore/ArchiveResource.h>
#include <WebCore/DocumentFragment.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/SpellChecker.h>
#include <WebCore/StyleProperties.h>
#include <WebCore/UndoStep.h>
#include <WebCore/UserTypingGestureIndicator.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringView.h>

using namespace WebCore;
using namespace HTMLNames;

namespace WebKit {

static uint64_t generateTextCheckingRequestID()
{
    static uint64_t uniqueTextCheckingRequestID = 1;
    return uniqueTextCheckingRequestID++;
}

void WebEditorClient::pageDestroyed()
{
    delete this;
}

bool WebEditorClient::shouldDeleteRange(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldDeleteRange(m_page, range);
    notImplemented();
    return result;
}

#if ENABLE(DELETION_UI)
bool WebEditorClient::shouldShowDeleteInterface(HTMLElement*)
{
    notImplemented();
    return false;
}
#endif

bool WebEditorClient::smartInsertDeleteEnabled()
{
    return m_page->isSmartInsertDeleteEnabled();
}
 
bool WebEditorClient::isSelectTrailingWhitespaceEnabled()
{
    return m_page->isSelectTrailingWhitespaceEnabled();
}

bool WebEditorClient::isContinuousSpellCheckingEnabled()
{
    return WebProcess::shared().textCheckerState().isContinuousSpellCheckingEnabled;
}

void WebEditorClient::toggleContinuousSpellChecking()
{
    notImplemented();
}

bool WebEditorClient::isGrammarCheckingEnabled()
{
    return WebProcess::shared().textCheckerState().isGrammarCheckingEnabled;
}

void WebEditorClient::toggleGrammarChecking()
{
    notImplemented();
}

int WebEditorClient::spellCheckerDocumentTag()
{
    notImplemented();
    return false;
}

bool WebEditorClient::shouldBeginEditing(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldBeginEditing(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldEndEditing(Range* range)
{
    bool result = m_page->injectedBundleEditorClient().shouldEndEditing(m_page, range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldInsertNode(Node* node, Range* rangeToReplace, EditorInsertAction action)
{
    bool result = m_page->injectedBundleEditorClient().shouldInsertNode(m_page, node, rangeToReplace, action);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldInsertText(const String& text, Range* rangeToReplace, EditorInsertAction action)
{
    bool result = m_page->injectedBundleEditorClient().shouldInsertText(m_page, text.impl(), rangeToReplace, action);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity affinity, bool stillSelecting)
{
    bool result = m_page->injectedBundleEditorClient().shouldChangeSelectedRange(m_page, fromRange, toRange, affinity, stillSelecting);
    notImplemented();
    return result;
}
    
bool WebEditorClient::shouldApplyStyle(StyleProperties* style, Range* range)
{
    Ref<MutableStyleProperties> mutableStyle(style->isMutable() ? static_cast<MutableStyleProperties&>(*style) : style->mutableCopy());
    bool result = m_page->injectedBundleEditorClient().shouldApplyStyle(m_page, mutableStyle->ensureCSSStyleDeclaration(), range);
    notImplemented();
    return result;
}

bool WebEditorClient::shouldMoveRangeAfterDelete(Range*, Range*)
{
    notImplemented();
    return true;
}

void WebEditorClient::didBeginEditing()
{
    // FIXME: What good is a notification name, if it's always the same?
    static NeverDestroyed<String> WebViewDidBeginEditingNotification(ASCIILiteral("WebViewDidBeginEditingNotification"));
    m_page->injectedBundleEditorClient().didBeginEditing(m_page, WebViewDidBeginEditingNotification.get().impl());
    notImplemented();
}

void WebEditorClient::respondToChangedContents()
{
    static NeverDestroyed<String> WebViewDidChangeNotification(ASCIILiteral("WebViewDidChangeNotification"));
    m_page->injectedBundleEditorClient().didChange(m_page, WebViewDidChangeNotification.get().impl());
    notImplemented();
}

void WebEditorClient::respondToChangedSelection(Frame* frame)
{
    static NeverDestroyed<String> WebViewDidChangeSelectionNotification(ASCIILiteral("WebViewDidChangeSelectionNotification"));
    m_page->injectedBundleEditorClient().didChangeSelection(m_page, WebViewDidChangeSelectionNotification.get().impl());
    if (!frame)
        return;

    m_page->didChangeSelection();

#if PLATFORM(GTK)
    updateGlobalSelection(frame);
#endif
}

void WebEditorClient::didEndEditing()
{
    static NeverDestroyed<String> WebViewDidEndEditingNotification(ASCIILiteral("WebViewDidEndEditingNotification"));
    m_page->injectedBundleEditorClient().didEndEditing(m_page, WebViewDidEndEditingNotification.get().impl());
    notImplemented();
}

void WebEditorClient::didWriteSelectionToPasteboard()
{
    m_page->injectedBundleEditorClient().didWriteToPasteboard(m_page);
}

void WebEditorClient::willWriteSelectionToPasteboard(Range* range)
{
    m_page->injectedBundleEditorClient().willWriteToPasteboard(m_page, range);
}

void WebEditorClient::getClientPasteboardDataForRange(Range* range, Vector<String>& pasteboardTypes, Vector<RefPtr<SharedBuffer>>& pasteboardData)
{
    m_page->injectedBundleEditorClient().getPasteboardDataForRange(m_page, range, pasteboardTypes, pasteboardData);
}

void WebEditorClient::registerUndoStep(PassRefPtr<UndoStep> step)
{
    // FIXME: Add assertion that the command being reapplied is the same command that is
    // being passed to us.
    if (m_page->isInRedo())
        return;

    RefPtr<WebUndoStep> webStep = WebUndoStep::create(step);
    m_page->addWebUndoStep(webStep->stepID(), webStep.get());
    uint32_t editAction = static_cast<uint32_t>(webStep->step()->editingAction());

    m_page->send(Messages::WebPageProxy::RegisterEditCommandForUndo(webStep->stepID(), editAction));
}

void WebEditorClient::registerRedoStep(PassRefPtr<UndoStep>)
{
}

void WebEditorClient::clearUndoRedoOperations()
{
    m_page->send(Messages::WebPageProxy::ClearAllEditCommands());
}

bool WebEditorClient::canCopyCut(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canPaste(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canUndo() const
{
    bool result = false;
    m_page->sendSync(Messages::WebPageProxy::CanUndoRedo(static_cast<uint32_t>(WebPageProxy::Undo)), Messages::WebPageProxy::CanUndoRedo::Reply(result));
    return result;
}

bool WebEditorClient::canRedo() const
{
    bool result = false;
    m_page->sendSync(Messages::WebPageProxy::CanUndoRedo(static_cast<uint32_t>(WebPageProxy::Redo)), Messages::WebPageProxy::CanUndoRedo::Reply(result));
    return result;
}

void WebEditorClient::undo()
{
    bool result = false;
    m_page->sendSync(Messages::WebPageProxy::ExecuteUndoRedo(static_cast<uint32_t>(WebPageProxy::Undo)), Messages::WebPageProxy::ExecuteUndoRedo::Reply(result));
}

void WebEditorClient::redo()
{
    bool result = false;
    m_page->sendSync(Messages::WebPageProxy::ExecuteUndoRedo(static_cast<uint32_t>(WebPageProxy::Redo)), Messages::WebPageProxy::ExecuteUndoRedo::Reply(result));
}

#if !PLATFORM(GTK) && !PLATFORM(COCOA) && !PLATFORM(EFL)
void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    if (m_page->handleEditingKeyboardEvent(event))
        event->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent*)
{
    notImplemented();
}
#endif

void WebEditorClient::textFieldDidBeginEditing(Element* element)
{
    if (!isHTMLInputElement(element))
        return;

    WebFrame* webFrame = WebFrame::fromCoreFrame(*element->document().frame());
    ASSERT(webFrame);

    m_page->injectedBundleFormClient().textFieldDidBeginEditing(m_page, toHTMLInputElement(element), webFrame);
}

void WebEditorClient::textFieldDidEndEditing(Element* element)
{
    if (!isHTMLInputElement(element))
        return;

    WebFrame* webFrame = WebFrame::fromCoreFrame(*element->document().frame());
    ASSERT(webFrame);

    m_page->injectedBundleFormClient().textFieldDidEndEditing(m_page, toHTMLInputElement(element), webFrame);
}

void WebEditorClient::textDidChangeInTextField(Element* element)
{
    if (!isHTMLInputElement(element))
        return;

    bool initiatedByUserTyping = UserTypingGestureIndicator::processingUserTypingGesture() && UserTypingGestureIndicator::focusedElementAtGestureStart() == element;

    WebFrame* webFrame = WebFrame::fromCoreFrame(*element->document().frame());
    ASSERT(webFrame);

    m_page->injectedBundleFormClient().textDidChangeInTextField(m_page, toHTMLInputElement(element), webFrame, initiatedByUserTyping);
}

void WebEditorClient::textDidChangeInTextArea(Element* element)
{
    if (!isHTMLTextAreaElement(element))
        return;

    WebFrame* webFrame = WebFrame::fromCoreFrame(*element->document().frame());
    ASSERT(webFrame);

    m_page->injectedBundleFormClient().textDidChangeInTextArea(m_page, toHTMLTextAreaElement(element), webFrame);
}

#if !PLATFORM(IOS)
void WebEditorClient::overflowScrollPositionChanged()
{
    notImplemented();
}
#endif

static bool getActionTypeForKeyEvent(KeyboardEvent* event, WKInputFieldActionType& type)
{
    String key = event->keyIdentifier();
    if (key == "Up")
        type = WKInputFieldActionTypeMoveUp;
    else if (key == "Down")
        type = WKInputFieldActionTypeMoveDown;
    else if (key == "U+001B")
        type = WKInputFieldActionTypeCancel;
    else if (key == "U+0009") {
        if (event->shiftKey())
            type = WKInputFieldActionTypeInsertBacktab;
        else
            type = WKInputFieldActionTypeInsertTab;
    } else if (key == "Enter")
        type = WKInputFieldActionTypeInsertNewline;
    else
        return false;

    return true;
}

static API::InjectedBundle::FormClient::InputFieldAction toInputFieldAction(WKInputFieldActionType action)
{
    switch (action) {
    case WKInputFieldActionTypeMoveUp:
        return API::InjectedBundle::FormClient::InputFieldAction::MoveUp;
    case WKInputFieldActionTypeMoveDown:
        return API::InjectedBundle::FormClient::InputFieldAction::MoveDown;
    case WKInputFieldActionTypeCancel:
        return API::InjectedBundle::FormClient::InputFieldAction::Cancel;
    case WKInputFieldActionTypeInsertTab:
        return API::InjectedBundle::FormClient::InputFieldAction::InsertTab;
    case WKInputFieldActionTypeInsertNewline:
        return API::InjectedBundle::FormClient::InputFieldAction::InsertNewline;
    case WKInputFieldActionTypeInsertDelete:
        return API::InjectedBundle::FormClient::InputFieldAction::InsertDelete;
    case WKInputFieldActionTypeInsertBacktab:
        return API::InjectedBundle::FormClient::InputFieldAction::InsertBacktab;
    }

    ASSERT_NOT_REACHED();
    return API::InjectedBundle::FormClient::InputFieldAction::Cancel;
}

bool WebEditorClient::doTextFieldCommandFromEvent(Element* element, KeyboardEvent* event)
{
    if (!isHTMLInputElement(element))
        return false;

    WKInputFieldActionType actionType = static_cast<WKInputFieldActionType>(0);
    if (!getActionTypeForKeyEvent(event, actionType))
        return false;

    WebFrame* webFrame = WebFrame::fromCoreFrame(*element->document().frame());
    ASSERT(webFrame);

    return m_page->injectedBundleFormClient().shouldPerformActionInTextField(m_page, toHTMLInputElement(element), toInputFieldAction(actionType), webFrame);
}

void WebEditorClient::textWillBeDeletedInTextField(Element* element)
{
    if (!isHTMLInputElement(element))
        return;

    WebFrame* webFrame = WebFrame::fromCoreFrame(*element->document().frame());
    ASSERT(webFrame);

    m_page->injectedBundleFormClient().shouldPerformActionInTextField(m_page, toHTMLInputElement(element), toInputFieldAction(WKInputFieldActionTypeInsertDelete), webFrame);
}

bool WebEditorClient::shouldEraseMarkersAfterChangeSelection(WebCore::TextCheckingType type) const
{
    // This prevents erasing spelling markers on OS X Lion or later to match AppKit on these Mac OS X versions.
#if PLATFORM(COCOA) || PLATFORM(EFL)
    return type != TextCheckingTypeSpelling;
#else
    UNUSED_PARAM(type);
    return true;
#endif
}

void WebEditorClient::ignoreWordInSpellDocument(const String& word)
{
    m_page->send(Messages::WebPageProxy::IgnoreWord(word));
}

void WebEditorClient::learnWord(const String& word)
{
    m_page->send(Messages::WebPageProxy::LearnWord(word));
}

void WebEditorClient::checkSpellingOfString(StringView text, int* misspellingLocation, int* misspellingLength)
{
    int32_t resultLocation = -1;
    int32_t resultLength = 0;
    m_page->sendSync(Messages::WebPageProxy::CheckSpellingOfString(text.toStringWithoutCopying()),
        Messages::WebPageProxy::CheckSpellingOfString::Reply(resultLocation, resultLength));
    *misspellingLocation = resultLocation;
    *misspellingLength = resultLength;
}

String WebEditorClient::getAutoCorrectSuggestionForMisspelledWord(const String&)
{
    notImplemented();
    return String();
}

void WebEditorClient::checkGrammarOfString(StringView text, Vector<WebCore::GrammarDetail>& grammarDetails, int* badGrammarLocation, int* badGrammarLength)
{
    int32_t resultLocation = -1;
    int32_t resultLength = 0;
    m_page->sendSync(Messages::WebPageProxy::CheckGrammarOfString(text.toStringWithoutCopying()),
        Messages::WebPageProxy::CheckGrammarOfString::Reply(grammarDetails, resultLocation, resultLength));
    *badGrammarLocation = resultLocation;
    *badGrammarLength = resultLength;
}

#if USE(UNIFIED_TEXT_CHECKING)
Vector<TextCheckingResult> WebEditorClient::checkTextOfParagraph(StringView stringView, WebCore::TextCheckingTypeMask checkingTypes)
{
    Vector<TextCheckingResult> results;

    m_page->sendSync(Messages::WebPageProxy::CheckTextOfParagraph(stringView.toStringWithoutCopying(), checkingTypes), Messages::WebPageProxy::CheckTextOfParagraph::Reply(results));

    return results;
}
#endif

void WebEditorClient::updateSpellingUIWithGrammarString(const String& badGrammarPhrase, const GrammarDetail& grammarDetail)
{
    m_page->send(Messages::WebPageProxy::UpdateSpellingUIWithGrammarString(badGrammarPhrase, grammarDetail));
}

void WebEditorClient::updateSpellingUIWithMisspelledWord(const String& misspelledWord)
{
    m_page->send(Messages::WebPageProxy::UpdateSpellingUIWithMisspelledWord(misspelledWord));
}

void WebEditorClient::showSpellingUI(bool)
{
    notImplemented();
}

bool WebEditorClient::spellingUIIsShowing()
{
    bool isShowing = false;
    m_page->sendSync(Messages::WebPageProxy::SpellingUIIsShowing(), Messages::WebPageProxy::SpellingUIIsShowing::Reply(isShowing));
    return isShowing;
}

void WebEditorClient::getGuessesForWord(const String& word, const String& context, Vector<String>& guesses)
{
    m_page->sendSync(Messages::WebPageProxy::GetGuessesForWord(word, context), Messages::WebPageProxy::GetGuessesForWord::Reply(guesses));
}

void WebEditorClient::requestCheckingOfString(WTF::PassRefPtr<TextCheckingRequest> prpRequest)
{
    RefPtr<TextCheckingRequest> request = prpRequest;

    uint64_t requestID = generateTextCheckingRequestID();
    m_page->addTextCheckingRequest(requestID, request);

    m_page->send(Messages::WebPageProxy::RequestCheckingOfString(requestID, request->data()));
}

void WebEditorClient::willSetInputMethodState()
{
}

void WebEditorClient::setInputMethodState(bool)
{
    notImplemented();
}

bool WebEditorClient::supportsGlobalSelection()
{
#if PLATFORM(GTK) && PLATFORM(X11)
    return true;
#else
    // FIXME: Return true on other X11 platforms when they support global selection.
    return false;
#endif
}

#if ENABLE(SERVICE_CONTROLS) || ENABLE(TELEPHONE_NUMBER_DETECTION)
void WebEditorClient::selectedTelephoneNumberRangesChanged()
{
#if PLATFORM(MAC)
    m_page->servicesOverlayController().selectedTelephoneNumberRangesChanged();
#endif
}
void WebEditorClient::selectionRectsDidChange(const Vector<LayoutRect>& rects, const Vector<GapRects>& gapRects, bool isTextOnly)
{
#if PLATFORM(MAC)
    if (m_page->serviceControlsEnabled())
        m_page->servicesOverlayController().selectionRectsDidChange(rects, gapRects, isTextOnly);
#endif
}
#endif // ENABLE(SERVICE_CONTROLS) && ENABLE(TELEPHONE_NUMBER_DETECTION)

} // namespace WebKit
