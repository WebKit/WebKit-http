/*
    Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
    Copyright (C) 2008 Trolltech ASA
    Copyright (C) 2008 Collabora Ltd. All rights reserved.
    Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2011 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

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
#include "PlatformStrategiesHaiku.h"

#include "BlobRegistryImpl.h"
#include "NetworkStorageSession.h"
#include "wtf/NeverDestroyed.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "WebResourceLoadScheduler.h"


using namespace WebCore;


void PlatformStrategiesHaiku::initialize()
{
    static NeverDestroyed<PlatformStrategiesHaiku> platformStrategies;
    setPlatformStrategies(&platformStrategies.get());
}

PlatformStrategiesHaiku::PlatformStrategiesHaiku()
{
}

LoaderStrategy* PlatformStrategiesHaiku::createLoaderStrategy()
{
    return new WebResourceLoadScheduler();
}

PasteboardStrategy* PlatformStrategiesHaiku::createPasteboardStrategy()
{
    notImplemented();
    return 0;
}

WebCore::BlobRegistry* PlatformStrategiesHaiku::createBlobRegistry()
{
    return new BlobRegistryImpl();
}


bool PlatformStrategiesHaiku::cookiesEnabled(const NetworkStorageSession& session)
{
	return session.cookiesEnabled();
}


bool PlatformStrategiesHaiku::getRawCookies(const NetworkStorageSession& session, const URL& firstParty, const WebCore::SameSiteInfo& sameSite, const URL& url,
	WTF::Optional<uint64_t> frameID, WTF::Optional<PageIdentifier> pageID,
	Vector<Cookie>& rawCookies)
{
    return session.getRawCookies(firstParty, sameSite, url, frameID, pageID, rawCookies);
}

void PlatformStrategiesHaiku::deleteCookie(const NetworkStorageSession& session, const URL& url, const String& cookieName)
{
    session.deleteCookie(url, cookieName);
}
