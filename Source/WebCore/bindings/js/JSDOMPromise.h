/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef JSDOMPromise_h
#define JSDOMPromise_h

#include "JSCryptoKey.h"
#include "JSDOMBinding.h"
#include <runtime/JSGlobalObject.h>
#include <runtime/JSPromise.h>
#include <runtime/JSPromiseResolver.h>
#include <heap/StrongInlines.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

// FIXME: Using this class in DOM code makes it dependent on JS bindings.
class PromiseWrapper {
    WTF_MAKE_NONCOPYABLE(PromiseWrapper)
public:
    static std::unique_ptr<PromiseWrapper> create(JSDOMGlobalObject* globalObject, JSC::JSPromise* promise)
    {
        return std::unique_ptr<PromiseWrapper>(new PromiseWrapper(globalObject, promise));
    }

    template<class FulfillResultType>
    void fulfill(const FulfillResultType&);

    template<class RejectResultType>
    void reject(const RejectResultType&);

private:
    PromiseWrapper(JSDOMGlobalObject*, JSC::JSPromise*);

    JSC::Strong<JSDOMGlobalObject> m_globalObject;
    JSC::Strong<JSC::JSPromise> m_promise;
};

template<class FulfillResultType>
inline void PromiseWrapper::fulfill(const FulfillResultType& result)
{
    JSC::ExecState* exec = m_globalObject->globalExec();
    m_promise->resolver()->fulfillIfNotResolved(exec, toJS(exec, m_globalObject.get(), result));
}

template<class RejectResultType>
inline void PromiseWrapper::reject(const RejectResultType& result)
{
    JSC::ExecState* exec = m_globalObject->globalExec();
    m_promise->resolver()->rejectIfNotResolved(exec, toJS(exec, m_globalObject.get(), result));
}

template<>
inline void PromiseWrapper::reject(const std::nullptr_t&)
{
    JSC::ExecState* exec = m_globalObject->globalExec();
    m_promise->resolver()->rejectIfNotResolved(exec, JSC::jsNull());
}

template<>
inline void PromiseWrapper::fulfill<String>(const String& result)
{
    JSC::ExecState* exec = m_globalObject->globalExec();
    m_promise->resolver()->fulfillIfNotResolved(exec, jsString(exec, result));
}

template<>
inline void PromiseWrapper::fulfill<bool>(const bool& result)
{
    JSC::ExecState* exec = m_globalObject->globalExec();
    m_promise->resolver()->fulfillIfNotResolved(exec, JSC::jsBoolean(result));
}

template<>
inline void PromiseWrapper::fulfill<Vector<unsigned char>>(const Vector<unsigned char>& result)
{
    JSC::ExecState* exec = m_globalObject->globalExec();
    RefPtr<ArrayBuffer> buffer = ArrayBuffer::create(result.data(), result.size());
    m_promise->resolver()->fulfillIfNotResolved(exec, toJS(exec, m_globalObject.get(), buffer.get()));
}

template<>
inline void PromiseWrapper::reject<String>(const String& result)
{
    JSC::ExecState* exec = m_globalObject->globalExec();
    m_promise->resolver()->rejectIfNotResolved(exec, jsString(exec, result));
}

}

#endif // JSDOMPromise_h
