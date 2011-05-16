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
class SecurityOrigin;

class MediaStreamController {
    WTF_MAKE_NONCOPYABLE(MediaStreamController);
public:
    MediaStreamController(MediaStreamClient*);
    virtual ~MediaStreamController();

    bool isClientAvailable() const;
    void unregisterFrameController(MediaStreamFrameController*);

    void generateStream(MediaStreamFrameController*, int requestId, GenerateStreamOptionFlags, PassRefPtr<SecurityOrigin>);

    void streamGenerated(int requestId, const String& streamLabel);
    void streamGenerationFailed(int requestId, NavigatorUserMediaError::ErrorCode);

private:
    int registerRequest(int localRequestId, MediaStreamFrameController*);
    void registerStream(const String& streamLabel, MediaStreamFrameController*);

    class Request;
    typedef HashMap<int, Request> RequestMap;
    typedef HashMap<String, MediaStreamFrameController*> StreamMap;

    RequestMap m_requests;
    StreamMap m_streams;

    MediaStreamClient* m_client;
    int m_nextGlobalRequestId;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStreamController_h
