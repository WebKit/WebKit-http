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

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>


#if USE(DXDRM)
#include "DiscretixSession.h"
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

class AppendPipeline : public ThreadSafeRefCounted<AppendPipeline> {
public:
    enum AppendStage { Invalid, NotStarted, Ongoing, KeyNegotiation, NoDataToDecode, Sampling, LastSample, Aborting };

    static const unsigned int s_noDataToDecodeTimeoutMsec = 1000;
    static const unsigned int s_lastSampleTimeoutMsec = 250;

    AppendPipeline(PassRefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient, PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, MediaPlayerPrivateGStreamerMSE* playerPrivate);
    virtual ~AppendPipeline();

    void handleElementMessage(GstMessage*);

    gint id();
    AppendStage appendStage() { return m_appendStage; }
    void setAppendStage(AppendStage newAppendStage);

    GstFlowReturn handleNewSample(GstElement* appsink);

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

private:
    void resetPipeline();

// TODO: Hide everything and use getters/setters.
private:
    RefPtr<MediaSourceClientGStreamerMSE> m_mediaSourceClient;
    RefPtr<SourceBufferPrivateGStreamer> m_sourceBufferPrivate;
    MediaPlayerPrivateGStreamerMSE* m_playerPrivate;

    // (m_mediaType, m_id) is unique.
    gint m_id;

    GstFlowReturn m_flowReturn;

    GstElement* m_pipeline;
    GstElement* m_appsrc;
    GstElement* m_typefind;
    GstElement* m_qtdemux;

    GstElement* m_decryptor;

    // The demuxer has one src Stream only.
    GstElement* m_appsink;

    GMutex m_newSampleMutex;
    GCond m_newSampleCondition;
    GMutex m_padRemovedMutex;
    GCond m_padRemovedCondition;

    GstCaps* m_appSinkCaps;
    GstCaps* m_demuxerSrcPadCaps;
    FloatSize m_presentationSize;

    // Some appended data are only headers and don't generate any
    // useful stream data for decoding. This is detected with a
    // timeout and reported to the upper layers, so update/updateend
    // can be generated and the append operation doesn't block.
    guint m_noDataToDecodeTimeoutTag;

    // Used to detect the last sample. Rescheduled each time a new
    // sample arrives.
    guint m_lastSampleTimeoutTag;

    // Keeps track of the stages of append processing, to avoid
    // performing actions inappropriate for the current stage (eg:
    // processing more samples when the last one has been detected
    // or the noDataToDecodeTimeout has been triggered).
    // See setAppendStage() for valid transitions.
    AppendStage m_appendStage;

    // Aborts can only be completed when the normal sample detection
    // has finished. Meanwhile, the willing to abort is expressed in
    // this field.
    bool m_abortPending;

    WebCore::MediaSourceStreamTypeGStreamer m_streamType;
    RefPtr<WebCore::TrackPrivateBase> m_oldTrack;
    RefPtr<WebCore::TrackPrivateBase> m_track;
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
    , m_webKitMediaSrc(0)
    , m_seekCompleted(true)
{
    LOG_MEDIA_MESSAGE("%p", this);
}

MediaPlayerPrivateGStreamerMSE::~MediaPlayerPrivateGStreamerMSE()
{
    LOG_MEDIA_MESSAGE("destroying the player");

    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it)
        it->value->clearPlayerPrivate();

    clearSamples();

