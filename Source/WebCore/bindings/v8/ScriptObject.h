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

#ifndef ScriptObject_h
#define ScriptObject_h

#include "ScriptValue.h"

#include <v8.h>

namespace WebCore {
    class InjectedScriptHost;
    class InspectorFrontendHost;
    class ScriptState;

    class ScriptObject : public ScriptValue {
    public:
        ScriptObject(ScriptState*, v8::Handle<v8::Object>);
        ScriptObject(ScriptState*, const ScriptValue&);
        ScriptObject() : m_scriptState(0) { };
        virtual ~ScriptObject() { }

        v8::Local<v8::Object> v8Object() const;
        ScriptState* scriptState() const { return m_scriptState; }
    protected:
        ScriptState* m_scriptState;
    };

    class ScriptGlobalObject {
    public:
        static bool set(ScriptState*, const char* name, const ScriptObject&);
        static bool set(ScriptState*, const char* name, InspectorFrontendHost*);
        static bool set(ScriptState*, const char* name, InjectedScriptHost*);
        static bool get(ScriptState*, const char* name, ScriptObject&);
        static bool remove(ScriptState*, const char* name);
    private:
        ScriptGlobalObject() { }
    };

}

#endif // ScriptObject_h
