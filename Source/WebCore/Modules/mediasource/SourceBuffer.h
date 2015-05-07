/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013-2014 Apple Inc. All rights reserved.
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

#ifndef SourceBuffer_h
#define SourceBuffer_h

#if ENABLE(MEDIA_SOURCE)

#include "ActiveDOMObject.h"
#include "AudioTrack.h"
#include "Document.h"
#include "EventTarget.h"
#include "ExceptionCode.h"
#include "GenericEventQueue.h"
#include "ScriptWrappable.h"
#include "SourceBufferPrivateClient.h"
#include "TextTrack.h"
#include "Timer.h"
#include "VideoTrack.h"
#include <runtime/ArrayBufferView.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AudioTrackList;
class MediaSource;
class PlatformTimeRanges;
class SourceBufferPrivate;
class TimeRanges;
#if ENABLE(VIDEO_TRACK)
class TextTrackList;
class VideoTrackList;
#endif

class SourceBuffer : public RefCounted<SourceBuffer>, public ActiveDOMObject, public EventTargetWithInlineData, public ScriptWrappable, public SourceBufferPrivateClient
#if ENABLE(VIDEO_TRACK)
, public AudioTrackClient, public VideoTrackClient, public TextTrackClient
#endif
{
public:
    static Ref<SourceBuffer> create(Ref<SourceBufferPrivate>&&, MediaSource*);

    virtual ~SourceBuffer();

    // SourceBuffer.idl methods
    bool updating() const { return m_updating; }
    PassRefPtr<TimeRanges> buffered(ExceptionCode&) const;
    const RefPtr<TimeRanges>& buffered() const;
    double timestampOffset() const;
    void setTimestampOffset(double, ExceptionCode&);
    void appendBuffer(PassRefPtr<ArrayBuffer> data, ExceptionCode&);
    void appendBuffer(PassRefPtr<ArrayBufferView> data, ExceptionCode&);
    void abort(ExceptionCode&);
    void remove(double start, double end, ExceptionCode&, bool sync = false);
    void remove(const MediaTime&, const MediaTime&, ExceptionCode&, bool sync = false);

    void abortIfUpdating();
    void removedFromMediaSource();
#if ENABLE(VIDEO_TRACK)
    const MediaTime& highestPresentationEndTimestamp() const { return m_highestPresentationEndTimestamp; }
#endif
    void seekToTime(const MediaTime&);

#if ENABLE(VIDEO_TRACK)
    VideoTrackList* videoTracks();
    AudioTrackList* audioTracks();
    TextTrackList* textTracks();
#endif

    bool hasCurrentTime() const;
    bool hasFutureTime() const;
    bool canPlayThrough();

    bool hasVideo() const;
    bool hasAudio() const;

    bool active() const { return m_active; }

    // EventTarget interface
    virtual ScriptExecutionContext* scriptExecutionContext() const override { return ActiveDOMObject::scriptExecutionContext(); }
    virtual EventTargetInterface eventTargetInterface() const override { return SourceBufferEventTargetInterfaceType; }

    using RefCounted<SourceBuffer>::ref;
    using RefCounted<SourceBuffer>::deref;

    struct TrackBuffer;

    Document& document() const;

    void invalidateBuffered();

    // ActiveDOMObject API.
    bool hasPendingActivity() const override;

protected:
    // EventTarget interface
    virtual void refEventTarget() override { ref(); }
    virtual void derefEventTarget() override { deref(); }

private:
    SourceBuffer(Ref<SourceBufferPrivate>&&, MediaSource*);

    // ActiveDOMObject API.
    void stop() override;
    const char* activeDOMObjectName() const override;
    bool canSuspendForPageCache() const override;

    // SourceBufferPrivateClient
    virtual void sourceBufferPrivateDidEndStream(SourceBufferPrivate*, const WTF::AtomicString&) override;
    virtual void sourceBufferPrivateDidReceiveInitializationSegment(SourceBufferPrivate*, const InitializationSegment&) override;
    virtual void sourceBufferPrivateDidReceiveSample(SourceBufferPrivate*, PassRefPtr<MediaSample>) override;
    virtual bool sourceBufferPrivateHasAudio(const SourceBufferPrivate*) const override;
    virtual bool sourceBufferPrivateHasVideo(const SourceBufferPrivate*) const override;
    virtual void sourceBufferPrivateDidBecomeReadyForMoreSamples(SourceBufferPrivate*, AtomicString trackID) override;
    virtual MediaTime sourceBufferPrivateFastSeekTimeForMediaTime(SourceBufferPrivate*, const MediaTime&, const MediaTime& negativeThreshold, const MediaTime& positiveThreshold) override;
    virtual void sourceBufferPrivateAppendComplete(SourceBufferPrivate*, AppendResult) override;
    virtual void sourceBufferPrivateDidReceiveRenderingError(SourceBufferPrivate*, int errorCode) override;

#if ENABLE(VIDEO_TRACK)
    // AudioTrackClient
    virtual void audioTrackEnabledChanged(AudioTrack*);

    // VideoTrackClient
    virtual void videoTrackSelectedChanged(VideoTrack*);

    // TextTrackClient
    virtual void textTrackKindChanged(TextTrack*);
    virtual void textTrackModeChanged(TextTrack*);
    virtual void textTrackAddCues(TextTrack*, const TextTrackCueList*);
    virtual void textTrackRemoveCues(TextTrack*, const TextTrackCueList*);
    virtual void textTrackAddCue(TextTrack*, PassRefPtr<TextTrackCue>);
    virtual void textTrackRemoveCue(TextTrack*, PassRefPtr<TextTrackCue>);
#endif

    static const WTF::AtomicString& decodeError();
    static const WTF::AtomicString& networkError();

    bool isRemoved() const;
    void scheduleEvent(const AtomicString& eventName);

    void appendBufferInternal(unsigned char*, unsigned, ExceptionCode&);
    void appendBufferTimerFired();
    void resetParserState();

    void setActive(bool);

#if ENABLE(VIDEO_TRACK)
    bool validateInitializationSegment(const InitializationSegment&);
#endif

    void reenqueueMediaForTime(TrackBuffer&, AtomicString trackID, const MediaTime&);
    void provideMediaData(TrackBuffer&, AtomicString trackID);
    void didDropSample();
    void evictCodedFrames(size_t newDataSize);
    size_t maximumBufferSize() const;

    void monitorBufferingRate();

    void removeTimerFired();
    void removeCodedFrames(const MediaTime& start, const MediaTime& end);
    void recalculateBuffered() const;

    size_t extraMemoryCost() const;
    void reportExtraMemoryAllocated();

    std::unique_ptr<PlatformTimeRanges> bufferedAccountingForEndOfStream() const;

    // Internals
    friend class Internals;
    WEBCORE_EXPORT Vector<String> bufferedSamplesForTrackID(const AtomicString&);

    Ref<SourceBufferPrivate> m_private;
    MediaSource* m_source;
    GenericEventQueue m_asyncEventQueue;

    Vector<unsigned char> m_pendingAppendData;
    Timer m_appendBufferTimer;

#if ENABLE(VIDEO_TRACK)
    RefPtr<VideoTrackList> m_videoTracks;
    RefPtr<AudioTrackList> m_audioTracks;
    RefPtr<TextTrackList> m_textTracks;
#endif

    Vector<AtomicString> m_videoCodecs;
    Vector<AtomicString> m_audioCodecs;
    Vector<AtomicString> m_textCodecs;

#if ENABLE(VIDEO_TRACK)
    MediaTime m_timestampOffset;
    MediaTime m_highestPresentationEndTimestamp;
#else
    double m_timestampOffset;
#endif

    HashMap<AtomicString, TrackBuffer> m_trackBufferMap;
    mutable RefPtr<TimeRanges> m_buffered;

    enum AppendStateType { WaitingForSegment, ParsingInitSegment, ParsingMediaSegment };
    AppendStateType m_appendState;

    double m_timeOfBufferingMonitor;
    double m_bufferedSinceLastMonitor;
    double m_averageBufferRate;

    size_t m_reportedExtraMemoryCost;

    MediaTime m_pendingRemoveStart;
    MediaTime m_pendingRemoveEnd;
    Timer m_removeTimer;

    bool m_updating;
    bool m_receivedFirstInitializationSegment;
    bool m_active;
    bool m_bufferFull;
    mutable bool m_shouldRecalculateBuffered;
};

} // namespace WebCore

#endif

#endif
