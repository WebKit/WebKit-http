/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "TextViewCompleter.h"

#include <Looper.h>
#include <TextControl.h>
#include <stdio.h>

#include "AutoCompleterDefaultImpl.h"


// #pragma mark - TextViewWrapper


TextViewCompleter::TextViewWrapper::TextViewWrapper(BTextView* textView)
	:
	fTextView(textView)
{
}


void
TextViewCompleter::TextViewWrapper::GetEditViewState(BString& text,
	int32* caretPos)
{
	if (fTextView && fTextView->LockLooper()) {
		text = fTextView->Text();
		if (caretPos) {
			int32 end;
			fTextView->GetSelection(caretPos, &end);
		}
		fTextView->UnlockLooper();
	}
}


void
TextViewCompleter::TextViewWrapper::SetEditViewState(const BString& text,
	int32 caretPos, int32 selectionLength)
{
	if (fTextView && fTextView->LockLooper()) {
		fTextView->SetText(text.String(), text.Length());
		fTextView->Select(caretPos, caretPos + selectionLength);
		fTextView->ScrollToSelection();
		fTextView->UnlockLooper();
	}
}


BRect
TextViewCompleter::TextViewWrapper::GetAdjustmentFrame()
{
	BRect frame = fTextView->Bounds();
	frame = fTextView->ConvertToScreen(frame);
	frame.InsetBy(0, -3);
	return frame;
}


// #pragma mark -


TextViewCompleter::TextViewCompleter(BTextView* textView, ChoiceModel* model,
		PatternSelector* patternSelector)
	:
	BAutoCompleter(new TextViewWrapper(textView), model,
		new BDefaultChoiceView(), patternSelector),
	BMessageFilter(B_KEY_DOWN),
	fTextView(textView),
	fModificationsReported(false)
{
	fTextView->AddFilter(this);
}


TextViewCompleter::~TextViewCompleter()
{
	fTextView->RemoveFilter(this);
}


void
TextViewCompleter::SetModificationsReported(bool reported)
{
	fModificationsReported = reported;
}


void
TextViewCompleter::TextModified(bool updateChoices)
{
	EditViewStateChanged(updateChoices);
}


filter_result
TextViewCompleter::Filter(BMessage* message, BHandler** target)
{
	const char* bytes;
	int32 modifiers;
	if ((!target || message->FindString("bytes", &bytes) != B_OK
			|| message->FindInt32("modifiers", &modifiers) != B_OK)
		|| (modifiers & (B_CONTROL_KEY | B_COMMAND_KEY | B_OPTION_KEY
			| B_SHIFT_KEY)) != 0) {
		return B_DISPATCH_MESSAGE;
	}

	switch (bytes[0]) {
		case B_UP_ARROW:
			SelectPrevious();
			// Insert the current choice into the text view, so the user can
			// continue typing. This will not trigger another evaluation, since
			// we don't invoke EditViewStateChanged().
			ApplyChoice(false);
			return B_SKIP_MESSAGE;
		case B_DOWN_ARROW:
			SelectNext();
			// See above.
			ApplyChoice(false);
			return B_SKIP_MESSAGE;
		case B_PAGE_UP:
		{
			int32 index = SelectedChoiceIndex() - CountVisibleChoices();
			index = max_c(index, 0);
			Select(index);
			ApplyChoice(false);
			return B_SKIP_MESSAGE;
		}
		case B_PAGE_DOWN:
		{
			int32 index = SelectedChoiceIndex() + CountVisibleChoices();
			index = min_c(index, CountChoices() - 1);
			Select(index);
			ApplyChoice(false);
			return B_SKIP_MESSAGE;
		}

		case B_ESCAPE:
			CancelChoice();
			return B_DISPATCH_MESSAGE;
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
			if (!fModificationsReported) {
				// dispatch message to textview manually...
				Looper()->DispatchMessage(message, *target);
				// ...and propagate the new state to the auto-completer:
				EditViewStateChanged();
				return B_SKIP_MESSAGE;
			}
			return B_DISPATCH_MESSAGE;
	}
}
