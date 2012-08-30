/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#ifndef EflViewportHandler_h
#define EflViewportHandler_h

#if USE(COORDINATED_GRAPHICS)

#include "PageClientImpl.h"
#include <wtf/PassOwnPtr.h>

namespace WebKit {

class EflViewportHandler {
public:
    static PassOwnPtr<EflViewportHandler> create(Evas_Object* viewWidget)
    {
        return adoptPtr(new EflViewportHandler(viewWidget));
    }
    ~EflViewportHandler();

    DrawingAreaProxy* drawingArea() const;
    WebCore::IntSize viewSize() { return m_viewportSize; }

    void display(const WebCore::IntRect& rect);
    void updateViewportSize(const WebCore::IntSize& viewportSize);
    void setVisibleContentsRect(const WebCore::IntPoint&, float, const WebCore::FloatPoint&);
    void didChangeContentsSize(const WebCore::IntSize& size);

private:
    explicit EflViewportHandler(Evas_Object*);

    Evas_Object* m_viewWidget;
    WebCore::IntRect m_visibleContentRect;
    WebCore::IntSize m_contentsSize;
    WebCore::IntSize m_viewportSize;
    float m_scaleFactor;
};

} // namespace WebKit

#endif

#endif // EflViewportHandler_h
