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
#include "SettingsWindow.h"

#include <Button.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>
#include <TextControl.h>
#include <debugger.h>

#include <stdio.h>
#include <stdlib.h>

#include "BrowserApp.h"
#include "BrowsingHistory.h"
#include "BrowserWindow.h"
#include "FontSelectionView.h"
#include "SettingsKeys.h"
#include "SettingsMessage.h"
#include "WebSettings.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Settings Window"

enum {
	MSG_APPLY									= 'aply',
	MSG_CANCEL									= 'cncl',
	MSG_REVERT									= 'rvrt',

	MSG_START_PAGE_CHANGED						= 'hpch',
	MSG_SEARCH_PAGE_CHANGED						= 'spch',
	MSG_DOWNLOAD_FOLDER_CHANGED					= 'dnfc',
	MSG_NEW_WINDOWS_BEHAVIOR_CHANGED			= 'nwbc',
	MSG_NEW_TABS_BEHAVIOR_CHANGED				= 'ntbc',
	MSG_HISTORY_MENU_DAYS_CHANGED				= 'digm',
	MSG_TAB_DISPLAY_BEHAVIOR_CHANGED			= 'tdbc',
	MSG_AUTO_HIDE_INTERFACE_BEHAVIOR_CHANGED	= 'ahic',
	MSG_AUTO_HIDE_POINTER_BEHAVIOR_CHANGED		= 'ahpc',

	MSG_STANDARD_FONT_CHANGED					= 'stfc',
	MSG_SERIF_FONT_CHANGED						= 'sefc',
	MSG_SANS_SERIF_FONT_CHANGED					= 'ssfc',
	MSG_FIXED_FONT_CHANGED						= 'ffch',

	MSG_STANDARD_FONT_SIZE_SELECTED				= 'sfss',
	MSG_FIXED_FONT_SIZE_SELECTED				= 'ffss',

	MSG_USE_PROXY_CHANGED						= 'upsc',
	MSG_PROXY_ADDRESS_CHANGED					= 'psac',
	MSG_PROXY_PORT_CHANGED						= 'pspc',
};

static const int32 kDefaultFontSize = 14;


SettingsWindow::SettingsWindow(BRect frame, SettingsMessage* settings)
	:
	BWindow(frame, B_TRANSLATE("Settings"), B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_AUTO_UPDATE_SIZE_LIMITS
			| B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fSettings(settings)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(MSG_APPLY));
	fCancelButton = new BButton(B_TRANSLATE("Cancel"),
		new BMessage(MSG_CANCEL));
	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(MSG_REVERT));

	float spacing = be_control_look->DefaultItemSpacing();

	BTabView* tabView = new BTabView("settings pages", B_WIDTH_FROM_LABEL);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, spacing)
		.Add(tabView)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fApplyButton)
		)
		.SetInsets(spacing, spacing, spacing, spacing)
	);

	tabView->AddTab(_CreateGeneralPage(spacing));
	tabView->AddTab(_CreateFontsPage(spacing));
	tabView->AddTab(_CreateProxyPage(spacing));

	_SetupFontSelectionView(fStandardFontView,
		new BMessage(MSG_STANDARD_FONT_CHANGED));
	_SetupFontSelectionView(fSerifFontView,
		new BMessage(MSG_SERIF_FONT_CHANGED));
	_SetupFontSelectionView(fSansSerifFontView,
		new BMessage(MSG_SANS_SERIF_FONT_CHANGED));
	_SetupFontSelectionView(fFixedFontView,
		new BMessage(MSG_FIXED_FONT_CHANGED));

	fApplyButton->MakeDefault(true);

	if (!frame.IsValid())
		CenterOnScreen();

	// load settings from disk
	_RevertSettings();
	// apply to WebKit
	_ApplySettings();

	// Start hidden
	Hide();
	Show();
}


