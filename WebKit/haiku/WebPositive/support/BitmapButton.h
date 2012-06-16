/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BITMAP_BUTTON_H
#define BITMAP_BUTTON_H

#include <Button.h>


class BBitmap;


class BitmapButton : public BButton {
public:
	enum {
		BUTTON_BACKGROUND = 0,
		MENUBAR_BACKGROUND,
	};

								BitmapButton(const char* resourceName,
									BMessage* message);

								BitmapButton(const uint8* bits, uint32 width,
									uint32 height, color_space format,
									BMessage* message);

	virtual						~BitmapButton();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				Draw(BRect updateRect);

			void				SetBackgroundMode(uint32 mode);

private:
			BBitmap*			fBitmap;
			uint32				fBackgroundMode;
};


#endif // BITMAP_BUTTON_H
