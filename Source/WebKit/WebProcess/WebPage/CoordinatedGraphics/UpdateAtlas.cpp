/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Company 100, Inc.

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
#include "UpdateAtlas.h"

#if USE(COORDINATED_GRAPHICS)

#include <WebCore/CoordinatedGraphicsState.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/IntRect.h>
#include <wtf/MathExtras.h>
#include "Extensions3DCache.h"

using namespace WebCore;

namespace WebKit {

UpdateAtlas::UpdateAtlas(Client& client, const IntSize& size, CoordinatedBuffer::Flags flags)
    : m_client(client)
    , m_buffer(CoordinatedBuffer::create(size, flags))
{
    static uint32_t nextID = 0;
    m_ID = ++nextID;

    m_client.createUpdateAtlas(m_ID, m_buffer.copyRef());
}

UpdateAtlas::~UpdateAtlas()
{
    m_client.removeUpdateAtlas(m_ID);
}

void UpdateAtlas::buildLayoutIfNeeded()
{
    if (m_areaAllocator)
        return;
    m_areaAllocator = std::make_unique<GeneralAreaAllocator>(size());
}

void UpdateAtlas::didSwapBuffers()
{
    m_areaAllocator = nullptr;
}

bool UpdateAtlas::paintOnAvailableBuffer(const IntSize& size, uint32_t& atlasID, IntPoint& offset, CoordinatedBuffer::Client& client)
{
    m_inactivityInSeconds = 0;

    IntRect rect;
    if (Extensions3DCache::singleton().supportsUnpackSubimage()) {
        buildLayoutIfNeeded();
        rect = m_areaAllocator->allocate(size);

        // No available buffer was found.
        if (rect.isEmpty())
            return false;

        offset = rect.location();
    } else {
        // When GL_EXT_unpack_subimage is not available, the size requested to paint is the same
        // as the UpdateAtlas, so we don't need the AreaAllocator. The offset is always (0, 0)
        // and the size is always the size requested.
        offset = IntPoint(0, 0);
        rect = IntRect(offset, size);
    }

    atlasID = m_ID;

    // FIXME: Use tri-state buffers, to allow faster updates.
    offset = rect.location();

    {
        GraphicsContext& context = m_buffer->context();
        context.save();
        context.clip(rect);
        context.translate(rect.x(), rect.y());

        if (supportsAlpha()) {
            context.setCompositeOperation(CompositeCopy);
            context.fillRect(IntRect(IntPoint::zero(), size), Color::transparent);
            context.setCompositeOperation(CompositeSourceOver);
        }

        client.paintToSurfaceContext(context);
        context.restore();
    }

    return true;
}

} // namespace WebCore
#endif // USE(COORDINATED_GRAPHICS)
