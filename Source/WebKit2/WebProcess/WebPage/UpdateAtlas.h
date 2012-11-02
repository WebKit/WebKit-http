/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef UpdateAtlas_h
#define UpdateAtlas_h

#include "AreaAllocator.h"
#include "IntSize.h"
#include "ShareableSurface.h"

#if USE(COORDINATED_GRAPHICS)
namespace WebCore {
class GraphicsContext;
class IntPoint;
}

namespace WebKit {

class UpdateAtlasClient {
public:
    virtual void createUpdateAtlas(int atlasID, const ShareableSurface::Handle&) = 0;
    virtual void removeUpdateAtlas(int atlasID) = 0;
};

class UpdateAtlas {
    WTF_MAKE_NONCOPYABLE(UpdateAtlas);
public:
    UpdateAtlas(UpdateAtlasClient*, int dimension, ShareableBitmap::Flags);
    ~UpdateAtlas();

    inline WebCore::IntSize size() const { return m_surface->size(); }

    // Returns a null pointer of there is no available buffer.
    PassOwnPtr<WebCore::GraphicsContext> beginPaintingOnAvailableBuffer(int& atlasID, const WebCore::IntSize&, WebCore::IntPoint& offset);
    void didSwapBuffers();
    ShareableBitmap::Flags flags() const { return m_flags; }

    void addTimeInactive(double seconds)
    {
        ASSERT(!isInUse());
        m_inactivityInSeconds += seconds;
    }
    bool isInactive() const
    {
        const double inactiveSecondsTolerance = 3;
        return m_inactivityInSeconds > inactiveSecondsTolerance;
    }
    bool isInUse() const { return m_areaAllocator; }

private:
    void buildLayoutIfNeeded();

private:
    UpdateAtlasClient* m_client;
    OwnPtr<GeneralAreaAllocator> m_areaAllocator;
    ShareableBitmap::Flags m_flags;
    RefPtr<ShareableSurface> m_surface;
    ShareableSurface::Handle m_handle;
    double m_inactivityInSeconds;
    int m_ID;
    bool m_isVaild;
};

}
#endif
#endif // UpdateAtlas_h
