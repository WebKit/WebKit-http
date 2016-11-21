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

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "PluginProcess.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "PluginProcessCreationParameters.h"
#include "PluginProcessMessages.h"
#include <chrono>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void PluginProcess::didReceivePluginProcessMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::PluginProcess::InitializePluginProcess::name()) {
        IPC::handleMessage<Messages::PluginProcess::InitializePluginProcess>(decoder, this, &PluginProcess::initializePluginProcess);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcess::CreateWebProcessConnection::name()) {
        IPC::handleMessage<Messages::PluginProcess::CreateWebProcessConnection>(decoder, this, &PluginProcess::createWebProcessConnection);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcess::GetSitesWithData::name()) {
        IPC::handleMessage<Messages::PluginProcess::GetSitesWithData>(decoder, this, &PluginProcess::getSitesWithData);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcess::DeleteWebsiteData::name()) {
        IPC::handleMessage<Messages::PluginProcess::DeleteWebsiteData>(decoder, this, &PluginProcess::deleteWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcess::DeleteWebsiteDataForHostNames::name()) {
        IPC::handleMessage<Messages::PluginProcess::DeleteWebsiteDataForHostNames>(decoder, this, &PluginProcess::deleteWebsiteDataForHostNames);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcess::SetProcessSuppressionEnabled::name()) {
        IPC::handleMessage<Messages::PluginProcess::SetProcessSuppressionEnabled>(decoder, this, &PluginProcess::setProcessSuppressionEnabled);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProcess::SetQOS::name()) {
        IPC::handleMessage<Messages::PluginProcess::SetQOS>(decoder, this, &PluginProcess::setQOS);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
