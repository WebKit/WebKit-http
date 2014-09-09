/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

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

#ifndef GraphicsContext3DPrivate_h
#define GraphicsContext3DPrivate_h

#include "GraphicsContext3D.h"
#include <texmap/TextureMapperPlatformLayer.h>
#include "GLPlatformContext.h"

namespace WebCore {
class GraphicsContext3DPrivate: public TextureMapperPlatformLayer
{
public:
    static std::unique_ptr<GraphicsContext3DPrivate> create(GraphicsContext3D*, HostWindow*);

    GraphicsContext3DPrivate(GraphicsContext3D*, HostWindow*);
    ~GraphicsContext3DPrivate();

    PlatformGraphicsContext3D platformGraphicsContext3D() const;
    void setContextLostCallback(std::unique_ptr<GraphicsContext3D::ContextLostCallback>);
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix&, float) override;
    virtual IntSize platformLayerSize() const override;
    virtual uint32_t copyToGraphicsSurface() override;
    virtual GraphicsSurfaceToken graphicsSurfaceToken() const override;
    virtual GraphicsSurface::Flags graphicsSurfaceFlags() const override;
    void didResizeCanvas(const IntSize&);
    bool makeContextCurrent() const;

private:
    enum PendingOperation {
        Default = 0x00, // No Pending Operation.
        CreateSurface = 0x01,
        Resize = 0x02,
        DeletePreviousSurface = 0x04
    };

    typedef unsigned PendingSurfaceOperation;

    bool initialize();
    void createGraphicsSurface();
    bool prepareBuffer() const;
    void releaseResources();

    GraphicsContext3D* m_context;
    HostWindow* m_hostWindow;
    std::unique_ptr<GLPlatformContext> m_offScreenContext;
    std::unique_ptr<GLPlatformSurface> m_offScreenSurface;
    GraphicsSurfaceToken m_surfaceHandle;
    RefPtr<GraphicsSurface> m_graphicsSurface;
    RefPtr<GraphicsSurface> m_previousGraphicsSurface;
    PendingSurfaceOperation m_surfaceOperation : 3;
    std::unique_ptr<GraphicsContext3D::ContextLostCallback> m_contextLostCallback;
    ListHashSet<GC3Denum> m_syntheticErrors;
    IntSize m_size;
    IntRect m_targetRect;
};

} // namespace WebCore

#endif