SettingsWindow::~SettingsWindow()
{
	RemoveHandler(fStandardFontView);
	delete fStandardFontView;
	RemoveHandler(fSerifFontView);
	delete fSerifFontView;
	RemoveHandler(fSansSerifFontView);
	delete fSansSerifFontView;
	RemoveHandler(fFixedFontView);
	delete fFixedFontView;
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_APPLY:
			_ApplySettings();
			break;
		case MSG_CANCEL:
			_RevertSettings();
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_REVERT:
			_RevertSettings();
			break;

		case MSG_STANDARD_FONT_SIZE_SELECTED:
		{
			int32 size = _SizesMenuValue(fStandardSizesMenu->Menu());
			fStandardFontView->SetSize(size);
			fSerifFontView->SetSize(size);
			fSansSerifFontView->SetSize(size);
			_ValidateControlsEnabledStatus();
			break;
		}
		case MSG_FIXED_FONT_SIZE_SELECTED:
		{
			int32 size = _SizesMenuValue(fFixedSizesMenu->Menu());
			fFixedFontView->SetSize(size);
			_ValidateControlsEnabledStatus();
			break;
		}

		case MSG_START_PAGE_CHANGED:
		case MSG_SEARCH_PAGE_CHANGED:
		case MSG_DOWNLOAD_FOLDER_CHANGED:
		case MSG_NEW_WINDOWS_BEHAVIOR_CHANGED:
		case MSG_NEW_TABS_BEHAVIOR_CHANGED:
		case MSG_HISTORY_MENU_DAYS_CHANGED:
		case MSG_TAB_DISPLAY_BEHAVIOR_CHANGED:
		case MSG_AUTO_HIDE_INTERFACE_BEHAVIOR_CHANGED:
		case MSG_AUTO_HIDE_POINTER_BEHAVIOR_CHANGED:
		case MSG_STANDARD_FONT_CHANGED:
		case MSG_SERIF_FONT_CHANGED:
		case MSG_SANS_SERIF_FONT_CHANGED:
		case MSG_FIXED_FONT_CHANGED:
		case MSG_USE_PROXY_CHANGED:
		case MSG_PROXY_ADDRESS_CHANGED:
		case MSG_PROXY_PORT_CHANGED:
			// TODO: Some settings could change live, some others not?
			_ValidateControlsEnabledStatus();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
SettingsWindow::QuitRequested()
{
	if (!IsHidden())
		Hide();
	return false;
}


void
SettingsWindow::Show()
{
	// When showing the window, the this is always the
	// point to which we can revert the settings.
	_RevertSettings();
	BWindow::Show();
}


// #pragma mark - private


BView*
SettingsWindow::_CreateGeneralPage(float spacing)
{
	fStartPageControl = new BTextControl("start page",
		B_TRANSLATE("Start page:"), "", new BMessage(MSG_START_PAGE_CHANGED));
	fStartPageControl->SetModificationMessage(
		new BMessage(MSG_START_PAGE_CHANGED));
	fStartPageControl->SetText(
		fSettings->GetValue(kSettingsKeyStartPageURL, kDefaultStartPageURL));

	fSearchPageControl = new BTextControl("search page",
		B_TRANSLATE("Search page:"), "",
		new BMessage(MSG_SEARCH_PAGE_CHANGED));
	fSearchPageControl->SetModificationMessage(
		new BMessage(MSG_SEARCH_PAGE_CHANGED));
	fSearchPageControl->SetText(
		fSettings->GetValue(kSettingsKeySearchPageURL, kDefaultSearchPageURL));

	fDownloadFolderControl = new BTextControl("download folder",
		B_TRANSLATE("Download folder:"), "",
		new BMessage(MSG_DOWNLOAD_FOLDER_CHANGED));
	fDownloadFolderControl->SetModificationMessage(
		new BMessage(MSG_DOWNLOAD_FOLDER_CHANGED));
	fDownloadFolderControl->SetText(
		fSettings->GetValue(kSettingsKeyDownloadPath, kDefaultDownloadPath));

	fNewWindowBehaviorOpenHomeItem = new BMenuItem(
		B_TRANSLATE("Open start page"),
		new BMessage(MSG_NEW_WINDOWS_BEHAVIOR_CHANGED));
	fNewWindowBehaviorOpenSearchItem = new BMenuItem(
		B_TRANSLATE("Open search page"),
		new BMessage(MSG_NEW_WINDOWS_BEHAVIOR_CHANGED));
	fNewWindowBehaviorOpenBlankItem = new BMenuItem(
		B_TRANSLATE("Open blank page"),
		new BMessage(MSG_NEW_WINDOWS_BEHAVIOR_CHANGED));

	fNewTabBehaviorCloneCurrentItem = new BMenuItem(
		B_TRANSLATE("Clone current page"),
		new BMessage(MSG_NEW_TABS_BEHAVIOR_CHANGED));
	fNewTabBehaviorOpenHomeItem = new BMenuItem(
		B_TRANSLATE("Open start page"),
		new BMessage(MSG_NEW_TABS_BEHAVIOR_CHANGED));
	fNewTabBehaviorOpenSearchItem = new BMenuItem(
		B_TRANSLATE("Open search page"),
		new BMessage(MSG_NEW_TABS_BEHAVIOR_CHANGED));
	fNewTabBehaviorOpenBlankItem = new BMenuItem(
		B_TRANSLATE("Open blank page"),
		new BMessage(MSG_NEW_TABS_BEHAVIOR_CHANGED));

	fNewWindowBehaviorOpenHomeItem->SetMarked(true);
	fNewTabBehaviorOpenBlankItem->SetMarked(true);

	BPopUpMenu* newWindowBehaviorMenu = new BPopUpMenu("New windows");
	newWindowBehaviorMenu->AddItem(fNewWindowBehaviorOpenHomeItem);
	newWindowBehaviorMenu->AddItem(fNewWindowBehaviorOpenSearchItem);
	newWindowBehaviorMenu->AddItem(fNewWindowBehaviorOpenBlankItem);
	fNewWindowBehaviorMenu = new BMenuField("new window behavior",
		B_TRANSLATE("New windows:"), newWindowBehaviorMenu);

	BPopUpMenu* newTabBehaviorMenu = new BPopUpMenu("New tabs");
	newTabBehaviorMenu->AddItem(fNewTabBehaviorOpenBlankItem);
	newTabBehaviorMenu->AddItem(fNewTabBehaviorOpenHomeItem);
	newTabBehaviorMenu->AddItem(fNewTabBehaviorOpenSearchItem);
	newTabBehaviorMenu->AddItem(fNewTabBehaviorCloneCurrentItem);
	fNewTabBehaviorMenu = new BMenuField("new tab behavior",
		B_TRANSLATE("New tabs:"), newTabBehaviorMenu);

	fDaysInHistoryMenuControl = new BTextControl("days in history",
		B_TRANSLATE("Number of days to keep links in History menu:"), "",
		new BMessage(MSG_HISTORY_MENU_DAYS_CHANGED));
	fDaysInHistoryMenuControl->SetModificationMessage(
		new BMessage(MSG_HISTORY_MENU_DAYS_CHANGED));
	BString maxHistoryAge;
	maxHistoryAge << BrowsingHistory::DefaultInstance()->MaxHistoryItemAge();
	fDaysInHistoryMenuControl->SetText(maxHistoryAge.String());
	for (uchar i = 0; i < '0'; i++)
		fDaysInHistoryMenuControl->TextView()->DisallowChar(i);
	for (uchar i = '9' + 1; i <= 128; i++)
		fDaysInHistoryMenuControl->TextView()->DisallowChar(i);

	fShowTabsIfOnlyOnePage = new BCheckBox("show tabs if only one page",
		B_TRANSLATE("Show tabs if only one page is open."),
		new BMessage(MSG_TAB_DISPLAY_BEHAVIOR_CHANGED));
	fShowTabsIfOnlyOnePage->SetValue(B_CONTROL_ON);

	fAutoHideInterfaceInFullscreenMode = new BCheckBox("auto-hide interface",
		B_TRANSLATE("Auto-hide interface in fullscreen mode."),
		new BMessage(MSG_AUTO_HIDE_INTERFACE_BEHAVIOR_CHANGED));
	fAutoHideInterfaceInFullscreenMode->SetValue(B_CONTROL_OFF);

	fAutoHidePointer = new BCheckBox("auto-hide pointer",
		B_TRANSLATE("Auto-hide mouse pointer."),
		new BMessage(MSG_AUTO_HIDE_POINTER_BEHAVIOR_CHANGED));
	fAutoHidePointer->SetValue(B_CONTROL_OFF);

	BView* view = BGroupLayoutBuilder(B_VERTICAL, spacing / 2)
		.Add(BGridLayoutBuilder(spacing / 2, spacing / 2)
			.Add(fStartPageControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fStartPageControl->CreateTextViewLayoutItem(), 1, 0)

			.Add(fSearchPageControl->CreateLabelLayoutItem(), 0, 1)
			.Add(fSearchPageControl->CreateTextViewLayoutItem(), 1, 1)

			.Add(fDownloadFolderControl->CreateLabelLayoutItem(), 0, 2)
			.Add(fDownloadFolderControl->CreateTextViewLayoutItem(), 1, 2)

			.Add(fNewWindowBehaviorMenu->CreateLabelLayoutItem(), 0, 3)
			.Add(fNewWindowBehaviorMenu->CreateMenuBarLayoutItem(), 1, 3)

			.Add(fNewTabBehaviorMenu->CreateLabelLayoutItem(), 0, 4)
			.Add(fNewTabBehaviorMenu->CreateMenuBarLayoutItem(), 1, 4)
		)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing))
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing))
		.Add(fShowTabsIfOnlyOnePage)
		.Add(fAutoHideInterfaceInFullscreenMode)
		.Add(fAutoHidePointer)
		.Add(fDaysInHistoryMenuControl)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing))

		.SetInsets(spacing, spacing, spacing, spacing)

		.TopView()
	;
	view->SetName(B_TRANSLATE("General"));
	return view;
}