    if (m_webKitMediaSrc) {
        webkit_media_src_set_mediaplayerprivate(WEBKIT_MEDIA_SRC(m_webKitMediaSrc.get()), 0);
        g_signal_handlers_disconnect_matched(m_webKitMediaSrc.get(), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
    }
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

float MediaPlayerPrivateGStreamerMSE::duration() const
{
    if (!m_pipeline)
        return 0.0f;

    if (m_errorOccured)
        return 0.0f;

    return m_mediaDuration;
}

static gboolean dumpPipeline(gpointer data)
{
    GstElement* pipeline = reinterpret_cast<GstElement*>(data);

    g_printerr("Dumping pipeline\n");
    CString dotFileName = "pipeline-dump";
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.data());

    return G_SOURCE_REMOVE;
}

void MediaPlayerPrivateGStreamerMSE::notifySeekNeedsData(const MediaTime& seekTime)
{
    // Reenqueue samples needed to resume playback in the new position
    m_mediaSource->seekToTime(seekTime);

    LOG_MEDIA_MESSAGE("MSE seek to %f finished", seekTime.toDouble());
}

bool MediaPlayerPrivateGStreamerMSE::doSeek(gint64 position, float rate, GstSeekFlags seekType)
{
    gint64 startTime, endTime;

    if (rate > 0) {
        startTime = position;
        endTime = GST_CLOCK_TIME_NONE;
    } else {
        startTime = 0;
        // If we are at beginning of media, start from the end to
        // avoid immediate EOS.
        if (position < 0)
            endTime = static_cast<gint64>(duration() * GST_SECOND);
        else
            endTime = position;
    }

    if (!rate)
        rate = 1.0;

    LOG_MEDIA_MESSAGE("Actual seek to %" GST_TIME_FORMAT ", end time:  %" GST_TIME_FORMAT ", rate: %f", GST_TIME_ARGS(startTime), GST_TIME_ARGS(endTime), rate);

    MediaTime time(MediaTime::createWithDouble(double(static_cast<double>(position) / GST_SECOND)));

    // DEBUG
    {
        float currentTime = 0.0f;
        gint64 pos = GST_CLOCK_TIME_NONE;
        GstQuery* query= gst_query_new_position(GST_FORMAT_TIME);
        if (gst_element_query(m_pipeline.get(), query))
            gst_query_parse_position(query, 0, &pos);
        currentTime = static_cast<double>(pos) / GST_SECOND;
        LOG_MEDIA_MESSAGE("seekTime=%f, currentTime=%f", time.toFloat(), currentTime);
    }

    // This will call notifySeekNeedsData() after some time to tell that the pipeline is ready for sample enqueuing.
    webkit_media_src_prepare_seek(WEBKIT_MEDIA_SRC(m_webKitMediaSrc.get()), time);

    // DEBUG
    dumpPipeline(m_pipeline.get());

    if (!gst_element_seek(m_pipeline.get(), rate, GST_FORMAT_TIME, seekType,
        GST_SEEK_TYPE_SET, startTime, GST_SEEK_TYPE_SET, endTime)) {
        LOG_MEDIA_MESSAGE("Returning false");
        return false;
    }

    // The samples will be enqueued in notifySeekNeedsData()
    LOG_MEDIA_MESSAGE("Returning true");
    return true;
}

void MediaPlayerPrivateGStreamerMSE::updatePlaybackRate()
{
    notImplemented();
}

bool MediaPlayerPrivateGStreamerMSE::seeking() const
{
    return m_seeking && !m_seekCompleted;
}

// METRO FIXME: GStreamer mediaplayer manages the readystate on its own. We shouldn't change it manually.
void MediaPlayerPrivateGStreamerMSE::setReadyState(MediaPlayer::ReadyState state)
{
    // FIXME: early return here.
    if (state != m_readyState) {
        LOG_MEDIA_MESSAGE("Ready State Changed manually from %u to %u", m_readyState, state);
        MediaPlayer::ReadyState oldReadyState = m_readyState;
        m_readyState = state;
        LOG_MEDIA_MESSAGE("m_readyState: %s -> %s", dumpReadyState(oldReadyState), dumpReadyState(m_readyState));

        GstState state;
        GstStateChangeReturn getStateResult = gst_element_get_state(m_pipeline.get(), &state, NULL, 250 * GST_NSECOND);
        bool isPlaying = (getStateResult == GST_STATE_CHANGE_SUCCESS && state == GST_STATE_PLAYING);

        if (m_readyState == MediaPlayer::HaveMetadata && oldReadyState > MediaPlayer::HaveMetadata && isPlaying) {
            LOG_MEDIA_MESSAGE("Changing pipeline to PAUSED...");
            // Not using changePipelineState() because we don't want the state to drop to GST_STATE_NULL ever.
            bool ok = gst_element_set_state(m_pipeline.get(), GST_STATE_PAUSED) == GST_STATE_CHANGE_SUCCESS;
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
    m_seekCompleted = false;
}

void MediaPlayerPrivateGStreamerMSE::seekCompleted()
{
    if (m_seekCompleted)
        return;

    LOG_MEDIA_MESSAGE("MSE seek completed");
    m_seekCompleted = true;

    if (!seeking() && m_readyState >= MediaPlayer::HaveFutureData)
        gst_element_set_state(m_pipeline.get(), GST_STATE_PLAYING);

    if (!m_seeking)
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
    m_webKitMediaSrc.clear();
    g_object_get(m_pipeline.get(), "source", &m_webKitMediaSrc.outPtr(), nullptr);

    ASSERT(WEBKIT_IS_MEDIA_SRC(m_webKitMediaSrc.get()));

    m_playbackPipeline->setWebKitMediaSrc(WEBKIT_MEDIA_SRC(m_webKitMediaSrc.get()));

    MediaSourceGStreamer::open(m_mediaSource.get(), this);
    g_signal_connect_swapped(m_webKitMediaSrc.get(), "video-changed", G_CALLBACK(videoChangedCallback), this);
    g_signal_connect_swapped(m_webKitMediaSrc.get(), "audio-changed", G_CALLBACK(audioChangedCallback), this);
    g_signal_connect_swapped(m_webKitMediaSrc.get(), "text-changed", G_CALLBACK(textChangedCallback), this);
    webkit_media_src_set_mediaplayerprivate(WEBKIT_MEDIA_SRC(m_webKitMediaSrc.get()), this);
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
            m_mediaDuration = 0;
        } else {
            m_resetPipeline = false;
            cacheDuration();
        }

        bool didBuffering = m_buffering;

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
            if (m_seeking && !timeIsBuffered(m_seekTime)) {
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

            if (didBuffering && !m_buffering && !m_paused && m_playbackRate) {
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
        if (m_seekIsPending && timeIsBuffered(m_seekTime)) {
            LOG_MEDIA_MESSAGE("[Seek] committing pending seek to %f", m_seekTime);
            m_seekIsPending = false;
            m_seeking = doSeek(toGstClockTime(m_seekTime), m_player->rate(), static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE));
            LOG_MEDIA_MESSAGE("m_seeking=%s", m_seeking?"true":"false");
            if (!m_seeking) {
                m_cachedPosition = -1;
                LOG_MEDIA_MESSAGE("[Seek] seeking to %f failed", m_seekTime);
            }
        }
    }
}

bool MediaPlayerPrivateGStreamerMSE::timeIsBuffered(float time)
{
    bool result = m_mediaSource && m_mediaSource->buffered()->contain(MediaTime::createWithFloat(time));
    LOG_MEDIA_MESSAGE("Time %f buffered? %s", time, result ? "aye" : "nope");
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
    float previousDuration = m_mediaDuration;

    if (!m_mediaSourceClient) {
        LOG_MEDIA_MESSAGE("m_mediaSourceClient is null, not doing anything");
        return;
    }
    m_mediaDuration = m_mediaSourceClient->duration().toFloat();

    LOG_MEDIA_MESSAGE("previous=%f, new=%f", previousDuration, m_mediaDuration);
    cacheDuration();
    // Avoid emiting durationchanged in the case where the previous
    // duration was 0 because that case is already handled by the
    // HTMLMediaElement.
    if (m_mediaDuration != previousDuration && !std::isnan(m_mediaDuration) && !std::isnan(previousDuration)) {
        LOG_MEDIA_MESSAGE("Notifying player and WebKitMediaSrc");
        m_player->durationChanged();
        m_playbackPipeline->notifyDurationChanged();
    }
}

static HashSet<String> mimeTypeCache()
{
    initializeGStreamerAndRegisterWebKitMESElement();

    DEPRECATED_DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;

    if (typeListInitialized)
        return cache;

    const char* mimeTypes[] = {
#if !USE(HOLE_PUNCH_EXTERNAL)
        "video/mp4",
#endif
        "audio/mp4"
    };

    for (unsigned i = 0; i < (sizeof(mimeTypes) / sizeof(*mimeTypes)); ++i)
        cache.add(String(mimeTypes[i]));

    typeListInitialized = true;
    return cache;
}

void MediaPlayerPrivateGStreamerMSE::getSupportedTypes(HashSet<String>& types)
{
    types = mimeTypeCache();
}

void MediaPlayerPrivateGStreamerMSE::trackDetected(RefPtr<AppendPipeline> ap, RefPtr<WebCore::TrackPrivateBase> oldTrack, RefPtr<WebCore::TrackPrivateBase> newTrack)
{
    ASSERT (ap->track() == newTrack);

    LOG_MEDIA_MESSAGE("%s", newTrack->id().string().latin1().data());

    if (!oldTrack)
        m_playbackPipeline->attachTrack(ap->sourceBufferPrivate(), newTrack, ap->appSinkCaps());
    else
        m_playbackPipeline->reattachTrack(ap->sourceBufferPrivate(), newTrack, ap->appSinkCaps());
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
        gst_element_send_event(it->value->pipeline(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
            gst_structure_new("drm-cipher", "key", GST_TYPE_BUFFER, buffer, nullptr)));
        it->value->setAppendStage(AppendPipeline::AppendStage::Ongoing);
    }
}
#endif

#if USE(DXDRM)
void MediaPlayerPrivateGStreamerMSE::emitSession()
{
    DiscretixSession* session = dxdrmSession();
    if (!session->ready())
        return;

    for (HashMap<RefPtr<SourceBufferPrivateGStreamer>, RefPtr<AppendPipeline> >::iterator it = m_appendPipelinesMap.begin(); it != m_appendPipelinesMap.end(); ++it) {
        gst_element_send_event(it->value->pipeline(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
           gst_structure_new("dxdrm-session", "session", G_TYPE_POINTER, session, nullptr)));
        it->value->setAppendStage(AppendPipeline::AppendStage::Ongoing);
    }
}
#endif

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
    GstSample* sample = gst_sample_new(0, gst_caps_ref(caps), 0, 0);
    GStreamerMediaSample* s = new GStreamerMediaSample(sample, presentationSize, trackID);
    s->m_pts = pts;
    s->m_dts = dts;
    s->m_duration = duration;
    s->m_flags = MediaSample::NonDisplaying;
    return adoptRef(s);
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
        g_mutex_init(&m_mutex);
        g_cond_init(&m_condition);
    }
    virtual ~PadInfo()
    {
        g_cond_signal(&m_condition);
        g_cond_clear(&m_condition);
        g_mutex_clear(&m_mutex);
        gst_object_unref(m_demuxerSrcPad);
    }

    void lock()
    {
        g_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        g_mutex_unlock(&m_mutex);
    }

    void wait()
    {
        g_cond_wait(&m_condition, &m_mutex);
    }

    void signal()
    {
        g_cond_signal(&m_condition);
    }

    GstPad* demuxerSrcPad() { return m_demuxerSrcPad; }
    RefPtr<AppendPipeline> ap() { return m_ap; }

