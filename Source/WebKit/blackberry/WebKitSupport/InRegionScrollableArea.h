/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef InRegionScrollableArea_h
#define InRegionScrollableArea_h

#include "IntRect.h"

#include <interaction/ScrollViewBase.h>

namespace WebCore {
class LayerWebKitThread;
class Node;
class RenderLayer;
}

namespace BlackBerry {
namespace WebKit {

class WebPagePrivate;

class InRegionScrollableArea : public Platform::ScrollViewBase {
public:

    InRegionScrollableArea();
    InRegionScrollableArea(WebPagePrivate*, WebCore::RenderLayer*);
    virtual ~InRegionScrollableArea();

    void setVisibleWindowRect(const WebCore::IntRect&);
    Platform::IntRect visibleWindowRect() const;

    WebCore::RenderLayer* layer() const;

private:
    WebPagePrivate* m_webPage;
    WebCore::RenderLayer* m_layer;

    // We either cache one here: in case of a composited scrollable layer
    // cache the LayerWebKitThread. Otherwise, the Node.
    RefPtr<WebCore::LayerWebKitThread> m_cachedCompositedScrollableLayer;
    RefPtr<WebCore::Node> m_cachedNonCompositedScrollableNode;

    bool m_hasWindowVisibleRectCalculated;
};

}
}

#endif
