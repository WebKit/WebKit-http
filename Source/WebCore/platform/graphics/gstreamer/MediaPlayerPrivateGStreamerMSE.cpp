/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2016 Igalia S.L
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

#include "config.h"
#include "MediaPlayerPrivateGStreamerMSE.h"

#if ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(MEDIA_SOURCE) && ENABLE(VIDEO_TRACK)

#include "AudioTrackPrivateGStreamer.h"
#include "GStreamerUtilities.h"
#include "InbandTextTrackPrivateGStreamer.h"
#include "MIMETypeRegistry.h"
#include "MediaDescription.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "SourceBufferPrivateGStreamer.h"
#include "TimeRanges.h"
#include "URL.h"
#include "VideoTrackPrivateGStreamer.h"
#include <wtf/Condition.h>

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>

#if USE(PLAYREADY)
#include "PlayreadySession.h"
#endif

static const char* dumpReadyState(WebCore::MediaPlayer::ReadyState readyState)
{
    switch (readyState) {
    case WebCore::MediaPlayer::HaveNothing: return "HaveNothing";
    case WebCore::MediaPlayer::HaveMetadata: return "HaveMetadata";
    case WebCore::MediaPlayer::HaveCurrentData: return "HaveCurrentData";
    case WebCore::MediaPlayer::HaveFutureData: return "HaveFutureData";
    case WebCore::MediaPlayer::HaveEnoughData: return "HaveEnoughData";
    default: return "(unknown)";
    }
}

// Max interval in seconds to stay in the READY state on manual
// state change requests.
static const unsigned gReadyStateTimerInterval = 60;

GST_DEBUG_CATEGORY(webkit_mse_debug);
#define GST_CAT_DEFAULT webkit_mse_debug

using namespace std;

namespace WebCore {

struct PadProbeInformation {
    AppendPipeline* appendPipeline;
    const char* description;
    gulong probeId;
};

class AppendPipeline : public ThreadSafeRefCounted<AppendPipeline> {
public:
    enum AppendState { Invalid, NotStarted, Ongoing, KeyNegotiation, DataStarve, Sampling, LastSample, Aborting };

    AppendPipeline(PassRefPtr<MediaSourceClientGStreamerMSE>, PassRefPtr<SourceBufferPrivateGStreamer>, MediaPlayerPrivateGStreamerMSE*);
    virtual ~AppendPipeline();

    void handleNeedContextSyncMessage(GstMessage*);
    void handleApplicationMessage(GstMessage*);

    gint id();
    AppendState appendState() { return m_appendState; }
    void setAppendState(AppendState);

    GstFlowReturn handleNewAppsinkSample(GstElement* appsink);
    GstFlowReturn pushNewBuffer(GstBuffer*);
    void dispatchDecryptionKey(GstBuffer* buffer);

    // Takes ownership of caps.
    void parseDemuxerSrcPadCaps(GstCaps*);
    void appsinkCapsChanged();
    void appsinkNewSample(GstSample*);
    void appsinkEOS();
    void didReceiveInitializationSegment();
    AtomicString trackId();
    void abort();

    void clearPlayerPrivate();
    RefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient() { return m_mediaSourceClient; }
    RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate() { return m_sourceBufferPrivate; }
    GstBus* bus() { return m_bus.get(); }
    GstElement* pipeline() { return m_pipeline.get(); }
    GstElement* appsrc() { return m_appsrc.get(); }
    GstElement* appsink() { return m_appsink.get(); }
    GstCaps* demuxerSrcPadCaps() { return m_demuxerSrcPadCaps.get(); }
    GstCaps* appsinkCaps() { return m_appsinkCaps.get(); }
    RefPtr<WebCore::TrackPrivateBase> track() { return m_track; }
    WebCore::MediaSourceStreamTypeGStreamer streamType() { return m_streamType; }

    void disconnectDemuxerSrcPadFromAppsinkFromAnyThread();
    void disconnectDemuxerSrcPadFromAppsink();
    void connectDemuxerSrcPadToAppsinkFromAnyThread(GstPad*);
    void connectDemuxerSrcPadToAppsink(GstPad* demuxerSrcPad);

    void reportAppsrcAtLeastABufferLeft();
    void reportAppsrcNeedDataReceived();

private:
    void resetPipeline();
    void checkEndOfAppend();
    void handleAppsrcAtLeastABufferLeft();
    void handleAppsrcNeedDataReceived();
    void removeAppsrcDataLeavingProbe();
    void setAppsrcDataLeavingProbe();
    void dispatchPendingDecryptionKey();

private:
    RefPtr<MediaSourceClientGStreamerMSE> m_mediaSourceClient;
    RefPtr<SourceBufferPrivateGStreamer> m_sourceBufferPrivate;
    MediaPlayerPrivateGStreamerMSE* m_playerPrivate;

    // (m_mediaType, m_id) is unique.
    gint m_id;

    MediaTime m_initialDuration;

    GstFlowReturn m_flowReturn;

    GRefPtr<GstElement> m_pipeline;
    GRefPtr<GstBus> m_bus;
    GRefPtr<GstElement> m_appsrc;
    GRefPtr<GstElement> m_demux;
    GRefPtr<GstElement> m_decryptor;
    GRefPtr<GstElement> m_appsink;
    // The demuxer has one src Stream only, so only one appsink is needed and linked to it.

    Lock m_newSampleLock;
    Condition m_newSampleCondition;
    Lock m_padAddRemoveLock;
    Condition m_padAddRemoveCondition;

    GRefPtr<GstCaps> m_appsinkCaps;
    GRefPtr<GstCaps> m_demuxerSrcPadCaps;
    FloatSize m_presentationSize;

    bool m_appsrcAtLeastABufferLeft;
    bool m_appsrcNeedDataReceived;

    gulong m_appsrcDataLeavingProbeId;
#ifdef DEBUG_APPEND_PIPELINE_PADS
    struct PadProbeInformation m_demuxerDataEnteringPadProbeInformation;
    struct PadProbeInformation m_appsinkDataEnteringPadProbeInformation;
#endif

    // Keeps track of the states of append processing, to avoid
    // performing actions inappropriate for the current state (eg:
    // processing more samples when the last one has been detected,
    // etc.). See setAppendState() for valid transitions.
    AppendState m_appendState;

    // Aborts can only be completed when the normal sample detection
    // has finished. Meanwhile, the willing to abort is expressed in
    // this field.
    bool m_abortPending;

    WebCore::MediaSourceStreamTypeGStreamer m_streamType;
    RefPtr<WebCore::TrackPrivateBase> m_oldTrack;
    RefPtr<WebCore::TrackPrivateBase> m_track;

    GRefPtr<GstBuffer> m_pendingBuffer;
    GRefPtr<GstBuffer> m_pendingKey;

    static gint totalAudio;
    static gint totalVideo;
    static gint totalText;
};

void MediaPlayerPrivateGStreamerMSE::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable()) {
        registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivateGStreamerMSE>(player); },
            getSupportedTypes, supportsType, nullptr, nullptr, nullptr, supportsKeySystem);
    }
}

bool initializeGStreamerAndRegisterWebKitMESElement()
{
    if (!initializeGStreamer())
        return false;

    registerWebKitGStreamerElements();

    GST_DEBUG_CATEGORY_INIT(webkit_mse_debug, "webkitmse", 0, "WebKit MSE media player");

    GRefPtr<GstElementFactory> WebKitMediaSrcFactory = gst_element_factory_find("webkitmediasrc");
    if (!WebKitMediaSrcFactory)
        gst_element_register(nullptr, "webkitmediasrc", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_SRC);
    return true;
}

bool MediaPlayerPrivateGStreamerMSE::isAvailable()
{
    if (!initializeGStreamerAndRegisterWebKitMESElement())
        return false;

    GRefPtr<GstElementFactory> factory = gst_element_factory_find("playbin");
    return factory;
}

MediaPlayerPrivateGStreamerMSE::MediaPlayerPrivateGStreamerMSE(MediaPlayer* player)
    : MediaPlayerPrivateGStreamer(player)
    , m_eosMarked(false)
    , m_eosPending(false)
    , m_gstSeekCompleted(true)
    , m_mseSeekCompleted(true)
    , m_loadingProgressed(false)
{
    GST_DEBUG("%p", this);
}

MediaPlayerPrivateGStreamerMSE::~MediaPlayerPrivateGStreamerMSE()
{
    GST_DEBUG("destroying the player");

    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline>>::iterator iterator = m_appendPipelinesMap.begin(); iterator != m_appendPipelinesMap.end(); ++iterator)
        iterator->value->clearPlayerPrivate();

    clearSamples();

    if (m_source) {
        webkit_media_src_set_mediaplayerprivate(WEBKIT_MEDIA_SRC(m_source.get()), nullptr);
        g_signal_handlers_disconnect_by_data(m_source.get(), this);
    }

    if (m_playbackPipeline)
        m_playbackPipeline->setWebKitMediaSrc(nullptr);

}

void MediaPlayerPrivateGStreamerMSE::load(const String& urlString)
{
    if (!initializeGStreamerAndRegisterWebKitMESElement())
        return;

    if (!urlString.startsWith("mediasource")) {
        GST_DEBUG("Unsupported url: %s", urlString.utf8().data());
        return;
    }

    if (!m_playbackPipeline)
        m_playbackPipeline = PlaybackPipeline::create();

    MediaPlayerPrivateGStreamer::load(urlString);
}

void MediaPlayerPrivateGStreamerMSE::load(const String& url, MediaSourcePrivateClient* mediaSource)
{
    String mediasourceUri = String::format("mediasource%s", url.utf8().data());
    m_mediaSource = mediaSource;
    load(mediasourceUri);
}

FloatSize MediaPlayerPrivateGStreamerMSE::naturalSize() const
{
    if (!hasVideo())
        return FloatSize();

    if (!m_videoSize.isEmpty())
        return m_videoSize;

    return MediaPlayerPrivateGStreamerBase::naturalSize();
}

void MediaPlayerPrivateGStreamerMSE::pause()
{
    m_paused = true;
    MediaPlayerPrivateGStreamer::pause();
}

MediaTime MediaPlayerPrivateGStreamerMSE::durationMediaTime() const
{
    if (!m_pipeline || m_errorOccured)
        return { };

    return m_mediaTimeDuration;
}

void MediaPlayerPrivateGStreamerMSE::seek(float time)
{
    if (!m_pipeline || m_errorOccured)
        return;

    GST_INFO("[Seek] seek attempt to %f secs", time);

    // Avoid useless seeking.
    float current = currentMediaTime().toDouble();
    if (time == current) {
        if (!m_seeking)
            timeChanged();
        return;
    }

    if (isLiveStream())
        return;

    if (m_seeking && m_seekIsPending) {
        m_seekTime = time;
        return;
    }

    GST_DEBUG("Seeking from %f to %f seconds", current, time);

    float prevSeekTime = m_seekTime;
    m_seekTime = time;

    if (!doSeek()) {
        m_seekTime = prevSeekTime;
        GST_DEBUG("Seeking to %f failed", time);
        return;
    }

    m_isEndReached = false;
    GST_DEBUG("m_seeking=%s, m_seekTime=%f", m_seeking?"true":"false", m_seekTime);
}

