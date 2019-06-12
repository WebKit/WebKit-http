/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.

 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef qwebviewaccessible_p_h
#define qwebviewaccessible_p_h

#include <qaccessible.h>
#include <qaccessibleobject.h>
#include <qaccessiblewidget.h>

#ifndef QT_NO_ACCESSIBILITY

class QWebFrame;
class QWebPage;
class QWebView;

/*
 * Classes representing accessible objects for View, Frame and Page.
 *
 * Each of these just returns one child which lets the accessibility
 * framwork navigate towards the actual contents in the frame.
 */

class QWebFrameAccessible : public QAccessibleObject {
public:
    QWebFrameAccessible(QWebFrame*);

    QWebFrame* frame() const;

    QAccessibleInterface* parent() const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    QAccessibleInterface* child(int index) const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface*) const Q_DECL_OVERRIDE;
    int navigate(QAccessible::RelationFlag, int, QAccessibleInterface** target) const;

    QString text(QAccessible::Text) const Q_DECL_OVERRIDE;
    QAccessible::Role role() const Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;
};

class QWebPageAccessible : public QAccessibleObject {
public:
    QWebPageAccessible(QWebPage*);

    QWebPage* page() const;

    QAccessibleInterface* parent() const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    QAccessibleInterface* child(int index) const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface*) const Q_DECL_OVERRIDE;
    int navigate(QAccessible::RelationFlag, int, QAccessibleInterface** target) const;

    QString text(QAccessible::Text) const Q_DECL_OVERRIDE;
    QAccessible::Role role() const Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;
};

class QWebViewAccessible : public QAccessibleWidget {
public:
    QWebViewAccessible(QWebView*);

    QWebView* view() const;

    int childCount() const Q_DECL_OVERRIDE;
    QAccessibleInterface* child(int index) const Q_DECL_OVERRIDE;
};

#endif // !QT_NO_ACCESSIBILITY

#endif
