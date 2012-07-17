/*
 * Copyright (C) 2005, 2005 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2008 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "SVGImageLoader.h"

#include "Event.h"
#include "EventNames.h"
#include "HTMLParserIdioms.h"
#include "ImageLoaderClient.h"
#include "RenderImage.h"
#include "SVGImageElement.h"

namespace WebCore {

SVGImageLoader::SVGImageLoader(ImageLoaderClient* client)
    : ImageLoader(client)
{
}

void SVGImageLoader::dispatchLoadEvent()
{
    if (image()->errorOccurred())
        client()->imageElement()->dispatchEvent(Event::create(eventNames().errorEvent, false, false));
    else {
        SVGImageElement* imageElement = static_cast<SVGImageElement*>(client()->imageElement());
        if (imageElement->externalResourcesRequiredBaseValue())
            imageElement->sendSVGLoadEventIfPossible(true);
    }
}

String SVGImageLoader::sourceURI(const AtomicString& attribute) const
{
    KURL base = client()->sourceElement()->baseURI();
    if (base.isValid())
        return KURL(base, stripLeadingAndTrailingHTMLSpaces(attribute)).string();
    return client()->sourceElement()->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(attribute));
}

}

#endif // ENABLE(SVG)
