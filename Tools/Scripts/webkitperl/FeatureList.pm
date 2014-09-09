# Copyright (C) 2012 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# A module to contain all the enable/disable feature option code.

use strict;
use warnings;

use FindBin;
use lib $FindBin::Bin;
use webkitdirs;

BEGIN {
   use Exporter   ();
   our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
   $VERSION     = 1.00;
   @ISA         = qw(Exporter);
   @EXPORT      = qw(&getFeatureOptionList);
   %EXPORT_TAGS = ( );
   @EXPORT_OK   = ();
}

my (
    $threeDRenderingSupport,
    $accelerated2DCanvasSupport,
    $batteryStatusSupport,
    $canvasPathSupport,
    $canvasProxySupport,
    $channelMessagingSupport,
    $cspNextSupport,
    $css3ConditionalRulesSupport,
    $css3TextSupport,
    $css3TextLineBreakSupport,
    $css4ImagesSupport,
    $cssBoxDecorationBreakSupport,
    $cssDeviceAdaptation,
    $cssGridLayoutSupport,
    $cssImageOrientationSupport,
    $cssImageResolutionSupport,
    $cssImageSetSupport,
    $cssRegionsSupport,
    $cssShapesSupport,
    $cssCompositingSupport,
    $customSchemeHandlerSupport,
    $dataTransferItemsSupport,
    $datalistElementSupport,
    $detailsElementSupport,
    $deviceOrientationSupport,
    $directoryUploadSupport,
    $dom4EventsConstructor,
    $downloadAttributeSupport,
    $fontLoadEventsSupport,
    $ftpDirSupport,
    $fullscreenAPISupport,
    $gamepadSupport,
    $geolocationSupport,
    $highDPICanvasSupport,
    $icondatabaseSupport,
    $indexedDatabaseSupport,
    $inputSpeechSupport,
    $inputTypeColorSupport,
    $inputTypeDateSupport,
    $inputTypeDatetimeSupport,
    $inputTypeDatetimelocalSupport,
    $inputTypeMonthSupport,
    $inputTypeTimeSupport,
    $inputTypeWeekSupport,
    $inspectorSupport,
    $legacyNotificationsSupport,
    $legacyVendorPrefixSupport,
    $legacyWebAudioSupport,
    $linkPrefetchSupport,
    $linkPrerenderSupport,
    $mathmlSupport,
    $mediaCaptureSupport,
    $mediaSourceSupport,
    $mediaStatisticsSupport,
    $mediaStreamSupport,
    $meterElementSupport,
    $mhtmlSupport,
    $mouseCursorScaleSupport,
    $netscapePluginAPISupport,
    $networkInfoSupport,
    $nosniffSupport,
    $notificationsSupport,
    $orientationEventsSupport,
    $pageVisibilityAPISupport,
    $performanceTimelineSupport,
    $pictureSizesSupport,
    $promiseSupport,
    $proximityEventsSupport,
    $quotaSupport,
    $resolutionMediaQuerySupport,
    $registerProtocolHandlerSupport,
    $requestAnimationFrameSupport,
    $resourceTimingSupport,
    $scriptedSpeechSupport,
    $seccompFiltersSupport,
    $sharedWorkersSupport,
    $sqlDatabaseSupport,
    $styleScopedSupport,
    $subtleCrypto,
    $suidLinuxSandbox,
    $svgDOMObjCBindingsSupport,
    $svgFontsSupport,
    $svgSupport,
    $systemMallocSupport,
    $templateElementSupport,
    $textAutosizingSupport,
    $tiledBackingStoreSupport,
    $threadedHTMLParserSupport,
    $touchEventsSupport,
    $touchSliderSupport,
    $touchIconLoadingSupport,
    $userTimingSupport,
    $vibrationSupport,
    $videoSupport,
    $videoTrackSupport,
    $webglSupport,
    $webAudioSupport,
    $webReplaySupport,
    $webSocketsSupport,
    $webTimingSupport,
    $xhrTimeoutSupport,
    $xsltSupport,
    $ftlJITSupport,
    $forceCLoop,
);

