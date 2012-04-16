/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef TypingCommand_h
#define TypingCommand_h

#include "TextInsertionBaseCommand.h"

namespace WebCore {

class TypingCommand : public TextInsertionBaseCommand {
public:
    enum ETypingCommand { 
        DeleteSelection,
        DeleteKey, 
        ForwardDeleteKey, 
        InsertText, 
        InsertLineBreak, 
        InsertParagraphSeparator,
        InsertParagraphSeparatorInQuotedContent
    };

    enum TextCompositionType {
        TextCompositionNone,
        TextCompositionUpdate,
        TextCompositionConfirm
    };

    enum Option {
        SelectInsertedText = 1 << 0,
        KillRing = 1 << 1,
        RetainAutocorrectionIndicator = 1 << 2,
        PreventSpellChecking = 1 << 3,
        SmartDelete = 1 << 4
    };
    typedef unsigned Options;

    static void deleteSelection(Document*, Options = 0);
    static void deleteKeyPressed(Document*, Options = 0, TextGranularity = CharacterGranularity);
    static void forwardDeleteKeyPressed(Document*, Options = 0, TextGranularity = CharacterGranularity);
    static void insertText(Document*, const String&, Options, TextCompositionType = TextCompositionNone);
    static void insertText(Document*, const String&, const VisibleSelection&, Options, TextCompositionType = TextCompositionNone);
    static void insertLineBreak(Document*, Options);
    static void insertParagraphSeparator(Document*, Options);
    static void insertParagraphSeparatorInQuotedContent(Document*);
    static void closeTyping(Frame*);

    void insertText(const String &text, bool selectInsertedText);
    void insertTextRunWithoutNewlines(const String &text, bool selectInsertedText);
    void insertLineBreak();
    void insertParagraphSeparatorInQuotedContent();
    void insertParagraphSeparator();
    void deleteKeyPressed(TextGranularity, bool killRing);
    void forwardDeleteKeyPressed(TextGranularity, bool killRing);
    void deleteSelection(bool smartDelete);
    void setCompositionType(TextCompositionType type) { m_compositionType = type; }

private:
    static PassRefPtr<TypingCommand> create(Document* document, ETypingCommand command, const String& text = "", Options options = 0, TextGranularity granularity = CharacterGranularity)
    {
        return adoptRef(new TypingCommand(document, command, text, options, granularity, TextCompositionNone));
    }

    static PassRefPtr<TypingCommand> create(Document* document, ETypingCommand command, const String& text, Options options, TextCompositionType compositionType)
    {
        return adoptRef(new TypingCommand(document, command, text, options, CharacterGranularity, compositionType));
    }

    TypingCommand(Document*, ETypingCommand, const String& text, Options, TextGranularity, TextCompositionType);

    bool smartDelete() const { return m_smartDelete; }
    void setSmartDelete(bool smartDelete) { m_smartDelete = smartDelete; }
    bool isOpenForMoreTyping() const { return m_openForMoreTyping; }
    void closeTyping() { m_openForMoreTyping = false; }

    static PassRefPtr<TypingCommand> lastTypingCommandIfStillOpenForTyping(Frame*);

    virtual void doApply();
    virtual EditAction editingAction() const;
    virtual bool isTypingCommand() const;
    virtual bool preservesTypingStyle() const { return m_preservesTypingStyle; }
    virtual bool shouldRetainAutocorrectionIndicator() const
    {
        ASSERT(isTopLevelCommand());
        return m_shouldRetainAutocorrectionIndicator;
    }
    virtual void setShouldRetainAutocorrectionIndicator(bool retain) { m_shouldRetainAutocorrectionIndicator = retain; }
    virtual bool shouldStopCaretBlinking() const { return true; }
    void setShouldPreventSpellChecking(bool prevent) { m_shouldPreventSpellChecking = prevent; }

    static void updateSelectionIfDifferentFromCurrentSelection(TypingCommand*, Frame*);

    void updatePreservesTypingStyle(ETypingCommand);
    void markMisspellingsAfterTyping(ETypingCommand);
    void typingAddedToOpenCommand(ETypingCommand);
    bool makeEditableRootEmpty();

    ETypingCommand m_commandType;
    String m_textToInsert;
    bool m_openForMoreTyping;
    bool m_selectInsertedText;
    bool m_smartDelete;
    TextGranularity m_granularity;
    TextCompositionType m_compositionType;
    bool m_killRing;
    bool m_preservesTypingStyle;
    
    // Undoing a series of backward deletes will restore a selection around all of the
    // characters that were deleted, but only if the typing command being undone
    // was opened with a backward delete.
    bool m_openedByBackwardDelete;

    bool m_shouldRetainAutocorrectionIndicator;
    bool m_shouldPreventSpellChecking;
};

} // namespace WebCore

#endif // TypingCommand_h
