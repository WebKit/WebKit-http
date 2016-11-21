/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WebResourceLoader.h"

#include "DataReference.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#if ENABLE(SHAREABLE_RESOURCE)
#include "ShareableResource.h"
#endif
#include "WebCoreArgumentCoders.h"
#include "WebResourceLoaderMessages.h"
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
#include <WebCore/ProtectionSpace.h>
#endif
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>

namespace WebKit {

void WebResourceLoader::didReceiveWebResourceLoaderMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebResourceLoader::WillSendRequest::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::WillSendRequest>(decoder, this, &WebResourceLoader::willSendRequest);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidSendData::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidSendData>(decoder, this, &WebResourceLoader::didSendData);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidReceiveResponse::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidReceiveResponse>(decoder, this, &WebResourceLoader::didReceiveResponse);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidReceiveData::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidReceiveData>(decoder, this, &WebResourceLoader::didReceiveData);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidFinishResourceLoad::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidFinishResourceLoad>(decoder, this, &WebResourceLoader::didFinishResourceLoad);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidFailResourceLoad::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidFailResourceLoad>(decoder, this, &WebResourceLoader::didFailResourceLoad);
        return;
    }
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    if (decoder.messageName() == Messages::WebResourceLoader::CanAuthenticateAgainstProtectionSpace::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::CanAuthenticateAgainstProtectionSpace>(decoder, this, &WebResourceLoader::canAuthenticateAgainstProtectionSpace);
        return;
    }
#endif
#if ENABLE(SHAREABLE_RESOURCE)
    if (decoder.messageName() == Messages::WebResourceLoader::DidReceiveResource::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidReceiveResource>(decoder, this, &WebResourceLoader::didReceiveResource);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
