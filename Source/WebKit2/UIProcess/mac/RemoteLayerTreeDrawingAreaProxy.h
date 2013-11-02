/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef RemoteLayerTreeDrawingAreaProxy_h
#define RemoteLayerTreeDrawingAreaProxy_h

#include "DrawingAreaProxy.h"
#include "RemoteLayerTreeHost.h"
#include <WebCore/IntPoint.h>
#include <WebCore/IntSize.h>

namespace WebKit {

class RemoteLayerTreeDrawingAreaProxy : public DrawingAreaProxy {
public:
    explicit RemoteLayerTreeDrawingAreaProxy(WebPageProxy*);
    virtual ~RemoteLayerTreeDrawingAreaProxy();

private:
    virtual void sizeDidChange() OVERRIDE;
    virtual void deviceScaleFactorDidChange() OVERRIDE;
    virtual void didUpdateGeometry() OVERRIDE;

    void sendUpdateGeometry();

    RemoteLayerTreeHost m_remoteLayerTreeHost;
    bool m_isWaitingForDidUpdateGeometry;

    WebCore::IntSize m_lastSentSize;
    WebCore::IntSize m_lastSentLayerPosition;
};

} // namespace WebKit

#endif // RemoteLayerTreeDrawingAreaProxy_h
