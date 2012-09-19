/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "InputHandler.h"

#include "BackingStore.h"
#include "BackingStoreClient.h"
#include "CSSStyleDeclaration.h"
#include "Chrome.h"
#include "ColorPickerClient.h"
#include "DOMSupport.h"
#include "DatePickerClient.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DocumentMarkerController.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"
#include "HTMLTextAreaElement.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PluginView.h"
#include "Range.h"
#include "RenderLayer.h"
#include "RenderMenuList.h"
#include "RenderPart.h"
#include "RenderText.h"
#include "RenderTextControl.h"
#include "RenderWidget.h"
#include "RenderedDocumentMarker.h"
#include "ScopePointer.h"
#include "SelectPopupClient.h"
#include "SelectionHandler.h"
#include "SpellChecker.h"
#include "TextCheckerClient.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include "WebPageClient.h"
#include "WebPage_p.h"
#include "WebSettings.h"
#include "visible_units.h"

#include <BlackBerryPlatformKeyboardEvent.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformMisc.h>
#include <BlackBerryPlatformScreen.h>
#include <BlackBerryPlatformSettings.h>
#include <sys/keycodes.h>
#include <wtf/text/CString.h>

#define ENABLE_INPUT_LOG 0
#define ENABLE_FOCUS_LOG 0
#define ENABLE_SPELLING_LOG 0

static const unsigned MaxLearnTextDataSize = 500;
static const unsigned MaxSpellCheckingStringLength = 250;

using namespace BlackBerry::Platform;
using namespace WebCore;

#if ENABLE_INPUT_LOG
#define InputLog(severity, format, ...) logAlways(severity, format, ## __VA_ARGS__)
#else
#define InputLog(severity, format, ...)
#endif // ENABLE_INPUT_LOG

#if ENABLE_FOCUS_LOG
#define FocusLog(severity, format, ...) logAlways(severity, format, ## __VA_ARGS__)
#else
#define FocusLog(severity, format, ...)
#endif // ENABLE_FOCUS_LOG

#if ENABLE_SPELLING_LOG
#define SpellingLog(severity, format, ...) logAlways(severity, format, ## __VA_ARGS__)
#else
#define SpellingLog(severity, format, ...)
#endif // ENABLE_SPELLING_LOG

namespace BlackBerry {
namespace WebKit {

class ProcessingChangeGuard {
public:
    ProcessingChangeGuard(InputHandler* inputHandler)
        : m_inputHandler(inputHandler)
        , m_savedProcessingChange(false)
    {
        ASSERT(m_inputHandler);

        m_savedProcessingChange = m_inputHandler->processingChange();
        m_inputHandler->setProcessingChange(true);
    }

    ~ProcessingChangeGuard()
    {
        m_inputHandler->setProcessingChange(m_savedProcessingChange);
    }

private:
    InputHandler* m_inputHandler;
    bool m_savedProcessingChange;
};

InputHandler::InputHandler(WebPagePrivate* page)
    : m_webPage(page)
    , m_currentFocusElement(0)
    , m_inputModeEnabled(false)
    , m_processingChange(false)
    , m_changingFocus(false)
    , m_currentFocusElementType(TextEdit)
    , m_currentFocusElementTextEditMask(DEFAULT_STYLE)
    , m_composingTextStart(0)
    , m_composingTextEnd(0)
    , m_pendingKeyboardVisibilityChange(NoChange)
    , m_delayKeyboardVisibilityChange(false)
    , m_request(0)
    , m_processingTransactionId(-1)
    , m_focusZoomScale(0.0)
{
}

InputHandler::~InputHandler()
{
}

static BlackBerryInputType convertInputType(const HTMLInputElement* inputElement)
{
    if (inputElement->isPasswordField())
        return InputTypePassword;
    if (inputElement->isSearchField())
        return InputTypeSearch;
    if (inputElement->isEmailField())
        return InputTypeEmail;
    if (inputElement->isMonthControl())
        return InputTypeMonth;
    if (inputElement->isNumberField())
        return InputTypeNumber;
    if (inputElement->isTelephoneField())
        return InputTypeTelephone;
    if (inputElement->isURLField())
        return InputTypeURL;
#if ENABLE(INPUT_TYPE_COLOR)
    if (inputElement->isColorControl())
        return InputTypeColor;
#endif
    if (inputElement->isDateControl())
        return InputTypeDate;
    if (inputElement->isDateTimeControl())
        return InputTypeDateTime;
    if (inputElement->isDateTimeLocalControl())
        return InputTypeDateTimeLocal;
    if (inputElement->isTimeControl())
        return InputTypeTime;
    // FIXME: missing WEEK popup selector
    if (DOMSupport::elementIdOrNameIndicatesEmail(inputElement))
        return InputTypeEmail;
    if (DOMSupport::elementIdOrNameIndicatesUrl(inputElement))
        return InputTypeURL;
    if (DOMSupport::elementPatternIndicatesNumber(inputElement))
        return InputTypeNumber;
    if (DOMSupport::elementPatternIndicatesHexadecimal(inputElement))
        return InputTypeHexadecimal;

    return InputTypeText;
}

static int inputStyle(BlackBerryInputType type, const Element* element)
{
    switch (type) {
    case InputTypeEmail:
    case InputTypeURL:
    case InputTypeSearch:
    case InputTypeText:
    case InputTypeTextArea:
        {
            // Regular input mode, disable help if autocomplete is off.
            int imfMask = 0;
            DOMSupport::AttributeState autoCompleteState = DOMSupport::elementSupportsAutocomplete(element);
            if (autoCompleteState == DOMSupport::Off)
                imfMask = NO_AUTO_TEXT | NO_PREDICTION;
            else if (autoCompleteState != DOMSupport::On
                     && DOMSupport::elementIdOrNameIndicatesNoAutocomplete(element))
                imfMask = NO_AUTO_TEXT | NO_PREDICTION;

            // Disable autocorrection if it's specifically off, of if it is in default mode
            // and we have disabled auto text and prediction.
            if (DOMSupport::elementSupportsAutocorrect(element) == DOMSupport::Off
                || (imfMask && DOMSupport::elementSupportsAutocorrect(element) == DOMSupport::Default))
                imfMask |= NO_AUTO_CORRECTION;

            if (imfMask)
                return imfMask;
            if ((type == InputTypeEmail || type == InputTypeURL) && autoCompleteState != DOMSupport::On)
                return NO_AUTO_TEXT | NO_PREDICTION | NO_AUTO_CORRECTION;
            break;
        }
    case InputTypeIsIndex:
    case InputTypePassword:
    case InputTypeNumber:
    case InputTypeTelephone:
    case InputTypeHexadecimal:
        // Disable special handling.
        return NO_AUTO_TEXT | NO_PREDICTION | NO_AUTO_CORRECTION;
    default:
        break;
    }
    return DEFAULT_STYLE;
}

static VirtualKeyboardType convertStringToKeyboardType(const AtomicString& string)
{
    DEFINE_STATIC_LOCAL(AtomicString, Default, ("default"));
    DEFINE_STATIC_LOCAL(AtomicString, Url, ("url"));
    DEFINE_STATIC_LOCAL(AtomicString, Email, ("email"));
    DEFINE_STATIC_LOCAL(AtomicString, Password, ("password"));
    DEFINE_STATIC_LOCAL(AtomicString, Web, ("web"));
    DEFINE_STATIC_LOCAL(AtomicString, Number, ("number"));
    DEFINE_STATIC_LOCAL(AtomicString, Symbol, ("symbol"));
    DEFINE_STATIC_LOCAL(AtomicString, Phone, ("phone"));
    DEFINE_STATIC_LOCAL(AtomicString, Pin, ("pin"));
    DEFINE_STATIC_LOCAL(AtomicString, Hex, ("hexadecimal"));

    if (string.isEmpty())
        return VKBTypeNotSet;
    if (equalIgnoringCase(string, Default))
        return VKBTypeDefault;
    if (equalIgnoringCase(string, Url))
        return VKBTypeUrl;
    if (equalIgnoringCase(string, Email))
        return VKBTypeEmail;
    if (equalIgnoringCase(string, Password))
        return VKBTypePassword;
    if (equalIgnoringCase(string, Web))
        return VKBTypeWeb;
    if (equalIgnoringCase(string, Number))
        return VKBTypeNumPunc;
    if (equalIgnoringCase(string, Symbol))
        return VKBTypeSymbol;
    if (equalIgnoringCase(string, Phone))
        return VKBTypePhone;
    if (equalIgnoringCase(string, Pin) || equalIgnoringCase(string, Hex))
        return VKBTypePin;
    return VKBTypeNotSet;
}

static VirtualKeyboardType keyboardTypeAttribute(const WebCore::Element* element)
{
    DEFINE_STATIC_LOCAL(QualifiedName, keyboardTypeAttr, (nullAtom, "data-blackberry-virtual-keyboard-type", nullAtom));

    if (element->fastHasAttribute(keyboardTypeAttr)) {
        AtomicString attributeString = element->fastGetAttribute(keyboardTypeAttr);
        return convertStringToKeyboardType(attributeString);
    }

    if (element->isFormControlElement()) {
        const HTMLFormControlElement* formElement = static_cast<const HTMLFormControlElement*>(element);
        if (formElement->form() && formElement->form()->fastHasAttribute(keyboardTypeAttr)) {
            AtomicString attributeString = formElement->form()->fastGetAttribute(keyboardTypeAttr);
            return convertStringToKeyboardType(attributeString);
        }
    }

    return VKBTypeNotSet;
}

static VirtualKeyboardEnterKeyType convertStringToKeyboardEnterKeyType(const AtomicString& string)
{
    DEFINE_STATIC_LOCAL(AtomicString, Default, ("default"));
    DEFINE_STATIC_LOCAL(AtomicString, Connect, ("connect"));
    DEFINE_STATIC_LOCAL(AtomicString, Done, ("done"));
    DEFINE_STATIC_LOCAL(AtomicString, Go, ("go"));
    DEFINE_STATIC_LOCAL(AtomicString, Join, ("join"));
    DEFINE_STATIC_LOCAL(AtomicString, Next, ("next"));
    DEFINE_STATIC_LOCAL(AtomicString, Search, ("search"));
    DEFINE_STATIC_LOCAL(AtomicString, Send, ("send"));
    DEFINE_STATIC_LOCAL(AtomicString, Submit, ("submit"));

    if (string.isEmpty())
        return VKBEnterKeyNotSet;
    if (equalIgnoringCase(string, Default))
        return VKBEnterKeyDefault;
    if (equalIgnoringCase(string, Connect))
        return VKBEnterKeyConnect;
    if (equalIgnoringCase(string, Done))
        return VKBEnterKeyDone;
    if (equalIgnoringCase(string, Go))
        return VKBEnterKeyGo;
    if (equalIgnoringCase(string, Join))
        return VKBEnterKeyJoin;
    if (equalIgnoringCase(string, Next))
        return VKBEnterKeyNext;
    if (equalIgnoringCase(string, Search))
        return VKBEnterKeySearch;
    if (equalIgnoringCase(string, Send))
        return VKBEnterKeySend;
    if (equalIgnoringCase(string, Submit))
        return VKBEnterKeySubmit;
    return VKBEnterKeyNotSet;
}

static VirtualKeyboardEnterKeyType keyboardEnterKeyTypeAttribute(const WebCore::Element* element)
{
    DEFINE_STATIC_LOCAL(QualifiedName, keyboardEnterKeyTypeAttr, (nullAtom, "data-blackberry-virtual-keyboard-enter-key", nullAtom));

    if (element->fastHasAttribute(keyboardEnterKeyTypeAttr)) {
        AtomicString attributeString = element->fastGetAttribute(keyboardEnterKeyTypeAttr);
        return convertStringToKeyboardEnterKeyType(attributeString);
    }

    if (element->isFormControlElement()) {
        const HTMLFormControlElement* formElement = static_cast<const HTMLFormControlElement*>(element);
        if (formElement->form() && formElement->form()->fastHasAttribute(keyboardEnterKeyTypeAttr)) {
            AtomicString attributeString = formElement->form()->fastGetAttribute(keyboardEnterKeyTypeAttr);
            return convertStringToKeyboardEnterKeyType(attributeString);
        }
    }

    return VKBEnterKeyNotSet;
}

void InputHandler::setProcessingChange(bool processingChange)
{
    if (processingChange == m_processingChange)
        return;

    m_processingChange = processingChange;

    if (!m_processingChange)
        m_webPage->m_selectionHandler->inputHandlerDidFinishProcessingChange();
}

WTF::String InputHandler::elementText()
{
    if (!isActiveTextEdit())
        return WTF::String();

    return DOMSupport::inputElementText(m_currentFocusElement.get());
}

BlackBerryInputType InputHandler::elementType(Element* element) const
{
    // <isIndex> is bundled with input so we need to check the formControlName
    // first to differentiate it from input which is essentially the same as
    // isIndex has been deprecated.
    if (element->formControlName() == HTMLNames::isindexTag)
        return InputTypeIsIndex;

    if (const HTMLInputElement* inputElement = static_cast<const HTMLInputElement*>(element->toInputElement()))
        return convertInputType(inputElement);

    if (element->hasTagName(HTMLNames::textareaTag))
        return InputTypeTextArea;

    // Default to InputTypeTextArea for content editable fields.
    return InputTypeTextArea;
}

void InputHandler::focusedNodeChanged()
{
    ASSERT(m_webPage->m_page->focusController());
    Frame* frame = m_webPage->m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->document())
        return;

