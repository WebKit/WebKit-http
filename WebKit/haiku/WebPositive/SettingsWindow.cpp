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

#include "BrowserApp.h"
#include "WebSettings.h"

#include <Button.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <stdio.h>

#include "SettingsMessage.h"


enum {
	MSG_OK			= 'aply',
	MSG_CANCEL		= 'cncl',
	MSG_REVERT		= 'rvrt',
};


SettingsWindow::SettingsWindow(BRect frame, SettingsMessage* settings)
	:
	BWindow(frame, "Settings", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fSettings(settings)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	fOkButton = new BButton("OK", new BMessage(MSG_OK));
	fCancelButton = new BButton("Cancel", new BMessage(MSG_CANCEL));
	fRevertButton = new BButton("Revert", new BMessage(MSG_REVERT));

	float spacing = be_control_look->DefaultItemSpacing();

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(BGridLayoutBuilder(spacing, spacing)
		)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fOkButton)
			.SetInsets(spacing, spacing, spacing, spacing)
		)
	);

	fOkButton->MakeDefault(true);

	if (!frame.IsValid())
		CenterOnScreen();

	// Start hidden
	Hide();
	Show();
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_OK:
		_ApplySettings();
		PostMessage(B_QUIT_REQUESTED);
		break;
	case MSG_CANCEL:
		_RevertSettings();
		PostMessage(B_QUIT_REQUESTED);
		break;
	case MSG_REVERT:
		_RevertSettings();
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


// #pragma mark - private


void
SettingsWindow::_ApplySettings()
{
}


void
SettingsWindow::_RevertSettings()
{
}


