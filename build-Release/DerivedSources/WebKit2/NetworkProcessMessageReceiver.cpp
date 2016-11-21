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

#include "NetworkProcess.h"

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "Decoder.h"
#include "DownloadID.h"
#include "HandleMessage.h"
#include "NetworkProcessCreationParameters.h"
#include "NetworkProcessMessages.h"
#include "SandboxExtension.h"
#include "WebCoreArgumentCoders.h"
#include "WebsiteDataFetchOption.h"
#include "WebsiteDataType.h"
#include <WebCore/CertificateInfo.h>
#if USE(SOUP)
#include <WebCore/Proxy.h>
#endif
#include <WebCore/ResourceRequest.h>
#include <WebCore/SecurityOriginData.h>
#include <WebCore/SessionID.h>
#include <chrono>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void NetworkProcess::didReceiveNetworkProcessMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::NetworkProcess::InitializeNetworkProcess::name()) {
        IPC::handleMessage<Messages::NetworkProcess::InitializeNetworkProcess>(decoder, this, &NetworkProcess::initializeNetworkProcess);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::CreateNetworkConnectionToWebProcess::name()) {
        IPC::handleMessage<Messages::NetworkProcess::CreateNetworkConnectionToWebProcess>(decoder, this, &NetworkProcess::createNetworkConnectionToWebProcess);
        return;
    }
#if USE(SOUP)
    if (decoder.messageName() == Messages::NetworkProcess::SetIgnoreTLSErrors::name()) {
        IPC::handleMessage<Messages::NetworkProcess::SetIgnoreTLSErrors>(decoder, this, &NetworkProcess::setIgnoreTLSErrors);
        return;
    }
#endif
#if USE(SOUP)
    if (decoder.messageName() == Messages::NetworkProcess::UserPreferredLanguagesChanged::name()) {
        IPC::handleMessage<Messages::NetworkProcess::UserPreferredLanguagesChanged>(decoder, this, &NetworkProcess::userPreferredLanguagesChanged);
        return;
    }
#endif
#if USE(SOUP)
    if (decoder.messageName() == Messages::NetworkProcess::SetProxies::name()) {
        IPC::handleMessage<Messages::NetworkProcess::SetProxies>(decoder, this, &NetworkProcess::setProxies);
        return;
    }
