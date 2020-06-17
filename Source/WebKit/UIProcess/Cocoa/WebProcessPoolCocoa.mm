/*
 * Copyright (C) 2010-2020 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WebProcessPool.h"

#import "AccessibilitySupportSPI.h"
#import "CookieStorageUtilsCF.h"
#import "LegacyCustomProtocolManagerClient.h"
#import "Logging.h"
#import "NetworkProcessCreationParameters.h"
#import "NetworkProcessMessages.h"
#import "NetworkProcessProxy.h"
#import "PluginProcessManager.h"
#import "PreferenceObserver.h"
#import "SandboxUtilities.h"
#import "TextChecker.h"
#import "UserInterfaceIdiom.h"
#import "VersionChecks.h"
#import "WKBrowsingContextControllerInternal.h"
#import "WebBackForwardCache.h"
#import "WebMemoryPressureHandler.h"
#import "WebPageGroup.h"
#import "WebPreferencesKeys.h"
#import "WebProcessCache.h"
#import "WebProcessCreationParameters.h"
#import "WebProcessMessages.h"
#import "WindowServerConnection.h"
#import <WebCore/AGXCompilerService.h>
#import <WebCore/Color.h>
#import <WebCore/LocalizedDeviceModel.h>
#import <WebCore/NetworkStorageSession.h>
#import <WebCore/NotImplemented.h>
#import <WebCore/PictureInPictureSupport.h>
#import <WebCore/PlatformPasteboard.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/UTIUtilities.h>
#import <objc/runtime.h>
#import <pal/cf/CoreMediaSoftLink.h>
#import <pal/cocoa/MediaToolboxSoftLink.h>
#import <pal/spi/cf/CFNetworkSPI.h>
#import <sys/param.h>
#import <wtf/FileSystem.h>
#import <wtf/ProcessPrivilege.h>
#import <wtf/SoftLinking.h>
#import <wtf/cocoa/Entitlements.h>
#import <wtf/spi/darwin/SandboxSPI.h>
#import <wtf/spi/darwin/dyldSPI.h>

#if ENABLE(REMOTE_INSPECTOR)
#import <JavaScriptCore/RemoteInspector.h>
#import <JavaScriptCore/RemoteInspectorConstants.h>
#endif

#if PLATFORM(MAC)
#import <QuartzCore/CARemoteLayerServer.h>
#else
#import "UIKitSPI.h"
#import <pal/ios/ManagedConfigurationSoftLink.h>
#import <pal/spi/ios/ManagedConfigurationSPI.h>
#endif

#if PLATFORM(IOS_FAMILY)
#import <pal/spi/ios/MobileGestaltSPI.h>
#endif

#if PLATFORM(IOS)
#import <pal/spi/cocoa/WebFilterEvaluatorSPI.h>

SOFT_LINK_PRIVATE_FRAMEWORK(WebContentAnalysis);
SOFT_LINK_CLASS(WebContentAnalysis, WebFilterEvaluator);
#endif

#if PLATFORM(COCOA)
#import <WebCore/SystemBattery.h>
#import <pal/spi/cocoa/NEFilterSourceSPI.h>

SOFT_LINK_FRAMEWORK_OPTIONAL(NetworkExtension);
SOFT_LINK_CLASS_OPTIONAL(NetworkExtension, NEFilterSource);
#endif

NSString *WebServiceWorkerRegistrationDirectoryDefaultsKey = @"WebServiceWorkerRegistrationDirectory";
NSString *WebKitLocalCacheDefaultsKey = @"WebKitLocalCache";
NSString *WebKitJSCJITEnabledDefaultsKey = @"WebKitJSCJITEnabledDefaultsKey";
NSString *WebKitJSCFTLJITEnabledDefaultsKey = @"WebKitJSCFTLJITEnabledDefaultsKey";

#if !PLATFORM(IOS_FAMILY) || PLATFORM(MACCATALYST)
static NSString *WebKitApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification = @"NSApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification";
#endif

static NSString * const WebKitSuppressMemoryPressureHandlerDefaultsKey = @"WebKitSuppressMemoryPressureHandler";

#if ENABLE(RESOURCE_LOAD_STATISTICS) && !RELEASE_LOG_DISABLED
static NSString * const WebKitLogCookieInformationDefaultsKey = @"WebKitLogCookieInformation";
#endif

#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
SOFT_LINK_PRIVATE_FRAMEWORK(BackBoardServices)
SOFT_LINK(BackBoardServices, BKSDisplayBrightnessGetCurrent, float, (), ());
#endif

#define WEBPROCESSPOOL_RELEASE_LOG(channel, fmt, ...) RELEASE_LOG(channel, "%p - WebProcessPool::" fmt, this, ##__VA_ARGS__)

namespace WebKit {
using namespace WebCore;

static void registerUserDefaultsIfNeeded()
{
    static bool didRegister;
    if (didRegister)
        return;

    didRegister = true;
    NSMutableDictionary *registrationDictionary = [NSMutableDictionary dictionary];
    
    [registrationDictionary setObject:@YES forKey:WebKitJSCJITEnabledDefaultsKey];
    [registrationDictionary setObject:@YES forKey:WebKitJSCFTLJITEnabledDefaultsKey];

    [[NSUserDefaults standardUserDefaults] registerDefaults:registrationDictionary];
}

void WebProcessPool::updateProcessSuppressionState()
{
    if (m_networkProcess)
        m_networkProcess->setProcessSuppressionEnabled(processSuppressionEnabled());

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (!m_processSuppressionDisabledForPageCounter.value())
        m_pluginProcessManagerProcessSuppressionDisabledToken = nullptr;
    else if (!m_pluginProcessManagerProcessSuppressionDisabledToken)
        m_pluginProcessManagerProcessSuppressionDisabledToken = PluginProcessManager::singleton().processSuppressionDisabledToken();
#endif
}

NSMutableDictionary *WebProcessPool::ensureBundleParameters()
{
    if (!m_bundleParameters)
        m_bundleParameters = adoptNS([[NSMutableDictionary alloc] init]);

    return m_bundleParameters.get();
}

void WebProcessPool::platformInitialize()
{
    registerUserDefaultsIfNeeded();
    registerNotificationObservers();
    initializeClassesForParameterCoding();

    // FIXME: This should be able to share code with WebCore's MemoryPressureHandler (and be platform independent).
    // Right now it cannot because WebKit1 and WebKit2 need to be able to coexist in the UI process,
    // and you can only have one WebCore::MemoryPressureHandler.

    if (![[NSUserDefaults standardUserDefaults] boolForKey:@"WebKitSuppressMemoryPressureHandler"])
        installMemoryPressureHandler();

    setLegacyCustomProtocolManagerClient(makeUnique<LegacyCustomProtocolManagerClient>());
}

#if PLATFORM(IOS_FAMILY)
String WebProcessPool::cookieStorageDirectory()
{
    String path = pathForProcessContainer();
    if (path.isEmpty())
        path = NSHomeDirectory();

    path = path + "/Library/Cookies";
    path = stringByResolvingSymlinksInPath(path);
    return path;
}
#endif

void WebProcessPool::platformResolvePathsForSandboxExtensions()
{
    m_resolvedPaths.uiProcessBundleResourcePath = resolvePathForSandboxExtension([[NSBundle mainBundle] resourcePath]);

#if PLATFORM(IOS_FAMILY)
    m_resolvedPaths.cookieStorageDirectory = resolveAndCreateReadWriteDirectoryForSandboxExtension(WebProcessPool::cookieStorageDirectory());
    m_resolvedPaths.containerCachesDirectory = resolveAndCreateReadWriteDirectoryForSandboxExtension(WebProcessPool::webContentCachesDirectory());
    m_resolvedPaths.containerTemporaryDirectory = resolveAndCreateReadWriteDirectoryForSandboxExtension(WebProcessPool::containerTemporaryDirectory());
#endif
}

#if PLATFORM(IOS_FAMILY)
static bool isInternalInstall()
{
    static bool isInternal = MGGetBoolAnswer(kMGQAppleInternalInstallCapability);
    return isInternal;
}
#endif

// FIXME(207716): The following should be removed when the GPU process is complete.
static const Vector<String>& mediaRelatedMachServices()
{
    ASSERT(isMainThread());
    static const auto services = makeNeverDestroyed(Vector<String> {
        "com.apple.audio.AudioComponentPrefs", "com.apple.audio.AudioComponentRegistrar",
        "com.apple.audio.AudioQueueServer", "com.apple.audio.toolbox.reporting.service", "com.apple.coremedia.endpoint.xpc",
        "com.apple.coremedia.routediscoverer.xpc", "com.apple.coremedia.routingcontext.xpc",
        "com.apple.coremedia.volumecontroller.xpc", "com.apple.accessibility.mediaaccessibilityd",
        "com.apple.mediaremoted.xpc",
#if PLATFORM(IOS_FAMILY)
        "com.apple.audio.AudioSession", "com.apple.MediaPlayer.RemotePlayerService",
        "com.apple.coremedia.admin",
        "com.apple.coremedia.asset.xpc", "com.apple.coremedia.assetimagegenerator.xpc",
        "com.apple.coremedia.audiodeviceclock.xpc", "com.apple.coremedia.audioprocessingtap.xpc",
        "com.apple.coremedia.capturesession", "com.apple.coremedia.capturesource",
        "com.apple.coremedia.compressionsession", "com.apple.coremedia.cpe.xpc",
        "com.apple.coremedia.cpeprotector.xpc", "com.apple.coremedia.customurlloader.xpc",
        "com.apple.coremedia.decompressionsession", "com.apple.coremedia.figcontentkeysession.xpc",
        "com.apple.coremedia.figcpecryptor", "com.apple.coremedia.formatreader.xpc",
        "com.apple.coremedia.player.xpc", "com.apple.coremedia.remaker",
        "com.apple.coremedia.remotequeue", "com.apple.coremedia.routingsessionmanager.xpc",
        "com.apple.coremedia.samplebufferaudiorenderer.xpc", "com.apple.coremedia.samplebufferrendersynchronizer.xpc",
        "com.apple.coremedia.sandboxserver.xpc", "com.apple.coremedia.sts",
        "com.apple.coremedia.systemcontroller.xpc", "com.apple.coremedia.videoqueue",
        "com.apple.coremedia.visualcontext.xpc", "com.apple.airplay.apsynccontroller.xpc",
        "com.apple.audio.AURemoteIOServer"
#endif
#if PLATFORM(MAC) || PLATFORM(MACCATALYST)
        "com.apple.coremedia.endpointstream.xpc", "com.apple.coremedia.endpointplaybacksession.xpc",
        "com.apple.coremedia.endpointremotecontrolsession.xpc", "com.apple.coremedia.videodecoder",
        "com.apple.coremedia.videoencoder", "com.apple.BluetoothServices"
#endif
    });
    return services;
}

#if PLATFORM(IOS_FAMILY)
static const Vector<String>& nonBrowserServices()
{
    ASSERT(isMainThread());
    static const auto services = makeNeverDestroyed(Vector<String> {
        "com.apple.lsd.open",
        "com.apple.mobileassetd",
        "com.apple.iconservices",
        "com.apple.PowerManagement.control",
        "com.apple.frontboard.systemappservices"
    });
    return services;
}

static const Vector<String>& diagnosticServices()
{
    ASSERT(isMainThread());
    static const auto services = makeNeverDestroyed(Vector<String> {
        "com.apple.diagnosticd",
        "com.apple.osanalytics.osanalyticshelper"
    });
    return services;
}

static const Vector<String>& agxCompilerClasses()
{
    ASSERT(isMainThread());
    static const auto iokitClasses = makeNeverDestroyed(Vector<String> {
        "AGXCommandQueue",
        "AGXDevice",
        "AGXSharedUserClient",
        "IOAccelContext",
        "IOAccelContext2",
        "IOAccelDevice",
        "IOAccelDevice2",
        "IOAccelSharedUserClient",
        "IOAccelSharedUserClient2"
        "IOAccelSubmitter2",
    });
    return iokitClasses;
}

#endif

void WebProcessPool::platformInitializeWebProcess(const WebProcessProxy& process, WebProcessCreationParameters& parameters)
{
    parameters.mediaMIMETypes = process.mediaMIMETypes();

#if PLATFORM(MAC)
    ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    ASSERT(hasProcessPrivilege(ProcessPrivilege::CanCommunicateWithWindowServer));
    parameters.accessibilityEnhancedUserInterfaceEnabled = [[NSApp accessibilityAttributeValue:@"AXEnhancedUserInterface"] boolValue];
    ALLOW_DEPRECATED_DECLARATIONS_END
#else
    parameters.accessibilityEnhancedUserInterfaceEnabled = false;
#endif

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    parameters.shouldEnableJIT = [defaults boolForKey:WebKitJSCJITEnabledDefaultsKey];
    parameters.shouldEnableFTLJIT = [defaults boolForKey:WebKitJSCFTLJITEnabledDefaultsKey];
    parameters.shouldEnableMemoryPressureReliefLogging = [defaults boolForKey:@"LogMemoryJetsamDetails"];
    parameters.shouldSuppressMemoryPressureHandler = [defaults boolForKey:WebKitSuppressMemoryPressureHandlerDefaultsKey];

#if HAVE(HOSTED_CORE_ANIMATION)
    parameters.acceleratedCompositingPort = MachSendRight::create([CARemoteLayerServer sharedServer].serverPort);
#endif

    // FIXME: This should really be configurable; we shouldn't just blindly allow read access to the UI process bundle.
    parameters.uiProcessBundleResourcePath = m_resolvedPaths.uiProcessBundleResourcePath;
    SandboxExtension::createHandleWithoutResolvingPath(parameters.uiProcessBundleResourcePath, SandboxExtension::Type::ReadOnly, parameters.uiProcessBundleResourcePathExtensionHandle);

    parameters.uiProcessBundleIdentifier = applicationBundleIdentifier();
    parameters.uiProcessSDKVersion = dyld_get_program_sdk_version();

#if PLATFORM(IOS_FAMILY)
    if (!m_resolvedPaths.cookieStorageDirectory.isEmpty())
        SandboxExtension::createHandleWithoutResolvingPath(m_resolvedPaths.cookieStorageDirectory, SandboxExtension::Type::ReadWrite, parameters.cookieStorageDirectoryExtensionHandle);

    if (!m_resolvedPaths.containerCachesDirectory.isEmpty())
        SandboxExtension::createHandleWithoutResolvingPath(m_resolvedPaths.containerCachesDirectory, SandboxExtension::Type::ReadWrite, parameters.containerCachesDirectoryExtensionHandle);

    if (!m_resolvedPaths.containerTemporaryDirectory.isEmpty())
        SandboxExtension::createHandleWithoutResolvingPath(m_resolvedPaths.containerTemporaryDirectory, SandboxExtension::Type::ReadWrite, parameters.containerTemporaryDirectoryExtensionHandle);
#endif

    parameters.fontWhitelist = m_fontWhitelist;

    if (m_bundleParameters) {
        auto keyedArchiver = adoptNS([[NSKeyedArchiver alloc] initRequiringSecureCoding:YES]);

        @try {
            [keyedArchiver encodeObject:m_bundleParameters.get() forKey:@"parameters"];
            [keyedArchiver finishEncoding];
        } @catch (NSException *exception) {
            LOG_ERROR("Failed to encode bundle parameters: %@", exception);
        }

        auto data = keyedArchiver.get().encodedData;

        parameters.bundleParameterData = API::Data::createWithoutCopying(WTFMove(data));
    }
    parameters.networkATSContext = adoptCF(_CFNetworkCopyATSContext());

#if ENABLE(MEDIA_STREAM)
    // Allow microphone access if either preference is set because WebRTC requires microphone access.
    bool mediaDevicesEnabled = m_defaultPageGroup->preferences().mediaDevicesEnabled();
    bool webRTCEnabled = m_defaultPageGroup->preferences().peerConnectionEnabled();
    if ([defaults objectForKey:@"ExperimentalPeerConnectionEnabled"])
        webRTCEnabled = [defaults boolForKey:@"ExperimentalPeerConnectionEnabled"];

    bool isSafari = false;
#if PLATFORM(IOS_FAMILY)
    if (WebCore::IOSApplication::isMobileSafari())
        isSafari = true;
#elif PLATFORM(MAC)
    if (WebCore::MacApplication::isSafari())
        isSafari = true;
#endif

#if !LOG_DISABLED || !RELEASE_LOG_DISABLED
    parameters.webCoreLoggingChannels = [[NSUserDefaults standardUserDefaults] stringForKey:@"WebCoreLogging"];
    parameters.webKitLoggingChannels = [[NSUserDefaults standardUserDefaults] stringForKey:@"WebKit2Logging"];
#endif

    // FIXME: Remove this and related parameter when <rdar://problem/29448368> is fixed.
    if (isSafari && mediaDevicesEnabled && !m_defaultPageGroup->preferences().captureAudioInUIProcessEnabled() && !m_defaultPageGroup->preferences().captureAudioInGPUProcessEnabled())
        SandboxExtension::createHandleForGenericExtension("com.apple.webkit.microphone", parameters.audioCaptureExtensionHandle);
#endif

#if ENABLE(RESOURCE_LOAD_STATISTICS) && !RELEASE_LOG_DISABLED
    parameters.shouldLogUserInteraction = [defaults boolForKey:WebKitLogCookieInformationDefaultsKey];
#endif

    auto screenProperties = WebCore::collectScreenProperties();
    parameters.screenProperties = WTFMove(screenProperties);
#if PLATFORM(MAC)
    parameters.useOverlayScrollbars = ([NSScroller preferredScrollerStyle] == NSScrollerStyleOverlay);
#endif
    
#if PLATFORM(IOS)
    if (WebCore::deviceHasAGXCompilerService()) {
        SandboxExtension::Handle compilerServiceExtensionHandle;
        SandboxExtension::createHandleForMachLookup("com.apple.AGXCompilerService", WTF::nullopt, compilerServiceExtensionHandle);
        parameters.compilerServiceExtensionHandle = WTFMove(compilerServiceExtensionHandle);
    }
#endif

#if PLATFORM(IOS_FAMILY)
    if (!WebCore::IOSApplication::isMobileSafari())
        parameters.dynamicMachExtensionHandles = SandboxExtension::createHandlesForMachLookup(nonBrowserServices(), WTF::nullopt);
    
    if (isInternalInstall())
        parameters.diagnosticsExtensionHandles = SandboxExtension::createHandlesForMachLookup(diagnosticServices(), WTF::nullopt, SandboxExtension::Flags::NoReport);

    if (WebCore::deviceHasAGXCompilerService())
        parameters.dynamicIOKitExtensionHandles = SandboxExtension::createHandlesForIOKitClassExtensions(agxCompilerClasses(), WTF::nullopt);
#endif
    
#if PLATFORM(COCOA)
    if ([getNEFilterSourceClass() filterRequired]) {
        SandboxExtension::Handle helperHandle;
        SandboxExtension::createHandleForMachLookup("com.apple.nehelper", WTF::nullopt, helperHandle);
        parameters.neHelperExtensionHandle = WTFMove(helperHandle);
        SandboxExtension::Handle managerHandle;
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101500
        SandboxExtension::createHandleForMachLookup("com.apple.nesessionmanager", WTF::nullopt, managerHandle);
#else
        SandboxExtension::createHandleForMachLookup("com.apple.nesessionmanager.content-filter", WTF::nullopt, managerHandle);
#endif
        parameters.neSessionManagerExtensionHandle = WTFMove(managerHandle);
    }
    parameters.systemHasBattery = systemHasBattery();

    SandboxExtension::Handle mapDBHandle;
    if (SandboxExtension::createHandleForMachLookup("com.apple.lsd.mapdb", WTF::nullopt, mapDBHandle, SandboxExtension::Flags::NoReport))
        parameters.mapDBExtensionHandle = WTFMove(mapDBHandle);
#endif
    
#if PLATFORM(IOS)
    if ([getWebFilterEvaluatorClass() isManagedSession]) {
        SandboxExtension::Handle handle;
        SandboxExtension::createHandleForMachLookup("com.apple.uikit.viewservice.com.apple.WebContentFilter.remoteUI", WTF::nullopt, handle);
        parameters.contentFilterExtensionHandle = WTFMove(handle);

        SandboxExtension::Handle frontboardServiceExtensionHandle;
        if (SandboxExtension::createHandleForMachLookup("com.apple.frontboard.systemappservices", WTF::nullopt, frontboardServiceExtensionHandle))
            parameters.frontboardServiceExtensionHandle = WTFMove(frontboardServiceExtensionHandle);
    }
#endif
    
#if PLATFORM(IOS_FAMILY)
    parameters.currentUserInterfaceIdiomIsPad = currentUserInterfaceIdiomIsPad();
    parameters.supportsPictureInPicture = supportsPictureInPicture();
    parameters.cssValueToSystemColorMap = RenderThemeIOS::cssValueToSystemColorMap();
    parameters.focusRingColor = RenderTheme::singleton().focusRingColor(OptionSet<StyleColor::Options>());
    parameters.localizedDeviceModel = localizedDeviceModel();
#endif
    
    // Allow microphone access if either preference is set because WebRTC requires microphone access.
    bool needWebProcessExtensions = !m_defaultPageGroup->preferences().useGPUProcessForMedia()
        || !m_defaultPageGroup->preferences().captureAudioInGPUProcessEnabled()
        || !m_defaultPageGroup->preferences().captureVideoInGPUProcessEnabled();

    if (needWebProcessExtensions) {
        // FIXME(207716): The following should be removed when the GPU process is complete.
        parameters.mediaExtensionHandles = SandboxExtension::createHandlesForMachLookup(mediaRelatedMachServices(), WTF::nullopt);
    }

#if ENABLE(CFPREFS_DIRECT_MODE) && PLATFORM(IOS_FAMILY)
    if (_AXSApplicationAccessibilityEnabled())
        parameters.preferencesExtensionHandles = SandboxExtension::createHandlesForMachLookup({ "com.apple.cfprefsd.agent"_s, "com.apple.cfprefsd.daemon"_s }, WTF::nullopt);
#endif

#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    if (!_MGCacheValid())
        [adoptNS([[objc_getClass("MobileGestaltHelperProxy") alloc] init]) proxyRebuildCache];
#endif

#if PLATFORM(IOS_FAMILY) && ENABLE(CFPREFS_DIRECT_MODE)
    if ([UIApplication sharedApplication]) {
        auto state = [[UIApplication sharedApplication] applicationState];
        if (state == UIApplicationStateActive)
            startObservingPreferenceChanges();
    }
#endif
}

void WebProcessPool::platformInitializeNetworkProcess(NetworkProcessCreationParameters& parameters)
{
    parameters.uiProcessBundleIdentifier = applicationBundleIdentifier();
    parameters.uiProcessSDKVersion = dyld_get_program_sdk_version();

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    {
        bool isSafari = false;
        bool isMiniBrowser = false;
#if PLATFORM(IOS_FAMILY)
        isSafari = WebCore::IOSApplication::isMobileSafari();
        isMiniBrowser = WebCore::IOSApplication::isMiniBrowser();
#elif PLATFORM(MAC)
        isSafari = WebCore::MacApplication::isSafari();
        isMiniBrowser = WebCore::MacApplication::isMiniBrowser();
#endif
        if (isSafari || isMiniBrowser) {
            parameters.defaultDataStoreParameters.networkSessionParameters.httpProxy = URL(URL(), [defaults stringForKey:(NSString *)WebKit2HTTPProxyDefaultsKey]);
            parameters.defaultDataStoreParameters.networkSessionParameters.httpsProxy = URL(URL(), [defaults stringForKey:(NSString *)WebKit2HTTPSProxyDefaultsKey]);
        }
    }

    parameters.networkATSContext = adoptCF(_CFNetworkCopyATSContext());

    parameters.shouldSuppressMemoryPressureHandler = [defaults boolForKey:WebKitSuppressMemoryPressureHandlerDefaultsKey];

#if PLATFORM(MAC) || PLATFORM(MACCATALYST)
    ASSERT(parameters.uiProcessCookieStorageIdentifier.isEmpty());
    ASSERT(hasProcessPrivilege(ProcessPrivilege::CanAccessRawCookies));
    parameters.uiProcessCookieStorageIdentifier = identifyingDataFromCookieStorage([[NSHTTPCookieStorage sharedHTTPCookieStorage] _cookieStorage]);
#endif

    parameters.storageAccessAPIEnabled = storageAccessAPIEnabled();

    NSNumber *databaseEnabledValue = [defaults objectForKey:[NSString stringWithFormat:@"InternalDebug%@", WebPreferencesKey::isITPDatabaseEnabledKey().createCFString().get()]];
    if (databaseEnabledValue)
        parameters.shouldEnableITPDatabase = databaseEnabledValue.boolValue;
    else
        parameters.shouldEnableITPDatabase = m_defaultPageGroup->preferences().isITPDatabaseEnabled();

    parameters.enableAdClickAttributionDebugMode = [defaults boolForKey:[NSString stringWithFormat:@"Experimental%@", WebPreferencesKey::adClickAttributionDebugModeEnabledKey().createCFString().get()]];
}

void WebProcessPool::platformInvalidateContext()
{
    unregisterNotificationObservers();
}

#if PLATFORM(IOS_FAMILY)
String WebProcessPool::parentBundleDirectory()
{
    return [[[NSBundle mainBundle] bundlePath] stringByStandardizingPath];
}

String WebProcessPool::networkingCachesDirectory()
{
    String path = pathForProcessContainer();
    if (path.isEmpty())
        path = NSHomeDirectory();

    path = path + "/Library/Caches/com.apple.WebKit.Networking/";
    path = stringByResolvingSymlinksInPath(path);

    NSError *error = nil;
    NSString* nsPath = path;
    if (![[NSFileManager defaultManager] createDirectoryAtPath:nsPath withIntermediateDirectories:YES attributes:nil error:&error]) {
        NSLog(@"could not create networking caches directory \"%@\", error %@", nsPath, error);
        return String();
    }

    return path;
}

String WebProcessPool::webContentCachesDirectory()
{
    String path = pathForProcessContainer();
    if (path.isEmpty())
        path = NSHomeDirectory();

    path = path + "/Library/Caches/com.apple.WebKit.WebContent/";
    path = stringByResolvingSymlinksInPath(path);

    NSError *error = nil;
    NSString* nsPath = path;
    if (![[NSFileManager defaultManager] createDirectoryAtPath:nsPath withIntermediateDirectories:YES attributes:nil error:&error]) {
        NSLog(@"could not create web content caches directory \"%@\", error %@", nsPath, error);
        return String();
    }

    return path;
}

String WebProcessPool::containerTemporaryDirectory()
{
    String path = NSTemporaryDirectory();
    return stringByResolvingSymlinksInPath(path);
}
#endif

#if PLATFORM(IOS_FAMILY)
void WebProcessPool::setJavaScriptConfigurationFileEnabledFromDefaults()
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    setJavaScriptConfigurationFileEnabled([defaults boolForKey:@"WebKitJavaScriptCoreUseConfigFile"]);
}
#endif

bool WebProcessPool::omitPDFSupport()
{
    // Since this is a "secret default" we don't bother registering it.
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"WebKitOmitPDFSupport"];
}

bool WebProcessPool::processSuppressionEnabled() const
{
    return !m_userObservablePageCounter.value() && !m_processSuppressionDisabledForPageCounter.value();
}

bool WebProcessPool::networkProcessHasEntitlementForTesting(const String& entitlement)
{
    return WTF::hasEntitlement(ensureNetworkProcess().connection()->xpcConnection(), entitlement.utf8().data());
}

#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
float WebProcessPool::displayBrightness()
{
    return BKSDisplayBrightnessGetCurrent();
}
    
void WebProcessPool::backlightLevelDidChangeCallback(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo)
{
    auto* pool = reinterpret_cast<WebProcessPool*>(observer);
    pool->sendToAllProcesses(Messages::WebProcess::BacklightLevelDidChange(BKSDisplayBrightnessGetCurrent()));
}
#endif

#if ENABLE(REMOTE_INSPECTOR) && PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
void WebProcessPool::remoteWebInspectorEnabledCallback(CFNotificationCenterRef, void *observer, CFStringRef name, const void *, CFDictionaryRef userInfo)
{
    auto* pool = reinterpret_cast<WebProcessPool*>(observer);
    for (size_t i = 0; i < pool->m_processes.size(); ++i) {
        auto process = pool->m_processes[i];
        process->enableRemoteInspectorIfNeeded();
    }
}
#endif

#if ENABLE(CFPREFS_DIRECT_MODE)
void WebProcessPool::startObservingPreferenceChanges()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            // Start observing preference changes.
            [WKPreferenceObserver sharedInstance];
        });
    });
}
#endif

void WebProcessPool::registerNotificationObservers()
{
#if !PLATFORM(IOS_FAMILY)
    // Listen for enhanced accessibility changes and propagate them to the WebProcess.
    m_enhancedAccessibilityObserver = [[NSNotificationCenter defaultCenter] addObserverForName:WebKitApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *note) {
#if ENABLE(CFPREFS_DIRECT_MODE)
        for (auto& process : m_processes)
            process->unblockPreferenceServiceIfNeeded();
#endif
        setEnhancedAccessibility([[[note userInfo] objectForKey:@"AXEnhancedUserInterface"] boolValue]);
    }];

    m_automaticTextReplacementNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticTextReplacementNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticTextReplacementEnabled();
        textCheckerStateChanged();
    }];
    
    m_automaticSpellingCorrectionNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticSpellingCorrectionNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticSpellingCorrectionEnabled();
        textCheckerStateChanged();
    }];

    m_automaticQuoteSubstitutionNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticQuoteSubstitutionNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticQuoteSubstitutionEnabled();
        textCheckerStateChanged();
    }];

    m_automaticDashSubstitutionNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticDashSubstitutionNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticDashSubstitutionEnabled();
        textCheckerStateChanged();
    }];

    m_accessibilityDisplayOptionsNotificationObserver = [[NSWorkspace.sharedWorkspace notificationCenter] addObserverForName:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        screenPropertiesStateChanged();
    }];

#if ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    m_scrollerStyleNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSPreferredScrollerStyleDidChangeNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        auto scrollbarStyle = [NSScroller preferredScrollerStyle];
        sendToAllProcesses(Messages::WebProcess::ScrollerStylePreferenceChanged(scrollbarStyle));
    }];
#endif

    m_activationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidBecomeActiveNotification object:NSApp queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
#if ENABLE(CFPREFS_DIRECT_MODE)
        startObservingPreferenceChanges();
#endif
        setApplicationIsActive(true);
    }];

    m_deactivationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidResignActiveNotification object:NSApp queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        setApplicationIsActive(false);
    }];
#elif !PLATFORM(MACCATALYST)
    CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), this, backlightLevelDidChangeCallback, static_cast<CFStringRef>(UIBacklightLevelChangedNotification), nullptr, CFNotificationSuspensionBehaviorCoalesce);
#if PLATFORM(IOS)
#if ENABLE(REMOTE_INSPECTOR)
    CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), this, remoteWebInspectorEnabledCallback, static_cast<CFStringRef>(CFSTR(WIRServiceEnabledNotification)), nullptr, CFNotificationSuspensionBehaviorCoalesce);
#endif
#endif // PLATFORM(IOS)
#endif // !PLATFORM(IOS_FAMILY)

#if PLATFORM(IOS_FAMILY)
    m_accessibilityEnabledObserver = [[NSNotificationCenter defaultCenter] addObserverForName:(__bridge id)kAXSApplicationAccessibilityEnabledNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *) {
        for (size_t i = 0; i < m_processes.size(); ++i) {
#if ENABLE(CFPREFS_DIRECT_MODE)
            m_processes[i]->unblockPreferenceServiceIfNeeded();
#endif
            m_processes[i]->unblockAccessibilityServerIfNeeded();
        }
    }];
#if ENABLE(CFPREFS_DIRECT_MODE)
    m_activationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:@"UIApplicationDidBecomeActiveNotification" object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        startObservingPreferenceChanges();
    }];
#endif
#endif
}

void WebProcessPool::unregisterNotificationObservers()
{
#if !PLATFORM(IOS_FAMILY)
    [[NSNotificationCenter defaultCenter] removeObserver:m_enhancedAccessibilityObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticTextReplacementNotificationObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticSpellingCorrectionNotificationObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticQuoteSubstitutionNotificationObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticDashSubstitutionNotificationObserver.get()];
    [[NSWorkspace.sharedWorkspace notificationCenter] removeObserver:m_accessibilityDisplayOptionsNotificationObserver.get()];
#if ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    [[NSNotificationCenter defaultCenter] removeObserver:m_scrollerStyleNotificationObserver.get()];
#endif
    [[NSNotificationCenter defaultCenter] removeObserver:m_deactivationObserver.get()];
#elif !PLATFORM(MACCATALYST)
    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDarwinNotifyCenter(), this, static_cast<CFStringRef>(UIBacklightLevelChangedNotification) , nullptr);
#if PLATFORM(IOS)
#if ENABLE(REMOTE_INSPECTOR)
    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDarwinNotifyCenter(), this, CFSTR(WIRServiceEnabledNotification), nullptr);
#endif
#endif // PLATFORM(IOS)
#endif // !PLATFORM(IOS_FAMILY)

#if PLATFORM(IOS_FAMILY)
    [[NSNotificationCenter defaultCenter] removeObserver:m_accessibilityEnabledObserver.get()];
#endif

    [[NSNotificationCenter defaultCenter] removeObserver:m_activationObserver.get()];
}

static CFURLStorageSessionRef privateBrowsingSession()
{
    static CFURLStorageSessionRef session;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSString *identifier = [NSString stringWithFormat:@"%@.PrivateBrowsing", [[NSBundle mainBundle] bundleIdentifier]];
        session = createPrivateStorageSession((__bridge CFStringRef)identifier);
    });

    return session;
}

bool WebProcessPool::isURLKnownHSTSHost(const String& urlString) const
{
    RetainPtr<CFURLRef> url = URL(URL(), urlString).createCFURL();

    return _CFNetworkIsKnownHSTSHostWithSession(url.get(), nullptr);
}

void WebProcessPool::resetHSTSHosts()
{
    _CFNetworkResetHSTSHostsWithSession(nullptr);
    _CFNetworkResetHSTSHostsWithSession(privateBrowsingSession());
}

void WebProcessPool::resetHSTSHostsAddedAfterDate(double startDateIntervalSince1970)
{
    NSDate *startDate = [NSDate dateWithTimeIntervalSince1970:startDateIntervalSince1970];
    _CFNetworkResetHSTSHostsSinceDate(nullptr, (__bridge CFDateRef)startDate);
    _CFNetworkResetHSTSHostsSinceDate(privateBrowsingSession(), (__bridge CFDateRef)startDate);
}

#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
Optional<unsigned> WebProcessPool::nominalFramesPerSecondForDisplay(WebCore::PlatformDisplayID displayID)
{
    for (auto& displayLink : m_displayLinks) {
        if (displayLink->displayID() == displayID)
            return displayLink->nominalFramesPerSecond();
    }

    // Note that this creates a DisplayLink with no observers, but it's highly likely that we'll soon call startDisplayLink() for it.
    auto displayLink = makeUnique<DisplayLink>(displayID);
    auto frameRate = displayLink->nominalFramesPerSecond();
    m_displayLinks.append(WTFMove(displayLink));
    return frameRate;
}

void WebProcessPool::startDisplayLink(IPC::Connection& connection, DisplayLinkObserverID observerID, PlatformDisplayID displayID)
{
    for (auto& displayLink : m_displayLinks) {
        if (displayLink->displayID() == displayID) {
            displayLink->addObserver(connection, observerID);
            return;
        }
    }
    auto displayLink = makeUnique<DisplayLink>(displayID);
    displayLink->addObserver(connection, observerID);
    m_displayLinks.append(WTFMove(displayLink));
}

void WebProcessPool::stopDisplayLink(IPC::Connection& connection, DisplayLinkObserverID observerID, PlatformDisplayID displayID)
{
    for (auto& displayLink : m_displayLinks) {
        if (displayLink->displayID() == displayID) {
            displayLink->removeObserver(connection, observerID);
            return;
        }
    }
}

void WebProcessPool::stopDisplayLinks(IPC::Connection& connection)
{
    for (auto& displayLink : m_displayLinks)
        displayLink->removeObservers(connection);
}
#endif

// FIXME: Deprecated. Left here until a final decision is made.
void WebProcessPool::setCookieStoragePartitioningEnabled(bool enabled)
{
    m_cookieStoragePartitioningEnabled = enabled;
}

void WebProcessPool::clearPermanentCredentialsForProtectionSpace(WebCore::ProtectionSpace&& protectionSpace)
{
    auto sharedStorage = [NSURLCredentialStorage sharedCredentialStorage];
    auto credentials = [sharedStorage credentialsForProtectionSpace:protectionSpace.nsSpace()];
    for (NSString* user in credentials) {
        auto credential = credentials[user];
        if (credential.persistence == NSURLCredentialPersistencePermanent)
            [sharedStorage removeCredential:credentials[user] forProtectionSpace:protectionSpace.nsSpace()];
    }
}

int networkProcessLatencyQOS()
{
    static const int qos = [[NSUserDefaults standardUserDefaults] integerForKey:@"WebKitNetworkProcessLatencyQOS"];
    return qos;
}

int networkProcessThroughputQOS()
{
    static const int qos = [[NSUserDefaults standardUserDefaults] integerForKey:@"WebKitNetworkProcessThroughputQOS"];
    return qos;
}

int webProcessLatencyQOS()
{
    static const int qos = [[NSUserDefaults standardUserDefaults] integerForKey:@"WebKitWebProcessLatencyQOS"];
    return qos;
}

int webProcessThroughputQOS()
{
    static const int qos = [[NSUserDefaults standardUserDefaults] integerForKey:@"WebKitWebProcessThroughputQOS"];
    return qos;
}

#if PLATFORM(IOS_FAMILY)
void WebProcessPool::applicationIsAboutToSuspend()
{
    WEBPROCESSPOOL_RELEASE_LOG(ProcessSuspension, "applicationIsAboutToSuspend: Terminating non-critical processes");

    m_backForwardCache->pruneToSize(1);
    m_webProcessCache->clear();
}

void WebProcessPool::notifyProcessPoolsApplicationIsAboutToSuspend()
{
    for (auto* processPool : allProcessPools())
        processPool->applicationIsAboutToSuspend();
}
#endif

void WebProcessPool::initializeClassesForParameterCoding()
{
    const auto& customClasses = m_configuration->customClassesForParameterCoder();
    if (customClasses.isEmpty())
        return;

    auto standardClasses = [NSSet setWithObjects:[NSArray class], [NSData class], [NSDate class], [NSDictionary class], [NSNull class],
        [NSNumber class], [NSSet class], [NSString class], [NSTimeZone class], [NSURL class], [NSUUID class], nil];
    
    auto mutableSet = adoptNS([standardClasses mutableCopy]);

    for (const auto& customClass : customClasses) {
        auto className = customClass.utf8();
        Class objectClass = objc_lookUpClass(className.data());
        if (!objectClass) {
            WTFLogAlways("InjectedBundle::extendClassesForParameterCoder - Class %s is not a valid Objective C class.\n", className.data());
            break;
        }

        [mutableSet.get() addObject:objectClass];
    }

    m_classesForParameterCoder = mutableSet;
}

NSSet *WebProcessPool::allowedClassesForParameterCoding() const
{
    return m_classesForParameterCoder.get();
}

#if ENABLE(CFPREFS_DIRECT_MODE)
void WebProcessPool::notifyPreferencesChanged(const String& domain, const String& key, const Optional<String>& encodedValue)
{
    for (auto process : m_processes)
        process->send(Messages::WebProcess::NotifyPreferencesChanged(domain, key, encodedValue), 0);
}
#endif

#if PLATFORM(MAC)
static void webProcessPoolHighDynamicRangeDidChangeCallback(CMNotificationCenterRef, const void*, CFStringRef notificationName, const void*, CFTypeRef)
{
    auto screenProperties = WebCore::collectScreenProperties();
    for (auto& processPool : WebProcessPool::allProcessPools())
        processPool->sendToAllProcesses(Messages::WebProcess::SetScreenProperties(screenProperties));
}

void WebProcessPool::registerHighDynamicRangeChangeCallback()
{
    static std::once_flag onceFlag;
    std::call_once(
        onceFlag,
        [] {
        if (!PAL::isMediaToolboxFrameworkAvailable()
            || !PAL::canLoad_MediaToolbox_MTShouldPlayHDRVideo()
            || !PAL::canLoad_MediaToolbox_MT_GetShouldPlayHDRVideoNotificationSingleton()
            || !PAL::canLoad_MediaToolbox_kMTSupportNotification_ShouldPlayHDRVideoChanged())
            return;

        auto center = PAL::softLink_CoreMedia_CMNotificationCenterGetDefaultLocalCenter();
        auto notification = PAL::get_MediaToolbox_kMTSupportNotification_ShouldPlayHDRVideoChanged();
        auto object = PAL::softLinkMediaToolboxMT_GetShouldPlayHDRVideoNotificationSingleton();

        // Note: CMNotificationCenterAddListener requires a non-null listener pointer. Just use the singleton
        // object itself as the "listener", since the notification method is a static global and doesn't need
        // the listener pointer at all.
        PAL::softLink_CoreMedia_CMNotificationCenterAddListener(center, object, webProcessPoolHighDynamicRangeDidChangeCallback, notification, object, 0);
    });
}
#endif

} // namespace WebKit
