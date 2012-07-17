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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
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

#include "InspectorStateClient.h"
#include <wtf/Forward.h>

namespace WebCore {

class InspectorController;
class InspectorFrontendChannel;
class Frame;
class Page;

class InspectorClient : public InspectorStateClient {
public:
    virtual ~InspectorClient() { }

    virtual void inspectorDestroyed() = 0;

    virtual InspectorFrontendChannel* openInspectorFrontend(InspectorController*) = 0;
    virtual void closeInspectorFrontend() = 0;
    virtual void bringFrontendToFront() = 0;
    virtual void didResizeMainFrame(Frame*) { }

    virtual void highlight() = 0;
    virtual void hideHighlight() = 0;

    virtual bool canClearBrowserCache() { return false; }
    virtual void clearBrowserCache() { }
    virtual bool canClearBrowserCookies() { return false; }
    virtual void clearBrowserCookies() { }
    virtual void startMainThreadMonitoring() { }
    virtual void stopMainThreadMonitoring() { }

    virtual bool canOverrideDeviceMetrics() { return false; }

    virtual void overrideDeviceMetrics(int /*width*/, int /*height*/, float /*fontScaleFactor*/, bool /*fitWindow*/)
    {
        // FIXME: Platforms may want to implement this (see https://bugs.webkit.org/show_bug.cgi?id=82886).
    }
    virtual void autoZoomPageToFitWidth()
    {
        // FIXME: Platforms may want to implement this (see https://bugs.webkit.org/show_bug.cgi?id=82886).
    }

    virtual bool supportsFrameInstrumentation() { return false; }

    bool doDispatchMessageOnFrontendPage(Page* frontendPage, const String& message);
};

} // namespace WebCore

#endif // !defined(InspectorClient_h)
