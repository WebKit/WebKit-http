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

#ifndef InternalSettingsGenerated_h
#define InternalSettingsGenerated_h

#include "Supplementable.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Page;

class InternalSettingsGenerated : public RefCounted<InternalSettingsGenerated> {
public:
    explicit InternalSettingsGenerated(Page*);
    virtual ~InternalSettingsGenerated();
    void resetToConsistentState();
    void setDOMPasteAllowed(bool DOMPasteAllowed);
    void setAccelerated2dCanvasEnabled(bool accelerated2dCanvasEnabled);
    void setAcceleratedCompositedAnimationsEnabled(bool acceleratedCompositedAnimationsEnabled);
    void setAcceleratedCompositingEnabled(bool acceleratedCompositingEnabled);
    void setAcceleratedCompositingForFixedPositionEnabled(bool acceleratedCompositingForFixedPositionEnabled);
    void setAcceleratedCompositingForOverflowScrollEnabled(bool acceleratedCompositingForOverflowScrollEnabled);
    void setAcceleratedDrawingEnabled(bool acceleratedDrawingEnabled);
    void setAcceleratedFiltersEnabled(bool acceleratedFiltersEnabled);
    void setAggressiveTileRetentionEnabled(bool aggressiveTileRetentionEnabled);
    void setAllowContentSecurityPolicySourceStarToMatchAnyProtocol(bool allowContentSecurityPolicySourceStarToMatchAnyProtocol);
    void setAllowCustomScrollbarInMainFrame(bool allowCustomScrollbarInMainFrame);
    void setAllowDisplayOfInsecureContent(bool allowDisplayOfInsecureContent);
    void setAllowFileAccessFromFileURLs(bool allowFileAccessFromFileURLs);
    void setAllowMultiElementImplicitSubmission(bool allowMultiElementImplicitSubmission);
    void setAllowRunningOfInsecureContent(bool allowRunningOfInsecureContent);
    void setAllowScriptsToCloseWindows(bool allowScriptsToCloseWindows);
    void setAllowUniversalAccessFromFileURLs(bool allowUniversalAccessFromFileURLs);
    void setAllowWindowOpenWithoutUserGesture(bool allowWindowOpenWithoutUserGesture);
    void setAllowsAirPlayForMediaPlayback(bool allowsAirPlayForMediaPlayback);
    void setAllowsInlineMediaPlayback(bool allowsInlineMediaPlayback);
    void setAllowsInlineMediaPlaybackAfterFullscreen(bool allowsInlineMediaPlaybackAfterFullscreen);
    void setAllowsPictureInPictureMediaPlayback(bool allowsPictureInPictureMediaPlayback);
    void setAlwaysUseAcceleratedOverflowScroll(bool alwaysUseAcceleratedOverflowScroll);
    void setAntialiased2dCanvasEnabled(bool antialiased2dCanvasEnabled);
    void setAppleMailPaginationQuirkEnabled(bool appleMailPaginationQuirkEnabled);
    void setAsynchronousSpellCheckingEnabled(bool asynchronousSpellCheckingEnabled);
    void setAttachmentElementEnabled(bool attachmentElementEnabled);
    void setAudioPlaybackRequiresUserGesture(bool audioPlaybackRequiresUserGesture);
    void setAuthorAndUserStylesEnabled(bool authorAndUserStylesEnabled);
    void setAutoscrollForDragAndDropEnabled(bool autoscrollForDragAndDropEnabled);
    void setAutostartOriginPlugInSnapshottingEnabled(bool autostartOriginPlugInSnapshottingEnabled);
    void setBackForwardCacheExpirationInterval(double backForwardCacheExpirationInterval);
    void setBackspaceKeyNavigationEnabled(bool backspaceKeyNavigationEnabled);
    void setCanvasUsesAcceleratedDrawing(bool canvasUsesAcceleratedDrawing);
    void setCaretBrowsingEnabled(bool caretBrowsingEnabled);
    void setContentDispositionAttachmentSandboxEnabled(bool contentDispositionAttachmentSandboxEnabled);
    void setCookieEnabled(bool cookieEnabled);
    void setCrossOriginCheckInGetMatchedCSSRulesDisabled(bool crossOriginCheckInGetMatchedCSSRulesDisabled);
    void setDefaultFixedFontSize(int defaultFixedFontSize);
    void setDefaultFontSize(int defaultFontSize);
    void setDefaultTextEncodingName(const String& defaultTextEncodingName);
    void setDefaultVideoPosterURL(const String& defaultVideoPosterURL);
    void setDelegatesPageScaling(bool delegatesPageScaling);
    void setDeveloperExtrasEnabled(bool developerExtrasEnabled);
    void setDeviceHeight(int deviceHeight);
    void setDeviceWidth(int deviceWidth);
    void setDiagnosticLoggingEnabled(bool diagnosticLoggingEnabled);
    void setDisplayListDrawingEnabled(bool displayListDrawingEnabled);
    void setDOMTimersThrottlingEnabled(bool domTimersThrottlingEnabled);
    void setDownloadableBinaryFontsEnabled(bool downloadableBinaryFontsEnabled);
    void setEnableInheritURIQueryComponent(bool enableInheritURIQueryComponent);
    void setEnforceCSSMIMETypeInNoQuirksMode(bool enforceCSSMIMETypeInNoQuirksMode);
    void setExperimentalNotificationsEnabled(bool experimentalNotificationsEnabled);
    void setFixedBackgroundsPaintRelativeToDocument(bool fixedBackgroundsPaintRelativeToDocument);
    void setFixedElementsLayoutRelativeToFrame(bool fixedElementsLayoutRelativeToFrame);
    void setFixedPositionCreatesStackingContext(bool fixedPositionCreatesStackingContext);
    void setForceCompositingMode(bool forceCompositingMode);
    void setForceFTPDirectoryListings(bool forceFTPDirectoryListings);
    void setForceSoftwareWebGLRendering(bool forceSoftwareWebGLRendering);
    void setForceUpdateScrollbarsOnMainThreadForPerformanceTesting(bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting);
    void setFrameFlatteningEnabled(bool frameFlatteningEnabled);
    void setFTPDirectoryTemplatePath(const String& ftpDirectoryTemplatePath);
    void setFullScreenEnabled(bool fullScreenEnabled);
    void setHttpEquivEnabled(bool httpEquivEnabled);
    void setHyperlinkAuditingEnabled(bool hyperlinkAuditingEnabled);
    void setImageControlsEnabled(bool imageControlsEnabled);
    void setImageSubsamplingEnabled(bool imageSubsamplingEnabled);
    void setIncrementalRenderingSuppressionTimeoutInSeconds(double incrementalRenderingSuppressionTimeoutInSeconds);
    void setInlineMediaPlaybackRequiresPlaysInlineAttribute(bool inlineMediaPlaybackRequiresPlaysInlineAttribute);
    void setInteractiveFormValidationEnabled(bool interactiveFormValidationEnabled);
    void setInvisibleAutoplayNotPermitted(bool invisibleAutoplayNotPermitted);
    void setJavaScriptCanAccessClipboard(bool javaScriptCanAccessClipboard);
    void setJavaScriptCanOpenWindowsAutomatically(bool javaScriptCanOpenWindowsAutomatically);
    void setLayoutFallbackWidth(int layoutFallbackWidth);
    void setLoadDeferringEnabled(bool loadDeferringEnabled);
    void setLoadsSiteIconsIgnoringImageLoadingSetting(bool loadsSiteIconsIgnoringImageLoadingSetting);
    void setLocalFileContentSniffingEnabled(bool localFileContentSniffingEnabled);
    void setLocalStorageDatabasePath(const String& localStorageDatabasePath);
    void setLocalStorageEnabled(bool localStorageEnabled);
    void setLogsPageMessagesToSystemConsoleEnabled(bool logsPageMessagesToSystemConsoleEnabled);
    void setMainContentUserGestureOverrideEnabled(bool mainContentUserGestureOverrideEnabled);
    void setMaxParseDuration(double maxParseDuration);
    void setMaximumHTMLParserDOMTreeDepth(unsigned maximumHTMLParserDOMTreeDepth);
    void setMaximumPlugInSnapshotAttempts(unsigned maximumPlugInSnapshotAttempts);
    void setMaximumSourceBufferSize(int maximumSourceBufferSize);
    void setMediaControlsScaleWithPageZoom(bool mediaControlsScaleWithPageZoom);
    void setMediaDataLoadsAutomatically(bool mediaDataLoadsAutomatically);
    void setMediaEnabled(bool mediaEnabled);
    void setMediaSourceEnabled(bool mediaSourceEnabled);
    void setMediaStreamEnabled(bool mediaStreamEnabled);
    void setMinimumAccelerated2dCanvasSize(int minimumAccelerated2dCanvasSize);
    void setMinimumFontSize(int minimumFontSize);
    void setMinimumLogicalFontSize(int minimumLogicalFontSize);
    void setMinimumZoomFontSize(float minimumZoomFontSize);
    void setNeedsIsLoadingInAPISenseQuirk(bool needsIsLoadingInAPISenseQuirk);
    void setNeedsKeyboardEventDisambiguationQuirks(bool needsKeyboardEventDisambiguationQuirks);
    void setNeedsSiteSpecificQuirks(bool needsSiteSpecificQuirks);
    void setNewBlockInsideInlineModelEnabled(bool newBlockInsideInlineModelEnabled);
    void setNotificationsEnabled(bool notificationsEnabled);
    void setOfflineWebApplicationCacheEnabled(bool offlineWebApplicationCacheEnabled);
    void setOpenGLMultisamplingEnabled(bool openGLMultisamplingEnabled);
    void setPageCacheSupportsPlugins(bool pageCacheSupportsPlugins);
    void setPaginateDuringLayoutEnabled(bool paginateDuringLayoutEnabled);
    void setPasswordEchoDurationInSeconds(double passwordEchoDurationInSeconds);
    void setPasswordEchoEnabled(bool passwordEchoEnabled);
    void setPlugInSnapshottingEnabled(bool plugInSnapshottingEnabled);
    void setPreventKeyboardDOMEventDispatch(bool preventKeyboardDOMEventDispatch);
    void setPrimaryPlugInSnapshotDetectionEnabled(bool primaryPlugInSnapshotDetectionEnabled);
    void setRequestAnimationFrameEnabled(bool requestAnimationFrameEnabled);
    void setRubberBandingForSubScrollableRegionsEnabled(bool rubberBandingForSubScrollableRegionsEnabled);
    void setScriptMarkupEnabled(bool scriptMarkupEnabled);
    void setScrollAnimatorEnabled(bool scrollAnimatorEnabled);
    void setScrollingCoordinatorEnabled(bool scrollingCoordinatorEnabled);
    void setScrollingTreeIncludesFrames(bool scrollingTreeIncludesFrames);
    void setSelectTrailingWhitespaceEnabled(bool selectTrailingWhitespaceEnabled);
    void setSelectionIncludesAltImageText(bool selectionIncludesAltImageText);
    void setSelectionPaintingWithoutSelectionGapsEnabled(bool selectionPaintingWithoutSelectionGapsEnabled);
    void setServiceControlsEnabled(bool serviceControlsEnabled);
    void setSessionStorageQuota(unsigned sessionStorageQuota);
    void setShouldConvertInvalidURLsToBlank(bool shouldConvertInvalidURLsToBlank);
    void setShouldConvertPositionStyleOnCopy(bool shouldConvertPositionStyleOnCopy);
    void setShouldDispatchJavaScriptWindowOnErrorEvents(bool shouldDispatchJavaScriptWindowOnErrorEvents);
    void setShouldDisplayCaptions(bool shouldDisplayCaptions);
    void setShouldDisplaySubtitles(bool shouldDisplaySubtitles);
    void setShouldDisplayTextDescriptions(bool shouldDisplayTextDescriptions);
    void setShouldInjectUserScriptsInInitialEmptyDocument(bool shouldInjectUserScriptsInInitialEmptyDocument);
    void setShouldPrintBackgrounds(bool shouldPrintBackgrounds);
    void setShouldRespectImageOrientation(bool shouldRespectImageOrientation);
    void setShouldTransformsAffectOverflow(bool shouldTransformsAffectOverflow);
    void setShowDebugBorders(bool showDebugBorders);
    void setShowRepaintCounter(bool showRepaintCounter);
    void setShowsToolTipOverTruncatedText(bool showsToolTipOverTruncatedText);
    void setShowsURLsInToolTips(bool showsURLsInToolTips);
    void setShrinksStandaloneImagesToFit(bool shrinksStandaloneImagesToFit);
    void setSimpleLineLayoutDebugBordersEnabled(bool simpleLineLayoutDebugBordersEnabled);
    void setSimpleLineLayoutEnabled(bool simpleLineLayoutEnabled);
    void setSmartInsertDeleteEnabled(bool smartInsertDeleteEnabled);
    void setSnapshotAllPlugIns(bool snapshotAllPlugIns);
    void setSpatialNavigationEnabled(bool spatialNavigationEnabled);
    void setSpringTimingFunctionEnabled(bool springTimingFunctionEnabled);
    void setStandalone(bool standalone);
    void setSubpixelCSSOMElementMetricsEnabled(bool subpixelCSSOMElementMetricsEnabled);
    void setSuppressesIncrementalRendering(bool suppressesIncrementalRendering);
    void setSyncXHRInDocumentsEnabled(bool syncXHRInDocumentsEnabled);
    void setTelephoneNumberParsingEnabled(bool telephoneNumberParsingEnabled);
    void setTemporaryTileCohortRetentionEnabled(bool temporaryTileCohortRetentionEnabled);
    void setTextAreasAreResizable(bool textAreasAreResizable);
    void setTextAutosizingEnabled(bool textAutosizingEnabled);
    void setTreatIPAddressAsDomain(bool treatIPAddressAsDomain);
    void setTreatsAnyTextCSSLinkAsStylesheet(bool treatsAnyTextCSSLinkAsStylesheet);
    void setUnifiedTextCheckerEnabled(bool unifiedTextCheckerEnabled);
    void setUnsafePluginPastingEnabled(bool unsafePluginPastingEnabled);
    void setUseGiantTiles(bool useGiantTiles);
    void setUseImageDocumentForSubframePDF(bool useImageDocumentForSubframePDF);
    void setUseLegacyBackgroundSizeShorthandBehavior(bool useLegacyBackgroundSizeShorthandBehavior);
    void setUseLegacyTextAlignPositionedElementBehavior(bool useLegacyTextAlignPositionedElementBehavior);
    void setUsePreHTML5ParserQuirks(bool usePreHTML5ParserQuirks);
    void setUsesDashboardBackwardCompatibilityMode(bool usesDashboardBackwardCompatibilityMode);
    void setUsesEncodingDetector(bool usesEncodingDetector);
    void setValidationMessageTimerMagnification(int validationMessageTimerMagnification);
    void setVideoPlaybackRequiresUserGesture(bool videoPlaybackRequiresUserGesture);
    void setVisualViewportEnabled(bool visualViewportEnabled);
    void setWantsBalancedSetDefersLoadingBehavior(bool wantsBalancedSetDefersLoadingBehavior);
    void setWebArchiveDebugModeEnabled(bool webArchiveDebugModeEnabled);
    void setWebAudioEnabled(bool webAudioEnabled);
    void setWebGLEnabled(bool webGLEnabled);
    void setWebGLErrorsToConsoleEnabled(bool webGLErrorsToConsoleEnabled);
    void setWebSecurityEnabled(bool webSecurityEnabled);
    void setWindowFocusRestricted(bool windowFocusRestricted);
    void setXSSAuditorEnabled(bool xssAuditorEnabled);

private:
    Page* m_page;

