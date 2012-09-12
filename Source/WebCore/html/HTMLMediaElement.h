/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLMediaElement_h
#define HTMLMediaElement_h

#if ENABLE(VIDEO)

#include "HTMLElement.h"
#include "ActiveDOMObject.h"
#include "GenericEventQueue.h"
#include "MediaCanStartListener.h"
#include "MediaControllerInterface.h"
#include "MediaPlayer.h"

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "MediaPlayerProxy.h"
#endif

#if ENABLE(VIDEO_TRACK)
#include "PODIntervalTree.h"
#include "TextTrack.h"
#include "TextTrackCue.h"
#endif

namespace WebCore {

#if ENABLE(WEB_AUDIO)
class AudioSourceProvider;
class MediaElementAudioSourceNode;
#endif
class Event;
class HTMLSourceElement;
class HTMLTrackElement;
class MediaController;
class MediaControls;
class MediaError;
class KURL;
class TextTrackList;
class TimeRanges;
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
class Widget;
#endif
#if PLATFORM(MAC)
class DisplaySleepDisabler;
#endif

#if ENABLE(VIDEO_TRACK)
typedef PODIntervalTree<double, TextTrackCue*> CueIntervalTree;
typedef Vector<CueIntervalTree::IntervalType> CueList;
#endif

// FIXME: The inheritance from MediaPlayerClient here should be private inheritance.
// But it can't be until the Chromium WebMediaPlayerClientImpl class is fixed so it
// no longer depends on typecasting a MediaPlayerClient to an HTMLMediaElement.

class HTMLMediaElement : public HTMLElement, public MediaPlayerClient, public MediaPlayerSupportsTypeClient, private MediaCanStartListener, public ActiveDOMObject, public MediaControllerInterface
#if ENABLE(VIDEO_TRACK)
    , private TextTrackClient
#endif
{
public:
    MediaPlayer* player() const { return m_player.get(); }

    void createShadowSubtree();
    virtual void willAddAuthorShadowRoot() OVERRIDE;
    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    virtual bool isVideo() const = 0;
    virtual bool hasVideo() const { return false; }
    virtual bool hasAudio() const;

    void rewind(float timeDelta);
    void returnToRealtime();

    // Eventually overloaded in HTMLVideoElement
    virtual bool supportsFullscreen() const { return false; };

    virtual bool supportsSave() const;
    virtual bool supportsScanning() const;
    
    PlatformMedia platformMedia() const;
#if USE(ACCELERATED_COMPOSITING)
    PlatformLayer* platformLayer() const;
#endif

    enum LoadType {
        MediaResource = 1 << 0,
        TextTrackResource = 1 << 1
    };
    void scheduleLoad(LoadType);
    
    MediaPlayer::MovieLoadType movieLoadType() const;
    
    bool inActiveDocument() const { return m_inActiveDocument; }
    
// DOM API
// error state
    PassRefPtr<MediaError> error() const;

// network state
    void setSrc(const String&);
    const KURL& currentSrc() const { return m_currentSrc; }

    enum NetworkState { NETWORK_EMPTY, NETWORK_IDLE, NETWORK_LOADING, NETWORK_NO_SOURCE };
    NetworkState networkState() const;

    String preload() const;    
    void setPreload(const String&);

    PassRefPtr<TimeRanges> buffered() const;
    void load(ExceptionCode&);
    String canPlayType(const String& mimeType, const String& keySystem = String(), const KURL& = KURL()) const;

// ready state
    ReadyState readyState() const;
    bool seeking() const;

// playback state
    float currentTime() const;
    void setCurrentTime(float, ExceptionCode&);
    double initialTime() const;
    float startTime() const;
    float duration() const;
    bool paused() const;
    float defaultPlaybackRate() const;
    void setDefaultPlaybackRate(float);
    float playbackRate() const;
    void setPlaybackRate(float);
    void updatePlaybackRate();
    bool webkitPreservesPitch() const;
    void setWebkitPreservesPitch(bool);
    PassRefPtr<TimeRanges> played();
    PassRefPtr<TimeRanges> seekable() const;
    bool ended() const;
    bool autoplay() const;    
    void setAutoplay(bool b);
    bool loop() const;    
    void setLoop(bool b);
    void play();
    void pause();

// captions
    bool webkitHasClosedCaptions() const;
    bool webkitClosedCaptionsVisible() const;
    void setWebkitClosedCaptionsVisible(bool);

#if ENABLE(MEDIA_STATISTICS)
// Statistics
    unsigned webkitAudioDecodedByteCount() const;
    unsigned webkitVideoDecodedByteCount() const;
#endif

#if ENABLE(MEDIA_SOURCE)
//  Media Source.
    void setSourceState(const String&);
#endif 

#if ENABLE(ENCRYPTED_MEDIA)
    void webkitGenerateKeyRequest(const String& keySystem, PassRefPtr<Uint8Array> initData, ExceptionCode&);
    void webkitGenerateKeyRequest(const String& keySystem, ExceptionCode&);
    void webkitAddKey(const String& keySystem, PassRefPtr<Uint8Array> key, PassRefPtr<Uint8Array> initData, const String& sessionId, ExceptionCode&);
    void webkitAddKey(const String& keySystem, PassRefPtr<Uint8Array> key, ExceptionCode&);
    void webkitCancelKeyRequest(const String& keySystem, const String& sessionId, ExceptionCode&);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitkeyadded);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitkeyerror);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitkeymessage);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitneedkey);
