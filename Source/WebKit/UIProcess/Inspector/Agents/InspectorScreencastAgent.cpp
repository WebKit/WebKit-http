/*
 * Copyright (C) 2020 Microsoft Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorScreencastAgent.h"

#include "GenericCallback.h"
#include "PageClient.h"
#include "ScreencastEncoder.h"
#include "WebPageInspectorController.h"
#include "WebPageProxy.h"
#include "WebsiteDataStore.h"
#include <JavaScriptCore/InspectorFrontendRouter.h>
#include <WebCore/NotImplemented.h>
#include <wtf/RunLoop.h>
#include <wtf/UUID.h>
#include <wtf/text/Base64.h>

#if USE(CAIRO)
#include "CairoJpegEncoder.h"
#include "DrawingAreaProxyCoordinatedGraphics.h"
#include "DrawingAreaProxy.h"
#endif

#if PLATFORM(MAC)
#include <WebCore/ImageBufferUtilitiesCG.h>
#endif

namespace WebKit {

const int kMaxFramesInFlight = 1;

using namespace Inspector;

InspectorScreencastAgent::InspectorScreencastAgent(BackendDispatcher& backendDispatcher, Inspector::FrontendRouter& frontendRouter, WebPageProxy& page)
    : InspectorAgentBase("Screencast"_s)
    , m_frontendDispatcher(makeUnique<ScreencastFrontendDispatcher>(frontendRouter))
    , m_backendDispatcher(ScreencastBackendDispatcher::create(backendDispatcher, this))
    , m_page(page)
{
}

InspectorScreencastAgent::~InspectorScreencastAgent()
{
}

void InspectorScreencastAgent::didCreateFrontendAndBackend(FrontendRouter*, BackendDispatcher*)
{
}

void InspectorScreencastAgent::willDestroyFrontendAndBackend(DisconnectReason)
{
    if (!m_encoder)
        return;

    // The agent may be destroyed when the callback is invoked.
    m_encoder->finish([sessionID = m_page.websiteDataStore().sessionID(), screencastID = WTFMove(m_currentScreencastID)] {
        if (WebPageInspectorController::observer())
            WebPageInspectorController::observer()->didFinishScreencast(sessionID, screencastID);
    });

    m_encoder = nullptr;
}

#if USE(CAIRO)
void InspectorScreencastAgent::didPaint(cairo_surface_t* surface)
{
    if (m_encoder)
        m_encoder->encodeFrame(surface, m_page.drawingArea()->size());
    if (m_screencast) {
        if (m_screencastFramesInFlight > kMaxFramesInFlight)
            return;
        // Scale image to fit width / height
        WebCore::IntSize size = m_page.drawingArea()->size();
        double scale = std::min(m_screencastWidth / size.width(), m_screencastHeight / size.height());
        cairo_matrix_t transform;
        cairo_matrix_init_scale(&transform, scale, scale);

        RefPtr<cairo_surface_t> scaledSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(size.width() * scale), ceil(size.height() * scale)));
        RefPtr<cairo_t> cr = adoptRef(cairo_create(scaledSurface.get()));
        cairo_transform(cr.get(), &transform);
        cairo_set_source_surface(cr.get(), surface, 0, 0);
        cairo_paint(cr.get());

        unsigned char *data = nullptr;
        size_t len = 0;
        cairo_image_surface_write_to_jpeg_mem(scaledSurface.get(), &data, &len, m_screencastQuality);
        String result = base64Encode(data, len);
        ++m_screencastFramesInFlight;
        m_frontendDispatcher->screencastFrame(result, size.width(), size.height());
    }
}
#endif

Inspector::Protocol::ErrorStringOr<String /* screencastID */> InspectorScreencastAgent::startVideo(const String& file, int width, int height, Optional<double>&& scale)
{
    if (m_encoder)
        return makeUnexpected("Already recording"_s);

    if (width < 10 || width > 10000 || height < 10 || height > 10000)
        return makeUnexpected("Invalid size"_s);

    if (scale && (*scale <= 0 || *scale > 1))
        return makeUnexpected("Unsupported scale"_s);

    String errorString;
    m_encoder = ScreencastEncoder::create(errorString, file, WebCore::IntSize(width, height), WTFMove(scale));
    if (!m_encoder)
        return makeUnexpected(errorString);

    m_currentScreencastID = createCanonicalUUIDString();

#if PLATFORM(MAC)
    m_encoder->setOffsetTop(m_page.pageClient().browserToolbarHeight());
#endif

    kickFramesStarted();
    return { { m_currentScreencastID } };
}