private:
    GstPad* m_demuxerSrcPad;
    RefPtr<AppendPipeline> m_ap;
    GMutex m_mutex;
    GCond m_condition;
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
    case AppendPipeline::AppendStage::NoDataToDecode:
        return "NoDataToDecode";
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

static void appendPipelineDemuxerPadAdded(GstElement*, GstPad*, AppendPipeline*);
static void appendPipelineDemuxerPadRemoved(GstElement*, GstPad*, AppendPipeline*);
static gboolean appendPipelineDemuxerConnectToAppSinkMainThread(PadInfo*);
static gboolean appendPipelineDemuxerDisconnectFromAppSinkMainThread(PadInfo*);
static void appendPipelineAppSinkCapsChanged(GObject*, GParamSpec*, AppendPipeline*);
static GstFlowReturn appendPipelineAppSinkNewSample(GstElement*, AppendPipeline*);
static gboolean appendPipelineAppSinkNewSampleMainThread(NewSampleInfo*);
static void appendPipelineAppSinkEOS(GstElement*, AppendPipeline*);
static gboolean appendPipelineAppSinkEOSMainThread(AppendPipeline* ap);
static gboolean appendPipelineNoDataToDecodeTimeout(AppendPipeline* ap);
static gboolean appendPipelineLastSampleTimeout(AppendPipeline* ap);

