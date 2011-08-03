/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef MediaStreamController_h
#define MediaStreamController_h

#if ENABLE(MEDIA_STREAM)

#include "MediaStreamClient.h"
#include "NavigatorUserMediaError.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class MediaStreamClient;
class MediaStreamFrameController;
class MediaStreamTrackList;
class SecurityOrigin;

class MediaStreamController {
    WTF_MAKE_NONCOPYABLE(MediaStreamController);
public:
    MediaStreamController(MediaStreamClient*);
    virtual ~MediaStreamController();

    bool isClientAvailable() const;
    void unregisterFrameController(MediaStreamFrameController*);

    void generateStream(MediaStreamFrameController*, int requestId, GenerateStreamOptionFlags, PassRefPtr<SecurityOrigin>);
    void stopGeneratedStream(const String& streamLabel);

    // Enable/disable an track.
    void setMediaStreamTrackEnabled(const String& trackId, bool enabled);

    void streamGenerated(int requestId, const String& streamLabel, PassRefPtr<MediaStreamTrackList> tracks);
    void streamGenerationFailed(int requestId, NavigatorUserMediaError::ErrorCode);
    void streamFailed(const String& streamLabel);

    /* JS -> UA */
    void newPeerConnection(MediaStreamFrameController*, int framePeerConnectionId, const String& configuration);
    void startNegotiation(MediaStreamFrameController*, int framePeerConnectionId);
    void processSignalingMessage(MediaStreamFrameController*, int framePeerConnectionId, const String& message);
    void message(MediaStreamFrameController*, int framePeerConnectionId, const String& message);
    void addStream(MediaStreamFrameController*, int framePeerConnectionId, const String& streamLabel);
    void removeStream(MediaStreamFrameController*, int framePeerConnectionId, const String& streamLabel);
    void closePeerConnection(MediaStreamFrameController*, int framePeerConnectionId);

    /* UA -> JS */
    void onSignalingMessage(int controllerPeerConnectionId, const String& message);
    void onMessage(int controllerPeerConnectionId, const String& message);
    void onAddStream(int controllerPeerConnectionId, const String& streamLabel, PassRefPtr<MediaStreamTrackList> tracks);
    void onRemoveStream(int controllerPeerConnectionId, const String& streamLabel);
    void onNegotiationStarted(int controllerPeerConnectionId);
    void onNegotiationDone(int controllerPeerConnectionId);

private:
    int registerRequest(int localRequestId, MediaStreamFrameController*);
    void registerStream(const String& streamLabel, MediaStreamFrameController*);

    bool frameToPagePeerConnectionId(MediaStreamFrameController*, int framePeerConnectionId, int& pagePeerConnectionId);

    class Request;
    typedef HashMap<int, Request> RequestMap;
    typedef HashMap<String, MediaStreamFrameController*> StreamMap;

    RequestMap m_requests;
    StreamMap m_streams;
    RequestMap m_peerConnections;

    MediaStreamClient* m_client;
    int m_nextGlobalRequestId;
    int m_nextGlobalPeerConnectionId;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStreamController_h
