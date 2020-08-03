/*
 * Copyright (C) 2016 Apple Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FetchBodyConsumer.h"

#include "JSBlob.h"
#include "ReadableStreamChunk.h"
#include "TextResourceDecoder.h"

namespace WebCore {

static inline Ref<Blob> blobFromData(const unsigned char* data, unsigned length, const String& contentType)
{
    Vector<uint8_t> value(length);
    memcpy(value.data(), data, length);
    return Blob::create(WTFMove(value), contentType);
}

static void resolveWithTypeAndData(Ref<DeferredPromise>&& promise, FetchBodyConsumer::Type type, const String& contentType, const unsigned char* data, unsigned length)
{
    switch (type) {
    case FetchBodyConsumer::Type::ArrayBuffer:
        fulfillPromiseWithArrayBuffer(WTFMove(promise), data, length);
        return;
    case FetchBodyConsumer::Type::Blob:
        promise->resolveCallbackValueWithNewlyCreated<IDLInterface<Blob>>([&data, &length, &contentType](auto&) {
            return blobFromData(data, length, contentType);
        });
        return;
    case FetchBodyConsumer::Type::JSON:
        fulfillPromiseWithJSON(WTFMove(promise), TextResourceDecoder::textFromUTF8(data, length));
        return;
    case FetchBodyConsumer::Type::Text:
        promise->resolve<IDLDOMString>(TextResourceDecoder::textFromUTF8(data, length));
        return;
    case FetchBodyConsumer::Type::None:
        ASSERT_NOT_REACHED();
        return;
    }
}

void FetchBodyConsumer::clean()
{
    m_buffer = nullptr;
    resetConsumePromise();
    if (m_sink) {
        m_sink->clearCallback();
        return;
    }
}

void FetchBodyConsumer::resolveWithData(Ref<DeferredPromise>&& promise, const unsigned char* data, unsigned length)
{
    resolveWithTypeAndData(WTFMove(promise), m_type, m_contentType, data, length);
}

void FetchBodyConsumer::extract(ReadableStream& stream, ReadableStreamToSharedBufferSink::Callback&& callback)
{
    ASSERT(!m_sink);
    m_sink = ReadableStreamToSharedBufferSink::create(WTFMove(callback));
    m_sink->pipeFrom(stream);
}

void FetchBodyConsumer::resolve(Ref<DeferredPromise>&& promise, ReadableStream* stream)
{
    if (stream) {
        ASSERT(!m_sink);
        m_sink = ReadableStreamToSharedBufferSink::create([promise = WTFMove(promise), data = SharedBuffer::create(), type = m_type, contentType = m_contentType](auto&& result) mutable {
            if (result.hasException()) {
                promise->reject(result.releaseException());
                return;
            }

            if (auto chunk = result.returnValue())
                data->append(reinterpret_cast<const char*>(chunk->data), chunk->size);
            else
                resolveWithTypeAndData(WTFMove(promise), type, contentType, reinterpret_cast<const unsigned char*>(data->data()), data->size());
        });
        m_sink->pipeFrom(*stream);
        return;
    }

    if (m_isLoading) {
        setConsumePromise(WTFMove(promise));
        return;
    }

    ASSERT(m_type != Type::None);
    switch (m_type) {
    case Type::ArrayBuffer:
        fulfillPromiseWithArrayBuffer(WTFMove(promise), takeAsArrayBuffer().get());
        return;
    case Type::Blob:
        promise->resolveCallbackValueWithNewlyCreated<IDLInterface<Blob>>([this](auto&) {
            return takeAsBlob();
        });
        return;
    case Type::JSON:
        fulfillPromiseWithJSON(WTFMove(promise), takeAsText());
        return;
    case Type::Text:
        promise->resolve<IDLDOMString>(takeAsText());
        return;
    case Type::None:
        ASSERT_NOT_REACHED();
        return;
    }
}

void FetchBodyConsumer::append(const char* data, unsigned size)
{
    if (m_source) {
        m_source->enqueue(ArrayBuffer::tryCreate(data, size));
        return;
    }
    if (!m_buffer) {
        m_buffer = SharedBuffer::create(data, size);
        return;
    }
    m_buffer->append(data, size);
}

void FetchBodyConsumer::append(const unsigned char* data, unsigned size)
{
    append(reinterpret_cast<const char*>(data), size);
}

RefPtr<SharedBuffer> FetchBodyConsumer::takeData()
{
    return WTFMove(m_buffer);
}

RefPtr<JSC::ArrayBuffer> FetchBodyConsumer::takeAsArrayBuffer()
{
    if (!m_buffer)
        return ArrayBuffer::tryCreate(nullptr, 0);

    auto arrayBuffer = m_buffer->tryCreateArrayBuffer();
    m_buffer = nullptr;
    return arrayBuffer;
}

Ref<Blob> FetchBodyConsumer::takeAsBlob()
{
    if (!m_buffer)
        return Blob::create(Vector<uint8_t>(), m_contentType);

    // FIXME: We should try to move m_buffer to Blob without doing extra copy.
    return blobFromData(reinterpret_cast<const unsigned char*>(m_buffer->data()), m_buffer->size(), m_contentType);
}

String FetchBodyConsumer::takeAsText()
{
    // FIXME: We could probably text decode on the fly as soon as m_type is set to JSON or Text.
    if (!m_buffer)
        return String();

    auto text = TextResourceDecoder::textFromUTF8(reinterpret_cast<const unsigned char*>(m_buffer->data()), m_buffer->size());
    m_buffer = nullptr;
    return text;
}

void FetchBodyConsumer::setConsumePromise(Ref<DeferredPromise>&& promise)
{
    ASSERT(!m_consumePromise);
    m_userGestureToken = UserGestureIndicator::currentUserGesture();
    m_consumePromise = WTFMove(promise);
}

void FetchBodyConsumer::resetConsumePromise()
{
    m_consumePromise = nullptr;
    m_userGestureToken = nullptr;
}

void FetchBodyConsumer::setSource(Ref<FetchBodySource>&& source)
{
    m_source = WTFMove(source);
    if (m_buffer) {
        m_source->enqueue(m_buffer->tryCreateArrayBuffer());
        m_buffer = nullptr;
    }
}

void FetchBodyConsumer::loadingFailed(const Exception& exception)
{
    m_isLoading = false;
    if (m_consumePromise) {
        m_consumePromise->reject(exception);
        resetConsumePromise();
    }
    if (m_source) {
        m_source->error(exception);
        m_source = nullptr;
    }
}

void FetchBodyConsumer::loadingSucceeded()
{
    m_isLoading = false;

    if (m_consumePromise) {
        if (!m_userGestureToken || m_userGestureToken->hasExpired(UserGestureToken::maximumIntervalForUserGestureForwardingForFetch()) || !m_userGestureToken->processingUserGesture())
            resolve(m_consumePromise.releaseNonNull(), nullptr);
        else {
            UserGestureIndicator gestureIndicator(m_userGestureToken, UserGestureToken::GestureScope::MediaOnly, UserGestureToken::IsPropagatedFromFetch::Yes);
            resolve(m_consumePromise.releaseNonNull(), nullptr);
        }
    }
    if (m_source) {
        m_source->close();
        m_source = nullptr;
    }
}

} // namespace WebCore
