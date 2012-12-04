/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2009 Robert Hogan <robert@roberthogan.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QWEBKITGLOBAL_H
#define QWEBKITGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#define QTWEBKIT_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

#ifndef QT_STATIC
#  if !defined(QT_BUILD_WEBKITWIDGETS_LIB) && defined(BUILDING_WEBKIT)
#      define QWEBKIT_EXPORT Q_DECL_EXPORT
#  else
#      define QWEBKIT_EXPORT Q_DECL_IMPORT
#  endif
#  if defined(QT_BUILD_WEBKITWIDGETS_LIB)
#      define QWEBKITWIDGETS_EXPORT Q_DECL_EXPORT
#  else
#      define QWEBKITWIDGETS_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define QWEBKITWIDGETS_EXPORT
#  define QWEBKIT_EXPORT
#endif

QWEBKIT_EXPORT QString qWebKitVersion();
QWEBKIT_EXPORT int qWebKitMajorVersion();
QWEBKIT_EXPORT int qWebKitMinorVersion();

#endif // QWEBKITGLOBAL_H