#endif

// controls
    bool controls() const;
    void setControls(bool);
    float volume() const;
    void setVolume(float, ExceptionCode&);
    bool muted() const;
    void setMuted(bool);

    void togglePlayState();
    void beginScrubbing();
    void endScrubbing();
    
    bool canPlay() const;

    float percentLoaded() const;

#if ENABLE(VIDEO_TRACK)
    PassRefPtr<TextTrack> addTextTrack(const String& kind, const String& label, const String& language, ExceptionCode&);
    PassRefPtr<TextTrack> addTextTrack(const String& kind, const String& label, ExceptionCode& ec) { return addTextTrack(kind, label, emptyString(), ec); }
    PassRefPtr<TextTrack> addTextTrack(const String& kind, ExceptionCode& ec) { return addTextTrack(kind, emptyString(), emptyString(), ec); }

    TextTrackList* textTracks();
    CueList currentlyActiveCues() const { return m_currentlyActiveCues; }

    virtual void didAddTrack(HTMLTrackElement*);
    virtual void willRemoveTrack(HTMLTrackElement*);

    struct TrackGroup {
        enum GroupKind { CaptionsAndSubtitles, Description, Chapter, Metadata, Other };

        TrackGroup(GroupKind kind)
            : visibleTrack(0)
            , defaultTrack(0)
            , kind(kind)
            , hasSrcLang(false)
        {
        }

        Vector<HTMLTrackElement*> tracks;
        HTMLTrackElement* visibleTrack;
        HTMLTrackElement* defaultTrack;
        GroupKind kind;
        bool hasSrcLang;
    };

    void configureTextTrackGroupForLanguage(const TrackGroup&) const;
    void configureTextTracks();
    void configureTextTrackGroup(const TrackGroup&) const;

    bool userIsInterestedInThisTrackKind(String) const;
    bool textTracksAreReady() const;
    void configureTextTrackDisplay();
    void updateClosedCaptionsControls();

    // TextTrackClient
    virtual void textTrackReadyStateChanged(TextTrack*);
    virtual void textTrackKindChanged(TextTrack*);
    virtual void textTrackModeChanged(TextTrack*);
    virtual void textTrackAddCues(TextTrack*, const TextTrackCueList*);
    virtual void textTrackRemoveCues(TextTrack*, const TextTrackCueList*);
    virtual void textTrackAddCue(TextTrack*, PassRefPtr<TextTrackCue>);
    virtual void textTrackRemoveCue(TextTrack*, PassRefPtr<TextTrackCue>);
#endif

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    void allocateMediaPlayerIfNecessary();
    void setNeedWidgetUpdate(bool needWidgetUpdate) { m_needWidgetUpdate = needWidgetUpdate; }
    void deliverNotification(MediaPlayerProxyNotificationType notification);
    void setMediaPlayerProxy(WebMediaPlayerProxy* proxy);
    void getPluginProxyParams(KURL& url, Vector<String>& names, Vector<String>& values);
    void createMediaPlayerProxy();
    void updateWidget(PluginCreationOption);
