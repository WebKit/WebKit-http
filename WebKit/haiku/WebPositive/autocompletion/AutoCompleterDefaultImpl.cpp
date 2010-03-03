/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "AutoCompleterDefaultImpl.h"

#include <ListView.h>
#include <Screen.h>
#include <Window.h>


// #pragma mark - BDefaultPatternSelector

void
BDefaultPatternSelector::SelectPatternBounds(const BString& text,
	int32 caretPos, int32* start, int32* length)
{
	if (!start || !length)
		return;
	*start = 0;
	*length = text.Length();
}


// #pragma mark - BDefaultCompletionStyle


BDefaultCompletionStyle::BDefaultCompletionStyle(
		BAutoCompleter::EditView* editView,
		BAutoCompleter::ChoiceModel* choiceModel,
		BAutoCompleter::ChoiceView* choiceView,
		BAutoCompleter::PatternSelector* patternSelector)
	:
	CompletionStyle(editView, choiceModel, choiceView, patternSelector),
	fSelectedIndex(-1),
	fPatternStartPos(0),
	fPatternLength(0)
{
}


BDefaultCompletionStyle::~BDefaultCompletionStyle()
{
}


bool
BDefaultCompletionStyle::Select(int32 index)
{
	if (!fChoiceView || !fChoiceModel || index == fSelectedIndex
		|| index < -1 || index >= fChoiceModel->CountChoices()) {
		return false;
	}

	fSelectedIndex = index;
	fChoiceView->SelectChoiceAt(index);
	return true;
}


bool
BDefaultCompletionStyle::SelectNext(bool wrap)
{
	if (!fChoiceModel || fChoiceModel->CountChoices() == 0)
		return false;

	int32 newIndex = fSelectedIndex + 1;
	if (newIndex >= fChoiceModel->CountChoices()) {
		if (wrap)
			newIndex = 0;
		else
			newIndex = fSelectedIndex;
	}
	return Select(newIndex);
}


bool
BDefaultCompletionStyle::SelectPrevious(bool wrap)
{
	if (!fChoiceModel || fChoiceModel->CountChoices() == 0)
		return false;

	int32 newIndex = fSelectedIndex - 1;
	if (newIndex < 0) {
		if (wrap)
			newIndex = fChoiceModel->CountChoices() - 1;
		else
			newIndex = 0;
	}
	return Select(newIndex);
}


bool
BDefaultCompletionStyle::IsChoiceSelected() const
{
	return fSelectedIndex >= 0;
}


void
BDefaultCompletionStyle::ApplyChoice(bool hideChoices)
{
	if (!fChoiceModel || !fChoiceView || !fEditView || fSelectedIndex < 0)
		return;

	BString completedText(fFullEnteredText);
	completedText.Remove(fPatternStartPos, fPatternLength);
	const BString& choiceStr = fChoiceModel->ChoiceAt(fSelectedIndex)->Text();
	completedText.Insert(choiceStr, fPatternStartPos);

	fFullEnteredText = completedText;
	fPatternLength = choiceStr.Length();
	fEditView->SetEditViewState(completedText, 
		fPatternStartPos+choiceStr.Length());

	if (hideChoices) {
		fChoiceView->HideChoices();
		Select(-1);
	}
}


void
BDefaultCompletionStyle::CancelChoice()
{
	if (!fChoiceView || !fEditView)
		return;
	if (fChoiceView->ChoicesAreShown()) {
		fEditView->SetEditViewState(fFullEnteredText, 
			fPatternStartPos+fPatternLength);
		fChoiceView->HideChoices();
		Select(-1);
	}
}

void
BDefaultCompletionStyle::EditViewStateChanged()
{
	if (!fChoiceModel || !fChoiceView || !fEditView)
		return;

	BString text;
	int32 caretPos;
	fEditView->GetEditViewState(text, &caretPos);
	if (fFullEnteredText == text)
		return;
	fFullEnteredText = text;
	fPatternSelector->SelectPatternBounds(text, caretPos, &fPatternStartPos, 
		&fPatternLength);
	BString pattern(text.String() + fPatternStartPos, fPatternLength);
	fChoiceModel->FetchChoicesFor(pattern);

	Select(-1);
	// show a single choice only if it doesn't match the pattern exactly:
	if (fChoiceModel->CountChoices() > 1 || (fChoiceModel->CountChoices() == 1
			&& pattern.ICompare(fChoiceModel->ChoiceAt(0)->Text())) != 0) {
		fChoiceView->ShowChoices(this);
		fChoiceView->SelectChoiceAt(fSelectedIndex);
	} else
		fChoiceView->HideChoices();
}


// #pragma mark - BDefaultChoiceView::ListView


static const int32 BM_INVOKED = 'bmin';


BDefaultChoiceView::ListView::ListView(
		BAutoCompleter::CompletionStyle* completer)
	:
	BListView(BRect(0,0,100,100), "ChoiceViewList"),
	fCompleter(completer)
{
	// we need to check if user clicks outside of window-bounds:
	SetEventMask(B_POINTER_EVENTS);
}


void
BDefaultChoiceView::ListView::AttachedToWindow()
{
	SetTarget(this);
	SetInvocationMessage(new BMessage(BM_INVOKED));
	BListView::AttachedToWindow();
}


void
BDefaultChoiceView::ListView::SelectionChanged()
{
	fCompleter->Select(CurrentSelection(0));
}


