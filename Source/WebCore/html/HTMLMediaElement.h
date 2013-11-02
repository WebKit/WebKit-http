/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#include "HTMLPlugInImageElement.h"
#include "MediaPlayerProxy.h"
#endif

#if ENABLE(VIDEO_TRACK)
#include "AudioTrack.h"
#include "CaptionUserPreferences.h"
#include "PODIntervalTree.h"
#include "TextTrack.h"
#include "TextTrackCue.h"
#include "VideoTrack.h"
#endif



namespace WebCore {

#if USE(AUDIO_SESSION)
class AudioSessionManagerToken;
#endif
#if ENABLE(WEB_AUDIO)
class AudioSourceProvider;
class MediaElementAudioSourceNode;
#endif
class Event;
class HTMLSourceElement;
class HTMLTrackElement;
class URL;
class MediaController;
class MediaControls;
class MediaControlsHost;
class MediaError;
class PageActivityAssertionToken;
class TimeRanges;
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
class Widget;
#endif
#if PLATFORM(MAC)
class DisplaySleepDisabler;
#endif
#if ENABLE(ENCRYPTED_MEDIA_V2)
class MediaKeys;
#endif

#if ENABLE(VIDEO_TRACK)
class AudioTrackList;
class AudioTrackPrivate;
class InbandTextTrackPrivate;
class TextTrackList;
class VideoTrackList;
class VideoTrackPrivate;

typedef PODIntervalTree<double, TextTrackCue*> CueIntervalTree;
typedef CueIntervalTree::IntervalType CueInterval;
typedef Vector<CueInterval> CueList;
#endif

class HTMLMediaElement : public HTMLElement, private MediaPlayerClient, public MediaPlayerSupportsTypeClient, private MediaCanStartListener, public ActiveDOMObject, public MediaControllerInterface
#if ENABLE(VIDEO_TRACK)
    , private AudioTrackClient
    , private TextTrackClient
    , private VideoTrackClient
#endif
#if USE(PLATFORM_TEXT_TRACK_MENU)
    , public PlatformTextTrackMenuClient
#endif
{
public:
    MediaPlayer* player() const { return m_player.get(); }

    virtual bool isVideo() const { return false; }
    virtual bool hasVideo() const OVERRIDE { return false; }
    virtual bool hasAudio() const OVERRIDE;

    void rewind(double timeDelta);
    virtual void returnToRealtime() OVERRIDE;

    // Eventually overloaded in HTMLVideoElement
    virtual bool supportsFullscreen() const OVERRIDE { return false; };

    virtual bool supportsSave() const;
    virtual bool supportsScanning() const OVERRIDE;
    
    PlatformMedia platformMedia() const;
#if USE(ACCELERATED_COMPOSITING)
    PlatformLayer* platformLayer() const;
#endif

    enum DelayedActionType {
        LoadMediaResource = 1 << 0,
        ConfigureTextTracks = 1 << 1,
        TextTrackChangesNotification = 1 << 2,
        ConfigureTextTrackDisplay = 1 << 3,
    };
    void scheduleDelayedAction(DelayedActionType);
    
    MediaPlayer::MovieLoadType movieLoadType() const;
    
    bool inActiveDocument() const { return m_inActiveDocument; }
    
// DOM API
// error state
    PassRefPtr<MediaError> error() const;

// network state
    void setSrc(const String&);
    const URL& currentSrc() const { return m_currentSrc; }

    enum NetworkState { NETWORK_EMPTY, NETWORK_IDLE, NETWORK_LOADING, NETWORK_NO_SOURCE };
    NetworkState networkState() const;

    String preload() const;    
    void setPreload(const String&);

    virtual PassRefPtr<TimeRanges> buffered() const OVERRIDE;
    void load();
    String canPlayType(const String& mimeType, const String& keySystem = String(), const URL& = URL()) const;

// ready state
    virtual ReadyState readyState() const OVERRIDE;
    bool seeking() const;

// playback state
    virtual double currentTime() const OVERRIDE;
    virtual void setCurrentTime(double, ExceptionCode&) OVERRIDE;
    double initialTime() const;
    virtual double duration() const OVERRIDE;
    virtual bool paused() const OVERRIDE;
    virtual double defaultPlaybackRate() const OVERRIDE;
    virtual void setDefaultPlaybackRate(double) OVERRIDE;
    virtual double playbackRate() const OVERRIDE;
    virtual void setPlaybackRate(double) OVERRIDE;
    void updatePlaybackRate();
    bool webkitPreservesPitch() const;
    void setWebkitPreservesPitch(bool);
    virtual PassRefPtr<TimeRanges> played() OVERRIDE;
    virtual PassRefPtr<TimeRanges> seekable() const OVERRIDE;
    bool ended() const;
    bool autoplay() const;    
    void setAutoplay(bool b);
    bool loop() const;    
    void setLoop(bool b);
    virtual void play() OVERRIDE;
    virtual void pause() OVERRIDE;

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
    void closeMediaSource();
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
#endif
#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitneedkey);
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    MediaKeys* keys() const { return m_mediaKeys.get(); }
    void setMediaKeys(MediaKeys*);
#endif

// controls
    bool controls() const;
    void setControls(bool);
    virtual double volume() const OVERRIDE;
    virtual void setVolume(double, ExceptionCode&) OVERRIDE;
    virtual bool muted() const OVERRIDE;
    virtual void setMuted(bool) OVERRIDE;

    void togglePlayState();
    virtual void beginScrubbing() OVERRIDE;
    virtual void endScrubbing() OVERRIDE;
    
    virtual bool canPlay() const OVERRIDE;

    double percentLoaded() const;

#if ENABLE(VIDEO_TRACK)
    PassRefPtr<TextTrack> addTextTrack(const String& kind, const String& label, const String& language, ExceptionCode&);
    PassRefPtr<TextTrack> addTextTrack(const String& kind, const String& label, ExceptionCode& ec) { return addTextTrack(kind, label, emptyString(), ec); }
    PassRefPtr<TextTrack> addTextTrack(const String& kind, ExceptionCode& ec) { return addTextTrack(kind, emptyString(), emptyString(), ec); }

    AudioTrackList* audioTracks();
    TextTrackList* textTracks();
    VideoTrackList* videoTracks();

    CueList currentlyActiveCues() const { return m_currentlyActiveCues; }

    void addAudioTrack(PassRefPtr<AudioTrack>);
    void addTextTrack(PassRefPtr<TextTrack>);
    void addVideoTrack(PassRefPtr<VideoTrack>);
    void removeAudioTrack(AudioTrack*);
    void removeTextTrack(TextTrack*);
    void removeVideoTrack(VideoTrack*);
    void removeAllInbandTracks();
    void closeCaptionTracksChanged();
    void notifyMediaPlayerOfTextTrackChanges();

    virtual void didAddTextTrack(HTMLTrackElement*);
    virtual void didRemoveTextTrack(HTMLTrackElement*);

    virtual void mediaPlayerDidAddAudioTrack(PassRefPtr<AudioTrackPrivate>) OVERRIDE;
    virtual void mediaPlayerDidAddTextTrack(PassRefPtr<InbandTextTrackPrivate>) OVERRIDE;
    virtual void mediaPlayerDidAddVideoTrack(PassRefPtr<VideoTrackPrivate>) OVERRIDE;
    virtual void mediaPlayerDidRemoveAudioTrack(PassRefPtr<AudioTrackPrivate>) OVERRIDE;
    virtual void mediaPlayerDidRemoveTextTrack(PassRefPtr<InbandTextTrackPrivate>) OVERRIDE;
    virtual void mediaPlayerDidRemoveVideoTrack(PassRefPtr<VideoTrackPrivate>) OVERRIDE;

#if USE(PLATFORM_TEXT_TRACK_MENU)
    virtual void setSelectedTextTrack(PassRefPtr<PlatformTextTrack>) OVERRIDE;
    virtual Vector<RefPtr<PlatformTextTrack>> platformTextTracks() OVERRIDE;
    PlatformTextTrackMenuInterface* platformTextTrackMenu();
#endif

    struct TrackGroup {
        enum GroupKind { CaptionsAndSubtitles, Description, Chapter, Metadata, Other };

        TrackGroup(GroupKind kind)
            : visibleTrack(0)
            , defaultTrack(0)
            , kind(kind)
            , hasSrcLang(false)
        {
        }

        Vector<RefPtr<TextTrack>> tracks;
        RefPtr<TextTrack> visibleTrack;
        RefPtr<TextTrack> defaultTrack;
        GroupKind kind;
        bool hasSrcLang;
    };

    void configureTextTrackGroupForLanguage(const TrackGroup&) const;
    void configureTextTracks();
    void configureTextTrackGroup(const TrackGroup&);

    void setSelectedTextTrack(TextTrack*);

    bool textTracksAreReady() const;
    enum TextTrackVisibilityCheckType { CheckTextTrackVisibility, AssumeTextTrackVisibilityChanged };
    void configureTextTrackDisplay(TextTrackVisibilityCheckType checkType = CheckTextTrackVisibility);
    void updateTextTrackDisplay();

    // AudioTrackClient
    virtual void audioTrackEnabledChanged(AudioTrack*) OVERRIDE;

    // TextTrackClient
    virtual void textTrackReadyStateChanged(TextTrack*);
    virtual void textTrackKindChanged(TextTrack*) OVERRIDE;
    virtual void textTrackModeChanged(TextTrack*) OVERRIDE;
    virtual void textTrackAddCues(TextTrack*, const TextTrackCueList*) OVERRIDE;
    virtual void textTrackRemoveCues(TextTrack*, const TextTrackCueList*) OVERRIDE;
    virtual void textTrackAddCue(TextTrack*, PassRefPtr<TextTrackCue>) OVERRIDE;
    virtual void textTrackRemoveCue(TextTrack*, PassRefPtr<TextTrackCue>) OVERRIDE;

    // VideoTrackClient
    virtual void videoTrackSelectedChanged(VideoTrack*) OVERRIDE;

    bool requiresTextTrackRepresentation() const;
    void setTextTrackRepresentation(TextTrackRepresentation*);
#endif

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    void allocateMediaPlayerIfNecessary();
    void setNeedWidgetUpdate(bool needWidgetUpdate) { m_needWidgetUpdate = needWidgetUpdate; }
    void deliverNotification(MediaPlayerProxyNotificationType notification);
    void setMediaPlayerProxy(WebMediaPlayerProxy* proxy);
    void getPluginProxyParams(URL& url, Vector<String>& names, Vector<String>& values);
    void createMediaPlayerProxy();
    void updateWidget(PluginCreationOption);
#endif

    // EventTarget function.
    // Both Node (via HTMLElement) and ActiveDOMObject define this method, which
    // causes an ambiguity error at compile time. This class's constructor
    // ensures that both implementations return document, so return the result
    // of one of them here.
    using HTMLElement::scriptExecutionContext;

    bool hasSingleSecurityOrigin() const { return !m_player || m_player->hasSingleSecurityOrigin(); }
    
    virtual bool isFullscreen() const OVERRIDE;
    void toggleFullscreenState();
    virtual void enterFullscreen() OVERRIDE;
    void exitFullscreen();

    virtual bool hasClosedCaptions() const OVERRIDE;
    virtual bool closedCaptionsVisible() const OVERRIDE;
    virtual void setClosedCaptionsVisible(bool) OVERRIDE;

    MediaControls* mediaControls() const;

    void sourceWasRemoved(HTMLSourceElement*);
    void sourceWasAdded(HTMLSourceElement*);

    virtual void privateBrowsingStateDidChange() OVERRIDE;

    // Media cache management.
    static void getSitesInMediaCache(Vector<String>&);
    static void clearMediaCache();
    static void clearMediaCacheForSite(const String&);
    static void resetMediaEngines();

    bool isPlaying() const { return m_playing; }

    virtual bool hasPendingActivity() const OVERRIDE;

#if ENABLE(WEB_AUDIO)
    MediaElementAudioSourceNode* audioSourceNode() { return m_audioSourceNode; }
    void setAudioSourceNode(MediaElementAudioSourceNode*);

    AudioSourceProvider* audioSourceProvider();
#endif

    enum InvalidURLAction { DoNothing, Complain };
    bool isSafeToLoadURL(const URL&, InvalidURLAction);

    const String& mediaGroup() const;
    void setMediaGroup(const String&);

    MediaController* controller() const;
    void setController(PassRefPtr<MediaController>);

    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

    virtual bool willRespondToMouseClickEvents() OVERRIDE;

    void enteredOrExitedFullscreen() { configureMediaControls(); }

protected:
    HTMLMediaElement(const QualifiedName&, Document&, bool);
    virtual ~HTMLMediaElement();

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void finishParsingChildren() OVERRIDE;
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;
    virtual void willAttachRenderers() OVERRIDE;
    virtual void didAttachRenderers() OVERRIDE;

    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

    enum DisplayMode { Unknown, None, Poster, PosterWaitingForVideo, Video };
    DisplayMode displayMode() const { return m_displayMode; }
    virtual void setDisplayMode(DisplayMode mode) { m_displayMode = mode; }
    
    virtual bool isMediaElement() const OVERRIDE { return true; }

    // Restrictions to change default behaviors.
    enum BehaviorRestrictionFlags {
        NoRestrictions = 0,
        RequireUserGestureForLoadRestriction = 1 << 0,
        RequireUserGestureForRateChangeRestriction = 1 << 1,
        RequireUserGestureForFullscreenRestriction = 1 << 2,
        RequirePageConsentToLoadMediaRestriction = 1 << 3,
        RequirePageConsentToResumeMediaRestriction = 1 << 4,
    };
    typedef unsigned BehaviorRestrictions;
    
    bool userGestureRequiredForLoad() const { return m_restrictions & RequireUserGestureForLoadRestriction; }
    bool userGestureRequiredForRateChange() const { return m_restrictions & RequireUserGestureForRateChangeRestriction; }
    bool userGestureRequiredForFullscreen() const { return m_restrictions & RequireUserGestureForFullscreenRestriction; }
    bool pageConsentRequiredForLoad() const { return m_restrictions & RequirePageConsentToLoadMediaRestriction; }
    bool pageConsentRequiredForResume() const { return m_restrictions & RequirePageConsentToResumeMediaRestriction; }
    
    void addBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions |= restriction; }
    void removeBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions &= ~restriction; }

