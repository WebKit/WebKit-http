/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "ContextMenu.h"

#include "GOwnPtr.h"
#include <gtk/gtk.h>

namespace WebCore {

ContextMenu::ContextMenu()
{
    m_platformDescription = GTK_MENU(gtk_menu_new());
}

ContextMenu::ContextMenu(const PlatformMenuDescription menu)
    : m_platformDescription(menu)
{
}

ContextMenu::~ContextMenu()
{
    if (m_platformDescription)
        gtk_widget_destroy(GTK_WIDGET(m_platformDescription));
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
    ASSERT(m_platformDescription);

    GtkMenuItem* platformItem = item.releasePlatformDescription();
    ASSERT(platformItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(m_platformDescription), GTK_WIDGET(platformItem));
    gtk_widget_show(GTK_WIDGET(platformItem));
}

void ContextMenu::setPlatformDescription(PlatformMenuDescription menu)
{
    ASSERT(menu);
    if (m_platformDescription == menu)
        return;
    if (m_platformDescription)
        gtk_widget_destroy(GTK_WIDGET(m_platformDescription));

    m_platformDescription = menu;
}

PlatformMenuDescription ContextMenu::platformDescription() const
{
    return m_platformDescription;
}

PlatformMenuDescription ContextMenu::releasePlatformDescription()
{
    PlatformMenuDescription description = m_platformDescription;
    m_platformDescription = 0;

    return description;
}

Vector<ContextMenuItem> contextMenuItemVector(const PlatformMenuDescription menu)
{
    Vector<ContextMenuItem> menuItemVector;

    GOwnPtr<GList> children(gtk_container_get_children(GTK_CONTAINER(menu)));
    int itemCount = g_list_length(children.get());
    menuItemVector.reserveCapacity(itemCount);

    for (GList* item = children.get(); item; item = g_list_next(item)) {
        GtkWidget* widget = static_cast<GtkWidget*>(item->data);
        if (!GTK_IS_MENU_ITEM(widget))
            continue;
        menuItemVector.append(ContextMenuItem(GTK_MENU_ITEM(widget)));
    }

    return menuItemVector;
}

PlatformMenuDescription platformMenuDescription(Vector<ContextMenuItem>& subMenuItems)
{
    GtkMenu* menu = GTK_MENU(gtk_menu_new());
    for (size_t i = 0; i < subMenuItems.size(); i++) {
        GtkWidget* platformItem = GTK_WIDGET(subMenuItems[i].releasePlatformDescription());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), platformItem);
        gtk_widget_show(platformItem);
    }
    return menu;
}

}
