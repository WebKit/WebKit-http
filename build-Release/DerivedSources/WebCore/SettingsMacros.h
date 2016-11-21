/*
 * THIS FILE WAS AUTOMATICALLY GENERATED, DO NOT EDIT.
 *
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
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

#ifndef SettingsMacros_h
#define SettingsMacros_h

#if ENABLE(ATTACHMENT_ELEMENT)
#define ATTACHMENT_ELEMENT_SETTINGS_GETTER_AND_SETTERS \
    bool attachmentElementEnabled() const { return m_attachmentElementEnabled; } \
     void setAttachmentElementEnabled(bool attachmentElementEnabled) { m_attachmentElementEnabled = attachmentElementEnabled; } \
// End of ATTACHMENT_ELEMENT_SETTINGS_GETTER_AND_SETTERS
#define ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define ATTACHMENT_ELEMENT_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_attachmentElementEnabled : 1; \
// End of ATTACHMENT_ELEMENT_SETTINGS_BOOL_MEMBER_VARIABLES
#define ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_INITIALIZERS \
// End of ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_INITIALIZERS
#define ATTACHMENT_ELEMENT_SETTINGS_BOOL_INITIALIZERS \
    , m_attachmentElementEnabled(true) \
// End of ATTACHMENT_ELEMENT_SETTINGS_BOOL_INITIALIZERS
#define ATTACHMENT_ELEMENT_SETTINGS_SETTER_BODIES \
// End of ATTACHMENT_ELEMENT_SETTINGS_SETTER_BODIES
#else
#define ATTACHMENT_ELEMENT_SETTINGS_GETTER_AND_SETTERS
#define ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define ATTACHMENT_ELEMENT_SETTINGS_BOOL_MEMBER_VARIABLES
#define ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_INITIALIZERS
#define ATTACHMENT_ELEMENT_SETTINGS_BOOL_INITIALIZERS
#define ATTACHMENT_ELEMENT_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(DASHBOARD_SUPPORT)
#define DASHBOARD_SUPPORT_SETTINGS_GETTER_AND_SETTERS \
    bool usesDashboardBackwardCompatibilityMode() const { return m_usesDashboardBackwardCompatibilityMode; } \
     void setUsesDashboardBackwardCompatibilityMode(bool usesDashboardBackwardCompatibilityMode) { m_usesDashboardBackwardCompatibilityMode = usesDashboardBackwardCompatibilityMode; } \
// End of DASHBOARD_SUPPORT_SETTINGS_GETTER_AND_SETTERS
#define DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define DASHBOARD_SUPPORT_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_usesDashboardBackwardCompatibilityMode : 1; \
// End of DASHBOARD_SUPPORT_SETTINGS_BOOL_MEMBER_VARIABLES
#define DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_INITIALIZERS \
// End of DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_INITIALIZERS
#define DASHBOARD_SUPPORT_SETTINGS_BOOL_INITIALIZERS \
    , m_usesDashboardBackwardCompatibilityMode(false) \
// End of DASHBOARD_SUPPORT_SETTINGS_BOOL_INITIALIZERS
#define DASHBOARD_SUPPORT_SETTINGS_SETTER_BODIES \
// End of DASHBOARD_SUPPORT_SETTINGS_SETTER_BODIES
#else
#define DASHBOARD_SUPPORT_SETTINGS_GETTER_AND_SETTERS
#define DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define DASHBOARD_SUPPORT_SETTINGS_BOOL_MEMBER_VARIABLES
#define DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_INITIALIZERS
#define DASHBOARD_SUPPORT_SETTINGS_BOOL_INITIALIZERS
#define DASHBOARD_SUPPORT_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(DATA_DETECTION)
#define DATA_DETECTION_SETTINGS_GETTER_AND_SETTERS \
    const DataDetectorTypes& dataDetectorTypes() { return m_dataDetectorTypes; } \
     void setDataDetectorTypes(const DataDetectorTypes& dataDetectorTypes) { m_dataDetectorTypes = dataDetectorTypes; } \
// End of DATA_DETECTION_SETTINGS_GETTER_AND_SETTERS
#define DATA_DETECTION_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    DataDetectorTypes m_dataDetectorTypes; \
// End of DATA_DETECTION_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define DATA_DETECTION_SETTINGS_BOOL_MEMBER_VARIABLES \
// End of DATA_DETECTION_SETTINGS_BOOL_MEMBER_VARIABLES
#define DATA_DETECTION_SETTINGS_NON_BOOL_INITIALIZERS \
    , m_dataDetectorTypes(DataDetectorTypeNone) \
// End of DATA_DETECTION_SETTINGS_NON_BOOL_INITIALIZERS
#define DATA_DETECTION_SETTINGS_BOOL_INITIALIZERS \
// End of DATA_DETECTION_SETTINGS_BOOL_INITIALIZERS
#define DATA_DETECTION_SETTINGS_SETTER_BODIES \
// End of DATA_DETECTION_SETTINGS_SETTER_BODIES
#else
#define DATA_DETECTION_SETTINGS_GETTER_AND_SETTERS
#define DATA_DETECTION_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define DATA_DETECTION_SETTINGS_BOOL_MEMBER_VARIABLES
#define DATA_DETECTION_SETTINGS_NON_BOOL_INITIALIZERS
#define DATA_DETECTION_SETTINGS_BOOL_INITIALIZERS
#define DATA_DETECTION_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(FULLSCREEN_API)
#define FULLSCREEN_API_SETTINGS_GETTER_AND_SETTERS \
    bool fullScreenEnabled() const { return m_fullScreenEnabled; } \
     void setFullScreenEnabled(bool fullScreenEnabled) { m_fullScreenEnabled = fullScreenEnabled; } \
// End of FULLSCREEN_API_SETTINGS_GETTER_AND_SETTERS
#define FULLSCREEN_API_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of FULLSCREEN_API_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define FULLSCREEN_API_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_fullScreenEnabled : 1; \
// End of FULLSCREEN_API_SETTINGS_BOOL_MEMBER_VARIABLES
#define FULLSCREEN_API_SETTINGS_NON_BOOL_INITIALIZERS \
// End of FULLSCREEN_API_SETTINGS_NON_BOOL_INITIALIZERS
#define FULLSCREEN_API_SETTINGS_BOOL_INITIALIZERS \
    , m_fullScreenEnabled(false) \
// End of FULLSCREEN_API_SETTINGS_BOOL_INITIALIZERS
#define FULLSCREEN_API_SETTINGS_SETTER_BODIES \
// End of FULLSCREEN_API_SETTINGS_SETTER_BODIES
#else
#define FULLSCREEN_API_SETTINGS_GETTER_AND_SETTERS
#define FULLSCREEN_API_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define FULLSCREEN_API_SETTINGS_BOOL_MEMBER_VARIABLES
#define FULLSCREEN_API_SETTINGS_NON_BOOL_INITIALIZERS
#define FULLSCREEN_API_SETTINGS_BOOL_INITIALIZERS
#define FULLSCREEN_API_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(IOS_TEXT_AUTOSIZING)
#define IOS_TEXT_AUTOSIZING_SETTINGS_GETTER_AND_SETTERS \
    float minimumZoomFontSize() const { return m_minimumZoomFontSize; } \
     void setMinimumZoomFontSize(float minimumZoomFontSize) { m_minimumZoomFontSize = minimumZoomFontSize; } \
    bool textAutosizingEnabled() const { return m_textAutosizingEnabled; } \
    WEBCORE_EXPORT void setTextAutosizingEnabled(bool textAutosizingEnabled); \
    const IntSize& textAutosizingWindowSizeOverride() { return m_textAutosizingWindowSizeOverride; } \
    WEBCORE_EXPORT void setTextAutosizingWindowSizeOverride(const IntSize& textAutosizingWindowSizeOverride); \
// End of IOS_TEXT_AUTOSIZING_SETTINGS_GETTER_AND_SETTERS
#define IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    float m_minimumZoomFontSize; \
    IntSize m_textAutosizingWindowSizeOverride; \
// End of IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_textAutosizingEnabled : 1; \
// End of IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_MEMBER_VARIABLES
#define IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_INITIALIZERS \
    , m_minimumZoomFontSize(defaultMinimumZoomFontSize()) \
// End of IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_INITIALIZERS
#define IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_INITIALIZERS \
    , m_textAutosizingEnabled(defaultTextAutosizingEnabled()) \
// End of IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_INITIALIZERS
#define IOS_TEXT_AUTOSIZING_SETTINGS_SETTER_BODIES \
void Settings::setTextAutosizingEnabled(bool textAutosizingEnabled) \
{ \
    if (m_textAutosizingEnabled == textAutosizingEnabled) \
        return; \
    m_textAutosizingEnabled = textAutosizingEnabled; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setTextAutosizingWindowSizeOverride(const IntSize& textAutosizingWindowSizeOverride) \
{ \
    if (m_textAutosizingWindowSizeOverride == textAutosizingWindowSizeOverride) \
        return; \
    m_textAutosizingWindowSizeOverride = textAutosizingWindowSizeOverride; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
// End of IOS_TEXT_AUTOSIZING_SETTINGS_SETTER_BODIES
#else
#define IOS_TEXT_AUTOSIZING_SETTINGS_GETTER_AND_SETTERS
#define IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_MEMBER_VARIABLES
#define IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_INITIALIZERS
#define IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_INITIALIZERS
#define IOS_TEXT_AUTOSIZING_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(MEDIA_SOURCE)
#define MEDIA_SOURCE_SETTINGS_GETTER_AND_SETTERS \
    int maximumSourceBufferSize() const { return m_maximumSourceBufferSize; } \
     void setMaximumSourceBufferSize(int maximumSourceBufferSize) { m_maximumSourceBufferSize = maximumSourceBufferSize; } \
    bool mediaSourceEnabled() const { return m_mediaSourceEnabled; } \
     void setMediaSourceEnabled(bool mediaSourceEnabled) { m_mediaSourceEnabled = mediaSourceEnabled; } \
// End of MEDIA_SOURCE_SETTINGS_GETTER_AND_SETTERS
#define MEDIA_SOURCE_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    int m_maximumSourceBufferSize; \
// End of MEDIA_SOURCE_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define MEDIA_SOURCE_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_mediaSourceEnabled : 1; \
// End of MEDIA_SOURCE_SETTINGS_BOOL_MEMBER_VARIABLES
#define MEDIA_SOURCE_SETTINGS_NON_BOOL_INITIALIZERS \
    , m_maximumSourceBufferSize(318767104) \
// End of MEDIA_SOURCE_SETTINGS_NON_BOOL_INITIALIZERS
#define MEDIA_SOURCE_SETTINGS_BOOL_INITIALIZERS \
    , m_mediaSourceEnabled(true) \
// End of MEDIA_SOURCE_SETTINGS_BOOL_INITIALIZERS
#define MEDIA_SOURCE_SETTINGS_SETTER_BODIES \
// End of MEDIA_SOURCE_SETTINGS_SETTER_BODIES
#else
#define MEDIA_SOURCE_SETTINGS_GETTER_AND_SETTERS
#define MEDIA_SOURCE_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define MEDIA_SOURCE_SETTINGS_BOOL_MEMBER_VARIABLES
#define MEDIA_SOURCE_SETTINGS_NON_BOOL_INITIALIZERS
#define MEDIA_SOURCE_SETTINGS_BOOL_INITIALIZERS
#define MEDIA_SOURCE_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(RUBBER_BANDING)
#define RUBBER_BANDING_SETTINGS_GETTER_AND_SETTERS \
    bool rubberBandingForSubScrollableRegionsEnabled() const { return m_rubberBandingForSubScrollableRegionsEnabled; } \
     void setRubberBandingForSubScrollableRegionsEnabled(bool rubberBandingForSubScrollableRegionsEnabled) { m_rubberBandingForSubScrollableRegionsEnabled = rubberBandingForSubScrollableRegionsEnabled; } \
// End of RUBBER_BANDING_SETTINGS_GETTER_AND_SETTERS
#define RUBBER_BANDING_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of RUBBER_BANDING_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define RUBBER_BANDING_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_rubberBandingForSubScrollableRegionsEnabled : 1; \
// End of RUBBER_BANDING_SETTINGS_BOOL_MEMBER_VARIABLES
#define RUBBER_BANDING_SETTINGS_NON_BOOL_INITIALIZERS \
// End of RUBBER_BANDING_SETTINGS_NON_BOOL_INITIALIZERS
#define RUBBER_BANDING_SETTINGS_BOOL_INITIALIZERS \
    , m_rubberBandingForSubScrollableRegionsEnabled(true) \
// End of RUBBER_BANDING_SETTINGS_BOOL_INITIALIZERS
#define RUBBER_BANDING_SETTINGS_SETTER_BODIES \
// End of RUBBER_BANDING_SETTINGS_SETTER_BODIES
#else
#define RUBBER_BANDING_SETTINGS_GETTER_AND_SETTERS
#define RUBBER_BANDING_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define RUBBER_BANDING_SETTINGS_BOOL_MEMBER_VARIABLES
#define RUBBER_BANDING_SETTINGS_NON_BOOL_INITIALIZERS
#define RUBBER_BANDING_SETTINGS_BOOL_INITIALIZERS
#define RUBBER_BANDING_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(SERVICE_CONTROLS)
#define SERVICE_CONTROLS_SETTINGS_GETTER_AND_SETTERS \
    bool imageControlsEnabled() const { return m_imageControlsEnabled; } \
     void setImageControlsEnabled(bool imageControlsEnabled) { m_imageControlsEnabled = imageControlsEnabled; } \
    bool serviceControlsEnabled() const { return m_serviceControlsEnabled; } \
     void setServiceControlsEnabled(bool serviceControlsEnabled) { m_serviceControlsEnabled = serviceControlsEnabled; } \
// End of SERVICE_CONTROLS_SETTINGS_GETTER_AND_SETTERS
#define SERVICE_CONTROLS_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of SERVICE_CONTROLS_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define SERVICE_CONTROLS_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_imageControlsEnabled : 1; \
    bool m_serviceControlsEnabled : 1; \
// End of SERVICE_CONTROLS_SETTINGS_BOOL_MEMBER_VARIABLES
#define SERVICE_CONTROLS_SETTINGS_NON_BOOL_INITIALIZERS \
// End of SERVICE_CONTROLS_SETTINGS_NON_BOOL_INITIALIZERS
#define SERVICE_CONTROLS_SETTINGS_BOOL_INITIALIZERS \
    , m_imageControlsEnabled(false) \
    , m_serviceControlsEnabled(false) \
// End of SERVICE_CONTROLS_SETTINGS_BOOL_INITIALIZERS
#define SERVICE_CONTROLS_SETTINGS_SETTER_BODIES \
// End of SERVICE_CONTROLS_SETTINGS_SETTER_BODIES
#else
#define SERVICE_CONTROLS_SETTINGS_GETTER_AND_SETTERS
#define SERVICE_CONTROLS_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define SERVICE_CONTROLS_SETTINGS_BOOL_MEMBER_VARIABLES
#define SERVICE_CONTROLS_SETTINGS_NON_BOOL_INITIALIZERS
#define SERVICE_CONTROLS_SETTINGS_BOOL_INITIALIZERS
#define SERVICE_CONTROLS_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(SMOOTH_SCROLLING)
#define SMOOTH_SCROLLING_SETTINGS_GETTER_AND_SETTERS \
    bool scrollAnimatorEnabled() const { return m_scrollAnimatorEnabled; } \
     void setScrollAnimatorEnabled(bool scrollAnimatorEnabled) { m_scrollAnimatorEnabled = scrollAnimatorEnabled; } \
// End of SMOOTH_SCROLLING_SETTINGS_GETTER_AND_SETTERS
#define SMOOTH_SCROLLING_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of SMOOTH_SCROLLING_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define SMOOTH_SCROLLING_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_scrollAnimatorEnabled : 1; \
// End of SMOOTH_SCROLLING_SETTINGS_BOOL_MEMBER_VARIABLES
#define SMOOTH_SCROLLING_SETTINGS_NON_BOOL_INITIALIZERS \
// End of SMOOTH_SCROLLING_SETTINGS_NON_BOOL_INITIALIZERS
#define SMOOTH_SCROLLING_SETTINGS_BOOL_INITIALIZERS \
    , m_scrollAnimatorEnabled(true) \
// End of SMOOTH_SCROLLING_SETTINGS_BOOL_INITIALIZERS
#define SMOOTH_SCROLLING_SETTINGS_SETTER_BODIES \
// End of SMOOTH_SCROLLING_SETTINGS_SETTER_BODIES
#else
#define SMOOTH_SCROLLING_SETTINGS_GETTER_AND_SETTERS
#define SMOOTH_SCROLLING_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define SMOOTH_SCROLLING_SETTINGS_BOOL_MEMBER_VARIABLES
#define SMOOTH_SCROLLING_SETTINGS_NON_BOOL_INITIALIZERS
#define SMOOTH_SCROLLING_SETTINGS_BOOL_INITIALIZERS
#define SMOOTH_SCROLLING_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(VIDEO_TRACK)
#define VIDEO_TRACK_SETTINGS_GETTER_AND_SETTERS \
    bool shouldDisplayCaptions() const { return m_shouldDisplayCaptions; } \
     void setShouldDisplayCaptions(bool shouldDisplayCaptions) { m_shouldDisplayCaptions = shouldDisplayCaptions; } \
    bool shouldDisplaySubtitles() const { return m_shouldDisplaySubtitles; } \
     void setShouldDisplaySubtitles(bool shouldDisplaySubtitles) { m_shouldDisplaySubtitles = shouldDisplaySubtitles; } \
    bool shouldDisplayTextDescriptions() const { return m_shouldDisplayTextDescriptions; } \
     void setShouldDisplayTextDescriptions(bool shouldDisplayTextDescriptions) { m_shouldDisplayTextDescriptions = shouldDisplayTextDescriptions; } \
// End of VIDEO_TRACK_SETTINGS_GETTER_AND_SETTERS
#define VIDEO_TRACK_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of VIDEO_TRACK_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define VIDEO_TRACK_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_shouldDisplayCaptions : 1; \
    bool m_shouldDisplaySubtitles : 1; \
    bool m_shouldDisplayTextDescriptions : 1; \
// End of VIDEO_TRACK_SETTINGS_BOOL_MEMBER_VARIABLES
#define VIDEO_TRACK_SETTINGS_NON_BOOL_INITIALIZERS \
// End of VIDEO_TRACK_SETTINGS_NON_BOOL_INITIALIZERS
#define VIDEO_TRACK_SETTINGS_BOOL_INITIALIZERS \
    , m_shouldDisplayCaptions(false) \
    , m_shouldDisplaySubtitles(false) \
    , m_shouldDisplayTextDescriptions(false) \
// End of VIDEO_TRACK_SETTINGS_BOOL_INITIALIZERS
#define VIDEO_TRACK_SETTINGS_SETTER_BODIES \
// End of VIDEO_TRACK_SETTINGS_SETTER_BODIES
#else
#define VIDEO_TRACK_SETTINGS_GETTER_AND_SETTERS
#define VIDEO_TRACK_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define VIDEO_TRACK_SETTINGS_BOOL_MEMBER_VARIABLES
#define VIDEO_TRACK_SETTINGS_NON_BOOL_INITIALIZERS
#define VIDEO_TRACK_SETTINGS_BOOL_INITIALIZERS
#define VIDEO_TRACK_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(WEB_ARCHIVE)
#define WEB_ARCHIVE_SETTINGS_GETTER_AND_SETTERS \
    bool webArchiveDebugModeEnabled() const { return m_webArchiveDebugModeEnabled; } \
     void setWebArchiveDebugModeEnabled(bool webArchiveDebugModeEnabled) { m_webArchiveDebugModeEnabled = webArchiveDebugModeEnabled; } \
// End of WEB_ARCHIVE_SETTINGS_GETTER_AND_SETTERS
#define WEB_ARCHIVE_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of WEB_ARCHIVE_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define WEB_ARCHIVE_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_webArchiveDebugModeEnabled : 1; \
// End of WEB_ARCHIVE_SETTINGS_BOOL_MEMBER_VARIABLES
#define WEB_ARCHIVE_SETTINGS_NON_BOOL_INITIALIZERS \
// End of WEB_ARCHIVE_SETTINGS_NON_BOOL_INITIALIZERS
#define WEB_ARCHIVE_SETTINGS_BOOL_INITIALIZERS \
    , m_webArchiveDebugModeEnabled(false) \
// End of WEB_ARCHIVE_SETTINGS_BOOL_INITIALIZERS
#define WEB_ARCHIVE_SETTINGS_SETTER_BODIES \
// End of WEB_ARCHIVE_SETTINGS_SETTER_BODIES
#else
#define WEB_ARCHIVE_SETTINGS_GETTER_AND_SETTERS
#define WEB_ARCHIVE_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define WEB_ARCHIVE_SETTINGS_BOOL_MEMBER_VARIABLES
#define WEB_ARCHIVE_SETTINGS_NON_BOOL_INITIALIZERS
#define WEB_ARCHIVE_SETTINGS_BOOL_INITIALIZERS
#define WEB_ARCHIVE_SETTINGS_SETTER_BODIES
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_GETTER_AND_SETTERS \
    bool allowsAirPlayForMediaPlayback() const { return m_allowsAirPlayForMediaPlayback; } \
     void setAllowsAirPlayForMediaPlayback(bool allowsAirPlayForMediaPlayback) { m_allowsAirPlayForMediaPlayback = allowsAirPlayForMediaPlayback; } \
// End of WIRELESS_PLAYBACK_TARGET_SETTINGS_GETTER_AND_SETTERS
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
// End of WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_MEMBER_VARIABLES \
    bool m_allowsAirPlayForMediaPlayback : 1; \
// End of WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_MEMBER_VARIABLES
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_INITIALIZERS \
// End of WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_INITIALIZERS
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_INITIALIZERS \
    , m_allowsAirPlayForMediaPlayback(true) \
// End of WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_INITIALIZERS
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_SETTER_BODIES \
// End of WIRELESS_PLAYBACK_TARGET_SETTINGS_SETTER_BODIES
#else
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_GETTER_AND_SETTERS
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_MEMBER_VARIABLES
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_MEMBER_VARIABLES
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_INITIALIZERS
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_INITIALIZERS
#define WIRELESS_PLAYBACK_TARGET_SETTINGS_SETTER_BODIES
#endif

#define SETTINGS_GETTERS_AND_SETTERS \
    bool DOMPasteAllowed() const { return m_DOMPasteAllowed; } \
     void setDOMPasteAllowed(bool DOMPasteAllowed) { m_DOMPasteAllowed = DOMPasteAllowed; } \
    bool accelerated2dCanvasEnabled() const { return m_accelerated2dCanvasEnabled; } \
     void setAccelerated2dCanvasEnabled(bool accelerated2dCanvasEnabled) { m_accelerated2dCanvasEnabled = accelerated2dCanvasEnabled; } \
    bool acceleratedCompositedAnimationsEnabled() const { return m_acceleratedCompositedAnimationsEnabled; } \
    WEBCORE_EXPORT void setAcceleratedCompositedAnimationsEnabled(bool acceleratedCompositedAnimationsEnabled); \
    bool acceleratedCompositingEnabled() const { return m_acceleratedCompositingEnabled; } \
    WEBCORE_EXPORT void setAcceleratedCompositingEnabled(bool acceleratedCompositingEnabled); \
    bool acceleratedCompositingForFixedPositionEnabled() const { return m_acceleratedCompositingForFixedPositionEnabled; } \
     void setAcceleratedCompositingForFixedPositionEnabled(bool acceleratedCompositingForFixedPositionEnabled) { m_acceleratedCompositingForFixedPositionEnabled = acceleratedCompositingForFixedPositionEnabled; } \
    bool acceleratedCompositingForOverflowScrollEnabled() const { return m_acceleratedCompositingForOverflowScrollEnabled; } \
     void setAcceleratedCompositingForOverflowScrollEnabled(bool acceleratedCompositingForOverflowScrollEnabled) { m_acceleratedCompositingForOverflowScrollEnabled = acceleratedCompositingForOverflowScrollEnabled; } \
    bool acceleratedDrawingEnabled() const { return m_acceleratedDrawingEnabled; } \
     void setAcceleratedDrawingEnabled(bool acceleratedDrawingEnabled) { m_acceleratedDrawingEnabled = acceleratedDrawingEnabled; } \
    bool acceleratedFiltersEnabled() const { return m_acceleratedFiltersEnabled; } \
     void setAcceleratedFiltersEnabled(bool acceleratedFiltersEnabled) { m_acceleratedFiltersEnabled = acceleratedFiltersEnabled; } \
    bool aggressiveTileRetentionEnabled() const { return m_aggressiveTileRetentionEnabled; } \
     void setAggressiveTileRetentionEnabled(bool aggressiveTileRetentionEnabled) { m_aggressiveTileRetentionEnabled = aggressiveTileRetentionEnabled; } \
    bool allowContentSecurityPolicySourceStarToMatchAnyProtocol() const { return m_allowContentSecurityPolicySourceStarToMatchAnyProtocol; } \
     void setAllowContentSecurityPolicySourceStarToMatchAnyProtocol(bool allowContentSecurityPolicySourceStarToMatchAnyProtocol) { m_allowContentSecurityPolicySourceStarToMatchAnyProtocol = allowContentSecurityPolicySourceStarToMatchAnyProtocol; } \
    bool allowCustomScrollbarInMainFrame() const { return m_allowCustomScrollbarInMainFrame; } \
     void setAllowCustomScrollbarInMainFrame(bool allowCustomScrollbarInMainFrame) { m_allowCustomScrollbarInMainFrame = allowCustomScrollbarInMainFrame; } \
    bool allowDisplayOfInsecureContent() const { return m_allowDisplayOfInsecureContent; } \
     void setAllowDisplayOfInsecureContent(bool allowDisplayOfInsecureContent) { m_allowDisplayOfInsecureContent = allowDisplayOfInsecureContent; } \
    bool allowFileAccessFromFileURLs() const { return m_allowFileAccessFromFileURLs; } \
     void setAllowFileAccessFromFileURLs(bool allowFileAccessFromFileURLs) { m_allowFileAccessFromFileURLs = allowFileAccessFromFileURLs; } \
    bool allowMultiElementImplicitSubmission() const { return m_allowMultiElementImplicitSubmission; } \
     void setAllowMultiElementImplicitSubmission(bool allowMultiElementImplicitSubmission) { m_allowMultiElementImplicitSubmission = allowMultiElementImplicitSubmission; } \
    bool allowRunningOfInsecureContent() const { return m_allowRunningOfInsecureContent; } \
     void setAllowRunningOfInsecureContent(bool allowRunningOfInsecureContent) { m_allowRunningOfInsecureContent = allowRunningOfInsecureContent; } \
    bool allowScriptsToCloseWindows() const { return m_allowScriptsToCloseWindows; } \
     void setAllowScriptsToCloseWindows(bool allowScriptsToCloseWindows) { m_allowScriptsToCloseWindows = allowScriptsToCloseWindows; } \
    bool allowUniversalAccessFromFileURLs() const { return m_allowUniversalAccessFromFileURLs; } \
     void setAllowUniversalAccessFromFileURLs(bool allowUniversalAccessFromFileURLs) { m_allowUniversalAccessFromFileURLs = allowUniversalAccessFromFileURLs; } \
    bool allowWindowOpenWithoutUserGesture() const { return m_allowWindowOpenWithoutUserGesture; } \
     void setAllowWindowOpenWithoutUserGesture(bool allowWindowOpenWithoutUserGesture) { m_allowWindowOpenWithoutUserGesture = allowWindowOpenWithoutUserGesture; } \
    bool allowsInlineMediaPlayback() const { return m_allowsInlineMediaPlayback; } \
     void setAllowsInlineMediaPlayback(bool allowsInlineMediaPlayback) { m_allowsInlineMediaPlayback = allowsInlineMediaPlayback; } \
    bool allowsInlineMediaPlaybackAfterFullscreen() const { return m_allowsInlineMediaPlaybackAfterFullscreen; } \
     void setAllowsInlineMediaPlaybackAfterFullscreen(bool allowsInlineMediaPlaybackAfterFullscreen) { m_allowsInlineMediaPlaybackAfterFullscreen = allowsInlineMediaPlaybackAfterFullscreen; } \
    bool allowsPictureInPictureMediaPlayback() const { return m_allowsPictureInPictureMediaPlayback; } \
     void setAllowsPictureInPictureMediaPlayback(bool allowsPictureInPictureMediaPlayback) { m_allowsPictureInPictureMediaPlayback = allowsPictureInPictureMediaPlayback; } \
    bool alwaysUseAcceleratedOverflowScroll() const { return m_alwaysUseAcceleratedOverflowScroll; } \
     void setAlwaysUseAcceleratedOverflowScroll(bool alwaysUseAcceleratedOverflowScroll) { m_alwaysUseAcceleratedOverflowScroll = alwaysUseAcceleratedOverflowScroll; } \
    bool antialiased2dCanvasEnabled() const { return m_antialiased2dCanvasEnabled; } \
     void setAntialiased2dCanvasEnabled(bool antialiased2dCanvasEnabled) { m_antialiased2dCanvasEnabled = antialiased2dCanvasEnabled; } \
    bool appleMailPaginationQuirkEnabled() const { return m_appleMailPaginationQuirkEnabled; } \
     void setAppleMailPaginationQuirkEnabled(bool appleMailPaginationQuirkEnabled) { m_appleMailPaginationQuirkEnabled = appleMailPaginationQuirkEnabled; } \
    bool asynchronousSpellCheckingEnabled() const { return m_asynchronousSpellCheckingEnabled; } \
     void setAsynchronousSpellCheckingEnabled(bool asynchronousSpellCheckingEnabled) { m_asynchronousSpellCheckingEnabled = asynchronousSpellCheckingEnabled; } \
    bool audioPlaybackRequiresUserGesture() const { return m_audioPlaybackRequiresUserGesture; } \
     void setAudioPlaybackRequiresUserGesture(bool audioPlaybackRequiresUserGesture) { m_audioPlaybackRequiresUserGesture = audioPlaybackRequiresUserGesture; } \
    bool authorAndUserStylesEnabled() const { return m_authorAndUserStylesEnabled; } \
    WEBCORE_EXPORT void setAuthorAndUserStylesEnabled(bool authorAndUserStylesEnabled); \
    bool autoscrollForDragAndDropEnabled() const { return m_autoscrollForDragAndDropEnabled; } \
     void setAutoscrollForDragAndDropEnabled(bool autoscrollForDragAndDropEnabled) { m_autoscrollForDragAndDropEnabled = autoscrollForDragAndDropEnabled; } \
    bool autostartOriginPlugInSnapshottingEnabled() const { return m_autostartOriginPlugInSnapshottingEnabled; } \
     void setAutostartOriginPlugInSnapshottingEnabled(bool autostartOriginPlugInSnapshottingEnabled) { m_autostartOriginPlugInSnapshottingEnabled = autostartOriginPlugInSnapshottingEnabled; } \
    double backForwardCacheExpirationInterval() const { return m_backForwardCacheExpirationInterval; } \
     void setBackForwardCacheExpirationInterval(double backForwardCacheExpirationInterval) { m_backForwardCacheExpirationInterval = backForwardCacheExpirationInterval; } \
    bool backspaceKeyNavigationEnabled() const { return m_backspaceKeyNavigationEnabled; } \
     void setBackspaceKeyNavigationEnabled(bool backspaceKeyNavigationEnabled) { m_backspaceKeyNavigationEnabled = backspaceKeyNavigationEnabled; } \
    bool canvasUsesAcceleratedDrawing() const { return m_canvasUsesAcceleratedDrawing; } \
     void setCanvasUsesAcceleratedDrawing(bool canvasUsesAcceleratedDrawing) { m_canvasUsesAcceleratedDrawing = canvasUsesAcceleratedDrawing; } \
    bool caretBrowsingEnabled() const { return m_caretBrowsingEnabled; } \
     void setCaretBrowsingEnabled(bool caretBrowsingEnabled) { m_caretBrowsingEnabled = caretBrowsingEnabled; } \
    const ClipboardAccessPolicy& clipboardAccessPolicy() { return m_clipboardAccessPolicy; } \
     void setClipboardAccessPolicy(const ClipboardAccessPolicy& clipboardAccessPolicy) { m_clipboardAccessPolicy = clipboardAccessPolicy; } \
    bool contentDispositionAttachmentSandboxEnabled() const { return m_contentDispositionAttachmentSandboxEnabled; } \
     void setContentDispositionAttachmentSandboxEnabled(bool contentDispositionAttachmentSandboxEnabled) { m_contentDispositionAttachmentSandboxEnabled = contentDispositionAttachmentSandboxEnabled; } \
    bool cookieEnabled() const { return m_cookieEnabled; } \
     void setCookieEnabled(bool cookieEnabled) { m_cookieEnabled = cookieEnabled; } \
    bool crossOriginCheckInGetMatchedCSSRulesDisabled() const { return m_crossOriginCheckInGetMatchedCSSRulesDisabled; } \
     void setCrossOriginCheckInGetMatchedCSSRulesDisabled(bool crossOriginCheckInGetMatchedCSSRulesDisabled) { m_crossOriginCheckInGetMatchedCSSRulesDisabled = crossOriginCheckInGetMatchedCSSRulesDisabled; } \
    int defaultFixedFontSize() const { return m_defaultFixedFontSize; } \
    WEBCORE_EXPORT void setDefaultFixedFontSize(int defaultFixedFontSize); \
    int defaultFontSize() const { return m_defaultFontSize; } \
    WEBCORE_EXPORT void setDefaultFontSize(int defaultFontSize); \
    const String& defaultTextEncodingName() { return m_defaultTextEncodingName; } \
     void setDefaultTextEncodingName(const String& defaultTextEncodingName) { m_defaultTextEncodingName = defaultTextEncodingName; } \
    const String& defaultVideoPosterURL() { return m_defaultVideoPosterURL; } \
     void setDefaultVideoPosterURL(const String& defaultVideoPosterURL) { m_defaultVideoPosterURL = defaultVideoPosterURL; } \
    bool delegatesPageScaling() const { return m_delegatesPageScaling; } \
     void setDelegatesPageScaling(bool delegatesPageScaling) { m_delegatesPageScaling = delegatesPageScaling; } \
    bool developerExtrasEnabled() const { return m_developerExtrasEnabled; } \
     void setDeveloperExtrasEnabled(bool developerExtrasEnabled) { m_developerExtrasEnabled = developerExtrasEnabled; } \
    int deviceHeight() const { return m_deviceHeight; } \
     void setDeviceHeight(int deviceHeight) { m_deviceHeight = deviceHeight; } \
    int deviceWidth() const { return m_deviceWidth; } \
     void setDeviceWidth(int deviceWidth) { m_deviceWidth = deviceWidth; } \
    bool diagnosticLoggingEnabled() const { return m_diagnosticLoggingEnabled; } \
     void setDiagnosticLoggingEnabled(bool diagnosticLoggingEnabled) { m_diagnosticLoggingEnabled = diagnosticLoggingEnabled; } \
    bool displayListDrawingEnabled() const { return m_displayListDrawingEnabled; } \
     void setDisplayListDrawingEnabled(bool displayListDrawingEnabled) { m_displayListDrawingEnabled = displayListDrawingEnabled; } \
    bool domTimersThrottlingEnabled() const { return m_domTimersThrottlingEnabled; } \
     void setDOMTimersThrottlingEnabled(bool domTimersThrottlingEnabled) { m_domTimersThrottlingEnabled = domTimersThrottlingEnabled; } \
    bool downloadableBinaryFontsEnabled() const { return m_downloadableBinaryFontsEnabled; } \
     void setDownloadableBinaryFontsEnabled(bool downloadableBinaryFontsEnabled) { m_downloadableBinaryFontsEnabled = downloadableBinaryFontsEnabled; } \
    const EditableLinkBehavior& editableLinkBehavior() { return m_editableLinkBehavior; } \
     void setEditableLinkBehavior(const EditableLinkBehavior& editableLinkBehavior) { m_editableLinkBehavior = editableLinkBehavior; } \
    const EditingBehaviorType& editingBehaviorType() { return m_editingBehaviorType; } \
     void setEditingBehaviorType(const EditingBehaviorType& editingBehaviorType) { m_editingBehaviorType = editingBehaviorType; } \
    bool enableInheritURIQueryComponent() const { return m_enableInheritURIQueryComponent; } \
     void setEnableInheritURIQueryComponent(bool enableInheritURIQueryComponent) { m_enableInheritURIQueryComponent = enableInheritURIQueryComponent; } \
    bool enforceCSSMIMETypeInNoQuirksMode() const { return m_enforceCSSMIMETypeInNoQuirksMode; } \
     void setEnforceCSSMIMETypeInNoQuirksMode(bool enforceCSSMIMETypeInNoQuirksMode) { m_enforceCSSMIMETypeInNoQuirksMode = enforceCSSMIMETypeInNoQuirksMode; } \
    bool experimentalNotificationsEnabled() const { return m_experimentalNotificationsEnabled; } \
     void setExperimentalNotificationsEnabled(bool experimentalNotificationsEnabled) { m_experimentalNotificationsEnabled = experimentalNotificationsEnabled; } \
    bool fixedBackgroundsPaintRelativeToDocument() const { return m_fixedBackgroundsPaintRelativeToDocument; } \
     void setFixedBackgroundsPaintRelativeToDocument(bool fixedBackgroundsPaintRelativeToDocument) { m_fixedBackgroundsPaintRelativeToDocument = fixedBackgroundsPaintRelativeToDocument; } \
    bool fixedElementsLayoutRelativeToFrame() const { return m_fixedElementsLayoutRelativeToFrame; } \
     void setFixedElementsLayoutRelativeToFrame(bool fixedElementsLayoutRelativeToFrame) { m_fixedElementsLayoutRelativeToFrame = fixedElementsLayoutRelativeToFrame; } \
    bool fixedPositionCreatesStackingContext() const { return m_fixedPositionCreatesStackingContext; } \
     void setFixedPositionCreatesStackingContext(bool fixedPositionCreatesStackingContext) { m_fixedPositionCreatesStackingContext = fixedPositionCreatesStackingContext; } \
    bool forceCompositingMode() const { return m_forceCompositingMode; } \
     void setForceCompositingMode(bool forceCompositingMode) { m_forceCompositingMode = forceCompositingMode; } \
    bool forceFTPDirectoryListings() const { return m_forceFTPDirectoryListings; } \
     void setForceFTPDirectoryListings(bool forceFTPDirectoryListings) { m_forceFTPDirectoryListings = forceFTPDirectoryListings; } \
    bool forceSoftwareWebGLRendering() const { return m_forceSoftwareWebGLRendering; } \
     void setForceSoftwareWebGLRendering(bool forceSoftwareWebGLRendering) { m_forceSoftwareWebGLRendering = forceSoftwareWebGLRendering; } \
    bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting() const { return m_forceUpdateScrollbarsOnMainThreadForPerformanceTesting; } \
     void setForceUpdateScrollbarsOnMainThreadForPerformanceTesting(bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting) { m_forceUpdateScrollbarsOnMainThreadForPerformanceTesting = forceUpdateScrollbarsOnMainThreadForPerformanceTesting; } \
    bool frameFlatteningEnabled() const { return m_frameFlatteningEnabled; } \
     void setFrameFlatteningEnabled(bool frameFlatteningEnabled) { m_frameFlatteningEnabled = frameFlatteningEnabled; } \
    const String& ftpDirectoryTemplatePath() { return m_ftpDirectoryTemplatePath; } \
     void setFTPDirectoryTemplatePath(const String& ftpDirectoryTemplatePath) { m_ftpDirectoryTemplatePath = ftpDirectoryTemplatePath; } \
    bool httpEquivEnabled() const { return m_httpEquivEnabled; } \
     void setHttpEquivEnabled(bool httpEquivEnabled) { m_httpEquivEnabled = httpEquivEnabled; } \
    bool hyperlinkAuditingEnabled() const { return m_hyperlinkAuditingEnabled; } \
     void setHyperlinkAuditingEnabled(bool hyperlinkAuditingEnabled) { m_hyperlinkAuditingEnabled = hyperlinkAuditingEnabled; } \
    bool imageSubsamplingEnabled() const { return m_imageSubsamplingEnabled; } \
     void setImageSubsamplingEnabled(bool imageSubsamplingEnabled) { m_imageSubsamplingEnabled = imageSubsamplingEnabled; } \
    double incrementalRenderingSuppressionTimeoutInSeconds() const { return m_incrementalRenderingSuppressionTimeoutInSeconds; } \
     void setIncrementalRenderingSuppressionTimeoutInSeconds(double incrementalRenderingSuppressionTimeoutInSeconds) { m_incrementalRenderingSuppressionTimeoutInSeconds = incrementalRenderingSuppressionTimeoutInSeconds; } \
    bool inlineMediaPlaybackRequiresPlaysInlineAttribute() const { return m_inlineMediaPlaybackRequiresPlaysInlineAttribute; } \
     void setInlineMediaPlaybackRequiresPlaysInlineAttribute(bool inlineMediaPlaybackRequiresPlaysInlineAttribute) { m_inlineMediaPlaybackRequiresPlaysInlineAttribute = inlineMediaPlaybackRequiresPlaysInlineAttribute; } \
    bool interactiveFormValidationEnabled() const { return m_interactiveFormValidationEnabled; } \
     void setInteractiveFormValidationEnabled(bool interactiveFormValidationEnabled) { m_interactiveFormValidationEnabled = interactiveFormValidationEnabled; } \
    bool invisibleAutoplayNotPermitted() const { return m_invisibleAutoplayNotPermitted; } \
     void setInvisibleAutoplayNotPermitted(bool invisibleAutoplayNotPermitted) { m_invisibleAutoplayNotPermitted = invisibleAutoplayNotPermitted; } \
    bool javaScriptCanAccessClipboard() const { return m_javaScriptCanAccessClipboard; } \
     void setJavaScriptCanAccessClipboard(bool javaScriptCanAccessClipboard) { m_javaScriptCanAccessClipboard = javaScriptCanAccessClipboard; } \
    bool javaScriptCanOpenWindowsAutomatically() const { return m_javaScriptCanOpenWindowsAutomatically; } \
     void setJavaScriptCanOpenWindowsAutomatically(bool javaScriptCanOpenWindowsAutomatically) { m_javaScriptCanOpenWindowsAutomatically = javaScriptCanOpenWindowsAutomatically; } \
    const JSC::RuntimeFlags& javaScriptRuntimeFlags() { return m_javaScriptRuntimeFlags; } \
     void setJavaScriptRuntimeFlags(const JSC::RuntimeFlags& javaScriptRuntimeFlags) { m_javaScriptRuntimeFlags = javaScriptRuntimeFlags; } \
    int layoutFallbackWidth() const { return m_layoutFallbackWidth; } \
     void setLayoutFallbackWidth(int layoutFallbackWidth) { m_layoutFallbackWidth = layoutFallbackWidth; } \
    bool loadDeferringEnabled() const { return m_loadDeferringEnabled; } \
     void setLoadDeferringEnabled(bool loadDeferringEnabled) { m_loadDeferringEnabled = loadDeferringEnabled; } \
    bool loadsSiteIconsIgnoringImageLoadingSetting() const { return m_loadsSiteIconsIgnoringImageLoadingSetting; } \
     void setLoadsSiteIconsIgnoringImageLoadingSetting(bool loadsSiteIconsIgnoringImageLoadingSetting) { m_loadsSiteIconsIgnoringImageLoadingSetting = loadsSiteIconsIgnoringImageLoadingSetting; } \
    bool localFileContentSniffingEnabled() const { return m_localFileContentSniffingEnabled; } \
     void setLocalFileContentSniffingEnabled(bool localFileContentSniffingEnabled) { m_localFileContentSniffingEnabled = localFileContentSniffingEnabled; } \
    const String& localStorageDatabasePath() { return m_localStorageDatabasePath; } \
     void setLocalStorageDatabasePath(const String& localStorageDatabasePath) { m_localStorageDatabasePath = localStorageDatabasePath; } \
    bool localStorageEnabled() const { return m_localStorageEnabled; } \
     void setLocalStorageEnabled(bool localStorageEnabled) { m_localStorageEnabled = localStorageEnabled; } \
    bool logsPageMessagesToSystemConsoleEnabled() const { return m_logsPageMessagesToSystemConsoleEnabled; } \
     void setLogsPageMessagesToSystemConsoleEnabled(bool logsPageMessagesToSystemConsoleEnabled) { m_logsPageMessagesToSystemConsoleEnabled = logsPageMessagesToSystemConsoleEnabled; } \
    bool mainContentUserGestureOverrideEnabled() const { return m_mainContentUserGestureOverrideEnabled; } \
     void setMainContentUserGestureOverrideEnabled(bool mainContentUserGestureOverrideEnabled) { m_mainContentUserGestureOverrideEnabled = mainContentUserGestureOverrideEnabled; } \
    double maxParseDuration() const { return m_maxParseDuration; } \
     void setMaxParseDuration(double maxParseDuration) { m_maxParseDuration = maxParseDuration; } \
    unsigned maximumHTMLParserDOMTreeDepth() const { return m_maximumHTMLParserDOMTreeDepth; } \
     void setMaximumHTMLParserDOMTreeDepth(unsigned maximumHTMLParserDOMTreeDepth) { m_maximumHTMLParserDOMTreeDepth = maximumHTMLParserDOMTreeDepth; } \
    unsigned maximumPlugInSnapshotAttempts() const { return m_maximumPlugInSnapshotAttempts; } \
     void setMaximumPlugInSnapshotAttempts(unsigned maximumPlugInSnapshotAttempts) { m_maximumPlugInSnapshotAttempts = maximumPlugInSnapshotAttempts; } \
    bool mediaControlsScaleWithPageZoom() const { return m_mediaControlsScaleWithPageZoom; } \
     void setMediaControlsScaleWithPageZoom(bool mediaControlsScaleWithPageZoom) { m_mediaControlsScaleWithPageZoom = mediaControlsScaleWithPageZoom; } \
    bool mediaDataLoadsAutomatically() const { return m_mediaDataLoadsAutomatically; } \
     void setMediaDataLoadsAutomatically(bool mediaDataLoadsAutomatically) { m_mediaDataLoadsAutomatically = mediaDataLoadsAutomatically; } \
    bool mediaEnabled() const { return m_mediaEnabled; } \
     void setMediaEnabled(bool mediaEnabled) { m_mediaEnabled = mediaEnabled; } \
    bool mediaStreamEnabled() const { return m_mediaStreamEnabled; } \
     void setMediaStreamEnabled(bool mediaStreamEnabled) { m_mediaStreamEnabled = mediaStreamEnabled; } \
    int minimumAccelerated2dCanvasSize() const { return m_minimumAccelerated2dCanvasSize; } \
     void setMinimumAccelerated2dCanvasSize(int minimumAccelerated2dCanvasSize) { m_minimumAccelerated2dCanvasSize = minimumAccelerated2dCanvasSize; } \
    int minimumFontSize() const { return m_minimumFontSize; } \
    WEBCORE_EXPORT void setMinimumFontSize(int minimumFontSize); \
    int minimumLogicalFontSize() const { return m_minimumLogicalFontSize; } \
    WEBCORE_EXPORT void setMinimumLogicalFontSize(int minimumLogicalFontSize); \
    bool needsIsLoadingInAPISenseQuirk() const { return m_needsIsLoadingInAPISenseQuirk; } \
     void setNeedsIsLoadingInAPISenseQuirk(bool needsIsLoadingInAPISenseQuirk) { m_needsIsLoadingInAPISenseQuirk = needsIsLoadingInAPISenseQuirk; } \
    bool needsKeyboardEventDisambiguationQuirks() const { return m_needsKeyboardEventDisambiguationQuirks; } \
     void setNeedsKeyboardEventDisambiguationQuirks(bool needsKeyboardEventDisambiguationQuirks) { m_needsKeyboardEventDisambiguationQuirks = needsKeyboardEventDisambiguationQuirks; } \
    bool needsSiteSpecificQuirks() const { return m_needsSiteSpecificQuirks; } \
     void setNeedsSiteSpecificQuirks(bool needsSiteSpecificQuirks) { m_needsSiteSpecificQuirks = needsSiteSpecificQuirks; } \
    bool newBlockInsideInlineModelEnabled() const { return m_newBlockInsideInlineModelEnabled; } \
    WEBCORE_EXPORT void setNewBlockInsideInlineModelEnabled(bool newBlockInsideInlineModelEnabled); \
    bool notificationsEnabled() const { return m_notificationsEnabled; } \
     void setNotificationsEnabled(bool notificationsEnabled) { m_notificationsEnabled = notificationsEnabled; } \
    bool offlineWebApplicationCacheEnabled() const { return m_offlineWebApplicationCacheEnabled; } \
     void setOfflineWebApplicationCacheEnabled(bool offlineWebApplicationCacheEnabled) { m_offlineWebApplicationCacheEnabled = offlineWebApplicationCacheEnabled; } \
    bool openGLMultisamplingEnabled() const { return m_openGLMultisamplingEnabled; } \
     void setOpenGLMultisamplingEnabled(bool openGLMultisamplingEnabled) { m_openGLMultisamplingEnabled = openGLMultisamplingEnabled; } \
    bool pageCacheSupportsPlugins() const { return m_pageCacheSupportsPlugins; } \
     void setPageCacheSupportsPlugins(bool pageCacheSupportsPlugins) { m_pageCacheSupportsPlugins = pageCacheSupportsPlugins; } \
    bool paginateDuringLayoutEnabled() const { return m_paginateDuringLayoutEnabled; } \
     void setPaginateDuringLayoutEnabled(bool paginateDuringLayoutEnabled) { m_paginateDuringLayoutEnabled = paginateDuringLayoutEnabled; } \
    double passwordEchoDurationInSeconds() const { return m_passwordEchoDurationInSeconds; } \
     void setPasswordEchoDurationInSeconds(double passwordEchoDurationInSeconds) { m_passwordEchoDurationInSeconds = passwordEchoDurationInSeconds; } \
    bool passwordEchoEnabled() const { return m_passwordEchoEnabled; } \
     void setPasswordEchoEnabled(bool passwordEchoEnabled) { m_passwordEchoEnabled = passwordEchoEnabled; } \
    bool plugInSnapshottingEnabled() const { return m_plugInSnapshottingEnabled; } \
     void setPlugInSnapshottingEnabled(bool plugInSnapshottingEnabled) { m_plugInSnapshottingEnabled = plugInSnapshottingEnabled; } \
    bool preventKeyboardDOMEventDispatch() const { return m_preventKeyboardDOMEventDispatch; } \
     void setPreventKeyboardDOMEventDispatch(bool preventKeyboardDOMEventDispatch) { m_preventKeyboardDOMEventDispatch = preventKeyboardDOMEventDispatch; } \
    bool primaryPlugInSnapshotDetectionEnabled() const { return m_primaryPlugInSnapshotDetectionEnabled; } \
     void setPrimaryPlugInSnapshotDetectionEnabled(bool primaryPlugInSnapshotDetectionEnabled) { m_primaryPlugInSnapshotDetectionEnabled = primaryPlugInSnapshotDetectionEnabled; } \
    bool requestAnimationFrameEnabled() const { return m_requestAnimationFrameEnabled; } \
     void setRequestAnimationFrameEnabled(bool requestAnimationFrameEnabled) { m_requestAnimationFrameEnabled = requestAnimationFrameEnabled; } \
    bool scriptMarkupEnabled() const { return m_scriptMarkupEnabled; } \
     void setScriptMarkupEnabled(bool scriptMarkupEnabled) { m_scriptMarkupEnabled = scriptMarkupEnabled; } \
    bool scrollingCoordinatorEnabled() const { return m_scrollingCoordinatorEnabled; } \
     void setScrollingCoordinatorEnabled(bool scrollingCoordinatorEnabled) { m_scrollingCoordinatorEnabled = scrollingCoordinatorEnabled; } \
    bool scrollingTreeIncludesFrames() const { return m_scrollingTreeIncludesFrames; } \
     void setScrollingTreeIncludesFrames(bool scrollingTreeIncludesFrames) { m_scrollingTreeIncludesFrames = scrollingTreeIncludesFrames; } \
    bool selectTrailingWhitespaceEnabled() const { return m_selectTrailingWhitespaceEnabled; } \
     void setSelectTrailingWhitespaceEnabled(bool selectTrailingWhitespaceEnabled) { m_selectTrailingWhitespaceEnabled = selectTrailingWhitespaceEnabled; } \
    bool selectionIncludesAltImageText() const { return m_selectionIncludesAltImageText; } \
     void setSelectionIncludesAltImageText(bool selectionIncludesAltImageText) { m_selectionIncludesAltImageText = selectionIncludesAltImageText; } \
    bool selectionPaintingWithoutSelectionGapsEnabled() const { return m_selectionPaintingWithoutSelectionGapsEnabled; } \
     void setSelectionPaintingWithoutSelectionGapsEnabled(bool selectionPaintingWithoutSelectionGapsEnabled) { m_selectionPaintingWithoutSelectionGapsEnabled = selectionPaintingWithoutSelectionGapsEnabled; } \
    unsigned sessionStorageQuota() const { return m_sessionStorageQuota; } \
     void setSessionStorageQuota(unsigned sessionStorageQuota) { m_sessionStorageQuota = sessionStorageQuota; } \
    bool shouldConvertInvalidURLsToBlank() const { return m_shouldConvertInvalidURLsToBlank; } \
     void setShouldConvertInvalidURLsToBlank(bool shouldConvertInvalidURLsToBlank) { m_shouldConvertInvalidURLsToBlank = shouldConvertInvalidURLsToBlank; } \
    bool shouldConvertPositionStyleOnCopy() const { return m_shouldConvertPositionStyleOnCopy; } \
     void setShouldConvertPositionStyleOnCopy(bool shouldConvertPositionStyleOnCopy) { m_shouldConvertPositionStyleOnCopy = shouldConvertPositionStyleOnCopy; } \
    bool shouldDispatchJavaScriptWindowOnErrorEvents() const { return m_shouldDispatchJavaScriptWindowOnErrorEvents; } \
     void setShouldDispatchJavaScriptWindowOnErrorEvents(bool shouldDispatchJavaScriptWindowOnErrorEvents) { m_shouldDispatchJavaScriptWindowOnErrorEvents = shouldDispatchJavaScriptWindowOnErrorEvents; } \
    bool shouldInjectUserScriptsInInitialEmptyDocument() const { return m_shouldInjectUserScriptsInInitialEmptyDocument; } \
     void setShouldInjectUserScriptsInInitialEmptyDocument(bool shouldInjectUserScriptsInInitialEmptyDocument) { m_shouldInjectUserScriptsInInitialEmptyDocument = shouldInjectUserScriptsInInitialEmptyDocument; } \
    bool shouldPrintBackgrounds() const { return m_shouldPrintBackgrounds; } \
     void setShouldPrintBackgrounds(bool shouldPrintBackgrounds) { m_shouldPrintBackgrounds = shouldPrintBackgrounds; } \
    bool shouldRespectImageOrientation() const { return m_shouldRespectImageOrientation; } \
     void setShouldRespectImageOrientation(bool shouldRespectImageOrientation) { m_shouldRespectImageOrientation = shouldRespectImageOrientation; } \
    bool shouldTransformsAffectOverflow() const { return m_shouldTransformsAffectOverflow; } \
     void setShouldTransformsAffectOverflow(bool shouldTransformsAffectOverflow) { m_shouldTransformsAffectOverflow = shouldTransformsAffectOverflow; } \
    bool showDebugBorders() const { return m_showDebugBorders; } \
    WEBCORE_EXPORT void setShowDebugBorders(bool showDebugBorders); \
    bool showRepaintCounter() const { return m_showRepaintCounter; } \
    WEBCORE_EXPORT void setShowRepaintCounter(bool showRepaintCounter); \
    bool showsToolTipOverTruncatedText() const { return m_showsToolTipOverTruncatedText; } \
     void setShowsToolTipOverTruncatedText(bool showsToolTipOverTruncatedText) { m_showsToolTipOverTruncatedText = showsToolTipOverTruncatedText; } \
    bool showsURLsInToolTips() const { return m_showsURLsInToolTips; } \
     void setShowsURLsInToolTips(bool showsURLsInToolTips) { m_showsURLsInToolTips = showsURLsInToolTips; } \
    bool shrinksStandaloneImagesToFit() const { return m_shrinksStandaloneImagesToFit; } \
     void setShrinksStandaloneImagesToFit(bool shrinksStandaloneImagesToFit) { m_shrinksStandaloneImagesToFit = shrinksStandaloneImagesToFit; } \
    bool simpleLineLayoutDebugBordersEnabled() const { return m_simpleLineLayoutDebugBordersEnabled; } \
    WEBCORE_EXPORT void setSimpleLineLayoutDebugBordersEnabled(bool simpleLineLayoutDebugBordersEnabled); \
    bool simpleLineLayoutEnabled() const { return m_simpleLineLayoutEnabled; } \
    WEBCORE_EXPORT void setSimpleLineLayoutEnabled(bool simpleLineLayoutEnabled); \
    bool smartInsertDeleteEnabled() const { return m_smartInsertDeleteEnabled; } \
     void setSmartInsertDeleteEnabled(bool smartInsertDeleteEnabled) { m_smartInsertDeleteEnabled = smartInsertDeleteEnabled; } \
    bool snapshotAllPlugIns() const { return m_snapshotAllPlugIns; } \
     void setSnapshotAllPlugIns(bool snapshotAllPlugIns) { m_snapshotAllPlugIns = snapshotAllPlugIns; } \
    bool spatialNavigationEnabled() const { return m_spatialNavigationEnabled; } \
     void setSpatialNavigationEnabled(bool spatialNavigationEnabled) { m_spatialNavigationEnabled = spatialNavigationEnabled; } \
    bool springTimingFunctionEnabled() const { return m_springTimingFunctionEnabled; } \
     void setSpringTimingFunctionEnabled(bool springTimingFunctionEnabled) { m_springTimingFunctionEnabled = springTimingFunctionEnabled; } \
    bool standalone() const { return m_standalone; } \
     void setStandalone(bool standalone) { m_standalone = standalone; } \
    bool subpixelCSSOMElementMetricsEnabled() const { return m_subpixelCSSOMElementMetricsEnabled; } \
     void setSubpixelCSSOMElementMetricsEnabled(bool subpixelCSSOMElementMetricsEnabled) { m_subpixelCSSOMElementMetricsEnabled = subpixelCSSOMElementMetricsEnabled; } \
    bool suppressesIncrementalRendering() const { return m_suppressesIncrementalRendering; } \
     void setSuppressesIncrementalRendering(bool suppressesIncrementalRendering) { m_suppressesIncrementalRendering = suppressesIncrementalRendering; } \
    bool syncXHRInDocumentsEnabled() const { return m_syncXHRInDocumentsEnabled; } \
     void setSyncXHRInDocumentsEnabled(bool syncXHRInDocumentsEnabled) { m_syncXHRInDocumentsEnabled = syncXHRInDocumentsEnabled; } \
    const TextDirection& systemLayoutDirection() { return m_systemLayoutDirection; } \
     void setSystemLayoutDirection(const TextDirection& systemLayoutDirection) { m_systemLayoutDirection = systemLayoutDirection; } \
    bool telephoneNumberParsingEnabled() const { return m_telephoneNumberParsingEnabled; } \
     void setTelephoneNumberParsingEnabled(bool telephoneNumberParsingEnabled) { m_telephoneNumberParsingEnabled = telephoneNumberParsingEnabled; } \
    bool temporaryTileCohortRetentionEnabled() const { return m_temporaryTileCohortRetentionEnabled; } \
     void setTemporaryTileCohortRetentionEnabled(bool temporaryTileCohortRetentionEnabled) { m_temporaryTileCohortRetentionEnabled = temporaryTileCohortRetentionEnabled; } \
    bool textAreasAreResizable() const { return m_textAreasAreResizable; } \
    WEBCORE_EXPORT void setTextAreasAreResizable(bool textAreasAreResizable); \
    const TextDirectionSubmenuInclusionBehavior& textDirectionSubmenuInclusionBehavior() { return m_textDirectionSubmenuInclusionBehavior; } \
     void setTextDirectionSubmenuInclusionBehavior(const TextDirectionSubmenuInclusionBehavior& textDirectionSubmenuInclusionBehavior) { m_textDirectionSubmenuInclusionBehavior = textDirectionSubmenuInclusionBehavior; } \
    bool treatIPAddressAsDomain() const { return m_treatIPAddressAsDomain; } \
     void setTreatIPAddressAsDomain(bool treatIPAddressAsDomain) { m_treatIPAddressAsDomain = treatIPAddressAsDomain; } \
    bool treatsAnyTextCSSLinkAsStylesheet() const { return m_treatsAnyTextCSSLinkAsStylesheet; } \
     void setTreatsAnyTextCSSLinkAsStylesheet(bool treatsAnyTextCSSLinkAsStylesheet) { m_treatsAnyTextCSSLinkAsStylesheet = treatsAnyTextCSSLinkAsStylesheet; } \
    bool unifiedTextCheckerEnabled() const { return m_unifiedTextCheckerEnabled; } \
     void setUnifiedTextCheckerEnabled(bool unifiedTextCheckerEnabled) { m_unifiedTextCheckerEnabled = unifiedTextCheckerEnabled; } \
    bool unsafePluginPastingEnabled() const { return m_unsafePluginPastingEnabled; } \
     void setUnsafePluginPastingEnabled(bool unsafePluginPastingEnabled) { m_unsafePluginPastingEnabled = unsafePluginPastingEnabled; } \
    bool useGiantTiles() const { return m_useGiantTiles; } \
     void setUseGiantTiles(bool useGiantTiles) { m_useGiantTiles = useGiantTiles; } \
    bool useImageDocumentForSubframePDF() const { return m_useImageDocumentForSubframePDF; } \
     void setUseImageDocumentForSubframePDF(bool useImageDocumentForSubframePDF) { m_useImageDocumentForSubframePDF = useImageDocumentForSubframePDF; } \
    bool useLegacyBackgroundSizeShorthandBehavior() const { return m_useLegacyBackgroundSizeShorthandBehavior; } \
     void setUseLegacyBackgroundSizeShorthandBehavior(bool useLegacyBackgroundSizeShorthandBehavior) { m_useLegacyBackgroundSizeShorthandBehavior = useLegacyBackgroundSizeShorthandBehavior; } \
    bool useLegacyTextAlignPositionedElementBehavior() const { return m_useLegacyTextAlignPositionedElementBehavior; } \
     void setUseLegacyTextAlignPositionedElementBehavior(bool useLegacyTextAlignPositionedElementBehavior) { m_useLegacyTextAlignPositionedElementBehavior = useLegacyTextAlignPositionedElementBehavior; } \
    bool usePreHTML5ParserQuirks() const { return m_usePreHTML5ParserQuirks; } \
     void setUsePreHTML5ParserQuirks(bool usePreHTML5ParserQuirks) { m_usePreHTML5ParserQuirks = usePreHTML5ParserQuirks; } \
    const UserInterfaceDirectionPolicy& userInterfaceDirectionPolicy() { return m_userInterfaceDirectionPolicy; } \
     void setUserInterfaceDirectionPolicy(const UserInterfaceDirectionPolicy& userInterfaceDirectionPolicy) { m_userInterfaceDirectionPolicy = userInterfaceDirectionPolicy; } \
    bool usesEncodingDetector() const { return m_usesEncodingDetector; } \
     void setUsesEncodingDetector(bool usesEncodingDetector) { m_usesEncodingDetector = usesEncodingDetector; } \
    int validationMessageTimerMagnification() const { return m_validationMessageTimerMagnification; } \
     void setValidationMessageTimerMagnification(int validationMessageTimerMagnification) { m_validationMessageTimerMagnification = validationMessageTimerMagnification; } \
    bool videoPlaybackRequiresUserGesture() const { return m_videoPlaybackRequiresUserGesture; } \
     void setVideoPlaybackRequiresUserGesture(bool videoPlaybackRequiresUserGesture) { m_videoPlaybackRequiresUserGesture = videoPlaybackRequiresUserGesture; } \
    const DebugOverlayRegions& visibleDebugOverlayRegions() { return m_visibleDebugOverlayRegions; } \
     void setVisibleDebugOverlayRegions(const DebugOverlayRegions& visibleDebugOverlayRegions) { m_visibleDebugOverlayRegions = visibleDebugOverlayRegions; } \
    bool visualViewportEnabled() const { return m_visualViewportEnabled; } \
     void setVisualViewportEnabled(bool visualViewportEnabled) { m_visualViewportEnabled = visualViewportEnabled; } \
    bool wantsBalancedSetDefersLoadingBehavior() const { return m_wantsBalancedSetDefersLoadingBehavior; } \
     void setWantsBalancedSetDefersLoadingBehavior(bool wantsBalancedSetDefersLoadingBehavior) { m_wantsBalancedSetDefersLoadingBehavior = wantsBalancedSetDefersLoadingBehavior; } \
    bool webAudioEnabled() const { return m_webAudioEnabled; } \
     void setWebAudioEnabled(bool webAudioEnabled) { m_webAudioEnabled = webAudioEnabled; } \
    bool webGLEnabled() const { return m_webGLEnabled; } \
     void setWebGLEnabled(bool webGLEnabled) { m_webGLEnabled = webGLEnabled; } \
    bool webGLErrorsToConsoleEnabled() const { return m_webGLErrorsToConsoleEnabled; } \
     void setWebGLErrorsToConsoleEnabled(bool webGLErrorsToConsoleEnabled) { m_webGLErrorsToConsoleEnabled = webGLErrorsToConsoleEnabled; } \
    bool webSecurityEnabled() const { return m_webSecurityEnabled; } \
     void setWebSecurityEnabled(bool webSecurityEnabled) { m_webSecurityEnabled = webSecurityEnabled; } \
    bool windowFocusRestricted() const { return m_windowFocusRestricted; } \
     void setWindowFocusRestricted(bool windowFocusRestricted) { m_windowFocusRestricted = windowFocusRestricted; } \
    bool xssAuditorEnabled() const { return m_xssAuditorEnabled; } \
     void setXSSAuditorEnabled(bool xssAuditorEnabled) { m_xssAuditorEnabled = xssAuditorEnabled; } \
    ATTACHMENT_ELEMENT_SETTINGS_GETTER_AND_SETTERS \
    DASHBOARD_SUPPORT_SETTINGS_GETTER_AND_SETTERS \
    DATA_DETECTION_SETTINGS_GETTER_AND_SETTERS \
    FULLSCREEN_API_SETTINGS_GETTER_AND_SETTERS \
    IOS_TEXT_AUTOSIZING_SETTINGS_GETTER_AND_SETTERS \
    MEDIA_SOURCE_SETTINGS_GETTER_AND_SETTERS \
    RUBBER_BANDING_SETTINGS_GETTER_AND_SETTERS \
    SERVICE_CONTROLS_SETTINGS_GETTER_AND_SETTERS \
    SMOOTH_SCROLLING_SETTINGS_GETTER_AND_SETTERS \
    VIDEO_TRACK_SETTINGS_GETTER_AND_SETTERS \
    WEB_ARCHIVE_SETTINGS_GETTER_AND_SETTERS \
    WIRELESS_PLAYBACK_TARGET_SETTINGS_GETTER_AND_SETTERS \
// End of SETTINGS_GETTERS_AND_SETTERS.

#define SETTINGS_MEMBER_VARIABLES \
    double m_backForwardCacheExpirationInterval; \
    ClipboardAccessPolicy m_clipboardAccessPolicy; \
    int m_defaultFixedFontSize; \
    int m_defaultFontSize; \
    String m_defaultTextEncodingName; \
    String m_defaultVideoPosterURL; \
    int m_deviceHeight; \
    int m_deviceWidth; \
    EditableLinkBehavior m_editableLinkBehavior; \
    EditingBehaviorType m_editingBehaviorType; \
    String m_ftpDirectoryTemplatePath; \
    double m_incrementalRenderingSuppressionTimeoutInSeconds; \
    JSC::RuntimeFlags m_javaScriptRuntimeFlags; \
    int m_layoutFallbackWidth; \
    String m_localStorageDatabasePath; \
    double m_maxParseDuration; \
    unsigned m_maximumHTMLParserDOMTreeDepth; \
    unsigned m_maximumPlugInSnapshotAttempts; \
    int m_minimumAccelerated2dCanvasSize; \
    int m_minimumFontSize; \
    int m_minimumLogicalFontSize; \
    double m_passwordEchoDurationInSeconds; \
    unsigned m_sessionStorageQuota; \
    TextDirection m_systemLayoutDirection; \
    TextDirectionSubmenuInclusionBehavior m_textDirectionSubmenuInclusionBehavior; \
    UserInterfaceDirectionPolicy m_userInterfaceDirectionPolicy; \
    int m_validationMessageTimerMagnification; \
    DebugOverlayRegions m_visibleDebugOverlayRegions; \
    ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    DATA_DETECTION_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    FULLSCREEN_API_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    MEDIA_SOURCE_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    RUBBER_BANDING_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    SERVICE_CONTROLS_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    SMOOTH_SCROLLING_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    VIDEO_TRACK_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    WEB_ARCHIVE_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_MEMBER_VARIABLES \
    bool m_DOMPasteAllowed : 1; \
    bool m_accelerated2dCanvasEnabled : 1; \
    bool m_acceleratedCompositedAnimationsEnabled : 1; \
    bool m_acceleratedCompositingEnabled : 1; \
    bool m_acceleratedCompositingForFixedPositionEnabled : 1; \
    bool m_acceleratedCompositingForOverflowScrollEnabled : 1; \
    bool m_acceleratedDrawingEnabled : 1; \
    bool m_acceleratedFiltersEnabled : 1; \
    bool m_aggressiveTileRetentionEnabled : 1; \
    bool m_allowContentSecurityPolicySourceStarToMatchAnyProtocol : 1; \
    bool m_allowCustomScrollbarInMainFrame : 1; \
    bool m_allowDisplayOfInsecureContent : 1; \
    bool m_allowFileAccessFromFileURLs : 1; \
    bool m_allowMultiElementImplicitSubmission : 1; \
    bool m_allowRunningOfInsecureContent : 1; \
    bool m_allowScriptsToCloseWindows : 1; \
    bool m_allowUniversalAccessFromFileURLs : 1; \
    bool m_allowWindowOpenWithoutUserGesture : 1; \
    bool m_allowsInlineMediaPlayback : 1; \
    bool m_allowsInlineMediaPlaybackAfterFullscreen : 1; \
    bool m_allowsPictureInPictureMediaPlayback : 1; \
    bool m_alwaysUseAcceleratedOverflowScroll : 1; \
    bool m_antialiased2dCanvasEnabled : 1; \
    bool m_appleMailPaginationQuirkEnabled : 1; \
    bool m_asynchronousSpellCheckingEnabled : 1; \
    bool m_audioPlaybackRequiresUserGesture : 1; \
    bool m_authorAndUserStylesEnabled : 1; \
    bool m_autoscrollForDragAndDropEnabled : 1; \
    bool m_autostartOriginPlugInSnapshottingEnabled : 1; \
    bool m_backspaceKeyNavigationEnabled : 1; \
    bool m_canvasUsesAcceleratedDrawing : 1; \
    bool m_caretBrowsingEnabled : 1; \
    bool m_contentDispositionAttachmentSandboxEnabled : 1; \
    bool m_cookieEnabled : 1; \
    bool m_crossOriginCheckInGetMatchedCSSRulesDisabled : 1; \
    bool m_delegatesPageScaling : 1; \
    bool m_developerExtrasEnabled : 1; \
    bool m_diagnosticLoggingEnabled : 1; \
    bool m_displayListDrawingEnabled : 1; \
    bool m_domTimersThrottlingEnabled : 1; \
    bool m_downloadableBinaryFontsEnabled : 1; \
    bool m_enableInheritURIQueryComponent : 1; \
    bool m_enforceCSSMIMETypeInNoQuirksMode : 1; \
    bool m_experimentalNotificationsEnabled : 1; \
    bool m_fixedBackgroundsPaintRelativeToDocument : 1; \
    bool m_fixedElementsLayoutRelativeToFrame : 1; \
    bool m_fixedPositionCreatesStackingContext : 1; \
    bool m_forceCompositingMode : 1; \
    bool m_forceFTPDirectoryListings : 1; \
    bool m_forceSoftwareWebGLRendering : 1; \
    bool m_forceUpdateScrollbarsOnMainThreadForPerformanceTesting : 1; \
    bool m_frameFlatteningEnabled : 1; \
    bool m_httpEquivEnabled : 1; \
    bool m_hyperlinkAuditingEnabled : 1; \
    bool m_imageSubsamplingEnabled : 1; \
    bool m_inlineMediaPlaybackRequiresPlaysInlineAttribute : 1; \
    bool m_interactiveFormValidationEnabled : 1; \
    bool m_invisibleAutoplayNotPermitted : 1; \
    bool m_javaScriptCanAccessClipboard : 1; \
    bool m_javaScriptCanOpenWindowsAutomatically : 1; \
    bool m_loadDeferringEnabled : 1; \
    bool m_loadsSiteIconsIgnoringImageLoadingSetting : 1; \
    bool m_localFileContentSniffingEnabled : 1; \
    bool m_localStorageEnabled : 1; \
    bool m_logsPageMessagesToSystemConsoleEnabled : 1; \
    bool m_mainContentUserGestureOverrideEnabled : 1; \
    bool m_mediaControlsScaleWithPageZoom : 1; \
    bool m_mediaDataLoadsAutomatically : 1; \
    bool m_mediaEnabled : 1; \
    bool m_mediaStreamEnabled : 1; \
    bool m_needsIsLoadingInAPISenseQuirk : 1; \
    bool m_needsKeyboardEventDisambiguationQuirks : 1; \
    bool m_needsSiteSpecificQuirks : 1; \
    bool m_newBlockInsideInlineModelEnabled : 1; \
    bool m_notificationsEnabled : 1; \
    bool m_offlineWebApplicationCacheEnabled : 1; \
    bool m_openGLMultisamplingEnabled : 1; \
    bool m_pageCacheSupportsPlugins : 1; \
    bool m_paginateDuringLayoutEnabled : 1; \
    bool m_passwordEchoEnabled : 1; \
    bool m_plugInSnapshottingEnabled : 1; \
    bool m_preventKeyboardDOMEventDispatch : 1; \
    bool m_primaryPlugInSnapshotDetectionEnabled : 1; \
    bool m_requestAnimationFrameEnabled : 1; \
    bool m_scriptMarkupEnabled : 1; \
    bool m_scrollingCoordinatorEnabled : 1; \
    bool m_scrollingTreeIncludesFrames : 1; \
    bool m_selectTrailingWhitespaceEnabled : 1; \
    bool m_selectionIncludesAltImageText : 1; \
    bool m_selectionPaintingWithoutSelectionGapsEnabled : 1; \
    bool m_shouldConvertInvalidURLsToBlank : 1; \
    bool m_shouldConvertPositionStyleOnCopy : 1; \
    bool m_shouldDispatchJavaScriptWindowOnErrorEvents : 1; \
    bool m_shouldInjectUserScriptsInInitialEmptyDocument : 1; \
    bool m_shouldPrintBackgrounds : 1; \
    bool m_shouldRespectImageOrientation : 1; \
    bool m_shouldTransformsAffectOverflow : 1; \
    bool m_showDebugBorders : 1; \
    bool m_showRepaintCounter : 1; \
    bool m_showsToolTipOverTruncatedText : 1; \
    bool m_showsURLsInToolTips : 1; \
    bool m_shrinksStandaloneImagesToFit : 1; \
    bool m_simpleLineLayoutDebugBordersEnabled : 1; \
    bool m_simpleLineLayoutEnabled : 1; \
    bool m_smartInsertDeleteEnabled : 1; \
    bool m_snapshotAllPlugIns : 1; \
    bool m_spatialNavigationEnabled : 1; \
    bool m_springTimingFunctionEnabled : 1; \
    bool m_standalone : 1; \
    bool m_subpixelCSSOMElementMetricsEnabled : 1; \
    bool m_suppressesIncrementalRendering : 1; \
    bool m_syncXHRInDocumentsEnabled : 1; \
    bool m_telephoneNumberParsingEnabled : 1; \
    bool m_temporaryTileCohortRetentionEnabled : 1; \
    bool m_textAreasAreResizable : 1; \
    bool m_treatIPAddressAsDomain : 1; \
    bool m_treatsAnyTextCSSLinkAsStylesheet : 1; \
    bool m_unifiedTextCheckerEnabled : 1; \
    bool m_unsafePluginPastingEnabled : 1; \
    bool m_useGiantTiles : 1; \
    bool m_useImageDocumentForSubframePDF : 1; \
    bool m_useLegacyBackgroundSizeShorthandBehavior : 1; \
    bool m_useLegacyTextAlignPositionedElementBehavior : 1; \
    bool m_usePreHTML5ParserQuirks : 1; \
    bool m_usesEncodingDetector : 1; \
    bool m_videoPlaybackRequiresUserGesture : 1; \
    bool m_visualViewportEnabled : 1; \
    bool m_wantsBalancedSetDefersLoadingBehavior : 1; \
    bool m_webAudioEnabled : 1; \
    bool m_webGLEnabled : 1; \
    bool m_webGLErrorsToConsoleEnabled : 1; \
    bool m_webSecurityEnabled : 1; \
    bool m_windowFocusRestricted : 1; \
    bool m_xssAuditorEnabled : 1; \
    ATTACHMENT_ELEMENT_SETTINGS_BOOL_MEMBER_VARIABLES \
    DASHBOARD_SUPPORT_SETTINGS_BOOL_MEMBER_VARIABLES \
    DATA_DETECTION_SETTINGS_BOOL_MEMBER_VARIABLES \
    FULLSCREEN_API_SETTINGS_BOOL_MEMBER_VARIABLES \
    IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_MEMBER_VARIABLES \
    MEDIA_SOURCE_SETTINGS_BOOL_MEMBER_VARIABLES \
    RUBBER_BANDING_SETTINGS_BOOL_MEMBER_VARIABLES \
    SERVICE_CONTROLS_SETTINGS_BOOL_MEMBER_VARIABLES \
    SMOOTH_SCROLLING_SETTINGS_BOOL_MEMBER_VARIABLES \
    VIDEO_TRACK_SETTINGS_BOOL_MEMBER_VARIABLES \
    WEB_ARCHIVE_SETTINGS_BOOL_MEMBER_VARIABLES \
    WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_MEMBER_VARIABLES \
// End of SETTINGS_MEMBER_VARIABLES.

#define SETTINGS_INITIALIZER_LIST \
    , m_backForwardCacheExpirationInterval(1800) \
    , m_clipboardAccessPolicy(ClipboardAccessPolicy::RequiresUserGesture) \
    , m_defaultFixedFontSize(0) \
    , m_defaultFontSize(16) \
    , m_deviceHeight(0) \
    , m_deviceWidth(0) \
    , m_editableLinkBehavior(EditableLinkDefaultBehavior) \
    , m_editingBehaviorType(editingBehaviorTypeForPlatform()) \
    , m_incrementalRenderingSuppressionTimeoutInSeconds(defaultIncrementalRenderingSuppressionTimeoutInSeconds) \
    , m_layoutFallbackWidth(980) \
    , m_maxParseDuration(-1) \
    , m_maximumHTMLParserDOMTreeDepth(defaultMaximumHTMLParserDOMTreeDepth) \
    , m_maximumPlugInSnapshotAttempts(20) \
    , m_minimumAccelerated2dCanvasSize(257*256) \
    , m_minimumFontSize(0) \
    , m_minimumLogicalFontSize(0) \
    , m_passwordEchoDurationInSeconds(1) \
    , m_sessionStorageQuota(StorageMap::noQuota) \
    , m_systemLayoutDirection(LTR) \
    , m_textDirectionSubmenuInclusionBehavior(TextDirectionSubmenuAutomaticallyIncluded) \
    , m_userInterfaceDirectionPolicy(UserInterfaceDirectionPolicy::Content) \
    , m_validationMessageTimerMagnification(50) \
    , m_visibleDebugOverlayRegions(0) \
    ATTACHMENT_ELEMENT_SETTINGS_NON_BOOL_INITIALIZERS \
    DASHBOARD_SUPPORT_SETTINGS_NON_BOOL_INITIALIZERS \
    DATA_DETECTION_SETTINGS_NON_BOOL_INITIALIZERS \
    FULLSCREEN_API_SETTINGS_NON_BOOL_INITIALIZERS \
    IOS_TEXT_AUTOSIZING_SETTINGS_NON_BOOL_INITIALIZERS \
    MEDIA_SOURCE_SETTINGS_NON_BOOL_INITIALIZERS \
    RUBBER_BANDING_SETTINGS_NON_BOOL_INITIALIZERS \
    SERVICE_CONTROLS_SETTINGS_NON_BOOL_INITIALIZERS \
    SMOOTH_SCROLLING_SETTINGS_NON_BOOL_INITIALIZERS \
    VIDEO_TRACK_SETTINGS_NON_BOOL_INITIALIZERS \
    WEB_ARCHIVE_SETTINGS_NON_BOOL_INITIALIZERS \
    WIRELESS_PLAYBACK_TARGET_SETTINGS_NON_BOOL_INITIALIZERS \
    , m_DOMPasteAllowed(false) \
    , m_accelerated2dCanvasEnabled(false) \
    , m_acceleratedCompositedAnimationsEnabled(true) \
    , m_acceleratedCompositingEnabled(true) \
    , m_acceleratedCompositingForFixedPositionEnabled(defaultAcceleratedCompositingForFixedPositionEnabled) \
    , m_acceleratedCompositingForOverflowScrollEnabled(false) \
    , m_acceleratedDrawingEnabled(false) \
    , m_acceleratedFiltersEnabled(false) \
    , m_aggressiveTileRetentionEnabled(false) \
    , m_allowContentSecurityPolicySourceStarToMatchAnyProtocol(false) \
    , m_allowCustomScrollbarInMainFrame(true) \
    , m_allowDisplayOfInsecureContent(false) \
    , m_allowFileAccessFromFileURLs(true) \
    , m_allowMultiElementImplicitSubmission(false) \
    , m_allowRunningOfInsecureContent(false) \
    , m_allowScriptsToCloseWindows(false) \
    , m_allowUniversalAccessFromFileURLs(true) \
    , m_allowWindowOpenWithoutUserGesture(false) \
    , m_allowsInlineMediaPlayback(defaultAllowsInlineMediaPlayback) \
    , m_allowsInlineMediaPlaybackAfterFullscreen(true) \
    , m_allowsPictureInPictureMediaPlayback(defaultAllowsPictureInPictureMediaPlayback) \
    , m_alwaysUseAcceleratedOverflowScroll(false) \
    , m_antialiased2dCanvasEnabled(true) \
    , m_appleMailPaginationQuirkEnabled(false) \
    , m_asynchronousSpellCheckingEnabled(false) \
    , m_audioPlaybackRequiresUserGesture(defaultAudioPlaybackRequiresUserGesture) \
    , m_authorAndUserStylesEnabled(true) \
    , m_autoscrollForDragAndDropEnabled(false) \
    , m_autostartOriginPlugInSnapshottingEnabled(true) \
    , m_backspaceKeyNavigationEnabled(true) \
    , m_canvasUsesAcceleratedDrawing(false) \
    , m_caretBrowsingEnabled(false) \
    , m_contentDispositionAttachmentSandboxEnabled(false) \
    , m_cookieEnabled(true) \
    , m_crossOriginCheckInGetMatchedCSSRulesDisabled(false) \
    , m_delegatesPageScaling(false) \
    , m_developerExtrasEnabled(false) \
    , m_diagnosticLoggingEnabled(false) \
    , m_displayListDrawingEnabled(false) \
    , m_domTimersThrottlingEnabled(true) \
    , m_downloadableBinaryFontsEnabled(true) \
    , m_enableInheritURIQueryComponent(false) \
    , m_enforceCSSMIMETypeInNoQuirksMode(true) \
    , m_experimentalNotificationsEnabled(false) \
    , m_fixedBackgroundsPaintRelativeToDocument(defaultFixedBackgroundsPaintRelativeToDocument) \
    , m_fixedElementsLayoutRelativeToFrame(false) \
    , m_fixedPositionCreatesStackingContext(defaultFixedPositionCreatesStackingContext) \
    , m_forceCompositingMode(false) \
    , m_forceFTPDirectoryListings(false) \
    , m_forceSoftwareWebGLRendering(false) \
    , m_forceUpdateScrollbarsOnMainThreadForPerformanceTesting(false) \
    , m_frameFlatteningEnabled(false) \
    , m_httpEquivEnabled(true) \
    , m_hyperlinkAuditingEnabled(false) \
    , m_imageSubsamplingEnabled(defaultImageSubsamplingEnabled) \
    , m_inlineMediaPlaybackRequiresPlaysInlineAttribute(defaultInlineMediaPlaybackRequiresPlaysInlineAttribute) \
    , m_interactiveFormValidationEnabled(false) \
    , m_invisibleAutoplayNotPermitted(false) \
    , m_javaScriptCanAccessClipboard(false) \
    , m_javaScriptCanOpenWindowsAutomatically(false) \
    , m_loadDeferringEnabled(true) \
    , m_loadsSiteIconsIgnoringImageLoadingSetting(false) \
    , m_localFileContentSniffingEnabled(false) \
    , m_localStorageEnabled(false) \
    , m_logsPageMessagesToSystemConsoleEnabled(false) \
    , m_mainContentUserGestureOverrideEnabled(false) \
    , m_mediaControlsScaleWithPageZoom(defaultMediaControlsScaleWithPageZoom) \
    , m_mediaDataLoadsAutomatically(defaultMediaDataLoadsAutomatically) \
    , m_mediaEnabled(true) \
    , m_mediaStreamEnabled(false) \
    , m_needsIsLoadingInAPISenseQuirk(false) \
    , m_needsKeyboardEventDisambiguationQuirks(false) \
    , m_needsSiteSpecificQuirks(false) \
    , m_newBlockInsideInlineModelEnabled(false) \
    , m_notificationsEnabled(true) \
    , m_offlineWebApplicationCacheEnabled(false) \
    , m_openGLMultisamplingEnabled(true) \
    , m_pageCacheSupportsPlugins(false) \
    , m_paginateDuringLayoutEnabled(false) \
    , m_passwordEchoEnabled(false) \
    , m_plugInSnapshottingEnabled(false) \
    , m_preventKeyboardDOMEventDispatch(false) \
    , m_primaryPlugInSnapshotDetectionEnabled(true) \
    , m_requestAnimationFrameEnabled(true) \
    , m_scriptMarkupEnabled(true) \
    , m_scrollingCoordinatorEnabled(false) \
    , m_scrollingTreeIncludesFrames(defaultScrollingTreeIncludesFrames) \
    , m_selectTrailingWhitespaceEnabled(defaultSelectTrailingWhitespaceEnabled) \
    , m_selectionIncludesAltImageText(true) \
    , m_selectionPaintingWithoutSelectionGapsEnabled(false) \
    , m_shouldConvertInvalidURLsToBlank(true) \
    , m_shouldConvertPositionStyleOnCopy(false) \
    , m_shouldDispatchJavaScriptWindowOnErrorEvents(false) \
    , m_shouldInjectUserScriptsInInitialEmptyDocument(false) \
    , m_shouldPrintBackgrounds(false) \
    , m_shouldRespectImageOrientation(defaultShouldRespectImageOrientation) \
    , m_shouldTransformsAffectOverflow(true) \
    , m_showDebugBorders(false) \
    , m_showRepaintCounter(false) \
    , m_showsToolTipOverTruncatedText(false) \
    , m_showsURLsInToolTips(false) \
    , m_shrinksStandaloneImagesToFit(true) \
    , m_simpleLineLayoutDebugBordersEnabled(false) \
    , m_simpleLineLayoutEnabled(true) \
    , m_smartInsertDeleteEnabled(defaultSmartInsertDeleteEnabled) \
    , m_snapshotAllPlugIns(false) \
    , m_spatialNavigationEnabled(false) \
    , m_springTimingFunctionEnabled(false) \
    , m_standalone(false) \
    , m_subpixelCSSOMElementMetricsEnabled(false) \
    , m_suppressesIncrementalRendering(false) \
    , m_syncXHRInDocumentsEnabled(true) \
    , m_telephoneNumberParsingEnabled(false) \
    , m_temporaryTileCohortRetentionEnabled(true) \
    , m_textAreasAreResizable(false) \
    , m_treatIPAddressAsDomain(false) \
    , m_treatsAnyTextCSSLinkAsStylesheet(false) \
    , m_unifiedTextCheckerEnabled(defaultUnifiedTextCheckerEnabled) \
    , m_unsafePluginPastingEnabled(true) \
    , m_useGiantTiles(false) \
    , m_useImageDocumentForSubframePDF(false) \
    , m_useLegacyBackgroundSizeShorthandBehavior(false) \
    , m_useLegacyTextAlignPositionedElementBehavior(false) \
    , m_usePreHTML5ParserQuirks(false) \
    , m_usesEncodingDetector(false) \
    , m_videoPlaybackRequiresUserGesture(defaultVideoPlaybackRequiresUserGesture) \
    , m_visualViewportEnabled(false) \
    , m_wantsBalancedSetDefersLoadingBehavior(false) \
    , m_webAudioEnabled(false) \
    , m_webGLEnabled(false) \
    , m_webGLErrorsToConsoleEnabled(true) \
    , m_webSecurityEnabled(true) \
    , m_windowFocusRestricted(true) \
    , m_xssAuditorEnabled(false) \
    ATTACHMENT_ELEMENT_SETTINGS_BOOL_INITIALIZERS \
    DASHBOARD_SUPPORT_SETTINGS_BOOL_INITIALIZERS \
    DATA_DETECTION_SETTINGS_BOOL_INITIALIZERS \
    FULLSCREEN_API_SETTINGS_BOOL_INITIALIZERS \
    IOS_TEXT_AUTOSIZING_SETTINGS_BOOL_INITIALIZERS \
    MEDIA_SOURCE_SETTINGS_BOOL_INITIALIZERS \
    RUBBER_BANDING_SETTINGS_BOOL_INITIALIZERS \
    SERVICE_CONTROLS_SETTINGS_BOOL_INITIALIZERS \
    SMOOTH_SCROLLING_SETTINGS_BOOL_INITIALIZERS \
    VIDEO_TRACK_SETTINGS_BOOL_INITIALIZERS \
    WEB_ARCHIVE_SETTINGS_BOOL_INITIALIZERS \
    WIRELESS_PLAYBACK_TARGET_SETTINGS_BOOL_INITIALIZERS \
// End of SETTINGS_INITIALIZER_LIST.

#define SETTINGS_SETTER_BODIES \
void Settings::setAcceleratedCompositedAnimationsEnabled(bool acceleratedCompositedAnimationsEnabled) \
{ \
    if (m_acceleratedCompositedAnimationsEnabled == acceleratedCompositedAnimationsEnabled) \
        return; \
    m_acceleratedCompositedAnimationsEnabled = acceleratedCompositedAnimationsEnabled; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setAcceleratedCompositingEnabled(bool acceleratedCompositingEnabled) \
{ \
    if (m_acceleratedCompositingEnabled == acceleratedCompositingEnabled) \
        return; \
    m_acceleratedCompositingEnabled = acceleratedCompositingEnabled; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setAuthorAndUserStylesEnabled(bool authorAndUserStylesEnabled) \
{ \
    if (m_authorAndUserStylesEnabled == authorAndUserStylesEnabled) \
        return; \
    m_authorAndUserStylesEnabled = authorAndUserStylesEnabled; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setDefaultFixedFontSize(int defaultFixedFontSize) \
{ \
    if (m_defaultFixedFontSize == defaultFixedFontSize) \
        return; \
    m_defaultFixedFontSize = defaultFixedFontSize; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setDefaultFontSize(int defaultFontSize) \
{ \
    if (m_defaultFontSize == defaultFontSize) \
        return; \
    m_defaultFontSize = defaultFontSize; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setMinimumFontSize(int minimumFontSize) \
{ \
    if (m_minimumFontSize == minimumFontSize) \
        return; \
    m_minimumFontSize = minimumFontSize; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setMinimumLogicalFontSize(int minimumLogicalFontSize) \
{ \
    if (m_minimumLogicalFontSize == minimumLogicalFontSize) \
        return; \
    m_minimumLogicalFontSize = minimumLogicalFontSize; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setNewBlockInsideInlineModelEnabled(bool newBlockInsideInlineModelEnabled) \
{ \
    if (m_newBlockInsideInlineModelEnabled == newBlockInsideInlineModelEnabled) \
        return; \
    m_newBlockInsideInlineModelEnabled = newBlockInsideInlineModelEnabled; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setShowDebugBorders(bool showDebugBorders) \
{ \
    if (m_showDebugBorders == showDebugBorders) \
        return; \
    m_showDebugBorders = showDebugBorders; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setShowRepaintCounter(bool showRepaintCounter) \
{ \
    if (m_showRepaintCounter == showRepaintCounter) \
        return; \
    m_showRepaintCounter = showRepaintCounter; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setSimpleLineLayoutDebugBordersEnabled(bool simpleLineLayoutDebugBordersEnabled) \
{ \
    if (m_simpleLineLayoutDebugBordersEnabled == simpleLineLayoutDebugBordersEnabled) \
        return; \
    m_simpleLineLayoutDebugBordersEnabled = simpleLineLayoutDebugBordersEnabled; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setSimpleLineLayoutEnabled(bool simpleLineLayoutEnabled) \
{ \
    if (m_simpleLineLayoutEnabled == simpleLineLayoutEnabled) \
        return; \
    m_simpleLineLayoutEnabled = simpleLineLayoutEnabled; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
void Settings::setTextAreasAreResizable(bool textAreasAreResizable) \
{ \
    if (m_textAreasAreResizable == textAreasAreResizable) \
        return; \
    m_textAreasAreResizable = textAreasAreResizable; \
    m_page->setNeedsRecalcStyleInAllFrames(); \
} \
    ATTACHMENT_ELEMENT_SETTINGS_SETTER_BODIES \
    DASHBOARD_SUPPORT_SETTINGS_SETTER_BODIES \
    DATA_DETECTION_SETTINGS_SETTER_BODIES \
    FULLSCREEN_API_SETTINGS_SETTER_BODIES \
    IOS_TEXT_AUTOSIZING_SETTINGS_SETTER_BODIES \
    MEDIA_SOURCE_SETTINGS_SETTER_BODIES \
    RUBBER_BANDING_SETTINGS_SETTER_BODIES \
    SERVICE_CONTROLS_SETTINGS_SETTER_BODIES \
    SMOOTH_SCROLLING_SETTINGS_SETTER_BODIES \
    VIDEO_TRACK_SETTINGS_SETTER_BODIES \
    WEB_ARCHIVE_SETTINGS_SETTER_BODIES \
    WIRELESS_PLAYBACK_TARGET_SETTINGS_SETTER_BODIES \
// End of SETTINGS_SETTER_BODIES.

#endif // SettingsMacros_h
