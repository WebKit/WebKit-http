/*
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WPEView.h"

#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebTouchEvent.h"
#include "NativeWebWheelEvent.h"
#include "WebPageGroup.h"
#include "WebProcessPool.h"

using namespace WebKit;

namespace WKWPE {

View::View(WebProcessPool* pool, WebPageGroup* pageGroup)
    : m_pageClient(std::make_unique<PageClientImpl>(*this))
    , m_size{800, 600}
{
    pageGroup->preferences().setAcceleratedCompositingEnabled(true);
    pageGroup->preferences().setForceCompositingMode(true);
    pageGroup->preferences().setAccelerated2dCanvasEnabled(true);
    pageGroup->preferences().setWebGLEnabled(true);
    pageGroup->preferences().setLogsPageMessagesToSystemConsoleEnabled(true);
    pageGroup->preferences().setWebSecurityEnabled(false);
    pageGroup->preferences().setDeveloperExtrasEnabled(true);

    WebPageConfiguration configuration;
    configuration.preferences = nullptr;
    configuration.pageGroup = pageGroup;
    configuration.relatedPage = nullptr;
    configuration.userContentController = nullptr;
    m_pageProxy = pool->createWebPage(*m_pageClient, WTF::move(configuration));
    m_pageProxy->initializeWebPage();
}

void View::setSize(const WebCore::IntSize& size)
{
    m_size = size;
    if (m_pageProxy->drawingArea())
        m_pageProxy->drawingArea()->setSize(size, WebCore::IntSize(), WebCore::IntSize());
}

void View::makeWPEInputTarget()
{
    WPE::Input::Server::singleton().setTarget(this);
}

void View::handleKeyboardEvent(WPE::Input::KeyboardEvent&& event)
{
    page().handleKeyboardEvent(WebKit::NativeWebKeyboardEvent(WTF::move(event)));
}

void View::handlePointerEvent(WPE::Input::PointerEvent&& event)
{
    page().handleMouseEvent(WebKit::NativeWebMouseEvent(WTF::move(event)));
}

void View::handleAxisEvent(WPE::Input::AxisEvent&& event)
{
    page().handleWheelEvent(WebKit::NativeWebWheelEvent(WTF::move(event)));
}

void View::handleTouchEvent(WPE::Input::TouchEvent&& event)
{
    page().handleTouchEvent(WebKit::NativeWebTouchEvent(WTF::move(event)));
}

} // namespace WKWPE
