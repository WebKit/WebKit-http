/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WebInspectorClient_h
#define WebInspectorClient_h

#if ENABLE(INSPECTOR)

#include "PageOverlay.h"

#include <WebCore/InspectorClient.h>

namespace WebCore {
    class GraphicsContext;
    class IntRect;
}

namespace WebKit {

class WebPage;

class WebInspectorClient : public WebCore::InspectorClient, private PageOverlay::Client {
public:
    WebInspectorClient(WebPage* page)
        : m_page(page)
        , m_highlightOverlay(0)
    {
    }

private:
    virtual void inspectorDestroyed();

    virtual void openInspectorFrontend(WebCore::InspectorController*);

    virtual void highlight();
    virtual void hideHighlight();

    virtual bool sendMessageToFrontend(const String&);

    // PageOverlay::Client
    virtual void pageOverlayDestroyed(PageOverlay*);
    virtual void willMoveToWebPage(PageOverlay*, WebPage*);
    virtual void didMoveToWebPage(PageOverlay*, WebPage*);
    virtual void drawRect(PageOverlay*, WebCore::GraphicsContext&, const WebCore::IntRect&);
    virtual bool mouseEvent(PageOverlay*, const WebMouseEvent&);

    WebPage* m_page;
    PageOverlay* m_highlightOverlay;
};

} // namespace WebKit

#endif // ENABLE(INSPECTOR)

#endif // WebInspectorClient_h
