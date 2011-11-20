/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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

#import "CorrectionPanel.h"
#import <WebCore/EditorClient.h>
#import <WebCore/TextCheckerClient.h>
#import <wtf/Forward.h>
#import <wtf/RetainPtr.h>
#import <wtf/Vector.h>

@class WebView;
@class WebEditorUndoTarget;

class WebEditorClient : public WebCore::EditorClient, public WebCore::TextCheckerClient {
public:
    WebEditorClient(WebView *);
    virtual ~WebEditorClient();
    virtual void pageDestroyed() OVERRIDE;

    virtual bool isGrammarCheckingEnabled() OVERRIDE;
    virtual void toggleGrammarChecking() OVERRIDE;
    virtual bool isContinuousSpellCheckingEnabled() OVERRIDE;
    virtual void toggleContinuousSpellChecking() OVERRIDE;
    virtual int spellCheckerDocumentTag() OVERRIDE;

    virtual bool smartInsertDeleteEnabled() OVERRIDE;
    virtual bool isSelectTrailingWhitespaceEnabled() OVERRIDE;

    virtual bool shouldDeleteRange(WebCore::Range*) OVERRIDE;
    virtual bool shouldShowDeleteInterface(WebCore::HTMLElement*) OVERRIDE;

    virtual bool shouldBeginEditing(WebCore::Range*) OVERRIDE;
    virtual bool shouldEndEditing(WebCore::Range*) OVERRIDE;
    virtual bool shouldInsertNode(WebCore::Node*, WebCore::Range*, WebCore::EditorInsertAction) OVERRIDE;
    virtual bool shouldInsertText(const WTF::String&, WebCore::Range*, WebCore::EditorInsertAction) OVERRIDE;
    virtual bool shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity, bool stillSelecting) OVERRIDE;

    virtual bool shouldApplyStyle(WebCore::CSSStyleDeclaration*, WebCore::Range*) OVERRIDE;
    
    virtual bool shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range* rangeToBeReplaced) OVERRIDE;

    virtual void didBeginEditing() OVERRIDE;
    virtual void didEndEditing() OVERRIDE;
    virtual void didWriteSelectionToPasteboard() OVERRIDE;
    virtual void didSetSelectionTypesForPasteboard() OVERRIDE;

    virtual NSString* userVisibleString(NSURL *) OVERRIDE;
    virtual WebCore::DocumentFragment* documentFragmentFromAttributedString(NSAttributedString *, Vector< RefPtr<WebCore::ArchiveResource> >&) OVERRIDE;
    virtual void setInsertionPasteboard(NSPasteboard *) OVERRIDE;
    virtual NSURL* canonicalizeURL(NSURL*) OVERRIDE;
    virtual NSURL* canonicalizeURLString(NSString*) OVERRIDE;
    
#ifndef BUILDING_ON_LEOPARD
    virtual void uppercaseWord() OVERRIDE;
    virtual void lowercaseWord() OVERRIDE;
    virtual void capitalizeWord() OVERRIDE;
    virtual void showSubstitutionsPanel(bool show) OVERRIDE;
    virtual bool substitutionsPanelIsShowing() OVERRIDE;
    virtual void toggleSmartInsertDelete() OVERRIDE;
    virtual bool isAutomaticQuoteSubstitutionEnabled() OVERRIDE;
    virtual void toggleAutomaticQuoteSubstitution() OVERRIDE;
    virtual bool isAutomaticLinkDetectionEnabled() OVERRIDE;
    virtual void toggleAutomaticLinkDetection() OVERRIDE;
    virtual bool isAutomaticDashSubstitutionEnabled() OVERRIDE;
    virtual void toggleAutomaticDashSubstitution() OVERRIDE;
    virtual bool isAutomaticTextReplacementEnabled() OVERRIDE;
    virtual void toggleAutomaticTextReplacement() OVERRIDE;
    virtual bool isAutomaticSpellingCorrectionEnabled() OVERRIDE;
    virtual void toggleAutomaticSpellingCorrection() OVERRIDE;
