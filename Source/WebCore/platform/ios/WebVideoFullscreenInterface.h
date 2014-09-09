/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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


#ifndef WebVideoFullscreenInterface_h
#define WebVideoFullscreenInterface_h

#if PLATFORM(IOS)

#include <wtf/Vector.h>

namespace WTF {
class String;
}

namespace WebCore {
    
class TimeRanges;

class WebVideoFullscreenInterface {
public:
    enum ExternalPlaybackTargetType { TargetTypeNone, TargetTypeAirPlay, TargetTypeTVOut };
    
    virtual ~WebVideoFullscreenInterface() { };
    virtual void setDuration(double) = 0;
    virtual void setCurrentTime(double currentTime, double anchorTime) = 0;
    virtual void setRate(bool isPlaying, float playbackRate) = 0;
    virtual void setVideoDimensions(bool hasVideo, float width, float height) = 0;
    virtual void setSeekableRanges(const TimeRanges&) = 0;
    virtual void setCanPlayFastReverse(bool) = 0;
    virtual void setAudioMediaSelectionOptions(const Vector<String>& options, uint64_t selectedIndex) = 0;
    virtual void setLegibleMediaSelectionOptions(const Vector<String>& options, uint64_t selectedIndex) = 0;
    virtual void setExternalPlayback(bool enabled, ExternalPlaybackTargetType, String localizedDeviceName) = 0;
};

}

#endif
#endif