void MediaPlayerPrivateGStreamerMSE::configurePlaySink()
{
    MediaPlayerPrivateGStreamer::configurePlaySink();

    GRefPtr<GstElement> playsink = adoptGRef(gst_bin_get_by_name(GST_BIN(m_pipeline.get()), "playsink"));
    if (playsink) {
        // The default value (0) means "send events to all the sinks", instead
        // of "only to the first that returns true". This is needed for MSE seek.
        g_object_set(G_OBJECT(playsink.get()), "send-event-mode", 0, NULL);
    }
}

bool MediaPlayerPrivateGStreamerMSE::changePipelineState(GstState newState)
{
    if (seeking()) {
        GST_DEBUG("Rejected state change to %s while seeking",
            gst_element_state_get_name(newState));
        return true;
    }

    return MediaPlayerPrivateGStreamer::changePipelineState(newState);
}

void MediaPlayerPrivateGStreamerMSE::notifySeekNeedsDataForTime(const MediaTime& seekTime)
{
    // Reenqueue samples needed to resume playback in the new position.
    m_mediaSource->seekToTime(seekTime);

    GST_DEBUG("MSE seek to %f finished", seekTime.toDouble());

    if (!m_gstSeekCompleted) {
        m_gstSeekCompleted = true;
        maybeFinishSeek();
    }
}

bool MediaPlayerPrivateGStreamerMSE::doSeek(gint64, float, GstSeekFlags)
{
    // Use doSeek() instead. If anybody is calling this version of doSeek(), something is wrong.
    ASSERT_NOT_REACHED();
    return false;
}

bool MediaPlayerPrivateGStreamerMSE::doSeek()
{
    GstClockTime position = toGstClockTime(m_seekTime);
    MediaTime seekTime = MediaTime::createWithDouble(m_seekTime);
    double rate = m_player->rate();
    GstSeekFlags seekType = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | hardwareDependantSeekFlags());

    // Always move to seeking state to report correct 'currentTime' while pending for actual seek to complete.
    m_seeking = true;

    // Check if playback pipeline is ready for seek.
    GstState state, newState;
    GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, &newState, 0);
    if (getStateResult == GST_STATE_CHANGE_FAILURE || getStateResult == GST_STATE_CHANGE_NO_PREROLL) {
        GST_DEBUG("[Seek] cannot seek, current state change is %s", gst_element_state_change_return_get_name(getStateResult));
        webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), true);
        m_seeking = false;
        return false;
    }
    if ((getStateResult == GST_STATE_CHANGE_ASYNC
        && !(state == GST_STATE_PLAYING && newState == GST_STATE_PAUSED))
        || state < GST_STATE_PAUSED
        || m_isEndReached
        || !m_gstSeekCompleted) {
        CString reason = "Unknown reason";
        if (getStateResult == GST_STATE_CHANGE_ASYNC) {
            reason = String::format("In async change %s --> %s",
                gst_element_state_get_name(state),
                gst_element_state_get_name(newState)).utf8();
        } else if (state < GST_STATE_PAUSED)
            reason = "State less than PAUSED";
        else if (m_isEndReached)
            reason = "End reached";
        else if (!m_gstSeekCompleted)
            reason = "Previous seek is not finished yet";

        GST_DEBUG("[Seek] Delaying the seek: %s", reason.data());

        m_seekIsPending = true;

        if (m_isEndReached) {
            GST_DEBUG("[Seek] reset pipeline");
            m_resetPipeline = true;
            m_seeking = false;
            if (!changePipelineState(GST_STATE_PAUSED))
                loadingFailed(MediaPlayer::Empty);
            else
                m_seeking = true;
        }

        return m_seeking;
    }

    // Stop accepting new samples until actual seek is finished.
    webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), false);

    // Correct seek time if it helps to fix a small gap.
    if (!isTimeBuffered(seekTime)) {
        // Look if a near future time (<0.1 sec.) is buffered and change the seek target time.
        if (m_mediaSource) {
            const MediaTime miniGap = MediaTime::createWithDouble(0.1);
            MediaTime nearest = m_mediaSource->buffered()->nearest(seekTime);
            if (nearest.isValid() && nearest > seekTime && (nearest - seekTime) <= miniGap && isTimeBuffered(nearest + miniGap)) {
                GST_DEBUG("[Seek] Changed the seek target time from %f to %f, a near point in the future", seekTime.toFloat(), nearest.toFloat());
                seekTime = nearest;
            }
        }
    }

    // Check if MSE has samples for requested time and defer actual seek if needed.
    if (!isTimeBuffered(seekTime)) {
        GST_DEBUG("[Seek] Delaying the seek: MSE is not ready");
        GstStateChangeReturn setStateResult = gst_element_set_state(m_pipeline.get(), GST_STATE_PAUSED);
        if (setStateResult == GST_STATE_CHANGE_FAILURE) {
            GST_DEBUG("[Seek] Cannot seek, failed to pause playback pipeline.");
            webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), true);
            m_seeking = false;
            return false;
        }
        m_readyState = MediaPlayer::HaveMetadata;
        notifySeekNeedsDataForTime(seekTime);
        ASSERT(!m_mseSeekCompleted);
        return true;
    }

    // Complete previous MSE seek if needed.
    if (!m_mseSeekCompleted) {
        m_mediaSource->monitorSourceBuffers();
        ASSERT(m_mseSeekCompleted);
        // Note: seekCompleted will recursively call us.
        return m_seeking;
    }

    GST_DEBUG("We can seek now");

    gint64 startTime, endTime;

    if (rate > 0) {
        startTime = position;
        endTime = GST_CLOCK_TIME_NONE;
    } else {
        startTime = 0;
        endTime = position;
    }

    if (!rate)
        rate = 1;

    GST_DEBUG("Actual seek to %" GST_TIME_FORMAT ", end time:  %" GST_TIME_FORMAT ", rate: %f", GST_TIME_ARGS(startTime), GST_TIME_ARGS(endTime), rate);

    // This will call notifySeekNeedsData() after some time to tell that the pipeline is ready for sample enqueuing.
    webkit_media_src_prepare_seek(WEBKIT_MEDIA_SRC(m_source.get()), seekTime);

    m_gstSeekCompleted = false;
    if (!gst_element_seek(m_pipeline.get(), rate, GST_FORMAT_TIME, seekType, GST_SEEK_TYPE_SET, startTime, GST_SEEK_TYPE_SET, endTime)) {
        webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), true);
        m_seeking = false;
        m_gstSeekCompleted = true;
        GST_DEBUG("Returning false");
        return false;
    }

    // The samples will be enqueued in notifySeekNeedsData().
    GST_DEBUG("Returning true");
    return true;
}

void MediaPlayerPrivateGStreamerMSE::maybeFinishSeek()
{
    if (!m_seeking || !m_mseSeekCompleted || !m_gstSeekCompleted)
        return;

    GstState state, newState;
    GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, &newState, 0);

    if (getStateResult == GST_STATE_CHANGE_ASYNC
        && !(state == GST_STATE_PLAYING && newState == GST_STATE_PAUSED)) {
        GST_DEBUG("[Seek] Delaying seek finish");
        return;
    }

    if (m_seekIsPending) {
        GST_DEBUG("[Seek] Committing pending seek to %f", m_seekTime);
        m_seekIsPending = false;
        if (!doSeek()) {
            GST_DEBUG("[Seek] Seeking to %f failed", m_seekTime);
            m_cachedPosition = -1;
        }
        return;
    }

    GST_DEBUG("[Seek] Seeked to %f", m_seekTime);

    webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), true);
    m_seeking = false;
    m_cachedPosition = -1;
    // The pipeline can still have a pending state. In this case a position query will fail.
    // Right now we can use m_seekTime as a fallback.
    m_canFallBackToLastFinishedSeekPosition = true;
    timeChanged();
}

void MediaPlayerPrivateGStreamerMSE::updatePlaybackRate()
{
    notImplemented();
}

bool MediaPlayerPrivateGStreamerMSE::seeking() const
{
    return m_seeking;
}

// FIXME: MediaPlayerPrivateGStreamer manages the ReadyState on its own. We shouldn't change it manually.
void MediaPlayerPrivateGStreamerMSE::setReadyState(MediaPlayer::ReadyState readyState)
{
    if (readyState == m_readyState)
        return;

    if (seeking()) {
        GST_DEBUG("Skip ready state change(%s -> %s) due to seek\n", dumpReadyState(m_readyState), dumpReadyState(readyState));
        return;
    }

    GST_DEBUG("Ready State Changed manually from %u to %u", m_readyState, readyState);
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    m_readyState = readyState;
    GST_DEBUG("m_readyState: %s -> %s", dumpReadyState(oldReadyState), dumpReadyState(m_readyState));

    if (oldReadyState < MediaPlayer::HaveCurrentData && m_readyState >= MediaPlayer::HaveCurrentData) {
        GST_DEBUG("[Seek] Reporting load state changed to trigger seek continuation");
        loadStateChanged();
    }
    m_player->readyStateChanged();

    GstState pipelineState;
    GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &pipelineState, nullptr, 250 * GST_NSECOND);
    bool isPlaying = (getStateResult == GST_STATE_CHANGE_SUCCESS && pipelineState == GST_STATE_PLAYING);

    if (m_readyState == MediaPlayer::HaveMetadata && oldReadyState > MediaPlayer::HaveMetadata && isPlaying) {
        GST_DEBUG("Changing pipeline to PAUSED...");
        bool ok = changePipelineState(GST_STATE_PAUSED);
        GST_DEBUG("Changing pipeline to PAUSED: %s", ok?"Ok":"Error");
    }

    m_buffering = (m_readyState == MediaPlayer::HaveMetadata);
}

void MediaPlayerPrivateGStreamerMSE::waitForSeekCompleted()
{
    if (!m_seeking)
        return;

    GST_DEBUG("Waiting for MSE seek completed");
    m_mseSeekCompleted = false;
}

void MediaPlayerPrivateGStreamerMSE::seekCompleted()
{
    if (m_mseSeekCompleted)
        return;

    GST_DEBUG("MSE seek completed");
    m_mseSeekCompleted = true;

    doSeek();

    if (!seeking() && m_readyState >= MediaPlayer::HaveFutureData)
        changePipelineState(GST_STATE_PLAYING);

    if (!seeking())
        m_player->timeChanged();
}