    Node* node = frame->document()->focusedNode();

    if (isActiveTextEdit() && m_currentFocusElement == node) {
        notifyClientOfKeyboardVisibilityChange(true);
        return;
    }

    if (node && node->isElementNode()) {
        Element* element = static_cast<Element*>(node);
        if (DOMSupport::isElementTypePlugin(element)) {
            setPluginFocused(element);
            return;
        }

        if (DOMSupport::isTextBasedContentEditableElement(element)) {
            // Focused node is a text based input field, textarea or content editable field.
            setElementFocused(element);
            return;
        }
    }

    if (isActiveTextEdit() && m_currentFocusElement->isContentEditable()) {
        // This is a special handler for content editable fields. The focus node is the top most
        // field that is content editable, however, by enabling / disabling designmode and
        // content editable, it is possible through javascript or selection to trigger the focus node to
        // change even while maintaining editing on the same selection point. If the focus element
        // isn't contentEditable, but the current selection is, don't send a change notification.

        // When processing changes selection changes occur that may reset the focus element
        // and could potentially cause a crash as m_currentFocusElement should not be
        // changed during processing of an EditorCommand.
        if (processingChange())
            return;

        Frame* frame = m_currentFocusElement->document()->frame();
        ASSERT(frame);

        // Test the current selection to make sure that the content in focus is still content
        // editable. This may mean Javascript triggered a focus change by modifying the
        // top level parent of this object's content editable state without actually modifying
        // this particular object.
        // Example site: html5demos.com/contentEditable - blur event triggers focus change.
        if (frame == m_webPage->focusedOrMainFrame() && frame->selection()->start().anchorNode()
            && frame->selection()->start().anchorNode()->isContentEditable())
                return;
    }

