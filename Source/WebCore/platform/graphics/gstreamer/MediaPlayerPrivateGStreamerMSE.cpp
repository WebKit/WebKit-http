/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Igalia S.L
 * Copyright (C) 2014 Cable Television Laboratories, Inc.
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
#include <wtf/glib/GMutexLocker.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>


#if USE(DXDRM)
#include "DiscretixSession.h"
#elif USE(PLAYREADY)
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

GST_DEBUG_CATEGORY_EXTERN(webkit_media_player_debug);
#define GST_CAT_DEFAULT webkit_media_player_debug

using namespace std;

namespace WebCore {

struct PadProbeInformation
{
    AppendPipeline* m_appendPipeline;
    const char* m_description;
    gulong m_probeId;
};

class AppendPipeline : public ThreadSafeRefCounted<AppendPipeline> {
public:
    enum AppendStage { Invalid, NotStarted, Ongoing, KeyNegotiation, DataStarve, Sampling, LastSample, Aborting };

    AppendPipeline(PassRefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient, PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, MediaPlayerPrivateGStreamerMSE* playerPrivate);
    virtual ~AppendPipeline();

    void handleElementMessage(GstMessage*);
    void handleApplicationMessage(GstMessage*);

    gint id();
    AppendStage appendStage() { return m_appendStage; }
    void setAppendStage(AppendStage newAppendStage);

    GstFlowReturn handleNewSample(GstElement* appsink);
    GstFlowReturn pushNewBuffer(GstBuffer* buffer);

    // Takes ownership of caps.
    void parseDemuxerCaps(GstCaps* demuxerSrcPadCaps);
    void appSinkCapsChanged();
    void appSinkNewSample(GstSample* sample);
    void appSinkEOS();
    void didReceiveInitializationSegment();
    AtomicString trackId();
    void abort();

    void clearPlayerPrivate();
    RefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient() { return m_mediaSourceClient; }
    RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate() { return m_sourceBufferPrivate; }
    GstElement* pipeline() { return m_pipeline; }
    GstElement* appsrc() { return m_appsrc; }
    GstCaps* demuxerSrcPadCaps() { return m_demuxerSrcPadCaps; }
    GstCaps* appSinkCaps() { return m_appSinkCaps; }
    RefPtr<WebCore::TrackPrivateBase> track() { return m_track; }
    WebCore::MediaSourceStreamTypeGStreamer streamType() { return m_streamType; }

    void disconnectFromAppSinkFromAnyThread();
    void disconnectFromAppSink();
    void connectToAppSinkFromAnyThread(GstPad* demuxersrcpad);
    void connectToAppSink(GstPad* demuxersrcpad);

    void reportAppsrcAtLeastABufferLeft();
    void reportAppsrcNeedDataReceived();

private:
    void resetPipeline();
    void checkEndOfAppend();
    void handleAppsrcAtLeastABufferLeft();
    void handleAppsrcNeedDataReceived();
    void removeAppsrcDataLeavingProbe();
    void setAppsrcDataLeavingProbe();

// TODO: Hide everything and use getters/setters.
private:
    RefPtr<MediaSourceClientGStreamerMSE> m_mediaSourceClient;
    RefPtr<SourceBufferPrivateGStreamer> m_sourceBufferPrivate;
    MediaPlayerPrivateGStreamerMSE* m_playerPrivate;

    // (m_mediaType, m_id) is unique.
    gint m_id;

    MediaTime m_initialDuration;

    GstFlowReturn m_flowReturn;

    GstElement* m_pipeline;
    GRefPtr<GstBus> m_bus;
    GstElement* m_appsrc;
    GstElement* m_qtdemux;

    GstElement* m_decryptor;

    // The demuxer has one src Stream only.
    GstElement* m_appsink;

    GMutex m_newSampleMutex;
    GCond m_newSampleCondition;
    GMutex m_padAddRemoveMutex;
    GCond m_padAddRemoveCondition;

    GstCaps* m_appSinkCaps;
    GstCaps* m_demuxerSrcPadCaps;
    FloatSize m_presentationSize;

    bool m_appsrcAtLeastABufferLeft;
    bool m_appsrcNeedDataReceived;

    gulong m_appsrcDataLeavingProbeId;
#ifdef DEBUG_APPEND_PIPELINE_PADS
    struct PadProbeInformation m_demuxerDataEnteringPadProbeInformation;
    struct PadProbeInformation m_appsinkDataEnteringPadProbeInformation;
#endif

    // Keeps track of the stages of append processing, to avoid
    // performing actions inappropriate for the current stage (eg:
    // processing more samples when the last one has been detected,
    // etc.).  See setAppendStage() for valid transitions.
    AppendStage m_appendStage;

    // Aborts can only be completed when the normal sample detection
    // has finished. Meanwhile, the willing to abort is expressed in
    // this field.
    bool m_abortPending;

    WebCore::MediaSourceStreamTypeGStreamer m_streamType;
    RefPtr<WebCore::TrackPrivateBase> m_oldTrack;
    RefPtr<WebCore::TrackPrivateBase> m_track;

    GRefPtr<GstBuffer> m_pendingBuffer;
};

void MediaPlayerPrivateGStreamerMSE::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
         registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivateGStreamerMSE>(player); },
            getSupportedTypes, supportsType, 0, 0, 0, supportsKeySystem);
}

