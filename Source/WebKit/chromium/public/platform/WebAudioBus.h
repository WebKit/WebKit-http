/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef WebAudioBus_h
#define WebAudioBus_h

#include "WebCommon.h"

namespace WebCore { class AudioBus; }

#if WEBKIT_IMPLEMENTATION
namespace WTF { template <typename T> class PassOwnPtr; }
#endif

namespace WebKit {

class WebAudioBusPrivate;

// A container for multi-channel linear PCM audio data.
//
// WARNING: It is not safe to pass a WebAudioBus across threads!!!
//
class WebAudioBus {
public:
    WebAudioBus() : m_private(0) { }
    ~WebAudioBus() { reset(); }
    
    // initialize() allocates memory of the given length for the given number of channels.
    WEBKIT_EXPORT void initialize(unsigned numberOfChannels, size_t length, double sampleRate);

    // reset() releases the memory allocated from initialize().
    WEBKIT_EXPORT void reset();
    
    WEBKIT_EXPORT unsigned numberOfChannels() const;
    WEBKIT_EXPORT size_t length() const;
    WEBKIT_EXPORT double sampleRate() const;
    
    WEBKIT_EXPORT float* channelData(unsigned channelIndex);

#if WEBKIT_IMPLEMENTATION
    WTF::PassOwnPtr<WebCore::AudioBus> release();    
#endif

private:
    // Disallow copy and assign.
    WebAudioBus(const WebAudioBus&);
    void operator=(const WebAudioBus&);

    WebCore::AudioBus* m_private;
};

} // namespace WebKit

#endif // WebAudioBus_h