    // No valid focus element found for handling.
    setElementUnfocused();
}

void InputHandler::setPluginFocused(Element* element)
{
    ASSERT(DOMSupport::isElementTypePlugin(element));

    if (isActiveTextEdit())
        setElementUnfocused();

    m_currentFocusElementType = Plugin;
    m_currentFocusElement = element;
}

static bool convertStringToWchar(const String& string, wchar_t* dest, int destCapacity, int* destLength)
{
    ASSERT(dest);

    if (!string.length()) {
        destLength = 0;
        return true;
    }

    UErrorCode ec = U_ZERO_ERROR;
    // wchar_t strings sent to IMF are 32 bit so casting to UChar32 is safe.
    u_strToUTF32(reinterpret_cast<UChar32*>(dest), destCapacity, destLength, string.characters(), string.length(), &ec);
    if (ec) {
        logAlways(LogLevelCritical, "InputHandler::convertStringToWchar Error converting string ec (%d).", ec);
        destLength = 0;
        return false;
    }
    return true;
}

static bool convertStringToWcharVector(const String& string, WTF::Vector<wchar_t>& wcharString)
{
    ASSERT(wcharString.isEmpty());

    int length = string.length();
    if (!length)
        return true;

    if (!wcharString.tryReserveCapacity(length + 1)) {
        logAlways(LogLevelCritical, "InputHandler::convertStringToWcharVector Cannot allocate memory for string.");
        return false;
    }

    int destLength = 0;
    if (!convertStringToWchar(string, wcharString.data(), length + 1, &destLength))
        return false;

    wcharString.resize(destLength);
    return true;
}

static String convertSpannableStringToString(spannable_string_t* src)
{
    if (!src || !src->str || !src->length)
        return String();

    WTF::Vector<UChar> dest;
    int destCapacity = (src->length * 2) + 1;
    if (!dest.tryReserveCapacity(destCapacity)) {
        logAlways(LogLevelCritical, "InputHandler::convertSpannableStringToString Cannot allocate memory for string.");
        return String();
    }

    int destLength = 0;
    UErrorCode ec = U_ZERO_ERROR;
    // wchar_t strings sent from IMF are 32 bit so casting to UChar32 is safe.
    u_strFromUTF32(dest.data(), destCapacity, &destLength, reinterpret_cast<UChar32*>(src->str), src->length, &ec);
    if (ec) {
        logAlways(LogLevelCritical, "InputHandler::convertSpannableStringToString Error converting string ec (%d).", ec);
        return String();
    }
    dest.resize(destLength);
    return String(dest.data(), destLength);
}

void InputHandler::sendLearnTextDetails(const WTF::String& string)
{
    Vector<wchar_t> wcharString;
    if (!convertStringToWcharVector(string, wcharString) || wcharString.isEmpty())
        return;

    m_webPage->m_client->inputLearnText(wcharString.data(), wcharString.size());
}

void InputHandler::learnText()
{
    if (!isActiveTextEdit())
        return;

    // Do not send (or calculate) the text when the field is NO_PREDICTION or NO_AUTO_TEXT.
    if (m_currentFocusElementTextEditMask & NO_PREDICTION || m_currentFocusElementTextEditMask & NO_AUTO_TEXT)
        return;

    String textInField(elementText());
    textInField = textInField.substring(std::max(0, static_cast<int>(textInField.length() - MaxLearnTextDataSize)), textInField.length());
    textInField.remove(0, textInField.find(" "));

    // Build up the 500 character strings in word chunks.
    // Spec says 1000, but memory corruption has been observed.
    ASSERT(textInField.length() <= MaxLearnTextDataSize);

    if (textInField.isEmpty())
        return;

    InputLog(LogLevelInfo, "InputHandler::learnText '%s'", textInField.latin1().data());
    sendLearnTextDetails(textInField);
}

void InputHandler::requestCheckingOfString(PassRefPtr<WebCore::TextCheckingRequest> textCheckingRequest)
{
    m_request = textCheckingRequest;

    if (!m_request) {
        SpellingLog(LogLevelWarn, "InputHandler::requestCheckingOfString did not receive a valid request.");
        return;
    }

    unsigned requestLength = m_request->text().length();

    // Check if the field should be spellchecked.
    if (!isActiveTextEdit() || !shouldSpellCheckElement(m_currentFocusElement.get()) || requestLength < 2) {
        m_request->didCancel();
        return;
    }

    if (requestLength > MaxSpellCheckingStringLength) {
        // Cancel this request and send it off in newly created chunks.
        m_request->didCancel();
        if (m_currentFocusElement->document() && m_currentFocusElement->document()->frame() && m_currentFocusElement->document()->frame()->selection()) {
            // Convert from position back to selection so we can expand the range to include the previous line. This should handle cases when the user hits
            // enter to finish composing a word and create a new line.
            VisiblePosition caretPosition = m_currentFocusElement->document()->frame()->selection()->start();
            VisibleSelection visibleSelection = VisibleSelection(previousLinePosition(caretPosition, caretPosition.lineDirectionPointForBlockDirectionNavigation()), caretPosition);
            spellCheckBlock(visibleSelection, TextCheckingProcessIncremental);
        }
        return;
    }

    wchar_t* checkingString = (wchar_t*)malloc(sizeof(wchar_t) * (requestLength + 1));
    if (!checkingString) {
        logAlways(LogLevelCritical, "InputHandler::requestCheckingOfString Cannot allocate memory for string.");
        m_request->didCancel();
        return;
    }

    int paragraphLength = 0;
    if (!convertStringToWchar(m_request->text(), checkingString, requestLength + 1, &paragraphLength)) {
        logAlways(LogLevelCritical, "InputHandler::requestCheckingOfString Failed to convert String to wchar type.");
        free(checkingString);
        m_request->didCancel();
        return;
    }

    m_processingTransactionId = m_webPage->m_client->checkSpellingOfStringAsync(checkingString, paragraphLength);
    free(checkingString);

    // If the call to the input service did not go through, then cancel the request so we don't block endlessly.
    // This should still take transactionId as a parameter to maintain the same behavior as if InputMethodSupport
    // were to cancel a request during processing.
    if (m_processingTransactionId == -1) { // Error before sending request to input service.
        m_request->didCancel();
        return;
    }
}

void InputHandler::spellCheckingRequestCancelled(int32_t transactionId)
{
    SpellingLog(LogLevelWarn, "InputHandler::spellCheckingRequestCancelled Expected transaction id %d, received %d. %s"
                , transactionId
                , m_processingTransactionId
                , transactionId == m_processingTransactionId ? "" : "We are out of sync with input service.");

    m_request->didCancel();
    m_processingTransactionId = -1;
}

void InputHandler::spellCheckingRequestProcessed(int32_t transactionId, spannable_string_t* spannableString)
{
    SpellingLog(LogLevelWarn, "InputHandler::spellCheckingRequestProcessed Expected transaction id %d, received %d. %s"
                , transactionId
                , m_processingTransactionId
                , transactionId == m_processingTransactionId ? "" : "We are out of sync with input service.");

    if (!spannableString || !isActiveTextEdit()) {
        SpellingLog(LogLevelWarn, "InputHandler::spellCheckingRequestProcessed Cancelling request with transactionId %d.", transactionId);
        m_request->didCancel();
        m_processingTransactionId = -1;
        return;
    }

    Vector<TextCheckingResult> results;

    // Convert the spannableString to TextCheckingResult then append to results vector.
    String replacement;
    TextCheckingResult textCheckingResult;
    textCheckingResult.type = TextCheckingTypeSpelling;
    textCheckingResult.replacement = replacement;
    textCheckingResult.location = 0;
    textCheckingResult.length = 0;

    span_t* span = spannableString->spans;
    for (unsigned int i = 0; i < spannableString->spans_count; i++) {
        if (!span)
            break;
        if (span->end < span->start) {
            m_request->didCancel();
            return;
        }
        if (span->attributes_mask & MISSPELLED_WORD_ATTRIB) {
            textCheckingResult.location = span->start;
            // The end point includes the character that it is before. Ie, 0, 0
            // applies to the first character as the end point includes the character
            // at the position. This means the endPosition is always +1.
            textCheckingResult.length = span->end - span->start + 1;
            results.append(textCheckingResult);
        }
        span++;
    }

    m_request->didSucceed(results);
}

SpellChecker* InputHandler::getSpellChecker()
{
    if (!m_currentFocusElement || !m_currentFocusElement->document())
        return 0;

    if (Frame* frame = m_currentFocusElement->document()->frame())
        if (Editor* editor = frame->editor())
            return editor->spellChecker();

    return 0;
}

bool InputHandler::shouldRequestSpellCheckingOptionsForPoint(Platform::IntPoint& point, const Element* touchedElement, imf_sp_text_t& spellCheckingOptionRequest)
{
    if (!isActiveTextEdit() || touchedElement != m_currentFocusElement)
        return false;

    LayoutPoint contentPos(m_webPage->mapFromViewportToContents(point));
    contentPos = DOMSupport::convertPointToFrame(m_webPage->mainFrame(), m_webPage->focusedOrMainFrame(), roundedIntPoint(contentPos));

    Document* document = m_currentFocusElement->document();
    ASSERT(document);

    RenderedDocumentMarker* marker = document->markers()->renderedMarkerContainingPoint(contentPos, DocumentMarker::Spelling);
    if (!marker)
        return false;

    SpellingLog(LogLevelInfo, "InputHandler::shouldRequestSpellCheckingOptionsForPoint Found spelling marker at point %d, %d", point.x(), point.y());

    // imf_sp_text_t should be generated in pixel viewport coordinates.
    WebCore::IntRect rect = m_webPage->mapToTransformed(m_webPage->focusedOrMainFrame()->view()->contentsToWindow(enclosingIntRect(marker->renderedRect())));
    m_webPage->clipToTransformedContentsRect(rect);

    // TODO use the actual caret position after it is placed.
    spellCheckingOptionRequest.caret_rect.caret_top_x = point.x();
    spellCheckingOptionRequest.caret_rect.caret_top_y = rect.y();
    spellCheckingOptionRequest.caret_rect.caret_bottom_x = point.x();
    spellCheckingOptionRequest.caret_rect.caret_bottom_y = rect.y() + rect.height();
    spellCheckingOptionRequest.startTextPosition = marker->startOffset();
    spellCheckingOptionRequest.endTextPosition = marker->endOffset();

    SpellingLog(LogLevelInfo, "InputHandler::shouldRequestSpellCheckingOptionsForPoint spellCheckingOptionRequest\ntop %d, %d\nbottom %d %d\nMarker start %d end %d"
                , spellCheckingOptionRequest.caret_rect.caret_top_x, spellCheckingOptionRequest.caret_rect.caret_top_y
                , spellCheckingOptionRequest.caret_rect.caret_bottom_x, spellCheckingOptionRequest.caret_rect.caret_bottom_y
                , spellCheckingOptionRequest.startTextPosition, spellCheckingOptionRequest.endTextPosition);

    return true;
}

void InputHandler::requestSpellingCheckingOptions(imf_sp_text_t& spellCheckingOptionRequest)
{
    SpellingLog(LogLevelInfo, "InputHandler::requestSpellingCheckingOptions Sending request:\ncaret_rect.caret_top_x = %d\ncaret_rect.caret_top_y = %d" \
                              "\ncaret_rect.caret_bottom_x = %d\ncaret_rect.caret_bottom_y = %d\nstartTextPosition = %d\nendTextPosition = %d",
                              spellCheckingOptionRequest.caret_rect.caret_top_x, spellCheckingOptionRequest.caret_rect.caret_top_y,
                              spellCheckingOptionRequest.caret_rect.caret_bottom_x, spellCheckingOptionRequest.caret_rect.caret_bottom_y,
                              spellCheckingOptionRequest.startTextPosition, spellCheckingOptionRequest.endTextPosition);

    if (spellCheckingOptionRequest.startTextPosition || spellCheckingOptionRequest.endTextPosition)
        m_webPage->m_client->requestSpellingCheckingOptions(spellCheckingOptionRequest);
}

void InputHandler::setElementUnfocused(bool refocusOccuring)
{
    if (isActiveTextEdit()) {
        FocusLog(LogLevelInfo, "InputHandler::setElementUnfocused");

        // Pass any text into the field to IMF to learn.
        learnText();

        // End any composition that is in progress.
        finishComposition();

        // Only hide the keyboard if we aren't refocusing on a new input field.
        if (!refocusOccuring)
            notifyClientOfKeyboardVisibilityChange(false);

        m_webPage->m_client->inputFocusLost();

        // If the frame selection isn't focused, focus it.
        if (!m_currentFocusElement->document()->frame()->selection()->isFocused())
            m_currentFocusElement->document()->frame()->selection()->setFocused(true);
    }

    // Clear the node details.
    m_currentFocusElement = 0;
    m_currentFocusElementType = TextEdit;
}

bool InputHandler::isInputModeEnabled() const
{
    // Input mode is enabled when set, or when dump render tree or always show keyboard setting is enabled.
    return m_inputModeEnabled || m_webPage->m_dumpRenderTree || Platform::Settings::instance()->alwaysShowKeyboardOnFocus();
}

void InputHandler::setInputModeEnabled(bool active)
{
    FocusLog(LogLevelInfo, "InputHandler::setInputModeEnabled '%s', override is '%s'"
             , active ? "true" : "false"
             , m_webPage->m_dumpRenderTree || Platform::Settings::instance()->alwaysShowKeyboardOnFocus() ? "true" : "false");

    m_inputModeEnabled = active;

    // If the frame selection isn't focused, focus it.
    if (isInputModeEnabled() && isActiveTextEdit() && !m_currentFocusElement->document()->frame()->selection()->isFocused())
        m_currentFocusElement->document()->frame()->selection()->setFocused(true);
}

void InputHandler::setElementFocused(Element* element)
{
    ASSERT(DOMSupport::isTextBasedContentEditableElement(element));
    ASSERT(element && element->document() && element->document()->frame());

#ifdef ENABLE_SPELLING_LOG
    BlackBerry::Platform::StopWatch timer;
    timer.start();
#endif

    if (!element || !(element->document()))
        return;

    Frame* frame = element->document()->frame();
    if (!frame)
        return;

    if (frame->selection()->isFocused() != isInputModeEnabled())
        frame->selection()->setFocused(isInputModeEnabled());

    // Clear the existing focus node details.
    setElementUnfocused(true /*refocusOccuring*/);

    // Mark this element as active and add to frame set.
    m_currentFocusElement = element;
    m_currentFocusElementType = TextEdit;

    // Send details to the client about this element.
    BlackBerryInputType type = elementType(element);
    m_currentFocusElementTextEditMask = inputStyle(type, element);

    VirtualKeyboardType keyboardType = keyboardTypeAttribute(element);
    VirtualKeyboardEnterKeyType enterKeyType = keyboardEnterKeyTypeAttribute(element);

    if (enterKeyType == VKBEnterKeyNotSet && type != InputTypeTextArea) {
        if (element->isFormControlElement()) {
            const HTMLFormControlElement* formElement = static_cast<const HTMLFormControlElement*>(element);
            if (formElement->form() && formElement->form()->defaultButton())
                enterKeyType = VKBEnterKeySubmit;
        }
    }

    FocusLog(LogLevelInfo, "InputHandler::setElementFocused, Type=%d, Style=%d, Keyboard Type=%d, Enter Key=%d", type, m_currentFocusElementTextEditMask, keyboardType, enterKeyType);
    m_webPage->m_client->inputFocusGained(type, m_currentFocusElementTextEditMask, keyboardType, enterKeyType);

    handleInputLocaleChanged(m_webPage->m_webSettings->isWritingDirectionRTL());

    if (!m_delayKeyboardVisibilityChange)
        notifyClientOfKeyboardVisibilityChange(true);

#ifdef ENABLE_SPELLING_LOG
    SpellingLog(LogLevelInfo, "InputHandler::setElementFocused Focusing the field took %f seconds.", timer.elapsed());
#endif

    // Check if the field should be spellchecked.
    if (!shouldSpellCheckElement(element))
        return;

    // Spellcheck the field in its entirety.
    VisibleSelection focusedBlock = DOMSupport::visibleSelectionForInputElement(element);
    spellCheckBlock(focusedBlock, TextCheckingProcessBatch);

#ifdef ENABLE_SPELLING_LOG
    SpellingLog(LogLevelInfo, "InputHandler::setElementFocused Spellchecking the field increased the total time to focus to %f seconds.", timer.elapsed());
#endif
}

bool InputHandler::shouldSpellCheckElement(const Element* element) const
{
    DOMSupport::AttributeState spellCheckAttr = DOMSupport::elementSupportsSpellCheck(element);

    // Explicitly set to off.
    if (spellCheckAttr == DOMSupport::Off)
        return false;

    // Undefined and part of a set of cases which we do not wish to check. This includes user names and email addresses, so we are piggybacking on NoAutocomplete cases.
    if (spellCheckAttr == DOMSupport::Default && (m_currentFocusElementTextEditMask & NO_AUTO_TEXT))
        return false;

    return true;
}

void InputHandler::spellCheckBlock(VisibleSelection& visibleSelection, TextCheckingProcessType textCheckingProcessType)
{
    if (!isActiveTextEdit())
        return;

    RefPtr<Range> rangeForSpellChecking = visibleSelection.toNormalizedRange();
    if (!rangeForSpellChecking || !rangeForSpellChecking->text() || !rangeForSpellChecking->text().length())
        return;

    SpellChecker* spellChecker = getSpellChecker();
    if (!spellChecker) {
        SpellingLog(LogLevelInfo, "InputHandler::spellCheckBlock Failed to spellcheck the current focused element.");
        return;
    }

    // If we have a batch request, try to send off the entire block.
    if (textCheckingProcessType == TextCheckingProcessBatch) {
        // If total block text is under the limited amount, send the entire chunk.
        if (rangeForSpellChecking->text().length() < MaxSpellCheckingStringLength) {
            spellChecker->requestCheckingFor(SpellCheckRequest::create(TextCheckingTypeSpelling, TextCheckingProcessBatch, rangeForSpellChecking, rangeForSpellChecking));
            return;
        }
    }

    // Since we couldn't check the entire block at once, set up starting and ending markers to fire incrementally.
    VisiblePosition startPos = visibleSelection.visibleStart();
    VisiblePosition startOfCurrentLine = startOfLine(startPos);
    VisiblePosition endOfCurrentLine = endOfLine(startOfCurrentLine);

    while (!isEndOfBlock(startOfCurrentLine)) {
        // Create a selection with the start and end points of the line, and convert to Range to create a SpellCheckRequest.
        rangeForSpellChecking = VisibleSelection(startOfCurrentLine, endOfCurrentLine).toNormalizedRange();

        if (rangeForSpellChecking->text().length() < MaxSpellCheckingStringLength) {
            startOfCurrentLine = nextLinePosition(startOfCurrentLine, startOfCurrentLine.lineDirectionPointForBlockDirectionNavigation());
            endOfCurrentLine = endOfLine(startOfCurrentLine);
        } else {
            // Iterate through words from the start of the line to the end.
            rangeForSpellChecking = getRangeForSpellCheckWithFineGranularity(startOfCurrentLine, endOfCurrentLine);
            if (!rangeForSpellChecking) {
                SpellingLog(LogLevelWarn, "InputHandler::spellCheckBlock Failed to set text range for spellchecking");
                return;
            }
            startOfCurrentLine = VisiblePosition(rangeForSpellChecking->endPosition());
        }

        SpellingLog(LogLevelInfo, "InputHandler::spellCheckBlock Substring text is '%s', of size %d", rangeForSpellChecking->text().latin1().data(), rangeForSpellChecking->text().length());

        // Call spellcheck with substring.
        spellChecker->requestCheckingFor(SpellCheckRequest::create(TextCheckingTypeSpelling, TextCheckingProcessBatch, rangeForSpellChecking, rangeForSpellChecking));
    }
}

PassRefPtr<Range> InputHandler::getRangeForSpellCheckWithFineGranularity(VisiblePosition startPosition, VisiblePosition endPosition)
{
    VisiblePosition endOfCurrentWord = endOfWord(startPosition);

    // Keep iterating until one of our cases is hit, or we've incremented the starting position right to the end.
    while (startPosition != endPosition) {
        // Check the text length within this range.
        if (VisibleSelection(startPosition, endOfCurrentWord).toNormalizedRange()->text().length() >= MaxSpellCheckingStringLength) {
            // If this is not the first word, return a Range with end boundary set to the previous word.
            if (startOfWord(endOfCurrentWord, LeftWordIfOnBoundary) != startPosition)
                return VisibleSelection(startPosition, endOfWord(previousWordPosition(endOfCurrentWord), LeftWordIfOnBoundary)).toNormalizedRange();

            // Our first word has gone over the character limit. Increment the starting position past an uncheckable word.
            startPosition = endOfCurrentWord;
        } else if (endOfCurrentWord == endPosition) {
            // Return the last segment if the end of our word lies at the end of the range.
            return VisibleSelection(startPosition, endPosition).toNormalizedRange();
        } else {
            // Increment the current word.
            endOfCurrentWord = endOfWord(nextWordPosition(endOfCurrentWord));
        }
    }
    return 0;
}

bool InputHandler::openDatePopup(HTMLInputElement* element, BlackBerryInputType type)
{
    if (!element || element->disabled() || !DOMSupport::isDateTimeInputField(element))
        return false;

    if (isActiveTextEdit())
        clearCurrentFocusElement();

    switch (type) {
    case BlackBerry::Platform::InputTypeDate:
    case BlackBerry::Platform::InputTypeTime:
    case BlackBerry::Platform::InputTypeDateTime:
    case BlackBerry::Platform::InputTypeDateTimeLocal:
    case BlackBerry::Platform::InputTypeMonth: {
        // Check if popup already exists, close it if does.
        m_webPage->m_page->chrome()->client()->closePagePopup(0);
        String value = element->value();
        String min = element->getAttribute(HTMLNames::minAttr).string();
        String max = element->getAttribute(HTMLNames::maxAttr).string();
        double step = element->getAttribute(HTMLNames::stepAttr).toDouble();

        DatePickerClient* client = new DatePickerClient(type, value, min, max, step,  m_webPage, element);
        return m_webPage->m_page->chrome()->client()->openPagePopup(client,  WebCore::IntRect());
        }
    default: // Other types not supported
        return false;
    }
}

bool InputHandler::openColorPopup(HTMLInputElement* element)
{
    if (!element || element->disabled() || !DOMSupport::isColorInputField(element))
        return false;

    if (isActiveTextEdit())
        clearCurrentFocusElement();

    m_currentFocusElement = element;
    m_currentFocusElementType = TextPopup;

    // Check if popup already exists, close it if does.
    m_webPage->m_page->chrome()->client()->closePagePopup(0);

    ColorPickerClient* client = new ColorPickerClient(element->value(), m_webPage, element);
    return m_webPage->m_page->chrome()->client()->openPagePopup(client,  WebCore::IntRect());
}

void InputHandler::setInputValue(const WTF::String& value)
{
    if (!isActiveTextPopup())
        return;

    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(m_currentFocusElement.get());
    inputElement->setValue(value);
    clearCurrentFocusElement();
}

void InputHandler::nodeTextChanged(const Node* node)
{
    if (processingChange() || !node)
        return;

    if (node != m_currentFocusElement)
        return;

    InputLog(LogLevelInfo, "InputHandler::nodeTextChanged");

    m_webPage->m_client->inputTextChanged();

    // Remove the attributed text markers as the previous call triggered an end to
    // the composition.
    removeAttributedTextMarker();
}

WebCore::IntRect InputHandler::boundingBoxForInputField()
{
    if (!isActiveTextEdit())
        return WebCore::IntRect();

    if (!m_currentFocusElement->renderer())
        return WebCore::IntRect();

    return m_currentFocusElement->renderer()->absoluteBoundingBoxRect();
}

void InputHandler::ensureFocusTextElementVisible(CaretScrollType scrollType)
{
    if (!isActiveTextEdit() || !isInputModeEnabled() || !m_currentFocusElement->document())
        return;

    if (!(Platform::Settings::instance()->allowedScrollAdjustmentForInputFields() & scrollType))
        return;

    Frame* elementFrame = m_currentFocusElement->document()->frame();
    if (!elementFrame)
        return;

    Frame* mainFrame = m_webPage->mainFrame();
    if (!mainFrame)
        return;

    FrameView* mainFrameView = mainFrame->view();
    if (!mainFrameView)
        return;

    WebCore::IntRect selectionFocusRect;
    switch (elementFrame->selection()->selectionType()) {
    case VisibleSelection::CaretSelection:
        selectionFocusRect = elementFrame->selection()->absoluteCaretBounds();
        break;
    case VisibleSelection::RangeSelection: {
        Position selectionPosition;
        if (m_webPage->m_selectionHandler->lastUpdatedEndPointIsValid())
            selectionPosition = elementFrame->selection()->end();
        else
            selectionPosition = elementFrame->selection()->start();
        selectionFocusRect = VisiblePosition(selectionPosition).absoluteCaretBounds();
        break;
    }
    case VisibleSelection::NoSelection:
        return;
    }

    int fontHeight = selectionFocusRect.height();

    if (elementFrame != mainFrame) { // Element is in a subframe.
        // Remove any scroll offset within the subframe to get the point relative to the main frame.
        selectionFocusRect.move(-elementFrame->view()->scrollPosition().x(), -elementFrame->view()->scrollPosition().y());

        // Adjust the selection rect based on the frame offset in relation to the main frame if it's a subframe.
        if (elementFrame->ownerRenderer()) {
            WebCore::IntPoint frameOffset = elementFrame->ownerRenderer()->absoluteContentBox().location();
            selectionFocusRect.move(frameOffset.x(), frameOffset.y());
        }
    }

    Position start = elementFrame->selection()->start();
    if (start.anchorNode() && start.anchorNode()->renderer()) {
        if (RenderLayer* layer = start.anchorNode()->renderer()->enclosingLayer()) {
            WebCore::IntRect actualScreenRect = WebCore::IntRect(mainFrameView->scrollPosition(), m_webPage->actualVisibleSize());
            ScrollAlignment horizontalScrollAlignment = ScrollAlignment::alignToEdgeIfNeeded;
            ScrollAlignment verticalScrollAlignment = ScrollAlignment::alignToEdgeIfNeeded;

            if (scrollType != EdgeIfNeeded) {
                // Align the selection rect if possible so that we show the field's
                // outline if the caret is at the edge of the field.
                if (RenderObject* focusedRenderer = m_currentFocusElement->renderer()) {
                    WebCore::IntRect nodeOutlineBounds = focusedRenderer->absoluteOutlineBounds();
                    WebCore::IntRect caretAtEdgeRect = rectForCaret(0);
                    int paddingX = abs(caretAtEdgeRect.x() - nodeOutlineBounds.x());
                    int paddingY = abs(caretAtEdgeRect.y() - nodeOutlineBounds.y());

                    if (selectionFocusRect.x() - paddingX == nodeOutlineBounds.x())
                        selectionFocusRect.setX(nodeOutlineBounds.x());
                    else if (selectionFocusRect.maxX() + paddingX == nodeOutlineBounds.maxX())
                        selectionFocusRect.setX(nodeOutlineBounds.maxX() - selectionFocusRect.width());
                    if (selectionFocusRect.y() - paddingY == nodeOutlineBounds.y())
                        selectionFocusRect.setY(nodeOutlineBounds.y() - selectionFocusRect.height());
                    else if (selectionFocusRect.maxY() + paddingY == nodeOutlineBounds.maxY())
                        selectionFocusRect.setY(nodeOutlineBounds.maxY() - selectionFocusRect.height());

                    // If the editing point is on the left hand side of the screen when the node's
                    // rect is edge aligned, edge align the node rect.
                    if (selectionFocusRect.x() - caretAtEdgeRect.x() < actualScreenRect.width() / 2)
                        selectionFocusRect.setX(nodeOutlineBounds.x());
                    else
                        horizontalScrollAlignment = ScrollAlignment::alignCenterIfNeeded;

                }
                verticalScrollAlignment = (scrollType == CenterAlways) ? ScrollAlignment::alignCenterAlways : ScrollAlignment::alignCenterIfNeeded;
            }

            // Pad the rect to improve the visual appearance.
            // Padding must be large enough to expose the selection / FCC should they exist. Dragging the handle offscreen and releasing
            // will not trigger an automatic scroll. Using a padding of 40 will fully exposing the width of the current handle and half of
            // the height making it usable.
            // FIXME: This will need to be updated when the graphics change.
            // FIXME: The value of 40 should be calculated as a unit of measure using Graphics::Screen::primaryScreen()->heightInMMToPixels
            // using a relative value to the size of the handle. We should also consider expanding different amounts horizontally vs vertically.
            selectionFocusRect.inflate(40 /* padding in pixels */);
            WebCore::IntRect revealRect = layer->getRectToExpose(actualScreenRect, selectionFocusRect,
                                                                 horizontalScrollAlignment,
                                                                 verticalScrollAlignment);

            mainFrameView->setConstrainsScrollingToContentEdge(false);
            // In order to adjust the scroll position to ensure the focused input field is visible,
            // we allow overscrolling. However this overscroll has to be strictly allowed towards the
            // bottom of the page on the y axis only, where the virtual keyboard pops up from.
            WebCore::IntPoint scrollLocation = revealRect.location();
            scrollLocation.clampNegativeToZero();
            WebCore::IntPoint maximumScrollPosition = WebCore::IntPoint(mainFrameView->contentsWidth() - actualScreenRect.width(), mainFrameView->contentsHeight() - actualScreenRect.height());
            scrollLocation = scrollLocation.shrunkTo(maximumScrollPosition);
            mainFrameView->setScrollPosition(scrollLocation);
            mainFrameView->setConstrainsScrollingToContentEdge(true);
        }
    }

    // If the text is too small, zoom in to make it a minimum size.
    // The minimum size being defined as 3 mm is a good value based on my observations.
    static const int s_minimumTextHeightInPixels = Graphics::Screen::primaryScreen()->widthInMMToPixels(3);
    if (fontHeight && fontHeight < s_minimumTextHeightInPixels) {
        m_focusZoomScale = s_minimumTextHeightInPixels / fontHeight;
        m_focusZoomLocation = selectionFocusRect.location();
        m_webPage->zoomAboutPoint(m_focusZoomScale, m_focusZoomLocation);
    } else {
        m_focusZoomScale = 0.0;
        m_focusZoomLocation = WebCore::IntPoint();
    }
}

void InputHandler::ensureFocusPluginElementVisible()
{
    if (!isActivePlugin() || !m_currentFocusElement->document())
        return;

    Frame* elementFrame = m_currentFocusElement->document()->frame();
    if (!elementFrame)
        return;

    Frame* mainFrame = m_webPage->mainFrame();
    if (!mainFrame)
        return;

    FrameView* mainFrameView = mainFrame->view();
    if (!mainFrameView)
        return;

    WebCore::IntRect selectionFocusRect;

    RenderWidget* renderWidget = static_cast<RenderWidget*>(m_currentFocusElement->renderer());
    if (renderWidget) {
        PluginView* pluginView = static_cast<PluginView*>(renderWidget->widget());

        if (pluginView)
            selectionFocusRect = pluginView->ensureVisibleRect();
    }

    if (selectionFocusRect.isEmpty())
        return;

    // FIXME: We may need to scroll the subframe (recursively) in the future. Revisit this...
    if (elementFrame != mainFrame) { // Element is in a subframe.
        // Remove any scroll offset within the subframe to get the point relative to the main frame.
        selectionFocusRect.move(-elementFrame->view()->scrollPosition().x(), -elementFrame->view()->scrollPosition().y());

        // Adjust the selection rect based on the frame offset in relation to the main frame if it's a subframe.
        if (elementFrame->ownerRenderer()) {
            WebCore::IntPoint frameOffset = elementFrame->ownerRenderer()->absoluteContentBox().location();
            selectionFocusRect.move(frameOffset.x(), frameOffset.y());
        }
    }

    WebCore::IntRect actualScreenRect = WebCore::IntRect(mainFrameView->scrollPosition(), m_webPage->actualVisibleSize());
    if (actualScreenRect.contains(selectionFocusRect))
        return;

    // Calculate a point such that the center of the requested rectangle
    // is at the center of the screen. FIXME: If the element was partially on screen
    // we might want to just bring the offscreen portion into view, someone needs
    // to decide if that's the behavior we want or not.
    WebCore::IntPoint pos(selectionFocusRect.center().x() - actualScreenRect.width() / 2,
                 selectionFocusRect.center().y() - actualScreenRect.height() / 2);

    mainFrameView->setScrollPosition(pos);
}

void InputHandler::ensureFocusElementVisible(bool centerInView)
{
    if (isActivePlugin())
        ensureFocusPluginElementVisible();
    else
        ensureFocusTextElementVisible(centerInView ? CenterAlways : CenterIfNeeded);
}

void InputHandler::frameUnloaded(const Frame* frame)
{
    if (!isActiveTextEdit())
        return;

    if (m_currentFocusElement->document()->frame() != frame)
        return;

    FocusLog(LogLevelInfo, "InputHandler::frameUnloaded");

    setElementUnfocused(false /*refocusOccuring*/);
}

void InputHandler::setDelayKeyboardVisibilityChange(bool value)
{
    m_delayKeyboardVisibilityChange = value;
    m_pendingKeyboardVisibilityChange = NoChange;
}

void InputHandler::processPendingKeyboardVisibilityChange()
{
    if (!m_delayKeyboardVisibilityChange) {
        ASSERT(m_pendingKeyboardVisibilityChange == NoChange);
        return;
    }

    m_delayKeyboardVisibilityChange = false;

    if (m_pendingKeyboardVisibilityChange == NoChange)
        return;

    notifyClientOfKeyboardVisibilityChange(m_pendingKeyboardVisibilityChange == Visible);
    m_pendingKeyboardVisibilityChange = NoChange;
}

void InputHandler::notifyClientOfKeyboardVisibilityChange(bool visible)
{
    // If we aren't ready for input, keyboard changes should be ignored.
    if (!isInputModeEnabled() && visible)
        return;

    if (processingChange()) {
        ASSERT(visible);
        return;
    }

    if (!m_delayKeyboardVisibilityChange) {
        m_webPage->showVirtualKeyboard(visible);
        return;
    }

    m_pendingKeyboardVisibilityChange = visible ? Visible : NotVisible;
}

bool InputHandler::selectionAtStartOfElement()
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    if (!selectionStart())
        return true;

