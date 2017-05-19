/*
 * Copyright (C) 2016 Igalia S.L.
 * Copyright (C) 2016 Metrological
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "Extensions3DCache.h"
#include "GLContext.h"
#include "GraphicsContext3D.h"
#include "Extensions3D.h"

namespace WebCore {

const Extensions3DCache& Extensions3DCache::singleton()
{
    static std::once_flag onceFlag;
    static LazyNeverDestroyed<Extensions3DCache> extensions3DCache;
    std::call_once(onceFlag, []{
        extensions3DCache.construct();
    });

    return extensions3DCache;
}

Extensions3DCache::Extensions3DCache()
{
    GLContext* previousActiveContext = GLContext::current();

    if (!previousActiveContext)
        PlatformDisplay::sharedDisplayForCompositing().sharingGLContext()->makeContextCurrent();

    RefPtr<GraphicsContext3D> context3D = GraphicsContext3D::createForCurrentGLContext();
    m_GL_EXT_unpack_subimage = context3D->getExtensions().supports("GL_EXT_unpack_subimage");
    m_GL_OES_packed_depth_stencil = context3D->getExtensions().supports("GL_OES_packed_depth_stencil");
    m_GL_EXT_multisampled_render_to_texture = context3D->getExtensions().supports("GL_EXT_multisampled_render_to_texture");
    context3D = nullptr;

    if (previousActiveContext)
        previousActiveContext->makeContextCurrent();
}

} // namespace WebCore