bool initializeGStreamerAndRegisterWebKitMESElement()
{
    if (!initializeGStreamer())
        return false;

    registerWebKitGStreamerElements();

    GRefPtr<GstElementFactory> WebKitMediaSrcFactory = gst_element_factory_find("webkitmediasrc");
    if (!WebKitMediaSrcFactory)
        gst_element_register(0, "webkitmediasrc", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_SRC);
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
    , m_mseSeekCompleted(true)
    , m_gstSeekCompleted(true)
{
    m_eosPending = false;
    LOG_MEDIA_MESSAGE("%p", this);
}

MediaPlayerPrivateGStreamerMSE::~MediaPlayerPrivateGStreamerMSE()
{
    LOG_MEDIA_MESSAGE("destroying the player");

    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it)
        it->value->clearPlayerPrivate();

    clearSamples();

    if (m_source) {
        webkit_media_src_set_mediaplayerprivate(WEBKIT_MEDIA_SRC(m_source.get()), 0);
        g_signal_handlers_disconnect_matched(m_source.get(), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
    }

    if (m_playbackPipeline)
        m_playbackPipeline->setWebKitMediaSrc(nullptr);

}

void MediaPlayerPrivateGStreamerMSE::load(const String& urlString)
{
    if (!initializeGStreamerAndRegisterWebKitMESElement())
        return;

    if (!urlString.startsWith("mediasource")) {
        LOG_MEDIA_MESSAGE("Unsupported url: %s", urlString.utf8().data());
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

float MediaPlayerPrivateGStreamerMSE::duration() const
{
    if (!m_pipeline)
        return 0.0f;

    if (m_errorOccured)
        return 0.0f;

    return m_mediaTimeDuration.toFloat();
}

float MediaPlayerPrivateGStreamerMSE::mediaTimeForTimeValue(float timeValue) const
{
    GstClockTime position = toGstClockTime(timeValue);
    return MediaTime(position, GST_SECOND).toFloat();
}

void MediaPlayerPrivateGStreamerMSE::seek(float time)
{
    if (!m_pipeline)
        return;

    if (m_errorOccured)
        return;

    INFO_MEDIA_MESSAGE("[Seek] seek attempt to %f secs", time);

    // Avoid useless seeking.
    float current = currentTime();
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

    LOG_MEDIA_MESSAGE("Seeking from %f to %f seconds", current, time);

    float prevSeekTime = m_seekTime;
    m_seekTime = time;

    if (!doSeek()) {
        m_seekTime = prevSeekTime;
        LOG_MEDIA_MESSAGE("Seeking to %f failed", time);
        return;
    }

    m_isEndReached = false;
    LOG_MEDIA_MESSAGE("m_seeking=%s, m_seekTime=%f", m_seeking?"true":"false", m_seekTime);
}

bool MediaPlayerPrivateGStreamerMSE::changePipelineState(GstState newState)
{
    if (seeking()) {
        LOG_MEDIA_MESSAGE("Rejected state change to %s while seeking",
            gst_element_state_get_name(newState));
        return true;
    }

    return MediaPlayerPrivateGStreamer::changePipelineState(newState);
}

void MediaPlayerPrivateGStreamerMSE::notifySeekNeedsData(const MediaTime& seekTime)
{
    // Reenqueue samples needed to resume playback in the new position
    m_mediaSource->seekToTime(seekTime);

    LOG_MEDIA_MESSAGE("MSE seek to %f finished", seekTime.toDouble());

    if (!m_gstSeekCompleted) {
        m_gstSeekCompleted = true;
        maybeFinishSeek();
    }
}

bool MediaPlayerPrivateGStreamerMSE::doSeek(gint64, float, GstSeekFlags)
{
    // Use doSeek() instead. If anybody is calling this version of doSeek(), something is wrong.
    notImplemented();
    ASSERT_NOT_REACHED();
    return false;
}

bool MediaPlayerPrivateGStreamerMSE::doSeek()
{
    GstClockTime position = toGstClockTime(m_seekTime);
    MediaTime seekTime = MediaTime::createWithDouble(m_seekTime + MediaTime::FuzzinessThreshold);
    double rate = m_player->rate();
    GstSeekFlags seekType = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

    // Always move to seeking state to report correct 'currentTime' while pending for actual seek to complete
    m_seeking = true;

    // Check if playback pipeline is ready for seek
    GstState state;
    GstState newState;
    GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, &newState, 0);
    if (getStateResult == GST_STATE_CHANGE_FAILURE || getStateResult == GST_STATE_CHANGE_NO_PREROLL) {
        LOG_MEDIA_MESSAGE("[Seek] cannot seek, current state change is %s",
                          gst_element_state_change_return_get_name(getStateResult));
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
        if (getStateResult == GST_STATE_CHANGE_ASYNC)
            reason = String::format("In async change %s --> %s",
                                    gst_element_state_get_name(state),
                                    gst_element_state_get_name(newState)).utf8();
        else if (state < GST_STATE_PAUSED)
            reason = "State less than PAUSED";
        else if (m_isEndReached)
            reason = "End reached";
        else if (!m_gstSeekCompleted)
            reason = "Previous seek is not finished yet";

        LOG_MEDIA_MESSAGE("[Seek] Delaying the seek: %s", reason.data());

        m_seekIsPending = true;

        if (m_isEndReached) {
            LOG_MEDIA_MESSAGE("[Seek] reset pipeline");
            m_resetPipeline = true;
            m_seeking = false;
            if (!changePipelineState(GST_STATE_PAUSED))
                loadingFailed(MediaPlayer::Empty);
            else
                m_seeking = true;
        }

        return m_seeking;
    }

    // Stop accepting new samples until actual seek is finished
    webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), false);

    // Correct seek time if it helps to fix a small gap
    if (!timeIsBuffered(seekTime)) {
        // Look if a near future time (<0.1 sec.) is buffered and change the seek target time
        if (m_mediaSource) {
            const MediaTime miniGap = MediaTime::createWithDouble(0.1);
            MediaTime nearest = m_mediaSource->buffered()->nearest(seekTime);
            if (nearest.isValid() && nearest > seekTime && (nearest - seekTime) <= miniGap && timeIsBuffered(nearest + miniGap)) {
                LOG_MEDIA_MESSAGE("[Seek] Changed the seek target time from %f to %f, a near point in the future", seekTime.toFloat(), nearest.toFloat());
                seekTime = nearest;
            }
        }
    }

    // Check if MSE has samples for requested time and defer actual seek if needed
    if (!timeIsBuffered(seekTime)) {
        LOG_MEDIA_MESSAGE("[Seek] Delaying the seek: MSE is not ready");
        GstStateChangeReturn setStateResult = gst_element_set_state(m_pipeline.get(), GST_STATE_PAUSED);
        if (setStateResult == GST_STATE_CHANGE_FAILURE) {
            LOG_MEDIA_MESSAGE("[Seek] Cannot seek, failed to pause playback pipeline.");
            webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), true);
            m_seeking = false;
            return false;
        }
        m_readyState = MediaPlayer::HaveMetadata;
        notifySeekNeedsData(seekTime);
        ASSERT(!m_mseSeekCompleted);
        return true;
    }

    // Complete previous MSE seek if needed
    if (!m_mseSeekCompleted) {
        m_mediaSource->monitorSourceBuffers();
        ASSERT(m_mseSeekCompleted);
        return m_seeking;  // Note seekCompleted will recursively call us
    }

    LOG_MEDIA_MESSAGE("We can seek now");

    gint64 startTime, endTime;

    if (rate > 0) {
        startTime = position;
        endTime = GST_CLOCK_TIME_NONE;
    } else {
        startTime = 0;
        endTime = position;
    }

    if (!rate)
        rate = 1.0;

    LOG_MEDIA_MESSAGE("Actual seek to %" GST_TIME_FORMAT ", end time:  %" GST_TIME_FORMAT ", rate: %f", GST_TIME_ARGS(startTime), GST_TIME_ARGS(endTime), rate);

    // This will call notifySeekNeedsData() after some time to tell that the pipeline is ready for sample enqueuing.
    webkit_media_src_prepare_seek(WEBKIT_MEDIA_SRC(m_source.get()), seekTime);

    m_gstSeekCompleted = false;
    if (!gst_element_seek(m_pipeline.get(), rate, GST_FORMAT_TIME, seekType,
        GST_SEEK_TYPE_SET, startTime, GST_SEEK_TYPE_SET, endTime)) {
        webkit_media_src_set_readyforsamples(WEBKIT_MEDIA_SRC(m_source.get()), true);
        m_seeking = false;
        m_gstSeekCompleted = true;
        LOG_MEDIA_MESSAGE("Returning false");
        return false;
    }

    // The samples will be enqueued in notifySeekNeedsData()
    LOG_MEDIA_MESSAGE("Returning true");
    return true;
}

void MediaPlayerPrivateGStreamerMSE::maybeFinishSeek()
{
    if (!m_seeking || !m_mseSeekCompleted || !m_gstSeekCompleted) {
        return;
    }

    GstState state;
    GstState newState;
    GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, &newState, 0);

    if (getStateResult == GST_STATE_CHANGE_ASYNC
            && !(state == GST_STATE_PLAYING && newState == GST_STATE_PAUSED)) {
        LOG_MEDIA_MESSAGE("[Seek] Delaying seek finish");
        return;
    }

    if (m_seekIsPending) {
        LOG_MEDIA_MESSAGE("[Seek] Committing pending seek to %f", m_seekTime);
        m_seekIsPending = false;
        if (!doSeek()) {
            LOG_MEDIA_MESSAGE("[Seek] Seeking to %f failed", m_seekTime);
            m_cachedPosition = -1;
        }
        return;
    }

    LOG_MEDIA_MESSAGE("[Seek] Seeked to %f", m_seekTime);

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

// METRO FIXME: GStreamer mediaplayer manages the readystate on its own. We shouldn't change it manually.
void MediaPlayerPrivateGStreamerMSE::setReadyState(MediaPlayer::ReadyState state)
{
    // FIXME: early return here.
    if (state != m_readyState) {
        if (seeking()) {
            LOG_MEDIA_MESSAGE("Skip ready state change(%s -> %s) due to seek\n", dumpReadyState(m_readyState), dumpReadyState(state));
            return;
        }

        LOG_MEDIA_MESSAGE("Ready State Changed manually from %u to %u", m_readyState, state);
        MediaPlayer::ReadyState oldReadyState = m_readyState;
        m_readyState = state;
        LOG_MEDIA_MESSAGE("m_readyState: %s -> %s", dumpReadyState(oldReadyState), dumpReadyState(m_readyState));

        GstState state;
        GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, NULL, 250 * GST_NSECOND);
        bool isPlaying = (getStateResult == GST_STATE_CHANGE_SUCCESS && state == GST_STATE_PLAYING);

        if (m_readyState == MediaPlayer::HaveMetadata && oldReadyState > MediaPlayer::HaveMetadata && isPlaying) {
            LOG_MEDIA_MESSAGE("Changing pipeline to PAUSED...");
            bool ok = changePipelineState(GST_STATE_PAUSED);
            LOG_MEDIA_MESSAGE("Changing pipeline to PAUSED: %s", (ok)?"OK":"ERROR");
        }
        if (oldReadyState < MediaPlayer::HaveCurrentData && m_readyState >= MediaPlayer::HaveCurrentData) {
            LOG_MEDIA_MESSAGE("[Seek] Reporting load state changed to trigger seek continuation");
            loadStateChanged();
        }
        m_player->readyStateChanged();
    }
}

void MediaPlayerPrivateGStreamerMSE::waitForSeekCompleted()
{
    if (!m_seeking)
        return;

    LOG_MEDIA_MESSAGE("Waiting for MSE seek completed");
    m_mseSeekCompleted = false;
}

void MediaPlayerPrivateGStreamerMSE::seekCompleted()
{
    if (m_mseSeekCompleted)
        return;

    LOG_MEDIA_MESSAGE("MSE seek completed");
    m_mseSeekCompleted = true;

    doSeek();

    if (!seeking() && m_readyState >= MediaPlayer::HaveFutureData)
        changePipelineState(GST_STATE_PLAYING);

    if (!seeking())
        m_player->timeChanged();
}