void MediaPlayerPrivateGStreamerMSE::setRate(float)
{
    notImplemented();
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateGStreamerMSE::buffered() const
{
    return m_mediaSource->buffered();
}

void MediaPlayerPrivateGStreamerMSE::sourceChanged()
{
    m_source.clear();
    g_object_get(m_pipeline.get(), "source", &m_source.outPtr(), nullptr);

    ASSERT(WEBKIT_IS_MEDIA_SRC(m_source.get()));

    m_playbackPipeline->setWebKitMediaSrc(WEBKIT_MEDIA_SRC(m_source.get()));

    MediaSourceGStreamer::open(m_mediaSource.get(), this);
    g_signal_connect_swapped(m_source.get(), "video-changed", G_CALLBACK(videoChangedCallback), this);
    g_signal_connect_swapped(m_source.get(), "audio-changed", G_CALLBACK(audioChangedCallback), this);
    g_signal_connect_swapped(m_source.get(), "text-changed", G_CALLBACK(textChangedCallback), this);
    webkit_media_src_set_mediaplayerprivate(WEBKIT_MEDIA_SRC(m_source.get()), this);
}

void MediaPlayerPrivateGStreamerMSE::updateStates()
{
    if (!m_pipeline || m_errorOccured)
        return;

    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    GstState state, pending;

    GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, &pending, 250 * GST_NSECOND);

    bool shouldUpdatePlaybackState = false;
    switch (getStateResult) {
    case GST_STATE_CHANGE_SUCCESS: {
        GST_DEBUG("State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));

        // Do nothing if on EOS and state changed to READY to avoid recreating the player
        // on HTMLMediaElement and properly generate the video 'ended' event.
        if (m_isEndReached && state == GST_STATE_READY)
            break;

        if (state <= GST_STATE_READY) {
            m_resetPipeline = true;
            m_mediaTimeDuration = MediaTime::zeroTime();
        } else
            m_resetPipeline = false;

        // Update ready and network states.
        switch (state) {
        case GST_STATE_NULL:
            m_readyState = MediaPlayer::HaveNothing;
            GST_DEBUG("m_readyState=%s", dumpReadyState(m_readyState));
            m_networkState = MediaPlayer::Empty;
            break;
        case GST_STATE_READY:
            m_readyState = MediaPlayer::HaveMetadata;
            GST_DEBUG("m_readyState=%s", dumpReadyState(m_readyState));
            m_networkState = MediaPlayer::Empty;
            break;
        case GST_STATE_PAUSED:
        case GST_STATE_PLAYING:
            if (seeking()) {
                m_readyState = MediaPlayer::HaveMetadata;
                // TODO: NetworkState?
            } else if (m_mediaSource) {
                m_mediaSource->monitorSourceBuffers();
                m_networkState = MediaPlayer::Loading;
            }
            GST_DEBUG("m_readyState=%s", dumpReadyState(m_readyState));
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }

        // Sync states where needed.
        if (state == GST_STATE_PAUSED) {
            if (!m_volumeAndMuteInitialized) {
                notifyPlayerOfVolumeChange();
                notifyPlayerOfMute();
                m_volumeAndMuteInitialized = true;
            }

            if (!seeking() && !m_buffering && !m_paused && m_playbackRate) {
                GST_DEBUG("[Buffering] Restarting playback.");
                changePipelineState(GST_STATE_PLAYING);
            }
        } else if (state == GST_STATE_PLAYING) {
            m_paused = false;

            if ((m_buffering && !isLiveStream()) || !m_playbackRate) {
                GST_DEBUG("[Buffering] Pausing stream for buffering.");
                changePipelineState(GST_STATE_PAUSED);
            }
        } else
            m_paused = true;

        if (m_requestedState == GST_STATE_PAUSED && state == GST_STATE_PAUSED) {
            shouldUpdatePlaybackState = true;
            GST_DEBUG("Requested state change to %s was completed", gst_element_state_get_name(state));
        }

        break;
    }
    case GST_STATE_CHANGE_ASYNC:
        GST_DEBUG("Async: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));
        // Change in progress.
        break;
    case GST_STATE_CHANGE_FAILURE:
        GST_WARNING("Failure: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));
        // Change failed.
        return;
    case GST_STATE_CHANGE_NO_PREROLL:
        GST_DEBUG("No preroll: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));

        // Live pipelines go in PAUSED without prerolling.
        m_isStreaming = true;

        if (state == GST_STATE_READY) {
            m_readyState = MediaPlayer::HaveNothing;
            GST_DEBUG("m_readyState=%s", dumpReadyState(m_readyState));
        } else if (state == GST_STATE_PAUSED) {
            m_readyState = MediaPlayer::HaveEnoughData;
            GST_DEBUG("m_readyState=%s", dumpReadyState(m_readyState));
            m_paused = true;
        } else if (state == GST_STATE_PLAYING)
            m_paused = false;

        if (!m_paused && m_playbackRate)
            changePipelineState(GST_STATE_PLAYING);

        m_networkState = MediaPlayer::Loading;
        break;
    default:
        GST_DEBUG("Else : %d", getStateResult);
        break;
    }

    m_requestedState = GST_STATE_VOID_PENDING;

    if (shouldUpdatePlaybackState)
        m_player->playbackStateChanged();

    if (m_networkState != oldNetworkState) {
        GST_DEBUG("Network State Changed from %u to %u", oldNetworkState, m_networkState);
        m_player->networkStateChanged();
    }
    if (m_readyState != oldReadyState) {
        GST_DEBUG("Ready State Changed from %u to %u", oldReadyState, m_readyState);
        m_player->readyStateChanged();
    }

    if (getStateResult == GST_STATE_CHANGE_SUCCESS && state >= GST_STATE_PAUSED) {
        updatePlaybackRate();
        maybeFinishSeek();
    }
}
void MediaPlayerPrivateGStreamerMSE::asyncStateChangeDone()
{
    if (!m_pipeline || m_errorOccured)
        return;

    if (m_seeking)
        maybeFinishSeek();
    else
        updateStates();
}

bool MediaPlayerPrivateGStreamerMSE::isTimeBuffered(const MediaTime &time) const
{
    bool result = m_mediaSource && m_mediaSource->buffered()->contain(time);
    GST_DEBUG("Time %f buffered? %s", time.toDouble(), result ? "Yes" : "No");
    return result;
}

void MediaPlayerPrivateGStreamerMSE::setMediaSourceClient(PassRefPtr<MediaSourceClientGStreamerMSE> client)
{
    m_mediaSourceClient = client;
}

RefPtr<MediaSourceClientGStreamerMSE> MediaPlayerPrivateGStreamerMSE::mediaSourceClient()
{
    return m_mediaSourceClient;
}

bool MediaPlayerPrivateGStreamerMSE::loadingProgressed() const
{
    bool loadingProgressed = m_loadingProgressed;
    m_loadingProgressed = false;
    return loadingProgressed;
}

void MediaPlayerPrivateGStreamerMSE::setLoadingProgressed(bool loadingProgressed)
{
    m_loadingProgressed = loadingProgressed;
}

void MediaPlayerPrivateGStreamerMSE::durationChanged()
{
    MediaTime previousDuration = m_mediaTimeDuration;

    if (!m_mediaSourceClient) {
        GST_DEBUG("m_mediaSourceClient is null, not doing anything");
        return;
    }
    m_mediaTimeDuration = m_mediaSourceClient->duration();

    GST_TRACE("previous=%f, new=%f", previousDuration.toFloat(), m_mediaTimeDuration.toFloat());

    // Avoid emiting durationchanged in the case where the previous
    // duration was 0 because that case is already handled by the
    // HTMLMediaElement.
    if (m_mediaTimeDuration != previousDuration && m_mediaTimeDuration.isValid() && previousDuration.isValid()) {
        m_player->durationChanged();
        m_playbackPipeline->notifyDurationChanged();
        m_mediaSource->durationChanged(m_mediaTimeDuration);
    }
}

static HashSet<String, ASCIICaseInsensitiveHash>& mimeTypeCache()
{
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cache = []()
    {
        initializeGStreamerAndRegisterWebKitMESElement();
        HashSet<String, ASCIICaseInsensitiveHash> set;
        const char* mimeTypes[] = {
#if !USE(HOLE_PUNCH_EXTERNAL)
            "video/mp4",
#endif
            "audio/mp4"
        };
        for (auto& type : mimeTypes)
            set.add(type);
        return set;
    }();
    return cache;
}

void MediaPlayerPrivateGStreamerMSE::getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types)
{
    types = mimeTypeCache();
}

void MediaPlayerPrivateGStreamerMSE::trackDetected(RefPtr<AppendPipeline> appendPipeline, RefPtr<WebCore::TrackPrivateBase> oldTrack, RefPtr<WebCore::TrackPrivateBase> newTrack)
{
    ASSERT(appendPipeline->track() == newTrack);

    GstCaps* caps = appendPipeline->appsinkCaps();
    ASSERT(caps);
    GST_DEBUG("track ID: %s, caps: %" GST_PTR_FORMAT, newTrack->id().string().latin1().data(), caps);

    GstStructure* structure = gst_caps_get_structure(caps, 0);
    const gchar* mediaType = gst_structure_get_name(structure);
    GstVideoInfo info;

    if (g_str_has_prefix(mediaType, "video/") && gst_video_info_from_caps(&info, caps)) {
        float width, height;

        width = info.width;
        height = info.height * ((float) info.par_d / (float) info.par_n);
        m_videoSize.setWidth(width);
        m_videoSize.setHeight(height);
    }

    if (!oldTrack)
        m_playbackPipeline->attachTrack(appendPipeline->sourceBufferPrivate(), newTrack, structure, caps);
    else
        m_playbackPipeline->reattachTrack(appendPipeline->sourceBufferPrivate(), newTrack);
}

MediaPlayer::SupportsType MediaPlayerPrivateGStreamerMSE::supportsType(const MediaEngineSupportParameters& parameters)
{
    MediaPlayer::SupportsType result = MediaPlayer::IsNotSupported;
    if (!parameters.isMediaSource)
        return result;

    // Disable VPX/Opus on MSE for now, mp4/avc1 seems way more reliable currently.
    if (parameters.type.endsWith("webm"))
        return result;

    // YouTube TV provides empty types for some videos and we want to be selected as best media engine for them.
    if (parameters.type.isNull() || parameters.type.isEmpty()) {
        result = MediaPlayer::MayBeSupported;
        return result;
    }

    // Spec says we should not return "probably" if the codecs string is empty.
    if (mimeTypeCache().contains(parameters.type))
        result = parameters.codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;

    return extendedSupportsType(parameters, result);
}

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
void MediaPlayerPrivateGStreamerMSE::dispatchDecryptionKey(GstBuffer* buffer)
{
    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it)
        it->value->dispatchDecryptionKey(buffer);
}
#endif

#if USE(PLAYREADY)
void MediaPlayerPrivateGStreamerMSE::emitSession()
{
    PlayreadySession* session = prSession();
    if (!session->ready())
        return;

    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it) {
        gst_element_send_event(it->value->pipeline(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
            gst_structure_new("playready-session", "session", G_TYPE_POINTER, session, nullptr)));
        it->value->setAppendState(AppendPipeline::AppendState::Ongoing);
    }
}
#endif

void MediaPlayerPrivateGStreamerMSE::markEndOfStream(MediaSourcePrivate::EndOfStreamStatus status)
{
    if (status != MediaSourcePrivate::EosNoError)
        return;

    GST_DEBUG("Marking end of stream");
    m_eosPending = true;
}

class GStreamerMediaDescription : public MediaDescription {
private:
    GRefPtr<GstCaps> m_caps;
public:
    static PassRefPtr<GStreamerMediaDescription> create(GstCaps* caps)
    {
        return adoptRef(new GStreamerMediaDescription(caps));
    }

    virtual ~GStreamerMediaDescription() { }

    AtomicString codec() const override
    {
        GUniquePtr<gchar> description(gst_pb_utils_get_codec_description(m_caps.get()));
        String codecName(description.get());

        // Report "H.264 (Main Profile)" and "H.264 (High Profile)" just as
        // "H.264" to allow changes between both variants go unnoticed to the
        // SourceBuffer layer.
        if (codecName.startsWith("H.264")) {
            size_t braceStart = codecName.find(" (");
            size_t braceEnd = codecName.find(")");
            if (braceStart != notFound && braceEnd != notFound)
                codecName.remove(braceStart, braceEnd-braceStart);
        }
        AtomicString simpleCodecName(codecName);

        return simpleCodecName;
    }

    bool isVideo() const override
    {
        GstStructure* structure = gst_caps_get_structure(m_caps.get(), 0);
        const gchar* name = gst_structure_get_name(structure);

#if GST_CHECK_VERSION(1, 5, 3)
        if (!g_strcmp0(name, "application/x-cenc"))
            return g_str_has_prefix(gst_structure_get_string(structure, "original-media-type"), "video/");
#endif
        return g_str_has_prefix(name, "video/");
    }

    bool isAudio() const override
    {
        GstStructure* structure = gst_caps_get_structure(m_caps.get(), 0);
        const gchar* name = gst_structure_get_name(structure);

#if GST_CHECK_VERSION(1, 5, 3)
        if (!g_strcmp0(name, "application/x-cenc"))
            return g_str_has_prefix(gst_structure_get_string(structure, "original-media-type"), "audio/");
#endif
        return g_str_has_prefix(name, "audio/");
    }

    bool isText() const override
    {
        // FIXME: Implement proper text track support.
        return false;
    }

private:
    GStreamerMediaDescription(GstCaps* caps)
        : MediaDescription()
        , m_caps(caps)
    {
    }
};

GStreamerMediaSample::GStreamerMediaSample(GstSample* sample, const FloatSize& presentationSize, const AtomicString& trackId)
    : MediaSample()
    , m_pts(MediaTime::zeroTime())
    , m_dts(MediaTime::zeroTime())
    , m_duration(MediaTime::zeroTime())
    , m_trackId(trackId)
    , m_size(0)
    , m_presentationSize(presentationSize)
    , m_flags(MediaSample::IsSync)
{

    if (!sample)
        return;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer)
        return;

    auto createMediaTime =
        [](GstClockTime time) -> MediaTime {
            return MediaTime(GST_TIME_AS_USECONDS(time), G_USEC_PER_SEC);
        };

    if (GST_BUFFER_PTS_IS_VALID(buffer))
        m_pts = createMediaTime(GST_BUFFER_PTS(buffer));
    if (GST_BUFFER_DTS_IS_VALID(buffer))
        m_dts = createMediaTime(GST_BUFFER_DTS(buffer));
    if (GST_BUFFER_DURATION_IS_VALID(buffer))
        m_duration = createMediaTime(GST_BUFFER_DURATION(buffer));

    m_size = gst_buffer_get_size(buffer);
    m_sample = sample;

    if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))
        m_flags = MediaSample::None;

    if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY))
        m_flags = (MediaSample::SampleFlags) (m_flags | MediaSample::NonDisplaying);
}

