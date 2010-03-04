/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Philippe Saint-Pierre, stpere@gmail.com 
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FontSelectionView.h"

#include <Box.h>
#include <Catalog.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>
#include <LayoutItem.h>
#include <GroupLayoutBuilder.h>

#include <stdio.h>

#undef TR_CONTEXT
#define TR_CONTEXT "Font Selection view"


static const float kMinSize = 8.0;
static const float kMaxSize = 18.0;

static const int32 kMsgSetFamily = 'fmly';
static const int32 kMsgSetStyle = 'styl';
static const int32 kMsgSetSize = 'size';


//	#pragma mark -


FontSelectionView::FontSelectionView(const char* name, const char* label,
		bool separateStyles, const BFont* currentFont)
	:
	BHandler(name)
{
	if (currentFont == NULL)
		fCurrentFont = _DefaultFont();
	else
		fCurrentFont = *currentFont;

	fSavedFont = fCurrentFont;

	fSizesMenu = new BPopUpMenu("size menu");
	fFontsMenu = new BPopUpMenu("font menu");

	// font menu
	fFontsMenuField = new BMenuField("fonts", label, fFontsMenu, NULL);
	fFontsMenuField->SetFont(be_bold_font);

	// styles menu, if desired
	if (separateStyles) {
		fStylesMenu = new BPopUpMenu("styles menu");
		fStylesMenuField = new BMenuField("styles", TR("Style:"), fStylesMenu,
			NULL);
	} else {
		fStylesMenu = NULL;
		fStylesMenuField = NULL;
	}

	// size menu
	fSizesMenuField = new BMenuField("size", TR("Size:"), fSizesMenu, NULL);
	fSizesMenuField->SetAlignment(B_ALIGN_RIGHT);

	// preview
	fPreviewText = new BStringView("preview text",
		TR_CMT("The quick brown fox jumps over the lazy dog.","Don't translate this literally ! Use a phrase showing all chars from A to Z.")); 

	fPreviewText->SetFont(&fCurrentFont);
	fPreviewText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED)); 
	fPreviewText->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		1.65));
	fPreviewText->SetAlignment(B_ALIGN_RIGHT);
}


FontSelectionView::~FontSelectionView()
{
	// Some controls may not have been attached...
	if (!fPreviewText->Window())
		delete fPreviewText;
	if (!fSizesMenuField->Window())
		delete fSizesMenuField;
	if (fStylesMenuField && !fStylesMenuField->Window())
		delete fStylesMenuField;
	if (!fFontsMenuField->Window())
		delete fFontsMenuField;
}


void
FontSelectionView::AttachedToLooper()
{
	_BuildSizesMenu();
	UpdateFontsMenu();
}


void
FontSelectionView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetSize:
		{
			int32 size;
			if (message->FindInt32("size", &size) != B_OK
				|| size == fCurrentFont.Size())
				break;

			fCurrentFont.SetSize(size);
			_UpdateFontPreview();
			break;
		}

		case kMsgSetFamily:
		{
			const char* family;
			if (message->FindString("family", &family) != B_OK)
				break;

			font_style style;
			fCurrentFont.GetFamilyAndStyle(NULL, &style);

			BMenuItem* familyItem = fFontsMenu->FindItem(family);
			if (familyItem != NULL) {
				_SelectCurrentFont(false);

				BMenuItem* styleItem;
				if (fStylesMenuField != NULL)
					styleItem = fStylesMenuField->Menu()->FindMarked();
				else {
					styleItem = familyItem->Submenu()->FindItem(style);
					if (styleItem == NULL)
						styleItem = familyItem->Submenu()->ItemAt(0);
				}

				if (styleItem != NULL) {
					styleItem->SetMarked(true);
					fCurrentFont.SetFamilyAndStyle(family, styleItem->Label());
					_UpdateFontPreview();
				}
				if (fStylesMenuField != NULL)
					_AddStylesToMenu(fCurrentFont, fStylesMenuField->Menu());
			}
			break;
		}

		case kMsgSetStyle:
		{
			const char* family;
			const char* style;
			if (message->FindString("family", &family) != B_OK
				|| message->FindString("style", &style) != B_OK)
				break;

			BMenuItem *familyItem = fFontsMenu->FindItem(family);
			if (!familyItem)
				break;

			_SelectCurrentFont(false);
			familyItem->SetMarked(true);

			fCurrentFont.SetFamilyAndStyle(family, style);
			_UpdateFontPreview();
			break;
		}

		default:
			BHandler::MessageReceived(message);
	}
}


