/*
 * Copyright (C) 2003, 2006, 2007, 2008, 2009, 2011, 2012, 2013 Apple Inc. All rights reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#ifndef Settings_h
#define Settings_h

#include "EditingBehaviorTypes.h"
#include "FontRenderingMode.h"
#include "IntSize.h"
#include "URL.h"
#include "SecurityOrigin.h"
#include "SettingsMacros.h"
#include "Timer.h"
#include <chrono>
#include <unicode/uscript.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class FontGenericFamilies;
class Page;

enum EditableLinkBehavior {
    EditableLinkDefaultBehavior,
    EditableLinkAlwaysLive,
    EditableLinkOnlyLiveWithShiftKey,
    EditableLinkLiveWhenNotFocused,
    EditableLinkNeverLive
};

enum TextDirectionSubmenuInclusionBehavior {
    TextDirectionSubmenuNeverIncluded,
    TextDirectionSubmenuAutomaticallyIncluded,
    TextDirectionSubmenuAlwaysIncluded
};

class Settings : public RefCounted<Settings> {
    WTF_MAKE_NONCOPYABLE(Settings); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<Settings> create(Page*);
    ~Settings();

    WEBCORE_EXPORT void setStandardFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
    WEBCORE_EXPORT const AtomicString& standardFontFamily(UScriptCode = USCRIPT_COMMON) const;

    WEBCORE_EXPORT void setFixedFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
    WEBCORE_EXPORT const AtomicString& fixedFontFamily(UScriptCode = USCRIPT_COMMON) const;

    WEBCORE_EXPORT void setSerifFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
    WEBCORE_EXPORT const AtomicString& serifFontFamily(UScriptCode = USCRIPT_COMMON) const;

    WEBCORE_EXPORT void setSansSerifFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
    WEBCORE_EXPORT const AtomicString& sansSerifFontFamily(UScriptCode = USCRIPT_COMMON) const;

    WEBCORE_EXPORT void setCursiveFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
    WEBCORE_EXPORT const AtomicString& cursiveFontFamily(UScriptCode = USCRIPT_COMMON) const;

    WEBCORE_EXPORT void setFantasyFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
    WEBCORE_EXPORT const AtomicString& fantasyFontFamily(UScriptCode = USCRIPT_COMMON) const;

    WEBCORE_EXPORT void setPictographFontFamily(const AtomicString&, UScriptCode = USCRIPT_COMMON);
    WEBCORE_EXPORT const AtomicString& pictographFontFamily(UScriptCode = USCRIPT_COMMON) const;

#if ENABLE(TEXT_AUTOSIZING)
    void setTextAutosizingEnabled(bool);
    bool textAutosizingEnabled() const { return m_textAutosizingEnabled; }

    void setTextAutosizingFontScaleFactor(float);
    float textAutosizingFontScaleFactor() const { return m_textAutosizingFontScaleFactor; }

    // Only set by Layout Tests, and only used if textAutosizingEnabled() returns true.
    void setTextAutosizingWindowSizeOverride(const IntSize&);
    const IntSize& textAutosizingWindowSizeOverride() const { return m_textAutosizingWindowSizeOverride; }
#endif

    // Only set by Layout Tests.
    WEBCORE_EXPORT void setMediaTypeOverride(const String&);
    const String& mediaTypeOverride() const { return m_mediaTypeOverride; }

    // Unlike areImagesEnabled, this only suppresses the network load of
    // the image URL.  A cached image will still be rendered if requested.
    WEBCORE_EXPORT void setLoadsImagesAutomatically(bool);
    bool loadsImagesAutomatically() const { return m_loadsImagesAutomatically; }

    // Clients that execute script should call ScriptController::canExecuteScripts()
    // instead of this function. ScriptController::canExecuteScripts() checks the
    // HTML sandbox, plug-in sandboxing, and other important details.
    bool isScriptEnabled() const { return m_isScriptEnabled; }
    WEBCORE_EXPORT void setScriptEnabled(bool);

    SETTINGS_GETTERS_AND_SETTERS

    WEBCORE_EXPORT void setScreenFontSubstitutionEnabled(bool);
    bool screenFontSubstitutionEnabled() const { return m_screenFontSubstitutionEnabled; }

    WEBCORE_EXPORT void setJavaEnabled(bool);
    bool isJavaEnabled() const { return m_isJavaEnabled; }

    // This settings is only consulted if isJavaEnabled() returns true;
    WEBCORE_EXPORT void setJavaEnabledForLocalFiles(bool);
    bool isJavaEnabledForLocalFiles() const { return m_isJavaEnabledForLocalFiles; }

    WEBCORE_EXPORT void setImagesEnabled(bool);
    bool areImagesEnabled() const { return m_areImagesEnabled; }

    WEBCORE_EXPORT void setPluginsEnabled(bool);
    bool arePluginsEnabled() const { return m_arePluginsEnabled; }

    // When this option is set, WebCore will avoid storing any record of browsing activity
    // that may persist on disk or remain displayed when the option is reset.
    // This option does not affect the storage of such information in RAM.
    // The following functions respect this setting:
    //  - HTML5/DOM Storage
    //  - Icon Database
    //  - Console Messages
    //  - MemoryCache
    //  - Application Cache
    //  - Back/Forward Page History
    //  - Page Search Results
    //  - HTTP Cookies
    //  - Plug-ins (that support NPNVprivateModeBool)
    void setPrivateBrowsingEnabled(bool);

    WEBCORE_EXPORT void setDNSPrefetchingEnabled(bool);
    bool dnsPrefetchingEnabled() const { return m_dnsPrefetchingEnabled; }

    WEBCORE_EXPORT void setUserStyleSheetLocation(const URL&);
    const URL& userStyleSheetLocation() const { return m_userStyleSheetLocation; }

    WEBCORE_EXPORT void setNeedsAdobeFrameReloadingQuirk(bool);
    bool needsAcrobatFrameReloadingQuirk() const { return m_needsAdobeFrameReloadingQuirk; }

    WEBCORE_EXPORT static void setDefaultMinDOMTimerInterval(double); // Interval specified in seconds.
    WEBCORE_EXPORT static double defaultMinDOMTimerInterval();
        
    static void setHiddenPageDOMTimerAlignmentInterval(double); // Interval specified in seconds.
    static double hiddenPageDOMTimerAlignmentInterval();

    WEBCORE_EXPORT void setMinDOMTimerInterval(double); // Per-page; initialized to default value.
    WEBCORE_EXPORT double minDOMTimerInterval();

    static void setDefaultDOMTimerAlignmentInterval(double);
    WEBCORE_EXPORT static double defaultDOMTimerAlignmentInterval();

    double domTimerAlignmentInterval() const;

    WEBCORE_EXPORT void setLayoutInterval(std::chrono::milliseconds);
    std::chrono::milliseconds layoutInterval() const { return m_layoutInterval; }

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    bool hiddenPageDOMTimerThrottlingEnabled() const { return m_hiddenPageDOMTimerThrottlingEnabled; }
    WEBCORE_EXPORT void setHiddenPageDOMTimerThrottlingEnabled(bool);
#endif

    WEBCORE_EXPORT void setUsesPageCache(bool);
    bool usesPageCache() const { return m_usesPageCache; }
        
    void setFontRenderingMode(FontRenderingMode mode);
    FontRenderingMode fontRenderingMode() const;

    WEBCORE_EXPORT void setShowTiledScrollingIndicator(bool);
    bool showTiledScrollingIndicator() const { return m_showTiledScrollingIndicator; }

#if PLATFORM(WIN)
    static void setShouldUseHighResolutionTimers(bool);
    static bool shouldUseHighResolutionTimers() { return gShouldUseHighResolutionTimers; }
#endif

    WEBCORE_EXPORT void setBackgroundShouldExtendBeyondPage(bool);
    bool backgroundShouldExtendBeyondPage() const { return m_backgroundShouldExtendBeyondPage; }

#if USE(AVFOUNDATION)
    WEBCORE_EXPORT static void setAVFoundationEnabled(bool flag);
    static bool isAVFoundationEnabled() { return gAVFoundationEnabled; }
#endif

#if PLATFORM(COCOA)
    WEBCORE_EXPORT static void setQTKitEnabled(bool flag);
    static bool isQTKitEnabled() { return gQTKitEnabled; }
#endif

    static const unsigned defaultMaximumHTMLParserDOMTreeDepth = 512;

#if USE(SAFARI_THEME)
    // Windows debugging pref (global) for switching between the Aqua look and a native windows look.
    static void setShouldPaintNativeControls(bool);
    static bool shouldPaintNativeControls() { return gShouldPaintNativeControls; }
#endif

    WEBCORE_EXPORT static void setMockScrollbarsEnabled(bool flag);
    WEBCORE_EXPORT static bool mockScrollbarsEnabled();

    WEBCORE_EXPORT static void setUsesOverlayScrollbars(bool flag);
    static bool usesOverlayScrollbars();

#if ENABLE(TOUCH_EVENTS)
    void setTouchEventEmulationEnabled(bool enabled) { m_touchEventEmulationEnabled = enabled; }
    bool isTouchEventEmulationEnabled() const { return m_touchEventEmulationEnabled; }
#endif

    WEBCORE_EXPORT void setStorageBlockingPolicy(SecurityOrigin::StorageBlockingPolicy);
    SecurityOrigin::StorageBlockingPolicy storageBlockingPolicy() const { return m_storageBlockingPolicy; }

    WEBCORE_EXPORT void setScrollingPerformanceLoggingEnabled(bool);
    bool scrollingPerformanceLoggingEnabled() { return m_scrollingPerformanceLoggingEnabled; }

    WEBCORE_EXPORT static void setShouldRespectPriorityInCSSAttributeSetters(bool);
    static bool shouldRespectPriorityInCSSAttributeSetters();

    void setTimeWithoutMouseMovementBeforeHidingControls(double time) { m_timeWithoutMouseMovementBeforeHidingControls = time; }
    double timeWithoutMouseMovementBeforeHidingControls() const { return m_timeWithoutMouseMovementBeforeHidingControls; }

    bool hiddenPageCSSAnimationSuspensionEnabled() const { return m_hiddenPageCSSAnimationSuspensionEnabled; }
    WEBCORE_EXPORT void setHiddenPageCSSAnimationSuspensionEnabled(bool);

    WEBCORE_EXPORT void setFontFallbackPrefersPictographs(bool);
    bool fontFallbackPrefersPictographs() const { return m_fontFallbackPrefersPictographs; }

    static bool lowPowerVideoAudioBufferSizeEnabled() { return gLowPowerVideoAudioBufferSizeEnabled; }
    WEBCORE_EXPORT static void setLowPowerVideoAudioBufferSizeEnabled(bool);

#if PLATFORM(IOS)
    WEBCORE_EXPORT static void setAudioSessionCategoryOverride(unsigned);
    static unsigned audioSessionCategoryOverride();

    WEBCORE_EXPORT static void setNetworkDataUsageTrackingEnabled(bool);
    static bool networkDataUsageTrackingEnabled();

    WEBCORE_EXPORT static void setNetworkInterfaceName(const String&);
    static const String& networkInterfaceName();

    static void setAVKitEnabled(bool flag) { gAVKitEnabled = flag; }
    static bool avKitEnabled() { return gAVKitEnabled; }

    static void setShouldOptOutOfNetworkStateObservation(bool flag) { gShouldOptOutOfNetworkStateObservation = flag; }
    static bool shouldOptOutOfNetworkStateObservation() { return gShouldOptOutOfNetworkStateObservation; }

    static void setShouldManageAudioSessionCategory(bool flag) { gManageAudioSession = flag; }
    static bool shouldManageAudioSessionCategory() { return gManageAudioSession; }
#endif

private:
    explicit Settings(Page*);

    void initializeDefaultFontFamilies();
    static bool shouldEnableScreenFontSubstitutionByDefault();

    Page* m_page;

    String m_mediaTypeOverride;
    URL m_userStyleSheetLocation;
    const std::unique_ptr<FontGenericFamilies> m_fontGenericFamilies;
    SecurityOrigin::StorageBlockingPolicy m_storageBlockingPolicy;
    std::chrono::milliseconds m_layoutInterval;
#if ENABLE(TEXT_AUTOSIZING)
    float m_textAutosizingFontScaleFactor;
    IntSize m_textAutosizingWindowSizeOverride;
    bool m_textAutosizingEnabled : 1;
#endif

    SETTINGS_MEMBER_VARIABLES

    bool m_screenFontSubstitutionEnabled : 1;
    bool m_isJavaEnabled : 1;
    bool m_isJavaEnabledForLocalFiles : 1;
    bool m_loadsImagesAutomatically : 1;
    bool m_areImagesEnabled : 1;
    bool m_arePluginsEnabled : 1;
    bool m_isScriptEnabled : 1;
    bool m_needsAdobeFrameReloadingQuirk : 1;
    bool m_usesPageCache : 1;
    unsigned m_fontRenderingMode : 1;
    bool m_showTiledScrollingIndicator : 1;
    bool m_backgroundShouldExtendBeyondPage : 1;
    bool m_dnsPrefetchingEnabled : 1;

#if ENABLE(TOUCH_EVENTS)
    bool m_touchEventEmulationEnabled : 1;
#endif
    bool m_scrollingPerformanceLoggingEnabled : 1;

    double m_timeWithoutMouseMovementBeforeHidingControls;

    Timer<Settings> m_setImageLoadingSettingsTimer;
    void imageLoadingSettingsTimerFired(Timer<Settings>*);

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    bool m_hiddenPageDOMTimerThrottlingEnabled : 1;
#endif
    bool m_hiddenPageCSSAnimationSuspensionEnabled : 1;
    bool m_fontFallbackPrefersPictographs : 1;

    static double gDefaultMinDOMTimerInterval;
    static double gDefaultDOMTimerAlignmentInterval;

#if USE(AVFOUNDATION)
    WEBCORE_EXPORT static bool gAVFoundationEnabled;
#endif

#if PLATFORM(COCOA)
    WEBCORE_EXPORT static bool gQTKitEnabled;
#endif
        
    static bool gMockScrollbarsEnabled;
    static bool gUsesOverlayScrollbars;

#if USE(SAFARI_THEME)
    static bool gShouldPaintNativeControls;
#endif
#if PLATFORM(WIN)
    static bool gShouldUseHighResolutionTimers;
#endif
    static bool gShouldRespectPriorityInCSSAttributeSetters;
#if PLATFORM(IOS)
    static bool gNetworkDataUsageTrackingEnabled;
    WEBCORE_EXPORT static bool gAVKitEnabled;
    WEBCORE_EXPORT static bool gShouldOptOutOfNetworkStateObservation;
    WEBCORE_EXPORT static bool gManageAudioSession;
#endif

    static double gHiddenPageDOMTimerAlignmentInterval;

    static bool gLowPowerVideoAudioBufferSizeEnabled;
};

} // namespace WebCore

#endif
