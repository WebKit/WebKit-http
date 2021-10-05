/*
 * Copyright (C) 2020 Metrological Group B.V.
 * Copyright (C) 2020 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NicosiaNonCompositedGC3DLayer.h"

#if USE(NICOSIA) && USE(TEXTURE_MAPPER)

#if USE(COORDINATED_GRAPHICS)
#include "TextureMapperGL.h"
#include "TextureMapperPlatformLayerBuffer.h"
#include "TextureMapperPlatformLayerProxy.h"
#endif

#include "GLContext.h"
#include "HostWindow.h"

namespace Nicosia {

using namespace WebCore;

static std::unique_ptr<GLContext> s_windowContext;

static void terminateWindowContext()
{
    s_windowContext = nullptr;
}

NonCompositedGC3DLayer::NonCompositedGC3DLayer(GraphicsContextGLOpenGL& context)
    : m_context(context)
    , m_contentLayer(Nicosia::ContentLayer::create(Nicosia::ContentLayerTextureMapperImpl::createFactory(*this)))
{
}

NonCompositedGC3DLayer::NonCompositedGC3DLayer(GraphicsContextGLOpenGL& context, GraphicsContextGLOpenGL::Destination destination, const HostWindow* hostWindow)
    : m_context(context)
    , m_contentLayer(Nicosia::ContentLayer::create(Nicosia::ContentLayerTextureMapperImpl::createFactory(*this)))
{
    switch (destination) {
    case GraphicsContextGLOpenGL::Destination::DirectlyToHostWindow:
        if (!s_windowContext) {
            s_windowContext = GLContext::createContextForWindow(reinterpret_cast<GLNativeWindowType>(hostWindow->nativeWindowID()), &PlatformDisplay::sharedDisplayForCompositing());
            std::atexit(terminateWindowContext);
        }
        break;
    case GraphicsContextGLOpenGL::Destination::Offscreen:
        ASSERT_NOT_REACHED();
        break;
    }
}

NonCompositedGC3DLayer::~NonCompositedGC3DLayer()
{
    downcast<ContentLayerTextureMapperImpl>(m_contentLayer->impl()).invalidateClient();
}

bool NonCompositedGC3DLayer::makeContextCurrent()
{
    ASSERT(s_windowContext);
    return s_windowContext->makeContextCurrent();
}

PlatformGraphicsContextGL NonCompositedGC3DLayer::platformContext() const
{
    ASSERT(s_windowContext);
    return s_windowContext->platformContext();
}

void NonCompositedGC3DLayer::swapBuffersIfNeeded()
{
    ASSERT(s_windowContext);
    s_windowContext->swapBuffers();
}

} // namespace Nicosia

#endif // USE(NICOSIA) && USE(TEXTURE_MAPPER)
