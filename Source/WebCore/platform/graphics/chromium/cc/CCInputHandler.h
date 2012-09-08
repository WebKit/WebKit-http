/* Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef CCInputHandler_h
#define CCInputHandler_h

#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class IntPoint;
class IntSize;

// The CCInputHandler is a way for the embedders to interact with
// the impl thread side of the compositor implementation.
//
// There is one CCInputHandler for every CCLayerTreeHost. It is
// created on the main thread and used only on the impl thread.
//
// The CCInputHandler is constructed with a CCInputHandlerClient, which is the
// interface by which the handler can manipulate the LayerTree.
class CCInputHandlerClient {
    WTF_MAKE_NONCOPYABLE(CCInputHandlerClient);
public:
    enum ScrollStatus { ScrollOnMainThread, ScrollStarted, ScrollIgnored };
    enum ScrollInputType { Gesture, Wheel };

    // Selects a layer to be scrolled at a given point in window coordinates.
    // Returns ScrollStarted if the layer at the coordinates can be scrolled,
    // ScrollOnMainThread if the scroll event should instead be delegated to the
    // main thread, or ScrollIgnored if there is nothing to be scrolled at the
    // given coordinates.
    virtual ScrollStatus scrollBegin(const IntPoint&, ScrollInputType) = 0;

    // Scroll the selected layer starting at the given window coordinate. If
    // there is no room to move the layer in the requested direction, its first
    // ancestor layer that can be scrolled will be moved instead. Should only be
    // called if scrollBegin() returned ScrollStarted.
    virtual void scrollBy(const IntPoint&, const IntSize&) = 0;

    // Stop scrolling the selected layer. Should only be called if scrollBegin()
    // returned ScrollStarted.
    virtual void scrollEnd() = 0;

    virtual void pinchGestureBegin() = 0;
    virtual void pinchGestureUpdate(float magnifyDelta, const IntPoint& anchor) = 0;
    virtual void pinchGestureEnd() = 0;

    virtual void startPageScaleAnimation(const IntSize& targetPosition,
                                         bool anchorPoint,
                                         float pageScale,
                                         double startTime,
                                         double duration) = 0;

    // Request another callback to CCInputHandler::animate().
    virtual void scheduleAnimation() = 0;

protected:
    CCInputHandlerClient() { }
    virtual ~CCInputHandlerClient() { }
};

class CCInputHandler {
    WTF_MAKE_NONCOPYABLE(CCInputHandler);
public:
    virtual ~CCInputHandler() { }

    virtual void bindToClient(CCInputHandlerClient*) = 0;
    virtual void animate(double monotonicTime) = 0;

protected:
    CCInputHandler() { }
};

}

#endif
