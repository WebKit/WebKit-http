/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "BitmapButton.h"

#include <string.h>

#include <Bitmap.h>
#include <ControlLook.h>
#include <TranslationUtils.h>


static const float kFrameInset = 2;


BitmapButton::BitmapButton(const char* resourceName, BMessage* message)
	:
	BButton("", message),
	fBitmap(BTranslationUtils::GetBitmap(resourceName)),
	fBackgroundMode(BUTTON_BACKGROUND)
{
}


BitmapButton::BitmapButton(const uint8* bits, uint32 width, uint32 height,
		color_space format, BMessage* message)
	:
	BButton("", message),
	fBitmap(new BBitmap(BRect(0, 0, width - 1, height - 1), 0, format)),
	fBackgroundMode(BUTTON_BACKGROUND)
{
	memcpy(fBitmap->Bits(), bits, fBitmap->BitsLength());
}


BitmapButton::~BitmapButton()
{
	delete fBitmap;
}


BSize
BitmapButton::MinSize()
{
	BSize min(0, 0);
	if (fBitmap) {
		min.width = fBitmap->Bounds().Width();
		min.height = fBitmap->Bounds().Height();
	}
	min.width += kFrameInset * 2;
	min.height += kFrameInset * 2;
	return min;
}


BSize
BitmapButton::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
BitmapButton::PreferredSize()
{
	return MinSize();
}


void
BitmapButton::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = be_control_look->Flags(this);

	if (fBackgroundMode == BUTTON_BACKGROUND || Value() == B_CONTROL_ON) {
		be_control_look->DrawButtonBackground(this, bounds, updateRect, base,
			flags);
	} else {
		SetHighColor(tint_color(base, B_DARKEN_2_TINT));
		StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
		bounds.bottom--;
		be_control_look->DrawMenuBarBackground(this, bounds, updateRect, base,
			flags);
	}

	if (fBitmap == NULL)
		return;

	SetDrawingMode(B_OP_ALPHA);

	if (!IsEnabled()) {
		SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		SetHighColor(0, 0, 0, 120);
	}

	BRect bitmapBounds(fBitmap->Bounds());
	BPoint bitmapLocation(
		floorf((bounds.left + bounds.right
			- (bitmapBounds.left + bitmapBounds.right)) / 2 + 0.5f),
		floorf((bounds.top + bounds.bottom
			- (bitmapBounds.top + bitmapBounds.bottom)) / 2 + 0.5f));

	DrawBitmap(fBitmap, bitmapLocation);
}


void
BitmapButton::SetBackgroundMode(uint32 mode)
{
	if (fBackgroundMode != mode) {
		fBackgroundMode = mode;
		Invalidate();
	}
}