#endif

    // EventTarget function.
    // Both Node (via HTMLElement) and ActiveDOMObject define this method, which
    // causes an ambiguity error at compile time. This class's constructor
    // ensures that both implementations return document, so return the result
    // of one of them here.
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE { return HTMLElement::scriptExecutionContext(); }

    bool hasSingleSecurityOrigin() const { return !m_player || m_player->hasSingleSecurityOrigin(); }
    
    bool isFullscreen() const;
    void enterFullscreen();
    void exitFullscreen();

    bool hasClosedCaptions() const;
    bool closedCaptionsVisible() const;
    void setClosedCaptionsVisible(bool);

    MediaControls* mediaControls() const;

    void sourceWasRemoved(HTMLSourceElement*);
    void sourceWasAdded(HTMLSourceElement*);

    void privateBrowsingStateDidChange();

    // Media cache management.
    static void getSitesInMediaCache(Vector<String>&);
    static void clearMediaCache();
    static void clearMediaCacheForSite(const String&);

    bool isPlaying() const { return m_playing; }

    virtual bool hasPendingActivity() const;

#if ENABLE(WEB_AUDIO)
    MediaElementAudioSourceNode* audioSourceNode() { return m_audioSourceNode; }
    void setAudioSourceNode(MediaElementAudioSourceNode*);

    AudioSourceProvider* audioSourceProvider();
#endif

    enum InvalidURLAction { DoNothing, Complain };
    bool isSafeToLoadURL(const KURL&, InvalidURLAction);

    const String& mediaGroup() const;
    void setMediaGroup(const String&);

    MediaController* controller() const;
    void setController(PassRefPtr<MediaController>);

    virtual bool dispatchEvent(PassRefPtr<Event>);

    virtual bool willRespondToMouseClickEvents() OVERRIDE;

protected:
    HTMLMediaElement(const QualifiedName&, Document*, bool);
    virtual ~HTMLMediaElement();

    virtual void parseAttribute(const Attribute&) OVERRIDE;
    virtual void finishParsingChildren();
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;
    virtual void attach();

    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

    enum DisplayMode { Unknown, None, Poster, PosterWaitingForVideo, Video };
    DisplayMode displayMode() const { return m_displayMode; }
    virtual void setDisplayMode(DisplayMode mode) { m_displayMode = mode; }
    
    virtual bool isMediaElement() const { return true; }

    // Restrictions to change default behaviors.
    enum BehaviorRestrictionFlags {
        NoRestrictions = 0,
        RequireUserGestureForLoadRestriction = 1 << 0,
        RequireUserGestureForRateChangeRestriction = 1 << 1,
        RequireUserGestureForFullscreenRestriction = 1 << 2,
        RequirePageConsentToLoadMediaRestriction = 1 << 3,
    };
    typedef unsigned BehaviorRestrictions;
    
    bool userGestureRequiredForLoad() const { return m_restrictions & RequireUserGestureForLoadRestriction; }
    bool userGestureRequiredForRateChange() const { return m_restrictions & RequireUserGestureForRateChangeRestriction; }
    bool userGestureRequiredForFullscreen() const { return m_restrictions & RequireUserGestureForFullscreenRestriction; }
    bool pageConsentRequiredForLoad() const { return m_restrictions & RequirePageConsentToLoadMediaRestriction; }
    
    void addBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions |= restriction; }
    void removeBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions &= ~restriction; }
    
private:
    void createMediaPlayer();

    virtual bool hasCustomFocusLogic() const OVERRIDE;
    virtual bool supportsFocus() const;
    virtual bool isMouseFocusable() const;
    virtual bool rendererIsNeeded(const NodeRenderingContext&);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual bool childShouldCreateRenderer(const NodeRenderingContext&) const OVERRIDE;
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual void didRecalcStyle(StyleChange);
    
    virtual void defaultEventHandler(Event*);

    virtual void didBecomeFullscreenElement();
    virtual void willStopBeingFullscreenElement();

    // ActiveDOMObject functions.
    virtual bool canSuspend() const;
    virtual void suspend(ReasonForSuspension);
    virtual void resume();
    virtual void stop();
    
    virtual void mediaVolumeDidChange();

    virtual void updateDisplayState() { }
    
    void setReadyState(MediaPlayer::ReadyState);
    void setNetworkState(MediaPlayer::NetworkState);

    virtual Document* mediaPlayerOwningDocument();
    virtual void mediaPlayerNetworkStateChanged(MediaPlayer*);
    virtual void mediaPlayerReadyStateChanged(MediaPlayer*);
    virtual void mediaPlayerTimeChanged(MediaPlayer*);
    virtual void mediaPlayerVolumeChanged(MediaPlayer*);
    virtual void mediaPlayerMuteChanged(MediaPlayer*);
    virtual void mediaPlayerDurationChanged(MediaPlayer*);
    virtual void mediaPlayerRateChanged(MediaPlayer*);
    virtual void mediaPlayerPlaybackStateChanged(MediaPlayer*);
    virtual void mediaPlayerSawUnsupportedTracks(MediaPlayer*);
    virtual void mediaPlayerResourceNotSupported(MediaPlayer*);
    virtual void mediaPlayerRepaint(MediaPlayer*);
    virtual void mediaPlayerSizeChanged(MediaPlayer*);
