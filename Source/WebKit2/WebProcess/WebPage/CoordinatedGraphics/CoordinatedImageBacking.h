/*
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
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


#ifndef CoordinatedImageBacking_h
#define CoordinatedImageBacking_h

#if USE(COORDINATED_GRAPHICS)
#include "Image.h"
#include "ShareableSurface.h"
#include "WebLayerTreeInfo.h"
#include <WebCore/Timer.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebKit {

class CoordinatedImageBacking : public RefCounted<CoordinatedImageBacking> {
public:
    class Coordinator {
    public:
        virtual void createImageBacking(CoordinatedImageBackingID) = 0;
        virtual void updateImageBacking(CoordinatedImageBackingID, const ShareableSurface::Handle&) = 0;
        virtual void clearImageBackingContents(CoordinatedImageBackingID) = 0;
        virtual void removeImageBacking(CoordinatedImageBackingID) = 0;
    };

    class Host {
    public:
        virtual bool imageBackingVisible() = 0;
    };

    static PassRefPtr<CoordinatedImageBacking> create(Coordinator*, PassRefPtr<WebCore::Image>);
    virtual ~CoordinatedImageBacking();

    static CoordinatedImageBackingID getCoordinatedImageBackingID(WebCore::Image*);
    CoordinatedImageBackingID id() const { return m_id; }

    void addHost(Host*);
    void removeHost(Host*);

    // When a new image is updated or an animated gif is progressed, CoordinatedGraphicsLayer calls markDirty().
    void markDirty();

    // Create, remove or update its backing.
    void update();

private:
    CoordinatedImageBacking(Coordinator*, PassRefPtr<WebCore::Image>);

    void releaseSurfaceIfNeeded();
    void updateVisibilityIfNeeded(bool& changedToVisible);
    void clearContentsTimerFired(WebCore::Timer<CoordinatedImageBacking>*);

    Coordinator* m_coordinator;
    RefPtr<WebCore::Image> m_image;
    WebCore::NativeImagePtr m_nativeImagePtr;
    CoordinatedImageBackingID m_id;
    Vector<Host*> m_hosts;

    RefPtr<ShareableSurface> m_surface;
    OwnPtr<ShareableSurface::Handle> m_handle;

    WebCore::Timer<CoordinatedImageBacking> m_clearContentsTimer;

    bool m_isDirty;
    bool m_isVisible;

};

}
#endif

#endif // CoordinatedImageBacking_H