void MediaPlayerPrivateGStreamerMSE::setRate(float rate)
{
    UNUSED_PARAM(rate);
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
    if (!m_pipeline)
        return;

    if (m_errorOccured)
        return;

    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    GstState state;
    GstState pending;

    GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, &pending, 250 * GST_NSECOND);

    bool shouldUpdatePlaybackState = false;
    switch (getStateResult) {
    case GST_STATE_CHANGE_SUCCESS: {
        LOG_MEDIA_MESSAGE("State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));

        // Do nothing if on EOS and state changed to READY to avoid recreating the player
        // on HTMLMediaElement and properly generate the video 'ended' event.
        if (m_isEndReached && state == GST_STATE_READY)
            break;

        if (state <= GST_STATE_READY) {
            m_resetPipeline = true;
            m_mediaTimeDuration = MediaTime::zeroTime();
        } else {
            m_resetPipeline = false;
            cacheDuration();
        }

        // Update ready and network states.
        switch (state) {
        case GST_STATE_NULL:
            m_readyState = MediaPlayer::HaveNothing;
            LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
            m_networkState = MediaPlayer::Empty;
            break;
        case GST_STATE_READY:
            m_readyState = MediaPlayer::HaveMetadata;
            LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
            m_networkState = MediaPlayer::Empty;
            break;
        case GST_STATE_PAUSED:
        case GST_STATE_PLAYING:
            if (seeking()) {
                m_readyState = MediaPlayer::HaveMetadata;
                // TODO: NetworkState?
                LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
            } else if (m_buffering) {
                if (m_bufferingPercentage == 100) {
                    LOG_MEDIA_MESSAGE("[Buffering] Complete.");
                    m_buffering = false;
                    m_readyState = MediaPlayer::HaveEnoughData;
                    LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
                    m_networkState = m_downloadFinished ? MediaPlayer::Idle : MediaPlayer::Loading;
                } else {
                    m_readyState = MediaPlayer::HaveCurrentData;
                    LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
                    m_networkState = MediaPlayer::Loading;
                }
            } else if (m_downloadFinished) {
                m_readyState = MediaPlayer::HaveEnoughData;
                LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
                m_networkState = MediaPlayer::Loaded;
            } else {
                m_readyState = MediaPlayer::HaveFutureData;
                LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
                m_networkState = MediaPlayer::Loading;
            }

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
                LOG_MEDIA_MESSAGE("[Buffering] Restarting playback.");
                changePipelineState(GST_STATE_PLAYING);
            }
        } else if (state == GST_STATE_PLAYING) {
            m_paused = false;

            if ((m_buffering && !isLiveStream()) || !m_playbackRate) {
                LOG_MEDIA_MESSAGE("[Buffering] Pausing stream for buffering.");
                changePipelineState(GST_STATE_PAUSED);
            }
        } else
            m_paused = true;

        if (m_requestedState == GST_STATE_PAUSED && state == GST_STATE_PAUSED) {
            shouldUpdatePlaybackState = true;
            LOG_MEDIA_MESSAGE("Requested state change to %s was completed", gst_element_state_get_name(state));
        }

        break;
    }
    case GST_STATE_CHANGE_ASYNC:
        LOG_MEDIA_MESSAGE("Async: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));
        // Change in progress.
        break;
    case GST_STATE_CHANGE_FAILURE:
        LOG_MEDIA_MESSAGE("Failure: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));
        // Change failed
        return;
    case GST_STATE_CHANGE_NO_PREROLL:
        LOG_MEDIA_MESSAGE("No preroll: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));

        // Live pipelines go in PAUSED without prerolling.
        m_isStreaming = true;

        if (state == GST_STATE_READY) {
            m_readyState = MediaPlayer::HaveNothing;
            LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
        } else if (state == GST_STATE_PAUSED) {
            m_readyState = MediaPlayer::HaveEnoughData;
            LOG_MEDIA_MESSAGE("m_readyState=%s", dumpReadyState(m_readyState));
            m_paused = true;
        } else if (state == GST_STATE_PLAYING)
            m_paused = false;

        if (!m_paused && m_playbackRate)
            changePipelineState(GST_STATE_PLAYING);

        m_networkState = MediaPlayer::Loading;
        break;
    default:
        LOG_MEDIA_MESSAGE("Else : %d", getStateResult);
        break;
    }

    m_requestedState = GST_STATE_VOID_PENDING;

    if (shouldUpdatePlaybackState)
        m_player->playbackStateChanged();

    if (m_networkState != oldNetworkState) {
        LOG_MEDIA_MESSAGE("Network State Changed from %u to %u", oldNetworkState, m_networkState);
        m_player->networkStateChanged();
    }
    if (m_readyState != oldReadyState) {
        LOG_MEDIA_MESSAGE("Ready State Changed from %u to %u", oldReadyState, m_readyState);
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

bool MediaPlayerPrivateGStreamerMSE::timeIsBuffered(const MediaTime &time) const
{
    bool result = m_mediaSource && m_mediaSource->buffered()->contain(time);
    LOG_MEDIA_MESSAGE("Time %f buffered? %s", time.toDouble(), result ? "aye" : "nope");
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

RefPtr<AppendPipeline> MediaPlayerPrivateGStreamerMSE::appendPipelineByTrackId(const AtomicString& trackId)
{
    if (trackId == AtomicString()) {
        LOG_MEDIA_MESSAGE("trackId is empty");
    }

    ASSERT(!(trackId.isNull() || trackId.isEmpty()));

    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it)
        if (it->value->trackId() == trackId)
            return it->value;

    return RefPtr<AppendPipeline>(0);
}

void MediaPlayerPrivateGStreamerMSE::durationChanged()
{
    MediaTime previousDuration = m_mediaTimeDuration;

    if (!m_mediaSourceClient) {
        LOG_MEDIA_MESSAGE("m_mediaSourceClient is null, not doing anything");
        return;
    }
    m_mediaTimeDuration = m_mediaSourceClient->duration();

    TRACE_MEDIA_MESSAGE("previous=%f, new=%f", previousDuration.toFloat(), m_mediaTimeDuration.toFloat());

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
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cache = []() {
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

void MediaPlayerPrivateGStreamerMSE::trackDetected(RefPtr<AppendPipeline> ap, RefPtr<WebCore::TrackPrivateBase> oldTrack, RefPtr<WebCore::TrackPrivateBase> newTrack)
{
    ASSERT(ap->track() == newTrack);

    GstCaps* caps = ap->appSinkCaps();
    ASSERT(caps);
    LOG_MEDIA_MESSAGE("track ID: %s, caps: %" GST_PTR_FORMAT, newTrack->id().string().latin1().data(), caps);

    GstStructure* s = gst_caps_get_structure(caps, 0);
    const gchar* mediaType = gst_structure_get_name(s);
    GstVideoInfo info;

    if (g_str_has_prefix(mediaType, "video/") && gst_video_info_from_caps(&info, caps)) {
        float width, height;

        width = info.width;
        height = info.height * ((float) info.par_d / (float) info.par_n);
        m_videoSize.setWidth(width);
        m_videoSize.setHeight(height);
    }

    if (!oldTrack)
        m_playbackPipeline->attachTrack(ap->sourceBufferPrivate(), newTrack, s, caps);
    else
        m_playbackPipeline->reattachTrack(ap->sourceBufferPrivate(), newTrack);
}

MediaPlayer::SupportsType MediaPlayerPrivateGStreamerMSE::supportsType(const MediaEngineSupportParameters& parameters)
{
    MediaPlayer::SupportsType result = MediaPlayer::IsNotSupported;
    if (!parameters.isMediaSource)
        return result;

    // Disable VPX/Opus on MSE for now, mp4/avc1 seems way more reliable currently.
    if (parameters.type.endsWith("webm"))
        return result;

    // Youtube TV provides empty types for some videos and we want to be selected as best media engine for them.
    if (parameters.type.isNull() || parameters.type.isEmpty()) {
        result = MediaPlayer::MayBeSupported;
        return result;
    }

    // spec says we should not return "probably" if the codecs string is empty
    if (mimeTypeCache().contains(parameters.type))
        result = parameters.codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;

    return extendedSupportsType(parameters, result);
}

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
void MediaPlayerPrivateGStreamerMSE::dispatchDecryptionKey(GstBuffer* buffer)
{
    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it) {
        if (it->value->appendStage() == AppendPipeline::AppendStage::KeyNegotiation) {
            TRACE_MEDIA_MESSAGE("append pipeline %p in key negotiation, setting key", it->value.get());
            gst_element_send_event(it->value->pipeline(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
                gst_structure_new("drm-cipher", "key", GST_TYPE_BUFFER, buffer, nullptr)));
            it->value->setAppendStage(AppendPipeline::AppendStage::Ongoing);
        } else
            TRACE_MEDIA_MESSAGE("append pipeline %p not in key negotiation", it->value.get());
    }
}
#endif

#if USE(DXDRM) || USE(PLAYREADY)
void MediaPlayerPrivateGStreamerMSE::emitSession()
{
#if USE(DXDRM)
    DiscretixSession* session = dxdrmSession();
    const char* label = "dxdrm-session";
#elif USE(PLAYREADY)
    PlayreadySession* session = prSession();
    const char* label = "playready-session";
#endif
    if (!session->ready())
        return;

    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it) {
        gst_element_send_event(it->value->pipeline(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
           gst_structure_new(label, "session", G_TYPE_POINTER, session, nullptr)));
        it->value->setAppendStage(AppendPipeline::AppendStage::Ongoing);
    }
}
#endif

void MediaPlayerPrivateGStreamerMSE::markEndOfStream(MediaSourcePrivate::EndOfStreamStatus status)
{
    if (status != MediaSourcePrivate::EosNoError)
        return;

    LOG_MEDIA_MESSAGE("Marking end of stream");
    m_eosPending = true;
}

class GStreamerMediaDescription : public MediaDescription {
private:
    GstCaps* m_caps;
public:
    static PassRefPtr<GStreamerMediaDescription> create(GstCaps* caps)
    {
        return adoptRef(new GStreamerMediaDescription(caps));
    }

    virtual ~GStreamerMediaDescription()
    {
        gst_caps_unref(m_caps);
    }

    AtomicString codec() const override
    {
        gchar* description = gst_pb_utils_get_codec_description(m_caps);
        String codecName(description);

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
        g_free(description);

        return simpleCodecName;
    }

    bool isVideo() const override
    {
        GstStructure* s = gst_caps_get_structure(m_caps, 0);
        const gchar* name = gst_structure_get_name(s);

#if GST_CHECK_VERSION(1, 5, 3)
        if (!g_strcmp0(name, "application/x-cenc"))
            return g_str_has_prefix(gst_structure_get_string(s, "original-media-type"), "video/");
#endif
        return g_str_has_prefix(name, "video/");
    }

    bool isAudio() const override
    {
        GstStructure* s = gst_caps_get_structure(m_caps, 0);
        const gchar* name = gst_structure_get_name(s);

#if GST_CHECK_VERSION(1, 5, 3)
        if (!g_strcmp0(name, "application/x-cenc"))
            return g_str_has_prefix(gst_structure_get_string(s, "original-media-type"), "audio/");
#endif
        return g_str_has_prefix(name, "audio/");
    }

    bool isText() const override
    {
        // TODO
        return false;
    }

private:
    GStreamerMediaDescription(GstCaps* caps)
        : MediaDescription()
        , m_caps(gst_caps_ref(caps))
    {
    }
};

// class GStreamerMediaSample : public MediaSample
GStreamerMediaSample::GStreamerMediaSample(GstSample* sample, const FloatSize& presentationSize, const AtomicString& trackID)
    : MediaSample()
    , m_pts(MediaTime::zeroTime())
    , m_dts(MediaTime::zeroTime())
    , m_duration(MediaTime::zeroTime())
    , m_trackID(trackID)
    , m_size(0)
    , m_sample(0)
    , m_presentationSize(presentationSize)
    , m_flags(MediaSample::IsSync)
{

    if (!sample)
        return;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer)
        return;

    if (GST_BUFFER_PTS_IS_VALID(buffer))
        m_pts = MediaTime(GST_BUFFER_PTS(buffer), GST_SECOND);
    if (GST_BUFFER_DTS_IS_VALID(buffer))
        m_dts = MediaTime(GST_BUFFER_DTS(buffer), GST_SECOND);
    if (GST_BUFFER_DURATION_IS_VALID(buffer))
        m_duration = MediaTime(GST_BUFFER_DURATION(buffer), GST_SECOND);
    m_size = gst_buffer_get_size(buffer);
    m_sample = gst_sample_ref(sample);

    if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))
        m_flags = MediaSample::None;

    if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY))
        m_flags = (MediaSample::SampleFlags) (m_flags | MediaSample::NonDisplaying);
}

