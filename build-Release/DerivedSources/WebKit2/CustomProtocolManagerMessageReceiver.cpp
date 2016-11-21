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

#include "CustomProtocolManager.h"

#include "ArgumentCoders.h"
#include "CustomProtocolManagerMessages.h"
#include "DataReference.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void CustomProtocolManager::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::CustomProtocolManager::DidFailWithError::name()) {
        IPC::handleMessage<Messages::CustomProtocolManager::DidFailWithError>(decoder, this, &CustomProtocolManager::didFailWithError);
        return;
    }
    if (decoder.messageName() == Messages::CustomProtocolManager::DidLoadData::name()) {
        IPC::handleMessage<Messages::CustomProtocolManager::DidLoadData>(decoder, this, &CustomProtocolManager::didLoadData);
        return;
    }
    if (decoder.messageName() == Messages::CustomProtocolManager::DidReceiveResponse::name()) {
        IPC::handleMessage<Messages::CustomProtocolManager::DidReceiveResponse>(decoder, this, &CustomProtocolManager::didReceiveResponse);
        return;
    }
    if (decoder.messageName() == Messages::CustomProtocolManager::DidFinishLoading::name()) {
        IPC::handleMessage<Messages::CustomProtocolManager::DidFinishLoading>(decoder, this, &CustomProtocolManager::didFinishLoading);
        return;
    }
    if (decoder.messageName() == Messages::CustomProtocolManager::WasRedirectedToRequest::name()) {
        IPC::handleMessage<Messages::CustomProtocolManager::WasRedirectedToRequest>(decoder, this, &CustomProtocolManager::wasRedirectedToRequest);
        return;
    }
    if (decoder.messageName() == Messages::CustomProtocolManager::RegisterScheme::name()) {
        IPC::handleMessage<Messages::CustomProtocolManager::RegisterScheme>(decoder, this, &CustomProtocolManager::registerScheme);
        return;
    }
    if (decoder.messageName() == Messages::CustomProtocolManager::UnregisterScheme::name()) {
        IPC::handleMessage<Messages::CustomProtocolManager::UnregisterScheme>(decoder, this, &CustomProtocolManager::unregisterScheme);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
