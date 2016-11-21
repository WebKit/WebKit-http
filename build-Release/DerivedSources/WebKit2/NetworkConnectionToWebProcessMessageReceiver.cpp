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

#include "NetworkConnectionToWebProcess.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "DownloadID.h"
#include "HandleMessage.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkResourceLoadParameters.h"
#include "SandboxExtension.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/BlobPart.h>
#include <WebCore/Cookie.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/SessionID.h>
#include <WebCore/URL.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace Messages {

namespace NetworkConnectionToWebProcess {

PerformSynchronousLoad::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

PerformSynchronousLoad::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool PerformSynchronousLoad::DelayedReply::send(const WebCore::ResourceError& error, const WebCore::ResourceResponse& response, const Vector<char>& data)
{
    ASSERT(m_encoder);
    *m_encoder << error;
    *m_encoder << response;
    *m_encoder << data;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

} // namespace NetworkConnectionToWebProcess

} // namespace Messages

namespace WebKit {

void NetworkConnectionToWebProcess::didReceiveNetworkConnectionToWebProcessMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::ScheduleResourceLoad::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::ScheduleResourceLoad>(decoder, this, &NetworkConnectionToWebProcess::scheduleResourceLoad);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::LoadPing::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::LoadPing>(decoder, this, &NetworkConnectionToWebProcess::loadPing);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::RemoveLoadIdentifier::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::RemoveLoadIdentifier>(decoder, this, &NetworkConnectionToWebProcess::removeLoadIdentifier);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::SetDefersLoading::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::SetDefersLoading>(decoder, this, &NetworkConnectionToWebProcess::setDefersLoading);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::PrefetchDNS::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::PrefetchDNS>(decoder, this, &NetworkConnectionToWebProcess::prefetchDNS);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::StartDownload::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::StartDownload>(decoder, this, &NetworkConnectionToWebProcess::startDownload);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::ConvertMainResourceLoadToDownload::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::ConvertMainResourceLoadToDownload>(decoder, this, &NetworkConnectionToWebProcess::convertMainResourceLoadToDownload);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::SetCookiesFromDOM::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::SetCookiesFromDOM>(decoder, this, &NetworkConnectionToWebProcess::setCookiesFromDOM);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::DeleteCookie::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::DeleteCookie>(decoder, this, &NetworkConnectionToWebProcess::deleteCookie);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::AddCookie::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::AddCookie>(decoder, this, &NetworkConnectionToWebProcess::addCookie);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::RegisterFileBlobURL::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::RegisterFileBlobURL>(decoder, this, &NetworkConnectionToWebProcess::registerFileBlobURL);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::RegisterBlobURL::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::RegisterBlobURL>(decoder, this, &NetworkConnectionToWebProcess::registerBlobURL);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::RegisterBlobURLFromURL::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::RegisterBlobURLFromURL>(decoder, this, &NetworkConnectionToWebProcess::registerBlobURLFromURL);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::PreregisterSandboxExtensionsForOptionallyFileBackedBlob::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::PreregisterSandboxExtensionsForOptionallyFileBackedBlob>(decoder, this, &NetworkConnectionToWebProcess::preregisterSandboxExtensionsForOptionallyFileBackedBlob);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::RegisterBlobURLOptionallyFileBacked::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::RegisterBlobURLOptionallyFileBacked>(decoder, this, &NetworkConnectionToWebProcess::registerBlobURLOptionallyFileBacked);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::RegisterBlobURLForSlice::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::RegisterBlobURLForSlice>(decoder, this, &NetworkConnectionToWebProcess::registerBlobURLForSlice);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::UnregisterBlobURL::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::UnregisterBlobURL>(decoder, this, &NetworkConnectionToWebProcess::unregisterBlobURL);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::WriteBlobsToTemporaryFiles::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::WriteBlobsToTemporaryFiles>(decoder, this, &NetworkConnectionToWebProcess::writeBlobsToTemporaryFiles);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::EnsureLegacyPrivateBrowsingSession::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::EnsureLegacyPrivateBrowsingSession>(decoder, this, &NetworkConnectionToWebProcess::ensureLegacyPrivateBrowsingSession);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void NetworkConnectionToWebProcess::didReceiveSyncNetworkConnectionToWebProcessMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad::name()) {
        IPC::handleMessageDelayed<Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad>(connection, decoder, replyEncoder, this, &NetworkConnectionToWebProcess::performSynchronousLoad);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::CookiesForDOM::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::CookiesForDOM>(decoder, *replyEncoder, this, &NetworkConnectionToWebProcess::cookiesForDOM);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::CookiesEnabled::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::CookiesEnabled>(decoder, *replyEncoder, this, &NetworkConnectionToWebProcess::cookiesEnabled);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue>(decoder, *replyEncoder, this, &NetworkConnectionToWebProcess::cookieRequestHeaderFieldValue);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::GetRawCookies::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::GetRawCookies>(decoder, *replyEncoder, this, &NetworkConnectionToWebProcess::getRawCookies);
        return;
    }
    if (decoder.messageName() == Messages::NetworkConnectionToWebProcess::BlobSize::name()) {
        IPC::handleMessage<Messages::NetworkConnectionToWebProcess::BlobSize>(decoder, *replyEncoder, this, &NetworkConnectionToWebProcess::blobSize);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
