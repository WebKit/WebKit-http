/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef URL_INPUT_GROUP_H
#define URL_INPUT_GROUP_H

#include <GroupView.h>

class BButton;
class BTextView;


class URLInputGroup : public BGroupView {
public:
								URLInputGroup(BMessage* goMessage);
	virtual						~URLInputGroup();

	virtual	void				AttachedToWindow();
	virtual	void				WindowActivated(bool active);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MakeFocus(bool focus = true);

			BTextView*			TextView() const;
			void				SetText(const char* text);
			const char*			Text() const;

			BButton*			GoButton() const;

private:
			class URLTextView;

			URLTextView*		fTextView;
			BButton*			fGoButton;
			bool				fWindowActive;
};

#endif // URL_INPUT_GROUP_H

