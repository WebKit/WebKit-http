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
#ifndef FONT_SELECTION_VIEW_H
#define FONT_SELECTION_VIEW_H


#include <Font.h>
#include <Handler.h>

class BLayoutItem;
class BBox;
class BMenu;
class BMenuField;
class BPopUpMenu;
class BStringView;
class BView;


class FontSelectionView : public BHandler {
public:
								FontSelectionView(const char* name,
									const char* label, bool separateStyles,
									const BFont* font = NULL);
	virtual						~FontSelectionView();

			void				AttachedToLooper();
	virtual void				MessageReceived(BMessage* message);

			void				SetFont(const BFont& font, float size);
			void				SetFont(const BFont& font);
			void				SetSize(float size);
			const BFont&		Font() const;

			void				SetDefaults();
			void				Revert();
			bool				IsDefaultable();
			bool				IsRevertable();

			void				UpdateFontsMenu();

			BLayoutItem*	 	CreateSizesLabelLayoutItem();
			BLayoutItem*		CreateSizesMenuBarLayoutItem();

			BLayoutItem* 		CreateFontsLabelLayoutItem();
			BLayoutItem*		CreateFontsMenuBarLayoutItem();

			BLayoutItem* 		CreateStylesLabelLayoutItem();
			BLayoutItem*		CreateStylesMenuBarLayoutItem();

			BView*				PreviewBox() const;
			
private:
			BFont				_DefaultFont() const;
			void				_SelectCurrentFont(bool select);
			void				_SelectCurrentSize(bool select);
			void				_UpdateFontPreview();
			void				_BuildSizesMenu();
			void				_AddStylesToMenu(const BFont& font,
									BMenu* menu) const;

protected:
			BMenuField*			fFontsMenuField;
			BMenuField*			fStylesMenuField;
			BMenuField*			fSizesMenuField;
			BPopUpMenu*			fFontsMenu;
			BPopUpMenu*			fStylesMenu;
			BPopUpMenu*			fSizesMenu;
			BStringView*		fPreviewText;

			BFont				fSavedFont;
			BFont				fCurrentFont;
};

#endif	// FONT_SELECTION_VIEW_H
