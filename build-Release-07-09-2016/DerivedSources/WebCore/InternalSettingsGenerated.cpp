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

#include "config.h"
#include "InternalSettingsGenerated.h"

#include "Page.h"
#include "Settings.h"

namespace WebCore {

InternalSettingsGenerated::InternalSettingsGenerated(Page* page)
    : m_page(page)
    , m_DOMPasteAllowed(page->settings().DOMPasteAllowed())
    , m_accelerated2dCanvasEnabled(page->settings().accelerated2dCanvasEnabled())
    , m_acceleratedCompositedAnimationsEnabled(page->settings().acceleratedCompositedAnimationsEnabled())
    , m_acceleratedCompositingEnabled(page->settings().acceleratedCompositingEnabled())
    , m_acceleratedCompositingForFixedPositionEnabled(page->settings().acceleratedCompositingForFixedPositionEnabled())
    , m_acceleratedCompositingForOverflowScrollEnabled(page->settings().acceleratedCompositingForOverflowScrollEnabled())
    , m_acceleratedDrawingEnabled(page->settings().acceleratedDrawingEnabled())
    , m_acceleratedFiltersEnabled(page->settings().acceleratedFiltersEnabled())
    , m_aggressiveTileRetentionEnabled(page->settings().aggressiveTileRetentionEnabled())
    , m_allowContentSecurityPolicySourceStarToMatchAnyProtocol(page->settings().allowContentSecurityPolicySourceStarToMatchAnyProtocol())
    , m_allowCustomScrollbarInMainFrame(page->settings().allowCustomScrollbarInMainFrame())
    , m_allowDisplayOfInsecureContent(page->settings().allowDisplayOfInsecureContent())
    , m_allowFileAccessFromFileURLs(page->settings().allowFileAccessFromFileURLs())
    , m_allowMultiElementImplicitSubmission(page->settings().allowMultiElementImplicitSubmission())
    , m_allowRunningOfInsecureContent(page->settings().allowRunningOfInsecureContent())
    , m_allowScriptsToCloseWindows(page->settings().allowScriptsToCloseWindows())
    , m_allowUniversalAccessFromFileURLs(page->settings().allowUniversalAccessFromFileURLs())
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , m_allowsAirPlayForMediaPlayback(page->settings().allowsAirPlayForMediaPlayback())
#endif
    , m_allowsInlineMediaPlayback(page->settings().allowsInlineMediaPlayback())
    , m_allowsInlineMediaPlaybackAfterFullscreen(page->settings().allowsInlineMediaPlaybackAfterFullscreen())
    , m_allowsPictureInPictureMediaPlayback(page->settings().allowsPictureInPictureMediaPlayback())
    , m_alwaysUseAcceleratedOverflowScroll(page->settings().alwaysUseAcceleratedOverflowScroll())
    , m_antialiased2dCanvasEnabled(page->settings().antialiased2dCanvasEnabled())
    , m_appleMailPaginationQuirkEnabled(page->settings().appleMailPaginationQuirkEnabled())
    , m_asynchronousSpellCheckingEnabled(page->settings().asynchronousSpellCheckingEnabled())
#if ENABLE(ATTACHMENT_ELEMENT)
    , m_attachmentElementEnabled(page->settings().attachmentElementEnabled())
#endif
    , m_audioPlaybackRequiresUserGesture(page->settings().audioPlaybackRequiresUserGesture())
    , m_authorAndUserStylesEnabled(page->settings().authorAndUserStylesEnabled())
    , m_autoscrollForDragAndDropEnabled(page->settings().autoscrollForDragAndDropEnabled())
    , m_autostartOriginPlugInSnapshottingEnabled(page->settings().autostartOriginPlugInSnapshottingEnabled())
    , m_backForwardCacheExpirationInterval(page->settings().backForwardCacheExpirationInterval())
    , m_backspaceKeyNavigationEnabled(page->settings().backspaceKeyNavigationEnabled())
    , m_canvasUsesAcceleratedDrawing(page->settings().canvasUsesAcceleratedDrawing())
    , m_caretBrowsingEnabled(page->settings().caretBrowsingEnabled())
    , m_contentDispositionAttachmentSandboxEnabled(page->settings().contentDispositionAttachmentSandboxEnabled())
    , m_cookieEnabled(page->settings().cookieEnabled())
    , m_crossOriginCheckInGetMatchedCSSRulesDisabled(page->settings().crossOriginCheckInGetMatchedCSSRulesDisabled())
    , m_defaultFixedFontSize(page->settings().defaultFixedFontSize())
    , m_defaultFontSize(page->settings().defaultFontSize())
    , m_defaultTextEncodingName(page->settings().defaultTextEncodingName())
    , m_defaultVideoPosterURL(page->settings().defaultVideoPosterURL())
    , m_delegatesPageScaling(page->settings().delegatesPageScaling())
    , m_developerExtrasEnabled(page->settings().developerExtrasEnabled())
    , m_deviceHeight(page->settings().deviceHeight())
    , m_deviceWidth(page->settings().deviceWidth())
    , m_diagnosticLoggingEnabled(page->settings().diagnosticLoggingEnabled())
    , m_displayListDrawingEnabled(page->settings().displayListDrawingEnabled())
    , m_domTimersThrottlingEnabled(page->settings().domTimersThrottlingEnabled())
    , m_downloadableBinaryFontsEnabled(page->settings().downloadableBinaryFontsEnabled())
    , m_enableInheritURIQueryComponent(page->settings().enableInheritURIQueryComponent())
    , m_enforceCSSMIMETypeInNoQuirksMode(page->settings().enforceCSSMIMETypeInNoQuirksMode())
    , m_experimentalNotificationsEnabled(page->settings().experimentalNotificationsEnabled())
    , m_fixedBackgroundsPaintRelativeToDocument(page->settings().fixedBackgroundsPaintRelativeToDocument())
    , m_fixedElementsLayoutRelativeToFrame(page->settings().fixedElementsLayoutRelativeToFrame())
    , m_fixedPositionCreatesStackingContext(page->settings().fixedPositionCreatesStackingContext())
    , m_forceCompositingMode(page->settings().forceCompositingMode())
    , m_forceFTPDirectoryListings(page->settings().forceFTPDirectoryListings())
    , m_forceSoftwareWebGLRendering(page->settings().forceSoftwareWebGLRendering())
    , m_forceUpdateScrollbarsOnMainThreadForPerformanceTesting(page->settings().forceUpdateScrollbarsOnMainThreadForPerformanceTesting())
    , m_frameFlatteningEnabled(page->settings().frameFlatteningEnabled())
    , m_ftpDirectoryTemplatePath(page->settings().ftpDirectoryTemplatePath())
#if ENABLE(FULLSCREEN_API)
    , m_fullScreenEnabled(page->settings().fullScreenEnabled())
#endif
    , m_httpEquivEnabled(page->settings().httpEquivEnabled())
    , m_hyperlinkAuditingEnabled(page->settings().hyperlinkAuditingEnabled())
#if ENABLE(SERVICE_CONTROLS)
    , m_imageControlsEnabled(page->settings().imageControlsEnabled())
#endif
    , m_imageSubsamplingEnabled(page->settings().imageSubsamplingEnabled())
    , m_incrementalRenderingSuppressionTimeoutInSeconds(page->settings().incrementalRenderingSuppressionTimeoutInSeconds())
    , m_inlineMediaPlaybackRequiresPlaysInlineAttribute(page->settings().inlineMediaPlaybackRequiresPlaysInlineAttribute())
    , m_interactiveFormValidationEnabled(page->settings().interactiveFormValidationEnabled())
    , m_invisibleAutoplayNotPermitted(page->settings().invisibleAutoplayNotPermitted())
    , m_javaScriptCanAccessClipboard(page->settings().javaScriptCanAccessClipboard())
    , m_javaScriptCanOpenWindowsAutomatically(page->settings().javaScriptCanOpenWindowsAutomatically())
    , m_layoutFallbackWidth(page->settings().layoutFallbackWidth())
    , m_loadDeferringEnabled(page->settings().loadDeferringEnabled())
    , m_loadsSiteIconsIgnoringImageLoadingSetting(page->settings().loadsSiteIconsIgnoringImageLoadingSetting())
    , m_localFileContentSniffingEnabled(page->settings().localFileContentSniffingEnabled())
    , m_localStorageDatabasePath(page->settings().localStorageDatabasePath())
    , m_localStorageEnabled(page->settings().localStorageEnabled())
    , m_logsPageMessagesToSystemConsoleEnabled(page->settings().logsPageMessagesToSystemConsoleEnabled())
    , m_mainContentUserGestureOverrideEnabled(page->settings().mainContentUserGestureOverrideEnabled())
    , m_maxParseDuration(page->settings().maxParseDuration())
    , m_maximumHTMLParserDOMTreeDepth(page->settings().maximumHTMLParserDOMTreeDepth())
    , m_maximumPlugInSnapshotAttempts(page->settings().maximumPlugInSnapshotAttempts())
#if ENABLE(MEDIA_SOURCE)
    , m_maximumSourceBufferSize(page->settings().maximumSourceBufferSize())
#endif
    , m_mediaControlsScaleWithPageZoom(page->settings().mediaControlsScaleWithPageZoom())
    , m_mediaDataLoadsAutomatically(page->settings().mediaDataLoadsAutomatically())
    , m_mediaEnabled(page->settings().mediaEnabled())
#if ENABLE(MEDIA_SOURCE)
    , m_mediaSourceEnabled(page->settings().mediaSourceEnabled())
#endif
    , m_mediaStreamEnabled(page->settings().mediaStreamEnabled())
    , m_minimumAccelerated2dCanvasSize(page->settings().minimumAccelerated2dCanvasSize())
    , m_minimumFontSize(page->settings().minimumFontSize())
    , m_minimumLogicalFontSize(page->settings().minimumLogicalFontSize())
#if ENABLE(IOS_TEXT_AUTOSIZING)
    , m_minimumZoomFontSize(page->settings().minimumZoomFontSize())
#endif
    , m_needsIsLoadingInAPISenseQuirk(page->settings().needsIsLoadingInAPISenseQuirk())
    , m_needsKeyboardEventDisambiguationQuirks(page->settings().needsKeyboardEventDisambiguationQuirks())
    , m_needsSiteSpecificQuirks(page->settings().needsSiteSpecificQuirks())
    , m_newBlockInsideInlineModelEnabled(page->settings().newBlockInsideInlineModelEnabled())
    , m_notificationsEnabled(page->settings().notificationsEnabled())
    , m_offlineWebApplicationCacheEnabled(page->settings().offlineWebApplicationCacheEnabled())
    , m_openGLMultisamplingEnabled(page->settings().openGLMultisamplingEnabled())
    , m_pageCacheSupportsPlugins(page->settings().pageCacheSupportsPlugins())
    , m_paginateDuringLayoutEnabled(page->settings().paginateDuringLayoutEnabled())
    , m_passwordEchoDurationInSeconds(page->settings().passwordEchoDurationInSeconds())
    , m_passwordEchoEnabled(page->settings().passwordEchoEnabled())
    , m_plugInSnapshottingEnabled(page->settings().plugInSnapshottingEnabled())
    , m_preventKeyboardDOMEventDispatch(page->settings().preventKeyboardDOMEventDispatch())
    , m_primaryPlugInSnapshotDetectionEnabled(page->settings().primaryPlugInSnapshotDetectionEnabled())
    , m_requestAnimationFrameEnabled(page->settings().requestAnimationFrameEnabled())
#if ENABLE(RUBBER_BANDING)
    , m_rubberBandingForSubScrollableRegionsEnabled(page->settings().rubberBandingForSubScrollableRegionsEnabled())
#endif
    , m_scriptMarkupEnabled(page->settings().scriptMarkupEnabled())
#if ENABLE(SMOOTH_SCROLLING)
    , m_scrollAnimatorEnabled(page->settings().scrollAnimatorEnabled())
#endif
    , m_scrollingCoordinatorEnabled(page->settings().scrollingCoordinatorEnabled())
    , m_scrollingTreeIncludesFrames(page->settings().scrollingTreeIncludesFrames())
    , m_selectTrailingWhitespaceEnabled(page->settings().selectTrailingWhitespaceEnabled())
    , m_selectionIncludesAltImageText(page->settings().selectionIncludesAltImageText())
    , m_selectionPaintingWithoutSelectionGapsEnabled(page->settings().selectionPaintingWithoutSelectionGapsEnabled())
#if ENABLE(SERVICE_CONTROLS)
    , m_serviceControlsEnabled(page->settings().serviceControlsEnabled())
#endif
    , m_sessionStorageQuota(page->settings().sessionStorageQuota())
    , m_shouldConvertInvalidURLsToBlank(page->settings().shouldConvertInvalidURLsToBlank())
    , m_shouldConvertPositionStyleOnCopy(page->settings().shouldConvertPositionStyleOnCopy())
    , m_shouldDispatchJavaScriptWindowOnErrorEvents(page->settings().shouldDispatchJavaScriptWindowOnErrorEvents())
#if ENABLE(VIDEO_TRACK)
    , m_shouldDisplayCaptions(page->settings().shouldDisplayCaptions())
#endif
#if ENABLE(VIDEO_TRACK)
    , m_shouldDisplaySubtitles(page->settings().shouldDisplaySubtitles())
#endif
#if ENABLE(VIDEO_TRACK)
    , m_shouldDisplayTextDescriptions(page->settings().shouldDisplayTextDescriptions())
#endif
    , m_shouldInjectUserScriptsInInitialEmptyDocument(page->settings().shouldInjectUserScriptsInInitialEmptyDocument())
    , m_shouldPrintBackgrounds(page->settings().shouldPrintBackgrounds())
    , m_shouldRespectImageOrientation(page->settings().shouldRespectImageOrientation())
    , m_shouldTransformsAffectOverflow(page->settings().shouldTransformsAffectOverflow())
    , m_showDebugBorders(page->settings().showDebugBorders())
    , m_showRepaintCounter(page->settings().showRepaintCounter())
    , m_showsToolTipOverTruncatedText(page->settings().showsToolTipOverTruncatedText())
    , m_showsURLsInToolTips(page->settings().showsURLsInToolTips())
    , m_shrinksStandaloneImagesToFit(page->settings().shrinksStandaloneImagesToFit())
    , m_simpleLineLayoutDebugBordersEnabled(page->settings().simpleLineLayoutDebugBordersEnabled())
    , m_simpleLineLayoutEnabled(page->settings().simpleLineLayoutEnabled())
    , m_smartInsertDeleteEnabled(page->settings().smartInsertDeleteEnabled())
    , m_snapshotAllPlugIns(page->settings().snapshotAllPlugIns())
    , m_spatialNavigationEnabled(page->settings().spatialNavigationEnabled())
    , m_springTimingFunctionEnabled(page->settings().springTimingFunctionEnabled())
    , m_standalone(page->settings().standalone())
    , m_subpixelCSSOMElementMetricsEnabled(page->settings().subpixelCSSOMElementMetricsEnabled())
    , m_suppressesIncrementalRendering(page->settings().suppressesIncrementalRendering())
    , m_syncXHRInDocumentsEnabled(page->settings().syncXHRInDocumentsEnabled())
    , m_telephoneNumberParsingEnabled(page->settings().telephoneNumberParsingEnabled())
    , m_temporaryTileCohortRetentionEnabled(page->settings().temporaryTileCohortRetentionEnabled())
    , m_textAreasAreResizable(page->settings().textAreasAreResizable())
#if ENABLE(IOS_TEXT_AUTOSIZING)
    , m_textAutosizingEnabled(page->settings().textAutosizingEnabled())
#endif
    , m_treatsAnyTextCSSLinkAsStylesheet(page->settings().treatsAnyTextCSSLinkAsStylesheet())
    , m_unifiedTextCheckerEnabled(page->settings().unifiedTextCheckerEnabled())
    , m_unsafePluginPastingEnabled(page->settings().unsafePluginPastingEnabled())
    , m_useGiantTiles(page->settings().useGiantTiles())
    , m_useImageDocumentForSubframePDF(page->settings().useImageDocumentForSubframePDF())
    , m_useLegacyBackgroundSizeShorthandBehavior(page->settings().useLegacyBackgroundSizeShorthandBehavior())
    , m_useLegacyTextAlignPositionedElementBehavior(page->settings().useLegacyTextAlignPositionedElementBehavior())
    , m_usePreHTML5ParserQuirks(page->settings().usePreHTML5ParserQuirks())
#if ENABLE(DASHBOARD_SUPPORT)
    , m_usesDashboardBackwardCompatibilityMode(page->settings().usesDashboardBackwardCompatibilityMode())
#endif
    , m_usesEncodingDetector(page->settings().usesEncodingDetector())
    , m_validationMessageTimerMagnification(page->settings().validationMessageTimerMagnification())
    , m_videoPlaybackRequiresUserGesture(page->settings().videoPlaybackRequiresUserGesture())
    , m_wantsBalancedSetDefersLoadingBehavior(page->settings().wantsBalancedSetDefersLoadingBehavior())
#if ENABLE(WEB_ARCHIVE)
    , m_webArchiveDebugModeEnabled(page->settings().webArchiveDebugModeEnabled())
#endif
    , m_webAudioEnabled(page->settings().webAudioEnabled())
    , m_webGLEnabled(page->settings().webGLEnabled())
    , m_webGLErrorsToConsoleEnabled(page->settings().webGLErrorsToConsoleEnabled())
    , m_webSecurityEnabled(page->settings().webSecurityEnabled())
    , m_windowFocusRestricted(page->settings().windowFocusRestricted())
    , m_xssAuditorEnabled(page->settings().xssAuditorEnabled())
{
}

InternalSettingsGenerated::~InternalSettingsGenerated()
{
}

void InternalSettingsGenerated::resetToConsistentState()
{
    m_page->settings().setDOMPasteAllowed(m_DOMPasteAllowed);
    m_page->settings().setAccelerated2dCanvasEnabled(m_accelerated2dCanvasEnabled);
    m_page->settings().setAcceleratedCompositedAnimationsEnabled(m_acceleratedCompositedAnimationsEnabled);
    m_page->settings().setAcceleratedCompositingEnabled(m_acceleratedCompositingEnabled);
    m_page->settings().setAcceleratedCompositingForFixedPositionEnabled(m_acceleratedCompositingForFixedPositionEnabled);
    m_page->settings().setAcceleratedCompositingForOverflowScrollEnabled(m_acceleratedCompositingForOverflowScrollEnabled);
    m_page->settings().setAcceleratedDrawingEnabled(m_acceleratedDrawingEnabled);
    m_page->settings().setAcceleratedFiltersEnabled(m_acceleratedFiltersEnabled);
    m_page->settings().setAggressiveTileRetentionEnabled(m_aggressiveTileRetentionEnabled);
    m_page->settings().setAllowContentSecurityPolicySourceStarToMatchAnyProtocol(m_allowContentSecurityPolicySourceStarToMatchAnyProtocol);
    m_page->settings().setAllowCustomScrollbarInMainFrame(m_allowCustomScrollbarInMainFrame);
    m_page->settings().setAllowDisplayOfInsecureContent(m_allowDisplayOfInsecureContent);
    m_page->settings().setAllowFileAccessFromFileURLs(m_allowFileAccessFromFileURLs);
    m_page->settings().setAllowMultiElementImplicitSubmission(m_allowMultiElementImplicitSubmission);
    m_page->settings().setAllowRunningOfInsecureContent(m_allowRunningOfInsecureContent);
    m_page->settings().setAllowScriptsToCloseWindows(m_allowScriptsToCloseWindows);
    m_page->settings().setAllowUniversalAccessFromFileURLs(m_allowUniversalAccessFromFileURLs);
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    m_page->settings().setAllowsAirPlayForMediaPlayback(m_allowsAirPlayForMediaPlayback);
#endif
    m_page->settings().setAllowsInlineMediaPlayback(m_allowsInlineMediaPlayback);
    m_page->settings().setAllowsInlineMediaPlaybackAfterFullscreen(m_allowsInlineMediaPlaybackAfterFullscreen);
    m_page->settings().setAllowsPictureInPictureMediaPlayback(m_allowsPictureInPictureMediaPlayback);
    m_page->settings().setAlwaysUseAcceleratedOverflowScroll(m_alwaysUseAcceleratedOverflowScroll);
    m_page->settings().setAntialiased2dCanvasEnabled(m_antialiased2dCanvasEnabled);
    m_page->settings().setAppleMailPaginationQuirkEnabled(m_appleMailPaginationQuirkEnabled);
    m_page->settings().setAsynchronousSpellCheckingEnabled(m_asynchronousSpellCheckingEnabled);
#if ENABLE(ATTACHMENT_ELEMENT)
    m_page->settings().setAttachmentElementEnabled(m_attachmentElementEnabled);
#endif
    m_page->settings().setAudioPlaybackRequiresUserGesture(m_audioPlaybackRequiresUserGesture);
    m_page->settings().setAuthorAndUserStylesEnabled(m_authorAndUserStylesEnabled);
    m_page->settings().setAutoscrollForDragAndDropEnabled(m_autoscrollForDragAndDropEnabled);
    m_page->settings().setAutostartOriginPlugInSnapshottingEnabled(m_autostartOriginPlugInSnapshottingEnabled);
    m_page->settings().setBackForwardCacheExpirationInterval(m_backForwardCacheExpirationInterval);
    m_page->settings().setBackspaceKeyNavigationEnabled(m_backspaceKeyNavigationEnabled);
    m_page->settings().setCanvasUsesAcceleratedDrawing(m_canvasUsesAcceleratedDrawing);
    m_page->settings().setCaretBrowsingEnabled(m_caretBrowsingEnabled);
    m_page->settings().setContentDispositionAttachmentSandboxEnabled(m_contentDispositionAttachmentSandboxEnabled);
    m_page->settings().setCookieEnabled(m_cookieEnabled);
    m_page->settings().setCrossOriginCheckInGetMatchedCSSRulesDisabled(m_crossOriginCheckInGetMatchedCSSRulesDisabled);
    m_page->settings().setDefaultFixedFontSize(m_defaultFixedFontSize);
    m_page->settings().setDefaultFontSize(m_defaultFontSize);
    m_page->settings().setDefaultTextEncodingName(m_defaultTextEncodingName);
    m_page->settings().setDefaultVideoPosterURL(m_defaultVideoPosterURL);
    m_page->settings().setDelegatesPageScaling(m_delegatesPageScaling);
    m_page->settings().setDeveloperExtrasEnabled(m_developerExtrasEnabled);
    m_page->settings().setDeviceHeight(m_deviceHeight);
    m_page->settings().setDeviceWidth(m_deviceWidth);
    m_page->settings().setDiagnosticLoggingEnabled(m_diagnosticLoggingEnabled);
    m_page->settings().setDisplayListDrawingEnabled(m_displayListDrawingEnabled);
    m_page->settings().setDOMTimersThrottlingEnabled(m_domTimersThrottlingEnabled);
    m_page->settings().setDownloadableBinaryFontsEnabled(m_downloadableBinaryFontsEnabled);
    m_page->settings().setEnableInheritURIQueryComponent(m_enableInheritURIQueryComponent);
    m_page->settings().setEnforceCSSMIMETypeInNoQuirksMode(m_enforceCSSMIMETypeInNoQuirksMode);
    m_page->settings().setExperimentalNotificationsEnabled(m_experimentalNotificationsEnabled);
    m_page->settings().setFixedBackgroundsPaintRelativeToDocument(m_fixedBackgroundsPaintRelativeToDocument);
    m_page->settings().setFixedElementsLayoutRelativeToFrame(m_fixedElementsLayoutRelativeToFrame);
    m_page->settings().setFixedPositionCreatesStackingContext(m_fixedPositionCreatesStackingContext);
    m_page->settings().setForceCompositingMode(m_forceCompositingMode);
    m_page->settings().setForceFTPDirectoryListings(m_forceFTPDirectoryListings);
    m_page->settings().setForceSoftwareWebGLRendering(m_forceSoftwareWebGLRendering);
    m_page->settings().setForceUpdateScrollbarsOnMainThreadForPerformanceTesting(m_forceUpdateScrollbarsOnMainThreadForPerformanceTesting);
    m_page->settings().setFrameFlatteningEnabled(m_frameFlatteningEnabled);
    m_page->settings().setFTPDirectoryTemplatePath(m_ftpDirectoryTemplatePath);
#if ENABLE(FULLSCREEN_API)
    m_page->settings().setFullScreenEnabled(m_fullScreenEnabled);
#endif
    m_page->settings().setHttpEquivEnabled(m_httpEquivEnabled);
    m_page->settings().setHyperlinkAuditingEnabled(m_hyperlinkAuditingEnabled);
#if ENABLE(SERVICE_CONTROLS)
    m_page->settings().setImageControlsEnabled(m_imageControlsEnabled);
#endif
    m_page->settings().setImageSubsamplingEnabled(m_imageSubsamplingEnabled);
    m_page->settings().setIncrementalRenderingSuppressionTimeoutInSeconds(m_incrementalRenderingSuppressionTimeoutInSeconds);
    m_page->settings().setInlineMediaPlaybackRequiresPlaysInlineAttribute(m_inlineMediaPlaybackRequiresPlaysInlineAttribute);
    m_page->settings().setInteractiveFormValidationEnabled(m_interactiveFormValidationEnabled);
    m_page->settings().setInvisibleAutoplayNotPermitted(m_invisibleAutoplayNotPermitted);
    m_page->settings().setJavaScriptCanAccessClipboard(m_javaScriptCanAccessClipboard);
    m_page->settings().setJavaScriptCanOpenWindowsAutomatically(m_javaScriptCanOpenWindowsAutomatically);
    m_page->settings().setLayoutFallbackWidth(m_layoutFallbackWidth);
    m_page->settings().setLoadDeferringEnabled(m_loadDeferringEnabled);
    m_page->settings().setLoadsSiteIconsIgnoringImageLoadingSetting(m_loadsSiteIconsIgnoringImageLoadingSetting);
    m_page->settings().setLocalFileContentSniffingEnabled(m_localFileContentSniffingEnabled);
    m_page->settings().setLocalStorageDatabasePath(m_localStorageDatabasePath);
    m_page->settings().setLocalStorageEnabled(m_localStorageEnabled);
    m_page->settings().setLogsPageMessagesToSystemConsoleEnabled(m_logsPageMessagesToSystemConsoleEnabled);
    m_page->settings().setMainContentUserGestureOverrideEnabled(m_mainContentUserGestureOverrideEnabled);
    m_page->settings().setMaxParseDuration(m_maxParseDuration);
    m_page->settings().setMaximumHTMLParserDOMTreeDepth(m_maximumHTMLParserDOMTreeDepth);
    m_page->settings().setMaximumPlugInSnapshotAttempts(m_maximumPlugInSnapshotAttempts);
#if ENABLE(MEDIA_SOURCE)
    m_page->settings().setMaximumSourceBufferSize(m_maximumSourceBufferSize);
#endif
    m_page->settings().setMediaControlsScaleWithPageZoom(m_mediaControlsScaleWithPageZoom);
    m_page->settings().setMediaDataLoadsAutomatically(m_mediaDataLoadsAutomatically);
    m_page->settings().setMediaEnabled(m_mediaEnabled);
#if ENABLE(MEDIA_SOURCE)
    m_page->settings().setMediaSourceEnabled(m_mediaSourceEnabled);
#endif
    m_page->settings().setMediaStreamEnabled(m_mediaStreamEnabled);
    m_page->settings().setMinimumAccelerated2dCanvasSize(m_minimumAccelerated2dCanvasSize);
    m_page->settings().setMinimumFontSize(m_minimumFontSize);
    m_page->settings().setMinimumLogicalFontSize(m_minimumLogicalFontSize);
#if ENABLE(IOS_TEXT_AUTOSIZING)
    m_page->settings().setMinimumZoomFontSize(m_minimumZoomFontSize);
#endif
    m_page->settings().setNeedsIsLoadingInAPISenseQuirk(m_needsIsLoadingInAPISenseQuirk);
    m_page->settings().setNeedsKeyboardEventDisambiguationQuirks(m_needsKeyboardEventDisambiguationQuirks);
    m_page->settings().setNeedsSiteSpecificQuirks(m_needsSiteSpecificQuirks);
    m_page->settings().setNewBlockInsideInlineModelEnabled(m_newBlockInsideInlineModelEnabled);
    m_page->settings().setNotificationsEnabled(m_notificationsEnabled);
    m_page->settings().setOfflineWebApplicationCacheEnabled(m_offlineWebApplicationCacheEnabled);
    m_page->settings().setOpenGLMultisamplingEnabled(m_openGLMultisamplingEnabled);
    m_page->settings().setPageCacheSupportsPlugins(m_pageCacheSupportsPlugins);
    m_page->settings().setPaginateDuringLayoutEnabled(m_paginateDuringLayoutEnabled);
    m_page->settings().setPasswordEchoDurationInSeconds(m_passwordEchoDurationInSeconds);
    m_page->settings().setPasswordEchoEnabled(m_passwordEchoEnabled);
    m_page->settings().setPlugInSnapshottingEnabled(m_plugInSnapshottingEnabled);
    m_page->settings().setPreventKeyboardDOMEventDispatch(m_preventKeyboardDOMEventDispatch);
    m_page->settings().setPrimaryPlugInSnapshotDetectionEnabled(m_primaryPlugInSnapshotDetectionEnabled);
    m_page->settings().setRequestAnimationFrameEnabled(m_requestAnimationFrameEnabled);
#if ENABLE(RUBBER_BANDING)
    m_page->settings().setRubberBandingForSubScrollableRegionsEnabled(m_rubberBandingForSubScrollableRegionsEnabled);
#endif
    m_page->settings().setScriptMarkupEnabled(m_scriptMarkupEnabled);
#if ENABLE(SMOOTH_SCROLLING)
    m_page->settings().setScrollAnimatorEnabled(m_scrollAnimatorEnabled);
#endif
    m_page->settings().setScrollingCoordinatorEnabled(m_scrollingCoordinatorEnabled);
    m_page->settings().setScrollingTreeIncludesFrames(m_scrollingTreeIncludesFrames);
    m_page->settings().setSelectTrailingWhitespaceEnabled(m_selectTrailingWhitespaceEnabled);
    m_page->settings().setSelectionIncludesAltImageText(m_selectionIncludesAltImageText);
    m_page->settings().setSelectionPaintingWithoutSelectionGapsEnabled(m_selectionPaintingWithoutSelectionGapsEnabled);
#if ENABLE(SERVICE_CONTROLS)
    m_page->settings().setServiceControlsEnabled(m_serviceControlsEnabled);
#endif
    m_page->settings().setSessionStorageQuota(m_sessionStorageQuota);
    m_page->settings().setShouldConvertInvalidURLsToBlank(m_shouldConvertInvalidURLsToBlank);
    m_page->settings().setShouldConvertPositionStyleOnCopy(m_shouldConvertPositionStyleOnCopy);
    m_page->settings().setShouldDispatchJavaScriptWindowOnErrorEvents(m_shouldDispatchJavaScriptWindowOnErrorEvents);
#if ENABLE(VIDEO_TRACK)
    m_page->settings().setShouldDisplayCaptions(m_shouldDisplayCaptions);
#endif
#if ENABLE(VIDEO_TRACK)
    m_page->settings().setShouldDisplaySubtitles(m_shouldDisplaySubtitles);
#endif
#if ENABLE(VIDEO_TRACK)
    m_page->settings().setShouldDisplayTextDescriptions(m_shouldDisplayTextDescriptions);
#endif
    m_page->settings().setShouldInjectUserScriptsInInitialEmptyDocument(m_shouldInjectUserScriptsInInitialEmptyDocument);
    m_page->settings().setShouldPrintBackgrounds(m_shouldPrintBackgrounds);
    m_page->settings().setShouldRespectImageOrientation(m_shouldRespectImageOrientation);
    m_page->settings().setShouldTransformsAffectOverflow(m_shouldTransformsAffectOverflow);
    m_page->settings().setShowDebugBorders(m_showDebugBorders);
    m_page->settings().setShowRepaintCounter(m_showRepaintCounter);
    m_page->settings().setShowsToolTipOverTruncatedText(m_showsToolTipOverTruncatedText);
    m_page->settings().setShowsURLsInToolTips(m_showsURLsInToolTips);
    m_page->settings().setShrinksStandaloneImagesToFit(m_shrinksStandaloneImagesToFit);
    m_page->settings().setSimpleLineLayoutDebugBordersEnabled(m_simpleLineLayoutDebugBordersEnabled);
    m_page->settings().setSimpleLineLayoutEnabled(m_simpleLineLayoutEnabled);
    m_page->settings().setSmartInsertDeleteEnabled(m_smartInsertDeleteEnabled);
    m_page->settings().setSnapshotAllPlugIns(m_snapshotAllPlugIns);
    m_page->settings().setSpatialNavigationEnabled(m_spatialNavigationEnabled);
    m_page->settings().setSpringTimingFunctionEnabled(m_springTimingFunctionEnabled);
    m_page->settings().setStandalone(m_standalone);
    m_page->settings().setSubpixelCSSOMElementMetricsEnabled(m_subpixelCSSOMElementMetricsEnabled);
    m_page->settings().setSuppressesIncrementalRendering(m_suppressesIncrementalRendering);
    m_page->settings().setSyncXHRInDocumentsEnabled(m_syncXHRInDocumentsEnabled);
    m_page->settings().setTelephoneNumberParsingEnabled(m_telephoneNumberParsingEnabled);
    m_page->settings().setTemporaryTileCohortRetentionEnabled(m_temporaryTileCohortRetentionEnabled);
    m_page->settings().setTextAreasAreResizable(m_textAreasAreResizable);
#if ENABLE(IOS_TEXT_AUTOSIZING)
    m_page->settings().setTextAutosizingEnabled(m_textAutosizingEnabled);
#endif
    m_page->settings().setTreatsAnyTextCSSLinkAsStylesheet(m_treatsAnyTextCSSLinkAsStylesheet);
    m_page->settings().setUnifiedTextCheckerEnabled(m_unifiedTextCheckerEnabled);
    m_page->settings().setUnsafePluginPastingEnabled(m_unsafePluginPastingEnabled);
    m_page->settings().setUseGiantTiles(m_useGiantTiles);
    m_page->settings().setUseImageDocumentForSubframePDF(m_useImageDocumentForSubframePDF);
    m_page->settings().setUseLegacyBackgroundSizeShorthandBehavior(m_useLegacyBackgroundSizeShorthandBehavior);
    m_page->settings().setUseLegacyTextAlignPositionedElementBehavior(m_useLegacyTextAlignPositionedElementBehavior);
    m_page->settings().setUsePreHTML5ParserQuirks(m_usePreHTML5ParserQuirks);
#if ENABLE(DASHBOARD_SUPPORT)
    m_page->settings().setUsesDashboardBackwardCompatibilityMode(m_usesDashboardBackwardCompatibilityMode);
#endif
    m_page->settings().setUsesEncodingDetector(m_usesEncodingDetector);
    m_page->settings().setValidationMessageTimerMagnification(m_validationMessageTimerMagnification);
    m_page->settings().setVideoPlaybackRequiresUserGesture(m_videoPlaybackRequiresUserGesture);
    m_page->settings().setWantsBalancedSetDefersLoadingBehavior(m_wantsBalancedSetDefersLoadingBehavior);
#if ENABLE(WEB_ARCHIVE)
    m_page->settings().setWebArchiveDebugModeEnabled(m_webArchiveDebugModeEnabled);
#endif
    m_page->settings().setWebAudioEnabled(m_webAudioEnabled);
    m_page->settings().setWebGLEnabled(m_webGLEnabled);
    m_page->settings().setWebGLErrorsToConsoleEnabled(m_webGLErrorsToConsoleEnabled);
    m_page->settings().setWebSecurityEnabled(m_webSecurityEnabled);
    m_page->settings().setWindowFocusRestricted(m_windowFocusRestricted);
    m_page->settings().setXSSAuditorEnabled(m_xssAuditorEnabled);
}
void InternalSettingsGenerated::setDOMPasteAllowed(bool DOMPasteAllowed)
{
    m_page->settings().setDOMPasteAllowed(DOMPasteAllowed);
}

void InternalSettingsGenerated::setAccelerated2dCanvasEnabled(bool accelerated2dCanvasEnabled)
{
    m_page->settings().setAccelerated2dCanvasEnabled(accelerated2dCanvasEnabled);
}

void InternalSettingsGenerated::setAcceleratedCompositedAnimationsEnabled(bool acceleratedCompositedAnimationsEnabled)
{
    m_page->settings().setAcceleratedCompositedAnimationsEnabled(acceleratedCompositedAnimationsEnabled);
}

void InternalSettingsGenerated::setAcceleratedCompositingEnabled(bool acceleratedCompositingEnabled)
{
    m_page->settings().setAcceleratedCompositingEnabled(acceleratedCompositingEnabled);
}

void InternalSettingsGenerated::setAcceleratedCompositingForFixedPositionEnabled(bool acceleratedCompositingForFixedPositionEnabled)
{
    m_page->settings().setAcceleratedCompositingForFixedPositionEnabled(acceleratedCompositingForFixedPositionEnabled);
}

void InternalSettingsGenerated::setAcceleratedCompositingForOverflowScrollEnabled(bool acceleratedCompositingForOverflowScrollEnabled)
{
    m_page->settings().setAcceleratedCompositingForOverflowScrollEnabled(acceleratedCompositingForOverflowScrollEnabled);
}

void InternalSettingsGenerated::setAcceleratedDrawingEnabled(bool acceleratedDrawingEnabled)
{
    m_page->settings().setAcceleratedDrawingEnabled(acceleratedDrawingEnabled);
}

void InternalSettingsGenerated::setAcceleratedFiltersEnabled(bool acceleratedFiltersEnabled)
{
    m_page->settings().setAcceleratedFiltersEnabled(acceleratedFiltersEnabled);
}

void InternalSettingsGenerated::setAggressiveTileRetentionEnabled(bool aggressiveTileRetentionEnabled)
{
    m_page->settings().setAggressiveTileRetentionEnabled(aggressiveTileRetentionEnabled);
}

void InternalSettingsGenerated::setAllowContentSecurityPolicySourceStarToMatchAnyProtocol(bool allowContentSecurityPolicySourceStarToMatchAnyProtocol)
{
    m_page->settings().setAllowContentSecurityPolicySourceStarToMatchAnyProtocol(allowContentSecurityPolicySourceStarToMatchAnyProtocol);
}

void InternalSettingsGenerated::setAllowCustomScrollbarInMainFrame(bool allowCustomScrollbarInMainFrame)
{
    m_page->settings().setAllowCustomScrollbarInMainFrame(allowCustomScrollbarInMainFrame);
}

void InternalSettingsGenerated::setAllowDisplayOfInsecureContent(bool allowDisplayOfInsecureContent)
{
    m_page->settings().setAllowDisplayOfInsecureContent(allowDisplayOfInsecureContent);
}

void InternalSettingsGenerated::setAllowFileAccessFromFileURLs(bool allowFileAccessFromFileURLs)
{
    m_page->settings().setAllowFileAccessFromFileURLs(allowFileAccessFromFileURLs);
}

void InternalSettingsGenerated::setAllowMultiElementImplicitSubmission(bool allowMultiElementImplicitSubmission)
{
    m_page->settings().setAllowMultiElementImplicitSubmission(allowMultiElementImplicitSubmission);
}

void InternalSettingsGenerated::setAllowRunningOfInsecureContent(bool allowRunningOfInsecureContent)
{
    m_page->settings().setAllowRunningOfInsecureContent(allowRunningOfInsecureContent);
}

void InternalSettingsGenerated::setAllowScriptsToCloseWindows(bool allowScriptsToCloseWindows)
{
    m_page->settings().setAllowScriptsToCloseWindows(allowScriptsToCloseWindows);
}

void InternalSettingsGenerated::setAllowUniversalAccessFromFileURLs(bool allowUniversalAccessFromFileURLs)
{
    m_page->settings().setAllowUniversalAccessFromFileURLs(allowUniversalAccessFromFileURLs);
}

void InternalSettingsGenerated::setAllowsAirPlayForMediaPlayback(bool allowsAirPlayForMediaPlayback)
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    m_page->settings().setAllowsAirPlayForMediaPlayback(allowsAirPlayForMediaPlayback);
#else
    UNUSED_PARAM(allowsAirPlayForMediaPlayback);
#endif
}