    return false;
}

bool InputHandler::selectionAtEndOfElement()
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    return selectionStart() == static_cast<int>(elementText().length());
}

int InputHandler::selectionStart() const
{
    return selectionPosition(true);
}

int InputHandler::selectionEnd() const
{
    return selectionPosition(false);
}

int InputHandler::selectionPosition(bool start) const
{
    if (!m_currentFocusElement->document() || !m_currentFocusElement->document()->frame())
        return 0;

    if (HTMLTextFormControlElement* controlElement = DOMSupport::toTextControlElement(m_currentFocusElement.get()))
        return start ? controlElement->selectionStart() : controlElement->selectionEnd();

    FrameSelection caretSelection;
    caretSelection.setSelection(m_currentFocusElement->document()->frame()->selection()->selection());
    RefPtr<Range> rangeSelection = caretSelection.selection().toNormalizedRange();
    if (!rangeSelection)
        return 0;

    int selectionPointInNode = start ? rangeSelection->startOffset() : rangeSelection->endOffset();
    Node* containerNode = start ? rangeSelection->startContainer() : rangeSelection->endContainer();

    ExceptionCode ec;
    RefPtr<Range> rangeForNode = rangeOfContents(m_currentFocusElement.get());
    rangeForNode->setEnd(containerNode, selectionPointInNode, ec);
    ASSERT(!ec);

    return TextIterator::rangeLength(rangeForNode.get());
}