// #pragma mark -


void
FontSelectionView::SetFont(const BFont& font, float size)
{
	BFont resizedFont(font);
	resizedFont.SetSize(size);
	SetFont(resizedFont);
}


void
FontSelectionView::SetFont(const BFont& font)
{
	if (font == fCurrentFont && font == fSavedFont)
		return;

	_SelectCurrentFont(false);
	fSavedFont = fCurrentFont = font;
	_UpdateFontPreview();

	_SelectCurrentFont(true);
	_SelectCurrentSize(true);
}


void
FontSelectionView::SetSize(float size)
{
	SetFont(fCurrentFont, size);
}


const BFont&
FontSelectionView::Font() const
{
	return fCurrentFont;
}


void
FontSelectionView::SetDefaults()
{
	BFont defaultFont = _DefaultFont();
	if (defaultFont == fCurrentFont)
		return;

	_SelectCurrentFont(false);

	fCurrentFont = defaultFont;
	_UpdateFontPreview();

	_SelectCurrentFont(true);
	_SelectCurrentSize(true);
}


void
FontSelectionView::Revert()
{
	if (!IsRevertable())
		return;

	_SelectCurrentFont(false);

	fCurrentFont = fSavedFont;
	_UpdateFontPreview();

	_SelectCurrentFont(true);
	_SelectCurrentSize(true);
}


bool
FontSelectionView::IsDefaultable()
{
	return fCurrentFont != _DefaultFont();
}


bool
FontSelectionView::IsRevertable()
{
	return fCurrentFont != fSavedFont;
}


void
FontSelectionView::UpdateFontsMenu()
{
	int32 numFamilies = count_font_families();

	fFontsMenu->RemoveItems(0, fFontsMenu->CountItems(), true);

	BFont font = fCurrentFont;

	font_family currentFamily;
	font_style currentStyle;
	font.GetFamilyAndStyle(&currentFamily, &currentStyle);

	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		uint32 flags;
		if (get_font_family(i, &family, &flags) != B_OK)
			continue;

		// if we're setting the fixed font, we only want to show fixed fonts
		if (!strcmp(Name(), "fixed") && (flags & B_IS_FIXED) == 0)
			continue;

		font.SetFamilyAndFace(family, B_REGULAR_FACE);

		BMessage* message = new BMessage(kMsgSetFamily);
		message->AddString("family", family);
		message->AddString("name", Name());

		BMenuItem* familyItem;
		if (fStylesMenuField != NULL) {
			familyItem = new BMenuItem(family, message);
		} else {
			// Each family item has a submenu with all styles for that font.
			BMenu* stylesMenu = new BMenu(family);
			_AddStylesToMenu(font, stylesMenu);
			familyItem = new BMenuItem(stylesMenu, message);
		}

		familyItem->SetMarked(strcmp(family, currentFamily) == 0);
		fFontsMenu->AddItem(familyItem);
		familyItem->SetTarget(this);
	}

	// Separate styles menu for only the current font.
	if (fStylesMenuField != NULL)
		_AddStylesToMenu(fCurrentFont, fStylesMenuField->Menu());
}


// #pragma mark - private


BLayoutItem* 
FontSelectionView::CreateSizesLabelLayoutItem()
{
	return fSizesMenuField->CreateLabelLayoutItem();
}


