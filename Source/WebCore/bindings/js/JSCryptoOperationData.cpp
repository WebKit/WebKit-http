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

#include "config.h"
#include "JSCryptoOperationData.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "JSDOMBinding.h"

using namespace JSC;

namespace WebCore {

bool sequenceOfCryptoOperationDataFromJSValue(ExecState* exec, JSValue value, Vector<CryptoOperationData>& result)
{
    unsigned sequenceLength;
    JSObject* sequence = toJSSequence(exec, value, sequenceLength);
    if (!sequence) {
        ASSERT(exec->hadException());
        return false;
    }

    for (unsigned i = 0; i < sequenceLength; ++i) {
        JSValue item = sequence->get(exec, i);
        if (ArrayBuffer* buffer = toArrayBuffer(item))
            result.append(std::make_pair(static_cast<char*>(buffer->data()), buffer->byteLength()));
        else if (RefPtr<ArrayBufferView> bufferView = toArrayBufferView(item))
            result.append(std::make_pair(static_cast<char*>(bufferView->baseAddress()), bufferView->byteLength()));
        else {
            throwTypeError(exec, "Only ArrayBuffer and ArrayBufferView objects can be part of CryptoOperationData sequence");
            return false;
        }
    }
    return true;
}

bool cryptoOperationDataFromJSValue(ExecState* exec, JSValue value, CryptoOperationData& result)
{
    if (ArrayBuffer* buffer = toArrayBuffer(value))
        result = std::make_pair(static_cast<char*>(buffer->data()), buffer->byteLength());
    else if (RefPtr<ArrayBufferView> bufferView = toArrayBufferView(value))
        result = std::make_pair(static_cast<char*>(bufferView->baseAddress()), bufferView->byteLength());
    else {
        throwTypeError(exec, "Only ArrayBuffer and ArrayBufferView objects can be passed as CryptoOperationData");
        return false;
    }
    return true;
}

} // namespace WebCore

#endif // ENABLE(SUBTLE_CRYPTO)