#if USE(ACCELERATED_COMPOSITING)
    virtual bool mediaPlayerRenderingCanBeAccelerated(MediaPlayer*);
    virtual void mediaPlayerRenderingModeChanged(MediaPlayer*);
#endif
    virtual void mediaPlayerEngineUpdated(MediaPlayer*);
    
    virtual void mediaPlayerFirstVideoFrameAvailable(MediaPlayer*);
    virtual void mediaPlayerCharacteristicChanged(MediaPlayer*);

#if ENABLE(MEDIA_SOURCE)
    virtual void mediaPlayerSourceOpened();
    virtual String mediaPlayerSourceURL() const;
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    virtual void mediaPlayerKeyAdded(MediaPlayer*, const String& keySystem, const String& sessionId) OVERRIDE;
    virtual void mediaPlayerKeyError(MediaPlayer*, const String& keySystem, const String& sessionId, MediaPlayerClient::MediaKeyErrorCode, unsigned short systemCode) OVERRIDE;
    virtual void mediaPlayerKeyMessage(MediaPlayer*, const String& keySystem, const String& sessionId, const unsigned char* message, unsigned messageLength) OVERRIDE;
    virtual void mediaPlayerKeyNeeded(MediaPlayer*, const String& keySystem, const String& sessionId, const unsigned char* initData, unsigned initDataLength) OVERRIDE;
#endif

    virtual String mediaPlayerReferrer() const OVERRIDE;
    virtual String mediaPlayerUserAgent() const OVERRIDE;
    virtual CORSMode mediaPlayerCORSMode() const OVERRIDE;

    virtual bool mediaPlayerNeedsSiteSpecificHacks() const OVERRIDE;
    virtual String mediaPlayerDocumentHost() const OVERRIDE;

    virtual void mediaPlayerExitFullscreen() OVERRIDE;
    virtual bool mediaPlayerIsVideo() const OVERRIDE;
    virtual LayoutRect mediaPlayerContentBoxRect() const OVERRIDE;
    virtual void mediaPlayerSetSize(const IntSize&) OVERRIDE;
    virtual void mediaPlayerPause() OVERRIDE;
    virtual void mediaPlayerPlay() OVERRIDE;
    virtual bool mediaPlayerIsPaused() const OVERRIDE;
    virtual bool mediaPlayerIsLooping() const OVERRIDE;
    virtual HostWindow* mediaPlayerHostWindow() OVERRIDE;
    virtual IntRect mediaPlayerWindowClipRect() OVERRIDE;

#if PLATFORM(WIN) && USE(AVFOUNDATION)
    virtual GraphicsDeviceAdapter* mediaPlayerGraphicsDeviceAdapter(const MediaPlayer*) const OVERRIDE;
#endif

    void loadTimerFired(Timer<HTMLMediaElement>*);
    void progressEventTimerFired(Timer<HTMLMediaElement>*);
    void playbackProgressTimerFired(Timer<HTMLMediaElement>*);
    void startPlaybackProgressTimer();
    void startProgressEventTimer();
    void stopPeriodicTimers();

    void seek(float time, ExceptionCode&);
    void finishSeek();
    void checkIfSeekNeeded();
    void addPlayedRange(float start, float end);
    
    void scheduleTimeupdateEvent(bool periodicEvent);
    void scheduleEvent(const AtomicString& eventName);
    
    // loading
    void selectMediaResource();
    void loadResource(const KURL&, ContentType&, const String& keySystem);
    void scheduleNextSourceChild();
    void loadNextSourceChild();
    void userCancelledLoad();
    bool havePotentialSourceChild();
    void noneSupported();
    void mediaEngineError(PassRefPtr<MediaError> err);
    void cancelPendingEventsAndCallbacks();
    void waitForSourceChange();
    void prepareToPlay();

    KURL selectNextSourceChild(ContentType*, String* keySystem, InvalidURLAction);

    void mediaLoadingFailed(MediaPlayer::NetworkState);