#if ENABLE(VIDEO_TRACK)
    bool ignoreTrackDisplayUpdateRequests() const { return m_ignoreTrackDisplayUpdate > 0 || !m_textTracks || !m_cueTree.size(); }
    void beginIgnoringTrackDisplayUpdateRequests();
    void endIgnoringTrackDisplayUpdateRequests();
#endif

private:
    void createMediaPlayer();

    virtual bool alwaysCreateUserAgentShadowRoot() const OVERRIDE { return true; }
    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    virtual bool hasCustomFocusLogic() const OVERRIDE;
    virtual bool supportsFocus() const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;
    virtual bool rendererIsNeeded(const RenderStyle&) OVERRIDE;
    virtual RenderElement* createRenderer(PassRef<RenderStyle>) OVERRIDE;
    virtual bool childShouldCreateRenderer(const Node*) const OVERRIDE;
    virtual InsertionNotificationRequest insertedInto(ContainerNode&) OVERRIDE;
    virtual void removedFrom(ContainerNode&) OVERRIDE;
    virtual void didRecalcStyle(Style::Change) OVERRIDE;

    virtual void defaultEventHandler(Event*) OVERRIDE;

    virtual void didBecomeFullscreenElement() OVERRIDE;
    virtual void willStopBeingFullscreenElement() OVERRIDE;

    // ActiveDOMObject functions.
    virtual bool canSuspend() const OVERRIDE;
    virtual void suspend(ReasonForSuspension) OVERRIDE;
    virtual void resume() OVERRIDE;
    virtual void stop() OVERRIDE;
    
    virtual void mediaVolumeDidChange() OVERRIDE;