BView*
SettingsWindow::_CreateFontsPage(float spacing)
{
	fStandardFontView = new FontSelectionView("standard",
		B_TRANSLATE("Standard font:"), true, be_plain_font);
	BFont defaultSerifFont = _FindDefaultSerifFont();
	fSerifFontView = new FontSelectionView("serif",
		B_TRANSLATE("Serif font:"), true, &defaultSerifFont);
	fSansSerifFontView = new FontSelectionView("sans serif",
		B_TRANSLATE("Sans serif font:"), true, be_plain_font);
	fFixedFontView = new FontSelectionView("fixed",
		B_TRANSLATE("Fixed font:"), true, be_fixed_font);

	fStandardSizesMenu =  new BMenuField("standard font size",
		B_TRANSLATE("Default standard font size:"), new BPopUpMenu("sizes"),
		NULL);
	_BuildSizesMenu(fStandardSizesMenu->Menu(),
		MSG_STANDARD_FONT_SIZE_SELECTED);

	fFixedSizesMenu =  new BMenuField("fixed font size",
		B_TRANSLATE("Default fixed font size:"), new BPopUpMenu("sizes"),
		NULL);
	_BuildSizesMenu(fFixedSizesMenu->Menu(), MSG_FIXED_FONT_SIZE_SELECTED);

	BView* view = BGridLayoutBuilder(spacing / 2, spacing / 2)
		.Add(fStandardFontView->CreateFontsLabelLayoutItem(), 0, 0)
		.Add(fStandardFontView->CreateFontsMenuBarLayoutItem(), 1, 0)
		.Add(fStandardFontView->PreviewBox(), 0, 1, 2)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing), 0, 2, 2)

		.Add(fSerifFontView->CreateFontsLabelLayoutItem(), 0, 3)
		.Add(fSerifFontView->CreateFontsMenuBarLayoutItem(), 1, 3)
		.Add(fSerifFontView->PreviewBox(), 0, 4, 2)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing), 0, 5, 2)

		.Add(fSansSerifFontView->CreateFontsLabelLayoutItem(), 0, 6)
		.Add(fSansSerifFontView->CreateFontsMenuBarLayoutItem(), 1, 6)
		.Add(fSansSerifFontView->PreviewBox(), 0, 7, 2)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing), 0, 8, 2)

		.Add(fFixedFontView->CreateFontsLabelLayoutItem(), 0, 9)
		.Add(fFixedFontView->CreateFontsMenuBarLayoutItem(), 1, 9)
		.Add(fFixedFontView->PreviewBox(), 0, 10, 2)
		.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing), 0, 11, 2)

		.Add(fStandardSizesMenu->CreateLabelLayoutItem(), 0, 12)
		.Add(fStandardSizesMenu->CreateMenuBarLayoutItem(), 1, 12)
		.Add(fFixedSizesMenu->CreateLabelLayoutItem(), 0, 13)
		.Add(fFixedSizesMenu->CreateMenuBarLayoutItem(), 1, 13)

		.SetInsets(spacing, spacing, spacing, spacing)

		.View()
	;
	view->SetName(B_TRANSLATE("Fonts"));
	return view;
}