void InternalSettingsGenerated::setAllowsInlineMediaPlayback(bool allowsInlineMediaPlayback)
{
    m_page->settings().setAllowsInlineMediaPlayback(allowsInlineMediaPlayback);
}

void InternalSettingsGenerated::setAllowsInlineMediaPlaybackAfterFullscreen(bool allowsInlineMediaPlaybackAfterFullscreen)
{
    m_page->settings().setAllowsInlineMediaPlaybackAfterFullscreen(allowsInlineMediaPlaybackAfterFullscreen);
}

void InternalSettingsGenerated::setAllowsPictureInPictureMediaPlayback(bool allowsPictureInPictureMediaPlayback)
{
    m_page->settings().setAllowsPictureInPictureMediaPlayback(allowsPictureInPictureMediaPlayback);
}

void InternalSettingsGenerated::setAlwaysUseAcceleratedOverflowScroll(bool alwaysUseAcceleratedOverflowScroll)
{
    m_page->settings().setAlwaysUseAcceleratedOverflowScroll(alwaysUseAcceleratedOverflowScroll);
}

void InternalSettingsGenerated::setAntialiased2dCanvasEnabled(bool antialiased2dCanvasEnabled)
{
    m_page->settings().setAntialiased2dCanvasEnabled(antialiased2dCanvasEnabled);
}

