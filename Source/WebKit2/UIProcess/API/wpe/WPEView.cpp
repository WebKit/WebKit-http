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

#include "APIPageConfiguration.h"
#include "CompositingManagerProxy.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebTouchEvent.h"
#include "NativeWebWheelEvent.h"
#include "WebPageGroup.h"
#include "WebProcessPool.h"

using namespace WebKit;

namespace WKWPE {

View::View(const API::PageConfiguration& baseConfiguration)
    : m_pageClient(std::make_unique<PageClientImpl>(*this))
    , m_viewBackend(WPE::ViewBackend::ViewBackend::create())
    , m_size{ 800, 600 }
    , m_viewStateFlags(WebCore::ViewState::IsVisible | WebCore::ViewState::IsInWindow)
{
    auto configuration = baseConfiguration.copy();
    auto* preferences = configuration->preferences();
    if (!preferences && configuration->pageGroup()) {
        preferences = &configuration->pageGroup()->preferences();
        configuration->setPreferences(preferences);
    }
    if (preferences) {
        preferences->setAcceleratedCompositingEnabled(true);
        preferences->setForceCompositingMode(true);
        preferences->setAccelerated2dCanvasEnabled(true);
        preferences->setWebGLEnabled(true);
        preferences->setWebSecurityEnabled(false);
        preferences->setDeveloperExtrasEnabled(true);
    }

    auto* pool = configuration->processPool();
    m_pageProxy = pool->createWebPage(*m_pageClient, WTFMove(configuration));

    // Construct the CompositingManagerProxy before initializing the WebPageProxy.
    m_compositingManagerProxy = std::make_unique<CompositingManagerProxy>(*this);
    m_viewBackend->setClient(m_compositingManagerProxy.get());
    m_viewBackend->setInputClient(this);

    m_pageProxy->initializeWebPage();
}

View::~View()
{
    m_viewBackend->setClient(nullptr);
    m_viewBackend->setInputClient(nullptr);
}

void View::setSize(const WebCore::IntSize& size)
{
    m_size = size;
    if (m_pageProxy->drawingArea())
        m_pageProxy->drawingArea()->setSize(size, WebCore::IntSize(), WebCore::IntSize());
}

void View::setViewState(WebCore::ViewState::Flags flags)
{
    WebCore::ViewState::Flags changedFlags = m_viewStateFlags ^ flags;
    m_viewStateFlags = flags;

    if (changedFlags)
        m_pageProxy->viewStateDidChange(changedFlags);
}

void View::handleKeyboardEvent(WPE::Input::KeyboardEvent&& event)
{
    page().handleKeyboardEvent(WebKit::NativeWebKeyboardEvent(WTFMove(event)));
}

void View::handlePointerEvent(WPE::Input::PointerEvent&& event)
{
    page().handleMouseEvent(WebKit::NativeWebMouseEvent(WTFMove(event)));
}

void View::handleAxisEvent(WPE::Input::AxisEvent&& event)
{
    page().handleWheelEvent(WebKit::NativeWebWheelEvent(WTFMove(event)));
}

void View::handleTouchEvent(WPE::Input::TouchEvent&& event)
{
    page().handleTouchEvent(WebKit::NativeWebTouchEvent(WTFMove(event)));
}

} // namespace WKWPE
