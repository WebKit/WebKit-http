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

#include "PluginProcessProxy.h"

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "PluginProcessProxyMessages.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void PluginProcessProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::PluginProcessProxy::DidCreateWebProcessConnection::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::DidCreateWebProcessConnection>(decoder, this, &PluginProcessProxy::didCreateWebProcessConnection);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcessProxy::DidGetSitesWithData::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::DidGetSitesWithData>(decoder, this, &PluginProcessProxy::didGetSitesWithData);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcessProxy::DidDeleteWebsiteData::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::DidDeleteWebsiteData>(decoder, this, &PluginProcessProxy::didDeleteWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::PluginProcessProxy::DidDeleteWebsiteDataForHostNames::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::DidDeleteWebsiteDataForHostNames>(decoder, this, &PluginProcessProxy::didDeleteWebsiteDataForHostNames);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProcessProxy::SetModalWindowIsShowing::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::SetModalWindowIsShowing>(decoder, this, &PluginProcessProxy::setModalWindowIsShowing);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProcessProxy::SetFullscreenWindowIsShowing::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::SetFullscreenWindowIsShowing>(decoder, this, &PluginProcessProxy::setFullscreenWindowIsShowing);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void PluginProcessProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProcessProxy::LaunchProcess::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::LaunchProcess>(decoder, *replyEncoder, this, &PluginProcessProxy::launchProcess);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProcessProxy::LaunchApplicationAtURL::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::LaunchApplicationAtURL>(decoder, *replyEncoder, this, &PluginProcessProxy::launchApplicationAtURL);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProcessProxy::OpenURL::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::OpenURL>(decoder, *replyEncoder, this, &PluginProcessProxy::openURL);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProcessProxy::OpenFile::name()) {
        IPC::handleMessage<Messages::PluginProcessProxy::OpenFile>(decoder, *replyEncoder, this, &PluginProcessProxy::openFile);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