void InternalSettingsGenerated::setAppleMailPaginationQuirkEnabled(bool appleMailPaginationQuirkEnabled)
{
    m_page->settings().setAppleMailPaginationQuirkEnabled(appleMailPaginationQuirkEnabled);
}

void InternalSettingsGenerated::setAsynchronousSpellCheckingEnabled(bool asynchronousSpellCheckingEnabled)
{
    m_page->settings().setAsynchronousSpellCheckingEnabled(asynchronousSpellCheckingEnabled);
}

void InternalSettingsGenerated::setAttachmentElementEnabled(bool attachmentElementEnabled)
{
#if ENABLE(ATTACHMENT_ELEMENT)
    m_page->settings().setAttachmentElementEnabled(attachmentElementEnabled);
#else
    UNUSED_PARAM(attachmentElementEnabled);
#endif
}

void InternalSettingsGenerated::setAudioPlaybackRequiresUserGesture(bool audioPlaybackRequiresUserGesture)
{
    m_page->settings().setAudioPlaybackRequiresUserGesture(audioPlaybackRequiresUserGesture);
}

void InternalSettingsGenerated::setAuthorAndUserStylesEnabled(bool authorAndUserStylesEnabled)
{
    m_page->settings().setAuthorAndUserStylesEnabled(authorAndUserStylesEnabled);
}

