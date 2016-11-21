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

#include "NetworkProcessProxy.h"

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "NetworkProcessProxyMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebsiteData.h"
#include <WebCore/AuthenticationChallenge.h>
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
#include <WebCore/ProtectionSpace.h>
#endif
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void NetworkProcessProxy::didReceiveNetworkProcessProxyMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::NetworkProcessProxy::DidCreateNetworkConnectionToWebProcess::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::DidCreateNetworkConnectionToWebProcess>(decoder, this, &NetworkProcessProxy::didCreateNetworkConnectionToWebProcess);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::DidReceiveAuthenticationChallenge::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::DidReceiveAuthenticationChallenge>(decoder, this, &NetworkProcessProxy::didReceiveAuthenticationChallenge);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::DidFetchWebsiteData::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::DidFetchWebsiteData>(decoder, this, &NetworkProcessProxy::didFetchWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::DidDeleteWebsiteData::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::DidDeleteWebsiteData>(decoder, this, &NetworkProcessProxy::didDeleteWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::DidDeleteWebsiteDataForOrigins::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::DidDeleteWebsiteDataForOrigins>(decoder, this, &NetworkProcessProxy::didDeleteWebsiteDataForOrigins);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::GrantSandboxExtensionsToDatabaseProcessForBlobs::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::GrantSandboxExtensionsToDatabaseProcessForBlobs>(decoder, this, &NetworkProcessProxy::grantSandboxExtensionsToDatabaseProcessForBlobs);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::ProcessReadyToSuspend::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::ProcessReadyToSuspend>(decoder, this, &NetworkProcessProxy::processReadyToSuspend);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::SetIsHoldingLockedFiles::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::SetIsHoldingLockedFiles>(decoder, this, &NetworkProcessProxy::setIsHoldingLockedFiles);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::LogSampledDiagnosticMessage::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::LogSampledDiagnosticMessage>(decoder, this, &NetworkProcessProxy::logSampledDiagnosticMessage);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::LogSampledDiagnosticMessageWithResult::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::LogSampledDiagnosticMessageWithResult>(decoder, this, &NetworkProcessProxy::logSampledDiagnosticMessageWithResult);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcessProxy::LogSampledDiagnosticMessageWithValue::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::LogSampledDiagnosticMessageWithValue>(decoder, this, &NetworkProcessProxy::logSampledDiagnosticMessageWithValue);
        return;
    }
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    if (decoder.messageName() == Messages::NetworkProcessProxy::CanAuthenticateAgainstProtectionSpace::name()) {
        IPC::handleMessage<Messages::NetworkProcessProxy::CanAuthenticateAgainstProtectionSpace>(decoder, this, &NetworkProcessProxy::canAuthenticateAgainstProtectionSpace);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
