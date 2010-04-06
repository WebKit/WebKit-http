/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Window.h>

class BButton;
class BCheckBox;
class BMenu;
class BMenuField;
class BMenuItem;
class BTextControl;
class FontSelectionView;
class SettingsMessage;


class SettingsWindow : public BWindow {
public:
								SettingsWindow(BRect frame,
									SettingsMessage* settings);
	virtual						~SettingsWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	virtual	void				Show();

private:
			BView*				_CreateGeneralPage(float spacing);
			BView*				_CreateFontsPage(float spacing);

			void				_ApplySettings();
			void				_RevertSettings();

			void				_BuildSizesMenu(BMenu* menu,
									uint32 messageWhat);
			void				_SetSizesMenuValue(BMenu* menu, int32 value);
			int32				_SizesMenuValue(BMenu* menu);

			BFont				_FindDefaultSerifFont() const;

private:
			SettingsMessage*	fSettings;

			BTextControl*		fDownloadFolderControl;
			BMenuField*			fNewPageBehaviorMenu;
			BMenuItem*			fNewPageBehaviorCloneCurrentItem;
			BMenuItem*			fNewPageBehaviorOpenHomeItem;
			BMenuItem*			fNewPageBehaviorOpenSearchItem;
			BMenuItem*			fNewPageBehaviorOpenBlankItem;
			BTextControl*		fDaysInGoMenuControl;
			BCheckBox*			fShowTabsIfOnlyOnePage;

			FontSelectionView*	fStandardFontView;
			FontSelectionView*	fSerifFontView;
			FontSelectionView*	fSansSerifFontView;
			FontSelectionView*	fFixedFontView;

			BButton*			fApplyButton;
			BButton*			fCancelButton;
			BButton*			fRevertButton;

			BMenuField*			fStandardSizesMenu;
			BMenuField*			fFixedSizesMenu;
};


#endif // SETTINGS_WINDOW_H

