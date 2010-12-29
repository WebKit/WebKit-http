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
#include <ScrollView.h>
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
	fPatternLength(0),
	fIgnoreEditViewStateChanges(false)
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


int32
BDefaultCompletionStyle::SelectedChoiceIndex() const
{
	return fSelectedIndex;
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

	fIgnoreEditViewStateChanges = true;

	fFullEnteredText = completedText;
	fPatternLength = choiceStr.Length();
	fEditView->SetEditViewState(completedText, 
		fPatternStartPos + choiceStr.Length());

	if (hideChoices) {
		fChoiceView->HideChoices();
		Select(-1);
	}

	fIgnoreEditViewStateChanges = false;
}


void
BDefaultCompletionStyle::CancelChoice()
{
	if (!fChoiceView || !fEditView)
		return;
	if (fChoiceView->ChoicesAreShown()) {
		fIgnoreEditViewStateChanges = true;

		fEditView->SetEditViewState(fFullEnteredText,
			fPatternStartPos + fPatternLength);
		fChoiceView->HideChoices();
		Select(-1);

		fIgnoreEditViewStateChanges = false;
	}
}

void
BDefaultCompletionStyle::EditViewStateChanged(bool updateChoices)
{
	if (fIgnoreEditViewStateChanges || !fChoiceModel || !fChoiceView
		|| !fEditView) {
		return;
	}

	BString text;
	int32 caretPos;
	fEditView->GetEditViewState(text, &caretPos);
	if (fFullEnteredText == text)
		return;

	fFullEnteredText = text;

	if (!updateChoices)
		return;

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


static const int32 MSG_INVOKED = 'invk';


BDefaultChoiceView::ListView::ListView(
		BAutoCompleter::CompletionStyle* completer)
	:
	BListView(BRect(0, 0, 100, 100), "ChoiceViewList"),
	fCompleter(completer)
{
	// we need to check if user clicks outside of window-bounds:
	SetEventMask(B_POINTER_EVENTS);
}


void
BDefaultChoiceView::ListView::AttachedToWindow()
{
	SetTarget(this);
	SetInvocationMessage(new BMessage(MSG_INVOKED));
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
		case MSG_INVOKED:
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
	rgb_color textColor;
	rgb_color nonMatchTextColor;
	rgb_color backColor;
	rgb_color matchColor;
	if (IsSelected()) {
		textColor = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
		backColor = ui_color(B_MENU_SELECTED_BACKGROUND_COLOR);
	} else {
		textColor = ui_color(B_DOCUMENT_TEXT_COLOR);
		backColor = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	}
	matchColor = tint_color(backColor, (B_NO_TINT + B_DARKEN_1_TINT) / 2);
	nonMatchTextColor = tint_color(backColor, 1.7);

	BFont font;
	font_height fontHeight;
	owner->GetFont(&font);
	font.GetHeight(&fontHeight);
	float xPos = frame.left + 1;
	float yPos = frame.top + fontHeight.ascent;
	float w;
	if (fPreText.Length()) {
		w = owner->StringWidth(fPreText.String());
		owner->SetLowColor(backColor);
		owner->FillRect(BRect(xPos, frame.top, xPos + w - 1, frame.bottom),
			B_SOLID_LOW);
		owner->SetHighColor(nonMatchTextColor);
		owner->DrawString(fPreText.String(), BPoint(xPos, yPos));
		xPos += w;
	}
	if (fMatchText.Length()) {
		w = owner->StringWidth(fMatchText.String());
		owner->SetLowColor(matchColor);
		owner->FillRect(BRect(xPos, frame.top, xPos + w - 1, frame.bottom), 
			B_SOLID_LOW);
		owner->SetHighColor(textColor);
		owner->DrawString(fMatchText.String(), BPoint(xPos, yPos));
		xPos += w;
	}
	if (fPostText.Length()) {
		w = owner->StringWidth(fPostText.String());
		owner->SetLowColor(backColor);
		owner->FillRect(BRect(xPos, frame.top, xPos + w - 1, frame.bottom), 
			B_SOLID_LOW);
		owner->SetHighColor(nonMatchTextColor);
		owner->DrawString(fPostText.String(), BPoint(xPos, yPos));
	}
}


// #pragma mark - BDefaultChoiceView


BDefaultChoiceView::BDefaultChoiceView()
	:
	fWindow(NULL),
	fListView(NULL),
	fMaxVisibleChoices(8)
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
	for(int32 i = 0; i<count; ++i) {
		fListView->AddItem(
			new ListItem(choiceModel->ChoiceAt(i))
		);
	}

	BScrollView *scrollView = new BScrollView("", fListView, B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);

	fWindow = new BWindow(BRect(0, 0, 100, 100), "", B_BORDERED_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL, B_NOT_MOVABLE | B_WILL_ACCEPT_FIRST_CLICK 
			| B_AVOID_FOCUS | B_ASYNCHRONOUS_CONTROLS);
	fWindow->AddChild(scrollView);

	int32 visibleCount = min_c(count, fMaxVisibleChoices);
	float listHeight = fListView->ItemFrame(visibleCount - 1).bottom + 1;

	BRect pvRect = editView->GetAdjustmentFrame();
	BRect listRect = pvRect;
	listRect.bottom = listRect.top + listHeight - 1;
	BRect screenRect = BScreen().Frame();
	if (listRect.bottom + 1 + listHeight <= screenRect.bottom)
		listRect.OffsetTo(pvRect.left, pvRect.bottom + 1);
	else
		listRect.OffsetTo(pvRect.left, pvRect.top - listHeight);

	// Moving here to cut off the scrollbar top
	scrollView->MoveTo(0, -1);
	// Adding the 1 and 2 to cut-off the scroll-bar top, right and bottom
	scrollView->ResizeTo(listRect.Width() + 1, listRect.Height() + 2);
	// Move here to compensate for the above
	fListView->MoveTo(0, 1);
	fListView->ResizeTo(listRect.Width() - B_V_SCROLL_BAR_WIDTH, listRect.Height());
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


int32
BDefaultChoiceView::CountVisibleChoices() const
{
	if (fListView)
		return min_c(fMaxVisibleChoices, fListView->CountItems());
	else
		return 0;
}


void
BDefaultChoiceView::SetMaxVisibleChoices(int32 choices)
{
	if (choices < 1)
		choices = 1;
	if (choices == fMaxVisibleChoices)
		return;

	fMaxVisibleChoices = choices;

	// TODO: Update live?
}


int32
BDefaultChoiceView::MaxVisibleChoices() const
{
	return fMaxVisibleChoices;
}

