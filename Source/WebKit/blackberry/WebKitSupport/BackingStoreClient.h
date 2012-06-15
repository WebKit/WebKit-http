/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
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

#ifndef BackingStoreClient_h
#define BackingStoreClient_h

#include "WebPage_p.h"
#include <wtf/Vector.h>

namespace WebCore {
class FloatPoint;
class Frame;
class IntPoint;
class IntSize;
class IntRect;
}

namespace BlackBerry {
namespace WebKit {

class BackingStore;
class WebPagePrivate;

class BackingStoreClient {
public:
    static BackingStoreClient* create(WebCore::Frame*, WebCore::Frame* parentFrame, WebPage* parentPage);
    ~BackingStoreClient();

    BackingStore* backingStore() const { return m_backingStore; }
    WebCore::Frame* frame() const { return m_frame; }
    bool isMainFrame() const { return m_frame == m_webPage->d->m_mainFrame; }

    void addChild(BackingStoreClient* child);
    WTF::Vector <BackingStoreClient*> children() const;
    BackingStoreClient* parent() const { return m_parent; }

    WebCore::IntPoint absoluteLocation() const;
    WebCore::IntPoint transformedAbsoluteLocation() const;
    WebCore::IntRect absoluteRect() const;
    WebCore::IntRect transformedAbsoluteRect() const;

    // scroll position returned is in transformed coordinates
    WebCore::IntPoint scrollPosition() const;
    WebCore::IntPoint maximumScrollPosition() const;
    // scroll position provided should be in transformed coordinates
    void setScrollPosition(const WebCore::IntPoint&);

    WebCore::IntPoint transformedScrollPosition() const;
    WebCore::IntPoint transformedMaximumScrollPosition() const;

    WebCore::IntSize actualVisibleSize() const;
    WebCore::IntSize transformedActualVisibleSize() const;

    WebCore::IntSize viewportSize() const;
    WebCore::IntSize transformedViewportSize() const;

    WebCore::IntRect visibleContentsRect() const;
    WebCore::IntRect transformedVisibleContentsRect() const;

    WebCore::IntSize contentsSize() const;
    WebCore::IntSize transformedContentsSize() const;

    /* Generic conversions of points, rects, relative to and from contents and viewport*/
    WebCore::IntPoint mapFromContentsToViewport(const WebCore::IntPoint&) const;
    WebCore::IntPoint mapFromViewportToContents(const WebCore::IntPoint&) const;
    WebCore::IntRect mapFromContentsToViewport(const WebCore::IntRect&) const;
    WebCore::IntRect mapFromViewportToContents(const WebCore::IntRect&) const;

    /* Generic conversions of points, rects, relative to and from transformed contents and transformed viewport*/
    WebCore::IntPoint mapFromTransformedContentsToTransformedViewport(const WebCore::IntPoint&) const;
    WebCore::IntPoint mapFromTransformedViewportToTransformedContents(const WebCore::IntPoint&) const;
    WebCore::IntRect mapFromTransformedContentsToTransformedViewport(const WebCore::IntRect&) const;
    WebCore::IntRect mapFromTransformedViewportToTransformedContents(const WebCore::IntRect&) const;

    void clipToTransformedContentsRect(WebCore::IntRect&) const;

    bool isLoading() const;
    WebPagePrivate::LoadState loadState() const;

    bool isFocused() const;

    bool isClientGeneratedScroll() const;
    void setIsClientGeneratedScroll(bool);

    bool isScrollNotificationSuppressed() const;
    void setIsScrollNotificationSuppressed(bool);

    /* Called from within WebKit via ChromeClientBlackBerry */
    void checkOriginOfCurrentScrollOperation();

private:
    BackingStoreClient(WebCore::Frame*, WebCore::Frame* parentFrame, WebPage* parentPage);

    WebCore::Frame* m_frame;
    WebPage* m_webPage;
    BackingStore* m_backingStore;
    BackingStoreClient* m_parent;
    bool m_isClientGeneratedScroll;
    bool m_isScrollNotificationSuppressed;
};

}
}

#endif // BackingStoreClient_h
