/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#ifndef TEXT_CONTROL_COMPLETER_H
#define TEXT_CONTROL_COMPLETER_H

#include <MessageFilter.h>

#include "AutoCompleter.h"


class BTextView;

class TextViewCompleter : protected BAutoCompleter, public BMessageFilter {
public:
								TextViewCompleter(BTextView* textView,
									ChoiceModel* choiceModel = NULL,
									PatternSelector* patternSelector = NULL);
	virtual						~TextViewCompleter();

private:
	virtual	filter_result		Filter(BMessage* message, BHandler** target);

	class TextViewWrapper : public EditView {
	public:
								TextViewWrapper(BTextView* textView);
		virtual	BRect			GetAdjustmentFrame();
		virtual	void			GetEditViewState(BString& text,
									int32* caretPos);
		virtual	void			SetEditViewState(const BString& text,
									int32 caretPos, int32 selectionLength = 0);
	private:
				BTextView*		fTextView;
	};
private:
			BTextView*			fTextView;
};

#endif // TEXT_CONTROL_COMPLETER_H
