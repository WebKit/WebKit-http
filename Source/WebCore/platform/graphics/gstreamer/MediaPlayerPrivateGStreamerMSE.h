/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009, 2010, 2016 Igalia S.L
 * Copyright (C) 2014 Cable Television Laboratories, Inc.
 * Copyright (C) 2015 Sebastian Dröge <sebastian@centricular.com>
 * Copyright (C) 2015, 2016 Metrological Group B.V.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef MediaPlayerPrivateGStreamerMSE_h
#define MediaPlayerPrivateGStreamerMSE_h

#if ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(MEDIA_SOURCE)  && ENABLE(VIDEO_TRACK)

#include "GRefPtrGStreamer.h"
#include "MediaPlayerPrivateGStreamer.h"
#include "MediaSample.h"
#include "MediaSourceGStreamer.h"
#include "WebKitMediaSourceGStreamer.h"

namespace WebCore {

class MediaSourceClientGStreamerMSE;
class AppendPipeline;

class MediaPlayerPrivateGStreamerMSE : public MediaPlayerPrivateGStreamer {
    WTF_MAKE_NONCOPYABLE(MediaPlayerPrivateGStreamerMSE); WTF_MAKE_FAST_ALLOCATED;

    friend class MediaSourceClientGStreamerMSE;

public:
    explicit MediaPlayerPrivateGStreamerMSE(MediaPlayer*);
    virtual ~MediaPlayerPrivateGStreamerMSE();

    static void registerMediaEngine(MediaEngineRegistrar);

    void load(const String&) override;
    void load(const String&, MediaSourcePrivateClient*) override;

    FloatSize naturalSize() const override;

    void setDownloadBuffering() override { };

    bool isLiveStream() const override { return false; }
    float currentTime() const override;

    void pause() override;
    bool seeking() const override;
    void seek(float) override;
    void configurePlaySink() override;
    bool changePipelineState(GstState) override;

    void durationChanged() override;
    MediaTime durationMediaTime() const override { return m_mediaTimeDuration; }
    float duration() const override;

    float mediaTimeForTimeValue(float timeValue) const;
    void setRate(float) override;
    std::unique_ptr<PlatformTimeRanges> buffered() const override;
    float maxTimeSeekable() const override;

    void sourceChanged() override;

    void setReadyState(MediaPlayer::ReadyState);
    void waitForSeekCompleted();
    void seekCompleted();
    MediaSourcePrivateClient* mediaSourcePrivateClient() { return m_mediaSource.get(); }

    void markEndOfStream(MediaSourcePrivate::EndOfStreamStatus);

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
    void dispatchDecryptionKey(GstBuffer*) override;
#endif
#if USE(PLAYREADY)
    void emitSession() override;
#endif

    void trackDetected(RefPtr<AppendPipeline>, RefPtr<WebCore::TrackPrivateBase> oldTrack, RefPtr<WebCore::TrackPrivateBase> newTrack);
    void notifySeekNeedsDataForTime(const MediaTime&);

private:
    static void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>&);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);

    static bool isAvailable();

    // FIXME: Reduce code duplication.
    void updateStates() override;

    bool doSeek(gint64, float, GstSeekFlags) override;
    bool doSeek();
    void maybeFinishSeek();
    void updatePlaybackRate() override;
    void asyncStateChangeDone() override;

    // FIXME: Implement.
    unsigned long totalVideoFrames() override { return 0; }
    unsigned long droppedVideoFrames() override { return 0; }
    unsigned long corruptedVideoFrames() override { return 0; }
    MediaTime totalFrameDelay() override { return MediaTime::zeroTime(); }
    bool isTimeBuffered(const MediaTime&) const;

    bool isMediaSource() const override { return true; }

    void setMediaSourceClient(PassRefPtr<MediaSourceClientGStreamerMSE>);
    RefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient();

    HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline>> m_appendPipelinesMap;
    bool m_eosMarked;
    mutable bool m_eosPending;
    bool m_gstSeekCompleted;

    RefPtr<MediaSourcePrivateClient> m_mediaSource;
    RefPtr<MediaSourceClientGStreamerMSE> m_mediaSourceClient;
    MediaTime m_mediaTimeDuration;
    bool m_mseSeekCompleted;

    RefPtr<PlaybackPipeline> m_playbackPipeline;
};

