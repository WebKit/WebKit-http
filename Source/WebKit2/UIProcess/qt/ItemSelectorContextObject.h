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

#pragma once

#include "WebPopupItem.h"
#include <QtCore/QAbstractListModel>
#include <QtCore/QRectF>
#include <wtf/Vector.h>

namespace WebKit {

class PopupMenuItemModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        GroupRole = Qt::UserRole,
        EnabledRole = Qt::UserRole + 1,
        SelectedRole = Qt::UserRole + 2,
        IsSeparatorRole = Qt::UserRole + 3
    };

    PopupMenuItemModel(const Vector<WebPopupItem>&, bool multiple);
    int rowCount(const QModelIndex& parent = QModelIndex()) const final { return m_items.size(); }
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const final;
    QHash<int, QByteArray> roleNames() const final;

    Q_INVOKABLE void select(int);

    int selectedOriginalIndex() const;
    bool multiple() const { return m_allowMultiples; }
    void toggleItem(int);

Q_SIGNALS:
    void indexUpdated();

private:
    struct Item {
        Item(const WebPopupItem& webPopupItem, const QString& group, int originalIndex)
            : text(webPopupItem.m_text)
            , toolTip(webPopupItem.m_toolTip)
            , group(group)
            , originalIndex(originalIndex)
            , enabled(webPopupItem.m_isEnabled)
            , selected(webPopupItem.m_isSelected)
            , isSeparator(webPopupItem.m_type == WebPopupItem::Separator)
        { }

        QString text;
        QString toolTip;
        QString group;
        // Keep track of originalIndex because we don't add the label (group) items to our vector.
        int originalIndex;
        bool enabled;
        bool selected;
        bool isSeparator;
    };

    void buildItems(const Vector<WebPopupItem>& webPopupItems);

    Vector<Item> m_items;
    int m_selectedModelIndex;
    bool m_allowMultiples;
};

class ItemSelectorContextObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QRectF elementRect READ elementRect CONSTANT FINAL)
    Q_PROPERTY(QObject* items READ items CONSTANT FINAL)
    Q_PROPERTY(bool allowMultiSelect READ allowMultiSelect CONSTANT FINAL)

public:
    ItemSelectorContextObject(const QRectF& elementRect, const Vector<WebPopupItem>&, bool multiple);

    QRectF elementRect() const { return m_elementRect; }
    PopupMenuItemModel* items() { return &m_items; }
    bool allowMultiSelect() { return m_items.multiple(); }

    Q_INVOKABLE void accept(int index = -1);
    Q_INVOKABLE void reject() { emit done(); }
    Q_INVOKABLE void dismiss() { emit done(); }

Q_SIGNALS:
    void acceptedWithOriginalIndex(int);
    void done();

private Q_SLOTS:
    void onIndexUpdate();

private:
    QRectF m_elementRect;
    PopupMenuItemModel m_items;
};

} // namespace WebKit
