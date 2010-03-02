/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "AutoCompleter.h"

#include <Looper.h>
#include <Message.h>

#include "AutoCompleterDefaultImpl.h"


// #pragma mark - DefaultPatternSelector


class DefaultPatternSelector : public BAutoCompleter::PatternSelector {
public:
	virtual void SelectPatternBounds(const BString& text, int32 caretPos,
		int32* start, int32* length);
};


void
DefaultPatternSelector::SelectPatternBounds(const BString& text,
	int32 caretPos, int32* start, int32* length)
{
	if (!start || !length)
		return;
	*start = 0;
	*length = text.Length();
}


// #pragma mark - CompletionStyle


BAutoCompleter::CompletionStyle::CompletionStyle(EditView* editView,
		ChoiceModel* choiceModel, ChoiceView* choiceView, 
		PatternSelector* patternSelector)
	:
	fEditView(editView),
	fPatternSelector(patternSelector ? patternSelector
		: new DefaultPatternSelector()),
	fChoiceModel(choiceModel),
	fChoiceView(choiceView)
{
}


BAutoCompleter::CompletionStyle::~CompletionStyle()
{
	delete fEditView;
	delete fChoiceModel;
	delete fChoiceView;
	delete fPatternSelector;
}


void
BAutoCompleter::CompletionStyle::SetEditView(EditView* view)
{
	delete fEditView;
	fEditView = view;
}


void
BAutoCompleter::CompletionStyle::SetPatternSelector(
	PatternSelector* selector)
{
	delete fPatternSelector;
	fPatternSelector = selector;
}


void
BAutoCompleter::CompletionStyle::SetChoiceModel(ChoiceModel* model)
{
	delete fChoiceModel;
	fChoiceModel = model;
}


void
BAutoCompleter::CompletionStyle::SetChoiceView(ChoiceView* view)
{
	delete fChoiceView;
	fChoiceView = view;
}


// #pragma mark - BAutoCompleter


BAutoCompleter::BAutoCompleter(CompletionStyle* completionStyle)
	:
	fCompletionStyle(completionStyle)
{
}


BAutoCompleter::BAutoCompleter(EditView* editView, ChoiceModel* choiceModel,
		ChoiceView* choiceView, PatternSelector* patternSelector)
	:
	fCompletionStyle(new BDefaultCompletionStyle(editView, choiceModel,
		choiceView, patternSelector))
{
}


BAutoCompleter::~BAutoCompleter()
{
	delete fCompletionStyle;
}


bool
BAutoCompleter::Select(int32 index)
{
	if (fCompletionStyle)
		return fCompletionStyle->Select(index);
	else
		return false;
}


bool
BAutoCompleter::SelectNext(bool wrap)
{
	if (fCompletionStyle)
		return fCompletionStyle->SelectNext(wrap);
	else
		return false;
}


bool
BAutoCompleter::SelectPrevious(bool wrap)
{
	if (fCompletionStyle)
		return fCompletionStyle->SelectPrevious(wrap);
	else
		return false;
}


void
BAutoCompleter::ApplyChoice(bool hideChoices)
{
	if (fCompletionStyle)
		fCompletionStyle->ApplyChoice(hideChoices);
}


void
BAutoCompleter::CancelChoice()
{
	if (fCompletionStyle)
		fCompletionStyle->CancelChoice();
}


void
BAutoCompleter::EditViewStateChanged()
{
	if (fCompletionStyle)
		fCompletionStyle->EditViewStateChanged();
}


void
BAutoCompleter::SetEditView(EditView* view)
{
	if (fCompletionStyle)
		fCompletionStyle->SetEditView(view);
}


void
BAutoCompleter::SetPatternSelector(PatternSelector* selector)
{
	if (fCompletionStyle)
		fCompletionStyle->SetPatternSelector(selector);
}


void
BAutoCompleter::SetChoiceModel(ChoiceModel* model)
{
	if (fCompletionStyle)
		fCompletionStyle->SetChoiceModel(model);
}


void
BAutoCompleter::SetChoiceView(ChoiceView* view)
{
	if (fCompletionStyle)
		fCompletionStyle->SetChoiceView(view);
}


void
BAutoCompleter::SetCompletionStyle(CompletionStyle* style)
{
	delete fCompletionStyle;
	fCompletionStyle = style;
}
