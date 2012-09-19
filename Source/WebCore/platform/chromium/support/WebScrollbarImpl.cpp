/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WebScrollbarImpl.h"

#include "IntRect.h"
#include "Scrollbar.h"

namespace WebKit {

WebScrollbarImpl::WebScrollbarImpl(WebCore::Scrollbar* scrollbar)
    : m_scrollbar(scrollbar)
{
}

bool WebScrollbarImpl::isOverlay() const
{
    return m_scrollbar->isOverlayScrollbar();
}

int WebScrollbarImpl::value() const
{
    return m_scrollbar->value();
}

WebPoint WebScrollbarImpl::location() const
{
    return m_scrollbar->location();
}

WebSize WebScrollbarImpl::size() const
{
    return m_scrollbar->size();
}

bool WebScrollbarImpl::enabled() const
{
    return m_scrollbar->enabled();
}

int WebScrollbarImpl::maximum() const
{
    return m_scrollbar->maximum();
}

int WebScrollbarImpl::totalSize() const
{
    return m_scrollbar->totalSize();
}

bool WebScrollbarImpl::isScrollViewScrollbar() const
{
    return m_scrollbar->isScrollViewScrollbar();
}

bool WebScrollbarImpl::isScrollableAreaActive() const
{
    return m_scrollbar->isScrollableAreaActive();
}

void WebScrollbarImpl::getTickmarks(WebVector<WebRect>& webTickmarks) const
{
    Vector<WebCore::IntRect> tickmarks;
    m_scrollbar->getTickmarks(tickmarks);

    WebVector<WebRect> result(tickmarks.size());
    for (size_t i = 0; i < tickmarks.size(); ++i)
        result[i] = tickmarks[i];

    webTickmarks.swap(result);
}

WebScrollbar::ScrollbarControlSize WebScrollbarImpl::controlSize() const
{
    return static_cast<WebScrollbar::ScrollbarControlSize>(m_scrollbar->controlSize());
}

WebScrollbar::ScrollbarPart WebScrollbarImpl::pressedPart() const
{
    return static_cast<WebScrollbar::ScrollbarPart>(m_scrollbar->pressedPart());
}

WebScrollbar::ScrollbarPart WebScrollbarImpl::hoveredPart() const
{
    return static_cast<WebScrollbar::ScrollbarPart>(m_scrollbar->hoveredPart());
}

WebScrollbar::ScrollbarOverlayStyle WebScrollbarImpl::scrollbarOverlayStyle() const
{
    return static_cast<WebScrollbar::ScrollbarOverlayStyle>(m_scrollbar->scrollbarOverlayStyle());
}

WebScrollbar::Orientation WebScrollbarImpl::orientation() const
{
    return static_cast<WebScrollbar::Orientation>(m_scrollbar->orientation());
}

bool WebScrollbarImpl::isCustomScrollbar() const
{
    return m_scrollbar->isCustomScrollbar();
}

bool WebScrollbarImpl::isAlphaLocked() const
{
    return m_scrollbar->isAlphaLocked();
}

void WebScrollbarImpl::setIsAlphaLocked(bool flag)
{
    m_scrollbar->setIsAlphaLocked(flag);
}

} // namespace WebKit