static void appendPipelineElementMessageCallback(GstBus*, GstMessage* message, AppendPipeline* ap)
{
    ap->handleElementMessage(message);
}

AppendPipeline::AppendPipeline(PassRefPtr<MediaSourceClientGStreamerMSE> mediaSourceClient, PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, MediaPlayerPrivateGStreamerMSE* playerPrivate)
    : m_mediaSourceClient(mediaSourceClient)
    , m_sourceBufferPrivate(sourceBufferPrivate)
    , m_playerPrivate(playerPrivate)
    , m_id(0)
    , m_appSinkCaps(NULL)
    , m_demuxerSrcPadCaps(NULL)
    , m_noDataToDecodeTimeoutTag(0)
    , m_lastSampleTimeoutTag(0)
    , m_appendStage(NotStarted)
    , m_abortPending(false)
    , m_streamType(Unknown)
{
    ASSERT(WTF::isMainThread());

    LOG_MEDIA_MESSAGE("%p", this);

    // TODO: give a name to the pipeline, maybe related with the track it's managing.
    // The track name is still unknown at this time, though.
    m_pipeline = gst_pipeline_new(NULL);

    GRefPtr<GstBus> bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline)));
    gst_bus_enable_sync_message_emission(bus.get());
    g_signal_connect(bus.get(), "sync-message::element", G_CALLBACK(appendPipelineElementMessageCallback), this);

    g_mutex_init(&m_newSampleMutex);
    g_cond_init(&m_newSampleCondition);

    m_decryptor = NULL;
    m_appsrc = gst_element_factory_make("appsrc", NULL);
    m_typefind = gst_element_factory_make("typefind", NULL);
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

    GRefPtr<GstPad> appSinkPad = gst_element_get_static_pad(m_appsink, "sink");
    g_signal_connect(appSinkPad.get(), "notify::caps", G_CALLBACK(appendPipelineAppSinkCapsChanged), this);

    // These signals won't be connected outside of the lifetime of "this".
    g_signal_connect(m_qtdemux, "pad-added", G_CALLBACK(appendPipelineDemuxerPadAdded), this);
    g_signal_connect(m_qtdemux, "pad-removed", G_CALLBACK(appendPipelineDemuxerPadRemoved), this);
    g_signal_connect(m_appsink, "new-sample", G_CALLBACK(appendPipelineAppSinkNewSample), this);
    g_signal_connect(m_appsink, "eos", G_CALLBACK(appendPipelineAppSinkEOS), this);

    // Add_many will take ownership of a reference. Request one ref more for ourselves.
    gst_object_ref(m_appsrc);
    gst_object_ref(m_typefind);
    gst_object_ref(m_qtdemux);
    gst_object_ref(m_appsink);

    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, m_typefind, m_qtdemux, NULL);
    gst_element_link_many(m_appsrc, m_typefind, m_qtdemux, NULL);

    gst_element_set_state(m_pipeline, GST_STATE_READY);
};