void InternalSettingsGenerated::setAutoscrollForDragAndDropEnabled(bool autoscrollForDragAndDropEnabled)
{
    m_page->settings().setAutoscrollForDragAndDropEnabled(autoscrollForDragAndDropEnabled);
}

void InternalSettingsGenerated::setAutostartOriginPlugInSnapshottingEnabled(bool autostartOriginPlugInSnapshottingEnabled)
{
    m_page->settings().setAutostartOriginPlugInSnapshottingEnabled(autostartOriginPlugInSnapshottingEnabled);
}

void InternalSettingsGenerated::setBackForwardCacheExpirationInterval(double backForwardCacheExpirationInterval)
{
    m_page->settings().setBackForwardCacheExpirationInterval(backForwardCacheExpirationInterval);
}

void InternalSettingsGenerated::setBackspaceKeyNavigationEnabled(bool backspaceKeyNavigationEnabled)
{
    m_page->settings().setBackspaceKeyNavigationEnabled(backspaceKeyNavigationEnabled);
}

void InternalSettingsGenerated::setCanvasUsesAcceleratedDrawing(bool canvasUsesAcceleratedDrawing)
{
    m_page->settings().setCanvasUsesAcceleratedDrawing(canvasUsesAcceleratedDrawing);
}

void InternalSettingsGenerated::setCaretBrowsingEnabled(bool caretBrowsingEnabled)
{
    m_page->settings().setCaretBrowsingEnabled(caretBrowsingEnabled);
}