class GStreamerMediaSample : public MediaSample {
private:
    MediaTime m_pts, m_dts, m_duration;
    AtomicString m_trackId;
    size_t m_size;
    GRefPtr<GstSample> m_sample;
    FloatSize m_presentationSize;
    MediaSample::SampleFlags m_flags;

    GStreamerMediaSample(GstSample*, const FloatSize& presentationSize, const AtomicString& trackId);

public:
    static PassRefPtr<GStreamerMediaSample> create(GstSample*, const FloatSize& presentationSize, const AtomicString& trackID);
    static PassRefPtr<GStreamerMediaSample> createFakeSample(GstCaps*, MediaTime pts, MediaTime dts, MediaTime duration, const FloatSize& presentationSize, const AtomicString& trackID);

    virtual ~GStreamerMediaSample();

    void applyPtsOffset(MediaTime);
    MediaTime presentationTime() const { return m_pts; }
    MediaTime decodeTime() const { return m_dts; }
    MediaTime duration() const { return m_duration; }
    AtomicString trackID() const { return m_trackId; }
    void setTrackID(const String&) { }
    size_t sizeInBytes() const { return m_size; }
    GstSample* sample() const { return m_sample.get(); }
    FloatSize presentationSize() const { return m_presentationSize; }
    void offsetTimestampsBy(const MediaTime&);
    void setTimestamps(const MediaTime&, const MediaTime&) { }
    SampleFlags flags() const { return m_flags; }
    PlatformSample platformSample() { return PlatformSample(); }
    void dump(PrintStream&) const { }
};

class ContentType;
class SourceBufferPrivateGStreamer;

class MediaSourceClientGStreamerMSE: public RefCounted<MediaSourceClientGStreamerMSE> {
    public:
        static PassRefPtr<MediaSourceClientGStreamerMSE> create(MediaPlayerPrivateGStreamerMSE*);
        virtual ~MediaSourceClientGStreamerMSE();

        // From MediaSourceGStreamer.
        MediaSourcePrivate::AddStatus addSourceBuffer(RefPtr<SourceBufferPrivateGStreamer>, const ContentType&);
        void durationChanged(const MediaTime&);
        void markEndOfStream(MediaSourcePrivate::EndOfStreamStatus);

        // From SourceBufferPrivateGStreamer.
        void abort(PassRefPtr<SourceBufferPrivateGStreamer>);
        bool append(PassRefPtr<SourceBufferPrivateGStreamer>, const unsigned char*, unsigned);
        void removedFromMediaSource(RefPtr<SourceBufferPrivateGStreamer>);
        void flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample>>);
        void enqueueSample(PassRefPtr<MediaSample>);

        // From our WebKitMediaSrc.
        // FIXME: These can be removed easily by directly invoking methods on SourceBufferPrivateGStreamer.
        void didReceiveInitializationSegment(SourceBufferPrivateGStreamer*, const SourceBufferPrivateClient::InitializationSegment&);
        void didReceiveAllPendingSamples(SourceBufferPrivateGStreamer*);

        void clearPlayerPrivate();

        MediaTime duration();
        GRefPtr<WebKitMediaSrc> webKitMediaSrc();

    private:
        MediaSourceClientGStreamerMSE(MediaPlayerPrivateGStreamerMSE*);

        // Would better be a RefPtr, but the playerprivate is a unique_ptr, so
        // we can't mess with references here. In exchange, the playerprivate
        // must notify us when it's being destroyed, so we can clear our pointer.
        MediaPlayerPrivateGStreamerMSE* m_playerPrivate;
        MediaTime m_duration;
};

} // namespace WebCore

#endif // USE(GSTREAMER)
#endif