BView*
SettingsWindow::_CreateProxyPage(float spacing)
{
	fUseProxyCheckBox = new BCheckBox("use proxy",
		B_TRANSLATE("Use proxy server to connect to the internet."),
		new BMessage(MSG_USE_PROXY_CHANGED));
	fUseProxyCheckBox->SetValue(B_CONTROL_ON);

	fProxyAddressControl = new BTextControl("proxy address",
		B_TRANSLATE("Proxy server address:"), "",
		new BMessage(MSG_PROXY_ADDRESS_CHANGED));
	fProxyAddressControl->SetModificationMessage(
		new BMessage(MSG_PROXY_ADDRESS_CHANGED));
	fProxyAddressControl->SetText(
		fSettings->GetValue(kSettingsKeyProxyAddress, ""));

	fProxyPortControl = new BTextControl("proxy port",
		B_TRANSLATE("Proxy server port:"), "",
		new BMessage(MSG_PROXY_PORT_CHANGED));
	fProxyPortControl->SetModificationMessage(
		new BMessage(MSG_PROXY_PORT_CHANGED));
	fProxyPortControl->SetText(
		fSettings->GetValue(kSettingsKeyProxyAddress, ""));

	BView* view = BGroupLayoutBuilder(B_VERTICAL, spacing / 2)
		.Add(fUseProxyCheckBox)
		.Add(BGridLayoutBuilder(spacing / 2, spacing / 2)
			.Add(fProxyAddressControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fProxyAddressControl->CreateTextViewLayoutItem(), 1, 0)
		
			.Add(fProxyPortControl->CreateLabelLayoutItem(), 0, 1)
			.Add(fProxyPortControl->CreateTextViewLayoutItem(), 1, 1)
		)
		.Add(BSpaceLayoutItem::CreateGlue())

		.SetInsets(spacing, spacing, spacing, spacing)

		.TopView()
	;
	view->SetName(B_TRANSLATE("Proxy server"));
	return view;
}


