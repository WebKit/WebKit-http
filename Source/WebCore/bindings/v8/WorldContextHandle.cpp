/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WorldContextHandle.h"

#include "ScriptController.h"
#include "V8Binding.h"
#include "V8DOMWindow.h"
#include "V8DOMWindowShell.h"

namespace WebCore {

WorldContextHandle::WorldContextHandle(WorldToUse worldToUse)
    : m_worldToUse(worldToUse)
{
    if (worldToUse == UseMainWorld || worldToUse == UseWorkerWorld)
        return;

#if ENABLE(WORKERS)
    // FIXME We are duplicating a lot of effort here checking the context for the worker and for the isolated world.
    if (v8::Context::InContext()) {
        v8::Handle<v8::Context> context = v8::Context::GetCurrent();
        if (!context.IsEmpty()) {
            if (UNLIKELY(!V8DOMWrapper::isWrapperOfType(toInnerGlobalObject(context), &V8DOMWindow::info))) {
                m_worldToUse = UseWorkerWorld;
                return;
            }
        }
    }
#endif

    V8DOMWindowShell* shell = V8DOMWindowShell::getEntered();
    if (LIKELY(!shell)) {
        m_worldToUse = UseMainWorld;
        return;
    }

    ASSERT(!shell->context().IsEmpty());
    m_context = SharedPersistent<v8::Context>::create(shell->context());
}

v8::Local<v8::Context> WorldContextHandle::adjustedContext(ScriptController* script) const
{
    ASSERT(m_worldToUse != UseWorkerWorld);
    if (m_worldToUse == UseMainWorld)
        return script->mainWorldContext();

    ASSERT(!m_context->get().IsEmpty());
    return v8::Local<v8::Context>::New(m_context->get());
}

} // namespace WebCore