void InternalSettingsGenerated::setContentDispositionAttachmentSandboxEnabled(bool contentDispositionAttachmentSandboxEnabled)
{
    m_page->settings().setContentDispositionAttachmentSandboxEnabled(contentDispositionAttachmentSandboxEnabled);
}

void InternalSettingsGenerated::setCookieEnabled(bool cookieEnabled)
{
    m_page->settings().setCookieEnabled(cookieEnabled);
}

void InternalSettingsGenerated::setCrossOriginCheckInGetMatchedCSSRulesDisabled(bool crossOriginCheckInGetMatchedCSSRulesDisabled)
{
    m_page->settings().setCrossOriginCheckInGetMatchedCSSRulesDisabled(crossOriginCheckInGetMatchedCSSRulesDisabled);
}

void InternalSettingsGenerated::setDefaultFixedFontSize(int defaultFixedFontSize)
{
    m_page->settings().setDefaultFixedFontSize(defaultFixedFontSize);
}

void InternalSettingsGenerated::setDefaultFontSize(int defaultFontSize)
{
    m_page->settings().setDefaultFontSize(defaultFontSize);
}

void InternalSettingsGenerated::setDefaultTextEncodingName(const String& defaultTextEncodingName)
{
    m_page->settings().setDefaultTextEncodingName(defaultTextEncodingName);
}

void InternalSettingsGenerated::setDefaultVideoPosterURL(const String& defaultVideoPosterURL)
{
    m_page->settings().setDefaultVideoPosterURL(defaultVideoPosterURL);
}

void InternalSettingsGenerated::setDelegatesPageScaling(bool delegatesPageScaling)
{
    m_page->settings().setDelegatesPageScaling(delegatesPageScaling);
}

void InternalSettingsGenerated::setDeveloperExtrasEnabled(bool developerExtrasEnabled)
{
    m_page->settings().setDeveloperExtrasEnabled(developerExtrasEnabled);
}

void InternalSettingsGenerated::setDeviceHeight(int deviceHeight)
{
    m_page->settings().setDeviceHeight(deviceHeight);
}

void InternalSettingsGenerated::setDeviceWidth(int deviceWidth)
{
    m_page->settings().setDeviceWidth(deviceWidth);
}

void InternalSettingsGenerated::setDiagnosticLoggingEnabled(bool diagnosticLoggingEnabled)
{
    m_page->settings().setDiagnosticLoggingEnabled(diagnosticLoggingEnabled);
}

void InternalSettingsGenerated::setDisplayListDrawingEnabled(bool displayListDrawingEnabled)
{
    m_page->settings().setDisplayListDrawingEnabled(displayListDrawingEnabled);
}

void InternalSettingsGenerated::setDOMTimersThrottlingEnabled(bool domTimersThrottlingEnabled)
{
    m_page->settings().setDOMTimersThrottlingEnabled(domTimersThrottlingEnabled);
}