#if ENABLE(VIDEO_TRACK)
    void updateActiveTextTrackCues(float);
    bool userIsInterestedInThisLanguage(const String&) const;
    HTMLTrackElement* showingTrackWithSameKind(HTMLTrackElement*) const;

    bool ignoreTrackDisplayUpdateRequests() const { return m_ignoreTrackDisplayUpdate > 0; }
    void beginIgnoringTrackDisplayUpdateRequests() { ++m_ignoreTrackDisplayUpdate; }
    void endIgnoringTrackDisplayUpdateRequests() { ASSERT(m_ignoreTrackDisplayUpdate); --m_ignoreTrackDisplayUpdate; }
#endif

    // These "internal" functions do not check user gesture restrictions.
    void loadInternal();
    void playInternal();
    void pauseInternal();

    void prepareForLoad();
    void allowVideoRendering();

    bool processingMediaPlayerCallback() const { return m_processingMediaPlayerCallback > 0; }
    void beginProcessingMediaPlayerCallback() { ++m_processingMediaPlayerCallback; }
    void endProcessingMediaPlayerCallback() { ASSERT(m_processingMediaPlayerCallback); --m_processingMediaPlayerCallback; }

    void updateVolume();
    void updatePlayState();
    bool potentiallyPlaying() const;
    bool endedPlayback() const;
    bool stoppedDueToErrors() const;
    bool pausedForUserInteraction() const;
    bool couldPlayIfEnoughData() const;

    float minTimeSeekable() const;
    float maxTimeSeekable() const;

    // Pauses playback without changing any states or generating events
    void setPausedInternal(bool);

    void setPlaybackRateInternal(float);

    virtual void mediaCanStart();

    void setShouldDelayLoadEvent(bool);
    void invalidateCachedTime();
    void refreshCachedTime() const;

    bool hasMediaControls() const;
    bool createMediaControls();
    void configureMediaControls();

    void prepareMediaFragmentURI();
    void applyMediaFragmentURI();

    virtual void* preDispatchEventHandler(Event*);

    void changeNetworkStateFromLoadingToIdle();

    void removeBehaviorsRestrictionsAfterFirstUserGesture();

#if ENABLE(MICRODATA)
    virtual String itemValueText() const;
    virtual void setItemValueText(const String&, ExceptionCode&);
#endif

    void updateMediaController();
    bool isBlocked() const;
    bool isBlockedOnMediaController() const;
    bool hasCurrentSrc() const { return !m_currentSrc.isEmpty(); }
    bool isLiveStream() const { return movieLoadType() == MediaPlayer::LiveStream; }
    bool isAutoplaying() const { return m_autoplaying; }

#if PLATFORM(MAC)
    void updateDisableSleep();
    bool shouldDisableSleep() const;
#endif

    Timer<HTMLMediaElement> m_loadTimer;
    Timer<HTMLMediaElement> m_progressEventTimer;
    Timer<HTMLMediaElement> m_playbackProgressTimer;
    RefPtr<TimeRanges> m_playedTimeRanges;
    OwnPtr<GenericEventQueue> m_asyncEventQueue;

    float m_playbackRate;
    float m_defaultPlaybackRate;
    bool m_webkitPreservesPitch;
    NetworkState m_networkState;
    ReadyState m_readyState;
    ReadyState m_readyStateMaximum;
    KURL m_currentSrc;

    RefPtr<MediaError> m_error;

    float m_volume;
    float m_lastSeekTime;
    
    unsigned m_previousProgress;
    double m_previousProgressTime;

    // The last time a timeupdate event was sent (wall clock).
    double m_lastTimeUpdateEventWallTime;

    // The last time a timeupdate event was sent in movie time.
    float m_lastTimeUpdateEventMovieTime;
    
    // Loading state.
    enum LoadState { WaitingForSource, LoadingFromSrcAttr, LoadingFromSourceElement };
    LoadState m_loadState;
    RefPtr<HTMLSourceElement> m_currentSourceNode;
    RefPtr<Node> m_nextChildNodeToConsider;

    OwnPtr<MediaPlayer> m_player;
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    RefPtr<Widget> m_proxyWidget;
#endif

    BehaviorRestrictions m_restrictions;
    
    MediaPlayer::Preload m_preload;

    DisplayMode m_displayMode;

    // Counter incremented while processing a callback from the media player, so we can avoid
    // calling the media engine recursively.
    int m_processingMediaPlayerCallback;