void
SettingsWindow::_BuildSizesMenu(BMenu* menu, uint32 messageWhat)
{
	const float kMinSize = 8.0;
	const float kMaxSize = 18.0;

	const int32 kSizes[] = {7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 21, 24, 0};

	for (int32 i = 0; kSizes[i]; i++) {
		int32 size = kSizes[i];
		if (size < kMinSize || size > kMaxSize)
			continue;

		char label[32];
		snprintf(label, sizeof(label), "%ld", size);

		BMessage* message = new BMessage(messageWhat);
		message->AddInt32("size", size);

		BMenuItem* item = new BMenuItem(label, message);

		menu->AddItem(item);
		item->SetTarget(this);
	}
}


void
SettingsWindow::_SetupFontSelectionView(FontSelectionView* view,
	BMessage* message)
{
	AddHandler(view);
	view->AttachedToLooper();
	view->SetMessage(message);
	view->SetTarget(this);
}


// #pragma mark -


bool
SettingsWindow::_CanApplySettings() const
{
	bool canApply = false;

	// General settings
	canApply = canApply || (strcmp(fStartPageControl->Text(),
		fSettings->GetValue(kSettingsKeyStartPageURL,
			kDefaultStartPageURL)) != 0);

	canApply = canApply || (strcmp(fSearchPageControl->Text(),
		fSettings->GetValue(kSettingsKeySearchPageURL,
			kDefaultSearchPageURL)) != 0);

	canApply = canApply || (strcmp(fDownloadFolderControl->Text(),
		fSettings->GetValue(kSettingsKeyDownloadPath,
			kDefaultDownloadPath)) != 0);

	canApply = canApply || ((fShowTabsIfOnlyOnePage->Value() == B_CONTROL_ON)
		!= fSettings->GetValue(kSettingsKeyShowTabsIfSinglePageOpen, true));

	canApply = canApply || (
		(fAutoHideInterfaceInFullscreenMode->Value() == B_CONTROL_ON)
		!= fSettings->GetValue(kSettingsKeyAutoHideInterfaceInFullscreenMode,
			false));

	canApply = canApply || (
		(fAutoHidePointer->Value() == B_CONTROL_ON)
		!= fSettings->GetValue(kSettingsKeyAutoHidePointer, false));

	canApply = canApply || (_MaxHistoryAge()
		!= BrowsingHistory::DefaultInstance()->MaxHistoryItemAge());

	// New window policy
	canApply = canApply || (_NewWindowPolicy()
		!= fSettings->GetValue(kSettingsKeyNewWindowPolicy,
			(uint32)OpenStartPage));

	// New tab policy
	canApply = canApply || (_NewTabPolicy()
		!= fSettings->GetValue(kSettingsKeyNewTabPolicy,
			(uint32)OpenBlankPage));

	// Font settings
	canApply = canApply || (fStandardFontView->Font()
		!= fSettings->GetValue("standard font", *be_plain_font));

	canApply = canApply || (fSerifFontView->Font()
		!= fSettings->GetValue("serif font", _FindDefaultSerifFont()));

	canApply = canApply || (fSansSerifFontView->Font()
		!= fSettings->GetValue("sans serif font", *be_plain_font));

	canApply = canApply || (fFixedFontView->Font()
		!= fSettings->GetValue("fixed font", *be_fixed_font));

	canApply = canApply || (_SizesMenuValue(fStandardSizesMenu->Menu())
		!= fSettings->GetValue("standard font size", kDefaultFontSize));

	canApply = canApply || (_SizesMenuValue(fFixedSizesMenu->Menu())
		!= fSettings->GetValue("fixed font size", kDefaultFontSize));

	// Proxy settings
	canApply = canApply || ((fUseProxyCheckBox->Value() == B_CONTROL_ON)
		!= fSettings->GetValue(kSettingsKeyUseProxy, false));

	canApply = canApply || (strcmp(fProxyAddressControl->Text(),
		fSettings->GetValue(kSettingsKeyProxyAddress, "")) != 0);

	canApply = canApply || (_ProxyPort()
		!= fSettings->GetValue(kSettingsKeyProxyPort, (uint32)0));

	return canApply;
}


