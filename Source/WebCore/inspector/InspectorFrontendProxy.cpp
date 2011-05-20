/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "InspectorFrontendProxy.h"

#if ENABLE(INSPECTOR)

#include "EventsCollector.h"
#include "PlatformString.h"

namespace WebCore {

InspectorFrontendProxy::InspectorFrontendProxy(EventsCollector* collector)
{
    m_receiver = 0;
    m_collector = collector;
}

InspectorFrontendProxy::InspectorFrontendProxy(EventsCollector* collector, InspectorFrontendChannel* nextChannel)
{
    m_collector = collector;
    m_receiver = nextChannel;
}

void InspectorFrontendProxy::setInspectorFrontendChannel(InspectorFrontendChannel* receiver)
{
    m_receiver = receiver;
}

InspectorFrontendChannel* InspectorFrontendProxy::inspectorFrontendChannel()
{
    return m_receiver;
}

void InspectorFrontendProxy::setEventsCollector(EventsCollector* collector)
{
    m_collector = collector;
}

bool InspectorFrontendProxy::sendMessageToFrontend(const String& message)
{
    m_collector->addEvent(message);
    if (m_receiver)
        return m_receiver->sendMessageToFrontend(message);
    return false;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
