/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
#include "ItemSelectorContextObject.h"

#include "PlatformPopupMenuData.h"
#include "WebPopupItem.h"
#include "WebPopupMenuProxyQt.h"
#include "qquickwebview_p.h"
#include "qquickwebview_p_p.h"
#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

using namespace WebCore;

namespace WebKit {

ItemSelectorContextObject::ItemSelectorContextObject(const QRectF& elementRect, const Vector<WebPopupItem>& webPopupItems, bool multiple)
    : m_elementRect(elementRect)
    , m_items(webPopupItems, multiple)
{
    connect(&m_items, SIGNAL(indexUpdated()), SLOT(onIndexUpdate()));
}

void ItemSelectorContextObject::onIndexUpdate()
{
    // Send the update for multi-select list.
    if (m_items.multiple())
        emit acceptedWithOriginalIndex(m_items.selectedOriginalIndex());
}


void ItemSelectorContextObject::accept(int index)
{
    // If the index is not valid for multi-select lists, just hide the pop up as the selected indices have
    // already been sent.
    if ((index == -1) && m_items.multiple())
        emit done();
    else {
        if (index != -1)
            m_items.toggleItem(index);
        emit acceptedWithOriginalIndex(m_items.selectedOriginalIndex());
    }
}

static QHash<int, QByteArray> createRoleNamesHash()
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "text";
    roles[Qt::ToolTipRole] = "tooltip";
    roles[PopupMenuItemModel::GroupRole] = "group";
    roles[PopupMenuItemModel::EnabledRole] = "enabled";
    roles[PopupMenuItemModel::SelectedRole] = "selected";
    roles[PopupMenuItemModel::IsSeparatorRole] = "isSeparator";
    return roles;
}

PopupMenuItemModel::PopupMenuItemModel(const Vector<WebPopupItem>& webPopupItems, bool multiple)
    : m_selectedModelIndex(-1)
    , m_allowMultiples(multiple)
{
    buildItems(webPopupItems);
}

QHash<int, QByteArray> PopupMenuItemModel::roleNames() const
{
    static QHash<int, QByteArray> roles = createRoleNamesHash();
    return roles;
}

QVariant PopupMenuItemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const Item& item = m_items[index.row()];
    if (item.isSeparator) {
        if (role == IsSeparatorRole)
            return true;
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return item.text;
    case Qt::ToolTipRole:
        return item.toolTip;
    case GroupRole:
        return item.group;
    case EnabledRole:
        return item.enabled;
    case SelectedRole:
        return item.selected;
    case IsSeparatorRole:
        return false;
    }

    return QVariant();
}

void PopupMenuItemModel::select(int index)
{
    toggleItem(index);
    emit indexUpdated();
}

void PopupMenuItemModel::toggleItem(int index)
{
    int oldIndex = m_selectedModelIndex;
    if (index < 0 || index >= m_items.size())
        return;
    Item& item = m_items[index];
    if (!item.enabled)
        return;

    m_selectedModelIndex = index;
    if (m_allowMultiples)
        item.selected = !item.selected;
    else {
        if (index == oldIndex)
            return;
        item.selected = true;
        if (oldIndex != -1) {
            Item& oldItem = m_items[oldIndex];
            oldItem.selected = false;
            emit dataChanged(this->index(oldIndex), this->index(oldIndex));
        }
    }

    emit dataChanged(this->index(index), this->index(index));
}

int PopupMenuItemModel::selectedOriginalIndex() const
{
    if (m_selectedModelIndex == -1)
        return -1;
    return m_items[m_selectedModelIndex].originalIndex;
}

void PopupMenuItemModel::buildItems(const Vector<WebPopupItem>& webPopupItems)
{
    QString currentGroup;
    m_items.reserveInitialCapacity(webPopupItems.size());
    for (int i = 0; i < webPopupItems.size(); i++) {
        const WebPopupItem& webPopupItem = webPopupItems[i];
        if (webPopupItem.m_isLabel) {
            currentGroup = webPopupItem.m_text;
            continue;
        }
        if (webPopupItem.m_isSelected && !m_allowMultiples)
            m_selectedModelIndex = m_items.size();
        m_items.append(Item(webPopupItem, currentGroup, i));
    }
}

} // namespace WebKit
