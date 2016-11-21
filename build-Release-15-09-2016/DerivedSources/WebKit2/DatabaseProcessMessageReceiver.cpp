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

#if ENABLE(DATABASE_PROCESS)

#include "DatabaseProcess.h"

#include "ArgumentCoders.h"
#include "DatabaseProcessCreationParameters.h"
#include "DatabaseProcessMessages.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "SandboxExtension.h"
#include "WebCoreArgumentCoders.h"
#include "WebsiteDataType.h"
#include <WebCore/SecurityOriginData.h>
#include <WebCore/SessionID.h>
#include <chrono>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void DatabaseProcess::didReceiveDatabaseProcessMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::DatabaseProcess::InitializeDatabaseProcess::name()) {
        IPC::handleMessage<Messages::DatabaseProcess::InitializeDatabaseProcess>(decoder, this, &DatabaseProcess::initializeDatabaseProcess);
        return;
    }
    if (decoder.messageName() == Messages::DatabaseProcess::CreateDatabaseToWebProcessConnection::name()) {
        IPC::handleMessage<Messages::DatabaseProcess::CreateDatabaseToWebProcessConnection>(decoder, this, &DatabaseProcess::createDatabaseToWebProcessConnection);
        return;
    }
    if (decoder.messageName() == Messages::DatabaseProcess::FetchWebsiteData::name()) {
        IPC::handleMessage<Messages::DatabaseProcess::FetchWebsiteData>(decoder, this, &DatabaseProcess::fetchWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::DatabaseProcess::DeleteWebsiteData::name()) {
        IPC::handleMessage<Messages::DatabaseProcess::DeleteWebsiteData>(decoder, this, &DatabaseProcess::deleteWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::DatabaseProcess::DeleteWebsiteDataForOrigins::name()) {
        IPC::handleMessage<Messages::DatabaseProcess::DeleteWebsiteDataForOrigins>(decoder, this, &DatabaseProcess::deleteWebsiteDataForOrigins);
        return;
    }
    if (decoder.messageName() == Messages::DatabaseProcess::GrantSandboxExtensionsForBlobs::name()) {
        IPC::handleMessage<Messages::DatabaseProcess::GrantSandboxExtensionsForBlobs>(decoder, this, &DatabaseProcess::grantSandboxExtensionsForBlobs);
        return;
    }
    if (decoder.messageName() == Messages::DatabaseProcess::DidGetSandboxExtensionsForBlobFiles::name()) {
        IPC::handleMessage<Messages::DatabaseProcess::DidGetSandboxExtensionsForBlobFiles>(decoder, this, &DatabaseProcess::didGetSandboxExtensionsForBlobFiles);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(DATABASE_PROCESS)
