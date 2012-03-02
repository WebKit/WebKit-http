/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009, 2011 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"

#if ENABLE(WORKERS) && ENABLE(INDEXED_DATABASE)

#include "WorkerContextIndexedDatabase.h"

#include "IDBFactory.h"
#include "IDBFactoryBackendInterface.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"

namespace WebCore {

WorkerContextIndexedDatabase::WorkerContextIndexedDatabase(ScriptExecutionContext* context)
    : m_context(context)
{
}

WorkerContextIndexedDatabase::~WorkerContextIndexedDatabase()
{
}

WorkerContextIndexedDatabase* WorkerContextIndexedDatabase::from(ScriptExecutionContext* context)
{
    AtomicString name = "WorkderContextIndexedDatabase";
    WorkerContextIndexedDatabase* supplement = static_cast<WorkerContextIndexedDatabase*>(Supplement<ScriptExecutionContext>::from(context, name));
    if (!supplement) {
        supplement = new WorkerContextIndexedDatabase(context);
        provideTo(context, name, adoptPtr(supplement));
    }
    return supplement;
}

IDBFactory* WorkerContextIndexedDatabase::webkitIndexedDB(ScriptExecutionContext* context)
{
    return from(context)->webkitIndexedDB();
}

IDBFactory* WorkerContextIndexedDatabase::webkitIndexedDB()
{
    if (!m_context->securityOrigin()->canAccessDatabase())
        return 0;
    if (!m_factoryBackend)
        m_factoryBackend = IDBFactoryBackendInterface::create();
    if (!m_idbFactory)
        m_idbFactory = IDBFactory::create(m_factoryBackend.get());
    return m_idbFactory.get();
}

} // namespace WebCore

#endif // ENABLE(WORKERS) && ENABLE(INDEXED_DATABASE)