void InspectorScreencastAgent::stopVideo(Ref<StopVideoCallback>&& callback)
{
    if (!m_encoder) {
        callback->sendFailure("Not recording"_s);
        return;
    }

    // The agent may be destroyed when the callback is invoked.
    m_encoder->finish([sessionID = m_page.websiteDataStore().sessionID(), screencastID = WTFMove(m_currentScreencastID), callback = WTFMove(callback)] {
        if (WebPageInspectorController::observer())
            WebPageInspectorController::observer()->didFinishScreencast(sessionID, screencastID);
        callback->sendSuccess();
    });
    m_encoder = nullptr;
    if (!m_screencast)
      m_framesAreGoing = false;
}

Inspector::Protocol::ErrorStringOr<int /* generation */> InspectorScreencastAgent::startScreencast(int width, int height, int quality)
{
    if (m_screencast)
        return makeUnexpected("Already screencasting"_s);
    m_screencast = true;
    m_screencastWidth = width;
    m_screencastHeight = height;
    m_screencastQuality = quality;
    m_screencastFramesInFlight = 0;
    ++m_screencastGeneration;
    kickFramesStarted();
    return m_screencastGeneration;
}

Inspector::Protocol::ErrorStringOr<void> InspectorScreencastAgent::screencastFrameAck(int generation)
{
    if (m_screencastGeneration != generation)
        return { };
    --m_screencastFramesInFlight;
    return { };
}

Inspector::Protocol::ErrorStringOr<void> InspectorScreencastAgent::stopScreencast()
{
    if (!m_screencast)
        return makeUnexpected("Not screencasting"_s);
    m_screencast = false;
    if (!m_encoder)
      m_framesAreGoing = false;
    return { };
}

void InspectorScreencastAgent::kickFramesStarted()
{
    if (!m_framesAreGoing) {
        m_framesAreGoing = true;
#if !PLATFORM(WPE)
        scheduleFrameEncoding();
#endif
    }
    m_page.forceRepaint([] { });
}

#if !PLATFORM(WPE)
void InspectorScreencastAgent::scheduleFrameEncoding()
{
    if (!m_encoder && !m_screencast)
        return;

    RunLoop::main().dispatchAfter(Seconds(1.0 / ScreencastEncoder::fps), [agent = makeWeakPtr(this)]() mutable {
        if (!agent)
            return;

        agent->encodeFrame();
        agent->scheduleFrameEncoding();
    });
}
#endif

#if PLATFORM(MAC)
void InspectorScreencastAgent::encodeFrame()
{
    if (!m_encoder && !m_screencast)
        return;
    RetainPtr<CGImageRef> imageRef = m_page.pageClient().takeSnapshotForAutomation();
    if (m_screencast && m_screencastFramesInFlight <= kMaxFramesInFlight) {
        auto cfData = adoptCF(CFDataCreateMutable(kCFAllocatorDefault, 0));
        WebCore::encodeImage(imageRef.get(), CFSTR("public.jpeg"), m_screencastQuality * 0.1, cfData.get());
        Vector<char> base64Data;
        base64Encode(CFDataGetBytePtr(cfData.get()), CFDataGetLength(cfData.get()), base64Data);
        ++m_screencastFramesInFlight;
        m_frontendDispatcher->screencastFrame(
            String(base64Data.data(), base64Data.size()),
            CGImageGetWidth(imageRef.get()),
            CGImageGetHeight(imageRef.get()));
    }
    if (m_encoder)
        m_encoder->encodeFrame(WTFMove(imageRef));
}
#endif

#if USE(CAIRO) && !PLATFORM(WPE)
void InspectorScreencastAgent::encodeFrame()
{
    if (!m_encoder && !m_screencast)
        return;

    if (auto* drawingArea = m_page.drawingArea())
        static_cast<DrawingAreaProxyCoordinatedGraphics*>(drawingArea)->captureFrame();
}
#endif

} // namespace WebKit
