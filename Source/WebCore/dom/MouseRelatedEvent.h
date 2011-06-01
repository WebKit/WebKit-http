/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
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
 *
 */

#ifndef MouseRelatedEvent_h
#define MouseRelatedEvent_h

#include "IntPoint.h"
#include "UIEventWithKeyState.h"

namespace WebCore {

    // Internal only: Helper class for what's common between mouse and wheel events.
    class MouseRelatedEvent : public UIEventWithKeyState {
    public:
        // Note that these values are adjusted to counter the effects of zoom, so that values
        // exposed via DOM APIs are invariant under zooming.
        int screenX() const { return m_screenLocation.x(); }
        int screenY() const { return m_screenLocation.y(); }
        const IntPoint& screenLocation() const { return m_screenLocation; }
        int clientX() const { return m_clientLocation.x(); }
        int clientY() const { return m_clientLocation.y(); }
        const IntPoint& clientLocation() const { return m_clientLocation; }
        int layerX();
        int layerY();
        int offsetX();
        int offsetY();
        bool isSimulated() const { return m_isSimulated; }
        virtual int pageX() const;
        virtual int pageY() const;
        virtual const IntPoint& pageLocation() const;
        int x() const;
        int y() const;

        // Page point in "absolute" coordinates (i.e. post-zoomed, page-relative coords,
        // usable with RenderObject::absoluteToLocal).
        const IntPoint& absoluteLocation() const { return m_absoluteLocation; }
        void setAbsoluteLocation(const IntPoint& p) { m_absoluteLocation = p; }
    
    protected:
        MouseRelatedEvent();
        MouseRelatedEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<AbstractView>,
                          int detail, const IntPoint& screenLocation, const IntPoint& windowLocation,
                          bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool isSimulated = false);

        void initCoordinates();
        void initCoordinates(const IntPoint& clientLocation);
        virtual void receivedTarget();

        void computePageLocation();
        void computeRelativePosition();
        
        // Expose these so MouseEvent::initMouseEvent can set them.
        IntPoint m_screenLocation;
        IntPoint m_clientLocation;

    private:
        IntPoint m_pageLocation;
        IntPoint m_layerLocation;
        IntPoint m_offsetLocation;
        IntPoint m_absoluteLocation;
        bool m_isSimulated;
        bool m_hasCachedRelativePosition;
    };

} // namespace WebCore

#endif // MouseRelatedEvent_h
