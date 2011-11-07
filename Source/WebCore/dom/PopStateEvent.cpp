/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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
#include "PopStateEvent.h"

#include "EventNames.h"

namespace WebCore {

PopStateEventInit::PopStateEventInit()
{
}

PopStateEvent::PopStateEvent()
    : Event(eventNames().popstateEvent, false, true)
    , m_serializedState(0)
{
}

PopStateEvent::PopStateEvent(const AtomicString& type, const PopStateEventInit& initializer)
    : Event(type, initializer)
    , m_state(initializer.state)
    , m_serializedState(0)
{
}

PopStateEvent::PopStateEvent(const ScriptValue& state)
    : Event(eventNames().popstateEvent, false, true)
    , m_state(state)
    , m_serializedState(0)
{
}

PopStateEvent::PopStateEvent(PassRefPtr<SerializedScriptValue> serializedState)
    : Event(eventNames().popstateEvent, false, true)
    , m_serializedState(serializedState)
{
}

PopStateEvent::~PopStateEvent()
{
}

PassRefPtr<PopStateEvent> PopStateEvent::create()
{
    return adoptRef(new PopStateEvent);
}

PassRefPtr<PopStateEvent> PopStateEvent::create(const ScriptValue& state)
{
    return adoptRef(new PopStateEvent(state));
}

PassRefPtr<PopStateEvent> PopStateEvent::create(PassRefPtr<SerializedScriptValue> serializedState)
{
    return adoptRef(new PopStateEvent(serializedState));
}

PassRefPtr<PopStateEvent> PopStateEvent::create(const AtomicString& type, const PopStateEventInit& initializer)
{
    return adoptRef(new PopStateEvent(type, initializer));
}

const AtomicString& PopStateEvent::interfaceName() const
{
    return eventNames().interfaceForPopStateEvent;
}

} // namespace WebCore
