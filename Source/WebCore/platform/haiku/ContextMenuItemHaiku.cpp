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
#include "ContextMenuItem.h"

#include "ContextMenu.h"
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include "wtf/text/CString.h"

namespace WebCore {

BMenuItem* ContextMenuItem::platformContextMenuItem() const
{
    BMessage* message = new BMessage(m_action);
    message->AddPointer("ContextMenuItem", this);
    BMenuItem* item;

    if (m_type == SeparatorType)
        return new BSeparatorItem(message);

    if (m_type == SubmenuType) {
        BPopUpMenu* subMenu = ContextMenu::createPlatformContextMenuFromItems(
            m_subMenuItems);
        item = new BMenuItem(subMenu, message);
        item->SetLabel(m_title.utf8().data());
    } else
        item = new BMenuItem(m_title.utf8().data(), message);

    item->SetEnabled(m_enabled);
    item->SetMarked(m_checked);

    return item;
}

}
