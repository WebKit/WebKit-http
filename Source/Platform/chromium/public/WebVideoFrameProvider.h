/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebVideoFrameProvider_h
#define WebVideoFrameProvider_h

#include "WebCommon.h"
#include "WebVideoFrame.h"

namespace WebKit {

// Threading notes: This class may be used in a multi threaded manner. Specifically, the implementation
// may call getCurrentFrame() / putCurrentFrame() from a non-main thread. If so, the caller is responsible
// for making sure Client::didReceiveFrame and Client::didUpdateMatrix are only called from this thread.
class WebVideoFrameProvider {
public:
    virtual ~WebVideoFrameProvider() { }

    class Client {
    public:
        // Provider will call this method to tell the client to stop using it.
        // stopUsingProvider() may be called from any thread. The client should
        // block until it has putCurrentFrame any outstanding frames.
        virtual void stopUsingProvider() = 0;

        // Notifies the provider's client that a call to getCurrentFrame() will return new data.
        virtual void didReceiveFrame() = 0;

        // Notifies the provider's client of a new UV transform matrix to be used when drawing frames
        // of type WebVideoFrame::FormatStreamTexture.
        virtual void didUpdateMatrix(const float*) = 0;
    };

    // May be called from any thread, but there must be some external guarantee
    // that the provider is not destroyed before this call returns.
    virtual void setVideoFrameProviderClient(Client*) = 0;

    // This function places a lock on the current frame and returns a pointer to it.
    // Calls to this method should always be followed with a call to putCurrentFrame().
    // The ownership of the object is not transferred to the caller and
    // the caller should not free the returned object. Only the current provider
    // client should call this function.
    virtual WebVideoFrame* getCurrentFrame() = 0;
    // This function releases the lock on the video frame in chromium. It should
    // always be called after getCurrentFrame(). Frames passed into this method
    // should no longer be referenced after the call is made. Only the current
    // provider client should call this function.
    virtual void putCurrentFrame(WebVideoFrame*) = 0;
};

} // namespace WebKit

#endif