#endif
    if (decoder.messageName() == Messages::NetworkProcess::ClearCachedCredentials::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ClearCachedCredentials>(decoder, this, &NetworkProcess::clearCachedCredentials);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::EnsurePrivateBrowsingSession::name()) {
        IPC::handleMessage<Messages::NetworkProcess::EnsurePrivateBrowsingSession>(decoder, this, &NetworkProcess::ensurePrivateBrowsingSession);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::DestroyPrivateBrowsingSession::name()) {
        IPC::handleMessage<Messages::NetworkProcess::DestroyPrivateBrowsingSession>(decoder, this, &NetworkProcess::destroyPrivateBrowsingSession);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::FetchWebsiteData::name()) {
        IPC::handleMessage<Messages::NetworkProcess::FetchWebsiteData>(decoder, this, &NetworkProcess::fetchWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::DeleteWebsiteData::name()) {
        IPC::handleMessage<Messages::NetworkProcess::DeleteWebsiteData>(decoder, this, &NetworkProcess::deleteWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::DeleteWebsiteDataForOrigins::name()) {
        IPC::handleMessage<Messages::NetworkProcess::DeleteWebsiteDataForOrigins>(decoder, this, &NetworkProcess::deleteWebsiteDataForOrigins);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::DownloadRequest::name()) {
        IPC::handleMessage<Messages::NetworkProcess::DownloadRequest>(decoder, this, &NetworkProcess::downloadRequest);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::ResumeDownload::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ResumeDownload>(decoder, this, &NetworkProcess::resumeDownload);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::CancelDownload::name()) {
        IPC::handleMessage<Messages::NetworkProcess::CancelDownload>(decoder, this, &NetworkProcess::cancelDownload);
        return;
    }
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    if (decoder.messageName() == Messages::NetworkProcess::ContinueCanAuthenticateAgainstProtectionSpace::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ContinueCanAuthenticateAgainstProtectionSpace>(decoder, this, &NetworkProcess::continueCanAuthenticateAgainstProtectionSpace);
        return;
    }
#endif
#if USE(NETWORK_SESSION)
    if (decoder.messageName() == Messages::NetworkProcess::ContinueCanAuthenticateAgainstProtectionSpaceDownload::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ContinueCanAuthenticateAgainstProtectionSpaceDownload>(decoder, this, &NetworkProcess::continueCanAuthenticateAgainstProtectionSpaceDownload);
        return;
    }
#endif
#if USE(NETWORK_SESSION)
    if (decoder.messageName() == Messages::NetworkProcess::ContinueWillSendRequest::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ContinueWillSendRequest>(decoder, this, &NetworkProcess::continueWillSendRequest);
        return;
    }
#endif
#if USE(NETWORK_SESSION)
    if (decoder.messageName() == Messages::NetworkProcess::ContinueDecidePendingDownloadDestination::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ContinueDecidePendingDownloadDestination>(decoder, this, &NetworkProcess::continueDecidePendingDownloadDestination);
        return;
    }
#endif
    if (decoder.messageName() == Messages::NetworkProcess::SetProcessSuppressionEnabled::name()) {
        IPC::handleMessage<Messages::NetworkProcess::SetProcessSuppressionEnabled>(decoder, this, &NetworkProcess::setProcessSuppressionEnabled);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::NetworkProcess::SetQOS::name()) {
        IPC::handleMessage<Messages::NetworkProcess::SetQOS>(decoder, this, &NetworkProcess::setQOS);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::NetworkProcess::SetCookieStoragePartitioningEnabled::name()) {
        IPC::handleMessage<Messages::NetworkProcess::SetCookieStoragePartitioningEnabled>(decoder, this, &NetworkProcess::setCookieStoragePartitioningEnabled);
        return;
    }
#endif
    if (decoder.messageName() == Messages::NetworkProcess::AllowSpecificHTTPSCertificateForHost::name()) {
        IPC::handleMessage<Messages::NetworkProcess::AllowSpecificHTTPSCertificateForHost>(decoder, this, &NetworkProcess::allowSpecificHTTPSCertificateForHost);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::SetCanHandleHTTPSServerTrustEvaluation::name()) {
        IPC::handleMessage<Messages::NetworkProcess::SetCanHandleHTTPSServerTrustEvaluation>(decoder, this, &NetworkProcess::setCanHandleHTTPSServerTrustEvaluation);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::GetNetworkProcessStatistics::name()) {
        IPC::handleMessage<Messages::NetworkProcess::GetNetworkProcessStatistics>(decoder, this, &NetworkProcess::getNetworkProcessStatistics);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::ClearCacheForAllOrigins::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ClearCacheForAllOrigins>(decoder, this, &NetworkProcess::clearCacheForAllOrigins);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::SetCacheModel::name()) {
        IPC::handleMessage<Messages::NetworkProcess::SetCacheModel>(decoder, this, &NetworkProcess::setCacheModel);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::PrepareToSuspend::name()) {
        IPC::handleMessage<Messages::NetworkProcess::PrepareToSuspend>(decoder, this, &NetworkProcess::prepareToSuspend);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::CancelPrepareToSuspend::name()) {
        IPC::handleMessage<Messages::NetworkProcess::CancelPrepareToSuspend>(decoder, this, &NetworkProcess::cancelPrepareToSuspend);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::ProcessDidResume::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ProcessDidResume>(decoder, this, &NetworkProcess::processDidResume);
        return;
    }
    if (decoder.messageName() == Messages::NetworkProcess::DidGrantSandboxExtensionsToDatabaseProcessForBlobs::name()) {
        IPC::handleMessage<Messages::NetworkProcess::DidGrantSandboxExtensionsToDatabaseProcessForBlobs>(decoder, this, &NetworkProcess::didGrantSandboxExtensionsToDatabaseProcessForBlobs);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void NetworkProcess::didReceiveSyncNetworkProcessMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::NetworkProcess::ProcessWillSuspendImminently::name()) {
        IPC::handleMessage<Messages::NetworkProcess::ProcessWillSuspendImminently>(decoder, *replyEncoder, this, &NetworkProcess::processWillSuspendImminently);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