#endif

    TextCheckerClient* textChecker() OVERRIDE { return this; }

    virtual void respondToChangedContents() OVERRIDE;
    virtual void respondToChangedSelection(WebCore::Frame*) OVERRIDE;

    virtual void registerCommandForUndo(PassRefPtr<WebCore::EditCommand>) OVERRIDE;
    virtual void registerCommandForRedo(PassRefPtr<WebCore::EditCommand>) OVERRIDE;
    virtual void clearUndoRedoOperations() OVERRIDE;

    virtual bool canCopyCut(WebCore::Frame*, bool defaultValue) const OVERRIDE;
    virtual bool canPaste(WebCore::Frame*, bool defaultValue) const OVERRIDE;
    virtual bool canUndo() const OVERRIDE;
    virtual bool canRedo() const OVERRIDE;
    
    virtual void undo() OVERRIDE;
    virtual void redo() OVERRIDE;
    
    virtual void handleKeyboardEvent(WebCore::KeyboardEvent*) OVERRIDE;
    virtual void handleInputMethodKeydown(WebCore::KeyboardEvent*) OVERRIDE;

    virtual void textFieldDidBeginEditing(WebCore::Element*) OVERRIDE;
    virtual void textFieldDidEndEditing(WebCore::Element*) OVERRIDE;
    virtual void textDidChangeInTextField(WebCore::Element*) OVERRIDE;
    virtual bool doTextFieldCommandFromEvent(WebCore::Element*, WebCore::KeyboardEvent*) OVERRIDE;
    virtual void textWillBeDeletedInTextField(WebCore::Element*) OVERRIDE;
    virtual void textDidChangeInTextArea(WebCore::Element*) OVERRIDE;
    
    virtual void ignoreWordInSpellDocument(const WTF::String&) OVERRIDE;
    virtual void learnWord(const WTF::String&) OVERRIDE;
    virtual void checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength) OVERRIDE;
    virtual WTF::String getAutoCorrectSuggestionForMisspelledWord(const WTF::String&) OVERRIDE;
    virtual void checkGrammarOfString(const UChar*, int length, WTF::Vector<WebCore::GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength) OVERRIDE;
    virtual void checkTextOfParagraph(const UChar* text, int length, WebCore::TextCheckingTypeMask checkingTypes, WTF::Vector<WebCore::TextCheckingResult>& results) OVERRIDE;
    virtual void updateSpellingUIWithGrammarString(const WTF::String&, const WebCore::GrammarDetail&) OVERRIDE;
    virtual void updateSpellingUIWithMisspelledWord(const WTF::String&) OVERRIDE;
    virtual void showSpellingUI(bool show) OVERRIDE;
    virtual bool spellingUIIsShowing() OVERRIDE;
    virtual void getGuessesForWord(const WTF::String& word, const WTF::String& context, WTF::Vector<WTF::String>& guesses) OVERRIDE;
    virtual void willSetInputMethodState() OVERRIDE;
    virtual void setInputMethodState(bool enabled) OVERRIDE;
    virtual void requestCheckingOfString(WebCore::SpellChecker*, int, WebCore::TextCheckingTypeMask, const WTF::String&) OVERRIDE;
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    virtual void showCorrectionPanel(WebCore::CorrectionPanelInfo::PanelType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings) OVERRIDE;
    virtual void dismissCorrectionPanel(WebCore::ReasonForDismissingCorrectionPanel) OVERRIDE;
    virtual String dismissCorrectionPanelSoon(WebCore::ReasonForDismissingCorrectionPanel) OVERRIDE;
    virtual void recordAutocorrectionResponse(AutocorrectionResponseType, const String& replacedString, const String& replacementString) OVERRIDE;
#endif
private:
    void registerCommandForUndoOrRedo(PassRefPtr<WebCore::EditCommand>, bool isRedo);
    WebEditorClient();

    WebView *m_webView;
    RetainPtr<WebEditorUndoTarget> m_undoTarget;
    bool m_haveUndoRedoOperations;

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    CorrectionPanel m_correctionPanel;
#endif
};
