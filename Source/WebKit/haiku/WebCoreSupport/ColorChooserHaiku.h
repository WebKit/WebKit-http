/*
 * Copyright (C) 2014 Haiku, Inc.
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
#include "ColorChooser.h"

#include "ColorChooserClient.h"

#include <Button.h>
#include <ColorControl.h>
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Window.h>

namespace WebCore {

class ColorChooserWindow: public BWindow
{
public:
    ColorChooserWindow(ColorChooserClient& client)
        : BWindow(client.elementRectRelativeToRootView(), "Color Picker",
            B_FLOATING_WINDOW, B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE
            | B_AUTO_UPDATE_SIZE_LIMITS)
        , m_client(client)
    {
        SetLayout(new BGroupLayout(B_VERTICAL));
#if 0
        // FIXME when using a datalist, shouldShowSuggestions returns true, but
        // the suggestions vector is empty. This will have to wait until WebKit
        // gets working datalist support.
        if (client.shouldShowSuggestions()) {
            BGridView* swatches = new BGridView();

            Vector<Color> colors = client.suggestions();

            int i = 0;
            for(Color c: colors) {
                puts("Adding color");
                BButton* v = new BButton("swatch", " ", NULL);
                v->SetViewColor(c);
                swatches->GridLayout()->AddView(v, 0, i++);
            }

            AddChild(swatches);
        }
#endif

        control = new BColorControl(B_ORIGIN, B_CELLS_32x8, 8, "picker");
        control->SetValue(m_client.currentColor());

        BButton* ok = new BButton("ok", "Done", new BMessage('done'));
        BButton* cancel = new BButton("cancel", "Cancel", new BMessage('canc'));

        BGroupView* group = new BGroupView(B_VERTICAL);
        group->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        BGroupLayoutBuilder(group)
            .SetInsets(5, 5, 5, 5)
            .Add(control)
            .AddGroup(B_HORIZONTAL)
                .AddGlue()
                .Add(cancel)
                .Add(ok)
            .End()
        .End();
        AddChild(group);
    }

    void Hide() override {
        m_client.didEndChooser();
        BWindow::Hide();
    }

    void MessageReceived(BMessage* message) {
        switch(message->what) {
            case 'done':
                m_client.didChooseColor(control->ValueAsColor());
                // fallthrough
            case 'canc':
                Hide();
                return;
        }

        BWindow::MessageReceived(message);
    }

private:
    ColorChooserClient& m_client;
    BColorControl* control;
};


class ColorChooserHaiku: public ColorChooser
{
public:
    ColorChooserHaiku(ColorChooserClient* client, const Color& color)
        : m_window(new ColorChooserWindow(*client))
        , m_client(*client)
    {
        reattachColorChooser(color);
    }

    ~ColorChooserHaiku()
    {
        m_window->PostMessage(B_QUIT_REQUESTED);
    }


    void reattachColorChooser(const Color& color) override
    {
        setSelectedColor(color);
        m_window->Show();
    }

    void setSelectedColor(const Color& color) override
    {
        m_client.didChooseColor(color);
    }

    void endChooser() override {
        m_window->Hide();
    }

private:
    BWindow* m_window;
    ColorChooserClient& m_client;
};

}
