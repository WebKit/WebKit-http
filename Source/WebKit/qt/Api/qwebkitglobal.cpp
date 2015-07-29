/*
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

#include "config.h"
#include "qwebkitglobal.h"

#include <WebKitVersion.h>

/*!
    \relates QWebPage
    \since 4.6
    Returns the version number of WebKit at run-time as a string (for
    example, "531.3").

    This version is commonly used in WebKit based browsers as part
    of the user agent string. Web servers and JavaScript might use
    it to identify the presence of certain WebKit engine features
    and behaviour.

    The evolution of this version is bound to the releases of Apple's
    Safari browser.

    \sa QWebPage::userAgentForUrl()
*/
QString qWebKitVersion()
{
    return QString::fromLatin1("%1.%2").arg(WEBKIT_MAJOR_VERSION).arg(WEBKIT_MINOR_VERSION);
}

/*!
    \relates QWebPage
    \since 4.6
    Returns the 'major' version number of WebKit at run-time as an integer
    (for example, 531). This is the version of WebKit the application
    was compiled against.

    \sa qWebKitVersion()
*/
int qWebKitMajorVersion()
{
    return WEBKIT_MAJOR_VERSION;
}

/*!
    \relates QWebPage
    \since 4.6
    Returns the 'minor' version number of WebKit at run-time as an integer
    (for example, 3). This is the version of WebKit the application
    was compiled against.

    \sa qWebKitVersion()
*/
int qWebKitMinorVersion()
{
    return WEBKIT_MINOR_VERSION;
}
