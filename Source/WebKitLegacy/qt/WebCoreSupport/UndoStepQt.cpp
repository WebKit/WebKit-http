/*
    Copyright (C) 2007 Staikos Computing Services Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "UndoStepQt.h"

#include <qobject.h>

using namespace WebCore;

static QString undoNameForEditAction(const EditAction editAction)
{
    switch (editAction) {
    case EditAction::Unspecified:
        return QString();
    case EditAction::Insert:
    case EditAction::InsertFromDrop:
        return QObject::tr("Insert");
    case EditAction::SetColor:
        return QObject::tr("Set Color");
    case EditAction::SetBackgroundColor:
        return QObject::tr("Set Background Color");
    case EditAction::TurnOffKerning:
        return QObject::tr("Turn Off Kerning");
    case EditAction::TightenKerning:
        return QObject::tr("Tighten Kerning");
    case EditAction::LoosenKerning:
        return QObject::tr("Loosen Kerning");
    case EditAction::UseStandardKerning:
        return QObject::tr("Use Standard Kerning");
    case EditAction::TurnOffLigatures:
        return QObject::tr("Turn Off Ligatures");
    case EditAction::UseStandardLigatures:
        return QObject::tr("Use Standard Ligatures");
    case EditAction::UseAllLigatures:
        return QObject::tr("Use All Ligatures");
    case EditAction::RaiseBaseline:
        return QObject::tr("Raise Baseline");
    case EditAction::LowerBaseline:
        return QObject::tr("Lower Baseline");
    case EditAction::SetTraditionalCharacterShape:
        return QObject::tr("Set Traditional Character Shape");
    case EditAction::SetFont:
        return QObject::tr("Set Font");
    case EditAction::ChangeAttributes:
        return QObject::tr("Change Attributes");
    case EditAction::AlignLeft:
        return QObject::tr("Align Left");
    case EditAction::AlignRight:
        return QObject::tr("Align Right");
    case EditAction::Center:
        return QObject::tr("Center");
    case EditAction::Justify:
        return QObject::tr("Justify");
    case EditAction::SetBlockWritingDirection:
    case EditAction::SetInlineWritingDirection:
        return QObject::tr("Set Writing Direction");
    case EditAction::Subscript:
        return QObject::tr("Subscript");
    case EditAction::Bold:
        return QObject::tr("Bold");
    case EditAction::Italics:
        return QObject::tr("Italic");
    case EditAction::Superscript:
        return QObject::tr("Superscript");
    case EditAction::Underline:
        return QObject::tr("Underline");
    case EditAction::Outline:
        return QObject::tr("Outline");
    case EditAction::Unscript:
        return QObject::tr("Unscript");
    case EditAction::DeleteByDrag:
        return QObject::tr("Drag");
    case EditAction::Cut:
        return QObject::tr("Cut");
    case EditAction::Paste:
        return QObject::tr("Paste");
    case EditAction::Delete:
        return QObject::tr("Delete");
    case EditAction::Dictation:
        return QObject::tr("Dictation");
    case EditAction::PasteFont:
        return QObject::tr("Paste Font");
    case EditAction::PasteRuler:
        return QObject::tr("Paste Ruler");
    case EditAction::TypingDeleteSelection:
    case EditAction::TypingDeleteBackward:
    case EditAction::TypingDeleteForward:
    case EditAction::TypingDeleteWordBackward:
    case EditAction::TypingDeleteWordForward:
    case EditAction::TypingDeleteLineBackward:
    case EditAction::TypingDeleteLineForward:
    case EditAction::TypingInsertText:
    case EditAction::TypingInsertLineBreak:
    case EditAction::TypingInsertParagraph:
        return QObject::tr("Typing");
    case EditAction::CreateLink:
        return QObject::tr("Create Link");
    case EditAction::Unlink:
        return QObject::tr("Unlink");
    case EditAction::InsertOrderedList:
    case EditAction::InsertUnorderedList:
        return QObject::tr("Insert List");
    case EditAction::FormatBlock:
        return QObject::tr("Formatting");
    case EditAction::Indent:
        return QObject::tr("Indent");
    case EditAction::Outdent:
        return QObject::tr("Outdent");
    }

    ASSERT_NOT_REACHED();
    return QString();
}

UndoStepQt::UndoStepQt(UndoStep& step)
    : m_step(step)
    , m_first(true)
{
    m_text = undoNameForEditAction(step.editingAction());
}


UndoStepQt::~UndoStepQt()
{
}

void UndoStepQt::redo()
{
    if (m_first) {
        m_first = false;
        return;
    }
    m_step->reapply();
}


void UndoStepQt::undo()
{
    m_step->unapply();
}

QString UndoStepQt::text() const
{
    return m_text;
}

// vim: ts=4 sw=4 et