PassRefPtr<GStreamerMediaSample> GStreamerMediaSample::create(GstSample* sample, const FloatSize& presentationSize, const AtomicString& trackID)
{
    return adoptRef(new GStreamerMediaSample(sample, presentationSize, trackID));
}

PassRefPtr<GStreamerMediaSample> GStreamerMediaSample::createFakeSample(GstCaps* caps, MediaTime pts, MediaTime dts, MediaTime duration, const FloatSize& presentationSize, const AtomicString& trackID)
{
    UNUSED_PARAM(caps);
    GstSample* sample = NULL;
    GStreamerMediaSample* s = new GStreamerMediaSample(sample, presentationSize, trackID);
    s->m_pts = pts;
    s->m_dts = dts;
    s->m_duration = duration;
    s->m_flags = MediaSample::NonDisplaying;
    return adoptRef(s);
}

void GStreamerMediaSample::applyPtsOffset(MediaTime timestampOffset)
{
    if (m_pts > timestampOffset) {
        m_duration = m_duration + (m_pts - timestampOffset);
        m_pts = timestampOffset;
    }
}

void GStreamerMediaSample::offsetTimestampsBy(const MediaTime& timestampOffset) {
    if (!timestampOffset)
        return;
    m_pts += timestampOffset;
    m_dts += timestampOffset;
    GstBuffer* buffer = gst_sample_get_buffer(m_sample);
    if (buffer) {
        GST_BUFFER_PTS(buffer) = toGstClockTime(m_pts.toFloat());
        GST_BUFFER_DTS(buffer) = toGstClockTime(m_dts.toFloat());
    }
}

GStreamerMediaSample::~GStreamerMediaSample()
{
    if (m_sample)
        gst_sample_unref(m_sample);
}

// Auxiliar to pass several parameters to appendPipelineAppSinkNewSampleMainThread().
class NewSampleInfo
{
public:
    NewSampleInfo(GstSample* sample, AppendPipeline* appendPipeline)
    {
        m_sample = gst_sample_ref(sample);
        m_ap = appendPipeline;
    }
    virtual ~NewSampleInfo()
    {
        gst_sample_unref(m_sample);
    }

    GstSample* sample() { return m_sample; }
    RefPtr<AppendPipeline> ap() { return m_ap; }

private:
    GstSample* m_sample;
    RefPtr<AppendPipeline> m_ap;
};

// Auxiliar to pass several parameters to appendPipelineAppSinkDemuxerPadAddedMainThread().
class PadInfo
{
public:
    PadInfo(GstPad* demuxerSrcPad, AppendPipeline* appendPipeline)
    {
        m_demuxerSrcPad = GST_PAD(gst_object_ref(demuxerSrcPad));
        m_ap = appendPipeline;
    }
    virtual ~PadInfo()
    {
        gst_object_unref(m_demuxerSrcPad);
    }

    GstPad* demuxerSrcPad() { return m_demuxerSrcPad; }
    RefPtr<AppendPipeline> ap() { return m_ap; }

private:
    GstPad* m_demuxerSrcPad;
    RefPtr<AppendPipeline> m_ap;
};

static const char* dumpAppendStage(AppendPipeline::AppendStage appendStage)
{
    switch (appendStage) {
    case AppendPipeline::AppendStage::Invalid:
        return "Invalid";
    case AppendPipeline::AppendStage::NotStarted:
        return "NotStarted";
    case AppendPipeline::AppendStage::Ongoing:
        return "Ongoing";
    case AppendPipeline::AppendStage::KeyNegotiation:
        return "KeyNegotiation";
    case AppendPipeline::AppendStage::DataStarve:
        return "DataStarve";
    case AppendPipeline::AppendStage::Sampling:
        return "Sampling";
    case AppendPipeline::AppendStage::LastSample:
        return "LastSample";
    case AppendPipeline::AppendStage::Aborting:
        return "Aborting";
    default:
        return "?";
    }
}

static void appendPipelineAppsrcNeedData(GstAppSrc*, guint, AppendPipeline*);
static void appendPipelineDemuxerPadAdded(GstElement*, GstPad*, AppendPipeline*);
static void appendPipelineDemuxerPadRemoved(GstElement*, GstPad*, AppendPipeline*);
static gboolean appendPipelineDemuxerConnectToAppSinkMainThread(PadInfo*);
static gboolean appendPipelineDemuxerDisconnectFromAppSinkMainThread(PadInfo*);
static void appendPipelineAppSinkCapsChanged(GObject*, GParamSpec*, AppendPipeline*);
static GstPadProbeReturn appendPipelineAppsrcDataLeaving(GstPad*, GstPadProbeInfo*, AppendPipeline*);
#ifdef DEBUG_APPEND_PIPELINE_PADS
static GstPadProbeReturn appendPipelinePadProbeDebugInformation(GstPad*, GstPadProbeInfo*, struct PadProbeInformation*);
#endif
static GstFlowReturn appendPipelineAppSinkNewSample(GstElement*, AppendPipeline*);
static gboolean appendPipelineAppSinkNewSampleMainThread(NewSampleInfo*);
static void appendPipelineAppSinkEOS(GstElement*, AppendPipeline*);
static gboolean appendPipelineAppSinkEOSMainThread(AppendPipeline* ap);

static void appendPipelineElementMessageCallback(GstBus*, GstMessage* message, AppendPipeline* ap)
{
    ap->handleElementMessage(message);
}

static void appendPipelineApplicationMessageCallback(GstBus*, GstMessage* message, AppendPipeline* appendPipeline)
{
    appendPipeline->handleApplicationMessage(message);
}

AppendPipeline::AppendPipeline(PassRefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient, PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, MediaPlayerPrivateGStreamerMSE* playerPrivate)
    : m_mediaSourceClient(mediaSourceClient)
    , m_sourceBufferPrivate(sourceBufferPrivate)
    , m_playerPrivate(playerPrivate)
    , m_id(0)
    , m_appSinkCaps(NULL)
    , m_demuxerSrcPadCaps(NULL)
    , m_appsrcAtLeastABufferLeft(false)
    , m_appsrcNeedDataReceived(false)
    , m_appsrcDataLeavingProbeId(0)
    , m_appendStage(NotStarted)
    , m_abortPending(false)
    , m_streamType(Unknown)
{
    ASSERT(WTF::isMainThread());

    LOG_MEDIA_MESSAGE("%p", this);

    // TODO: give a name to the pipeline, maybe related with the track it's managing.
    // The track name is still unknown at this time, though.
    m_pipeline = gst_pipeline_new(NULL);

    m_bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline)));
    gst_bus_add_signal_watch(m_bus.get());
    gst_bus_enable_sync_message_emission(m_bus.get());

    g_signal_connect(m_bus.get(), "sync-message::element", G_CALLBACK(appendPipelineElementMessageCallback), this);
    g_signal_connect(m_bus.get(), "message::application", G_CALLBACK(appendPipelineApplicationMessageCallback), this);

    g_mutex_init(&m_newSampleMutex);
    g_cond_init(&m_newSampleCondition);

    g_mutex_init(&m_padAddRemoveMutex);
    g_cond_init(&m_padAddRemoveCondition);

    m_decryptor = NULL;
    m_appsrc = gst_element_factory_make("appsrc", NULL);
    m_qtdemux = gst_element_factory_make("qtdemux", NULL);
    {
        GValue val = G_VALUE_INIT;
        g_value_init(&val, G_TYPE_BOOLEAN);
        g_value_set_boolean(&val, TRUE);
        g_object_set_property(G_OBJECT(m_qtdemux), "always-honor-tfdt", &val);
        g_value_unset(&val);
    }
    m_appsink = gst_element_factory_make("appsink", NULL);
    gst_app_sink_set_emit_signals(GST_APP_SINK(m_appsink), TRUE);
    gst_base_sink_set_sync(GST_BASE_SINK(m_appsink), FALSE);

    GRefPtr<GstPad> appSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink, "sink"));
    g_signal_connect(appSinkPad.get(), "notify::caps", G_CALLBACK(appendPipelineAppSinkCapsChanged), this);

    setAppsrcDataLeavingProbe();

