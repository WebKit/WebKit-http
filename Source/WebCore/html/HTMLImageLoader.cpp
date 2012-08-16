/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "HTMLImageLoader.h"

#include "CachedImage.h"
#include "Element.h"
#include "Event.h"
#include "EventNames.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLParserIdioms.h"
#include "Settings.h"

#if USE(JSC)
#include "JSDOMWindowBase.h"
#include <runtime/JSLock.h>
#endif

namespace WebCore {

HTMLImageLoader::HTMLImageLoader(ImageLoaderClient* client)
    : ImageLoader(client)
{
}

HTMLImageLoader::~HTMLImageLoader()
{
}

void HTMLImageLoader::dispatchLoadEvent()
{
    // HTMLVideoElement uses this class to load the poster image, but it should not fire events for loading or failure.
    if (client()->sourceElement()->hasTagName(HTMLNames::videoTag))
        return;

    bool errorOccurred = image()->errorOccurred();
    if (!errorOccurred && image()->response().httpStatusCode() >= 400)
        errorOccurred = client()->sourceElement()->hasTagName(HTMLNames::objectTag); // An <object> considers a 404 to be an error and should fire onerror.
    client()->eventTarget()->dispatchEvent(Event::create(errorOccurred ? eventNames().errorEvent : eventNames().loadEvent, false, false));
}

String HTMLImageLoader::sourceURI(const AtomicString& attr) const
{
#if ENABLE(DASHBOARD_SUPPORT)
    Settings* settings = client()->sourceElement()->document()->settings();
    if (settings && settings->usesDashboardBackwardCompatibilityMode() && attr.length() > 7 && attr.startsWith("url(\"") && attr.endsWith("\")"))
        return attr.string().substring(5, attr.length() - 7);
#endif

    return stripLeadingAndTrailingHTMLSpaces(attr);
}

void HTMLImageLoader::notifyFinished(CachedResource*)
{
    CachedImage* cachedImage = image();

    RefPtr<Element> element = client()->sourceElement();
    ImageLoader::notifyFinished(cachedImage);

    bool loadError = cachedImage->errorOccurred() || cachedImage->response().httpStatusCode() >= 400;
#if USE(JSC)
    if (!loadError) {
        if (!element->inDocument()) {
            JSC::JSGlobalData* globalData = JSDOMWindowBase::commonJSGlobalData();
            JSC::JSLockHolder lock(globalData);
            globalData->heap.reportExtraMemoryCost(cachedImage->encodedSize());
        }
    }
#endif

    if (loadError && element->hasTagName(HTMLNames::objectTag))
        static_cast<HTMLObjectElement*>(element.get())->renderFallbackContent();
}

}
