/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorClient_h
#define InspectorClient_h

#include "InspectorForwarding.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>

namespace WebCore {

class InspectorController;
class Frame;
class Page;

class InspectorClient {
public:
    virtual ~InspectorClient() { }

    virtual void inspectorDestroyed() = 0;

    virtual InspectorFrontendChannel* openInspectorFrontend(InspectorController*) = 0;
    virtual void closeInspectorFrontend() = 0;
    virtual void bringFrontendToFront() = 0;
    virtual void didResizeMainFrame(Frame*) { }

    virtual void highlight() = 0;
    virtual void hideHighlight() = 0;

    virtual void showInspectorIndication() { }
    virtual void hideInspectorIndication() { }

    virtual bool canClearBrowserCache() { return false; }
    virtual void clearBrowserCache() { }
    virtual bool canClearBrowserCookies() { return false; }
    virtual void clearBrowserCookies() { }

    virtual bool overridesShowPaintRects() { return false; }
    virtual void setShowPaintRects(bool) { }

    virtual bool canShowDebugBorders() { return false; }
    virtual void setShowDebugBorders(bool) { }

    virtual bool canShowFPSCounter() { return false; }
    virtual void setShowFPSCounter(bool) { }

    virtual bool canContinuouslyPaint() { return false; }
    virtual void setContinuousPaintingEnabled(bool) { }

    virtual void didSetSearchingForNode(bool) { }

    virtual bool handleJavaScriptDialog(bool, const String*) { return false; }

    static bool doDispatchMessageOnFrontendPage(Page* frontendPage, const String& message);
};

} // namespace WebCore

#endif // !defined(InspectorClient_h)
