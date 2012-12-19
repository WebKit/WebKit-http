/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "APICast.h"
#include "APIShims.h"
#include "Completion.h"
#include "JSBasePrivate.h"
#include "JSGlobalData.h"
#include "JSScriptRefPrivate.h"
#include "OpaqueJSString.h"
#include "SourceCode.h"
#include "SourceProvider.h"

using namespace JSC;

struct OpaqueJSScript : public SourceProvider {
public:
    static WTF::PassRefPtr<OpaqueJSScript> create(JSGlobalData* globalData, const String& url, int startingLineNumber, const String& source)
    {
        return WTF::adoptRef(new OpaqueJSScript(globalData, url, startingLineNumber, source));
    }

    const String& source() const OVERRIDE
    {
        return m_source;
    }

    const JSGlobalData* globalData() const { return m_globalData; }

private:
    OpaqueJSScript(JSGlobalData* globalData, const String& url, int startingLineNumber, const String& source)
        : SourceProvider(url, TextPosition(OrdinalNumber::fromOneBasedInt(startingLineNumber), OrdinalNumber::first()))
        , m_globalData(globalData)
        , m_source(source)
    {
    }

    ~OpaqueJSScript() { }

    JSGlobalData* m_globalData;
    String m_source;
};

extern "C" {

JSScriptRef JSScriptCreateReferencingImmortalASCIIText(JSContextRef context, JSStringRef url, int startingLineNumber, const char* source, size_t length, JSValueRef* exception)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    JSGlobalData* globalData = &exec->globalData();
    for (size_t i = 0; i < length; i++) {
        if (!isASCII(source[i]))
            return 0;
    }

    RefPtr<OpaqueJSScript> result = OpaqueJSScript::create(globalData, url->string(), startingLineNumber, String(StringImpl::createFromLiteral(source, length)));
    JSValue exceptionValue;
    if (!checkSyntax(exec, SourceCode(result), &exceptionValue)) {
        if (exception)
            *exception = toRef(exec, exceptionValue);
        return 0;
    }

    return result.release().leakRef();
}

JSScriptRef JSScriptCreateFromString(JSContextRef context, JSStringRef url, int startingLineNumber, JSStringRef source, JSValueRef* exception)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    JSGlobalData* globalData = &exec->globalData();

    RefPtr<OpaqueJSScript> result = OpaqueJSScript::create(globalData, url->string(), startingLineNumber, source->string());
    JSValue exceptionValue;
    if (!checkSyntax(exec, SourceCode(result), &exceptionValue)) {
        if (exception)
            *exception = toRef(exec, exceptionValue);
        return 0;
    }
    return result.release().leakRef();
}

void JSScriptRetain(JSContextRef context, JSScriptRef script)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    if (script->globalData() != &exec->globalData())
        CRASH();
    script->ref();
}

void JSScriptRelease(JSContextRef context, JSScriptRef script)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    if (script->globalData() != &exec->globalData())
        CRASH();
    script->deref();
}

JSValueRef JSScriptEvaluate(JSContextRef context, JSScriptRef script, JSValueRef thisValueRef, JSValueRef* exception)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    if (script->globalData() != &exec->globalData()) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    JSValue internalException;
    JSValue thisValue = thisValueRef ? toJS(exec, thisValueRef) : jsUndefined();
    JSValue result = evaluate(exec, SourceCode(script), thisValue, &internalException);
    if (internalException) {
        if (exception)
            *exception = toRef(exec, internalException);
        return 0;
    }
    ASSERT(result);
    return toRef(exec, result);
}

}