BLayoutItem* 
FontSelectionView::CreateSizesMenuBarLayoutItem()
{
	return fSizesMenuField->CreateMenuBarLayoutItem();
}


BLayoutItem* 
FontSelectionView::CreateFontsLabelLayoutItem()
{
	return fFontsMenuField->CreateLabelLayoutItem();
}


BLayoutItem* 
FontSelectionView::CreateFontsMenuBarLayoutItem()
{
	return fFontsMenuField->CreateMenuBarLayoutItem();
}


BLayoutItem* 
FontSelectionView::CreateStylesLabelLayoutItem()
{
	if (fStylesMenuField)
		return fStylesMenuField->CreateLabelLayoutItem();
	return NULL;
}


BLayoutItem* 
FontSelectionView::CreateStylesMenuBarLayoutItem()
{
	if (fStylesMenuField)
		return fStylesMenuField->CreateMenuBarLayoutItem();
	return NULL;
}


BView*
FontSelectionView::PreviewBox() const
{
	return fPreviewText;
}


// #pragma mark - private


BFont
FontSelectionView::_DefaultFont() const
{
	if (strcmp(Name(), "bold") == 0)
		return *be_bold_font;
	if (strcmp(Name(), "fixed") == 0)
		return *be_fixed_font;
	else
		return *be_plain_font;
}


void
FontSelectionView::_SelectCurrentFont(bool select)
{
	font_family family;
	font_style style;
	fCurrentFont.GetFamilyAndStyle(&family, &style);

	BMenuItem *item = fFontsMenu->FindItem(family);
	if (item != NULL) {
		item->SetMarked(select);

		if (item->Submenu() != NULL) {
			item = item->Submenu()->FindItem(style);
			if (item != NULL)
				item->SetMarked(select);
		}
	}
}


void
FontSelectionView::_SelectCurrentSize(bool select)
{
	char label[16];
	snprintf(label, sizeof(label), "%ld", (int32)fCurrentFont.Size());

	BMenuItem* item = fSizesMenu->FindItem(label);
	if (item != NULL)
		item->SetMarked(select);
}


void
FontSelectionView::_UpdateFontPreview()
{
	fPreviewText->SetFont(&fCurrentFont);
}


void
FontSelectionView::_BuildSizesMenu()
{
	const int32 sizes[] = {7, 8, 9, 10, 11, 12, 13, 14, 18, 21, 24, 0};

	// build size menu
	for (int32 i = 0; sizes[i]; i++) {
		int32 size = sizes[i];
		if (size < kMinSize || size > kMaxSize)
			continue;

		char label[32];
		snprintf(label, sizeof(label), "%ld", size);

		BMessage* message = new BMessage(kMsgSetSize);
		message->AddInt32("size", size);
		message->AddString("name", Name()); 

		BMenuItem* item = new BMenuItem(label, message);
		if (size == fCurrentFont.Size())
			item->SetMarked(true);

		fSizesMenu->AddItem(item);
		item->SetTarget(this);
	}
}


void
FontSelectionView::_AddStylesToMenu(const BFont& font, BMenu* stylesMenu) const
{
	stylesMenu->RemoveItems(0, stylesMenu->CountItems(), true);
	stylesMenu->SetRadioMode(true);

	font_family family;
	font_style style;
	font.GetFamilyAndStyle(&family, &style);
	BString currentStyle(style);

	int32 numStyles = count_font_styles(family);

	for (int32 j = 0; j < numStyles; j++) {
		if (get_font_style(family, j, &style) != B_OK)
			continue;

		BMessage* message = new BMessage(kMsgSetStyle);
		message->AddString("family", (char*)family);
		message->AddString("style", (char*)style);

		BMenuItem* item = new BMenuItem(style, message);
		item->SetMarked(currentStyle == style);

		stylesMenu->AddItem(item);
		item->SetTarget(this);
	}
}

