/*
 * Copyright 2012 Adobe Systems Incorporated. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CSS_SHADERS)
#include "CustomFilterProgram.h"

#include "CustomFilterProgramClient.h"
#include "CustomFilterShader.h"

#if ENABLE(WEBGL)
#include "GraphicsContext3D.h"
#endif

namespace WebCore {

CustomFilterProgram::CustomFilterProgram()
{
    // Keep the constructor protected to prevent creating this object directly.
}

CustomFilterProgram::~CustomFilterProgram()
{
    // All the clients should keep a reference to this object.
    ASSERT(m_clients.isEmpty());
}

void CustomFilterProgram::addClient(CustomFilterProgramClient* client)
{
    if (m_clients.isEmpty()) {
        // Notify the StyleCustomFilterProgram that we now have at least a client
        // and the loading can begin.
        // Note: If the shader is already cached the first client will be notified,
        // even if the filter was already built. Add the client only after notifying
        // the cache about them, so that we avoid a useless recreation of the filters chain.
        willHaveClients();
    }
    m_clients.add(client);
}

void CustomFilterProgram::removeClient(CustomFilterProgramClient* client)
{
    m_clients.remove(client);
    if (m_clients.isEmpty()) {
        // We have no clients anymore, the cached resources can be purged from memory.
        didRemoveLastClient();
    }
}

void CustomFilterProgram::notifyClients()
{
    for (CustomFilterProgramClientList::iterator iter = m_clients.begin(), end = m_clients.end(); iter != end; ++iter)
        iter->first->notifyCustomFilterProgramLoaded(this);
}

#if ENABLE(WEBGL)
PassRefPtr<CustomFilterShader> CustomFilterProgram::createShaderWithContext(GraphicsContext3D* context)
{
    ASSERT(isLoaded());
    return CustomFilterShader::create(context, vertexShaderString(), fragmentShaderString());
}
#endif

} // namespace WebCore
#endif // ENABLE(CSS_SHADERS)