void
SettingsWindow::_ApplySettings()
{
	// Store general settings
	int32 maxHistoryAge = _MaxHistoryAge();
	BString text;
	text << maxHistoryAge;
	fDaysInHistoryMenuControl->SetText(text.String());
	BrowsingHistory::DefaultInstance()->SetMaxHistoryItemAge(maxHistoryAge);

	fSettings->SetValue(kSettingsKeyStartPageURL, fStartPageControl->Text());
	fSettings->SetValue(kSettingsKeySearchPageURL, fSearchPageControl->Text());
	fSettings->SetValue(kSettingsKeyDownloadPath, fDownloadFolderControl->Text());
	fSettings->SetValue(kSettingsKeyShowTabsIfSinglePageOpen,
		fShowTabsIfOnlyOnePage->Value() == B_CONTROL_ON);
	fSettings->SetValue(kSettingsKeyAutoHideInterfaceInFullscreenMode,
		fAutoHideInterfaceInFullscreenMode->Value() == B_CONTROL_ON);
	fSettings->SetValue(kSettingsKeyAutoHidePointer,
		fAutoHidePointer->Value() == B_CONTROL_ON);

	// New page policies
	fSettings->SetValue(kSettingsKeyNewWindowPolicy, _NewWindowPolicy());
	fSettings->SetValue(kSettingsKeyNewTabPolicy, _NewTabPolicy());

	// Store fond settings
	fSettings->SetValue("standard font", fStandardFontView->Font());
	fSettings->SetValue("serif font", fSerifFontView->Font());
	fSettings->SetValue("sans serif font", fSansSerifFontView->Font());
	fSettings->SetValue("fixed font", fFixedFontView->Font());
	int32 standardFontSize = _SizesMenuValue(fStandardSizesMenu->Menu());
	int32 fixedFontSize = _SizesMenuValue(fFixedSizesMenu->Menu());
	fSettings->SetValue("standard font size", standardFontSize);
	fSettings->SetValue("fixed font size", fixedFontSize);

	// Store proxy settings

	fSettings->SetValue(kSettingsKeyUseProxy,
		fUseProxyCheckBox->Value() == B_CONTROL_ON);
	fSettings->SetValue(kSettingsKeyProxyAddress,
		fProxyAddressControl->Text());
	uint32 proxyPort = _ProxyPort();
	fSettings->SetValue(kSettingsKeyProxyPort, proxyPort);

	fSettings->Save();

	// Apply settings to default web page settings.
	BWebSettings::Default()->SetStandardFont(fStandardFontView->Font());
	BWebSettings::Default()->SetSerifFont(fSerifFontView->Font());
	BWebSettings::Default()->SetSansSerifFont(fSansSerifFontView->Font());
	BWebSettings::Default()->SetFixedFont(fFixedFontView->Font());
	BWebSettings::Default()->SetDefaultStandardFontSize(standardFontSize);
	BWebSettings::Default()->SetDefaultFixedFontSize(fixedFontSize);

	if (fUseProxyCheckBox->Value() == B_CONTROL_ON) {
		BWebSettings::Default()->SetProxyInfo(fProxyAddressControl->Text(),
			proxyPort, B_PROXY_TYPE_HTTP, "", "");
	} else
		BWebSettings::Default()->SetProxyInfo();

	// This will find all currently instantiated page settings and apply
	// the default values, unless the page settings have local overrides.
	BWebSettings::Default()->Apply();


	_ValidateControlsEnabledStatus();
}