void InputHandler::selectionChanged()
{
    // This method can get called during WebPage shutdown process.
    // If that is the case, just bail out since the client is not
    // in a safe state of trust to request anything else from it.
    if (!m_webPage->m_mainFrame)
        return;

    if (!isActiveTextEdit())
        return;

    if (processingChange())
        return;

    // Scroll the field if necessary. This must be done even if we are processing
    // a change as the text change may have moved the caret. IMF doesn't require
    // the update, but the user needs to see the caret.
    ensureFocusTextElementVisible(EdgeIfNeeded);

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    int newSelectionStart = selectionStart();
    int newSelectionEnd = selectionEnd();

    InputLog(LogLevelInfo, "InputHandler::selectionChanged selectionStart=%u, selectionEnd=%u", newSelectionStart, newSelectionEnd);

    m_webPage->m_client->inputSelectionChanged(newSelectionStart, newSelectionEnd);

    // Remove the attributed text markers as the previous call triggered an end to
    // the composition.
    removeAttributedTextMarker();
}

bool InputHandler::setCursorPosition(int location)
{
    return setSelection(location, location);
}

bool InputHandler::setSelection(int start, int end, bool changeIsPartOfComposition)
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    ProcessingChangeGuard guard(this);

    VisibleSelection newSelection = DOMSupport::visibleSelectionForRangeInputElement(m_currentFocusElement.get(), start, end);
    m_currentFocusElement->document()->frame()->selection()->setSelection(newSelection, changeIsPartOfComposition ? 0 : FrameSelection::CloseTyping | FrameSelection::ClearTypingStyle);

    InputLog(LogLevelInfo, "InputHandler::setSelection selectionStart=%u, selectionEnd=%u", start, end);

    return start == selectionStart() && end == selectionEnd();
}