#if ENABLE(PAGE_VISIBILITY_API)
    virtual void visibilityStateChanged() OVERRIDE;
#endif

    virtual void updateDisplayState() { }
    
    void setReadyState(MediaPlayer::ReadyState);
    void setNetworkState(MediaPlayer::NetworkState);

    virtual Document* mediaPlayerOwningDocument() OVERRIDE;
    virtual void mediaPlayerNetworkStateChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerReadyStateChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerTimeChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerVolumeChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerMuteChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerDurationChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerRateChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerPlaybackStateChanged(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerSawUnsupportedTracks(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerResourceNotSupported(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerRepaint(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerSizeChanged(MediaPlayer*) OVERRIDE;
#if USE(ACCELERATED_COMPOSITING)
    virtual bool mediaPlayerRenderingCanBeAccelerated(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerRenderingModeChanged(MediaPlayer*) OVERRIDE;
#endif
    virtual void mediaPlayerEngineUpdated(MediaPlayer*) OVERRIDE;
    
    virtual void mediaPlayerFirstVideoFrameAvailable(MediaPlayer*) OVERRIDE;
    virtual void mediaPlayerCharacteristicChanged(MediaPlayer*) OVERRIDE;

#if ENABLE(ENCRYPTED_MEDIA)
    virtual void mediaPlayerKeyAdded(MediaPlayer*, const String& keySystem, const String& sessionId) OVERRIDE;
    virtual void mediaPlayerKeyError(MediaPlayer*, const String& keySystem, const String& sessionId, MediaPlayerClient::MediaKeyErrorCode, unsigned short systemCode) OVERRIDE;
    virtual void mediaPlayerKeyMessage(MediaPlayer*, const String& keySystem, const String& sessionId, const unsigned char* message, unsigned messageLength, const URL& defaultURL) OVERRIDE;
    virtual bool mediaPlayerKeyNeeded(MediaPlayer*, const String& keySystem, const String& sessionId, const unsigned char* initData, unsigned initDataLength) OVERRIDE;
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    virtual bool mediaPlayerKeyNeeded(MediaPlayer*, Uint8Array*) OVERRIDE;
#endif

    virtual String mediaPlayerReferrer() const OVERRIDE;
    virtual String mediaPlayerUserAgent() const OVERRIDE;
    virtual CORSMode mediaPlayerCORSMode() const OVERRIDE;

    virtual bool mediaPlayerNeedsSiteSpecificHacks() const OVERRIDE;
    virtual String mediaPlayerDocumentHost() const OVERRIDE;

    virtual void mediaPlayerEnterFullscreen() OVERRIDE;
    virtual void mediaPlayerExitFullscreen() OVERRIDE;
    virtual bool mediaPlayerIsFullscreen() const OVERRIDE;
    virtual bool mediaPlayerIsFullscreenPermitted() const OVERRIDE;
    virtual bool mediaPlayerIsVideo() const OVERRIDE;
    virtual LayoutRect mediaPlayerContentBoxRect() const OVERRIDE;
    virtual void mediaPlayerSetSize(const IntSize&) OVERRIDE;
    virtual void mediaPlayerPause() OVERRIDE;
    virtual void mediaPlayerPlay() OVERRIDE;
    virtual bool mediaPlayerPlatformVolumeConfigurationRequired() const OVERRIDE;
    virtual bool mediaPlayerIsPaused() const OVERRIDE;
    virtual bool mediaPlayerIsLooping() const OVERRIDE;
    virtual HostWindow* mediaPlayerHostWindow() OVERRIDE;
    virtual IntRect mediaPlayerWindowClipRect() OVERRIDE;
    virtual CachedResourceLoader* mediaPlayerCachedResourceLoader() OVERRIDE;

#if PLATFORM(WIN) && USE(AVFOUNDATION)
    virtual GraphicsDeviceAdapter* mediaPlayerGraphicsDeviceAdapter(const MediaPlayer*) const OVERRIDE;
#endif

    void loadTimerFired(Timer<HTMLMediaElement>*);
    void progressEventTimerFired(Timer<HTMLMediaElement>*);
    void playbackProgressTimerFired(Timer<HTMLMediaElement>*);
    void startPlaybackProgressTimer();
    void startProgressEventTimer();
    void stopPeriodicTimers();

    void seek(double time, ExceptionCode&);
    void finishSeek();
    void checkIfSeekNeeded();
    void addPlayedRange(double start, double end);
    
    void scheduleTimeupdateEvent(bool periodicEvent);
    void scheduleEvent(const AtomicString& eventName);
    
    // loading
    void selectMediaResource();
    void loadResource(const URL&, ContentType&, const String& keySystem);
    void scheduleNextSourceChild();
    void loadNextSourceChild();
    void userCancelledLoad();
    void clearMediaPlayer(int flags);
    bool havePotentialSourceChild();
    void noneSupported();
    void mediaEngineError(PassRefPtr<MediaError> err);
    void cancelPendingEventsAndCallbacks();
    void waitForSourceChange();
    void prepareToPlay();

    URL selectNextSourceChild(ContentType*, String* keySystem, InvalidURLAction);

    void mediaLoadingFailed(MediaPlayer::NetworkState);

#if ENABLE(VIDEO_TRACK)
    void updateActiveTextTrackCues(double);
    HTMLTrackElement* showingTrackWithSameKind(HTMLTrackElement*) const;

    enum ReconfigureMode {
        Immediately,
        AfterDelay,
    };
    void markCaptionAndSubtitleTracksAsUnconfigured(ReconfigureMode);
    virtual void captionPreferencesChanged() OVERRIDE;
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

    double minTimeSeekable() const;
    double maxTimeSeekable() const;

    // Pauses playback without changing any states or generating events
    void setPausedInternal(bool);

    void setPlaybackRateInternal(double);

    virtual void mediaCanStart() OVERRIDE;

    void setShouldDelayLoadEvent(bool);
    void invalidateCachedTime();
    void refreshCachedTime() const;

    bool hasMediaControls() const;
    bool createMediaControls();
    void configureMediaControls();

    void prepareMediaFragmentURI();
    void applyMediaFragmentURI();

    void changeNetworkStateFromLoadingToIdle();

    void removeBehaviorsRestrictionsAfterFirstUserGesture();

    void updateMediaController();
    bool isBlocked() const;
    bool isBlockedOnMediaController() const;
    virtual bool hasCurrentSrc() const OVERRIDE { return !m_currentSrc.isEmpty(); }
    virtual bool isLiveStream() const OVERRIDE { return movieLoadType() == MediaPlayer::LiveStream; }
    bool isAutoplaying() const { return m_autoplaying; }

    void updateSleepDisabling();
#if PLATFORM(MAC)
    bool shouldDisableSleep() const;
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    virtual void didAddUserAgentShadowRoot(ShadowRoot*) OVERRIDE;
    DOMWrapperWorld& ensureIsolatedWorld();
    bool ensureMediaControlsInjectedScript();
#endif

    Timer<HTMLMediaElement> m_loadTimer;
    Timer<HTMLMediaElement> m_progressEventTimer;
    Timer<HTMLMediaElement> m_playbackProgressTimer;
    RefPtr<TimeRanges> m_playedTimeRanges;
    GenericEventQueue m_asyncEventQueue;

    double m_playbackRate;
    double m_defaultPlaybackRate;
    bool m_webkitPreservesPitch;
    NetworkState m_networkState;
    ReadyState m_readyState;
    ReadyState m_readyStateMaximum;
    URL m_currentSrc;

    RefPtr<MediaError> m_error;

    double m_volume;
    bool m_volumeInitialized;
    double m_lastSeekTime;
    
    unsigned m_previousProgress;
    double m_previousProgressTime;

    // The last time a timeupdate event was sent (based on monotonic clock).
    double m_clockTimeAtLastUpdateEvent;

    // The last time a timeupdate event was sent in movie time.
    double m_lastTimeUpdateEventMovieTime;
    
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
    RefPtr<HTMLMediaSource> m_mediaSource;
#endif

    mutable double m_cachedTime;
    mutable double m_clockTimeAtLastCachedTimeUpdate;
    mutable double m_minimumClockTimeToUpdateCachedTime;

    double m_fragmentStartTime;
    double m_fragmentEndTime;

    typedef unsigned PendingActionFlags;
    PendingActionFlags m_pendingActionFlags;

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
    bool m_webkitLegacyClosedCaptionOverride : 1;

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    bool m_needWidgetUpdate : 1;
#endif

    bool m_dispatchingCanPlayEvent : 1;
    bool m_loadInitiatedByUserGesture : 1;
    bool m_completelyLoaded : 1;
    bool m_havePreparedToPlay : 1;
    bool m_parsingInProgress : 1;
#if ENABLE(PAGE_VISIBILITY_API)
    bool m_isDisplaySleepDisablingSuspended : 1;
#endif

#if ENABLE(VIDEO_TRACK)
    bool m_tracksAreReady : 1;
    bool m_haveVisibleTextTrack : 1;
    bool m_processingPreferenceChange : 1;

    String m_subtitleTrackLanguage;
    float m_lastTextTrackUpdateTime;

    CaptionUserPreferences::CaptionDisplayMode m_captionDisplayMode;

    RefPtr<AudioTrackList> m_audioTracks;
    RefPtr<TextTrackList> m_textTracks;
    RefPtr<VideoTrackList> m_videoTracks;
    Vector<RefPtr<TextTrack>> m_textTracksWhenResourceSelectionBegan;

    CueIntervalTree m_cueTree;

    CueList m_currentlyActiveCues;
    int m_ignoreTrackDisplayUpdate;
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

    friend class TrackDisplayUpdateScope;

#if ENABLE(ENCRYPTED_MEDIA_V2)
    RefPtr<MediaKeys> m_mediaKeys;
#endif

#if USE(PLATFORM_TEXT_TRACK_MENU)
    RefPtr<PlatformTextTrackMenuInterface> m_platformMenu;
#endif

#if USE(AUDIO_SESSION)
    OwnPtr<AudioSessionManagerToken> m_audioSessionManagerToken;
#endif

    std::unique_ptr<PageActivityAssertionToken> m_activityToken;
    size_t m_reportedExtraMemoryCost;

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    RefPtr<MediaControlsHost> m_mediaControlsHost;
    RefPtr<DOMWrapperWorld> m_isolatedWorld;
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

void isHTMLMediaElement(const HTMLMediaElement&); // Catch unnecessary runtime check of type known at compile time.
inline bool isHTMLMediaElement(const Element& element) { return element.isMediaElement(); }
inline bool isHTMLMediaElement(const Node& node) { return node.isElementNode() && toElement(node).isMediaElement(); }
template <> inline bool isElementOfType<const HTMLMediaElement>(const Element& element) { return element.isMediaElement(); }

NODE_TYPE_CASTS(HTMLMediaElement)

} //namespace

#endif
#endif
