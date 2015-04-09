/*
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ContextMenuItem.h"

#if ENABLE(CONTEXT_MENUS)

#include "NotImplemented.h"

namespace WebCore {

ContextMenuItem::ContextMenuItem()
    : m_platformDescription(nullptr)
{
    notImplemented();
}

ContextMenuItem::ContextMenuItem(PlatformMenuItemDescription item)
    : m_platformDescription(item)
{
    notImplemented();
}

ContextMenuItem::ContextMenuItem(ContextMenu*)
    : m_platformDescription(nullptr)
{
    notImplemented();
}

ContextMenuItem::ContextMenuItem(ContextMenuItemType, ContextMenuAction, const String&, ContextMenu*)
    : m_platformDescription(nullptr)
{
    notImplemented();
}

ContextMenuItem::ContextMenuItem(ContextMenuItemType, ContextMenuAction, const String&, bool, bool)
    : m_platformDescription(nullptr)
{
    notImplemented();
}

ContextMenuItem::ContextMenuItem(ContextMenuAction, const String&, bool, bool, Vector<ContextMenuItem>&)
    : m_platformDescription(nullptr)
{
    notImplemented();
}

ContextMenuItem::~ContextMenuItem()
{
    notImplemented();
}

ContextMenuItemType ContextMenuItem::type() const
{
    notImplemented();
    return ActionType;
}

void ContextMenuItem::setType(ContextMenuItemType)
{
    notImplemented();
}

ContextMenuAction ContextMenuItem::action() const
{
    notImplemented();
    return ContextMenuItemTagNoAction;
}

void ContextMenuItem::setAction(ContextMenuAction)
{
    notImplemented();
}

String ContextMenuItem::title() const
{
    notImplemented();
    return String();
}

void ContextMenuItem::setTitle(const String&)
{
    notImplemented();
}

PlatformMenuDescription ContextMenuItem::platformSubMenu() const
{
    notImplemented();
    return nullptr;
}

void ContextMenuItem::setSubMenu(ContextMenu*)
{
    notImplemented();
}

void ContextMenuItem::setSubMenu(Vector<ContextMenuItem>&)
{
    notImplemented();
}

void ContextMenuItem::setChecked(bool)
{
    notImplemented();
}

bool ContextMenuItem::checked() const
{
    notImplemented();
    return false;
}

bool ContextMenuItem::enabled() const
{
    notImplemented();
    return false;
}

void ContextMenuItem::setEnabled(bool)
{
    notImplemented();
}

ContextMenuItem ContextMenuItem::shareMenuItem(const URL&, const URL&, Image*, const String&)
{
    return ContextMenuItem(SubmenuType, ContextMenuItemTagShareMenu, emptyString());
}

} // namespace WebCore

#endif // ENABLE(CONTEXT_MENUS)
