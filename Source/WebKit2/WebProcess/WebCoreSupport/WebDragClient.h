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

#ifndef WebDragClient_h
#define WebDragClient_h

#include <WebCore/DragClient.h>

#if PLATFORM(MAC)
#ifdef __OBJC__
@class WKPasteboardFilePromiseOwner;
@class WKPasteboardOwner;
#else
class WKPasteboardFilePromiseOwner;
class WKPasteboardOwner;
#endif
#endif

namespace WebKit {

class WebPage;

class WebDragClient : public WebCore::DragClient {
public:
    WebDragClient(WebPage* page)
        : m_page(page)
    {
    }

private:
    virtual void willPerformDragDestinationAction(WebCore::DragDestinationAction, WebCore::DragData*);
    virtual void willPerformDragSourceAction(WebCore::DragSourceAction, const WebCore::IntPoint&, WebCore::Clipboard*);
    virtual WebCore::DragDestinationAction actionMaskForDrag(WebCore::DragData*);
    virtual WebCore::DragSourceAction dragSourceActionMaskForPoint(const WebCore::IntPoint& windowPoint);

    virtual void startDrag(WebCore::DragImageRef dragImage, const WebCore::IntPoint& dragImageOrigin, const WebCore::IntPoint& eventPos, WebCore::Clipboard*, WebCore::Frame*, bool linkDrag = false);

#if PLATFORM(MAC)
    virtual void declareAndWriteDragImage(NSPasteboard*, DOMElement*, NSURL*, NSString*, WebCore::Frame*);
#endif

    virtual void dragEnded();

    virtual void dragControllerDestroyed();

    WebPage* m_page;
    
#if PLATFORM(MAC)
    RetainPtr<WKPasteboardFilePromiseOwner> m_filePromiseOwner;
    RetainPtr<WKPasteboardOwner> m_pasteboardOwner;
#endif
};

} // namespace WebKit

#endif // WebDragClient_h
