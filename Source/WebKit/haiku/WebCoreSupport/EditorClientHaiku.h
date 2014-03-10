/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EditorClientHaiku_h
#define EditorClientHaiku_h

#include "EditorClient.h"
#include "Page.h"
#include "TextCheckerClient.h"
#include <wtf/RefCounted.h>
#include <wtf/Deque.h>
#include <wtf/Forward.h>
#include "wtf/text/StringView.h"
#include <String.h>

class BMessage;
class BWebPage;

namespace WebCore {
class PlatformKeyboardEvent;

class EditorClientHaiku : public EditorClient, public TextCheckerClient {
public:
    EditorClientHaiku(BWebPage* page);
    virtual void pageDestroyed() override;

    virtual bool shouldDeleteRange(Range*) override;
    virtual bool smartInsertDeleteEnabled() override;
    virtual bool isSelectTrailingWhitespaceEnabled() override;
    virtual bool isContinuousSpellCheckingEnabled() override;
    virtual void toggleContinuousSpellChecking() override;
    virtual bool isGrammarCheckingEnabled() override;
    virtual void toggleGrammarChecking() override;
    virtual int spellCheckerDocumentTag() override;

    virtual bool shouldBeginEditing(Range*) override;
    virtual bool shouldEndEditing(Range*) override;
    virtual bool shouldInsertNode(Node*, Range*, EditorInsertAction) override;
    virtual bool shouldInsertText(const String&, Range*, EditorInsertAction) override;
    virtual bool shouldChangeSelectedRange(Range* fromRange, Range* toRange,
                                           EAffinity, bool stillSelecting) override;

    virtual bool shouldApplyStyle(StyleProperties*, Range*) override;
    virtual bool shouldMoveRangeAfterDelete(Range*, Range*) override;

    virtual void didBeginEditing() override;
    virtual void respondToChangedContents() override;
    virtual void respondToChangedSelection(Frame*) override;
    virtual void didEndEditing() override;
    virtual void willWriteSelectionToPasteboard(WebCore::Range*) override;
    virtual void didWriteSelectionToPasteboard() override;
    virtual void getClientPasteboardDataForRange(WebCore::Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData) override;

    virtual void registerUndoStep(PassRefPtr<UndoStep>) override;
    virtual void registerRedoStep(PassRefPtr<UndoStep>) override;
    virtual void clearUndoRedoOperations() override;

    virtual bool canCopyCut(Frame*, bool defaultValue) const override;
    virtual bool canPaste(Frame*, bool defaultValue) const override;
    virtual bool canUndo() const override;
    virtual bool canRedo() const override;

    virtual void undo() override;
    virtual void redo() override;

    virtual void handleKeyboardEvent(KeyboardEvent*) override;
    virtual void handleInputMethodKeydown(KeyboardEvent*) override;

    virtual void textFieldDidBeginEditing(Element*) override;
    virtual void textFieldDidEndEditing(Element*) override;
    virtual void textDidChangeInTextField(Element*) override;
    virtual bool doTextFieldCommandFromEvent(Element*, KeyboardEvent*) override;
    virtual void textWillBeDeletedInTextField(Element*) override;
    virtual void textDidChangeInTextArea(Element*) override;

    virtual TextCheckerClient* textChecker() override { return this; }

    virtual void updateSpellingUIWithGrammarString(const String&, const GrammarDetail&) override;
    virtual void updateSpellingUIWithMisspelledWord(const String&) override;
    virtual void showSpellingUI(bool show) override;
    virtual bool spellingUIIsShowing() override;
    virtual void willSetInputMethodState() override;
    virtual void setInputMethodState(bool enabled) override;

    // TextCheckerClient

    virtual bool shouldEraseMarkersAfterChangeSelection(TextCheckingType) const override;
    virtual void ignoreWordInSpellDocument(const String&) override;
    virtual void learnWord(const String&) override;
    virtual void checkSpellingOfString(StringView, int* misspellingLocation,
                                       int* misspellingLength) override;
    virtual String getAutoCorrectSuggestionForMisspelledWord(const String& misspelledWord) override;
    virtual void checkGrammarOfString(StringView, Vector<GrammarDetail>&,
                                      int* badGrammarLocation, int* badGrammarLength) override;

    virtual void getGuessesForWord(const String& word, const String& context, Vector<String>& guesses) override;
    virtual void requestCheckingOfString(PassRefPtr<TextCheckingRequest>) override;

private:
    bool handleEditingKeyboardEvent(KeyboardEvent* event,
        const PlatformKeyboardEvent* platformEvent);
    void setPendingComposition(const char* newComposition);
    void setPendingPreedit(const char* newPreedit);
    void clearPendingIMData();
    void imContextCommitted(const char* str, EditorClient* client);
    void imContextPreeditChanged(EditorClient* client);

    void dispatchMessage(BMessage& message);

    BWebPage* m_page;

    typedef Deque<RefPtr<WebCore::UndoStep> > UndoManagerStack;
    UndoManagerStack m_undoStack;
    UndoManagerStack m_redoStack;

    bool m_isInRedo;

    BString m_pendingComposition;
    BString m_pendingPreedit;
};

} // namespace WebCore

#endif // EditorClientHaiku_h