void InternalSettingsGenerated::setDownloadableBinaryFontsEnabled(bool downloadableBinaryFontsEnabled)
{
    m_page->settings().setDownloadableBinaryFontsEnabled(downloadableBinaryFontsEnabled);
}

void InternalSettingsGenerated::setEnableInheritURIQueryComponent(bool enableInheritURIQueryComponent)
{
    m_page->settings().setEnableInheritURIQueryComponent(enableInheritURIQueryComponent);
}

void InternalSettingsGenerated::setEnforceCSSMIMETypeInNoQuirksMode(bool enforceCSSMIMETypeInNoQuirksMode)
{
    m_page->settings().setEnforceCSSMIMETypeInNoQuirksMode(enforceCSSMIMETypeInNoQuirksMode);
}

void InternalSettingsGenerated::setExperimentalNotificationsEnabled(bool experimentalNotificationsEnabled)
{
    m_page->settings().setExperimentalNotificationsEnabled(experimentalNotificationsEnabled);
}

void InternalSettingsGenerated::setFixedBackgroundsPaintRelativeToDocument(bool fixedBackgroundsPaintRelativeToDocument)
{
    m_page->settings().setFixedBackgroundsPaintRelativeToDocument(fixedBackgroundsPaintRelativeToDocument);
}

void InternalSettingsGenerated::setFixedElementsLayoutRelativeToFrame(bool fixedElementsLayoutRelativeToFrame)
{
    m_page->settings().setFixedElementsLayoutRelativeToFrame(fixedElementsLayoutRelativeToFrame);
}

void InternalSettingsGenerated::setFixedPositionCreatesStackingContext(bool fixedPositionCreatesStackingContext)
{
    m_page->settings().setFixedPositionCreatesStackingContext(fixedPositionCreatesStackingContext);
}

void InternalSettingsGenerated::setForceCompositingMode(bool forceCompositingMode)
{
    m_page->settings().setForceCompositingMode(forceCompositingMode);
}

void InternalSettingsGenerated::setForceFTPDirectoryListings(bool forceFTPDirectoryListings)
{
    m_page->settings().setForceFTPDirectoryListings(forceFTPDirectoryListings);
}

void InternalSettingsGenerated::setForceSoftwareWebGLRendering(bool forceSoftwareWebGLRendering)
{
    m_page->settings().setForceSoftwareWebGLRendering(forceSoftwareWebGLRendering);
}

void InternalSettingsGenerated::setForceUpdateScrollbarsOnMainThreadForPerformanceTesting(bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting)
{
    m_page->settings().setForceUpdateScrollbarsOnMainThreadForPerformanceTesting(forceUpdateScrollbarsOnMainThreadForPerformanceTesting);
}

void InternalSettingsGenerated::setFrameFlatteningEnabled(bool frameFlatteningEnabled)
{
    m_page->settings().setFrameFlatteningEnabled(frameFlatteningEnabled);
}

void InternalSettingsGenerated::setFTPDirectoryTemplatePath(const String& ftpDirectoryTemplatePath)
{
    m_page->settings().setFTPDirectoryTemplatePath(ftpDirectoryTemplatePath);
}

void InternalSettingsGenerated::setFullScreenEnabled(bool fullScreenEnabled)
{
#if ENABLE(FULLSCREEN_API)
    m_page->settings().setFullScreenEnabled(fullScreenEnabled);
#else
    UNUSED_PARAM(fullScreenEnabled);
#endif
}

void InternalSettingsGenerated::setHttpEquivEnabled(bool httpEquivEnabled)
{
    m_page->settings().setHttpEquivEnabled(httpEquivEnabled);
}

void InternalSettingsGenerated::setHyperlinkAuditingEnabled(bool hyperlinkAuditingEnabled)
{
    m_page->settings().setHyperlinkAuditingEnabled(hyperlinkAuditingEnabled);
}

void InternalSettingsGenerated::setImageControlsEnabled(bool imageControlsEnabled)
{
#if ENABLE(SERVICE_CONTROLS)
    m_page->settings().setImageControlsEnabled(imageControlsEnabled);
#else
    UNUSED_PARAM(imageControlsEnabled);
#endif
}

void InternalSettingsGenerated::setImageSubsamplingEnabled(bool imageSubsamplingEnabled)
{
    m_page->settings().setImageSubsamplingEnabled(imageSubsamplingEnabled);
}

void InternalSettingsGenerated::setIncrementalRenderingSuppressionTimeoutInSeconds(double incrementalRenderingSuppressionTimeoutInSeconds)
{
    m_page->settings().setIncrementalRenderingSuppressionTimeoutInSeconds(incrementalRenderingSuppressionTimeoutInSeconds);
}

void InternalSettingsGenerated::setInlineMediaPlaybackRequiresPlaysInlineAttribute(bool inlineMediaPlaybackRequiresPlaysInlineAttribute)
{
    m_page->settings().setInlineMediaPlaybackRequiresPlaysInlineAttribute(inlineMediaPlaybackRequiresPlaysInlineAttribute);
}

void InternalSettingsGenerated::setInteractiveFormValidationEnabled(bool interactiveFormValidationEnabled)
{
    m_page->settings().setInteractiveFormValidationEnabled(interactiveFormValidationEnabled);
}

void InternalSettingsGenerated::setInvisibleAutoplayNotPermitted(bool invisibleAutoplayNotPermitted)
{
    m_page->settings().setInvisibleAutoplayNotPermitted(invisibleAutoplayNotPermitted);
}

void InternalSettingsGenerated::setJavaScriptCanAccessClipboard(bool javaScriptCanAccessClipboard)
{
    m_page->settings().setJavaScriptCanAccessClipboard(javaScriptCanAccessClipboard);
}

void InternalSettingsGenerated::setJavaScriptCanOpenWindowsAutomatically(bool javaScriptCanOpenWindowsAutomatically)
{
    m_page->settings().setJavaScriptCanOpenWindowsAutomatically(javaScriptCanOpenWindowsAutomatically);
}

void InternalSettingsGenerated::setLayoutFallbackWidth(int layoutFallbackWidth)
{
    m_page->settings().setLayoutFallbackWidth(layoutFallbackWidth);
}

void InternalSettingsGenerated::setLoadDeferringEnabled(bool loadDeferringEnabled)
{
    m_page->settings().setLoadDeferringEnabled(loadDeferringEnabled);
}

void InternalSettingsGenerated::setLoadsSiteIconsIgnoringImageLoadingSetting(bool loadsSiteIconsIgnoringImageLoadingSetting)
{
    m_page->settings().setLoadsSiteIconsIgnoringImageLoadingSetting(loadsSiteIconsIgnoringImageLoadingSetting);
}

void InternalSettingsGenerated::setLocalFileContentSniffingEnabled(bool localFileContentSniffingEnabled)
{
    m_page->settings().setLocalFileContentSniffingEnabled(localFileContentSniffingEnabled);
}

void InternalSettingsGenerated::setLocalStorageDatabasePath(const String& localStorageDatabasePath)
{
    m_page->settings().setLocalStorageDatabasePath(localStorageDatabasePath);
}

void InternalSettingsGenerated::setLocalStorageEnabled(bool localStorageEnabled)
{
    m_page->settings().setLocalStorageEnabled(localStorageEnabled);
}

void InternalSettingsGenerated::setLogsPageMessagesToSystemConsoleEnabled(bool logsPageMessagesToSystemConsoleEnabled)
{
    m_page->settings().setLogsPageMessagesToSystemConsoleEnabled(logsPageMessagesToSystemConsoleEnabled);
}

void InternalSettingsGenerated::setMainContentUserGestureOverrideEnabled(bool mainContentUserGestureOverrideEnabled)
{
    m_page->settings().setMainContentUserGestureOverrideEnabled(mainContentUserGestureOverrideEnabled);
}

void InternalSettingsGenerated::setMaxParseDuration(double maxParseDuration)
{
    m_page->settings().setMaxParseDuration(maxParseDuration);
}

void InternalSettingsGenerated::setMaximumHTMLParserDOMTreeDepth(unsigned maximumHTMLParserDOMTreeDepth)
{
    m_page->settings().setMaximumHTMLParserDOMTreeDepth(maximumHTMLParserDOMTreeDepth);
}

void InternalSettingsGenerated::setMaximumPlugInSnapshotAttempts(unsigned maximumPlugInSnapshotAttempts)
{
    m_page->settings().setMaximumPlugInSnapshotAttempts(maximumPlugInSnapshotAttempts);
}

void InternalSettingsGenerated::setMaximumSourceBufferSize(int maximumSourceBufferSize)
{
#if ENABLE(MEDIA_SOURCE)
    m_page->settings().setMaximumSourceBufferSize(maximumSourceBufferSize);
#else
    UNUSED_PARAM(maximumSourceBufferSize);
#endif
}

void InternalSettingsGenerated::setMediaControlsScaleWithPageZoom(bool mediaControlsScaleWithPageZoom)
{
    m_page->settings().setMediaControlsScaleWithPageZoom(mediaControlsScaleWithPageZoom);
}

void InternalSettingsGenerated::setMediaDataLoadsAutomatically(bool mediaDataLoadsAutomatically)
{
    m_page->settings().setMediaDataLoadsAutomatically(mediaDataLoadsAutomatically);
}

void InternalSettingsGenerated::setMediaEnabled(bool mediaEnabled)
{
    m_page->settings().setMediaEnabled(mediaEnabled);
}

void InternalSettingsGenerated::setMediaSourceEnabled(bool mediaSourceEnabled)
{
#if ENABLE(MEDIA_SOURCE)
    m_page->settings().setMediaSourceEnabled(mediaSourceEnabled);
#else
    UNUSED_PARAM(mediaSourceEnabled);
#endif
}

