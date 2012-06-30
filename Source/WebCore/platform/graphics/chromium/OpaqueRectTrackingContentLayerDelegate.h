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

#ifndef OpaqueRectTrackingContentLayerDelegate_h
#define OpaqueRectTrackingContentLayerDelegate_h

#include <public/WebContentLayerClient.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

class SkCanvas;

namespace WebCore {

class GraphicsContext;
class IntRect;

class GraphicsContextPainter {
public:
    virtual void paint(GraphicsContext&, const IntRect& clip) = 0;

protected:
    virtual ~GraphicsContextPainter() { }
};

class OpaqueRectTrackingContentLayerDelegate : public WebKit::WebContentLayerClient {
    WTF_MAKE_NONCOPYABLE(OpaqueRectTrackingContentLayerDelegate);
public:
    explicit OpaqueRectTrackingContentLayerDelegate(GraphicsContextPainter*);
    virtual ~OpaqueRectTrackingContentLayerDelegate();

    // If we know that everything that will be painted through this delegate, then we don't bother
    // tracking opaqueness.
    void setOpaque(bool opaque) { m_opaque = opaque; }

    // WebKit::WebContentLayerClient implementation.
    virtual void paintContents(SkCanvas*, const WebKit::WebRect& clip, WebKit::WebFloatRect& opaque) OVERRIDE;

private:
    GraphicsContextPainter* m_painter;
    bool m_opaque;
};

}

#endif // OpaqueRectTrackingContentLayerDelegate_h