my @features = (
    { option => "3d-rendering", desc => "Toggle 3D Rendering support",
      define => "ENABLE_3D_RENDERING", default => (isAppleMacWebKit() || isIOSWebKit() || isGtk() || isEfl()), value => \$threeDRenderingSupport },

    { option => "accelerated-2d-canvas", desc => "Toggle Accelerated 2D Canvas support",
      define => "ENABLE_ACCELERATED_2D_CANVAS", default => isGtk(), value => \$accelerated2DCanvasSupport },

    { option => "battery-status", desc => "Toggle Battery Status support",
      define => "ENABLE_BATTERY_STATUS", default => isEfl(), value => \$batteryStatusSupport },

    { option => "canvas-path", desc => "Toggle Canvas Path support",
      define => "ENABLE_CANVAS_PATH", default => 1, value => \$canvasPathSupport },

    { option => "canvas-proxy", desc => "Toggle CanvasProxy support",
      define => "ENABLE_CANVAS_PROXY", default => 0, value => \$canvasProxySupport },

    { option => "channel-messaging", desc => "Toggle Channel Messaging support",
      define => "ENABLE_CHANNEL_MESSAGING", default => 1, value => \$channelMessagingSupport },

    { option => "csp-next", desc => "Toggle Content Security Policy 1.1 support",
      define => "ENABLE_CSP_NEXT", default => isGtk(), value => \$cspNextSupport },

    { option => "css-device-adaptation", desc => "Toggle CSS Device Adaptation support",
      define => "ENABLE_CSS_DEVICE_ADAPTATION", default => isEfl(), value => \$cssDeviceAdaptation },

    { option => "css-shapes", desc => "Toggle CSS Shapes support",
      define => "ENABLE_CSS_SHAPES", default => 1, value => \$cssShapesSupport },

    { option => "css-grid-layout", desc => "Toggle CSS Grid Layout support",
      define => "ENABLE_CSS_GRID_LAYOUT", default => 1, value => \$cssGridLayoutSupport },

    { option => "css3-conditional-rules", desc => "Toggle CSS3 Conditional Rules support (i.e. \@supports)",
      define => "ENABLE_CSS3_CONDITIONAL_RULES", default => 1, value => \$css3ConditionalRulesSupport },

    { option => "css3-text", desc => "Toggle CSS3 Text support",
      define => "ENABLE_CSS3_TEXT", default => (isEfl() || isGtk()), value => \$css3TextSupport },

    { option => "css3-text-line-break", desc => "Toggle CSS3 Text Line Break support",
      define => "ENABLE_CSS3_TEXT_LINE_BREAK", default => 0, value => \$css3TextLineBreakSupport },

    { option => "css-box-decoration-break", desc => "Toggle CSS box-decoration-break support",
      define => "ENABLE_CSS_BOX_DECORATION_BREAK", default => 1, value => \$cssBoxDecorationBreakSupport },

    { option => "css-image-orientation", desc => "Toggle CSS image-orientation support",
      define => "ENABLE_CSS_IMAGE_ORIENTATION", default => (isEfl() || isGtk()), value => \$cssImageOrientationSupport },

    { option => "css-image-resolution", desc => "Toggle CSS image-resolution support",
      define => "ENABLE_CSS_IMAGE_RESOLUTION", default => isGtk(), value => \$cssImageResolutionSupport },

    { option => "css-image-set", desc => "Toggle CSS image-set support",
      define => "ENABLE_CSS_IMAGE_SET", default => (isEfl() || isGtk()), value => \$cssImageSetSupport },

    { option => "css-regions", desc => "Toggle CSS Regions support",
      define => "ENABLE_CSS_REGIONS", default => 1, value => \$cssRegionsSupport },

    { option => "css-compositing", desc => "Toggle CSS Compositing support",
      define => "ENABLE_CSS_COMPOSITING", default => isAppleWebKit(), value => \$cssCompositingSupport },

    { option => "custom-scheme-handler", desc => "Toggle Custom Scheme Handler support",
      define => "ENABLE_CUSTOM_SCHEME_HANDLER", default => isEfl(), value => \$customSchemeHandlerSupport },

    { option => "datalist-element", desc => "Toggle Datalist Element support",
      define => "ENABLE_DATALIST_ELEMENT", default => isEfl(), value => \$datalistElementSupport },

    { option => "data-transfer-items", desc => "Toggle Data Transfer Items support",
      define => "ENABLE_DATA_TRANSFER_ITEMS", default => 0, value => \$dataTransferItemsSupport },

    { option => "details-element", desc => "Toggle Details Element support",
      define => "ENABLE_DETAILS_ELEMENT", default => 1, value => \$detailsElementSupport },

    { option => "device-orientation", desc => "Toggle Device Orientation support",
      define => "ENABLE_DEVICE_ORIENTATION", default => isIOSWebKit(), value => \$deviceOrientationSupport },

    { option => "dom4-events-constructor", desc => "Expose DOM4 Events constructors",
      define => "ENABLE_DOM4_EVENTS_CONSTRUCTOR", default => (isAppleWebKit() || isGtk() || isEfl()), value => \$dom4EventsConstructor },

    { option => "download-attribute", desc => "Toggle Download Attribute support",
      define => "ENABLE_DOWNLOAD_ATTRIBUTE", default => isEfl(), value => \$downloadAttributeSupport },

    { option => "font-load-events", desc => "Toggle Font Load Events support",
      define => "ENABLE_FONT_LOAD_EVENTS", default => 0, value => \$fontLoadEventsSupport },

    { option => "ftpdir", desc => "Toggle FTP Directory support",
      define => "ENABLE_FTPDIR", default => !isWinCE(), value => \$ftpDirSupport },

    { option => "fullscreen-api", desc => "Toggle Fullscreen API support",
      define => "ENABLE_FULLSCREEN_API", default => (isAppleMacWebKit() || isEfl() || isGtk()), value => \$fullscreenAPISupport },

    { option => "gamepad", desc => "Toggle Gamepad support",
      define => "ENABLE_GAMEPAD", default => 0, value => \$gamepadSupport },

    { option => "geolocation", desc => "Toggle Geolocation support",
      define => "ENABLE_GEOLOCATION", default => (isAppleWebKit() || isIOSWebKit() || isGtk() || isEfl()), value => \$geolocationSupport },

    { option => "high-dpi-canvas", desc => "Toggle High DPI Canvas support",
      define => "ENABLE_HIGH_DPI_CANVAS", default => (isAppleWebKit()), value => \$highDPICanvasSupport },

    { option => "icon-database", desc => "Toggle Icondatabase support",
      define => "ENABLE_ICONDATABASE", default => !isIOSWebKit(), value => \$icondatabaseSupport },

    { option => "indexed-database", desc => "Toggle Indexed Database support",
      define => "ENABLE_INDEXED_DATABASE", default => 0, value => \$indexedDatabaseSupport },

    { option => "input-speech", desc => "Toggle Input Speech support",
      define => "ENABLE_INPUT_SPEECH", default => 0, value => \$inputSpeechSupport },

    { option => "input-type-color", desc => "Toggle Input Type Color support",
      define => "ENABLE_INPUT_TYPE_COLOR", default => isEfl(), value => \$inputTypeColorSupport },

    { option => "input-type-date", desc => "Toggle Input Type Date support",
      define => "ENABLE_INPUT_TYPE_DATE", default => 0, value => \$inputTypeDateSupport },

    { option => "input-type-datetime", desc => "Toggle broken Input Type Datetime support",
      define => "ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE", default => 0, value => \$inputTypeDatetimeSupport },

    { option => "input-type-datetimelocal", desc => "Toggle Input Type Datetimelocal support",
      define => "ENABLE_INPUT_TYPE_DATETIMELOCAL", default => 0, value => \$inputTypeDatetimelocalSupport },

    { option => "input-type-month", desc => "Toggle Input Type Month support",
      define => "ENABLE_INPUT_TYPE_MONTH", default => 0, value => \$inputTypeMonthSupport },

    { option => "input-type-time", desc => "Toggle Input Type Time support",
      define => "ENABLE_INPUT_TYPE_TIME", default => 0, value => \$inputTypeTimeSupport },

    { option => "input-type-week", desc => "Toggle Input Type Week support",
      define => "ENABLE_INPUT_TYPE_WEEK", default => 0, value => \$inputTypeWeekSupport },

    { option => "inspector", desc => "Toggle Inspector support",
      define => "ENABLE_INSPECTOR", default => !isWinCE(), value => \$inspectorSupport },

    { option => "legacy-notifications", desc => "Toggle Legacy Notifications support",
      define => "ENABLE_LEGACY_NOTIFICATIONS", default => 0, value => \$legacyNotificationsSupport },

    { option => "legacy-vendor-prefixes", desc => "Toggle Legacy Vendor Prefix support",
      define => "ENABLE_LEGACY_VENDOR_PREFIXES", default => 1, value => \$legacyVendorPrefixSupport },

    { option => "legacy-web-audio", desc => "Toggle Legacy Web Audio support",
      define => "ENABLE_LEGACY_WEB_AUDIO", default => 1, value => \$legacyWebAudioSupport },

    { option => "link-prefetch", desc => "Toggle Link Prefetch support",
      define => "ENABLE_LINK_PREFETCH", default => (isGtk() || isEfl()), value => \$linkPrefetchSupport },

    { option => "mathml", desc => "Toggle MathML support",
      define => "ENABLE_MATHML", default => 1, value => \$mathmlSupport },

    { option => "media-capture", desc => "Toggle Media Capture support",
      define => "ENABLE_MEDIA_CAPTURE", default => isEfl(), value => \$mediaCaptureSupport },

    { option => "media-source", desc => "Toggle Media Source support",
      define => "ENABLE_MEDIA_SOURCE", default => isGtk(), value => \$mediaSourceSupport },

    { option => "media-statistics", desc => "Toggle Media Statistics support",
      define => "ENABLE_MEDIA_STATISTICS", default => 0, value => \$mediaStatisticsSupport },

    { option => "media-stream", desc => "Toggle Media Stream support",
      define => "ENABLE_MEDIA_STREAM", default => (isGtk() || isEfl()), value => \$mediaStreamSupport },

    { option => "meter-element", desc => "Toggle Meter Element support",
      define => "ENABLE_METER_ELEMENT", default => !isAppleWinWebKit(), value => \$meterElementSupport },

    { option => "mhtml", desc => "Toggle MHTML support",
      define => "ENABLE_MHTML", default => (isGtk() || isEfl()), value => \$mhtmlSupport },

    { option => "mouse-cursor-scale", desc => "Toggle Scaled mouse cursor support",
      define => "ENABLE_MOUSE_CURSOR_SCALE", default => isEfl(), value => \$mouseCursorScaleSupport },

    { option => "navigator-content-utils", desc => "Toggle Navigator Content Utils support",
      define => "ENABLE_NAVIGATOR_CONTENT_UTILS", default => isEfl(), value => \$registerProtocolHandlerSupport },

    { option => "navigator-hardware-concurrency", desc => "Toggle Navigator hardware concurrenct support",
      define => "ENABLE_NAVIGATOR_HWCONCURRENCY", default => 1, value => \$registerProtocolHandlerSupport },

    { option => "netscape-plugin-api", desc => "Toggle Netscape Plugin API support",
      define => "ENABLE_NETSCAPE_PLUGIN_API", default => !isIOSWebKit(), value => \$netscapePluginAPISupport },

    { option => "nosniff", desc => "Toggle support for 'X-Content-Type-Options: nosniff'",
      define => "ENABLE_NOSNIFF", default => isEfl(), value => \$nosniffSupport },

    { option => "notifications", desc => "Toggle Notifications support",
      define => "ENABLE_NOTIFICATIONS", default => 0, value => \$notificationsSupport },

    { option => "orientation-events", desc => "Toggle Orientation Events support",
      define => "ENABLE_ORIENTATION_EVENTS", default => isIOSWebKit(), value => \$orientationEventsSupport },

    { option => "performance-timeline", desc => "Toggle Performance Timeline support",
      define => "ENABLE_PERFORMANCE_TIMELINE", default => isGtk(), value => \$performanceTimelineSupport },

    { option => "picture-sizes", desc => "Toggle sizes attribute support",
      define => "ENABLE_PICTURE_SIZES", default => 1, value => \$pictureSizesSupport },

    { option => "promises", desc => "Toggle Promise support",
      define => "ENABLE_PROMISES", default => 1, value => \$promiseSupport },

    { option => "proximity-events", desc => "Toggle Proximity Events support",
      define => "ENABLE_PROXIMITY_EVENTS", default => 0, value => \$proximityEventsSupport },

    { option => "quota", desc => "Toggle Quota support",
      define => "ENABLE_QUOTA", default => 0, value => \$quotaSupport },

    { option => "resolution-media-query", desc => "Toggle resolution media query support",
      define => "ENABLE_RESOLUTION_MEDIA_QUERY", default => isEfl(), value => \$resolutionMediaQuerySupport },

    { option => "resource-timing", desc => "Toggle Resource Timing support",
      define => "ENABLE_RESOURCE_TIMING", default => isGtk(), value => \$resourceTimingSupport },

    { option => "request-animation-frame", desc => "Toggle Request Animation Frame support",
      define => "ENABLE_REQUEST_ANIMATION_FRAME", default => (isAppleMacWebKit() || isGtk() || isEfl()), value => \$requestAnimationFrameSupport },

    { option => "seccomp-filters", desc => "Toggle Seccomp Filter sandbox",
      define => "ENABLE_SECCOMP_FILTERS", default => 0, value => \$seccompFiltersSupport },

    { option => "scripted-speech", desc => "Toggle Scripted Speech support",
      define => "ENABLE_SCRIPTED_SPEECH", default => 0, value => \$scriptedSpeechSupport },

    { option => "shared-workers", desc => "Toggle Shared Workers support",
      define => "ENABLE_SHARED_WORKERS", default => (isAppleWebKit() || isGtk() || isEfl()), value => \$sharedWorkersSupport },

    { option => "sql-database", desc => "Toggle SQL Database support",
      define => "ENABLE_SQL_DATABASE", default => 1, value => \$sqlDatabaseSupport },

    { option => "subtle-crypto", desc => "Toggle WebCrypto Subtle-Crypto support",
      define => "ENABLE_SUBTLE_CRYPTO", default => (isGtk() || isAppleMacWebKit() || isIOSWebKit()), value => \$subtleCrypto },

    { option => "suid-linux-sandbox", desc => "Toggle suid sandbox for linux",
      define => "ENABLE_SUID_SANDBOX_LINUX", default => 0, value => \$suidLinuxSandbox },

    { option => "svg-fonts", desc => "Toggle SVG Fonts support",
      define => "ENABLE_SVG_FONTS", default => 1, value => \$svgFontsSupport },

    { option => "system-malloc", desc => "Toggle system allocator instead of TCmalloc",
      define => "USE_SYSTEM_MALLOC", default => isWinCE(), value => \$systemMallocSupport },

    { option => "template-element", desc => "Toggle HTMLTemplateElement support",
      define => "ENABLE_TEMPLATE_ELEMENT", default => 1, value => \$templateElementSupport },

    { option => "text-autosizing", desc => "Toggle Text Autosizing support",
      define => "ENABLE_TEXT_AUTOSIZING", default => 0, value => \$textAutosizingSupport },

    { option => "touch-events", desc => "Toggle Touch Events support",
      define => "ENABLE_TOUCH_EVENTS", default => (isIOSWebKit() || isEfl() || isGtk()), value => \$touchEventsSupport },

    { option => "touch-slider", desc => "Toggle Touch Slider support",
      define => "ENABLE_TOUCH_SLIDER", default => isEfl(), value => \$touchSliderSupport },

    { option => "touch-icon-loading", desc => "Toggle Touch Icon Loading Support",
      define => "ENABLE_TOUCH_ICON_LOADING", default => 0, value => \$touchIconLoadingSupport },

    { option => "user-timing", desc => "Toggle User Timing support",
      define => "ENABLE_USER_TIMING", default => isGtk(), value => \$userTimingSupport },

    { option => "vibration", desc => "Toggle Vibration support",
      define => "ENABLE_VIBRATION", default => isEfl(), value => \$vibrationSupport },

    { option => "video", desc => "Toggle Video support",
      define => "ENABLE_VIDEO", default => (isAppleWebKit() || isGtk() || isEfl()), value => \$videoSupport },

    { option => "video-track", desc => "Toggle Video Track support",
      define => "ENABLE_VIDEO_TRACK", default => (isAppleWebKit() || isGtk() || isEfl()), value => \$videoTrackSupport },

    { option => "webgl", desc => "Toggle WebGL support",
      define => "ENABLE_WEBGL", default => (isAppleMacWebKit() || isIOSWebKit() || isGtk() || isEfl()), value => \$webglSupport },

    { option => "web-audio", desc => "Toggle Web Audio support",
      define => "ENABLE_WEB_AUDIO", default => (isEfl() || isGtk()), value => \$webAudioSupport },

    { option => "web-replay", desc => "Toggle Web Replay support",
      define => "ENABLE_WEB_REPLAY", default => isAppleMacWebKit(), value => \$webReplaySupport },

    { option => "web-sockets", desc => "Toggle Web Sockets support",
      define => "ENABLE_WEB_SOCKETS", default => 1, value => \$webSocketsSupport },

    { option => "web-timing", desc => "Toggle Web Timing support",
      define => "ENABLE_WEB_TIMING", default => (isGtk() || isEfl()), value => \$webTimingSupport },

    { option => "xhr-timeout", desc => "Toggle XHR Timeout support",
      define => "ENABLE_XHR_TIMEOUT", default => (isEfl() || isGtk() || isAppleMacWebKit()), value => \$xhrTimeoutSupport },

    { option => "xslt", desc => "Toggle XSLT support",
      define => "ENABLE_XSLT", default => 1, value => \$xsltSupport },

    { option => "ftl-jit", desc => "Toggle FTLJIT support",
      define => "ENABLE_FTL_JIT", default => 0, value => \$ftlJITSupport },

    { option => "cloop", desc => "Force use of the llint c loop",
      define => "ENABLE_LLINT_C_LOOP", default => 0, value => \$forceCLoop },
);

sub getFeatureOptionList()
{
    return @features;
}

1;
