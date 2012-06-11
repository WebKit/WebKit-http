/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef WebMediaPlayerClient_h
#define WebMediaPlayerClient_h

#include "WebMediaPlayer.h"

namespace WebKit {

class WebFrame;
class WebPlugin;
class WebRequest;
class WebURL;

class WebMediaPlayerClient {
public:
    enum MediaKeyErrorCode {
        MediaKeyErrorCodeUnknown = 1,
        MediaKeyErrorCodeClient,
        MediaKeyErrorCodeService,
        MediaKeyErrorCodeOutput,
        MediaKeyErrorCodeHardwareChange,
        MediaKeyErrorCodeDomain,
        UnknownError = MediaKeyErrorCodeUnknown,
        ClientError = MediaKeyErrorCodeClient,
        ServiceError = MediaKeyErrorCodeService,
        OutputError = MediaKeyErrorCodeOutput,
        HardwareChangeError = MediaKeyErrorCodeHardwareChange,
        DomainError = MediaKeyErrorCodeDomain,
    };

    virtual void networkStateChanged() = 0;
    virtual void readyStateChanged() = 0;
    virtual void volumeChanged(float) = 0;
    virtual void muteChanged(bool) = 0;
    virtual void timeChanged() = 0;
    virtual void repaint() = 0;
    virtual void durationChanged() = 0;
    virtual void rateChanged() = 0;
    virtual void sizeChanged() = 0;
    virtual void setOpaque(bool) = 0;
    virtual void sawUnsupportedTracks() = 0;
    virtual float volume() const = 0;
    virtual void playbackStateChanged() = 0;
    virtual WebMediaPlayer::Preload preload() const = 0;
    virtual void sourceOpened() = 0;
    virtual WebKit::WebURL sourceURL() const = 0;
    virtual void keyAdded(const WebString&, const WebString&) = 0;
    virtual void keyError(const WebString&, const WebString&, MediaKeyErrorCode, unsigned short systemCode) = 0;
    virtual void keyMessage(const WebString&, const WebString&, const unsigned char*, unsigned) = 0;
    virtual void keyNeeded(const WebString&, const WebString&, const unsigned char* initData, unsigned initDataLength) = 0;
    // The returned pointer is valid until closeHelperPlugin() is called.
    // Returns 0 if the plugin could not be instantiated.
    virtual WebPlugin* createHelperPlugin(const WebString& pluginType, WebFrame*) = 0;
    virtual void closeHelperPlugin() = 0;
    virtual void disableAcceleratedCompositing() = 0;
protected:
    ~WebMediaPlayerClient() { }
};

} // namespace WebKit

#endif
