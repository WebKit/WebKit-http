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

#include "DownloadProxy.h"

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "Decoder.h"
#if USE(NETWORK_SESSION)
#include "DownloadID.h"
#endif
#include "DownloadProxyMessages.h"
#include "HandleMessage.h"
#if !USE(NETWORK_SESSION)
#include "SandboxExtension.h"
#endif
#include "WebCoreArgumentCoders.h"
#include <WebCore/AuthenticationChallenge.h>
#if USE(NETWORK_SESSION)
#include <WebCore/ProtectionSpace.h>
#endif
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void DownloadProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::DownloadProxy::DidStart::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidStart>(decoder, this, &DownloadProxy::didStart);
        return;
    }
    if (decoder.messageName() == Messages::DownloadProxy::DidReceiveAuthenticationChallenge::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidReceiveAuthenticationChallenge>(decoder, this, &DownloadProxy::didReceiveAuthenticationChallenge);
        return;
    }
#if USE(NETWORK_SESSION)
    if (decoder.messageName() == Messages::DownloadProxy::WillSendRequest::name()) {
        IPC::handleMessage<Messages::DownloadProxy::WillSendRequest>(decoder, this, &DownloadProxy::willSendRequest);
        return;
    }
#endif
#if USE(NETWORK_SESSION)
    if (decoder.messageName() == Messages::DownloadProxy::CanAuthenticateAgainstProtectionSpace::name()) {
        IPC::handleMessage<Messages::DownloadProxy::CanAuthenticateAgainstProtectionSpace>(decoder, this, &DownloadProxy::canAuthenticateAgainstProtectionSpace);
        return;
    }
#endif
#if USE(NETWORK_SESSION)
    if (decoder.messageName() == Messages::DownloadProxy::DecideDestinationWithSuggestedFilenameAsync::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DecideDestinationWithSuggestedFilenameAsync>(decoder, this, &DownloadProxy::decideDestinationWithSuggestedFilenameAsync);
        return;
    }
#endif
    if (decoder.messageName() == Messages::DownloadProxy::DidReceiveResponse::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidReceiveResponse>(decoder, this, &DownloadProxy::didReceiveResponse);
        return;
    }
    if (decoder.messageName() == Messages::DownloadProxy::DidReceiveData::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidReceiveData>(decoder, this, &DownloadProxy::didReceiveData);
        return;
    }
    if (decoder.messageName() == Messages::DownloadProxy::DidCreateDestination::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidCreateDestination>(decoder, this, &DownloadProxy::didCreateDestination);
        return;
    }
    if (decoder.messageName() == Messages::DownloadProxy::DidFinish::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidFinish>(decoder, this, &DownloadProxy::didFinish);
        return;
    }
    if (decoder.messageName() == Messages::DownloadProxy::DidFail::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidFail>(decoder, this, &DownloadProxy::didFail);
        return;
    }
    if (decoder.messageName() == Messages::DownloadProxy::DidCancel::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DidCancel>(decoder, this, &DownloadProxy::didCancel);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void DownloadProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::DownloadProxy::ShouldDecodeSourceDataOfMIMEType::name()) {
        IPC::handleMessage<Messages::DownloadProxy::ShouldDecodeSourceDataOfMIMEType>(decoder, *replyEncoder, this, &DownloadProxy::shouldDecodeSourceDataOfMIMEType);
        return;
    }
#if !USE(NETWORK_SESSION)
    if (decoder.messageName() == Messages::DownloadProxy::DecideDestinationWithSuggestedFilename::name()) {
        IPC::handleMessage<Messages::DownloadProxy::DecideDestinationWithSuggestedFilename>(decoder, *replyEncoder, this, &DownloadProxy::decideDestinationWithSuggestedFilename);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