#ifdef DEBUG_APPEND_PIPELINE_PADS
    GRefPtr<GstPad> demuxerPad = adoptGRef(gst_element_get_static_pad(m_qtdemux, "sink"));
    m_demuxerDataEnteringPadProbeInformation.m_appendPipeline = this;
    m_demuxerDataEnteringPadProbeInformation.m_description = "demuxer data entering";
    m_demuxerDataEnteringPadProbeInformation.m_probeId = gst_pad_add_probe(demuxerPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelinePadProbeDebugInformation), &m_demuxerDataEnteringPadProbeInformation, nullptr);
    m_appsinkDataEnteringPadProbeInformation.m_appendPipeline = this;
    m_appsinkDataEnteringPadProbeInformation.m_description = "appsink data entering";
    m_appsinkDataEnteringPadProbeInformation.m_probeId = gst_pad_add_probe(appSinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelinePadProbeDebugInformation), &m_appsinkDataEnteringPadProbeInformation, nullptr);
#endif

    // These signals won't be connected outside of the lifetime of "this".
    g_signal_connect(m_appsrc, "need-data", G_CALLBACK(appendPipelineAppsrcNeedData), this);
    g_signal_connect(m_qtdemux, "pad-added", G_CALLBACK(appendPipelineDemuxerPadAdded), this);
    g_signal_connect(m_qtdemux, "pad-removed", G_CALLBACK(appendPipelineDemuxerPadRemoved), this);
    g_signal_connect(m_appsink, "new-sample", G_CALLBACK(appendPipelineAppSinkNewSample), this);
    g_signal_connect(m_appsink, "eos", G_CALLBACK(appendPipelineAppSinkEOS), this);

    // Add_many will take ownership of a reference. Request one ref more for ourselves.
    gst_object_ref(m_appsrc);
    gst_object_ref(m_qtdemux);
    gst_object_ref(m_appsink);

    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, m_qtdemux, NULL);
    gst_element_link(m_appsrc, m_qtdemux);

    gst_element_set_state(m_pipeline, GST_STATE_READY);
};

AppendPipeline::~AppendPipeline()
{
    ASSERT(WTF::isMainThread());

    g_mutex_lock(&m_newSampleMutex);
    setAppendStage(Invalid);
    g_cond_signal(&m_newSampleCondition);
    g_mutex_unlock(&m_newSampleMutex);

    g_mutex_lock(&m_padAddRemoveMutex);
    m_playerPrivate = nullptr;
    g_cond_signal(&m_padAddRemoveCondition);
    g_mutex_unlock(&m_padAddRemoveMutex);

    LOG_MEDIA_MESSAGE("%p", this);

    // TODO: Maybe notify appendComplete here?

    if (m_pipeline) {
        ASSERT(m_bus);
        g_signal_handlers_disconnect_by_func(m_bus.get(), reinterpret_cast<gpointer>(appendPipelineElementMessageCallback), this);
        gst_bus_disable_sync_message_emission(m_bus.get());
        gst_bus_remove_signal_watch(m_bus.get());

        gst_element_set_state (m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = NULL;
    }

    if (m_appsrc) {
        removeAppsrcDataLeavingProbe();

        g_signal_handlers_disconnect_by_func(m_appsrc, (gpointer) appendPipelineAppsrcNeedData, this);

        gst_object_unref(m_appsrc);
        m_appsrc = NULL;
    }

    if (m_qtdemux) {
#ifdef DEBUG_APPEND_PIPELINE_PADS
        GRefPtr<GstPad> demuxerPad = adoptGRef(gst_element_get_static_pad(m_qtdemux, "sink"));
        gst_pad_remove_probe(demuxerPad.get(), m_demuxerDataEnteringPadProbeInformation.m_probeId);
#endif

        g_signal_handlers_disconnect_by_func(m_qtdemux, (gpointer)appendPipelineDemuxerPadAdded, this);
        g_signal_handlers_disconnect_by_func(m_qtdemux, (gpointer)appendPipelineDemuxerPadRemoved, this);

        gst_object_unref(m_qtdemux);
        m_qtdemux = NULL;
    }

    // Should have been freed by disconnectFromAppSink()
    ASSERT(!m_decryptor);

    if (m_appsink) {
        GRefPtr<GstPad> appSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink, "sink"));
        g_signal_handlers_disconnect_by_func(appSinkPad.get(), (gpointer)appendPipelineAppSinkCapsChanged, this);

        g_signal_handlers_disconnect_by_func(m_appsink, (gpointer)appendPipelineAppSinkNewSample, this);
        g_signal_handlers_disconnect_by_func(m_appsink, (gpointer)appendPipelineAppSinkEOS, this);

#ifdef DEBUG_APPEND_PIPELINE_PADS
        gst_pad_remove_probe(appSinkPad.get(), m_appsinkDataEnteringPadProbeInformation.m_probeId);
#endif

        gst_object_unref(m_appsink);
        m_appsink = NULL;
    }

    if (m_appSinkCaps) {
        gst_caps_unref(m_appSinkCaps);
        m_appSinkCaps = NULL;
    }

    if (m_demuxerSrcPadCaps) {
        gst_caps_unref(m_demuxerSrcPadCaps);
        m_demuxerSrcPadCaps = NULL;
    }

    g_cond_clear(&m_newSampleCondition);
    g_mutex_clear(&m_newSampleMutex);

    g_cond_clear(&m_padAddRemoveCondition);
    g_mutex_clear(&m_padAddRemoveMutex);
};

void AppendPipeline::clearPlayerPrivate()
{
    ASSERT(WTF::isMainThread());
    LOG_MEDIA_MESSAGE("cleaning private player");

    g_mutex_lock(&m_newSampleMutex);
    // Make sure that AppendPipeline won't process more data from now on and
    // instruct handleNewSample to abort itself from now on as well.
    setAppendStage(Invalid);

    // Awake any pending handleNewSample operation in the streaming thread.
    g_cond_signal(&m_newSampleCondition);
    g_mutex_unlock(&m_newSampleMutex);

    g_mutex_lock(&m_padAddRemoveMutex);
    m_playerPrivate = nullptr;
    g_cond_signal(&m_padAddRemoveCondition);
    g_mutex_unlock(&m_padAddRemoveMutex);

    // And now that no handleNewSample operations will remain stalled waiting
    // for the main thread, stop the pipeline.
    if (m_pipeline)
        gst_element_set_state (m_pipeline, GST_STATE_NULL);
}

void AppendPipeline::handleElementMessage(GstMessage* message)
{
    ASSERT(WTF::isMainThread());

    const GstStructure* structure = gst_message_get_structure(message);
    if (gst_structure_has_name(structure, "drm-key-needed"))
        setAppendStage(AppendPipeline::AppendStage::KeyNegotiation);

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

    ASSERT_NOT_REACHED();
}

void AppendPipeline::handleAppsrcNeedDataReceived()
{
    if (!m_appsrcAtLeastABufferLeft) {
        TRACE_MEDIA_MESSAGE("discarding until at least a buffer leaves appsrc");
        return;
    }

    ASSERT(m_appendStage == Ongoing || m_appendStage == Sampling);
    ASSERT(!m_appsrcNeedDataReceived);

    TRACE_MEDIA_MESSAGE("received need-data from appsrc");

    m_appsrcNeedDataReceived = true;
    checkEndOfAppend();
}

void AppendPipeline::handleAppsrcAtLeastABufferLeft()
{
    m_appsrcAtLeastABufferLeft = true;
    TRACE_MEDIA_MESSAGE("received buffer-left from appsrc");
#ifndef DEBUG_APPEND_PIPELINE_PADS
    removeAppsrcDataLeavingProbe();
#endif
}

gint AppendPipeline::id()
{
    ASSERT(WTF::isMainThread());

    static gint totalAudio = 0;
    static gint totalVideo = 0;
    static gint totalText = 0;

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
        FALLTHROUGH;
    case Invalid:
        LOG_MEDIA_MESSAGE("Trying to get id for a pipeline of Unknown/Invalid type");
        ASSERT_NOT_REACHED();
        break;
    }

    LOG_MEDIA_MESSAGE("streamType=%d, id=%d", static_cast<int>(m_streamType), m_id);

    return m_id;
}

