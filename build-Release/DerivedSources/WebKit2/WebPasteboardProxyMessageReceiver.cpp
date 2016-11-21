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

#include "WebPasteboardProxy.h"

#if PLATFORM(COCOA) || PLATFORM(WPE) || PLATFORM(IOS)
#include "ArgumentCoders.h"
#endif
#include "Decoder.h"
#include "HandleMessage.h"
#if PLATFORM(COCOA) || PLATFORM(IOS)
#include "SharedMemory.h"
#endif
#if PLATFORM(COCOA) || PLATFORM(WPE) || PLATFORM(IOS)
#include "WebCoreArgumentCoders.h"
#endif
#include "WebPasteboardProxyMessages.h"
#if PLATFORM(COCOA)
#include <WebCore/Color.h>
#endif
#if PLATFORM(WPE) || PLATFORM(IOS)
#include <WebCore/Pasteboard.h>
#endif
#if PLATFORM(COCOA) || PLATFORM(WPE)
#include <wtf/Vector.h>
#endif
#if PLATFORM(COCOA) || PLATFORM(WPE) || PLATFORM(IOS)
#include <wtf/text/WTFString.h>
#endif

namespace WebKit {

void WebPasteboardProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPasteboardProxy::WriteWebContentToPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::WriteWebContentToPasteboard>(decoder, this, &WebPasteboardProxy::writeWebContentToPasteboard);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPasteboardProxy::WriteImageToPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::WriteImageToPasteboard>(decoder, this, &WebPasteboardProxy::writeImageToPasteboard);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPasteboardProxy::WriteStringToPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::WriteStringToPasteboard>(decoder, this, &WebPasteboardProxy::writeStringToPasteboard);
        return;
    }
#endif
#if PLATFORM(WPE)
    if (decoder.messageName() == Messages::WebPasteboardProxy::WriteWebContentToPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::WriteWebContentToPasteboard>(decoder, this, &WebPasteboardProxy::writeWebContentToPasteboard);
        return;
    }
#endif
#if PLATFORM(WPE)
    if (decoder.messageName() == Messages::WebPasteboardProxy::WriteStringToPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::WriteStringToPasteboard>(decoder, this, &WebPasteboardProxy::writeStringToPasteboard);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebPasteboardProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPasteboardProxy::ReadStringFromPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::ReadStringFromPasteboard>(decoder, *replyEncoder, this, &WebPasteboardProxy::readStringFromPasteboard);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPasteboardProxy::ReadURLFromPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::ReadURLFromPasteboard>(decoder, *replyEncoder, this, &WebPasteboardProxy::readURLFromPasteboard);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPasteboardProxy::ReadBufferFromPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::ReadBufferFromPasteboard>(decoder, *replyEncoder, this, &WebPasteboardProxy::readBufferFromPasteboard);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardItemsCount::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardItemsCount>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardItemsCount);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardTypes::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardTypes>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardTypes);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardPathnamesForType::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardPathnamesForType>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardPathnamesForType);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardStringForType::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardStringForType>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardStringForType);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardBufferForType::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardBufferForType>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardBufferForType);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::PasteboardCopy::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::PasteboardCopy>(decoder, *replyEncoder, this, &WebPasteboardProxy::pasteboardCopy);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardChangeCount::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardChangeCount>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardChangeCount);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardUniqueName::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardUniqueName>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardUniqueName);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardColor::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardColor>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardColor);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardURL::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardURL>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardURL);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::AddPasteboardTypes::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::AddPasteboardTypes>(decoder, *replyEncoder, this, &WebPasteboardProxy::addPasteboardTypes);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::SetPasteboardTypes::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::SetPasteboardTypes>(decoder, *replyEncoder, this, &WebPasteboardProxy::setPasteboardTypes);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::SetPasteboardPathnamesForType::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::SetPasteboardPathnamesForType>(decoder, *replyEncoder, this, &WebPasteboardProxy::setPasteboardPathnamesForType);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::SetPasteboardStringForType::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::SetPasteboardStringForType>(decoder, *replyEncoder, this, &WebPasteboardProxy::setPasteboardStringForType);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPasteboardProxy::SetPasteboardBufferForType::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::SetPasteboardBufferForType>(decoder, *replyEncoder, this, &WebPasteboardProxy::setPasteboardBufferForType);
        return;
    }
#endif
#if PLATFORM(WPE)
    if (decoder.messageName() == Messages::WebPasteboardProxy::GetPasteboardTypes::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::GetPasteboardTypes>(decoder, *replyEncoder, this, &WebPasteboardProxy::getPasteboardTypes);
        return;
    }
#endif
#if PLATFORM(WPE)
    if (decoder.messageName() == Messages::WebPasteboardProxy::ReadStringFromPasteboard::name()) {
        IPC::handleMessage<Messages::WebPasteboardProxy::ReadStringFromPasteboard>(decoder, *replyEncoder, this, &WebPasteboardProxy::readStringFromPasteboard);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