PassRefPtr<GStreamerMediaSample> GStreamerMediaSample::create(GstSample* sample, const FloatSize& presentationSize, const AtomicString& trackId)
{
    return adoptRef(new GStreamerMediaSample(sample, presentationSize, trackId));
}

PassRefPtr<GStreamerMediaSample> GStreamerMediaSample::createFakeSample(GstCaps*, MediaTime pts, MediaTime dts, MediaTime duration, const FloatSize& presentationSize, const AtomicString& trackId)
{
    GstSample* gstSample = nullptr;
    GStreamerMediaSample* gstreamerMediaSample = new GStreamerMediaSample(gstSample, presentationSize, trackId);
    gstreamerMediaSample->m_pts = pts;
    gstreamerMediaSample->m_dts = dts;
    gstreamerMediaSample->m_duration = duration;
    gstreamerMediaSample->m_flags = MediaSample::NonDisplaying;
    return adoptRef(gstreamerMediaSample);
}

void GStreamerMediaSample::applyPtsOffset(MediaTime timestampOffset)
{
    if (m_pts > timestampOffset) {
        m_duration = m_duration + (m_pts - timestampOffset);
        m_pts = timestampOffset;
    }
}

void GStreamerMediaSample::offsetTimestampsBy(const MediaTime& timestampOffset)
{
    if (!timestampOffset)
        return;
    m_pts += timestampOffset;
    m_dts += timestampOffset;
    GstBuffer* buffer = gst_sample_get_buffer(m_sample.get());
    if (buffer) {
        GST_BUFFER_PTS(buffer) = toGstClockTime(m_pts.toFloat());
        GST_BUFFER_DTS(buffer) = toGstClockTime(m_dts.toFloat());
    }
}

GStreamerMediaSample::~GStreamerMediaSample()
{
}

// Auxiliar to pass several parameters to appendPipelineAppsinkNewSampleMainThread().
class NewSampleInfo {
public:
    NewSampleInfo(GstSample* sample, AppendPipeline* appendPipeline)
    {
        m_sample = sample;
        m_appendPipeline = appendPipeline;
    }
    virtual ~NewSampleInfo()
    {
    }

    GstSample* sample() { return m_sample.get(); }
    RefPtr<AppendPipeline> appendPipeline() { return m_appendPipeline; }

private:
    GRefPtr<GstSample> m_sample;
    RefPtr<AppendPipeline> m_appendPipeline;
};

static const char* dumpAppendState(AppendPipeline::AppendState appendState)
{
    switch (appendState) {
    case AppendPipeline::AppendState::Invalid:
        return "Invalid";
    case AppendPipeline::AppendState::NotStarted:
        return "NotStarted";
    case AppendPipeline::AppendState::Ongoing:
        return "Ongoing";
    case AppendPipeline::AppendState::KeyNegotiation:
        return "KeyNegotiation";
    case AppendPipeline::AppendState::DataStarve:
        return "DataStarve";
    case AppendPipeline::AppendState::Sampling:
        return "Sampling";
    case AppendPipeline::AppendState::LastSample:
        return "LastSample";
    case AppendPipeline::AppendState::Aborting:
        return "Aborting";
    default:
        return "(unknown)";
    }
}

static void appendPipelineAppsrcNeedData(GstAppSrc*, guint, AppendPipeline*);
static void appendPipelineDemuxerPadAdded(GstElement*, GstPad*, AppendPipeline*);
static void appendPipelineDemuxerPadRemoved(GstElement*, GstPad*, AppendPipeline*);
static void appendPipelineAppsinkCapsChanged(GObject*, GParamSpec*, AppendPipeline*);
static GstPadProbeReturn appendPipelineAppsrcDataLeaving(GstPad*, GstPadProbeInfo*, AppendPipeline*);
#ifdef DEBUG_APPEND_PIPELINE_PADS
static GstPadProbeReturn appendPipelinePadProbeDebugInformation(GstPad*, GstPadProbeInfo*, struct PadProbeInformation*);
#endif
static GstFlowReturn appendPipelineAppsinkNewSample(GstElement*, AppendPipeline*);
static void appendPipelineAppsinkEOS(GstElement*, AppendPipeline*);

static void appendPipelineNeedContextMessageCallback(GstBus*, GstMessage* message, AppendPipeline* appendPipeline)
{
    GST_TRACE("received callback");
    appendPipeline->handleNeedContextSyncMessage(message);
}

static void appendPipelineApplicationMessageCallback(GstBus*, GstMessage* message, AppendPipeline* appendPipeline)
{
    appendPipeline->handleApplicationMessage(message);
}

gint AppendPipeline::totalAudio = 0;
gint AppendPipeline::totalVideo = 0;
gint AppendPipeline::totalText = 0;

AppendPipeline::AppendPipeline(PassRefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient, PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, MediaPlayerPrivateGStreamerMSE* playerPrivate)
    : m_mediaSourceClient(mediaSourceClient)
    , m_sourceBufferPrivate(sourceBufferPrivate)
    , m_playerPrivate(playerPrivate)
    , m_id(0)
    , m_appsrcAtLeastABufferLeft(false)
    , m_appsrcNeedDataReceived(false)
    , m_appsrcDataLeavingProbeId(0)
    , m_appendState(NotStarted)
    , m_abortPending(false)
    , m_streamType(Unknown)
{
    ASSERT(WTF::isMainThread());

    GST_DEBUG("%p", this);

    // FIXME: give a name to the pipeline, maybe related with the track it's managing.
    // The track name is still unknown at this time, though.
    m_pipeline = adoptGRef(gst_pipeline_new(nullptr));

    m_bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline.get())));
    gst_bus_add_signal_watch_full(m_bus.get(), G_PRIORITY_HIGH + 30);
    gst_bus_enable_sync_message_emission(m_bus.get());

    g_signal_connect(m_bus.get(), "sync-message::need-context", G_CALLBACK(appendPipelineNeedContextMessageCallback), this);
    g_signal_connect(m_bus.get(), "message::application", G_CALLBACK(appendPipelineApplicationMessageCallback), this);

    // We assign the created instances here instead of adoptRef() because gst_bin_add_many()
    // below will already take the initial reference and we need an additional one for us.
    m_appsrc = gst_element_factory_make("appsrc", nullptr);
    m_demux = gst_element_factory_make("qtdemux", nullptr);
    m_appsink = gst_element_factory_make("appsink", nullptr);

    g_object_set(G_OBJECT(m_demux.get()), "always-honor-tfdt", TRUE, nullptr);

    gst_app_sink_set_emit_signals(GST_APP_SINK(m_appsink.get()), TRUE);
    gst_base_sink_set_sync(GST_BASE_SINK(m_appsink.get()), FALSE);

    GRefPtr<GstPad> appsinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));
    g_signal_connect(appsinkPad.get(), "notify::caps", G_CALLBACK(appendPipelineAppsinkCapsChanged), this);

    setAppsrcDataLeavingProbe();

#ifdef DEBUG_APPEND_PIPELINE_PADS
    GRefPtr<GstPad> demuxerPad = adoptGRef(gst_element_get_static_pad(m_demux.get(), "sink"));
    m_demuxerDataEnteringPadProbeInformation.appendPipeline = this;
    m_demuxerDataEnteringPadProbeInformation.description = "demuxer data entering";
    m_demuxerDataEnteringPadProbeInformation.probeId = gst_pad_add_probe(demuxerPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelinePadProbeDebugInformation), &m_demuxerDataEnteringPadProbeInformation, nullptr);
    m_appsinkDataEnteringPadProbeInformation.appendPipeline = this;
    m_appsinkDataEnteringPadProbeInformation.description = "appsink data entering";
    m_appsinkDataEnteringPadProbeInformation.probeId = gst_pad_add_probe(appsinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelinePadProbeDebugInformation), &m_appsinkDataEnteringPadProbeInformation, nullptr);
