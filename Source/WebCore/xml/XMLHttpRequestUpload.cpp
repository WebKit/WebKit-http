/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "XMLHttpRequestUpload.h"

#include "Event.h"
#include "EventException.h"
#include "EventNames.h"
#include "XMLHttpRequest.h"
#include "XMLHttpRequestProgressEvent.h"
#include <wtf/Assertions.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

XMLHttpRequestUpload::XMLHttpRequestUpload(XMLHttpRequest* xmlHttpRequest)
    : m_xmlHttpRequest(xmlHttpRequest)
{
}

const AtomicString& XMLHttpRequestUpload::interfaceName() const
{
    return eventNames().interfaceForXMLHttpRequestUpload;
}

ScriptExecutionContext* XMLHttpRequestUpload::scriptExecutionContext() const
{
    return m_xmlHttpRequest->scriptExecutionContext();
}

EventTargetData* XMLHttpRequestUpload::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* XMLHttpRequestUpload::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void XMLHttpRequestUpload::dispatchEventAndLoadEnd(PassRefPtr<Event> event)
{
    ASSERT(event->type() == eventNames().loadEvent || event->type() == eventNames().abortEvent || event->type() == eventNames().errorEvent || event->type() == eventNames().timeoutEvent);

    dispatchEvent(event);
    dispatchEvent(XMLHttpRequestProgressEvent::create(eventNames().loadendEvent));
}



} // namespace WebCore
