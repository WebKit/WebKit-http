/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef _AUTO_COMPLETER_H
#define _AUTO_COMPLETER_H

#include <MessageFilter.h>

#include <Rect.h>
#include <String.h>

class BAutoCompleter {
public:
	class Choice {
	public:
								Choice(const BString& choiceText,
									const BString& displayText, int32 matchPos,
									int32 matchLen)
									:
									fText(choiceText),
									fDisplayText(displayText),
									fMatchPos(matchPos),
									fMatchLen(matchLen)
								{
								}
		virtual					~Choice() {}
				const BString&	Text() const { return fText; }
				const BString&	DisplayText() const { return fDisplayText; }
				int32			MatchPos() const { return fMatchPos; }
				int32			MatchLen() const { return fMatchLen; }

	private:
				BString			fText;
				BString			fDisplayText;
				int32			fMatchPos;
				int32			fMatchLen;
	};

	class EditView {
	public:
		virtual					~EditView()	{}

		virtual	BRect			GetAdjustmentFrame() = 0;
		virtual	void			GetEditViewState(BString& text,
									int32* caretPos) = 0;
		virtual	void			SetEditViewState(const BString& text,
									int32 caretPos,
									int32 selectionLength = 0) = 0;
	};

	class PatternSelector {
	public:
		virtual					~PatternSelector() {}
		
		virtual	void			SelectPatternBounds(const BString& text,
									int32 caretPos, int32* start,
									int32* length) = 0;
	};

	class ChoiceModel {
	public:
	
		virtual					~ChoiceModel() {}
		
		virtual	void			FetchChoicesFor(const BString& pattern) = 0;

		virtual	int32			CountChoices() const = 0;
		virtual	const Choice*	ChoiceAt(int32 index) const = 0;
	};
	
	class CompletionStyle;
	class ChoiceView {
	public:
		virtual					~ChoiceView() {}

		virtual	void			SelectChoiceAt(int32 index) = 0;
		virtual	void			ShowChoices(
									BAutoCompleter::CompletionStyle* completer)
									= 0;
		virtual	void			HideChoices() = 0;
		virtual	bool			ChoicesAreShown() = 0;
	};

	class CompletionStyle {
	public:
								CompletionStyle(EditView* editView,
									ChoiceModel* choiceModel,
									ChoiceView* choiceView,
									PatternSelector* patternSelector);
		virtual					~CompletionStyle();

		virtual	bool			Select(int32 index) = 0;
		virtual	bool			SelectNext(bool wrap = false) = 0;
		virtual	bool			SelectPrevious(bool wrap = false) = 0;
		virtual	bool			IsChoiceSelected() const = 0;

		virtual	void			ApplyChoice(bool hideChoices = true) = 0;
		virtual	void			CancelChoice() = 0;

		virtual	void			EditViewStateChanged(bool updateChoices) = 0;

				void			SetEditView(EditView* view);
				void			SetPatternSelector(PatternSelector* selector);
				void			SetChoiceModel(ChoiceModel* model);
				void			SetChoiceView(ChoiceView* view);

				EditView*		GetEditView() { return fEditView; }
				PatternSelector* GetPatternSelector()
									{ return fPatternSelector; }
				ChoiceModel*	GetChoiceModel() { return fChoiceModel; }
				ChoiceView*		GetChoiceView() { return fChoiceView; }

	protected:
				EditView*		fEditView;
				PatternSelector* fPatternSelector;
				ChoiceModel*	fChoiceModel;
				ChoiceView*		fChoiceView;
	};

protected:
								BAutoCompleter(
									CompletionStyle* completionStyle = NULL);
								BAutoCompleter(EditView* editView,
									ChoiceModel* choiceModel,
									ChoiceView* choiceView,
									PatternSelector* patternSelector);
	virtual						~BAutoCompleter();
	
			void				EditViewStateChanged(
									bool updateChoices = true);
		
			bool				Select(int32 index);
			bool				SelectNext(bool wrap = false);
			bool				SelectPrevious(bool wrap = false);
			bool				IsChoiceSelected() const;
		
			void				ApplyChoice(bool hideChoices = true);
			void				CancelChoice();
			
			void				SetEditView(EditView* view);
			void				SetPatternSelector(PatternSelector* selector);
			void				SetChoiceModel(ChoiceModel* model);
			void				SetChoiceView(ChoiceView* view);
		
			void				SetCompletionStyle(CompletionStyle* style);
	
private:
			CompletionStyle*	fCompletionStyle;
};


#endif // _AUTO_COMPLETER_H
