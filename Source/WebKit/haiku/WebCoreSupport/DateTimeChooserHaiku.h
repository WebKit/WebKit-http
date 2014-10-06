/*
 * Copyright (C) 2014 Haiku, Inc.  All rights reserved.
 *
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
#include "config.h"
#include "DateTimeChooser.h"

#include "DateTimeChooserClient.h"
#include "InputTypeNames.h"

#include <private/shared/CalendarView.h>
#include <LocaleRoster.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <SeparatorView.h>
#include <TextControl.h>

namespace WebCore {

class DateTimeChooserWindow: public BWindow
{
public:
    DateTimeChooserWindow(DateTimeChooserClient* client,
            const DateTimeChooserParameters& params)
        : BWindow(params.anchorRectInRootView, "Date Picker", B_FLOATING_WINDOW,
            B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
        , m_client(client)
    {
        BGroupLayout* root = new BGroupLayout(B_VERTICAL);
        root->SetSpacing(0);
        SetLayout(root);
        BGroupView* group = new BGroupView(B_HORIZONTAL);
        group->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
        group->GroupLayout()->SetInsets(5, 5, 5, 5);
        AddChild(group);

        BButton* ok = new BButton("ok", "Done", new BMessage('done'));
        BButton* cancel = new BButton("cancel", "Cancel", new BMessage('canc'));

        BGroupView* bottomGroup = new BGroupView(B_HORIZONTAL);
        bottomGroup->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
        BGroupLayoutBuilder(bottomGroup)
            .SetInsets(5, 5, 5, 5)
            .AddGlue()
            .Add(cancel)
            .Add(ok);
        AddChild(bottomGroup);

        // TODO handle params.type to decide what to include in the window
        // (may be only a month, or so - but should we use a popup menu in that
        // case?)

        // FIXME we need to parse params.currentValue using the YYYY-MM-dd format
        // and set the fields to the appropriate values. (or other formats
        // depending on the type:
        // week: YYYY-WW
        // time: HH:MM
        // datetime: YYYY-MM-DD-THH:MMZ
        // month: YYYY-MM

        // TODO we should also handle the list of suggestions from the params
        // (probably as a BMenuField), and the min, max, and step values.

        if (params.type == InputTypeNames::datetime() 
                || params.type == InputTypeNames::datetimelocal()
                || params.type == InputTypeNames::date()
                || params.type == InputTypeNames::week()
                || params.type == InputTypeNames::month()) {
            BMenu* monthMenu = new BMenu("month");
            m_yearControl = new BTextControl("year", NULL, NULL,
                new BMessage('yech'));
            monthMenu->SetLabelFromMarked(true);

            BString out;
            BDateFormat format;
            format.SetDateFormat(B_SHORT_DATE_FORMAT, "LLLL");

            for (int i = 1; i <= 12; i++) {
                BDate date(1970, i, 1);
                format.Format(out, date, B_SHORT_DATE_FORMAT);
                BMessage* message = new BMessage('moch');
                message->AddInt32("month", i);
                monthMenu->AddItem(new BMenuItem(out, message));
            }

            BGroupLayoutBuilder(group)
                .AddGroup(B_VERTICAL)
                    .AddGroup(B_HORIZONTAL)
                        .Add(new BMenuField(NULL, monthMenu))
                        .Add(m_yearControl)
                    .End()
                    .Add(m_calendar = new BPrivate::BCalendarView("Date"))
                .End()
            .End();

            format.SetDateFormat(B_LONG_DATE_FORMAT, "YYYY");
            format.Format(out, m_calendar->Date(), B_LONG_DATE_FORMAT);
            m_yearControl->SetText(out.String());

            monthMenu->ItemAt(m_calendar->Date().Month() - 1)->SetMarked(true);
        }

        if (params.type == InputTypeNames::datetime() 
                || params.type == InputTypeNames::datetimelocal())
           group->AddChild(new BSeparatorView(B_VERTICAL));

        if (params.type == InputTypeNames::datetime() 
                || params.type == InputTypeNames::datetimelocal()
                || params.type == InputTypeNames::time()) {
            BMenu* hourMenu = new BMenu("hour");
            hourMenu->SetLabelFromMarked(true);
            BMenu* minuteMenu = new BMenu("minute");
            minuteMenu->SetLabelFromMarked(true);

            for (int i = 0; i <= 24; i++) {
                BString label;
                label << i; // TODO we could be more locale safe here
                hourMenu->AddItem(new BMenuItem(label, NULL));
            }

            hourMenu->ItemAt(0)->SetMarked(true); // TODO select the right one

            for (int i = 0; i <= 60; i++) {
                BString label;
                label << i; // TODO we could be more locale safe here
                minuteMenu->AddItem(new BMenuItem(label, NULL));
            }

            minuteMenu->ItemAt(0)->SetMarked(true); // TODO select the right one

            BGroupLayoutBuilder(group)
                .AddGroup(B_VERTICAL)
                    .AddGroup(B_HORIZONTAL)
                        .Add(new BMenuField(NULL, hourMenu))
                        .Add(new BMenuField(NULL, minuteMenu))
                        .AddGlue();
        }

        // Now show only the relevant parts of the window depending on the type
        // and configure the output format
        if (params.type == InputTypeNames::month()) {
            m_format = "YYYY-MM";
            m_calendar->Hide();
        } else if (params.type == InputTypeNames::date()) {
            m_format = "YYYY-MM-dd";
        } else {
            // TODO week, time, datetime, datetime-local
        }

    }

    void Hide() override {
        m_client->didEndChooser();
        BWindow::Hide();
    }

    void MessageReceived(BMessage* message) {
        switch(message->what) {
            case 'done':
            {
                BString str;
                BLanguage language("en");
                BFormattingConventions conventions("en_US");
                conventions.SetExplicitDateFormat(B_LONG_DATE_FORMAT, m_format);
                BDateFormat formatter(&language, &conventions);
                formatter.Format(str, m_calendar->Date(), B_LONG_DATE_FORMAT);
                m_client->didChooseValue(str);
                // fallthrough
            }
            case 'canc':
                Hide();
                return;

            case 'moch':
            {
                m_calendar->SetMonth(message->FindInt32("month"));
                return;
            }

            case 'yech':
            {
                // FIXME perform some validation on the value...
                int year = strtol(m_yearControl->Text(), NULL, 10);
                m_calendar->SetYear(year);
                return;
            }
        }

        BWindow::MessageReceived(message);
    }

private:
    BPrivate::BCalendarView* m_calendar;
    BTextControl* m_yearControl;
    BString m_format;

    DateTimeChooserClient* m_client;
};

class DateTimeChooserHaiku: public DateTimeChooser
{
public:
    DateTimeChooserHaiku(DateTimeChooserClient* client,
            const DateTimeChooserParameters& params)
        : m_window(new DateTimeChooserWindow(client, params))
    {
        m_window->Show();
    }

    ~DateTimeChooserHaiku()
    {
        m_window->PostMessage(B_QUIT_REQUESTED);
    }

    void endChooser() override
    {
        m_window->Hide();
    }

private:
    BWindow* m_window;
};

}

