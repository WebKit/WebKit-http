/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Google, Inc. ("Google") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Intent_h
#define Intent_h

#if ENABLE(WEB_INTENTS)

#include "Dictionary.h"
#include "KURL.h"
#include "MessagePort.h"
#include "MessagePortChannel.h"
#include "ScriptState.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class SerializedScriptValue;

typedef int ExceptionCode;

class Intent : public RefCounted<Intent> {
public:
    static PassRefPtr<Intent> create(const String& action, const String& type, PassRefPtr<SerializedScriptValue> data, const MessagePortArray& ports, ExceptionCode&);
    static PassRefPtr<Intent> create(ScriptState*, const Dictionary&, ExceptionCode&);

    virtual ~Intent() { }

    const String& action() const { return m_action; }
    const String& type() const { return m_type; }
    SerializedScriptValue* data() const { return m_data.get(); }

    MessagePortChannelArray* messagePorts() const { return m_ports.get(); }
    const KURL& service() const { return m_service; }
    const HashMap<String, String>& extras() const { return m_extras; }
    const Vector<KURL>& suggestions() const { return m_suggestions; }

    void setExtras(const WTF::HashMap<String, String>&);

protected:
    Intent(const String& action, const String& type,
           PassRefPtr<SerializedScriptValue> data, PassOwnPtr<MessagePortChannelArray> ports,
           const HashMap<String, String>& extras, const KURL& service, const Vector<KURL>& suggestions);

private:
    String m_action;
    String m_type;
    RefPtr<SerializedScriptValue> m_data;
    OwnPtr<MessagePortChannelArray> m_ports;
    KURL m_service;
    HashMap<String, String> m_extras;
    Vector<KURL> m_suggestions;
};

} // namespace WebCore

#endif

#endif // Intent_h