void AppendPipeline::setAppendStage(AppendStage newAppendStage)
{
    ASSERT(WTF::isMainThread());
    // Valid transitions:
    // NotStarted-->Ongoing-->DataStarve-->NotStarted
    //           |         |            `->Aborting-->NotStarted
    //           |         `->Sampling-···->Sampling-->LastSample-->NotStarted
    //           |         |                                     `->Aborting-->NotStarted
    //           |         `->KeyNegotiation-->Ongoing-->[...]
    //           `->Aborting-->NotStarted
    AppendStage oldAppendStage = m_appendStage;
    AppendStage nextAppendStage = Invalid;

    bool ok = false;

    if (oldAppendStage != newAppendStage)
        TRACE_MEDIA_MESSAGE("%s --> %s", dumpAppendStage(oldAppendStage), dumpAppendStage(newAppendStage));

    switch (oldAppendStage) {
    case NotStarted:
        switch (newAppendStage) {
        case Ongoing:
            ok = true;
            gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
            break;
        case NotStarted:
            ok = true;
            if (m_pendingBuffer) {
                TRACE_MEDIA_MESSAGE("pushing pending buffer %p", m_pendingBuffer.get());
                gst_app_src_push_buffer(GST_APP_SRC(appsrc()), m_pendingBuffer.leakRef());
                nextAppendStage = Ongoing;
            }
            break;
        case Aborting:
            ok = true;
            nextAppendStage = NotStarted;
            break;
        case Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case KeyNegotiation:
        switch (newAppendStage) {
        case Ongoing:
        case Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case Ongoing:
        switch (newAppendStage) {
        case KeyNegotiation:
        case Sampling:
        case Invalid:
            ok = true;
            break;
        case DataStarve:
            ok = true;
            m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
            if (m_abortPending)
                nextAppendStage = Aborting;
            else
                nextAppendStage = NotStarted;
            break;
        default:
            break;
        }
        break;
    case DataStarve:
        switch (newAppendStage) {
        case NotStarted:
        case Invalid:
            ok = true;
            break;
        case Aborting:
            ok = true;
            nextAppendStage = NotStarted;
            break;
        default:
            break;
        }
        break;
    case Sampling:
        switch (newAppendStage) {
        case Sampling:
        case Invalid:
            ok = true;
            break;
        case LastSample:
            ok = true;
            m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
            if (m_abortPending)
                nextAppendStage = Aborting;
            else
                nextAppendStage = NotStarted;
            break;
        default:
            break;
        }
        break;
    case LastSample:
        switch (newAppendStage) {
        case NotStarted:
        case Invalid:
            ok = true;
            break;
        case Aborting:
            ok = true;
            nextAppendStage = NotStarted;
            break;
        default:
            break;
        }
        break;
    case Aborting:
        switch (newAppendStage) {
        case NotStarted:
            ok = true;
            resetPipeline();
            m_abortPending = false;
            nextAppendStage = NotStarted;
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
        m_appendStage = newAppendStage;
    else {
        ERROR_MEDIA_MESSAGE("Invalid append stage transition %s --> %s", dumpAppendStage(oldAppendStage), dumpAppendStage(newAppendStage));
    }

    ASSERT(ok);

    if (nextAppendStage != Invalid)
        setAppendStage(nextAppendStage);
}

// Takes ownership of caps.
void AppendPipeline::parseDemuxerCaps(GstCaps* demuxerSrcPadCaps)
{
    ASSERT(WTF::isMainThread());

    if (m_demuxerSrcPadCaps)
        gst_caps_unref(m_demuxerSrcPadCaps);
    m_demuxerSrcPadCaps = demuxerSrcPadCaps;

    GstStructure* s = gst_caps_get_structure(m_demuxerSrcPadCaps, 0);
    const gchar* structureName = gst_structure_get_name(s);
    GstVideoInfo info;
    bool sizeConfigured = false;

    m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Unknown;

#if GST_CHECK_VERSION(1, 5, 3)
    if (gst_structure_has_name(s, "application/x-cenc")) {
        const gchar* originalMediaType = gst_structure_get_string(s, "original-media-type");

        // Any previous decriptor should have been removed from the pipeline by disconnectFromAppSinkFromStreamingThread()
        ASSERT(!m_decryptor);

        m_decryptor = WebCore::createGstDecryptor(gst_structure_get_string(s, "protection-system"));
        if (!m_decryptor) {
            ERROR_MEDIA_MESSAGE("decryptor not found for caps: %" GST_PTR_FORMAT, m_demuxerSrcPadCaps);
            return;
        }

        if (g_str_has_prefix(originalMediaType, "video/")) {
            int width = 0;
            int height = 0;
            float finalHeight = 0;

            gst_structure_get_int(s, "width", &width);
            if (gst_structure_get_int(s, "height", &height)) {
                gint par_n = 1;
                gint par_d = 1;

                gst_structure_get_fraction(s, "pixel-aspect-ratio", &par_n, &par_d);
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

void AppendPipeline::appSinkCapsChanged()
{
    ASSERT(WTF::isMainThread());

    if (!m_appsink)
        return;

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(m_appsink, "sink"));
    GstCaps* caps = gst_pad_get_current_caps(pad.get());

    // This means that we're right after a new track has appeared. Otherwise, it's a caps change inside the same track.
    bool previousCapsWereNull = (m_appSinkCaps == NULL);

    if (!caps)
        return;

    // Transfer caps ownership to m_appSinkCaps.
    if (gst_caps_replace(&m_appSinkCaps, caps)) {
        if (m_playerPrivate && previousCapsWereNull)
            m_playerPrivate->trackDetected(this, m_oldTrack, m_track);
        didReceiveInitializationSegment();
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    }

    gst_caps_unref(caps);
}

void AppendPipeline::checkEndOfAppend()
{
    ASSERT(WTF::isMainThread());

    if (!m_appsrcNeedDataReceived || (m_appendStage != Ongoing && m_appendStage != Sampling))
        return;

    TRACE_MEDIA_MESSAGE("end of append data mark was received");

    switch (m_appendStage) {
    case Ongoing:
        TRACE_MEDIA_MESSAGE("DataStarve");
        m_appsrcNeedDataReceived = false;
        setAppendStage(DataStarve);
        break;
    case Sampling:
        TRACE_MEDIA_MESSAGE("LastSample");
        m_appsrcNeedDataReceived = false;
        setAppendStage(LastSample);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void AppendPipeline::appSinkNewSample(GstSample* sample)
{
    ASSERT(WTF::isMainThread());

    g_mutex_lock(&m_newSampleMutex);

    // Ignore samples if we're not expecting them. Refuse processing if we're in Invalid state.
    if (!(m_appendStage == Ongoing || m_appendStage == Sampling)) {
        WARN_MEDIA_MESSAGE("Unexpected sample, stage=%s", dumpAppendStage(m_appendStage));
        // TODO: Return ERROR and find a more robust way to detect that all the
        // data has been processed, so we don't need to resort to these hacks.
        // All in all, return OK, even if it's not the proper thing to do. We don't want to break the demuxer.
        m_flowReturn = GST_FLOW_OK;
        g_cond_signal(&m_newSampleCondition);
        g_mutex_unlock(&m_newSampleMutex);
        return;
    }

    RefPtr<GStreamerMediaSample> mediaSample = WebCore::GStreamerMediaSample::create(sample, m_presentationSize, trackId());

    TRACE_MEDIA_MESSAGE("append: trackId=%s PTS=%f presentationSize=%.0fx%.0f", mediaSample->trackID().string().utf8().data(), mediaSample->presentationTime().toFloat(), mediaSample->presentationSize().width(), mediaSample->presentationSize().height());

    // If we're beyond the duration, ignore this sample and the remaining ones.
    MediaTime duration = m_mediaSourceClient->duration();
    if (duration.isValid() && !duration.indefiniteTime() && mediaSample->presentationTime() > duration) {
        LOG_MEDIA_MESSAGE("Detected sample (%f) beyond the duration (%f), declaring LastSample", mediaSample->presentationTime().toFloat(), duration.toFloat());
        setAppendStage(LastSample);
        m_flowReturn = GST_FLOW_OK;
        g_cond_signal(&m_newSampleCondition);
        g_mutex_unlock(&m_newSampleMutex);
        return;
    }

    // Add a fake sample if a gap is detected before the first sample
    if (mediaSample->decodeTime() == MediaTime::zeroTime() &&
        mediaSample->presentationTime() > MediaTime::zeroTime() &&
        mediaSample->presentationTime() <= MediaTime::createWithDouble(0.1)) {
         LOG_MEDIA_MESSAGE("Adding fake offset");
        mediaSample->applyPtsOffset(MediaTime::zeroTime());
    }

    m_sourceBufferPrivate->didReceiveSample(mediaSample);
    setAppendStage(Sampling);
    m_flowReturn = GST_FLOW_OK;
    g_cond_signal(&m_newSampleCondition);
    g_mutex_unlock(&m_newSampleMutex);

    checkEndOfAppend();
}

void AppendPipeline::appSinkEOS()
{
    ASSERT(WTF::isMainThread());

    switch (m_appendStage) {
    // Ignored. Operation completion will be managed by the Aborting->NotStarted transition.
    case Aborting:
        return;
    // Finish Ongoing and Sampling stages.
    case Ongoing:
        setAppendStage(DataStarve);
        break;
    case Sampling:
        setAppendStage(LastSample);
        break;
    default:
        LOG_MEDIA_MESSAGE("Unexpected EOS");
        break;
    }
}

void AppendPipeline::didReceiveInitializationSegment()
{
    ASSERT(WTF::isMainThread());


    WebCore::SourceBufferPrivateClient::InitializationSegment initializationSegment;

    LOG_MEDIA_MESSAGE("Nofifying SourceBuffer for track %s", m_track->id().string().utf8().data());
    initializationSegment.duration = m_mediaSourceClient->duration();
    switch (m_streamType) {
    case Audio:
        {
            WebCore::SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
            info.track = static_cast<AudioTrackPrivateGStreamer*>(m_track.get());
            info.description = WebCore::GStreamerMediaDescription::create(m_demuxerSrcPadCaps);
            initializationSegment.audioTracks.append(info);
        }
        break;
    case Video:
        {
            WebCore::SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
            info.track = static_cast<VideoTrackPrivateGStreamer*>(m_track.get());
            info.description = WebCore::GStreamerMediaDescription::create(m_demuxerSrcPadCaps);
            initializationSegment.videoTracks.append(info);
        }
        break;
    default:
        LOG_MEDIA_MESSAGE("Unsupported or unknown stream type");
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
    LOG_MEDIA_MESSAGE("resetting pipeline");
    m_appsrcAtLeastABufferLeft = false;
    setAppsrcDataLeavingProbe();
    g_mutex_lock(&m_newSampleMutex);
    g_cond_signal(&m_newSampleCondition);
    gst_element_set_state(m_pipeline, GST_STATE_READY);
    gst_element_get_state(m_pipeline, NULL, NULL, 0);
    g_mutex_unlock(&m_newSampleMutex);

    {
        static int i = 0;
        WTF::String  dotFileName = String::format("reset-pipeline-%d", ++i);
        gst_debug_bin_to_dot_file(GST_BIN(m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.utf8().data());
    }

}

void AppendPipeline::setAppsrcDataLeavingProbe()
{
    if (m_appsrcDataLeavingProbeId)
        return;

    TRACE_MEDIA_MESSAGE("setting appsrc data leaving probe");

    GRefPtr<GstPad> appsrcPad = adoptGRef(gst_element_get_static_pad(m_appsrc, "src"));
    m_appsrcDataLeavingProbeId = gst_pad_add_probe(appsrcPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelineAppsrcDataLeaving), this, nullptr);
}

void AppendPipeline::removeAppsrcDataLeavingProbe()
{
    if (!m_appsrcDataLeavingProbeId)
        return;

    TRACE_MEDIA_MESSAGE("removing appsrc data leaving probe");

    GRefPtr<GstPad> appsrcPad = adoptGRef(gst_element_get_static_pad(m_appsrc, "src"));
    gst_pad_remove_probe(appsrcPad.get(), m_appsrcDataLeavingProbeId);
    m_appsrcDataLeavingProbeId = 0;
}

void AppendPipeline::abort()
{
    ASSERT(WTF::isMainThread());
    LOG_MEDIA_MESSAGE("aborting");

    m_pendingBuffer.clear();

    // Abort already ongoing.
    if (m_abortPending)
        return;

    m_abortPending = true;
    if (m_appendStage == NotStarted)
        setAppendStage(Aborting);
    // Else, the automatic stage transitions will take care when the ongoing append finishes.
}

GstFlowReturn AppendPipeline::pushNewBuffer(GstBuffer* buffer)
{
    GstFlowReturn result;

    if (m_abortPending) {
        m_pendingBuffer = adoptGRef(buffer);
        result = GST_FLOW_OK;
    } else {
        setAppendStage(AppendPipeline::Ongoing);
        TRACE_MEDIA_MESSAGE("pushing new buffer %p", buffer);
        result = gst_app_src_push_buffer(GST_APP_SRC(appsrc()), buffer);
    }

    return result;
}

void AppendPipeline::reportAppsrcAtLeastABufferLeft()
{
    TRACE_MEDIA_MESSAGE("buffer left appsrc, reposting to bus");
    GstStructure* structure = gst_structure_new_empty("appsrc-buffer-left");
    GstMessage* message = gst_message_new_application(GST_OBJECT(m_appsrc), structure);
    gst_bus_post(m_bus.get(), message);
}

void AppendPipeline::reportAppsrcNeedDataReceived()
{
    TRACE_MEDIA_MESSAGE("received need-data signal at appsrc, reposting to bus");
    GstStructure* structure = gst_structure_new_empty("appsrc-need-data");
    GstMessage* message = gst_message_new_application(GST_OBJECT(m_appsrc), structure);
    gst_bus_post(m_bus.get(), message);
}

GstFlowReturn AppendPipeline::handleNewSample(GstElement* appsink)
{
    ASSERT(!WTF::isMainThread());

    bool invalid;
    g_mutex_lock(&m_newSampleMutex);
    invalid = !m_playerPrivate || m_appendStage == Invalid;
    g_mutex_unlock(&m_newSampleMutex);

    // Even if we're disabled, it's important to pull the sample out anyway to
    // avoid deadlocks when changing to NULL state having a non empty appsink.
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));

    if (invalid) {
        LOG_MEDIA_MESSAGE("AppendPipeline has been disabled, ignoring this sample");
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    g_mutex_lock(&m_newSampleMutex);
    if (!(!m_playerPrivate || m_appendStage == Invalid)) {
        NewSampleInfo* info = new NewSampleInfo(sample, this);
        g_timeout_add(0, GSourceFunc(appendPipelineAppSinkNewSampleMainThread), info);
        g_cond_wait(&m_newSampleCondition, &m_newSampleMutex);
        // We've been awaken because the sample was processed or because of
        // an exceptional condition (entered in Invalid state, destructor, etc.)
        // We can't reliably delete info here, appendPipelineAppSinkNewSampleMainThread will do it.
    }
    g_mutex_unlock(&m_newSampleMutex);
    gst_sample_unref(sample);
    return m_flowReturn;
}

void AppendPipeline::connectToAppSinkFromAnyThread(GstPad* demuxerSrcPad)
{
    if (!m_appsink)
        return;

    LOG_MEDIA_MESSAGE("connecting to appsink");

    GRefPtr<GstPad> sinkSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink, "sink"));

    // Only one Stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(sinkSinkPad.get()));

    gint64 timeLength = 0;
    if (gst_element_query_duration(m_qtdemux, GST_FORMAT_TIME, &timeLength) &&
        static_cast<guint64>(timeLength) != GST_CLOCK_TIME_NONE) {
        m_initialDuration = MediaTime(timeLength, GST_SECOND);
    } else {
        m_initialDuration = MediaTime::positiveInfiniteTime();
    }

    if (WTF::isMainThread()) {
        connectToAppSink(demuxerSrcPad);
    } else {
        // Call connectToAppSink() in the main thread and wait.
        WTF::GMutexLocker<GMutex> lock(m_padAddRemoveMutex);
        if (!m_playerPrivate)
            return;
        PadInfo* info = new PadInfo(demuxerSrcPad, this);  // will be deleted on main thread
        g_timeout_add(0, GSourceFunc(appendPipelineDemuxerConnectToAppSinkMainThread), info);
        g_cond_wait(&m_padAddRemoveCondition, &m_padAddRemoveMutex);
    }

    // Must be done in the thread we were called from (usually streaming thread).
    bool isData;

    switch (m_streamType) {
    case WebCore::MediaSourceStreamTypeGStreamer::Audio:
    case WebCore::MediaSourceStreamTypeGStreamer::Video:
    case WebCore::MediaSourceStreamTypeGStreamer::Text:
        isData = true;
        break;
    default:
        isData = false;
        break;
    }

    if (isData) {
        LOG_MEDIA_MESSAGE("Encrypted stream: %s", m_decryptor ? "yes" : "no");
        // FIXME: Only add appsink one time. This method can be called several times.
        if (gst_element_get_parent(m_appsink) == NULL)
            gst_bin_add(GST_BIN(m_pipeline), m_appsink);
        if (m_decryptor) {
            gst_object_ref(m_decryptor);
            gst_bin_add(GST_BIN(m_pipeline), m_decryptor);
            GRefPtr<GstPad> decryptorSrcPad = adoptGRef(gst_element_get_static_pad(m_decryptor, "src"));
            GRefPtr<GstPad> decryptorSinkPad = adoptGRef(gst_element_get_static_pad(m_decryptor, "sink"));
            gst_pad_link(demuxerSrcPad, decryptorSinkPad.get());
            gst_pad_link(decryptorSrcPad.get(), sinkSinkPad.get());
            gst_element_sync_state_with_parent(m_appsink);
            gst_element_sync_state_with_parent(m_decryptor);
        } else {
            gst_pad_link(demuxerSrcPad, sinkSinkPad.get());
            gst_element_sync_state_with_parent(m_appsink);
            //gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        }
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    }
}

void AppendPipeline::connectToAppSink(GstPad* demuxerSrcPad)
{
    ASSERT(WTF::isMainThread());
    LOG_MEDIA_MESSAGE("Connecting to appsink");

    WTF::GMutexLocker<GMutex> lock(m_padAddRemoveMutex);
    GRefPtr<GstPad> sinkSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink, "sink"));

    // Only one Stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(sinkSinkPad.get()));

    GRefPtr<GstCaps> caps = adoptGRef(gst_pad_get_current_caps(GST_PAD(demuxerSrcPad)));

    if (!caps || m_appendStage == Invalid || !m_playerPrivate) {
        g_cond_signal(&m_padAddRemoveCondition);
        return;
    }

#if !LOG_DISABLED
    {
        gchar* strcaps = gst_caps_to_string(caps.get());
        LOG_MEDIA_MESSAGE("%s", strcaps);
        g_free(strcaps);
    }
#endif

    if (m_initialDuration > m_mediaSourceClient->duration())
        m_mediaSourceClient->durationChanged(m_initialDuration);

    m_oldTrack = m_track;

    // May create m_decryptor
    parseDemuxerCaps(gst_caps_ref(caps.get()));

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
        // No useful data, but notify anyway to complete the append operation
        LOG_MEDIA_MESSAGE("(no data)");
        m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
        break;
    }

    g_cond_signal(&m_padAddRemoveCondition);
}

void AppendPipeline::disconnectFromAppSinkFromAnyThread()
{
    LOG_MEDIA_MESSAGE("Disconnecting appsink");

    // Must be done in the thread we were called from (usually streaming thread).
    if (m_decryptor) {
        gst_element_unlink(m_decryptor, m_appsink);
        gst_element_unlink(m_qtdemux, m_decryptor);
        gst_element_set_state(m_decryptor, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(m_pipeline), m_decryptor);
    } else
        gst_element_unlink(m_qtdemux, m_appsink);

    // Call disconnectFromAppSink() in the main thread and wait.
    // TODO: Optimize this and call only when there's m_decryptor. By now I call it always to keep code symmetry.
    if (WTF::isMainThread()) {
        disconnectFromAppSink();
    } else {
        WTF::GMutexLocker<GMutex> lock(m_padAddRemoveMutex);
        if (!m_playerPrivate) {
            if (m_decryptor) {
                LOG_MEDIA_MESSAGE("Releasing decryptor");
                gst_object_unref(m_decryptor);
                m_decryptor = NULL;
            }
            return;
        }
        PadInfo* info = new PadInfo(NULL, this);  // will be deleted on main thread
        g_timeout_add(0, GSourceFunc(appendPipelineDemuxerDisconnectFromAppSinkMainThread), info);
        g_cond_wait(&m_padAddRemoveCondition, &m_padAddRemoveMutex);
    }
}

void AppendPipeline::disconnectFromAppSink()
{
    ASSERT(WTF::isMainThread());

    WTF::GMutexLocker<GMutex> lock(m_padAddRemoveMutex);
    if (m_decryptor) {
        LOG_MEDIA_MESSAGE("Releasing decryptor");
        gst_object_unref(m_decryptor);
        m_decryptor = NULL;
    }
    g_cond_signal(&m_padAddRemoveCondition);
}

static gboolean appSinkCapsChangedFromMainThread(gpointer data)
{
    AppendPipeline* ap = reinterpret_cast<AppendPipeline*>(data);
    ap->appSinkCapsChanged();
    ap->deref();
    return G_SOURCE_REMOVE;
}

static void appendPipelineAppSinkCapsChanged(GObject*, GParamSpec*, AppendPipeline* ap)
{
    ap->ref();
    g_timeout_add(0, appSinkCapsChangedFromMainThread, ap);
}

static GstPadProbeReturn appendPipelineAppsrcDataLeaving(GstPad*, GstPadProbeInfo* info, AppendPipeline* appendPipeline)
{
    ASSERT(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER);

    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    gsize bufferSize = gst_buffer_get_size(buffer);

    TRACE_MEDIA_MESSAGE("buffer of size %" G_GSIZE_FORMAT " going thru", bufferSize);

    appendPipeline->reportAppsrcAtLeastABufferLeft();

    return GST_PAD_PROBE_OK;
}

#ifdef DEBUG_APPEND_PIPELINE_PADS
static GstPadProbeReturn appendPipelinePadProbeDebugInformation(GstPad*, GstPadProbeInfo* info, struct PadProbeInformation* padProbeInformation)
{
    ASSERT(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    TRACE_MEDIA_MESSAGE("%s: buffer of size %" G_GSIZE_FORMAT " going thru", padProbeInformation->m_description, gst_buffer_get_size(buffer));
    return GST_PAD_PROBE_OK;
}
#endif

static void appendPipelineAppsrcNeedData(GstAppSrc*, guint, AppendPipeline* appendPipeline)
{
    appendPipeline->reportAppsrcNeedDataReceived();
}

static void appendPipelineDemuxerPadAdded(GstElement*, GstPad* demuxerSrcPad, AppendPipeline* ap)
{
    ap->connectToAppSinkFromAnyThread(demuxerSrcPad);
}

static gboolean appendPipelineDemuxerConnectToAppSinkMainThread(PadInfo* info)
{
    info->ap()->connectToAppSink(info->demuxerSrcPad());
    delete info;
    return G_SOURCE_REMOVE;
}

static gboolean appendPipelineDemuxerDisconnectFromAppSinkMainThread(PadInfo* info)
{
    info->ap()->disconnectFromAppSink();
    delete info;
    return G_SOURCE_REMOVE;
}

static void appendPipelineDemuxerPadRemoved(GstElement*, GstPad*, AppendPipeline* ap)
{
    ap->disconnectFromAppSinkFromAnyThread();
}

static GstFlowReturn appendPipelineAppSinkNewSample(GstElement* appsink, AppendPipeline* ap)
{
    return ap->handleNewSample(appsink);
}

static gboolean appendPipelineAppSinkNewSampleMainThread(NewSampleInfo* info)
{
    info->ap()->appSinkNewSample(info->sample());
    delete info;

    return G_SOURCE_REMOVE;
}

static void appendPipelineAppSinkEOS(GstElement*, AppendPipeline* ap)
{
    if (WTF::isMainThread())
        ap->appSinkEOS();
    else {
        ap->ref();
        g_timeout_add(0, GSourceFunc(appendPipelineAppSinkEOSMainThread), ap);
    }

    LOG_MEDIA_MESSAGE("%s main thread", (WTF::isMainThread())?"IS":"NOT");
}

static gboolean appendPipelineAppSinkEOSMainThread(AppendPipeline* ap)
{
    ap->appSinkEOS();
    ap->deref();
    return G_SOURCE_REMOVE;
}

PassRefPtr<MediaSourceClientGStreamerMSE> MediaSourceClientGStreamerMSE::create(MediaPlayerPrivateGStreamerMSE* playerPrivate)
{
    ASSERT(WTF::isMainThread());

    // return adoptRef(new MediaSourceClientGStreamerMSE(playerPrivate));
    // No adoptRef because the ownership has already been transferred to MediaPlayerPrivateGStreamerMSE
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

    RefPtr<AppendPipeline> ap = adoptRef(new AppendPipeline(this, sourceBufferPrivate, m_playerPrivate));
    LOG_MEDIA_MESSAGE("this=%p sourceBuffer=%p ap=%p", this, sourceBufferPrivate.get(), ap.get());
    m_playerPrivate->m_appendPipelinesMap.add(sourceBufferPrivate, ap);

    ASSERT(m_playerPrivate->m_playbackPipeline);

    return m_playerPrivate->m_playbackPipeline->addSourceBuffer(sourceBufferPrivate);
}

float MediaPlayerPrivateGStreamerMSE::currentTime() const
{
    float position = MediaPlayerPrivateGStreamer::currentTime();

    if (m_eosPending && (paused() || (position >= duration()))) {
        if (m_networkState != MediaPlayer::Loaded) {
            m_networkState = MediaPlayer::Loaded;
            m_player->networkStateChanged();
        }

        m_eosPending = false;
        m_isEndReached = true;
        m_cachedPosition = m_mediaTimeDuration.toFloat();
        m_mediaDuration = m_mediaTimeDuration.toFloat();
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

    TRACE_MEDIA_MESSAGE("duration: %f", duration.toFloat());
    if (!duration.isValid() || duration.isPositiveInfinite() || duration.isNegativeInfinite())
        return;

    m_duration = duration;
    if (m_playerPrivate)
        m_playerPrivate->durationChanged();
}

void MediaSourceClientGStreamerMSE::abort(PassRefPtr<SourceBufferPrivateGStreamer> prpSourceBufferPrivate)
{
    ASSERT(WTF::isMainThread());

    LOG_MEDIA_MESSAGE("aborting");

    if (!m_playerPrivate)
        return;

    RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate = prpSourceBufferPrivate;
    RefPtr<AppendPipeline> ap = m_playerPrivate->m_appendPipelinesMap.get(sourceBufferPrivate);
    ap->abort();
}

bool MediaSourceClientGStreamerMSE::append(PassRefPtr<SourceBufferPrivateGStreamer> prpSourceBufferPrivate, const unsigned char* data, unsigned length)
{
    ASSERT(WTF::isMainThread());

    LOG_MEDIA_MESSAGE("Appending %u bytes", length);

    if (!m_playerPrivate)
        return false;

    RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate = prpSourceBufferPrivate;
    RefPtr<AppendPipeline> ap = m_playerPrivate->m_appendPipelinesMap.get(sourceBufferPrivate);
    GstBuffer* buffer = gst_buffer_new_and_alloc(length);
    gst_buffer_fill(buffer, 0, data, length);

    return GST_FLOW_OK == ap->pushNewBuffer(buffer);
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

    RefPtr<AppendPipeline> ap = m_playerPrivate->m_appendPipelinesMap.get(sourceBufferPrivate);
    ap->clearPlayerPrivate();
    m_playerPrivate->m_appendPipelinesMap.remove(sourceBufferPrivate);
    // AppendPipeline destructor will take care of cleaning up when appropriate.

    ASSERT(m_playerPrivate->m_playbackPipeline);

    m_playerPrivate->m_playbackPipeline->removeSourceBuffer(sourceBufferPrivate);
}

void MediaSourceClientGStreamerMSE::flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample> > samples)
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

    sourceBuffer->didReceiveInitializationSegment(initializationSegment);
}

void MediaSourceClientGStreamerMSE::didReceiveAllPendingSamples(SourceBufferPrivateGStreamer* sourceBuffer)
{
    ASSERT(WTF::isMainThread());

    LOG_MEDIA_MESSAGE("received all pending samples");
    sourceBuffer->didReceiveAllPendingSamples();
}

GRefPtr<WebKitMediaSrc> MediaSourceClientGStreamerMSE::webKitMediaSrc()
{
    ASSERT(WTF::isMainThread());

    if (!m_playerPrivate)
        return GRefPtr<WebKitMediaSrc>(NULL);

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
        return 0.0f;

    LOG_MEDIA_MESSAGE("maxTimeSeekable");
    float result = duration();
    // infinite duration means live stream
    if (isinf(result)) {
        MediaTime maxBufferedTime = buffered()->maximumBufferedTime();
        // Return the highest end time reported by the buffered attribute.
        result = maxBufferedTime.isValid() ? maxBufferedTime.toFloat() : 0;
    }

    return result;
}

} // namespace WebCore

#endif // USE(GSTREAMER)