void
SettingsWindow::_RevertSettings()
{
	fStartPageControl->SetText(
		fSettings->GetValue(kSettingsKeyStartPageURL, kDefaultStartPageURL));

	fSearchPageControl->SetText(
		fSettings->GetValue(kSettingsKeySearchPageURL, kDefaultSearchPageURL));

	fDownloadFolderControl->SetText(
		fSettings->GetValue(kSettingsKeyDownloadPath, kDefaultDownloadPath));
	fShowTabsIfOnlyOnePage->SetValue(
		fSettings->GetValue(kSettingsKeyShowTabsIfSinglePageOpen, true));
	fAutoHideInterfaceInFullscreenMode->SetValue(
		fSettings->GetValue(kSettingsKeyAutoHideInterfaceInFullscreenMode,
			false));
	fAutoHidePointer->SetValue(
		fSettings->GetValue(kSettingsKeyAutoHidePointer, false));

	BString text;
	text << BrowsingHistory::DefaultInstance()->MaxHistoryItemAge();
	fDaysInHistoryMenuControl->SetText(text.String());

	// New window policy
	uint32 newWindowPolicy = fSettings->GetValue(kSettingsKeyNewWindowPolicy,
		(uint32)OpenStartPage);
	switch (newWindowPolicy) {
		default:
		case OpenStartPage:
			fNewWindowBehaviorOpenHomeItem->SetMarked(true);
			break;
		case OpenSearchPage:
			fNewWindowBehaviorOpenSearchItem->SetMarked(true);
			break;
		case OpenBlankPage:
			fNewWindowBehaviorOpenBlankItem->SetMarked(true);
			break;
	}

	// New tab policy
	uint32 newTabPolicy = fSettings->GetValue(kSettingsKeyNewTabPolicy,
		(uint32)OpenBlankPage);
	switch (newTabPolicy) {
		default:
		case OpenBlankPage:
			fNewTabBehaviorOpenBlankItem->SetMarked(true);
			break;
		case OpenStartPage:
			fNewTabBehaviorOpenHomeItem->SetMarked(true);
			break;
		case OpenSearchPage:
			fNewTabBehaviorOpenSearchItem->SetMarked(true);
			break;
		case CloneCurrentPage:
			fNewTabBehaviorCloneCurrentItem->SetMarked(true);
			break;
	}

	// Font settings
	int32 defaultFontSize = fSettings->GetValue("standard font size",
		kDefaultFontSize);
	int32 defaultFixedFontSize = fSettings->GetValue("fixed font size",
		kDefaultFontSize);

	_SetSizesMenuValue(fStandardSizesMenu->Menu(), defaultFontSize);
	_SetSizesMenuValue(fFixedSizesMenu->Menu(), defaultFixedFontSize);

	fStandardFontView->SetFont(fSettings->GetValue("standard font",
		*be_plain_font), defaultFontSize);
	fSerifFontView->SetFont(fSettings->GetValue("serif font",
		_FindDefaultSerifFont()), defaultFontSize);
	fSansSerifFontView->SetFont(fSettings->GetValue("sans serif font",
		*be_plain_font), defaultFontSize);
	fFixedFontView->SetFont(fSettings->GetValue("fixed font",
		*be_fixed_font), defaultFixedFontSize);

	// Proxy settings
	fUseProxyCheckBox->SetValue(fSettings->GetValue(kSettingsKeyUseProxy,
		false));
	fProxyAddressControl->SetText(fSettings->GetValue(kSettingsKeyProxyAddress,
		""));
	text = "";
	text << fSettings->GetValue(kSettingsKeyProxyPort, (uint32)0);
	fProxyPortControl->SetText(text.String());

	_ValidateControlsEnabledStatus();
}


