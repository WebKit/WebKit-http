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

#include "AuthenticationPanel.h"

#include <Button.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <Screen.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <TextControl.h>
#include <stdio.h>

static const uint32 kMsgPanelOK = 'pnok';
static const uint32 kMsgJitter = 'jitr';
static const uint32 kHidePassword = 'hdpw';

AuthenticationPanel::AuthenticationPanel(BRect parentFrame)
	: BWindow(BRect(-1000, -1000, -900, -900), "Authentication Required",
	    B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
	    B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	        | B_CLOSE_ON_ESCAPE | B_AUTO_UPDATE_SIZE_LIMITS)
	, m_parentWindowFrame(parentFrame)
	, m_usernameTextControl(new BTextControl("user", "Username:", "", NULL))
	, m_passwordTextControl(new BTextControl("pass", "Password:", "", NULL))
	, m_hidePasswordCheckBox(new BCheckBox("hide", "Hide password text", new BMessage(kHidePassword)))
	, m_remberCredentialsCheckBox(new BCheckBox("remember", "Rember this username and password", NULL))
	, m_okButton(new BButton("ok", "OK", new BMessage(kMsgPanelOK)))
	, m_cancelButton(new BButton("cancel", "Cancel", new BMessage(B_QUIT_REQUESTED)))
	, m_cancelled(false)
	, m_exitSemaphore(create_sem(0, "Authentication Panel"))
{
}

AuthenticationPanel::~AuthenticationPanel()
{
	delete_sem(m_exitSemaphore);
}

bool
AuthenticationPanel::QuitRequested()
{
	m_cancelled = true;
	release_sem(m_exitSemaphore);
	return false;
}

void
AuthenticationPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case kMsgPanelOK:
		release_sem(m_exitSemaphore);
		break;
    case kHidePassword: {
    	// TODO: Toggling this is broken in BTextView. Workaround is to
    	// set the text and selection again.
        BString text = m_passwordTextControl->Text();
        int32 selectionStart;
        int32 selectionEnd;
        m_passwordTextControl->TextView()->GetSelection(&selectionStart, &selectionEnd);
        m_passwordTextControl->TextView()->HideTyping(
            m_hidePasswordCheckBox->Value() == B_CONTROL_ON);
        m_passwordTextControl->SetText(text.String());
        m_passwordTextControl->TextView()->Select(selectionStart, selectionEnd);
        break;
    }
    case kMsgJitter: {
    	UpdateIfNeeded();
		BPoint leftTop = Frame().LeftTop();
		const float jitterOffsets[] = { -10, 0, 10, 0 };
		const int32 jitterOffsetCount = sizeof(jitterOffsets) / sizeof(float);
		for (int32 i = 0; i < 20; i++) {
			float offset = jitterOffsets[i % jitterOffsetCount];
			MoveTo(leftTop.x + offset, leftTop.y);
			snooze(15000);
		}
		MoveTo(leftTop);
		break;
    }
	default:
		BWindow::MessageReceived(message);
	}
}

bool AuthenticationPanel::getAuthentication(const BString& realm,
    const BString& method, const BString& previousUser, const BString& previousPass,
	bool previousRememberCredentials, bool badPassword,
	BString& user, BString&  pass, bool* rememberCredentials)
{
	// Configure panel and layout controls.
	BString infoText("Enter login information for: ");
	infoText << realm << "\n\n";
	infoText << "Authentication method: ";
	infoText << method;

    m_usernameTextControl->SetText(previousUser.String());
	m_passwordTextControl->TextView()->HideTyping(true);
	// Ignore the previous password, if it didn't work.
	if (!badPassword)
	    m_passwordTextControl->SetText(previousPass.String());
	m_hidePasswordCheckBox->SetValue(B_CONTROL_ON);
	m_remberCredentialsCheckBox->SetValue(previousRememberCredentials);

	// create layout
	SetLayout(new BGroupLayout(B_VERTICAL));
	float spacing = be_control_look->DefaultItemSpacing();
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
	    // TODO: Add text view here explaining realm and method...
	    .Add(BGridLayoutBuilder(0, spacing)
	        .Add(m_usernameTextControl->CreateLabelLayoutItem(), 0, 0)
	        .Add(m_usernameTextControl->CreateTextViewLayoutItem(), 1, 0)
	        .Add(m_passwordTextControl->CreateLabelLayoutItem(), 0, 1)
	        .Add(m_passwordTextControl->CreateTextViewLayoutItem(), 1, 1)
	        .Add(BSpaceLayoutItem::CreateGlue(), 0, 2)
	        .Add(m_hidePasswordCheckBox, 1, 2)
	        .Add(m_remberCredentialsCheckBox, 0, 3, 2)
	        .SetInsets(spacing, spacing, spacing, spacing)
	    )
	    .Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	    .Add(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
	        .AddGlue()
	        .Add(m_cancelButton)
	        .Add(m_okButton)
	        .SetInsets(spacing, spacing, spacing, spacing)
	    )
	);

	SetDefaultButton(m_okButton);
	m_usernameTextControl->MakeFocus(true);

    if (m_parentWindowFrame.IsValid())
        CenterIn(m_parentWindowFrame);
    else
        CenterOnScreen();

	// Start AuthenticationPanel window thread
	Show();

	// Let the window jitter, if the previous password was invalid
	if (badPassword)
		PostMessage(kMsgJitter);

	// Block calling thread
	// TODO: When called from other window threads, this should periodically
	// call UpdateIfNeeded()...
	acquire_sem(m_exitSemaphore);

	// AuthenticationPanel wants to quit.
	Lock();

	user = m_usernameTextControl->Text();
	pass = m_passwordTextControl->Text();
	if (rememberCredentials)
        *rememberCredentials = m_remberCredentialsCheckBox->Value() == B_CONTROL_ON;

    bool canceled = m_cancelled;
	Quit();
	// AuthenticationPanel object is TOAST here.
	return !canceled;
}
