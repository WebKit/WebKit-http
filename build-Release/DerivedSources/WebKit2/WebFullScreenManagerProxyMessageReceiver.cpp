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

#if ENABLE(FULLSCREEN_API)

#include "WebFullScreenManagerProxy.h"

#include "Decoder.h"
#include "HandleMessage.h"
#include "WebCoreArgumentCoders.h"
#include "WebFullScreenManagerProxyMessages.h"
#include <WebCore/IntRect.h>

namespace WebKit {

void WebFullScreenManagerProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebFullScreenManagerProxy::EnterFullScreen::name()) {
        IPC::handleMessage<Messages::WebFullScreenManagerProxy::EnterFullScreen>(decoder, this, &WebFullScreenManagerProxy::enterFullScreen);
        return;
    }
    if (decoder.messageName() == Messages::WebFullScreenManagerProxy::ExitFullScreen::name()) {
        IPC::handleMessage<Messages::WebFullScreenManagerProxy::ExitFullScreen>(decoder, this, &WebFullScreenManagerProxy::exitFullScreen);
        return;
    }
    if (decoder.messageName() == Messages::WebFullScreenManagerProxy::BeganEnterFullScreen::name()) {
        IPC::handleMessage<Messages::WebFullScreenManagerProxy::BeganEnterFullScreen>(decoder, this, &WebFullScreenManagerProxy::beganEnterFullScreen);
        return;
    }
    if (decoder.messageName() == Messages::WebFullScreenManagerProxy::BeganExitFullScreen::name()) {
        IPC::handleMessage<Messages::WebFullScreenManagerProxy::BeganExitFullScreen>(decoder, this, &WebFullScreenManagerProxy::beganExitFullScreen);
        return;
    }
    if (decoder.messageName() == Messages::WebFullScreenManagerProxy::Close::name()) {
        IPC::handleMessage<Messages::WebFullScreenManagerProxy::Close>(decoder, this, &WebFullScreenManagerProxy::close);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebFullScreenManagerProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::WebFullScreenManagerProxy::SupportsFullScreen::name()) {
        IPC::handleMessage<Messages::WebFullScreenManagerProxy::SupportsFullScreen>(decoder, *replyEncoder, this, &WebFullScreenManagerProxy::supportsFullScreen);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(FULLSCREEN_API)
