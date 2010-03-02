/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "TextControlCompleter.h"

#include <Looper.h>
#include <TextControl.h>
#include <stdio.h>

#include "AutoCompleterDefaultImpl.h"


// #pragma mark - TextControlWrapper


TextControlCompleter::TextControlWrapper::TextControlWrapper(
		BTextControl* textControl)
	:
	fTextControl(textControl)
{
}


void
TextControlCompleter::TextControlWrapper::GetEditViewState(BString& text,
	int32* caretPos)
{
	if (fTextControl && fTextControl->LockLooper()) {
		text = fTextControl->Text();
		if (caretPos) {
			int32 end;
			fTextControl->TextView()->GetSelection(caretPos, &end);
		}
		fTextControl->UnlockLooper();
	}
}


void
TextControlCompleter::TextControlWrapper::SetEditViewState(const BString& text,
	int32 caretPos, int32 selectionLength)
{
	if (fTextControl && fTextControl->LockLooper()) {
		fTextControl->TextView()->SetText(text.String(), text.Length());
		fTextControl->TextView()->Select(caretPos, caretPos + selectionLength);
		fTextControl->TextView()->ScrollToSelection();
		fTextControl->UnlockLooper();
	}
}


BRect
TextControlCompleter::TextControlWrapper::GetAdjustmentFrame()
{
	BRect frame = fTextControl->TextView()->Bounds();
	frame = fTextControl->TextView()->ConvertToScreen(frame);
	frame.InsetBy(0, -3);
	return frame;
}


TextControlCompleter::TextControlCompleter(BTextControl* textControl, 
		ChoiceModel* model, PatternSelector* patternSelector)
	:
	BAutoCompleter(new TextControlWrapper(textControl), model, 
		new BDefaultChoiceView(), patternSelector),
	BMessageFilter(B_KEY_DOWN),
	fTextControl(textControl)
{
	fTextControl->TextView()->AddFilter(this);
}


TextControlCompleter::~TextControlCompleter()
{
	fTextControl->TextView()->RemoveFilter(this);
}


filter_result
TextControlCompleter::Filter(BMessage* message, BHandler** target)
{
	int32 rawChar, modifiers;
	if (!target || message->FindInt32("raw_char", &rawChar) != B_OK
		|| message->FindInt32("modifiers", &modifiers) != B_OK) {
		return B_DISPATCH_MESSAGE;
	}
	
	switch (rawChar) {
		case B_UP_ARROW:
			SelectPrevious();
			return B_SKIP_MESSAGE;
		case B_DOWN_ARROW:
			SelectNext();
			return B_SKIP_MESSAGE;
		case B_ESCAPE:
			CancelChoice();
			return B_SKIP_MESSAGE;
		case B_RETURN:
			if (IsChoiceSelected()) {
				ApplyChoice();
				EditViewStateChanged();
			} else
				CancelChoice();
			return B_DISPATCH_MESSAGE;
		case B_TAB: {
			// make sure that the choices-view is closed when tabbing out:
			CancelChoice();
			return B_DISPATCH_MESSAGE;
		}
		default:
			// dispatch message to textview manually...
			Looper()->DispatchMessage(message, *target);
			// ...and propagate the new state to the auto-completer:
			EditViewStateChanged();
			return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}