#if ENABLE(MEDIA_SOURCE)
    KURL m_mediaSourceURL;
    RefPtr<MediaSource> m_mediaSource;
#endif

    mutable float m_cachedTime;
    mutable double m_cachedTimeWallClockUpdateTime;
    mutable double m_minimumWallClockTimeToCacheMediaTime;

    double m_fragmentStartTime;
    double m_fragmentEndTime;

    typedef unsigned PendingLoadFlags;
    PendingLoadFlags m_pendingLoadFlags;

    bool m_playing : 1;
    bool m_isWaitingUntilMediaCanStart : 1;
    bool m_shouldDelayLoadEvent : 1;
    bool m_haveFiredLoadedData : 1;
    bool m_inActiveDocument : 1;
    bool m_autoplaying : 1;
    bool m_muted : 1;
    bool m_paused : 1;
    bool m_seeking : 1;

    // data has not been loaded since sending a "stalled" event
    bool m_sentStalledEvent : 1;

    // time has not changed since sending an "ended" event
    bool m_sentEndEvent : 1;

    bool m_pausedInternal : 1;

    // Not all media engines provide enough information about a file to be able to
    // support progress events so setting m_sendProgressEvents disables them 
    bool m_sendProgressEvents : 1;

    bool m_isFullscreen : 1;
    bool m_closedCaptionsVisible : 1;

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    bool m_needWidgetUpdate : 1;
#endif

    bool m_dispatchingCanPlayEvent : 1;
    bool m_loadInitiatedByUserGesture : 1;
    bool m_completelyLoaded : 1;
    bool m_havePreparedToPlay : 1;
    bool m_parsingInProgress : 1;

#if ENABLE(VIDEO_TRACK)
    bool m_tracksAreReady : 1;
    bool m_haveVisibleTextTrack : 1;
    float m_lastTextTrackUpdateTime;

    RefPtr<TextTrackList> m_textTracks;
    Vector<RefPtr<TextTrack> > m_textTracksWhenResourceSelectionBegan;

    CueIntervalTree m_cueTree;

    CueList m_currentlyActiveCues;
    int m_ignoreTrackDisplayUpdate;
    bool m_disableCaptions;
#endif

#if ENABLE(WEB_AUDIO)
    // This is a weak reference, since m_audioSourceNode holds a reference to us.
    // The value is set just after the MediaElementAudioSourceNode is created.
    // The value is cleared in MediaElementAudioSourceNode::~MediaElementAudioSourceNode().
    MediaElementAudioSourceNode* m_audioSourceNode;
#endif

    String m_mediaGroup;
    friend class MediaController;
    RefPtr<MediaController> m_mediaController;

#if PLATFORM(MAC)
    OwnPtr<DisplaySleepDisabler> m_sleepDisabler;
#endif
};

#if ENABLE(VIDEO_TRACK)
#ifndef NDEBUG
// Template specializations required by PodIntervalTree in debug mode.
template <>
struct ValueToString<double> {
    static String string(const double value)
    {
        return String::number(value);
    }
};

template <>
struct ValueToString<TextTrackCue*> {
    static String string(TextTrackCue* const& cue)
    {
        return String::format("%p id=%s interval=%f-->%f cue=%s)", cue, cue->id().utf8().data(), cue->startTime(), cue->endTime(), cue->text().utf8().data());
    }
};
#endif
#endif

inline bool isMediaElement(Node* node)
{
    return node && node->isElementNode() && toElement(node)->isMediaElement();
}

inline HTMLMediaElement* toMediaElement(Node* node)
{
    ASSERT(!node || isMediaElement(node));
    return static_cast<HTMLMediaElement*>(node);
}

} //namespace

#endif
#endif