void
BDefaultChoiceView::ListView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case BM_INVOKED:
			fCompleter->ApplyChoice();
			break;
		default:
			BListView::MessageReceived(message);
	}
}


void
BDefaultChoiceView::ListView::MouseDown(BPoint point)
{
	if (!Window()->Frame().Contains(ConvertToScreen(point)))
		// click outside of window, so we close it:
		Window()->Quit();
	else
		BListView::MouseDown(point);
}


// #pragma mark - BDefaultChoiceView::ListItem


BDefaultChoiceView::ListItem::ListItem(const BAutoCompleter::Choice* choice)
	:
	BListItem()
{
	fPreText = choice->DisplayText();
	if (choice->MatchLen() > 0) {
		fPreText.MoveInto(fMatchText, choice->MatchPos(), choice->MatchLen());
		fPreText.MoveInto(fPostText, choice->MatchPos(), fPreText.Length());
	}
}


void
BDefaultChoiceView::ListItem::DrawItem(BView* owner, BRect frame,
	bool complete)
{
	rgb_color textCol, backCol, matchCol;
	if (IsSelected()) {
		textCol = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
		backCol = ui_color(B_MENU_SELECTED_BACKGROUND_COLOR);
	} else {
		textCol = ui_color(B_DOCUMENT_TEXT_COLOR);
		backCol = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	}
	matchCol = tint_color(backCol, (B_NO_TINT + B_DARKEN_1_TINT) / 2);

	BFont font;
	font_height fontHeight;
	owner->GetFont(&font);
	font.GetHeight(&fontHeight);
	float xPos = frame.left + 1;
	float yPos = frame.top + fontHeight.ascent;
	float w;
	if (fPreText.Length()) {
		w = owner->StringWidth(fPreText.String());
		owner->SetLowColor(backCol);
		owner->FillRect(BRect(xPos, frame.top, xPos + w - 1, frame.bottom),
			B_SOLID_LOW);
		owner->SetHighColor(textCol);
		owner->DrawString(fPreText.String(), BPoint(xPos, yPos));
		xPos += w;
	}
	if (fMatchText.Length()) {
		w = owner->StringWidth(fMatchText.String());
		owner->SetLowColor(matchCol);
		owner->FillRect(BRect(xPos, frame.top, xPos + w - 1, frame.bottom), 
			B_SOLID_LOW);
		owner->DrawString(fMatchText.String(), BPoint(xPos, yPos));
		xPos += w;
	}
	if (fPostText.Length()) {
		w = owner->StringWidth(fPostText.String());
		owner->SetLowColor(backCol);
		owner->FillRect(BRect(xPos, frame.top, xPos + w - 1, frame.bottom), 
			B_SOLID_LOW);
		owner->DrawString(fPostText.String(), BPoint(xPos, yPos));
	}
}


// #pragma mark - BDefaultChoiceView


BDefaultChoiceView::BDefaultChoiceView()
	:
	fWindow(NULL),
	fListView(NULL)
{
	
}


BDefaultChoiceView::~BDefaultChoiceView()
{
	HideChoices();
}
	

void
BDefaultChoiceView::SelectChoiceAt(int32 index)
{
	if (fListView && fListView->LockLooper()) {
		if (index < 0)
			fListView->DeselectAll();
		else {
			fListView->Select(index);
			fListView->ScrollToSelection();
		}
		fListView->UnlockLooper();
	}
}


void
BDefaultChoiceView::ShowChoices(BAutoCompleter::CompletionStyle* completer)
{
	if (!completer)
		return;

	HideChoices();

	BAutoCompleter::ChoiceModel* choiceModel = completer->GetChoiceModel();
	BAutoCompleter::EditView* editView = completer->GetEditView();

	if (!editView || !choiceModel || choiceModel->CountChoices() == 0)
		return;

	fListView = new ListView(completer);
	int32 count = choiceModel->CountChoices();
	for(int32 i=0; i<count; ++i) {
		fListView->AddItem(
			new ListItem(choiceModel->ChoiceAt(i))
		);
	}

	fWindow = new BWindow(BRect(0, 0, 100, 100), "", B_BORDERED_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_MOVABLE | B_WILL_ACCEPT_FIRST_CLICK 
			| B_AVOID_FOCUS | B_ASYNCHRONOUS_CONTROLS);
	fWindow->AddChild(fListView);

	int32 visibleCount = min_c(count, 5);
	float listHeight = fListView->ItemFrame(visibleCount - 1).bottom + 1;

	BRect pvRect = editView->GetAdjustmentFrame();
	BRect listRect = pvRect;
	listRect.bottom = listRect.top + listHeight - 1;
	BRect screenRect = BScreen().Frame();
	if (listRect.bottom + 1 + listHeight <= screenRect.bottom)
		listRect.OffsetTo(pvRect.left, pvRect.bottom + 1);
	else
		listRect.OffsetTo(pvRect.left, pvRect.top - listHeight);

	fListView->MoveTo(0, 0);
	fListView->ResizeTo(listRect.Width(), listRect.Height());
	fWindow->MoveTo(listRect.left, listRect.top);
	fWindow->ResizeTo(listRect.Width(), listRect.Height());
	fWindow->Show();
}


void
BDefaultChoiceView::HideChoices()
{
	if (fWindow && fWindow->Lock()) {
		fWindow->Quit();
		fWindow = NULL;
		fListView = NULL;
	}
}


bool
BDefaultChoiceView::ChoicesAreShown()
{
	return (fWindow != NULL);
}