void InternalSettingsGenerated::setMediaStreamEnabled(bool mediaStreamEnabled)
{
    m_page->settings().setMediaStreamEnabled(mediaStreamEnabled);
}

void InternalSettingsGenerated::setMinimumAccelerated2dCanvasSize(int minimumAccelerated2dCanvasSize)
{
    m_page->settings().setMinimumAccelerated2dCanvasSize(minimumAccelerated2dCanvasSize);
}

void InternalSettingsGenerated::setMinimumFontSize(int minimumFontSize)
{
    m_page->settings().setMinimumFontSize(minimumFontSize);
}

void InternalSettingsGenerated::setMinimumLogicalFontSize(int minimumLogicalFontSize)
{
    m_page->settings().setMinimumLogicalFontSize(minimumLogicalFontSize);
}

void InternalSettingsGenerated::setMinimumZoomFontSize(float minimumZoomFontSize)
{
#if ENABLE(IOS_TEXT_AUTOSIZING)
    m_page->settings().setMinimumZoomFontSize(minimumZoomFontSize);
#else
    UNUSED_PARAM(minimumZoomFontSize);
#endif
}

void InternalSettingsGenerated::setNeedsIsLoadingInAPISenseQuirk(bool needsIsLoadingInAPISenseQuirk)
{
    m_page->settings().setNeedsIsLoadingInAPISenseQuirk(needsIsLoadingInAPISenseQuirk);
}

void InternalSettingsGenerated::setNeedsKeyboardEventDisambiguationQuirks(bool needsKeyboardEventDisambiguationQuirks)
{
    m_page->settings().setNeedsKeyboardEventDisambiguationQuirks(needsKeyboardEventDisambiguationQuirks);
}

void InternalSettingsGenerated::setNeedsSiteSpecificQuirks(bool needsSiteSpecificQuirks)
{
    m_page->settings().setNeedsSiteSpecificQuirks(needsSiteSpecificQuirks);
}

void InternalSettingsGenerated::setNewBlockInsideInlineModelEnabled(bool newBlockInsideInlineModelEnabled)
{
    m_page->settings().setNewBlockInsideInlineModelEnabled(newBlockInsideInlineModelEnabled);
}

void InternalSettingsGenerated::setNotificationsEnabled(bool notificationsEnabled)
{
    m_page->settings().setNotificationsEnabled(notificationsEnabled);
}

void InternalSettingsGenerated::setOfflineWebApplicationCacheEnabled(bool offlineWebApplicationCacheEnabled)
{
    m_page->settings().setOfflineWebApplicationCacheEnabled(offlineWebApplicationCacheEnabled);
}

void InternalSettingsGenerated::setOpenGLMultisamplingEnabled(bool openGLMultisamplingEnabled)
{
    m_page->settings().setOpenGLMultisamplingEnabled(openGLMultisamplingEnabled);
}

void InternalSettingsGenerated::setPageCacheSupportsPlugins(bool pageCacheSupportsPlugins)
{
    m_page->settings().setPageCacheSupportsPlugins(pageCacheSupportsPlugins);
}

void InternalSettingsGenerated::setPaginateDuringLayoutEnabled(bool paginateDuringLayoutEnabled)
{
    m_page->settings().setPaginateDuringLayoutEnabled(paginateDuringLayoutEnabled);
}

void InternalSettingsGenerated::setPasswordEchoDurationInSeconds(double passwordEchoDurationInSeconds)
{
    m_page->settings().setPasswordEchoDurationInSeconds(passwordEchoDurationInSeconds);
}

void InternalSettingsGenerated::setPasswordEchoEnabled(bool passwordEchoEnabled)
{
    m_page->settings().setPasswordEchoEnabled(passwordEchoEnabled);
}

void InternalSettingsGenerated::setPlugInSnapshottingEnabled(bool plugInSnapshottingEnabled)
{
    m_page->settings().setPlugInSnapshottingEnabled(plugInSnapshottingEnabled);
}

void InternalSettingsGenerated::setPreventKeyboardDOMEventDispatch(bool preventKeyboardDOMEventDispatch)
{
    m_page->settings().setPreventKeyboardDOMEventDispatch(preventKeyboardDOMEventDispatch);
}

void InternalSettingsGenerated::setPrimaryPlugInSnapshotDetectionEnabled(bool primaryPlugInSnapshotDetectionEnabled)
{
    m_page->settings().setPrimaryPlugInSnapshotDetectionEnabled(primaryPlugInSnapshotDetectionEnabled);
}

void InternalSettingsGenerated::setRequestAnimationFrameEnabled(bool requestAnimationFrameEnabled)
{
    m_page->settings().setRequestAnimationFrameEnabled(requestAnimationFrameEnabled);
}

void InternalSettingsGenerated::setRubberBandingForSubScrollableRegionsEnabled(bool rubberBandingForSubScrollableRegionsEnabled)
{
#if ENABLE(RUBBER_BANDING)
    m_page->settings().setRubberBandingForSubScrollableRegionsEnabled(rubberBandingForSubScrollableRegionsEnabled);
#else
    UNUSED_PARAM(rubberBandingForSubScrollableRegionsEnabled);
#endif
}

void InternalSettingsGenerated::setScriptMarkupEnabled(bool scriptMarkupEnabled)
{
    m_page->settings().setScriptMarkupEnabled(scriptMarkupEnabled);
}

void InternalSettingsGenerated::setScrollAnimatorEnabled(bool scrollAnimatorEnabled)
{
#if ENABLE(SMOOTH_SCROLLING)
    m_page->settings().setScrollAnimatorEnabled(scrollAnimatorEnabled);
#else
    UNUSED_PARAM(scrollAnimatorEnabled);
#endif
}

void InternalSettingsGenerated::setScrollingCoordinatorEnabled(bool scrollingCoordinatorEnabled)
{
    m_page->settings().setScrollingCoordinatorEnabled(scrollingCoordinatorEnabled);
}

void InternalSettingsGenerated::setScrollingTreeIncludesFrames(bool scrollingTreeIncludesFrames)
{
    m_page->settings().setScrollingTreeIncludesFrames(scrollingTreeIncludesFrames);
}

void InternalSettingsGenerated::setSelectTrailingWhitespaceEnabled(bool selectTrailingWhitespaceEnabled)
{
    m_page->settings().setSelectTrailingWhitespaceEnabled(selectTrailingWhitespaceEnabled);
}

void InternalSettingsGenerated::setSelectionIncludesAltImageText(bool selectionIncludesAltImageText)
{
    m_page->settings().setSelectionIncludesAltImageText(selectionIncludesAltImageText);
}

void InternalSettingsGenerated::setSelectionPaintingWithoutSelectionGapsEnabled(bool selectionPaintingWithoutSelectionGapsEnabled)
{
    m_page->settings().setSelectionPaintingWithoutSelectionGapsEnabled(selectionPaintingWithoutSelectionGapsEnabled);
}

void InternalSettingsGenerated::setServiceControlsEnabled(bool serviceControlsEnabled)
{
#if ENABLE(SERVICE_CONTROLS)
    m_page->settings().setServiceControlsEnabled(serviceControlsEnabled);
#else
    UNUSED_PARAM(serviceControlsEnabled);
#endif
}

void InternalSettingsGenerated::setSessionStorageQuota(unsigned sessionStorageQuota)
{
    m_page->settings().setSessionStorageQuota(sessionStorageQuota);
}

void InternalSettingsGenerated::setShouldConvertInvalidURLsToBlank(bool shouldConvertInvalidURLsToBlank)
{
    m_page->settings().setShouldConvertInvalidURLsToBlank(shouldConvertInvalidURLsToBlank);
}

void InternalSettingsGenerated::setShouldConvertPositionStyleOnCopy(bool shouldConvertPositionStyleOnCopy)
{
    m_page->settings().setShouldConvertPositionStyleOnCopy(shouldConvertPositionStyleOnCopy);
}

void InternalSettingsGenerated::setShouldDispatchJavaScriptWindowOnErrorEvents(bool shouldDispatchJavaScriptWindowOnErrorEvents)
{
    m_page->settings().setShouldDispatchJavaScriptWindowOnErrorEvents(shouldDispatchJavaScriptWindowOnErrorEvents);
}

void InternalSettingsGenerated::setShouldDisplayCaptions(bool shouldDisplayCaptions)
{
#if ENABLE(VIDEO_TRACK)
    m_page->settings().setShouldDisplayCaptions(shouldDisplayCaptions);
#else
    UNUSED_PARAM(shouldDisplayCaptions);
#endif
}

void InternalSettingsGenerated::setShouldDisplaySubtitles(bool shouldDisplaySubtitles)
{
#if ENABLE(VIDEO_TRACK)
    m_page->settings().setShouldDisplaySubtitles(shouldDisplaySubtitles);
#else
    UNUSED_PARAM(shouldDisplaySubtitles);
#endif
}

void InternalSettingsGenerated::setShouldDisplayTextDescriptions(bool shouldDisplayTextDescriptions)
{
#if ENABLE(VIDEO_TRACK)
    m_page->settings().setShouldDisplayTextDescriptions(shouldDisplayTextDescriptions);
#else
    UNUSED_PARAM(shouldDisplayTextDescriptions);
#endif
}

void InternalSettingsGenerated::setShouldInjectUserScriptsInInitialEmptyDocument(bool shouldInjectUserScriptsInInitialEmptyDocument)
{
    m_page->settings().setShouldInjectUserScriptsInInitialEmptyDocument(shouldInjectUserScriptsInInitialEmptyDocument);
}

