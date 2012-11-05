/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SelectionOverlay_h
#define SelectionOverlay_h

#include "BlackBerryGlobal.h"

#if USE(ACCELERATED_COMPOSITING)

#include "Color.h"
#include "GraphicsLayerClient.h"
#include "WebOverlay.h"

#include <BlackBerryPlatformIntRectRegion.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace BlackBerry {
namespace WebKit {

class WebPagePrivate;

class SelectionOverlay : public WebCore::GraphicsLayerClient {
public:
    static PassOwnPtr<SelectionOverlay> create(WebPagePrivate* page)
    {
        return adoptPtr(new SelectionOverlay(page));
    }

    virtual ~SelectionOverlay();

    virtual void draw(const Platform::IntRectRegion&);
    virtual void hide();

    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time) { }
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& inClip);

private:
    SelectionOverlay(WebPagePrivate*);

    WebPagePrivate* m_page;
    OwnPtr<WebOverlay> m_overlay;
    BlackBerry::Platform::IntRectRegion m_region;
};

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)

#endif // SelectionOverlay_h
