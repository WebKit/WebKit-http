/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
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
#include "ContextMenu.h"

#include "ContextMenuController.h"
#include "ContextMenuItem.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include <Application.h>
#include <Handler.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <wtf/Assertions.h>
#include <stdio.h>

namespace WebCore {

class ContextMenu::ContextMenuHandler : public BHandler {
public:
    ContextMenuHandler(ContextMenu* menu)
        : BHandler("context menu handler")
        , m_menu(menu)
    {
    }

    virtual void MessageReceived(BMessage* message)
    {
        int result = message->what;
        if (result != -1) {
            BMenuItem* item = m_menu->platformDescription()->FindItem(result);
            if (!item) {
                printf("Error: Context menu item with code %i not found!\n", result);
                return;
            }
            ContextMenuItem cmItem(item);
            m_menu->controller()->contextMenuItemSelected(&cmItem);
            cmItem.releasePlatformDescription();
        }
    }

private:
    ContextMenu* m_menu;
};

ContextMenu::ContextMenu(const HitTestResult& result)
    : m_hitTestResult(result)
    , m_platformDescription(new BMenu("context_menu"))
    , m_menuHandler(new ContextMenuHandler(this))
{
	if (be_app->Lock()) {
		be_app->AddHandler(m_menuHandler);
		be_app->Unlock();
	}
}

ContextMenu::~ContextMenu()
{
	if (be_app->Lock()) {
		be_app->RemoveHandler(m_menuHandler);
		be_app->Unlock();
	}
	delete m_menuHandler;
    delete m_platformDescription;
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
	insertItem(itemCount(), item);
}

unsigned ContextMenu::itemCount() const
{
    return m_platformDescription->CountItems();
}

void ContextMenu::insertItem(unsigned position, ContextMenuItem& item)
{
    checkOrEnableIfNeeded(item);

    BMenuItem* menuItem = item.releasePlatformDescription();
    if (menuItem) {
        m_platformDescription->AddItem(menuItem, position);
        menuItem->SetTarget(BMessenger(m_menuHandler));
    }
}

PlatformMenuDescription ContextMenu::platformDescription() const
{
    return m_platformDescription;
}

void ContextMenu::setPlatformDescription(PlatformMenuDescription menu)
{
    if (menu == m_platformDescription)
        return;

    delete m_platformDescription;
    m_platformDescription = menu;
}

PlatformMenuDescription ContextMenu::releasePlatformDescription()
{
    PlatformMenuDescription description = m_platformDescription;
    m_platformDescription = 0;

    return description;
}

} // namespace WebCore