WebCore::IntRect InputHandler::rectForCaret(int index)
{
    if (!isActiveTextEdit())
        return WebCore::IntRect();

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    if (index < 0 || index > static_cast<int>(elementText().length())) {
        // Invalid request.
        return WebCore::IntRect();
    }

    FrameSelection caretSelection;
    caretSelection.setSelection(DOMSupport::visibleSelectionForRangeInputElement(m_currentFocusElement.get(), index, index).visibleStart());
    caretSelection.modify(FrameSelection::AlterationExtend, DirectionForward, CharacterGranularity);
    return caretSelection.selection().visibleStart().absoluteCaretBounds();
}

void InputHandler::cancelSelection()
{
    if (!isActiveTextEdit())
        return;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    int selectionStartPosition = selectionStart();
    ProcessingChangeGuard guard(this);
    setCursorPosition(selectionStartPosition);
}

bool InputHandler::handleKeyboardInput(const Platform::KeyboardEvent& keyboardEvent, bool changeIsPartOfComposition)
{
    InputLog(LogLevelInfo, "InputHandler::handleKeyboardInput received character=%lc, type=%d", keyboardEvent.character(), keyboardEvent.type());

    // Enable input mode if we are processing a key event.
    setInputModeEnabled();

    // If we aren't specifically part of a composition, fail, IMF should never send key input
    // while composing text. If IMF has failed, we should have already finished the
    // composition manually.
    if (!changeIsPartOfComposition && compositionActive())
        return false;

    ProcessingChangeGuard guard(this);

    unsigned adjustedModifiers = keyboardEvent.modifiers();
    if (WTF::isASCIIUpper(keyboardEvent.character()))
        adjustedModifiers |= KEYMOD_SHIFT;

    ASSERT(m_webPage->m_page->focusController());
    bool keyboardEventHandled = false;
    if (Frame* focusedFrame = m_webPage->m_page->focusController()->focusedFrame()) {
        bool isKeyChar = keyboardEvent.type() == Platform::KeyboardEvent::KeyChar;
        Platform::KeyboardEvent::Type type = keyboardEvent.type();

        // If this is a KeyChar type then we handle it as a keydown followed by a key up.
        if (isKeyChar)
            type = Platform::KeyboardEvent::KeyDown;

        Platform::KeyboardEvent adjustedKeyboardEvent(keyboardEvent.character(), type, adjustedModifiers);
        keyboardEventHandled = focusedFrame->eventHandler()->keyEvent(PlatformKeyboardEvent(adjustedKeyboardEvent));

        if (isKeyChar) {
            type = Platform::KeyboardEvent::KeyUp;
            adjustedKeyboardEvent = Platform::KeyboardEvent(keyboardEvent.character(), type, adjustedModifiers);
            keyboardEventHandled = focusedFrame->eventHandler()->keyEvent(PlatformKeyboardEvent(adjustedKeyboardEvent)) || keyboardEventHandled;
        }

        if (!changeIsPartOfComposition && type == Platform::KeyboardEvent::KeyUp)
            ensureFocusTextElementVisible(EdgeIfNeeded);
    }
    return keyboardEventHandled;
}

bool InputHandler::deleteSelection()
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());
    Frame* frame = m_currentFocusElement->document()->frame();

    if (frame->selection()->selectionType() != VisibleSelection::RangeSelection)
        return false;

    ASSERT(frame->editor());
    return frame->editor()->command("DeleteBackward").execute();
}

void InputHandler::insertText(const WTF::String& string)
{
    if (!isActiveTextEdit())
        return;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame() && m_currentFocusElement->document()->frame()->editor());
    Editor* editor = m_currentFocusElement->document()->frame()->editor();

    editor->command("InsertText").execute(string);
}

void InputHandler::clearField()
{
    if (!isActiveTextEdit())
        return;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame() && m_currentFocusElement->document()->frame()->editor());
    Editor* editor = m_currentFocusElement->document()->frame()->editor();

    editor->command("SelectAll").execute();
    editor->command("DeleteBackward").execute();
}

bool InputHandler::executeTextEditCommand(const WTF::String& commandName)
{
    ASSERT(m_webPage->focusedOrMainFrame() && m_webPage->focusedOrMainFrame()->editor());
    Editor* editor = m_webPage->focusedOrMainFrame()->editor();

    return editor->command(commandName).execute();
}

void InputHandler::cut()
{
    executeTextEditCommand("Cut");
}

void InputHandler::copy()
{
    executeTextEditCommand("Copy");
}

void InputHandler::paste()
{
    executeTextEditCommand("Paste");
}

void InputHandler::selectAll()
{
    executeTextEditCommand("SelectAll");
}

void InputHandler::addAttributedTextMarker(int start, int end, const AttributeTextStyle& style)
{
    if ((end - start) < 1 || end > static_cast<int>(elementText().length()))
        return;

    RefPtr<Range> markerRange = DOMSupport::visibleSelectionForRangeInputElement(m_currentFocusElement.get(), start, end).toNormalizedRange();
    m_currentFocusElement->document()->markers()->addMarker(markerRange.get(), DocumentMarker::AttributeText, WTF::String("Input Marker"), style);
}

void InputHandler::removeAttributedTextMarker()
{
    // Remove all attribute text markers.
    if (m_currentFocusElement && m_currentFocusElement->document())
        m_currentFocusElement->document()->markers()->removeMarkers(DocumentMarker::AttributeText);

    m_composingTextStart = 0;
    m_composingTextEnd = 0;
}

void InputHandler::handleInputLocaleChanged(bool isRTL)
{
    if (!isActiveTextEdit())
        return;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());
    RenderObject* renderer = m_currentFocusElement->renderer();
    if (!renderer)
        return;

    Editor* editor = m_currentFocusElement->document()->frame()->editor();
    ASSERT(editor);
    if ((renderer->style()->direction() == RTL) != isRTL)
        editor->setBaseWritingDirection(isRTL ? RightToLeftWritingDirection : LeftToRightWritingDirection);
}

void InputHandler::clearCurrentFocusElement()
{
    if (m_currentFocusElement)
        m_currentFocusElement->blur();
}

bool InputHandler::willOpenPopupForNode(Node* node)
{
    // This method must be kept synchronized with InputHandler::didNodeOpenPopup.
    if (!node)
        return false;

    ASSERT(!node->isInShadowTree());

    if (node->hasTagName(HTMLNames::selectTag) || node->hasTagName(HTMLNames::optionTag)) {
        // We open list popups for options and selects.
        return true;
    }

    if (node->isElementNode()) {
        Element* element = static_cast<Element*>(node);
        if (DOMSupport::isPopupInputField(element))
            return true;
    }

    return false;
}

bool InputHandler::didNodeOpenPopup(Node* node)
{
    // This method must be kept synchronized with InputHandler::willOpenPopupForNode.
    if (!node)
        return false;

    ASSERT(!node->isInShadowTree());

    if (node->hasTagName(HTMLNames::selectTag))
        return openSelectPopup(static_cast<HTMLSelectElement*>(node));

    if (node->hasTagName(HTMLNames::optionTag)) {
        HTMLOptionElement* optionElement = static_cast<HTMLOptionElement*>(node);
        return openSelectPopup(optionElement->ownerSelectElement());
    }

    if (HTMLInputElement* element = node->toInputElement()) {
        if (DOMSupport::isDateTimeInputField(element))
            return openDatePopup(element, elementType(element));

        if (DOMSupport::isColorInputField(element))
            return openColorPopup(element);
    }
    return false;
}