    bool m_DOMPasteAllowed;
    bool m_accelerated2dCanvasEnabled;
    bool m_acceleratedCompositedAnimationsEnabled;
    bool m_acceleratedCompositingEnabled;
    bool m_acceleratedCompositingForFixedPositionEnabled;
    bool m_acceleratedCompositingForOverflowScrollEnabled;
    bool m_acceleratedDrawingEnabled;
    bool m_acceleratedFiltersEnabled;
    bool m_aggressiveTileRetentionEnabled;
    bool m_allowContentSecurityPolicySourceStarToMatchAnyProtocol;
    bool m_allowCustomScrollbarInMainFrame;
    bool m_allowDisplayOfInsecureContent;
    bool m_allowFileAccessFromFileURLs;
    bool m_allowMultiElementImplicitSubmission;
    bool m_allowRunningOfInsecureContent;
    bool m_allowScriptsToCloseWindows;
    bool m_allowUniversalAccessFromFileURLs;
    bool m_allowWindowOpenWithoutUserGesture;
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    bool m_allowsAirPlayForMediaPlayback;
#endif
    bool m_allowsInlineMediaPlayback;
    bool m_allowsInlineMediaPlaybackAfterFullscreen;
    bool m_allowsPictureInPictureMediaPlayback;
    bool m_alwaysUseAcceleratedOverflowScroll;
    bool m_antialiased2dCanvasEnabled;
    bool m_appleMailPaginationQuirkEnabled;
    bool m_asynchronousSpellCheckingEnabled;
#if ENABLE(ATTACHMENT_ELEMENT)
    bool m_attachmentElementEnabled;
#endif
    bool m_audioPlaybackRequiresUserGesture;
    bool m_authorAndUserStylesEnabled;
    bool m_autoscrollForDragAndDropEnabled;
    bool m_autostartOriginPlugInSnapshottingEnabled;
    double m_backForwardCacheExpirationInterval;
    bool m_backspaceKeyNavigationEnabled;
    bool m_canvasUsesAcceleratedDrawing;
    bool m_caretBrowsingEnabled;
    bool m_contentDispositionAttachmentSandboxEnabled;
    bool m_cookieEnabled;
    bool m_crossOriginCheckInGetMatchedCSSRulesDisabled;
    int m_defaultFixedFontSize;
    int m_defaultFontSize;
    String m_defaultTextEncodingName;
    String m_defaultVideoPosterURL;
    bool m_delegatesPageScaling;
    bool m_developerExtrasEnabled;
    int m_deviceHeight;
    int m_deviceWidth;
    bool m_diagnosticLoggingEnabled;
    bool m_displayListDrawingEnabled;
    bool m_domTimersThrottlingEnabled;
    bool m_downloadableBinaryFontsEnabled;
    bool m_enableInheritURIQueryComponent;
    bool m_enforceCSSMIMETypeInNoQuirksMode;
    bool m_experimentalNotificationsEnabled;
    bool m_fixedBackgroundsPaintRelativeToDocument;
    bool m_fixedElementsLayoutRelativeToFrame;
    bool m_fixedPositionCreatesStackingContext;
    bool m_forceCompositingMode;
    bool m_forceFTPDirectoryListings;
    bool m_forceSoftwareWebGLRendering;
    bool m_forceUpdateScrollbarsOnMainThreadForPerformanceTesting;
    bool m_frameFlatteningEnabled;
    String m_ftpDirectoryTemplatePath;
#if ENABLE(FULLSCREEN_API)
    bool m_fullScreenEnabled;
#endif
    bool m_httpEquivEnabled;
    bool m_hyperlinkAuditingEnabled;
#if ENABLE(SERVICE_CONTROLS)
    bool m_imageControlsEnabled;
#endif
    bool m_imageSubsamplingEnabled;
    double m_incrementalRenderingSuppressionTimeoutInSeconds;
    bool m_inlineMediaPlaybackRequiresPlaysInlineAttribute;
    bool m_interactiveFormValidationEnabled;
    bool m_invisibleAutoplayNotPermitted;
    bool m_javaScriptCanAccessClipboard;
    bool m_javaScriptCanOpenWindowsAutomatically;
    int m_layoutFallbackWidth;
    bool m_loadDeferringEnabled;
    bool m_loadsSiteIconsIgnoringImageLoadingSetting;
    bool m_localFileContentSniffingEnabled;
    String m_localStorageDatabasePath;
    bool m_localStorageEnabled;
    bool m_logsPageMessagesToSystemConsoleEnabled;
    bool m_mainContentUserGestureOverrideEnabled;
    double m_maxParseDuration;
    unsigned m_maximumHTMLParserDOMTreeDepth;
    unsigned m_maximumPlugInSnapshotAttempts;
#if ENABLE(MEDIA_SOURCE)
    int m_maximumSourceBufferSize;
#endif
    bool m_mediaControlsScaleWithPageZoom;
    bool m_mediaDataLoadsAutomatically;
    bool m_mediaEnabled;
#if ENABLE(MEDIA_SOURCE)
    bool m_mediaSourceEnabled;
#endif
    bool m_mediaStreamEnabled;
    int m_minimumAccelerated2dCanvasSize;
    int m_minimumFontSize;
    int m_minimumLogicalFontSize;
#if ENABLE(IOS_TEXT_AUTOSIZING)
    float m_minimumZoomFontSize;
#endif
    bool m_needsIsLoadingInAPISenseQuirk;
    bool m_needsKeyboardEventDisambiguationQuirks;
    bool m_needsSiteSpecificQuirks;
    bool m_newBlockInsideInlineModelEnabled;
    bool m_notificationsEnabled;
    bool m_offlineWebApplicationCacheEnabled;
    bool m_openGLMultisamplingEnabled;
    bool m_pageCacheSupportsPlugins;
    bool m_paginateDuringLayoutEnabled;
    double m_passwordEchoDurationInSeconds;
    bool m_passwordEchoEnabled;
    bool m_plugInSnapshottingEnabled;
    bool m_preventKeyboardDOMEventDispatch;
    bool m_primaryPlugInSnapshotDetectionEnabled;
    bool m_requestAnimationFrameEnabled;
#if ENABLE(RUBBER_BANDING)
    bool m_rubberBandingForSubScrollableRegionsEnabled;
#endif
    bool m_scriptMarkupEnabled;
#if ENABLE(SMOOTH_SCROLLING)
    bool m_scrollAnimatorEnabled;
#endif
    bool m_scrollingCoordinatorEnabled;
    bool m_scrollingTreeIncludesFrames;
    bool m_selectTrailingWhitespaceEnabled;
    bool m_selectionIncludesAltImageText;
    bool m_selectionPaintingWithoutSelectionGapsEnabled;
#if ENABLE(SERVICE_CONTROLS)
    bool m_serviceControlsEnabled;
#endif
    unsigned m_sessionStorageQuota;
    bool m_shouldConvertInvalidURLsToBlank;
    bool m_shouldConvertPositionStyleOnCopy;
    bool m_shouldDispatchJavaScriptWindowOnErrorEvents;
#if ENABLE(VIDEO_TRACK)
    bool m_shouldDisplayCaptions;
#endif
#if ENABLE(VIDEO_TRACK)
    bool m_shouldDisplaySubtitles;
#endif
#if ENABLE(VIDEO_TRACK)
    bool m_shouldDisplayTextDescriptions;
#endif
    bool m_shouldInjectUserScriptsInInitialEmptyDocument;
    bool m_shouldPrintBackgrounds;
    bool m_shouldRespectImageOrientation;
    bool m_shouldTransformsAffectOverflow;
    bool m_showDebugBorders;
    bool m_showRepaintCounter;
    bool m_showsToolTipOverTruncatedText;
    bool m_showsURLsInToolTips;
    bool m_shrinksStandaloneImagesToFit;
    bool m_simpleLineLayoutDebugBordersEnabled;
    bool m_simpleLineLayoutEnabled;
    bool m_smartInsertDeleteEnabled;
    bool m_snapshotAllPlugIns;
    bool m_spatialNavigationEnabled;
    bool m_springTimingFunctionEnabled;
    bool m_standalone;
    bool m_subpixelCSSOMElementMetricsEnabled;
    bool m_suppressesIncrementalRendering;
    bool m_syncXHRInDocumentsEnabled;
    bool m_telephoneNumberParsingEnabled;
    bool m_temporaryTileCohortRetentionEnabled;
    bool m_textAreasAreResizable;
#if ENABLE(IOS_TEXT_AUTOSIZING)
    bool m_textAutosizingEnabled;
#endif
    bool m_treatIPAddressAsDomain;
    bool m_treatsAnyTextCSSLinkAsStylesheet;
    bool m_unifiedTextCheckerEnabled;
    bool m_unsafePluginPastingEnabled;
    bool m_useGiantTiles;
    bool m_useImageDocumentForSubframePDF;
    bool m_useLegacyBackgroundSizeShorthandBehavior;
    bool m_useLegacyTextAlignPositionedElementBehavior;
    bool m_usePreHTML5ParserQuirks;
#if ENABLE(DASHBOARD_SUPPORT)
    bool m_usesDashboardBackwardCompatibilityMode;
#endif
    bool m_usesEncodingDetector;
    int m_validationMessageTimerMagnification;
    bool m_videoPlaybackRequiresUserGesture;
    bool m_visualViewportEnabled;
    bool m_wantsBalancedSetDefersLoadingBehavior;
#if ENABLE(WEB_ARCHIVE)
    bool m_webArchiveDebugModeEnabled;
#endif
    bool m_webAudioEnabled;
    bool m_webGLEnabled;
    bool m_webGLErrorsToConsoleEnabled;
    bool m_webSecurityEnabled;
    bool m_windowFocusRestricted;
    bool m_xssAuditorEnabled;
};

} // namespace WebCore
#endif // InternalSettingsGenerated_h