AppendPipeline::~AppendPipeline()
{
    ASSERT(WTF::isMainThread());

    g_cond_signal(&m_newSampleCondition);
    g_cond_clear(&m_newSampleCondition);
    g_mutex_clear(&m_newSampleMutex);

    LOG_MEDIA_MESSAGE("%p", this);
    if (m_noDataToDecodeTimeoutTag) {
        LOG_MEDIA_MESSAGE("m_noDataToDecodeTimeoutTag=%u", m_noDataToDecodeTimeoutTag);
        // TODO: Maybe notify appendComplete here?
        g_source_remove(m_noDataToDecodeTimeoutTag);
        m_noDataToDecodeTimeoutTag = 0;
    }

    if (m_lastSampleTimeoutTag) {
        LOG_MEDIA_MESSAGE("m_lastSampleTimeoutTag=%u", m_lastSampleTimeoutTag);
        // TODO: Maybe notify appendComplete here?
        g_source_remove(m_lastSampleTimeoutTag);
        m_lastSampleTimeoutTag = 0;
    }

    if (m_pipeline) {
        GRefPtr<GstBus> bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline)));
        ASSERT(bus);
        g_signal_handlers_disconnect_by_func(bus.get(), reinterpret_cast<gpointer>(appendPipelineElementMessageCallback), this);
        gst_bus_disable_sync_message_emission(bus.get());

        gst_element_set_state (m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = NULL;
    }

    if (m_appsrc) {
        gst_object_unref(m_appsrc);
        m_appsrc = NULL;
    }

    if (m_typefind) {
        gst_object_unref(m_typefind);
        m_typefind = NULL;
    }

    if (m_qtdemux) {
        g_signal_handlers_disconnect_by_func(m_qtdemux, (gpointer)appendPipelineDemuxerPadAdded, this);
        g_signal_handlers_disconnect_by_func(m_qtdemux, (gpointer)appendPipelineDemuxerPadRemoved, this);

        gst_object_unref(m_qtdemux);
        m_qtdemux = NULL;
    }

    // Should have been freed by disconnectFromAppSink()
    ASSERT(!m_decryptor);

    if (m_appsink) {
        GRefPtr<GstPad> appSinkPad = gst_element_get_static_pad(m_appsink, "sink");
        g_signal_handlers_disconnect_by_func(appSinkPad.get(), (gpointer)appendPipelineAppSinkCapsChanged, this);

        g_signal_handlers_disconnect_by_func(m_appsink, (gpointer)appendPipelineAppSinkNewSample, this);
        g_signal_handlers_disconnect_by_func(m_appsink, (gpointer)appendPipelineAppSinkEOS, this);

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

    m_playerPrivate = nullptr;
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

    // And now that no handleNewSample operations will remain stalled waiting
    // for the main thread, stop the pipeline.
    if (m_pipeline)
        gst_element_set_state (m_pipeline, GST_STATE_NULL);

    m_playerPrivate = nullptr;
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
    // NotStarted-->Ongoing-->NoDataToDecode-->NotStarted
    //           |         |                `->Aborting-->NotStarted
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
        ASSERT(m_noDataToDecodeTimeoutTag == 0);
        ASSERT(m_lastSampleTimeoutTag == 0);
        switch (newAppendStage) {
        case Ongoing:
            ok = true;
            gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
            m_noDataToDecodeTimeoutTag = g_timeout_add(s_noDataToDecodeTimeoutMsec, GSourceFunc(appendPipelineNoDataToDecodeTimeout), this);
            break;
        case NotStarted:
            ok = true;
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
        ASSERT(m_noDataToDecodeTimeoutTag == 0);
        ASSERT(m_lastSampleTimeoutTag == 0);
        switch (newAppendStage) {
        case Ongoing:
            ok = true;
            m_noDataToDecodeTimeoutTag = g_timeout_add(s_noDataToDecodeTimeoutMsec, GSourceFunc(appendPipelineNoDataToDecodeTimeout), this);
            break;
        case Invalid:
            ok = true;
            if (m_noDataToDecodeTimeoutTag) {
                g_source_remove(m_noDataToDecodeTimeoutTag);
                m_noDataToDecodeTimeoutTag = 0;
            }
            if (m_lastSampleTimeoutTag) {
                g_source_remove(m_lastSampleTimeoutTag);
                m_lastSampleTimeoutTag = 0;
            }
            break;
        default:
            break;
        }
        break;
    case Ongoing:
        ASSERT(m_noDataToDecodeTimeoutTag != 0);
        ASSERT(m_lastSampleTimeoutTag == 0);
        switch (newAppendStage) {
        case KeyNegotiation:
            ok = true;
            if (m_noDataToDecodeTimeoutTag) {
                g_source_remove(m_noDataToDecodeTimeoutTag);
                m_noDataToDecodeTimeoutTag = 0;
            }
            break;
        case NoDataToDecode:
            ok = true;
            if (m_noDataToDecodeTimeoutTag) {
                g_source_remove(m_noDataToDecodeTimeoutTag);
                m_noDataToDecodeTimeoutTag = 0;
            }
            m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
            if (m_abortPending)
                nextAppendStage = Aborting;
            else
                nextAppendStage = NotStarted;
            break;
        case Sampling:
            ok = true;
            if (m_noDataToDecodeTimeoutTag) {
                g_source_remove(m_noDataToDecodeTimeoutTag);
                m_noDataToDecodeTimeoutTag = 0;
            }

            if (m_lastSampleTimeoutTag) {
                TRACE_MEDIA_MESSAGE("lastSampleTimeoutTag already exists while transitioning Ongoing-->Sampling");
                g_source_remove(m_lastSampleTimeoutTag);
                m_lastSampleTimeoutTag = 0;
            }
            m_lastSampleTimeoutTag = g_timeout_add(s_lastSampleTimeoutMsec, GSourceFunc(appendPipelineLastSampleTimeout), this);
            break;
        case Invalid:
            ok = true;
            if (m_noDataToDecodeTimeoutTag) {
                g_source_remove(m_noDataToDecodeTimeoutTag);
                m_noDataToDecodeTimeoutTag = 0;
            }
            if (m_lastSampleTimeoutTag) {
                g_source_remove(m_lastSampleTimeoutTag);
                m_lastSampleTimeoutTag = 0;
            }
            break;
        default:
            break;
        }
        break;
    case NoDataToDecode:
        ASSERT(m_noDataToDecodeTimeoutTag == 0);
        ASSERT(m_lastSampleTimeoutTag == 0);
        switch (newAppendStage) {
        case NotStarted:
            ok = true;
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
    case Sampling:
        ASSERT(m_noDataToDecodeTimeoutTag == 0);
        ASSERT(m_lastSampleTimeoutTag != 0);
        switch (newAppendStage) {
        case Sampling:
            ok = true;
            if (m_lastSampleTimeoutTag) {
                g_source_remove(m_lastSampleTimeoutTag);
                m_lastSampleTimeoutTag = 0;
            }
            m_lastSampleTimeoutTag = g_timeout_add(s_lastSampleTimeoutMsec, GSourceFunc(appendPipelineLastSampleTimeout), this);
            break;
        case LastSample:
            ok = true;
            if (m_lastSampleTimeoutTag) {
                g_source_remove(m_lastSampleTimeoutTag);
                m_lastSampleTimeoutTag = 0;
            }
            m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
            if (m_abortPending)
                nextAppendStage = Aborting;
            else
                nextAppendStage = NotStarted;
            break;
        case Invalid:
            ok = true;
            if (m_lastSampleTimeoutTag) {
                g_source_remove(m_lastSampleTimeoutTag);
                m_lastSampleTimeoutTag = 0;
            }
            break;
        default:
            break;
        }
        break;
    case LastSample:
        ASSERT(m_noDataToDecodeTimeoutTag == 0);
        ASSERT(m_lastSampleTimeoutTag == 0);
        switch (newAppendStage) {
        case NotStarted:
            ok = true;
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
    case Aborting:
        ASSERT(m_noDataToDecodeTimeoutTag == 0);
        ASSERT(m_lastSampleTimeoutTag == 0);
        switch (newAppendStage) {
        case NotStarted:
            ok = true;
            resetPipeline();
            m_abortPending = false;
            break;
        case Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case Invalid:
        ASSERT(m_noDataToDecodeTimeoutTag == 0);
        ASSERT(m_lastSampleTimeoutTag == 0);
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

    GRefPtr<GstPad> pad = gst_element_get_static_pad(m_appsink, "sink");
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
}

void AppendPipeline::appSinkNewSample(GstSample* sample)
{
    ASSERT(WTF::isMainThread());

    g_mutex_lock(&m_newSampleMutex);

    // Ignore samples if we're not expecting them. Refuse processing if we're in Invalid state.
    if (!(m_appendStage == Ongoing || m_appendStage == Sampling)) {
        LOG_MEDIA_MESSAGE("Unexpected sample, stage=%s", dumpAppendStage(m_appendStage));
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

    MediaTime timestampOffset(MediaTime::createWithDouble(m_sourceBufferPrivate->timestampOffset()));

    // Add a fake sample if a gap is detected before the first sample
    if (mediaSample->presentationTime() >= timestampOffset &&
        mediaSample->presentationTime() <= timestampOffset + MediaTime::createWithDouble(0.1)) {
        LOG_MEDIA_MESSAGE("Adding fake sample");
        RefPtr<WebCore::GStreamerMediaSample> fakeSample = WebCore::GStreamerMediaSample::createFakeSample(
                gst_sample_get_caps(sample), timestampOffset, mediaSample->decodeTime(), mediaSample->presentationTime() - timestampOffset, mediaSample->presentationSize(),
                mediaSample->trackID());
        m_sourceBufferPrivate->didReceiveSample(fakeSample);
    }

    m_sourceBufferPrivate->didReceiveSample(mediaSample);
    setAppendStage(Sampling);
    m_flowReturn = GST_FLOW_OK;
    g_cond_signal(&m_newSampleCondition);
    g_mutex_unlock(&m_newSampleMutex);
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
        setAppendStage(NoDataToDecode);
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
    gst_element_set_state(m_pipeline, GST_STATE_READY);
    gst_element_get_state(m_pipeline, NULL, NULL, 0);

    {
        static int i = 0;
        WTF::String  dotFileName = String::format("reset-pipeline-%d", ++i);
        gst_debug_bin_to_dot_file(GST_BIN(m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.utf8().data());
    }

}

void AppendPipeline::abort()
{
    ASSERT(WTF::isMainThread());
    LOG_MEDIA_MESSAGE("aborting");

    // Abort already ongoing.
    if (m_abortPending)
        return;

    m_abortPending = true;
    if (m_appendStage == NotStarted)
        setAppendStage(Aborting);
    // Else, the automatic stage transitions will take care when the ongoing append finishes.
}


GstFlowReturn AppendPipeline::handleNewSample(GstElement* appsink)
{
    TRACE_MEDIA_MESSAGE("thread %d", WTF::currentThread());

    bool invalid;

    g_mutex_lock(&m_newSampleMutex);
    invalid = !m_playerPrivate || m_appendStage == Invalid;
    g_mutex_unlock(&m_newSampleMutex);

    if (invalid) {
        LOG_MEDIA_MESSAGE("AppendPipeline has been disabled, ignoring this sample");
        return GST_FLOW_ERROR;
    }

    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));

    if (WTF::isMainThread()) {
        appSinkNewSample(sample);
    } else {
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
    }
    gst_sample_unref(sample);
    return m_flowReturn;
}

void AppendPipeline::connectToAppSinkFromAnyThread(GstPad* demuxerSrcPad)
{
    if (!m_appsink)
        return;

    LOG_MEDIA_MESSAGE("connecting to appsink");

    GRefPtr<GstPad> sinkSinkPad = gst_element_get_static_pad(m_appsink, "sink");

    // Only one Stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(sinkSinkPad.get()));

    if (WTF::isMainThread()) {
        connectToAppSink(demuxerSrcPad);
    } else {
        // Call connectToAppSink() in the main thread and wait.
        PadInfo* info = new PadInfo(demuxerSrcPad, this);
        info->lock();
        g_timeout_add(0, GSourceFunc(appendPipelineDemuxerConnectToAppSinkMainThread), info);
        info->wait();
        info->unlock();
        delete info;
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
            GRefPtr<GstPad> decryptorSrcPad = gst_element_get_static_pad(m_decryptor, "src");
            GRefPtr<GstPad> decryptorSinkPad = gst_element_get_static_pad(m_decryptor, "sink");
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

    GRefPtr<GstPad> sinkSinkPad = gst_element_get_static_pad(m_appsink, "sink");

    // Only one Stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(sinkSinkPad.get()));

    // TODO: Use RefPtr
    GstCaps* caps = gst_pad_get_current_caps(GST_PAD(demuxerSrcPad));

    {
        gchar* strcaps = gst_caps_to_string(caps);
        LOG_MEDIA_MESSAGE("%s", strcaps);
        g_free(strcaps);
    }

    if (!caps)
        return;

    m_oldTrack = m_track;

    // May create m_decryptor
    parseDemuxerCaps(gst_caps_ref(caps));

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
        // No useful data, but notify anyway to complete the append operation (webKitMediaSrcLastSampleTimeout is cancelled and won't notify in this case)
        LOG_MEDIA_MESSAGE("(no data)");
        m_mediaSourceClient->didReceiveAllPendingSamples(m_sourceBufferPrivate.get());
        break;
    }

    gst_caps_unref(caps);
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
        PadInfo* info = new PadInfo(NULL, this);
        info->lock();
        g_timeout_add(0, GSourceFunc(appendPipelineDemuxerDisconnectFromAppSinkMainThread), info);
        info->wait();
        info->unlock();
        delete info;
    }
}

void AppendPipeline::disconnectFromAppSink()
{
    ASSERT(WTF::isMainThread());

    if (m_decryptor) {
        LOG_MEDIA_MESSAGE("Releasing decryptor");
        gst_object_unref(m_decryptor);
        m_decryptor = NULL;
    }
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

static void appendPipelineDemuxerPadAdded(GstElement*, GstPad* demuxerSrcPad, AppendPipeline* ap)
{
    ap->connectToAppSinkFromAnyThread(demuxerSrcPad);
}

static gboolean appendPipelineDemuxerConnectToAppSinkMainThread(PadInfo* info)
{
    info->lock();
    info->ap()->connectToAppSink(info->demuxerSrcPad());
    info->signal();
    info->unlock();
    return G_SOURCE_REMOVE;
}

static gboolean appendPipelineDemuxerDisconnectFromAppSinkMainThread(PadInfo* info)
{
    info->lock();
    info->ap()->disconnectFromAppSink();
    info->signal();
    info->unlock();
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

static gboolean appendPipelineNoDataToDecodeTimeout(AppendPipeline* ap)
{
    TRACE_MEDIA_MESSAGE("no data timeout fired");
    if (ap->appendStage()==AppendPipeline::AppendStage::Invalid)
        return G_SOURCE_REMOVE;

    ap->setAppendStage(AppendPipeline::NoDataToDecode);
    return G_SOURCE_REMOVE;
}

static gboolean appendPipelineLastSampleTimeout(AppendPipeline* ap)
{
    TRACE_MEDIA_MESSAGE("last sample timer fired");
    if (ap->appendStage()==AppendPipeline::AppendStage::Invalid)
        return G_SOURCE_REMOVE;

    ap->setAppendStage(AppendPipeline::LastSample);
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

    // TODO: cancel m_noDataToDecodeTimeoutTag if active and perform appendComplete()
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

MediaTime MediaSourceClientGStreamerMSE::duration()
{
    ASSERT(WTF::isMainThread());

    return m_duration;
}

void MediaSourceClientGStreamerMSE::durationChanged(const MediaTime& duration)
{
    ASSERT(WTF::isMainThread());

    LOG_MEDIA_MESSAGE("duration=%f", duration.toFloat());

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

    TRACE_MEDIA_MESSAGE("%u bytes", length);

    if (!m_playerPrivate)
        return false;

    RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate = prpSourceBufferPrivate;
    RefPtr<AppendPipeline> ap = m_playerPrivate->m_appendPipelinesMap.get(sourceBufferPrivate);
    GstBuffer* buffer = gst_buffer_new_and_alloc(length);
    gst_buffer_fill(buffer, 0, data, length);
    ap->setAppendStage(AppendPipeline::Ongoing);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(ap->appsrc()), buffer);
    return (ret == GST_FLOW_OK);
}

void MediaSourceClientGStreamerMSE::markEndOfStream(MediaSourcePrivate::EndOfStreamStatus)
{
    ASSERT(WTF::isMainThread());

    // TODO
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

    TRACE_MEDIA_MESSAGE("received all pending samples");
    sourceBuffer->didReceiveAllPendingSamples();
}

GRefPtr<WebKitMediaSrc> MediaSourceClientGStreamerMSE::webKitMediaSrc()
{
    ASSERT(WTF::isMainThread());

    if (!m_playerPrivate)
        return GRefPtr<WebKitMediaSrc>(NULL);

    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(m_playerPrivate->m_webKitMediaSrc.get());

    ASSERT(WEBKIT_IS_MEDIA_SRC(source));

    return GRefPtr<WebKitMediaSrc>(source);
}

void MediaSourceClientGStreamerMSE::clearPlayerPrivate()
{
    ASSERT(WTF::isMainThread());

    m_playerPrivate = nullptr;
}

} // namespace WebCore

#endif // USE(GSTREAMER)