bool InputHandler::openSelectPopup(HTMLSelectElement* select)
{
    if (!select || select->disabled())
        return false;

    // If there's no view, do nothing and return.
    if (!select->document()->view())
        return false;

    if (isActiveTextEdit())
        clearCurrentFocusElement();

    m_currentFocusElement = select;
    m_currentFocusElementType = SelectPopup;

    const WTF::Vector<HTMLElement*>& listItems = select->listItems();
    int size = listItems.size();

    bool multiple = select->multiple();
    ScopeArray<WebString> labels;
    labels.reset(new WebString[size]);

    // Check if popup already exists, close it if does.
    m_webPage->m_page->chrome()->client()->closePagePopup(0);

    bool* enableds = 0;
    int* itemTypes = 0;
    bool* selecteds = 0;

    if (size) {
        enableds = new bool[size];
        itemTypes = new int[size];
        selecteds = new bool[size];
        for (int i = 0; i < size; i++) {
            if (listItems[i]->hasTagName(HTMLNames::optionTag)) {
                HTMLOptionElement* option = static_cast<HTMLOptionElement*>(listItems[i]);
                labels[i] = option->textIndentedToRespectGroupLabel();
                enableds[i] = option->disabled() ? 0 : 1;
                selecteds[i] = option->selected();
                itemTypes[i] = option->parentNode() && option->parentNode()->hasTagName(HTMLNames::optgroupTag) ? TypeOptionInGroup : TypeOption;
            } else if (listItems[i]->hasTagName(HTMLNames::optgroupTag)) {
                HTMLOptGroupElement* optGroup = static_cast<HTMLOptGroupElement*>(listItems[i]);
                labels[i] = optGroup->groupLabelText();
                enableds[i] = optGroup->disabled() ? 0 : 1;
                selecteds[i] = false;
                itemTypes[i] = TypeGroup;
            } else if (listItems[i]->hasTagName(HTMLNames::hrTag)) {
                enableds[i] = false;
                selecteds[i] = false;
                itemTypes[i] = TypeSeparator;
            }
        }
    }

    SelectPopupClient* selectClient = new SelectPopupClient(multiple, size, labels, enableds, itemTypes, selecteds, m_webPage, select);
    WebCore::IntRect elementRectInRootView = select->document()->view()->contentsToRootView(enclosingIntRect(select->getRect()));
    // Fail to create HTML popup, use the old path
    if (!m_webPage->m_page->chrome()->client()->openPagePopup(selectClient, elementRectInRootView))
        m_webPage->m_client->openPopupList(multiple, size, labels, enableds, itemTypes, selecteds);
    delete[] enableds;
    delete[] itemTypes;
    delete[] selecteds;
    return true;
}

void InputHandler::setPopupListIndex(int index)
{
    if (index == -2) // Abandon
        return clearCurrentFocusElement();

    if (!isActiveSelectPopup())
        return clearCurrentFocusElement();

    RenderObject* renderer = m_currentFocusElement->renderer();
    if (renderer && renderer->isMenuList()) {
        RenderMenuList* renderMenu = toRenderMenuList(renderer);
        renderMenu->hidePopup();
    }

    HTMLSelectElement* selectElement = static_cast<HTMLSelectElement*>(m_currentFocusElement.get());
    int optionIndex = selectElement->listToOptionIndex(index);
    selectElement->optionSelectedByUser(optionIndex, true /* deselect = true */, true /* fireOnChangeNow = false */);
    clearCurrentFocusElement();
}

void InputHandler::setPopupListIndexes(int size, const bool* selecteds)
{
    if (!isActiveSelectPopup())
        return clearCurrentFocusElement();

    if (size < 0)
        return;

    HTMLSelectElement* selectElement = static_cast<HTMLSelectElement*>(m_currentFocusElement.get());
    const WTF::Vector<HTMLElement*>& items = selectElement->listItems();
    if (items.size() != static_cast<unsigned int>(size))
        return;

    HTMLOptionElement* option;
    for (int i = 0; i < size; i++) {
        if (items[i]->hasTagName(HTMLNames::optionTag)) {
            option = static_cast<HTMLOptionElement*>(items[i]);
            option->setSelectedState(selecteds[i]);
        }
    }

    // Force repaint because we do not send mouse events to the select element
    // and the element doesn't automatically repaint itself.
    selectElement->dispatchFormControlChangeEvent();
    selectElement->renderer()->repaint();
    clearCurrentFocusElement();
}

bool InputHandler::setBatchEditingActive(bool active)
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document());
    ASSERT(m_currentFocusElement->document()->frame());

    // FIXME switch this to m_currentFocusElement->document()->frame() when we have separate
    // backingstore for each frame.
    BackingStoreClient* backingStoreClientForFrame = m_webPage->backingStoreClientForFrame(m_webPage->mainFrame());
    ASSERT(backingStoreClientForFrame);

    // Enable / Disable the backingstore to prevent visual updates.
    if (!active)
        backingStoreClientForFrame->backingStore()->resumeScreenAndBackingStoreUpdates(BackingStore::RenderAndBlit);
    else
        backingStoreClientForFrame->backingStore()->suspendScreenAndBackingStoreUpdates();

    return true;
}

int InputHandler::caretPosition() const
{
    if (!isActiveTextEdit())
        return -1;

    // NOTE: This function is expected to return the start of the selection if
    // a selection is applied.
    return selectionStart();
}

int relativeLeftOffset(int caretPosition, int leftOffset)
{
    ASSERT(caretPosition >= 0);

    return std::max(0, caretPosition - leftOffset);
}

int relativeRightOffset(int caretPosition, unsigned totalLengthOfText, int rightOffset)
{
    ASSERT(caretPosition >= 0);
    ASSERT(totalLengthOfText < INT_MAX);

    return std::min(caretPosition + rightOffset, static_cast<int>(totalLengthOfText));
}

bool InputHandler::deleteTextRelativeToCursor(int leftOffset, int rightOffset)
{
    if (!isActiveTextEdit() || compositionActive())
        return false;

    ProcessingChangeGuard guard(this);

    InputLog(LogLevelInfo, "InputHandler::deleteTextRelativeToCursor left %d right %d", leftOffset, rightOffset);

    int caretOffset = caretPosition();
    int start = relativeLeftOffset(caretOffset, leftOffset);
    int end = relativeRightOffset(caretOffset, elementText().length(), rightOffset);
    if (!deleteText(start, end))
        return false;

    // Scroll the field if necessary. The automatic update is suppressed
    // by the processing change guard.
    ensureFocusTextElementVisible(EdgeIfNeeded);

    return true;
}

bool InputHandler::deleteText(int start, int end)
{
    if (!isActiveTextEdit())
        return false;

    ProcessingChangeGuard guard(this);

    if (!setSelection(start, end, true /*changeIsPartOfComposition*/))
        return false;

    InputLog(LogLevelInfo, "InputHandler::deleteText start %d end %d", start, end);

    return deleteSelection();
}

spannable_string_t* InputHandler::spannableTextInRange(int start, int end, int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    if (start == end)
        return 0;

    ASSERT(end > start);
    int length = end - start;

    WTF::String textString = elementText().substring(start, length);

    spannable_string_t* pst = (spannable_string_t*)fastMalloc(sizeof(spannable_string_t));

    // Don't use fastMalloc in case the string is unreasonably long. fastMalloc will
    // crash immediately on failure.
    pst->str = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));
    if (!pst->str) {
        logAlways(LogLevelCritical, "InputHandler::spannableTextInRange Cannot allocate memory for string.");
        free(pst);
        return 0;
    }

    int stringLength = 0;
    if (!convertStringToWchar(textString, pst->str, length + 1, &stringLength)) {
        logAlways(LogLevelCritical, "InputHandler::spannableTextInRange failed to convert string.");
        free(pst->str);
        free(pst);
        return 0;
    }

    pst->length = stringLength;
    pst->spans_count = 0;
    pst->spans = 0;

    return pst;
}

spannable_string_t* InputHandler::selectedText(int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    return spannableTextInRange(selectionStart(), selectionEnd(), flags);
}

spannable_string_t* InputHandler::textBeforeCursor(int32_t length, int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    int caretOffset = caretPosition();
    int start = relativeLeftOffset(caretOffset, length);
    int end = caretOffset;

    return spannableTextInRange(start, end, flags);
}

spannable_string_t* InputHandler::textAfterCursor(int32_t length, int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    int caretOffset = caretPosition();
    int start = caretOffset;
    int end = relativeRightOffset(caretOffset, elementText().length(), length);

    return spannableTextInRange(start, end, flags);
}

extracted_text_t* InputHandler::extractedTextRequest(extracted_text_request_t* request, int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    extracted_text_t* extractedText = (extracted_text_t *)fastMalloc(sizeof(extracted_text_t));

    // 'flags' indicates whether the text is being monitored. This is not currently used.

    // FIXME add smart limiting based on the hint sizes. Currently return all text.

    extractedText->text = spannableTextInRange(0, elementText().length(), flags);

    // FIXME confirm these should be 0 on this requested. Text changes will likely require
    // the end be the length.
    extractedText->partial_start_offset = 0;
    extractedText->partial_end_offset = 0;
    extractedText->start_offset = 0;

    // Adjust selection values relative to the start offset, which may be a subset
    // of the text in the field.
    extractedText->selection_start = selectionStart() - extractedText->start_offset;
    extractedText->selection_end = selectionEnd() - extractedText->start_offset;

    // selectionActive is not limited to inside the extracted text.
    bool selectionActive = extractedText->selection_start != extractedText->selection_end;
    bool singleLine = m_currentFocusElement->hasTagName(HTMLNames::inputTag);

    // FIXME flags has two values in doc, enum not in header yet.
    extractedText->flags = selectionActive & singleLine;

    return extractedText;
}

static void addCompositionTextStyleToAttributeTextStyle(AttributeTextStyle& style)
{
    style.setUnderline(AttributeTextStyle::StandardUnderline);
}

static void addActiveTextStyleToAttributeTextStyle(AttributeTextStyle& style)
{
    style.setBackgroundColor(Color("blue"));
    style.setTextColor(Color("white"));
}

static AttributeTextStyle compositionTextStyle()
{
    AttributeTextStyle style;
    addCompositionTextStyleToAttributeTextStyle(style);
    return style;
}

static AttributeTextStyle textStyleFromMask(int64_t mask)
{
    AttributeTextStyle style;
    if (mask & COMPOSED_TEXT_ATTRIB)
        addCompositionTextStyleToAttributeTextStyle(style);

    if (mask & ACTIVE_REGION_ATTRIB)
        addActiveTextStyleToAttributeTextStyle(style);

    return style;
}

bool InputHandler::removeComposedText()
{
    if (compositionActive()) {
        if (!deleteText(m_composingTextStart, m_composingTextEnd)) {
            // Could not remove the existing composition region.
            return false;
        }
        removeAttributedTextMarker();
    }

    return true;
}

