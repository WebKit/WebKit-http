/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
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

#ifndef EditorClientQt_h
#define EditorClientQt_h

#include "EditorClient.h"
#include "TextCheckerClientQt.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

class QWebPage;
class QWebPageAdapter;

namespace WebCore {

class EditorClientQt : public EditorClient {
public:
    EditorClientQt(QWebPageAdapter*);
    
    void pageDestroyed() override;
    
    bool shouldDeleteRange(Range*) override;
    bool smartInsertDeleteEnabled() override;
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    void toggleSmartInsertDelete() override;
#endif
    bool isSelectTrailingWhitespaceEnabled() override;
    bool isContinuousSpellCheckingEnabled() override;
    void toggleContinuousSpellChecking() override;
    bool isGrammarCheckingEnabled() override;
    void toggleGrammarChecking() override;
    int spellCheckerDocumentTag() override;

    bool shouldBeginEditing(Range*) override;
    bool shouldEndEditing(Range*) override;
    bool shouldInsertNode(Node*, Range*, EditorInsertAction) override;
    bool shouldInsertText(const String&, Range*, EditorInsertAction) override;
    bool shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity, bool stillSelecting) override;

    bool shouldApplyStyle(StyleProperties*, Range*) override;

    bool shouldMoveRangeAfterDelete(Range*, Range*) override;

    void didBeginEditing() override;
    void respondToChangedContents() override;
    void respondToChangedSelection(Frame*) override;
    void didEndEditing() override;
    void willWriteSelectionToPasteboard(Range*) override;
    void didWriteSelectionToPasteboard() override;
    void getClientPasteboardDataForRange(Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData) override;
    
    void registerUndoStep(PassRefPtr<UndoStep>) override;
    void registerRedoStep(PassRefPtr<UndoStep>) override;
    void clearUndoRedoOperations() override;

    bool canCopyCut(Frame*, bool defaultValue) const override;
    bool canPaste(Frame*, bool defaultValue) const override;
    bool canUndo() const override;
    bool canRedo() const override;
    
    void undo() override;
    void redo() override;

    void handleKeyboardEvent(KeyboardEvent*) override;
    void handleInputMethodKeydown(KeyboardEvent*) override;

    void textFieldDidBeginEditing(Element*) override;
    void textFieldDidEndEditing(Element*) override;
    void textDidChangeInTextField(Element*) override;
    bool doTextFieldCommandFromEvent(Element*, KeyboardEvent*) override;
    void textWillBeDeletedInTextField(Element*) override;
    void textDidChangeInTextArea(Element*) override;

    void updateSpellingUIWithGrammarString(const String&, const GrammarDetail&) override;
    void updateSpellingUIWithMisspelledWord(const String&) override;
    void showSpellingUI(bool show) override;
    bool spellingUIIsShowing() override;
    void willSetInputMethodState() override;
    void setInputMethodState(bool enabled) override;
    TextCheckerClient* textChecker() override { return &m_textCheckerClient; }

    bool supportsGlobalSelection() override;

    void didApplyStyle() override;
    void didChangeSelectionAndUpdateLayout() override;
    void discardedComposition(Frame *) override;
    void overflowScrollPositionChanged() override;

    bool isEditing() const;

    static bool dumpEditingCallbacks;
    static bool acceptsEditing;

private:
    TextCheckerClientQt m_textCheckerClient;
    QWebPageAdapter* m_page;
    bool m_editing;
    bool m_inUndoRedo; // our undo stack works differently - don't re-enter!
};

}

#endif

// vim: ts=4 sw=4 et
