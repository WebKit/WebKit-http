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

#include "PluginControllerProxy.h"

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "PluginControllerProxyMessages.h"
#include "ShareableBitmap.h"
#include "WebCoreArgumentCoders.h"
#include "WebEvent.h"
#include <WebCore/AffineTransform.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntSize.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void PluginControllerProxy::didReceivePluginControllerProxyMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::PluginControllerProxy::GeometryDidChange::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::GeometryDidChange>(decoder, this, &PluginControllerProxy::geometryDidChange);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::VisibilityDidChange::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::VisibilityDidChange>(decoder, this, &PluginControllerProxy::visibilityDidChange);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::FrameDidFinishLoading::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::FrameDidFinishLoading>(decoder, this, &PluginControllerProxy::frameDidFinishLoading);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::FrameDidFail::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::FrameDidFail>(decoder, this, &PluginControllerProxy::frameDidFail);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::DidEvaluateJavaScript::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::DidEvaluateJavaScript>(decoder, this, &PluginControllerProxy::didEvaluateJavaScript);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::StreamWillSendRequest::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::StreamWillSendRequest>(decoder, this, &PluginControllerProxy::streamWillSendRequest);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::StreamDidReceiveResponse::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::StreamDidReceiveResponse>(decoder, this, &PluginControllerProxy::streamDidReceiveResponse);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::StreamDidReceiveData::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::StreamDidReceiveData>(decoder, this, &PluginControllerProxy::streamDidReceiveData);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::StreamDidFinishLoading::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::StreamDidFinishLoading>(decoder, this, &PluginControllerProxy::streamDidFinishLoading);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::StreamDidFail::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::StreamDidFail>(decoder, this, &PluginControllerProxy::streamDidFail);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::ManualStreamDidReceiveResponse::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::ManualStreamDidReceiveResponse>(decoder, this, &PluginControllerProxy::manualStreamDidReceiveResponse);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::ManualStreamDidReceiveData::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::ManualStreamDidReceiveData>(decoder, this, &PluginControllerProxy::manualStreamDidReceiveData);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::ManualStreamDidFinishLoading::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::ManualStreamDidFinishLoading>(decoder, this, &PluginControllerProxy::manualStreamDidFinishLoading);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::ManualStreamDidFail::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::ManualStreamDidFail>(decoder, this, &PluginControllerProxy::manualStreamDidFail);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::HandleMouseEvent::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::HandleMouseEvent>(decoder, this, &PluginControllerProxy::handleMouseEvent);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::SetFocus::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::SetFocus>(decoder, this, &PluginControllerProxy::setFocus);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::DidUpdate::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::DidUpdate>(decoder, this, &PluginControllerProxy::didUpdate);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::WindowFocusChanged::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::WindowFocusChanged>(decoder, this, &PluginControllerProxy::windowFocusChanged);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::WindowVisibilityChanged::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::WindowVisibilityChanged>(decoder, this, &PluginControllerProxy::windowVisibilityChanged);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginControllerProxy::SendComplexTextInput::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::SendComplexTextInput>(decoder, this, &PluginControllerProxy::sendComplexTextInput);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginControllerProxy::WindowAndViewFramesChanged::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::WindowAndViewFramesChanged>(decoder, this, &PluginControllerProxy::windowAndViewFramesChanged);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::PluginControllerProxy::SetLayerHostingMode::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::SetLayerHostingMode>(decoder, this, &PluginControllerProxy::setLayerHostingMode);
        return;
    }
#endif
    if (decoder.messageName() == Messages::PluginControllerProxy::StorageBlockingStateChanged::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::StorageBlockingStateChanged>(decoder, this, &PluginControllerProxy::storageBlockingStateChanged);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::PrivateBrowsingStateChanged::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::PrivateBrowsingStateChanged>(decoder, this, &PluginControllerProxy::privateBrowsingStateChanged);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::MutedStateChanged::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::MutedStateChanged>(decoder, this, &PluginControllerProxy::mutedStateChanged);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void PluginControllerProxy::didReceiveSyncPluginControllerProxyMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder, std::unique_ptr<IPC::MessageEncoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::PluginControllerProxy::HandleWheelEvent::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::HandleWheelEvent>(decoder, *replyEncoder, this, &PluginControllerProxy::handleWheelEvent);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::HandleMouseEnterEvent::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::HandleMouseEnterEvent>(decoder, *replyEncoder, this, &PluginControllerProxy::handleMouseEnterEvent);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::HandleMouseLeaveEvent::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::HandleMouseLeaveEvent>(decoder, *replyEncoder, this, &PluginControllerProxy::handleMouseLeaveEvent);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::HandleKeyboardEvent::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::HandleKeyboardEvent>(decoder, *replyEncoder, this, &PluginControllerProxy::handleKeyboardEvent);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::HandleEditingCommand::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::HandleEditingCommand>(decoder, *replyEncoder, this, &PluginControllerProxy::handleEditingCommand);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::IsEditingCommandEnabled::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::IsEditingCommandEnabled>(decoder, *replyEncoder, this, &PluginControllerProxy::isEditingCommandEnabled);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::HandlesPageScaleFactor::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::HandlesPageScaleFactor>(decoder, *replyEncoder, this, &PluginControllerProxy::handlesPageScaleFactor);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::RequiresUnifiedScaleFactor::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::RequiresUnifiedScaleFactor>(decoder, *replyEncoder, this, &PluginControllerProxy::requiresUnifiedScaleFactor);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::PaintEntirePlugin::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::PaintEntirePlugin>(decoder, *replyEncoder, this, &PluginControllerProxy::paintEntirePlugin);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::GetPluginScriptableNPObject::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::GetPluginScriptableNPObject>(decoder, *replyEncoder, this, &PluginControllerProxy::getPluginScriptableNPObject);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::SupportsSnapshotting::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::SupportsSnapshotting>(decoder, *replyEncoder, this, &PluginControllerProxy::supportsSnapshotting);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::Snapshot::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::Snapshot>(decoder, *replyEncoder, this, &PluginControllerProxy::snapshot);
        return;
    }
    if (decoder.messageName() == Messages::PluginControllerProxy::GetFormValue::name()) {
        IPC::handleMessage<Messages::PluginControllerProxy::GetFormValue>(decoder, *replyEncoder, this, &PluginControllerProxy::getFormValue);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
