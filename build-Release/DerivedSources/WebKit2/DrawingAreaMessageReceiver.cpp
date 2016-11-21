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

#include "DrawingArea.h"

#if PLATFORM(COCOA)
#include "ArgumentCoders.h"
#endif
#if PLATFORM(COCOA)
#include "ColorSpaceData.h"
#endif
#include "Decoder.h"
#include "DrawingAreaMessages.h"
#include "HandleMessage.h"
#include "WebCoreArgumentCoders.h"
#if PLATFORM(COCOA)
#include <WebCore/FloatPoint.h>
#endif
#if PLATFORM(COCOA)
#include <WebCore/FloatRect.h>
#endif
#include <WebCore/IntSize.h>
#if PLATFORM(COCOA)
#include <WebCore/MachSendRight.h>
#endif
#if PLATFORM(COCOA)
#include <wtf/Optional.h>
#endif
#if PLATFORM(COCOA)
#include <wtf/text/WTFString.h>
#endif

namespace WebKit {

void DrawingArea::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::DrawingArea::UpdateBackingStoreState::name()) {
        IPC::handleMessage<Messages::DrawingArea::UpdateBackingStoreState>(decoder, this, &DrawingArea::updateBackingStoreState);
        return;
    }
    if (decoder.messageName() == Messages::DrawingArea::DidUpdate::name()) {
        IPC::handleMessage<Messages::DrawingArea::DidUpdate>(decoder, this, &DrawingArea::didUpdate);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::UpdateGeometry::name()) {
        IPC::handleMessage<Messages::DrawingArea::UpdateGeometry>(decoder, this, &DrawingArea::updateGeometry);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::SetDeviceScaleFactor::name()) {
        IPC::handleMessage<Messages::DrawingArea::SetDeviceScaleFactor>(decoder, this, &DrawingArea::setDeviceScaleFactor);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::SetColorSpace::name()) {
        IPC::handleMessage<Messages::DrawingArea::SetColorSpace>(decoder, this, &DrawingArea::setColorSpace);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::SetViewExposedRect::name()) {
        IPC::handleMessage<Messages::DrawingArea::SetViewExposedRect>(decoder, this, &DrawingArea::setViewExposedRect);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::AdjustTransientZoom::name()) {
        IPC::handleMessage<Messages::DrawingArea::AdjustTransientZoom>(decoder, this, &DrawingArea::adjustTransientZoom);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::CommitTransientZoom::name()) {
        IPC::handleMessage<Messages::DrawingArea::CommitTransientZoom>(decoder, this, &DrawingArea::commitTransientZoom);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::AcceleratedAnimationDidStart::name()) {
        IPC::handleMessage<Messages::DrawingArea::AcceleratedAnimationDidStart>(decoder, this, &DrawingArea::acceleratedAnimationDidStart);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::AcceleratedAnimationDidEnd::name()) {
        IPC::handleMessage<Messages::DrawingArea::AcceleratedAnimationDidEnd>(decoder, this, &DrawingArea::acceleratedAnimationDidEnd);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::DrawingArea::AddTransactionCallbackID::name()) {
        IPC::handleMessage<Messages::DrawingArea::AddTransactionCallbackID>(decoder, this, &DrawingArea::addTransactionCallbackID);
        return;
    }
#endif
#if USE(TEXTURE_MAPPER) && PLATFORM(GTK) && !USE(REDIRECTED_XCOMPOSITE_WINDOW)
    if (decoder.messageName() == Messages::DrawingArea::SetNativeSurfaceHandleForCompositing::name()) {
        IPC::handleMessage<Messages::DrawingArea::SetNativeSurfaceHandleForCompositing>(decoder, this, &DrawingArea::setNativeSurfaceHandleForCompositing);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void DrawingArea::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
#if USE(TEXTURE_MAPPER) && PLATFORM(GTK) && !USE(REDIRECTED_XCOMPOSITE_WINDOW)
    if (decoder.messageName() == Messages::DrawingArea::DestroyNativeSurfaceHandleForCompositing::name()) {
        IPC::handleMessage<Messages::DrawingArea::DestroyNativeSurfaceHandleForCompositing>(decoder, *replyEncoder, this, &DrawingArea::destroyNativeSurfaceHandleForCompositing);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