#endif

    // These signals won't be connected outside of the lifetime of "this".
    g_signal_connect(m_appsrc.get(), "need-data", G_CALLBACK(appendPipelineAppsrcNeedData), this);
    g_signal_connect(m_demux.get(), "pad-added", G_CALLBACK(appendPipelineDemuxerPadAdded), this);
    g_signal_connect(m_demux.get(), "pad-removed", G_CALLBACK(appendPipelineDemuxerPadRemoved), this);
    g_signal_connect(m_appsink.get(), "new-sample", G_CALLBACK(appendPipelineAppsinkNewSample), this);
    g_signal_connect(m_appsink.get(), "eos", G_CALLBACK(appendPipelineAppsinkEOS), this);

    // Add_many will take ownership of a reference. That's why we used an assignment before.
    gst_bin_add_many(GST_BIN(m_pipeline.get()), m_appsrc.get(), m_demux.get(), nullptr);
    gst_element_link(m_appsrc.get(), m_demux.get());

    gst_element_set_state(m_pipeline.get(), GST_STATE_READY);
};

AppendPipeline::~AppendPipeline()
{
    ASSERT(WTF::isMainThread());

    LockHolder newSampleLocker(m_newSampleLock);
    setAppendState(Invalid);
    m_newSampleCondition.notifyOne();
    newSampleLocker.unlockEarly();

    LockHolder padAddRemoveLocker(m_padAddRemoveLock);
    m_playerPrivate = nullptr;
    m_padAddRemoveCondition.notifyOne();
    padAddRemoveLocker.unlockEarly();

    GST_DEBUG("%p", this);

    // FIXME: Maybe notify appendComplete here?.

    if (m_pipeline) {
        ASSERT(m_bus);
        g_signal_handlers_disconnect_by_func(m_bus.get(), reinterpret_cast<gpointer>(appendPipelineNeedContextMessageCallback), this);
        gst_bus_disable_sync_message_emission(m_bus.get());
        gst_bus_remove_signal_watch(m_bus.get());

        gst_element_set_state(m_pipeline.get(), GST_STATE_NULL);
        m_pipeline = nullptr;
    }

    if (m_appsrc) {
        removeAppsrcDataLeavingProbe();
        g_signal_handlers_disconnect_by_data(m_appsrc.get(), this);
        m_appsrc = nullptr;
    }

    if (m_demux) {
#ifdef DEBUG_APPEND_PIPELINE_PADS
        GRefPtr<GstPad> demuxerPad = adoptGRef(gst_element_get_static_pad(m_demux.get(), "sink"));
        gst_pad_remove_probe(demuxerPad.get(), m_demuxerDataEnteringPadProbeInformation.probeId);
#endif

        g_signal_handlers_disconnect_by_data(m_demux.get(), this);
        m_demux = nullptr;
    }

    if (m_appsink) {
        GRefPtr<GstPad> appsinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));
        g_signal_handlers_disconnect_by_data(appsinkPad.get(), this);
        g_signal_handlers_disconnect_by_data(m_appsink.get(), this);

#ifdef DEBUG_APPEND_PIPELINE_PADS
        gst_pad_remove_probe(appsinkPad.get(), m_appsinkDataEnteringPadProbeInformation.probeId);
#endif

        m_appsink = nullptr;
    }

    if (m_appsinkCaps)
        m_appsinkCaps = nullptr;

    if (m_demuxerSrcPadCaps)
        m_demuxerSrcPadCaps = nullptr;
};

void AppendPipeline::dispatchPendingDecryptionKey()
{
    ASSERT(m_decryptor);
    ASSERT(m_pendingKey);
    ASSERT(m_appendState == KeyNegotiation);
    GST_TRACE("dispatching key to append pipeline %p", this);
    gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
        gst_structure_new("drm-cipher", "key", GST_TYPE_BUFFER, m_pendingKey.get(), nullptr)));
    m_pendingKey.clear();
    setAppendState(Ongoing);
}

void AppendPipeline::dispatchDecryptionKey(GstBuffer* buffer)
{
    if (m_appendState == KeyNegotiation) {
        GST_TRACE("append pipeline %p in key negotiation", this);
        m_pendingKey = buffer;
        if (m_decryptor)
            dispatchPendingDecryptionKey();
        else
            GST_TRACE("no decryptor yet, waiting for it");
    } else
        GST_TRACE("append pipeline %p not in key negotiation", this);
}

void AppendPipeline::clearPlayerPrivate()
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("cleaning private player");

    LockHolder newSampleLocker(m_newSampleLock);
    // Make sure that AppendPipeline won't process more data from now on and
    // instruct handleNewSample to abort itself from now on as well.
    setAppendState(Invalid);

    // Awake any pending handleNewSample operation in the streaming thread.
    m_newSampleCondition.notifyOne();
    newSampleLocker.unlockEarly();

    LockHolder padAddRemoveLocker(m_padAddRemoveLock);
    m_playerPrivate = nullptr;
    m_padAddRemoveCondition.notifyOne();
    padAddRemoveLocker.unlockEarly();

    // And now that no handleNewSample operations will remain stalled waiting
    // for the main thread, stop the pipeline.
    if (m_pipeline)
        gst_element_set_state(m_pipeline.get(), GST_STATE_NULL);
}

void AppendPipeline::handleNeedContextSyncMessage(GstMessage* message)
{
    const gchar* contextType = nullptr;
    gst_message_parse_context_type(message, &contextType);
    GST_TRACE("context type: %s", contextType);
    if (g_strcmp0(contextType, "drm-preferred-decryption-system-id") == 0)
        setAppendState(AppendPipeline::AppendState::KeyNegotiation);

    // MediaPlayerPrivateGStreamerBase will take care of setting up encryption.
    if (m_playerPrivate)
        m_playerPrivate->handleSyncMessage(message);
}

void AppendPipeline::handleApplicationMessage(GstMessage* message)
{
    ASSERT(WTF::isMainThread());

    const GstStructure* structure = gst_message_get_structure(message);

    if (gst_structure_has_name(structure, "appsrc-need-data")) {
        handleAppsrcNeedDataReceived();
        return;
    }

    if (gst_structure_has_name(structure, "appsrc-buffer-left")) {
        handleAppsrcAtLeastABufferLeft();
        return;
    }

    if (gst_structure_has_name(structure, "demuxer-connect-to-appsink")) {
        GstPad* demuxerSrcPad = nullptr;
        gst_structure_get(structure, "demuxer-src-pad", G_TYPE_POINTER, &demuxerSrcPad, nullptr);
        ASSERT(demuxerSrcPad);
        connectDemuxerSrcPadToAppsink(demuxerSrcPad);
        gst_object_unref(demuxerSrcPad);
        return;
    }

    if (gst_structure_has_name(structure, "demuxer-disconnect-from-appsink")) {
        disconnectDemuxerSrcPadFromAppsink();
        return;
    }

    if (gst_structure_has_name(structure, "appsink-caps-changed")) {
        appsinkCapsChanged();
        deref();
        return;
    }

    if (gst_structure_has_name(structure, "appsink-new-sample")) {
        GstSample* newSample = nullptr;
        gst_structure_get(structure, "new-sample", G_TYPE_POINTER, &newSample, nullptr);

        appsinkNewSample(newSample);
        gst_sample_unref(newSample);
        deref();
        return;
    }

    if (gst_structure_has_name(structure, "appsink-eos")) {
        appsinkEOS();
        deref();
        return;
    }

    ASSERT_NOT_REACHED();
}

void AppendPipeline::handleAppsrcNeedDataReceived()
{
    if (!m_appsrcAtLeastABufferLeft) {
        GST_TRACE("discarding until at least a buffer leaves appsrc");
        return;
    }

    ASSERT(m_appendState == Ongoing || m_appendState == Sampling);
    ASSERT(!m_appsrcNeedDataReceived);

    GST_TRACE("received need-data from appsrc");

    m_appsrcNeedDataReceived = true;
    checkEndOfAppend();
}

void AppendPipeline::handleAppsrcAtLeastABufferLeft()
{
    m_appsrcAtLeastABufferLeft = true;
    GST_TRACE("received buffer-left from appsrc");
#ifndef DEBUG_APPEND_PIPELINE_PADS
    removeAppsrcDataLeavingProbe();
#endif
}

gint AppendPipeline::id()
{
    ASSERT(WTF::isMainThread());

    if (m_id)
        return m_id;

    switch (m_streamType) {
    case Audio:
        totalAudio++;
        m_id = totalAudio;
        break;
    case Video:
        totalVideo++;
        m_id = totalVideo;
        break;
    case Text:
        totalText++;
        m_id = totalText;
        break;
    case Unknown:
    case Invalid:
        GST_DEBUG("Trying to get id for a pipeline of Unknown/Invalid type");
        ASSERT_NOT_REACHED();
        break;
    }

    GST_DEBUG("streamType=%d, id=%d", static_cast<int>(m_streamType), m_id);

    return m_id;
}

