/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "WebDOMEventListenerPrivate.h"

#include "EventListenerWrapper.h"
#include "WebDOMEventListener.h"

using namespace WebCore;

namespace WebKit {

WebDOMEventListenerPrivate::WebDOMEventListenerPrivate(WebDOMEventListener* webDOMEventListener)
    : m_webDOMEventListener(webDOMEventListener)
{
}

WebDOMEventListenerPrivate::~WebDOMEventListenerPrivate()
{
}

EventListenerWrapper* WebDOMEventListenerPrivate::createEventListenerWrapper(const WebString& eventType, bool useCapture, EventTarget* target)
{
    EventListenerWrapper* listenerWrapper = new EventListenerWrapper(m_webDOMEventListener);
    WebDOMEventListenerPrivate::ListenerInfo listenerInfo(eventType, useCapture, listenerWrapper, target);
    m_listenerWrappers.append(listenerInfo);
    return listenerWrapper;
}

EventListenerWrapper* WebDOMEventListenerPrivate::getEventListenerWrapper(const WebString& eventType, bool useCapture, EventTarget* target)
{
    Vector<WebDOMEventListenerPrivate::ListenerInfo>::const_iterator iter;
    for (iter = m_listenerWrappers.begin(); iter != m_listenerWrappers.end(); ++iter) {
        if (iter->target == target)
          return iter->eventListenerWrapper;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

void WebDOMEventListenerPrivate::webDOMEventListenerDeleted()
{
    // Notifies all WebDOMEventListenerWrappers that we are going away so they can
    // invalidate their pointer to us.
    Vector<WebDOMEventListenerPrivate::ListenerInfo>::const_iterator iter;
    for (iter = m_listenerWrappers.begin(); iter != m_listenerWrappers.end(); ++iter)
        iter->eventListenerWrapper->webDOMEventListenerDeleted();
}

void WebDOMEventListenerPrivate::eventListenerDeleted(EventListenerWrapper* eventListener)
{
    for (size_t i = 0; i < m_listenerWrappers.size(); ++i) {
        if (m_listenerWrappers[i].eventListenerWrapper == eventListener) {
            m_listenerWrappers.remove(i);
            return;
        }
    }
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