void InternalSettingsGenerated::setShouldPrintBackgrounds(bool shouldPrintBackgrounds)
{
    m_page->settings().setShouldPrintBackgrounds(shouldPrintBackgrounds);
}

void InternalSettingsGenerated::setShouldRespectImageOrientation(bool shouldRespectImageOrientation)
{
    m_page->settings().setShouldRespectImageOrientation(shouldRespectImageOrientation);
}

void InternalSettingsGenerated::setShouldTransformsAffectOverflow(bool shouldTransformsAffectOverflow)
{
    m_page->settings().setShouldTransformsAffectOverflow(shouldTransformsAffectOverflow);
}

void InternalSettingsGenerated::setShowDebugBorders(bool showDebugBorders)
{
    m_page->settings().setShowDebugBorders(showDebugBorders);
}

void InternalSettingsGenerated::setShowRepaintCounter(bool showRepaintCounter)
{
    m_page->settings().setShowRepaintCounter(showRepaintCounter);
}

void InternalSettingsGenerated::setShowsToolTipOverTruncatedText(bool showsToolTipOverTruncatedText)
{
    m_page->settings().setShowsToolTipOverTruncatedText(showsToolTipOverTruncatedText);
}

void InternalSettingsGenerated::setShowsURLsInToolTips(bool showsURLsInToolTips)
{
    m_page->settings().setShowsURLsInToolTips(showsURLsInToolTips);
}

void InternalSettingsGenerated::setShrinksStandaloneImagesToFit(bool shrinksStandaloneImagesToFit)
{
    m_page->settings().setShrinksStandaloneImagesToFit(shrinksStandaloneImagesToFit);
}

void InternalSettingsGenerated::setSimpleLineLayoutDebugBordersEnabled(bool simpleLineLayoutDebugBordersEnabled)
{
    m_page->settings().setSimpleLineLayoutDebugBordersEnabled(simpleLineLayoutDebugBordersEnabled);
}

void InternalSettingsGenerated::setSimpleLineLayoutEnabled(bool simpleLineLayoutEnabled)
{
    m_page->settings().setSimpleLineLayoutEnabled(simpleLineLayoutEnabled);
}

void InternalSettingsGenerated::setSmartInsertDeleteEnabled(bool smartInsertDeleteEnabled)
{
    m_page->settings().setSmartInsertDeleteEnabled(smartInsertDeleteEnabled);
}

void InternalSettingsGenerated::setSnapshotAllPlugIns(bool snapshotAllPlugIns)
{
    m_page->settings().setSnapshotAllPlugIns(snapshotAllPlugIns);
}

void InternalSettingsGenerated::setSpatialNavigationEnabled(bool spatialNavigationEnabled)
{
    m_page->settings().setSpatialNavigationEnabled(spatialNavigationEnabled);
}

void InternalSettingsGenerated::setSpringTimingFunctionEnabled(bool springTimingFunctionEnabled)
{
    m_page->settings().setSpringTimingFunctionEnabled(springTimingFunctionEnabled);
}

void InternalSettingsGenerated::setStandalone(bool standalone)
{
    m_page->settings().setStandalone(standalone);
}

void InternalSettingsGenerated::setSubpixelCSSOMElementMetricsEnabled(bool subpixelCSSOMElementMetricsEnabled)
{
    m_page->settings().setSubpixelCSSOMElementMetricsEnabled(subpixelCSSOMElementMetricsEnabled);
}

void InternalSettingsGenerated::setSuppressesIncrementalRendering(bool suppressesIncrementalRendering)
{
    m_page->settings().setSuppressesIncrementalRendering(suppressesIncrementalRendering);
}

void InternalSettingsGenerated::setSyncXHRInDocumentsEnabled(bool syncXHRInDocumentsEnabled)
{
    m_page->settings().setSyncXHRInDocumentsEnabled(syncXHRInDocumentsEnabled);
}

void InternalSettingsGenerated::setTelephoneNumberParsingEnabled(bool telephoneNumberParsingEnabled)
{
    m_page->settings().setTelephoneNumberParsingEnabled(telephoneNumberParsingEnabled);
}

void InternalSettingsGenerated::setTemporaryTileCohortRetentionEnabled(bool temporaryTileCohortRetentionEnabled)
{
    m_page->settings().setTemporaryTileCohortRetentionEnabled(temporaryTileCohortRetentionEnabled);
}

void InternalSettingsGenerated::setTextAreasAreResizable(bool textAreasAreResizable)
{
    m_page->settings().setTextAreasAreResizable(textAreasAreResizable);
}

void InternalSettingsGenerated::setTextAutosizingEnabled(bool textAutosizingEnabled)
{
#if ENABLE(IOS_TEXT_AUTOSIZING)
    m_page->settings().setTextAutosizingEnabled(textAutosizingEnabled);
#else
    UNUSED_PARAM(textAutosizingEnabled);
#endif
}

void InternalSettingsGenerated::setTreatsAnyTextCSSLinkAsStylesheet(bool treatsAnyTextCSSLinkAsStylesheet)
{
    m_page->settings().setTreatsAnyTextCSSLinkAsStylesheet(treatsAnyTextCSSLinkAsStylesheet);
}

void InternalSettingsGenerated::setUnifiedTextCheckerEnabled(bool unifiedTextCheckerEnabled)
{
    m_page->settings().setUnifiedTextCheckerEnabled(unifiedTextCheckerEnabled);
}

void InternalSettingsGenerated::setUnsafePluginPastingEnabled(bool unsafePluginPastingEnabled)
{
    m_page->settings().setUnsafePluginPastingEnabled(unsafePluginPastingEnabled);
}

void InternalSettingsGenerated::setUseGiantTiles(bool useGiantTiles)
{
    m_page->settings().setUseGiantTiles(useGiantTiles);
}

void InternalSettingsGenerated::setUseImageDocumentForSubframePDF(bool useImageDocumentForSubframePDF)
{
    m_page->settings().setUseImageDocumentForSubframePDF(useImageDocumentForSubframePDF);
}

void InternalSettingsGenerated::setUseLegacyBackgroundSizeShorthandBehavior(bool useLegacyBackgroundSizeShorthandBehavior)
{
    m_page->settings().setUseLegacyBackgroundSizeShorthandBehavior(useLegacyBackgroundSizeShorthandBehavior);
}

void InternalSettingsGenerated::setUseLegacyTextAlignPositionedElementBehavior(bool useLegacyTextAlignPositionedElementBehavior)
{
    m_page->settings().setUseLegacyTextAlignPositionedElementBehavior(useLegacyTextAlignPositionedElementBehavior);
}

void InternalSettingsGenerated::setUsePreHTML5ParserQuirks(bool usePreHTML5ParserQuirks)
{
    m_page->settings().setUsePreHTML5ParserQuirks(usePreHTML5ParserQuirks);
}

void InternalSettingsGenerated::setUsesDashboardBackwardCompatibilityMode(bool usesDashboardBackwardCompatibilityMode)
{
#if ENABLE(DASHBOARD_SUPPORT)
    m_page->settings().setUsesDashboardBackwardCompatibilityMode(usesDashboardBackwardCompatibilityMode);
#else
    UNUSED_PARAM(usesDashboardBackwardCompatibilityMode);
#endif
}

void InternalSettingsGenerated::setUsesEncodingDetector(bool usesEncodingDetector)
{
    m_page->settings().setUsesEncodingDetector(usesEncodingDetector);
}

void InternalSettingsGenerated::setValidationMessageTimerMagnification(int validationMessageTimerMagnification)
{
    m_page->settings().setValidationMessageTimerMagnification(validationMessageTimerMagnification);
}

void InternalSettingsGenerated::setVideoPlaybackRequiresUserGesture(bool videoPlaybackRequiresUserGesture)
{
    m_page->settings().setVideoPlaybackRequiresUserGesture(videoPlaybackRequiresUserGesture);
}

void InternalSettingsGenerated::setWantsBalancedSetDefersLoadingBehavior(bool wantsBalancedSetDefersLoadingBehavior)
{
    m_page->settings().setWantsBalancedSetDefersLoadingBehavior(wantsBalancedSetDefersLoadingBehavior);
}

void InternalSettingsGenerated::setWebArchiveDebugModeEnabled(bool webArchiveDebugModeEnabled)
{
#if ENABLE(WEB_ARCHIVE)
    m_page->settings().setWebArchiveDebugModeEnabled(webArchiveDebugModeEnabled);
#else
    UNUSED_PARAM(webArchiveDebugModeEnabled);
#endif
}

void InternalSettingsGenerated::setWebAudioEnabled(bool webAudioEnabled)
{
    m_page->settings().setWebAudioEnabled(webAudioEnabled);
}

void InternalSettingsGenerated::setWebGLEnabled(bool webGLEnabled)
{
    m_page->settings().setWebGLEnabled(webGLEnabled);
}

void InternalSettingsGenerated::setWebGLErrorsToConsoleEnabled(bool webGLErrorsToConsoleEnabled)
{
    m_page->settings().setWebGLErrorsToConsoleEnabled(webGLErrorsToConsoleEnabled);
}

void InternalSettingsGenerated::setWebSecurityEnabled(bool webSecurityEnabled)
{
    m_page->settings().setWebSecurityEnabled(webSecurityEnabled);
}

void InternalSettingsGenerated::setWindowFocusRestricted(bool windowFocusRestricted)
{
    m_page->settings().setWindowFocusRestricted(windowFocusRestricted);
}

void InternalSettingsGenerated::setXSSAuditorEnabled(bool xssAuditorEnabled)
{
    m_page->settings().setXSSAuditorEnabled(xssAuditorEnabled);
}

} // namespace WebCore