void AppendPipeline::setAppendState(AppendState newAppendState)
{
    ASSERT(WTF::isMainThread());
    // Valid transitions:
    // NotStarted-->Ongoing-->DataStarve-->NotStarted
    //           |         |            `->Aborting-->NotStarted
    //           |         `->Sampling-···->Sampling-->LastSample-->NotStarted
    //           |         |                                     `->Aborting-->NotStarted
    //           |         `->KeyNegotiation-->Ongoing-->[...]
    //           `->Aborting-->NotStarted
    AppendState oldAppendState = m_appendState;
    AppendState nextAppendState = Invalid;

    bool ok = false;

    if (oldAppendState != newAppendState)
        GST_TRACE("%s --> %s", dumpAppendState(oldAppendState), dumpAppendState(newAppendState));

    switch (oldAppendState) {
    case NotStarted:
        switch (newAppendState) {
        case Ongoing:
            ok = true;
            gst_element_set_state(m_pipeline.get(), GST_STATE_PLAYING);
            break;
        case NotStarted:
            ok = true;
            if (m_pendingBuffer) {
                GST_TRACE("pushing pending buffer %p", m_pendingBuffer.get());
                gst_app_src_push_buffer(GST_APP_SRC(appsrc()), m_pendingBuffer.leakRef());
                nextAppendState = Ongoing;
            }
            break;
        case Aborting:
            ok = true;
            nextAppendState = NotStarted;
            break;
        case Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case KeyNegotiation:
        switch (newAppendState) {
        case Ongoing:
        case Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case Ongoing:
        switch (newAppendState) {
        case KeyNegotiation:
        case Sampling:
        case Invalid:
            ok = true;
            break;
        case DataStarve:
            ok = true;
            m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
            if (m_abortPending)
                nextAppendState = Aborting;
            else
                nextAppendState = NotStarted;
            break;
        default:
            break;
        }
        break;
    case DataStarve:
        switch (newAppendState) {
        case NotStarted:
        case Invalid:
            ok = true;
            break;
        case Aborting:
            ok = true;
            nextAppendState = NotStarted;
            break;
        default:
            break;
        }
        break;
    case Sampling:
        switch (newAppendState) {
        case Sampling:
        case Invalid:
            ok = true;
            break;
        case LastSample:
            ok = true;
            m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
            if (m_abortPending)
                nextAppendState = Aborting;
            else
                nextAppendState = NotStarted;
            break;
        default:
            break;
        }
        break;
    case LastSample:
        switch (newAppendState) {
        case NotStarted:
        case Invalid:
            ok = true;
            break;
        case Aborting:
            ok = true;
            nextAppendState = NotStarted;
            break;
        default:
            break;
        }
        break;
    case Aborting:
        switch (newAppendState) {
        case NotStarted:
            ok = true;
            resetPipeline();
            m_abortPending = false;
            nextAppendState = NotStarted;
            break;
        case Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case Invalid:
        ok = true;
        break;
    }

    if (ok)
        m_appendState = newAppendState;
    else
        GST_ERROR("Invalid append state transition %s --> %s", dumpAppendState(oldAppendState), dumpAppendState(newAppendState));

    ASSERT(ok);

    if (nextAppendState != Invalid)
        setAppendState(nextAppendState);
}

// Takes ownership of caps.
void AppendPipeline::parseDemuxerSrcPadCaps(GstCaps* demuxerSrcPadCaps)
{
    ASSERT(WTF::isMainThread());

    m_demuxerSrcPadCaps = adoptGRef(demuxerSrcPadCaps);

    GstStructure* structure = gst_caps_get_structure(m_demuxerSrcPadCaps.get(), 0);
    const gchar* structureName = gst_structure_get_name(structure);
    GstVideoInfo info;
    bool sizeConfigured = false;

    m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Unknown;

#if GST_CHECK_VERSION(1, 5, 3)
    if (gst_structure_has_name(structure, "application/x-cenc")) {
        const gchar* originalMediaType = gst_structure_get_string(structure, "original-media-type");

        // Any previous decriptor should have been removed from the pipeline by disconnectFromAppSinkFromStreamingThread()
        ASSERT(!m_decryptor);

        m_decryptor = adoptGRef(WebCore::createGstDecryptor(gst_structure_get_string(structure, "protection-system")));
        if (!m_decryptor) {
            GST_ERROR("decryptor not found for caps: %" GST_PTR_FORMAT, m_demuxerSrcPadCaps.get());
            return;
        }

        if (g_str_has_prefix(originalMediaType, "video/")) {
            int width = 0;
            int height = 0;
            float finalHeight = 0;

            gst_structure_get_int(structure, "width", &width);
            if (gst_structure_get_int(structure, "height", &height)) {
                gint par_n = 1;
                gint par_d = 1;

                gst_structure_get_fraction(structure, "pixel-aspect-ratio", &par_n, &par_d);
                finalHeight = height * ((float) par_d / (float) par_n);
            }

            m_presentationSize = WebCore::FloatSize(width, finalHeight);
            m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Video;
        } else {
            m_presentationSize = WebCore::FloatSize();
            if (g_str_has_prefix(originalMediaType, "audio/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Audio;
            else if (g_str_has_prefix(originalMediaType, "text/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Text;
        }
        sizeConfigured = true;
    }
#endif

    if (!sizeConfigured) {
        if (g_str_has_prefix(structureName, "video/") && gst_video_info_from_caps(&info, demuxerSrcPadCaps)) {
            float width, height;

            width = info.width;
            height = info.height * ((float) info.par_d / (float) info.par_n);

            m_presentationSize = WebCore::FloatSize(width, height);
            m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Video;
        } else {
            m_presentationSize = WebCore::FloatSize();
            if (g_str_has_prefix(structureName, "audio/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Audio;
            else if (g_str_has_prefix(structureName, "text/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Text;
        }
    }
}

void AppendPipeline::appsinkCapsChanged()
{
    ASSERT(WTF::isMainThread());

    if (!m_appsink)
        return;

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));
    GRefPtr<GstCaps> caps = adoptGRef(gst_pad_get_current_caps(pad.get()));

    // This means that we're right after a new track has appeared. Otherwise, it's a caps change inside the same track.
    bool previousCapsWereNull = !m_appsinkCaps;

    if (!caps)
        return;

    // Exchange ownership of the current caps variable to m_appsinkCaps.
    // The old m_appsinkCaps will decrease refcount in 1.
    // The new caps will increase refcount in 1 because of ownership transfer inside the replace function.
    // The +1 caps refcount gained with gst_pad_get_current_caps() will be decreased when caps goes out of scope.
    GstCaps*& appsinkCaps = m_appsinkCaps.outPtr();

    if (gst_caps_replace(&appsinkCaps, caps.get())) {
        if (m_playerPrivate && previousCapsWereNull)
            m_playerPrivate->trackDetected(this, m_oldTrack, m_track);
        didReceiveInitializationSegment();
        gst_element_set_state(m_pipeline.get(), GST_STATE_PLAYING);
    }
}

void AppendPipeline::checkEndOfAppend()
{
    ASSERT(WTF::isMainThread());

    if (!m_appsrcNeedDataReceived || (m_appendState != Ongoing && m_appendState != Sampling))
        return;

    GST_TRACE("end of append data mark was received");

    switch (m_appendState) {
    case Ongoing:
        GST_TRACE("DataStarve");
        m_appsrcNeedDataReceived = false;
        setAppendState(DataStarve);
        break;
    case Sampling:
        GST_TRACE("LastSample");
        m_appsrcNeedDataReceived = false;
        setAppendState(LastSample);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void AppendPipeline::appsinkNewSample(GstSample* sample)
{
    ASSERT(WTF::isMainThread());

    LockHolder locker(m_newSampleLock);

    // Ignore samples if we're not expecting them. Refuse processing if we're in Invalid state.
    if (m_appendState != Ongoing && m_appendState != Sampling) {
        GST_WARNING("Unexpected sample, appendState=%s", dumpAppendState(m_appendState));
        // FIXME: Return ERROR and find a more robust way to detect that all the
        // data has been processed, so we don't need to resort to these hacks.
        // All in all, return OK, even if it's not the proper thing to do. We don't want to break the demuxer.
        m_flowReturn = GST_FLOW_OK;
        m_newSampleCondition.notifyOne();
        return;
    }

    RefPtr<GStreamerMediaSample> mediaSample = WebCore::GStreamerMediaSample::create(sample, m_presentationSize, trackId());

    GST_TRACE("append: trackId=%s PTS=%f presentationSize=%.0fx%.0f", mediaSample->trackID().string().utf8().data(), mediaSample->presentationTime().toFloat(), mediaSample->presentationSize().width(), mediaSample->presentationSize().height());

    // If we're beyond the duration, ignore this sample and the remaining ones.
    MediaTime duration = m_mediaSourceClient->duration();
    if (duration.isValid() && !duration.indefiniteTime() && mediaSample->presentationTime() > duration) {
        GST_DEBUG("Detected sample (%f) beyond the duration (%f), declaring LastSample", mediaSample->presentationTime().toFloat(), duration.toFloat());
        setAppendState(LastSample);
        m_flowReturn = GST_FLOW_OK;
        m_newSampleCondition.notifyOne();
        return;
    }

    // Add a gap sample if a gap is detected before the first sample.
    if (mediaSample->decodeTime() == MediaTime::zeroTime()
        && mediaSample->presentationTime() > MediaTime::zeroTime()
        && mediaSample->presentationTime() <= MediaTime::createWithDouble(0.1)) {
        GST_DEBUG("Adding gap offset");
        mediaSample->applyPtsOffset(MediaTime::zeroTime());
    }

    m_sourceBufferPrivate->didReceiveSample(mediaSample);
    setAppendState(Sampling);
    m_flowReturn = GST_FLOW_OK;
    m_newSampleCondition.notifyOne();
    locker.unlockEarly();

    checkEndOfAppend();
}

void AppendPipeline::appsinkEOS()
{
    ASSERT(WTF::isMainThread());

    switch (m_appendState) {
    // Ignored. Operation completion will be managed by the Aborting->NotStarted transition.
    case Aborting:
        return;
    // Finish Ongoing and Sampling states.
    case Ongoing:
        setAppendState(DataStarve);
        break;
    case Sampling:
        setAppendState(LastSample);
        break;
    default:
        GST_DEBUG("Unexpected EOS");
        break;
    }
}

void AppendPipeline::didReceiveInitializationSegment()
{
    ASSERT(WTF::isMainThread());

    WebCore::SourceBufferPrivateClient::InitializationSegment initializationSegment;

    GST_DEBUG("Notifying SourceBuffer for track %s", m_track->id().string().utf8().data());
    initializationSegment.duration = m_mediaSourceClient->duration();
    switch (m_streamType) {
    case Audio: {
        WebCore::SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
        info.track = static_cast<AudioTrackPrivateGStreamer*>(m_track.get());
        info.description = WebCore::GStreamerMediaDescription::create(m_demuxerSrcPadCaps.get());
        initializationSegment.audioTracks.append(info);
        break;
    }
    case Video: {
        WebCore::SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
        info.track = static_cast<VideoTrackPrivateGStreamer*>(m_track.get());
        info.description = WebCore::GStreamerMediaDescription::create(m_demuxerSrcPadCaps.get());
        initializationSegment.videoTracks.append(info);
        break;
    }
    default:
        GST_DEBUG("Unsupported or unknown stream type");
        ASSERT_NOT_REACHED();
        break;
    }

    m_mediaSourceClient->didReceiveInitializationSegment(m_sourceBufferPrivate.get(), initializationSegment);
}

AtomicString AppendPipeline::trackId()
{
    ASSERT(WTF::isMainThread());

    if (!m_track)
        return AtomicString();

    return m_track->id();
}

void AppendPipeline::resetPipeline()
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("resetting pipeline");
    m_appsrcAtLeastABufferLeft = false;
    setAppsrcDataLeavingProbe();

    LockHolder locker(m_newSampleLock);
    m_newSampleCondition.notifyOne();
    gst_element_set_state(m_pipeline.get(), GST_STATE_READY);
    gst_element_get_state(m_pipeline.get(), nullptr, nullptr, 0);
    locker.unlockEarly();

    {
        // This is here for debugging purposes. It does not make sense to have it as class member.
        static int i = 0;
        WTF::String  dotFileName = String::format("reset-pipeline-%d", ++i);
        gst_debug_bin_to_dot_file(GST_BIN(m_pipeline.get()), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.utf8().data());
    }

}

void AppendPipeline::setAppsrcDataLeavingProbe()
{
    if (m_appsrcDataLeavingProbeId)
        return;

    GST_TRACE("setting appsrc data leaving probe");

    GRefPtr<GstPad> appsrcPad = adoptGRef(gst_element_get_static_pad(m_appsrc.get(), "src"));
    m_appsrcDataLeavingProbeId = gst_pad_add_probe(appsrcPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelineAppsrcDataLeaving), this, nullptr);
}

void AppendPipeline::removeAppsrcDataLeavingProbe()
{
    if (!m_appsrcDataLeavingProbeId)
        return;

    GST_TRACE("removing appsrc data leaving probe");

    GRefPtr<GstPad> appsrcPad = adoptGRef(gst_element_get_static_pad(m_appsrc.get(), "src"));
    gst_pad_remove_probe(appsrcPad.get(), m_appsrcDataLeavingProbeId);
    m_appsrcDataLeavingProbeId = 0;
}

void AppendPipeline::abort()
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("aborting");

    m_pendingBuffer.clear();

    // Abort already ongoing.
    if (m_abortPending)
        return;

    m_abortPending = true;
    if (m_appendState == NotStarted)
        setAppendState(Aborting);
    // Else, the automatic state transitions will take care when the ongoing append finishes.
}

GstFlowReturn AppendPipeline::pushNewBuffer(GstBuffer* buffer)
{
    GstFlowReturn result;

    if (m_abortPending) {
        m_pendingBuffer = adoptGRef(buffer);
        result = GST_FLOW_OK;
    } else {
        setAppendState(AppendPipeline::Ongoing);
        GST_TRACE("pushing new buffer %p", buffer);
        result = gst_app_src_push_buffer(GST_APP_SRC(appsrc()), buffer);
    }

    return result;
}

void AppendPipeline::reportAppsrcAtLeastABufferLeft()
{
    GST_TRACE("buffer left appsrc, reposting to bus");
    GstStructure* structure = gst_structure_new_empty("appsrc-buffer-left");
    GstMessage* message = gst_message_new_application(GST_OBJECT(m_appsrc.get()), structure);
    gst_bus_post(m_bus.get(), message);
}

void AppendPipeline::reportAppsrcNeedDataReceived()
{
    GST_TRACE("received need-data signal at appsrc, reposting to bus");
    GstStructure* structure = gst_structure_new_empty("appsrc-need-data");
    GstMessage* message = gst_message_new_application(GST_OBJECT(m_appsrc.get()), structure);
    gst_bus_post(m_bus.get(), message);
}

GstFlowReturn AppendPipeline::handleNewAppsinkSample(GstElement* appsink)
{
    ASSERT(!WTF::isMainThread());

    LockHolder locker1(m_newSampleLock);
    bool invalid = !m_playerPrivate || m_appendState == Invalid;
    locker1.unlockEarly();

    // Even if we're disabled, it's important to pull the sample out anyway to
    // avoid deadlocks when changing to GST_STATE_NULL having a non empty appsink.
    GRefPtr<GstSample> sample = adoptGRef(gst_app_sink_pull_sample(GST_APP_SINK(appsink)));

    if (invalid) {
        GST_DEBUG("AppendPipeline has been disabled, ignoring this sample");
        return GST_FLOW_ERROR;
    }

    LockHolder locker2(m_newSampleLock);
    if (!(!m_playerPrivate || m_appendState == Invalid)) {
        ref();
        gst_sample_ref(sample.get());
        GstStructure* structure = gst_structure_new("appsink-new-sample", "new-sample", G_TYPE_POINTER, sample.get(), nullptr);
        GstMessage* message = gst_message_new_application(GST_OBJECT(appsink), structure);
        gst_bus_post(m_bus.get(), message);
        GST_TRACE("appsink-new-sample message posted to bus");

        m_newSampleCondition.wait(m_newSampleLock);
        // We've been awaken because the sample was processed or because of
        // an exceptional condition (entered in Invalid state, destructor, etc.)
        // We can't reliably delete info here, appendPipelineAppsinkNewSampleMainThread will do it.
    }
    locker2.unlockEarly();

    return m_flowReturn;
}

void AppendPipeline::connectDemuxerSrcPadToAppsinkFromAnyThread(GstPad* demuxerSrcPad)
{
    if (!m_appsink)
        return;

    GST_DEBUG("connecting to appsink");

    GRefPtr<GstPad> sinkSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));

    // Only one Stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(sinkSinkPad.get()));

    gint64 timeLength = 0;
    if (gst_element_query_duration(m_demux.get(), GST_FORMAT_TIME, &timeLength)
        && static_cast<guint64>(timeLength) != GST_CLOCK_TIME_NONE)
        m_initialDuration = MediaTime(GST_TIME_AS_USECONDS(timeLength), G_USEC_PER_SEC);
    else
        m_initialDuration = MediaTime::positiveInfiniteTime();

    if (WTF::isMainThread())
        connectDemuxerSrcPadToAppsink(demuxerSrcPad);
    else {
        // Call connectDemuxerSrcPadToAppsink() in the main thread and wait.
        LockHolder locker(m_padAddRemoveLock);
        if (!m_playerPrivate)
            return;

        gst_object_ref(GST_OBJECT(demuxerSrcPad));
        GstStructure* structure = gst_structure_new("demuxer-connect-to-appsink", "demuxer-src-pad", G_TYPE_POINTER, demuxerSrcPad, nullptr);
        GstMessage* message = gst_message_new_application(GST_OBJECT(m_demux.get()), structure);
        gst_bus_post(m_bus.get(), message);
        GST_TRACE("demuxer-connect-to-appsink message posted to bus");

        m_padAddRemoveCondition.wait(m_padAddRemoveLock);
    }

    // Must be done in the thread we were called from (usually streaming thread).
    bool isData = false;

    switch (m_streamType) {
    case WebCore::MediaSourceStreamTypeGStreamer::Audio:
    case WebCore::MediaSourceStreamTypeGStreamer::Video:
    case WebCore::MediaSourceStreamTypeGStreamer::Text:
        isData = true;
        break;
    default:
        break;
    }

    if (isData) {
        // FIXME: Only add appsink one time. This method can be called several times.
        GRefPtr<GstObject> parent = adoptGRef(gst_element_get_parent(m_appsink.get()));
        if (!parent)
            gst_bin_add(GST_BIN(m_pipeline.get()), m_appsink.get());

        if (m_decryptor) {
            gst_object_ref(m_decryptor.get());
            gst_bin_add(GST_BIN(m_pipeline.get()), m_decryptor.get());

            GRefPtr<GstPad> decryptorSrcPad = adoptGRef(gst_element_get_static_pad(m_decryptor.get(), "src"));
            GRefPtr<GstPad> decryptorSinkPad = adoptGRef(gst_element_get_static_pad(m_decryptor.get(), "sink"));
            gst_pad_link(demuxerSrcPad, decryptorSinkPad.get());
            gst_pad_link(decryptorSrcPad.get(), sinkSinkPad.get());
            gst_element_sync_state_with_parent(m_appsink.get());
            gst_element_sync_state_with_parent(m_decryptor.get());

            if (m_pendingKey)
                dispatchPendingDecryptionKey();
        } else {
            gst_pad_link(demuxerSrcPad, sinkSinkPad.get());
            gst_element_sync_state_with_parent(m_appsink.get());
        }

        gst_element_set_state(m_pipeline.get(), GST_STATE_PAUSED);
    }
}

void AppendPipeline::connectDemuxerSrcPadToAppsink(GstPad* demuxerSrcPad)
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("Connecting to appsink");

    LockHolder locker(m_padAddRemoveLock);
    GRefPtr<GstPad> sinkSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));

    // Only one Stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(sinkSinkPad.get()));

    GRefPtr<GstCaps> caps = adoptGRef(gst_pad_get_current_caps(GST_PAD(demuxerSrcPad)));

    if (!caps || m_appendState == Invalid || !m_playerPrivate) {
        m_padAddRemoveCondition.notifyOne();
        return;
    }

#if (!(LOG_DISABLED || GST_DISABLE_GST_DEBUG))
    {
        GUniquePtr<gchar> strcaps(gst_caps_to_string(caps.get()));
        GST_DEBUG("%s", strcaps.get());
    }
#endif

    if (m_initialDuration > m_mediaSourceClient->duration())
        m_mediaSourceClient->durationChanged(m_initialDuration);

    m_oldTrack = m_track;

    parseDemuxerSrcPadCaps(gst_caps_ref(caps.get()));

    switch (m_streamType) {
    case WebCore::MediaSourceStreamTypeGStreamer::Audio:
        if (m_playerPrivate)
            m_track = WebCore::AudioTrackPrivateGStreamer::create(m_playerPrivate->pipeline(), id(), sinkSinkPad.get());
        break;
    case WebCore::MediaSourceStreamTypeGStreamer::Video:
        if (m_playerPrivate)
            m_track = WebCore::VideoTrackPrivateGStreamer::create(m_playerPrivate->pipeline(), id(), sinkSinkPad.get());
        break;
    case WebCore::MediaSourceStreamTypeGStreamer::Text:
        m_track = WebCore::InbandTextTrackPrivateGStreamer::create(id(), sinkSinkPad.get());
        break;
    default:
        // No useful data, but notify anyway to complete the append operation.
        GST_DEBUG("(no data)");
        m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
        break;
    }

    m_padAddRemoveCondition.notifyOne();
}

void AppendPipeline::disconnectDemuxerSrcPadFromAppsinkFromAnyThread()
{
    GST_DEBUG("Disconnecting appsink");

    // Must be done in the thread we were called from (usually streaming thread).
    if (m_decryptor) {
        gst_element_unlink(m_decryptor.get(), m_appsink.get());
        gst_element_unlink(m_demux.get(), m_decryptor.get());
        gst_element_set_state(m_decryptor.get(), GST_STATE_NULL);
        gst_bin_remove(GST_BIN(m_pipeline.get()), m_decryptor.get());
    } else
        gst_element_unlink(m_demux.get(), m_appsink.get());

    // Call disconnectDemuxerSrcPadFromAppsink() in the main thread and wait.
    // TODO: Optimize this and call only when there's m_decryptor. By now I call it always to keep code symmetry.
    if (WTF::isMainThread())
        disconnectDemuxerSrcPadFromAppsink();
    else {
        LockHolder locker(m_padAddRemoveLock);
        if (!m_playerPrivate) {
            if (m_decryptor) {
                GST_DEBUG("Releasing decryptor");
                m_decryptor = nullptr;
            }
            return;
        }

        GstStructure* structure = gst_structure_new_empty("demuxer-disconnect-from-appsink");
        GstMessage* message = gst_message_new_application(GST_OBJECT(m_demux.get()), structure);
        gst_bus_post(m_bus.get(), message);
        GST_TRACE("demuxer-disconnect-from-appsink message posted to bus");

        m_padAddRemoveCondition.wait(m_padAddRemoveLock);
    }
}

void AppendPipeline::disconnectDemuxerSrcPadFromAppsink()
{
    ASSERT(WTF::isMainThread());

    LockHolder locker(m_padAddRemoveLock);
    if (m_decryptor) {
        GST_DEBUG("Releasing decryptor");
        m_decryptor = nullptr;
    }
    m_padAddRemoveCondition.notifyOne();
}

static void appendPipelineAppsinkCapsChanged(GObject* appsinkPad, GParamSpec*, AppendPipeline* appendPipeline)
{
    appendPipeline->ref();

    GstStructure* structure = gst_structure_new_empty("appsink-caps-changed");
    GstMessage* message = gst_message_new_application(GST_OBJECT(appsinkPad), structure);
    gst_bus_post(appendPipeline->bus(), message);
    GST_TRACE("appsink-caps-changed message posted to bus");
}

static GstPadProbeReturn appendPipelineAppsrcDataLeaving(GstPad*, GstPadProbeInfo* info, AppendPipeline* appendPipeline)
{
    ASSERT(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER);

    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    gsize bufferSize = gst_buffer_get_size(buffer);

    GST_TRACE("buffer of size %" G_GSIZE_FORMAT " going thru", bufferSize);

    appendPipeline->reportAppsrcAtLeastABufferLeft();

    return GST_PAD_PROBE_OK;
}

#ifdef DEBUG_APPEND_PIPELINE_PADS
static GstPadProbeReturn appendPipelinePadProbeDebugInformation(GstPad*, GstPadProbeInfo* info, struct PadProbeInformation* padProbeInformation)
{
    ASSERT(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GST_TRACE("%s: buffer of size %" G_GSIZE_FORMAT " going thru", padProbeInformation->description, gst_buffer_get_size(buffer));
    return GST_PAD_PROBE_OK;
}
#endif

static void appendPipelineAppsrcNeedData(GstAppSrc*, guint, AppendPipeline* appendPipeline)
{
    appendPipeline->reportAppsrcNeedDataReceived();
}

static void appendPipelineDemuxerPadAdded(GstElement*, GstPad* demuxerSrcPad, AppendPipeline* appendPipeline)
{
    appendPipeline->connectDemuxerSrcPadToAppsinkFromAnyThread(demuxerSrcPad);
}

static void appendPipelineDemuxerPadRemoved(GstElement*, GstPad*, AppendPipeline* appendPipeline)
{
    appendPipeline->disconnectDemuxerSrcPadFromAppsinkFromAnyThread();
}

static GstFlowReturn appendPipelineAppsinkNewSample(GstElement* appsink, AppendPipeline* appendPipeline)
{
    return appendPipeline->handleNewAppsinkSample(appsink);
}

static void appendPipelineAppsinkEOS(GstElement*, AppendPipeline* appendPipeline)
{
    if (WTF::isMainThread())
        appendPipeline->appsinkEOS();
    else {
        appendPipeline->ref();
        GstStructure* structure = gst_structure_new_empty("appsink-eos");
        GstMessage* message = gst_message_new_application(GST_OBJECT(appendPipeline->appsink()), structure);
        gst_bus_post(appendPipeline->bus(), message);
        GST_TRACE("appsink-eos message posted to bus");
    }

    GST_DEBUG("%s main thread", (WTF::isMainThread()) ? "Is" : "Not");
}

PassRefPtr<MediaSourceClientGStreamerMSE> MediaSourceClientGStreamerMSE::create(MediaPlayerPrivateGStreamerMSE* playerPrivate)
{
    ASSERT(WTF::isMainThread());

    // No return adoptRef(new MediaSourceClientGStreamerMSE(playerPrivate)) because the ownership has already been transferred to MediaPlayerPrivateGStreamerMSE.
    RefPtr<MediaSourceClientGStreamerMSE> client(adoptRef(new MediaSourceClientGStreamerMSE(playerPrivate)));
    playerPrivate->setMediaSourceClient(client);
    return client;
}

MediaSourceClientGStreamerMSE::MediaSourceClientGStreamerMSE(MediaPlayerPrivateGStreamerMSE* playerPrivate)
    : m_duration(MediaTime::invalidTime())
{
    ASSERT(WTF::isMainThread());

    m_playerPrivate = playerPrivate;
}

MediaSourceClientGStreamerMSE::~MediaSourceClientGStreamerMSE()
{
    ASSERT(WTF::isMainThread());
}

MediaSourcePrivate::AddStatus MediaSourceClientGStreamerMSE::addSourceBuffer(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, const ContentType&)
{
    ASSERT(WTF::isMainThread());

    if (!m_playerPrivate)
        return MediaSourcePrivate::AddStatus::NotSupported;

    RefPtr<AppendPipeline> appendPipeline = adoptRef(new AppendPipeline(this, sourceBufferPrivate, m_playerPrivate));
    GST_DEBUG("this=%p sourceBuffer=%p appendPipeline=%p", this, sourceBufferPrivate.get(), appendPipeline.get());
    m_playerPrivate->m_appendPipelinesMap.add(sourceBufferPrivate, appendPipeline);

    ASSERT(m_playerPrivate->m_playbackPipeline);

    return m_playerPrivate->m_playbackPipeline->addSourceBuffer(sourceBufferPrivate);
}

MediaTime MediaPlayerPrivateGStreamerMSE::currentMediaTime() const
{
    auto position = MediaPlayerPrivateGStreamer::currentMediaTime();

    if (m_eosPending && (paused() || (position >= durationMediaTime()))) {
        if (m_networkState != MediaPlayer::Loaded) {
            m_networkState = MediaPlayer::Loaded;
            m_player->networkStateChanged();
        }

        m_eosPending = false;
        m_isEndReached = true;
        m_cachedPosition = m_mediaTimeDuration.toFloat();
        m_durationAtEOS = m_mediaTimeDuration.toFloat();
        m_player->timeChanged();
    }
    return position;
}

MediaTime MediaSourceClientGStreamerMSE::duration()
{
    ASSERT(WTF::isMainThread());

    return m_duration;
}

void MediaSourceClientGStreamerMSE::durationChanged(const MediaTime& duration)
{
    ASSERT(WTF::isMainThread());

    GST_TRACE("duration: %f", duration.toFloat());
    if (!duration.isValid() || duration.isPositiveInfinite() || duration.isNegativeInfinite())
        return;

    m_duration = duration;
    if (m_playerPrivate)
        m_playerPrivate->durationChanged();
}

void MediaSourceClientGStreamerMSE::abort(PassRefPtr<SourceBufferPrivateGStreamer> prpSourceBufferPrivate)
{
    ASSERT(WTF::isMainThread());

    GST_DEBUG("aborting");

    if (!m_playerPrivate)
        return;

    RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate = prpSourceBufferPrivate;
    RefPtr<AppendPipeline> appendPipeline = m_playerPrivate->m_appendPipelinesMap.get(sourceBufferPrivate);
    appendPipeline->abort();
}

bool MediaSourceClientGStreamerMSE::append(PassRefPtr<SourceBufferPrivateGStreamer> prpSourceBufferPrivate, const unsigned char* data, unsigned length)
{
    ASSERT(WTF::isMainThread());

    GST_DEBUG("Appending %u bytes", length);

    if (!m_playerPrivate)
        return false;

    RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate = prpSourceBufferPrivate;
    RefPtr<AppendPipeline> appendPipeline = m_playerPrivate->m_appendPipelinesMap.get(sourceBufferPrivate);
    GstBuffer* buffer = gst_buffer_new_and_alloc(length);
    gst_buffer_fill(buffer, 0, data, length);

    return GST_FLOW_OK == appendPipeline->pushNewBuffer(buffer);
}

void MediaSourceClientGStreamerMSE::markEndOfStream(MediaSourcePrivate::EndOfStreamStatus status)
{
    ASSERT(WTF::isMainThread());

    m_playerPrivate->markEndOfStream(status);
}

void MediaSourceClientGStreamerMSE::removedFromMediaSource(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate)
{
    ASSERT(WTF::isMainThread());

    if (!m_playerPrivate)
        return;

    RefPtr<AppendPipeline> appendPipeline = m_playerPrivate->m_appendPipelinesMap.get(sourceBufferPrivate);
    appendPipeline->clearPlayerPrivate();
    m_playerPrivate->m_appendPipelinesMap.remove(sourceBufferPrivate);
    // AppendPipeline destructor will take care of cleaning up when appropriate.

    ASSERT(m_playerPrivate->m_playbackPipeline);

    m_playerPrivate->m_playbackPipeline->removeSourceBuffer(sourceBufferPrivate);
}

void MediaSourceClientGStreamerMSE::flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample>> samples)
{
    ASSERT(WTF::isMainThread());

    if (!m_playerPrivate)
        return;

    m_playerPrivate->m_playbackPipeline->flushAndEnqueueNonDisplayingSamples(samples);
}

void MediaSourceClientGStreamerMSE::enqueueSample(PassRefPtr<MediaSample> prpSample)
{
    ASSERT(WTF::isMainThread());

    if (!m_playerPrivate)
        return;

    m_playerPrivate->m_playbackPipeline->enqueueSample(prpSample);
}

void MediaSourceClientGStreamerMSE::didReceiveInitializationSegment(SourceBufferPrivateGStreamer* sourceBuffer, const SourceBufferPrivateClient::InitializationSegment& initializationSegment)
{
    ASSERT(WTF::isMainThread());

    if (m_playerPrivate)
        m_playerPrivate->setLoadingProgressed(true);

    sourceBuffer->didReceiveInitializationSegment(initializationSegment);
}

void MediaSourceClientGStreamerMSE::didReceiveAllPendingSamples(SourceBufferPrivateGStreamer* sourceBuffer)
{
    ASSERT(WTF::isMainThread());

    GST_DEBUG("received all pending samples");

    if (m_playerPrivate)
        m_playerPrivate->setLoadingProgressed(true);

    sourceBuffer->didReceiveAllPendingSamples();
}

GRefPtr<WebKitMediaSrc> MediaSourceClientGStreamerMSE::webKitMediaSrc()
{
    ASSERT(WTF::isMainThread());

    if (!m_playerPrivate)
        return GRefPtr<WebKitMediaSrc>(nullptr);

    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(m_playerPrivate->m_source.get());

    ASSERT(WEBKIT_IS_MEDIA_SRC(source));

    return GRefPtr<WebKitMediaSrc>(source);
}

void MediaSourceClientGStreamerMSE::clearPlayerPrivate()
{
    ASSERT(WTF::isMainThread());

    m_playerPrivate = nullptr;
}

float MediaPlayerPrivateGStreamerMSE::maxTimeSeekable() const
{
    if (m_errorOccured)
        return 0;

    GST_DEBUG("maxTimeSeekable");
    float result = durationMediaTime().toDouble();
    // Infinite duration means live stream.
    if (isinf(result)) {
        MediaTime maxBufferedTime = buffered()->maximumBufferedTime();
        // Return the highest end time reported by the buffered attribute.
        result = maxBufferedTime.isValid() ? maxBufferedTime.toFloat() : 0;
    }

    return result;
}

bool MediaPlayerPrivateGStreamerMSE::didLoadingProgress() const
{
    return loadingProgressed();
}

} // namespace WebCore.

#endif // USE(GSTREAMER)
