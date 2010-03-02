/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _AUTO_COMPLETER_DEFAULT_IMPL_H
#define _AUTO_COMPLETER_DEFAULT_IMPL_H

#include <ListView.h>
#include <String.h>

#include "AutoCompleter.h"

class BDefaultPatternSelector : public BAutoCompleter::PatternSelector {
public:
	virtual	void				SelectPatternBounds(const BString& text,
									int32 caretPos, int32* start,
									int32* length);
};


class BDefaultCompletionStyle : public BAutoCompleter::CompletionStyle {
public:
								BDefaultCompletionStyle(
									BAutoCompleter::EditView* editView, 
									BAutoCompleter::ChoiceModel* choiceModel,
									BAutoCompleter::ChoiceView* choiceView, 
									BAutoCompleter::PatternSelector*
										patternSelector);
	virtual						~BDefaultCompletionStyle();

	virtual	bool				Select(int32 index);
	virtual	bool				SelectNext(bool wrap = false);
	virtual	bool				SelectPrevious(bool wrap = false);
	virtual	bool				IsChoiceSelected() const;

	virtual	void				ApplyChoice(bool hideChoices = true);
	virtual	void				CancelChoice();

	virtual	void				EditViewStateChanged();

private:
			BString				fFullEnteredText;
			int32				fSelectedIndex;
			int32				fPatternStartPos;
			int32				fPatternLength;
};


class BDefaultChoiceView : public BAutoCompleter::ChoiceView {
protected:
	class ListView : public BListView {
	public:
								ListView(
									BAutoCompleter::CompletionStyle* completer);
		virtual	void			SelectionChanged();
		virtual	void			MessageReceived(BMessage* msg);
		virtual	void			MouseDown(BPoint point);
		virtual	void			AttachedToWindow();
	private:
				BAutoCompleter::CompletionStyle* fCompleter;
	};

	class ListItem : public BListItem {
	public:
								ListItem(const BAutoCompleter::Choice* choice);
		virtual	void			DrawItem(BView* owner, BRect frame,
									bool complete = false);
	private:
				BString			fPreText;
				BString			fMatchText;
				BString			fPostText;
	};

public:
								BDefaultChoiceView();
	virtual						~BDefaultChoiceView();
	
	virtual	void				SelectChoiceAt(int32 index);
	virtual	void				ShowChoices(
									BAutoCompleter::CompletionStyle* completer);
	virtual	void				HideChoices();
	virtual	bool				ChoicesAreShown();

private:
			BWindow*			fWindow;
			ListView*			fListView;
};

#endif // _AUTO_COMPLETER_DEFAULT_IMPL_H
