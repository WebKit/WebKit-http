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

#ifndef PopStateEvent_h
#define PopStateEvent_h

#include "Event.h"
#include "ScriptValue.h"
#include "SerializedScriptValue.h"

namespace WebCore {

struct PopStateEventInit : public EventInit {
    PopStateEventInit();

    ScriptValue state;
};

class PopStateEvent : public Event {
public:
    virtual ~PopStateEvent();
    static PassRefPtr<PopStateEvent> create();
    static PassRefPtr<PopStateEvent> create(const ScriptValue&);
    static PassRefPtr<PopStateEvent> create(PassRefPtr<SerializedScriptValue>);
    static PassRefPtr<PopStateEvent> create(const AtomicString&, const PopStateEventInit&);
    void initPopStateEvent(const AtomicString& type, bool canBubble, bool cancelable, const ScriptValue&);
    bool isPopStateEvent() const { return true; }

    SerializedScriptValue* serializedState() const { return m_serializedState.get(); }
    ScriptValue state() const { return m_state; }

private:
    PopStateEvent();
    PopStateEvent(const AtomicString&, const PopStateEventInit&);
    explicit PopStateEvent(const ScriptValue&);
    explicit PopStateEvent(PassRefPtr<SerializedScriptValue>);

    ScriptValue m_state;
    RefPtr<SerializedScriptValue> m_serializedState;
};

} // namespace WebCore

#endif // PopStateEvent_h
