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


class BTextControl;

class TextControlCompleter : protected BAutoCompleter, public BMessageFilter {
public:
								TextControlCompleter(BTextControl* textControl,
									ChoiceModel* choiceModel = NULL,
									PatternSelector* patternSelector = NULL);
	virtual						~TextControlCompleter();
	
private:
	virtual	filter_result		Filter(BMessage* message, BHandler** target);
	
	class TextControlWrapper : public EditView {
	public:
								TextControlWrapper(BTextControl* textControl);
		virtual	BRect			GetAdjustmentFrame();
		virtual	void			GetEditViewState(BString& text,
									int32* caretPos);
		virtual	void			SetEditViewState(const BString& text,
									int32 caretPos, int32 selectionLength = 0);
	private:
				BTextControl*	fTextControl;
	};
private:
			BTextControl*		fTextControl;
};

#endif // TEXT_CONTROL_COMPLETER_H