void
SettingsWindow::_ValidateControlsEnabledStatus()
{
	bool canApply = _CanApplySettings();
	fApplyButton->SetEnabled(canApply);
	fRevertButton->SetEnabled(canApply);
	// Let the Cancel button be enabled always, as another way to close the
	// window...
	fCancelButton->SetEnabled(true);

	bool useProxy = fUseProxyCheckBox->Value() == B_CONTROL_ON;
	fProxyAddressControl->SetEnabled(useProxy);
	fProxyPortControl->SetEnabled(useProxy);
}


// #pragma mark -


uint32
SettingsWindow::_NewWindowPolicy() const
{
	uint32 newWindowPolicy = OpenStartPage;
	BMenuItem* markedItem = fNewWindowBehaviorMenu->Menu()->FindMarked();
	if (markedItem == fNewWindowBehaviorOpenSearchItem)
		newWindowPolicy = OpenSearchPage;
	else if (markedItem == fNewWindowBehaviorOpenBlankItem)
		newWindowPolicy = OpenBlankPage;
	return newWindowPolicy;
}


uint32
SettingsWindow::_NewTabPolicy() const
{
	uint32 newTabPolicy = OpenBlankPage;
	BMenuItem* markedItem = fNewTabBehaviorMenu->Menu()->FindMarked();
	if (markedItem == fNewTabBehaviorCloneCurrentItem)
		newTabPolicy = CloneCurrentPage;
	else if (markedItem == fNewTabBehaviorOpenHomeItem)
		newTabPolicy = OpenStartPage;
	else if (markedItem == fNewTabBehaviorOpenSearchItem)
		newTabPolicy = OpenSearchPage;
	return newTabPolicy;
}


int32
SettingsWindow::_MaxHistoryAge() const
{
	int32 maxHistoryAge = atoi(fDaysInHistoryMenuControl->Text());
	if (maxHistoryAge <= 0)
		maxHistoryAge = 1;
	if (maxHistoryAge >= 35)
		maxHistoryAge = 35;
	return maxHistoryAge;
}


void
SettingsWindow::_SetSizesMenuValue(BMenu* menu, int32 value)
{
	for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
		bool marked = false;
		if (BMessage* message = item->Message()) {
			int32 size;
			if (message->FindInt32("size", &size) == B_OK && size == value)
				marked = true;
		}
		item->SetMarked(marked);
	}
}


int32
SettingsWindow::_SizesMenuValue(BMenu* menu) const
{
	if (BMenuItem* item = menu->FindMarked()) {
		if (BMessage* message = item->Message()) {
			int32 size;
			if (message->FindInt32("size", &size) == B_OK)
				return size;
		}
	}
	return kDefaultFontSize;
}


BFont
SettingsWindow::_FindDefaultSerifFont() const
{
	// Default to the first "serif" font we find.
	BFont serifFont(*be_plain_font);
	font_family family;
	int32 familyCount = count_font_families();
	for (int32 i = 0; i < familyCount; i++) {
		if (get_font_family(i, &family) == B_OK) {
			BString familyString(family);
			if (familyString.IFindFirst("sans") >= 0)
				continue;
			if (familyString.IFindFirst("serif") >= 0) {
				serifFont.SetFamilyAndFace(family, B_REGULAR_FACE);
				break;
			}
		}
	}
	return serifFont;
}


uint32
SettingsWindow::_ProxyPort() const
{
	return atoul(fProxyPortControl->Text());
}


