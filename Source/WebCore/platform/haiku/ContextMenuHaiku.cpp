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
#include "NotImplemented.h"
#include <Application.h>
#include <Handler.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <wtf/Assertions.h>

namespace WebCore {

class ContextMenu::ContextMenuHandler : public BHandler {
public:
    ContextMenuHandler()
        : BHandler("context menu handler")
        , m_controller(0)
    {
    }

    void setController(ContextMenuController* controller)
    {
        if (!controller || m_controller != 0)
            return;
        m_controller = controller;
    }

    virtual void MessageReceived(BMessage* message)
    {
        if (m_controller && message->what != ContextMenuItemTagNoAction) {
            // Create a temporary ContextMenuItem with a clone of the
            // message. The BMenuItem instance from which this message
            // originates may long be attached to another menu, and doing
            // it this way makes us completely independent of that.
            ContextMenuItem item(new BMenuItem("", new BMessage(*message)));
            m_controller->contextMenuItemSelected(&item);
        }
    }

private:
    ContextMenuController* m_controller;
};

ContextMenu::ContextMenu()
    : m_platformDescription(new BMenu("context_menu"))
    , m_menuHandler(new ContextMenuHandler())
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

void ContextMenu::setController(ContextMenuController* controller)
{
    m_menuHandler->setController(controller);
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
    insertItem(itemCount(), item);
}

unsigned ContextMenu::itemCount() const
{
    return m_platformDescription->CountItems();
}

static void setTargetForItemsRecursive(BMenu* menu, const BMessenger& target)
{
    if (!menu)
        return;
    for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
        item->SetTarget(target);
        setTargetForItemsRecursive(item->Submenu(), target);
    }
}

void ContextMenu::insertItem(unsigned position, ContextMenuItem& item)
{
    BMenuItem* menuItem = item.releasePlatformDescription();
    if (menuItem) {
        m_platformDescription->AddItem(menuItem, position);
        BMessenger target(m_menuHandler);
        menuItem->SetTarget(target);
        setTargetForItemsRecursive(menuItem->Submenu(), target);
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

