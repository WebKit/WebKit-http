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

#include "PluginProxy.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "NPVariantData.h"
#include "PluginProxyMessages.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/HTTPHeaderMap.h>
#include <WebCore/IntRect.h>
#include <WebCore/ProtectionSpace.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void PluginProxy::didReceivePluginProxyMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::PluginProxy::LoadURL::name()) {
        IPC::handleMessage<Messages::PluginProxy::LoadURL>(decoder, this, &PluginProxy::loadURL);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::Update::name()) {
        IPC::handleMessage<Messages::PluginProxy::Update>(decoder, this, &PluginProxy::update);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::SetCookiesForURL::name()) {
        IPC::handleMessage<Messages::PluginProxy::SetCookiesForURL>(decoder, this, &PluginProxy::setCookiesForURL);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::CancelStreamLoad::name()) {
        IPC::handleMessage<Messages::PluginProxy::CancelStreamLoad>(decoder, this, &PluginProxy::cancelStreamLoad);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::ContinueStreamLoad::name()) {
        IPC::handleMessage<Messages::PluginProxy::ContinueStreamLoad>(decoder, this, &PluginProxy::continueStreamLoad);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::CancelManualStreamLoad::name()) {
        IPC::handleMessage<Messages::PluginProxy::CancelManualStreamLoad>(decoder, this, &PluginProxy::cancelManualStreamLoad);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::SetStatusbarText::name()) {
        IPC::handleMessage<Messages::PluginProxy::SetStatusbarText>(decoder, this, &PluginProxy::setStatusbarText);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProxy::PluginFocusOrWindowFocusChanged::name()) {
        IPC::handleMessage<Messages::PluginProxy::PluginFocusOrWindowFocusChanged>(decoder, this, &PluginProxy::pluginFocusOrWindowFocusChanged);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProxy::SetComplexTextInputState::name()) {
        IPC::handleMessage<Messages::PluginProxy::SetComplexTextInputState>(decoder, this, &PluginProxy::setComplexTextInputState);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginProxy::SetLayerHostingContextID::name()) {
        IPC::handleMessage<Messages::PluginProxy::SetLayerHostingContextID>(decoder, this, &PluginProxy::setLayerHostingContextID);
        return;
    }
#endif
#if PLUGIN_ARCHITECTURE(X11)
    if (decoder.messageName() == Messages::PluginProxy::WindowedPluginGeometryDidChange::name()) {
        IPC::handleMessage<Messages::PluginProxy::WindowedPluginGeometryDidChange>(decoder, this, &PluginProxy::windowedPluginGeometryDidChange);
        return;
    }
#endif
#if PLUGIN_ARCHITECTURE(X11)
    if (decoder.messageName() == Messages::PluginProxy::WindowedPluginVisibilityDidChange::name()) {
        IPC::handleMessage<Messages::PluginProxy::WindowedPluginVisibilityDidChange>(decoder, this, &PluginProxy::windowedPluginVisibilityDidChange);
        return;
    }
#endif
    if (decoder.messageName() == Messages::PluginProxy::SetPluginIsPlayingAudio::name()) {
        IPC::handleMessage<Messages::PluginProxy::SetPluginIsPlayingAudio>(decoder, this, &PluginProxy::setPluginIsPlayingAudio);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void PluginProxy::didReceiveSyncPluginProxyMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::PluginProxy::ProxiesForURL::name()) {
        IPC::handleMessage<Messages::PluginProxy::ProxiesForURL>(decoder, *replyEncoder, this, &PluginProxy::proxiesForURL);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::CookiesForURL::name()) {
        IPC::handleMessage<Messages::PluginProxy::CookiesForURL>(decoder, *replyEncoder, this, &PluginProxy::cookiesForURL);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::GetAuthenticationInfo::name()) {
        IPC::handleMessage<Messages::PluginProxy::GetAuthenticationInfo>(decoder, *replyEncoder, this, &PluginProxy::getAuthenticationInfo);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::GetPluginElementNPObject::name()) {
        IPC::handleMessage<Messages::PluginProxy::GetPluginElementNPObject>(decoder, *replyEncoder, this, &PluginProxy::getPluginElementNPObject);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::Evaluate::name()) {
        IPC::handleMessage<Messages::PluginProxy::Evaluate>(decoder, *replyEncoder, this, &PluginProxy::evaluate);
        return;
    }
#if PLUGIN_ARCHITECTURE(X11)
    if (decoder.messageName() == Messages::PluginProxy::CreatePluginContainer::name()) {
        IPC::handleMessage<Messages::PluginProxy::CreatePluginContainer>(decoder, *replyEncoder, this, &PluginProxy::createPluginContainer);
        return;
    }
#endif
    if (decoder.messageName() == Messages::PluginProxy::DidCreatePlugin::name()) {
        IPC::handleMessage<Messages::PluginProxy::DidCreatePlugin>(decoder, *replyEncoder, this, &PluginProxy::didCreatePlugin);
        return;
    }
    if (decoder.messageName() == Messages::PluginProxy::DidFailToCreatePlugin::name()) {
        IPC::handleMessage<Messages::PluginProxy::DidFailToCreatePlugin>(decoder, *replyEncoder, this, &PluginProxy::didFailToCreatePlugin);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