int32_t InputHandler::setComposingRegion(int32_t start, int32_t end)
{
    if (!isActiveTextEdit())
        return -1;

    if (!removeComposedText()) {
        // Could not remove the existing composition region.
        return -1;
    }

    m_composingTextStart = start;
    m_composingTextEnd = end;

    if (compositionActive())
        addAttributedTextMarker(start, end, compositionTextStyle());

    InputLog(LogLevelInfo, "InputHandler::setComposingRegion start %d end %d", start, end);

    return 0;
}

int32_t InputHandler::finishComposition()
{
    if (!isActiveTextEdit())
        return -1;

    // FIXME if no composition is active, should we return failure -1?
    if (!compositionActive())
        return 0;

    // Remove all markers.
    removeAttributedTextMarker();

    InputLog(LogLevelInfo, "InputHandler::finishComposition completed");

    return 0;
}

span_t* InputHandler::firstSpanInString(spannable_string_t* spannableString, SpannableStringAttribute attrib)
{
    span_t* span = spannableString->spans;
    for (unsigned int i = 0; i < spannableString->spans_count; i++) {
        if (span->attributes_mask & attrib)
            return span;
        span++;
    }

    return 0;
}

bool InputHandler::isTrailingSingleCharacter(span_t* span, unsigned stringLength, unsigned composingTextLength)
{
    // Make sure the new string is one character larger than the old.
    if (composingTextLength != stringLength - 1)
        return false;

    if (!span)
        return false;

    // Has only 1 character changed?
    if (span->start == span->end) {
        // Is this character the last character in the string?
        if (span->start == stringLength - 1)
            return true;
    }
    // Return after the first changed_attrib is found.
    return false;
}

bool InputHandler::setText(spannable_string_t* spannableString)
{
    if (!isActiveTextEdit() || !spannableString)
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());
    Frame* frame = m_currentFocusElement->document()->frame();

    Editor* editor = frame->editor();
    ASSERT(editor);

    // Disable selectionHandler's active selection as we will be resetting and these
    // changes should not be handled as notification event.
    m_webPage->m_selectionHandler->setSelectionActive(false);

    String textToInsert = convertSpannableStringToString(spannableString);
    int textLength = textToInsert.length();

    InputLog(LogLevelInfo, "InputHandler::setText spannableString is '%s', of length %d", textToInsert.latin1().data(), textLength);

    span_t* changedSpan = firstSpanInString(spannableString, CHANGED_ATTRIB);
    int composingTextStart = m_composingTextStart;
    int composingTextEnd = m_composingTextEnd;
    int composingTextLength = compositionLength();
    removeAttributedTextMarker();

    if (isTrailingSingleCharacter(changedSpan, textLength, composingTextLength)) {
        // If the text is unconverted, do not allow JS processing as it is not a "real"
        // character in the field.
        if (firstSpanInString(spannableString, UNCONVERTED_TEXT_ATTRIB)) {
            InputLog(LogLevelInfo, "InputHandler::setText Single trailing character detected.  Text is unconverted.");
            return editor->command("InsertText").execute(textToInsert.right(1));
        }
        InputLog(LogLevelInfo, "InputHandler::setText Single trailing character detected.");
        return handleKeyboardInput(Platform::KeyboardEvent(textToInsert[textLength - 1], Platform::KeyboardEvent::KeyChar, 0), false /* changeIsPartOfComposition */);
    }

    // If no spans have changed, treat it as a delete operation.
    if (!changedSpan) {
        // If the composition length is the same as our string length, then we don't need to do anything.
        if (composingTextLength == textLength) {
            InputLog(LogLevelInfo, "InputHandler::setText No spans have changed. New text is the same length as the old. Nothing to do.");
            return true;
        }

        if (composingTextLength - textLength == 1) {
            InputLog(LogLevelInfo, "InputHandler::setText No spans have changed. New text is one character shorter than the old. Treating as 'delete'.");
            return editor->command("DeleteBackward").execute();
        }
    }

    if (composingTextLength && !setSelection(composingTextStart, composingTextEnd, true /* changeIsPartOfComposition */))
        return false;

    // If there is no text to add just delete.
    if (!textLength) {
        if (selectionActive())
            return editor->command("DeleteBackward").execute();

        // Nothing to do.
        return true;
    }

    // Triggering an insert of the text with a space character trailing
    // causes new text to adopt the previous text style.
    // Remove it and apply it as a keypress later.
    // Upstream Webkit bug created https://bugs.webkit.org/show_bug.cgi?id=70823
    bool requiresSpaceKeyPress = false;
    if (textLength > 0 && textToInsert[textLength - 1] == 32 /* space */) {
        requiresSpaceKeyPress = true;
        textLength--;
        textToInsert.remove(textLength, 1);
    }

    InputLog(LogLevelInfo, "InputHandler::setText Request being processed. Text before processing: '%s'", elementText().latin1().data());

    if (textLength == 1 && !spannableString->spans_count) {
        // Handle single key non-attributed entry as key press rather than insert to allow
        // triggering of javascript events.
        InputLog(LogLevelInfo, "InputHandler::setText Single character entry treated as key-press in the absense of spans.");
        return handleKeyboardInput(Platform::KeyboardEvent(textToInsert[0], Platform::KeyboardEvent::KeyChar, 0), true /* changeIsPartOfComposition */);
    }

    // Perform the text change as a single command if there is one.
    if (!textToInsert.isEmpty() && !editor->command("InsertText").execute(textToInsert)) {
        InputLog(LogLevelWarn, "InputHandler::setText Failed to insert text '%s'", textToInsert.latin1().data());
        return false;
    }

    if (requiresSpaceKeyPress)
        handleKeyboardInput(Platform::KeyboardEvent(32 /* space */, Platform::KeyboardEvent::KeyChar, 0), true /* changeIsPartOfComposition */);

    InputLog(LogLevelInfo, "InputHandler::setText Request being processed. Text after processing '%s'", elementText().latin1().data());

    return true;
}

bool InputHandler::setTextAttributes(int insertionPoint, spannable_string_t* spannableString)
{
    // Apply the attributes to the field.
    span_t* span = spannableString->spans;
    for (unsigned int i = 0; i < spannableString->spans_count; i++) {
        unsigned int startPosition = insertionPoint + span->start;
        // The end point includes the character that it is before. Ie, 0, 0
        // applies to the first character as the end point includes the character
        // at the position. This means the endPosition is always +1.
        unsigned int endPosition = insertionPoint + span->end + 1;
        if (endPosition < startPosition || endPosition > elementText().length())
            return false;

        if (!span->attributes_mask)
            continue; // Nothing to do.

        // MISSPELLED_WORD_ATTRIB is present as an option, but it is not currently
        // used by IMF. When they add support for on the fly spell checking we can
        // use it to apply spelling markers and disable continuous spell checking.

        InputLog(LogLevelInfo, "InputHandler::setTextAttributes adding marker %d to %d - %llu", startPosition, endPosition, span->attributes_mask);
        addAttributedTextMarker(startPosition, endPosition, textStyleFromMask(span->attributes_mask));

        span++;
    }

    InputLog(LogLevelInfo, "InputHandler::setTextAttributes attribute count %d", spannableString->spans_count);

    return true;
}

bool InputHandler::setRelativeCursorPosition(int insertionPoint, int relativeCursorPosition)
{
    if (!isActiveTextEdit())
        return false;

    // 1 place cursor at end of insertion text.
    if (relativeCursorPosition == 1) {
        m_currentFocusElement->document()->frame()->selection()->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);
        return true;
    }

    int cursorPosition = 0;
    if (relativeCursorPosition <= 0) {
        // Zero = insertionPoint
        // Negative value, move the cursor the requested number of characters before
        // the start of the inserted text.
        cursorPosition = insertionPoint + relativeCursorPosition;
    } else {
        // Positive value, move the cursor the requested number of characters after
        // the end of the inserted text minus 1.
        cursorPosition = caretPosition() + relativeCursorPosition - 1;
    }

    if (cursorPosition < 0 || cursorPosition > (int)elementText().length())
        return false;

    InputLog(LogLevelInfo, "InputHandler::setRelativeCursorPosition cursor position %d", cursorPosition);

    return setCursorPosition(cursorPosition);
}

bool InputHandler::setSpannableTextAndRelativeCursor(spannable_string_t* spannableString, int relativeCursorPosition, bool markTextAsComposing)
{
    InputLog(LogLevelInfo, "InputHandler::setSpannableTextAndRelativeCursor(%d, %d, %d)", spannableString->length, relativeCursorPosition, markTextAsComposing);
    int insertionPoint = compositionActive() ? m_composingTextStart : selectionStart();

    ProcessingChangeGuard guard(this);

    if (!setText(spannableString))
        return false;

    if (!setTextAttributes(insertionPoint, spannableString))
        return false;

    if (!setRelativeCursorPosition(insertionPoint, relativeCursorPosition))
        return false;

    if (markTextAsComposing) {
        m_composingTextStart = insertionPoint;
        m_composingTextEnd = insertionPoint + spannableString->length;
    }

    // Scroll the field if necessary. The automatic update is suppressed
    // by the processing change guard.
    ensureFocusTextElementVisible(EdgeIfNeeded);

    return true;
}

int32_t InputHandler::setComposingText(spannable_string_t* spannableString, int32_t relativeCursorPosition)
{
    if (!isActiveTextEdit())
        return -1;

    if (!spannableString)
        return -1;

    InputLog(LogLevelInfo, "InputHandler::setComposingText at relativeCursorPosition: %d", relativeCursorPosition);

    // Enable input mode if we are processing a key event.
    setInputModeEnabled();

    return setSpannableTextAndRelativeCursor(spannableString, relativeCursorPosition, true /* markTextAsComposing */) ? 0 : -1;
}

int32_t InputHandler::commitText(spannable_string_t* spannableString, int32_t relativeCursorPosition)
{
    if (!isActiveTextEdit())
        return -1;

    if (!spannableString)
        return -1;

    InputLog(LogLevelInfo, "InputHandler::commitText");

    return setSpannableTextAndRelativeCursor(spannableString, relativeCursorPosition, false /* markTextAsComposing */) ? 0 : -1;
}

}
}
