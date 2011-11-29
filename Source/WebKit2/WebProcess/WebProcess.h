/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WebProcess_h
#define WebProcess_h

#include "CacheModel.h"
#include "ChildProcess.h"
#include "DrawingArea.h"
#include "ResourceCachesToClear.h"
#include "SandboxExtension.h"
#include "SharedMemory.h"
#include "TextCheckerState.h"
#include "VisitedLinkTable.h"
#include "WebGeolocationManager.h"
#include "WebIconDatabaseProxy.h"
#include "WebPageGroupProxy.h"
#include <WebCore/LinkHash.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>

#if PLATFORM(QT)
class QNetworkAccessManager;
#endif

#if ENABLE(NOTIFICATIONS)
#include "WebNotificationManager.h"
#endif

#if ENABLE(PLUGIN_PROCESS)
#include "PluginProcessConnectionManager.h"
#endif

namespace WebCore {
    class IntSize;
    class PageGroup;
    class ResourceRequest;
    class ResourceResponse;
}

namespace WebKit {

class InjectedBundle;
class WebFrame;
class WebPage;
struct WebPageCreationParameters;
struct WebPageGroupData;
struct WebPreferencesStore;
struct WebProcessCreationParameters;

class WebProcess : public ChildProcess, private CoreIPC::Connection::QueueClient {
public:
    static WebProcess& shared();

    void initialize(CoreIPC::Connection::Identifier, RunLoop*);

    CoreIPC::Connection* connection() const { return m_connection.get(); }
    RunLoop* runLoop() const { return m_runLoop; }

    WebPage* webPage(uint64_t pageID) const;
    void createWebPage(uint64_t pageID, const WebPageCreationParameters&);
    void removeWebPage(uint64_t pageID);
    WebPage* focusedWebPage() const;
    
    InjectedBundle* injectedBundle() const { return m_injectedBundle.get(); }

    bool isSeparateProcess() const;

#if PLATFORM(MAC)
    void initializeShim();

#if USE(ACCELERATED_COMPOSITING)
    mach_port_t compositingRenderServerPort() const { return m_compositingRenderServerPort; }
#endif
#endif
    
    void addVisitedLink(WebCore::LinkHash);
    bool isLinkVisited(WebCore::LinkHash) const;

    bool fullKeyboardAccessEnabled() const { return m_fullKeyboardAccessEnabled; }

    WebFrame* webFrame(uint64_t) const;
    void addWebFrame(uint64_t, WebFrame*);
    void removeWebFrame(uint64_t);

    WebPageGroupProxy* webPageGroup(uint64_t pageGroupID);
    WebPageGroupProxy* webPageGroup(const WebPageGroupData&);
#if PLATFORM(MAC)
    pid_t presenterApplicationPid() const { return m_presenterApplicationPid; }
#endif 
    
#if PLATFORM(QT)
    QNetworkAccessManager* networkAccessManager() { return m_networkAccessManager; }
#endif

    bool shouldUseCustomRepresentationForResponse(const WebCore::ResourceResponse&) const;

    // Text Checking
    const TextCheckerState& textCheckerState() const { return m_textCheckerState; }

    // Geolocation
    WebGeolocationManager& geolocationManager() { return m_geolocationManager; }
    
#if ENABLE(NOTIFICATIONS)
    WebNotificationManager& notificationManager() { return m_notificationManager; }
#endif

    void clearResourceCaches(ResourceCachesToClear = AllResourceCaches);
    
    const String& localStorageDirectory() const { return m_localStorageDirectory; }

#if ENABLE(PLUGIN_PROCESS)
    PluginProcessConnectionManager& pluginProcessConnectionManager() { return m_pluginProcessConnectionManager; }
    bool disablePluginProcessMessageTimeout() const { return m_disablePluginProcessMessageTimeout; }
#endif

private:
    WebProcess();

    void initializeWebProcess(const WebProcessCreationParameters&, CoreIPC::ArgumentDecoder*);
    void platformInitializeWebProcess(const WebProcessCreationParameters&, CoreIPC::ArgumentDecoder*);
    void platformTerminate();
    void setShouldTrackVisitedLinks(bool);
    void registerURLSchemeAsEmptyDocument(const String&);
    void registerURLSchemeAsSecure(const String&) const;
    void setDomainRelaxationForbiddenForURLScheme(const String&) const;
    void setDefaultRequestTimeoutInterval(double);
    void setAlwaysUsesComplexTextCodePath(bool);
    void setShouldUseFontSmoothing(bool);
    void languageChanged(const String&) const;
    void fullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled);
#if PLATFORM(WIN)
    void setShouldPaintNativeControls(bool);
#endif

    void setVisitedLinkTable(const SharedMemory::Handle&);
    void visitedLinkStateChanged(const Vector<WebCore::LinkHash>& linkHashes);
    void allVisitedLinkStateChanged();

    void setCacheModel(uint32_t);
    void platformSetCacheModel(CacheModel);
    static void calculateCacheSizes(CacheModel cacheModel, uint64_t memorySize, uint64_t diskFreeSize,
        unsigned& cacheTotalCapacity, unsigned& cacheMinDeadCapacity, unsigned& cacheMaxDeadCapacity, double& deadDecodedDataDeletionInterval,
        unsigned& pageCacheCapacity, unsigned long& urlCacheMemoryCapacity, unsigned long& urlCacheDiskCapacity);
    void platformClearResourceCaches(ResourceCachesToClear);
    void clearApplicationCache();

    void setEnhancedAccessibility(bool);
    
#if !ENABLE(PLUGIN_PROCESS)
    void getSitesWithPluginData(const Vector<String>& pluginPaths, uint64_t callbackID);
    void clearPluginSiteData(const Vector<String>& pluginPaths, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID);
#endif
    
#if ENABLE(PLUGIN_PROCESS)
    void pluginProcessCrashed(const String& pluginPath);
#endif

    void startMemorySampler(const SandboxExtension::Handle&, const String&, const double);
    void stopMemorySampler();

    void downloadRequest(uint64_t downloadID, uint64_t initiatingPageID, const WebCore::ResourceRequest&);
    void cancelDownload(uint64_t downloadID);
#if PLATFORM(QT)
    void startTransfer(uint64_t downloadID, const String& destination);
#endif

    void setTextCheckerState(const TextCheckerState&);
    
    void getWebCoreStatistics(uint64_t callbackID);
    void garbageCollectJavaScriptObjects();

    // ChildProcess
    virtual bool shouldTerminate();
    virtual void terminate();

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*, OwnPtr<CoreIPC::ArgumentEncoder>&);
    virtual void didClose(CoreIPC::Connection*);
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::MessageID);
    virtual void syncMessageSendTimedOut(CoreIPC::Connection*);

    // CoreIPC::Connection::QueueClient
    virtual bool willProcessMessageOnClientRunLoop(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

#if PLATFORM(WIN)
    Vector<HWND> windowsToReceiveSentMessagesWhileWaitingForSyncReply();
#endif

    // Implemented in generated WebProcessMessageReceiver.cpp
    bool willProcessWebProcessMessageOnClientRunLoop(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    void didReceiveWebProcessMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    RefPtr<CoreIPC::Connection> m_connection;
    HashMap<uint64_t, RefPtr<WebPage> > m_pageMap;
    HashMap<uint64_t, RefPtr<WebPageGroupProxy> > m_pageGroupMap;
    RefPtr<InjectedBundle> m_injectedBundle;

    bool m_inDidClose;

    RunLoop* m_runLoop;

    // FIXME: The visited link table should not be per process.
    VisitedLinkTable m_visitedLinkTable;

    bool m_hasSetCacheModel;
    CacheModel m_cacheModel;

#if USE(ACCELERATED_COMPOSITING) && PLATFORM(MAC)
    mach_port_t m_compositingRenderServerPort;
#endif
#if PLATFORM(MAC)
    pid_t m_presenterApplicationPid;
#endif

    bool m_fullKeyboardAccessEnabled;

#if PLATFORM(QT)
    QNetworkAccessManager* m_networkAccessManager;
#endif

    HashMap<uint64_t, WebFrame*> m_frameMap;

    HashSet<String, CaseFoldingHash> m_mimeTypesWithCustomRepresentations;

    TextCheckerState m_textCheckerState;
    WebGeolocationManager m_geolocationManager;
#if ENABLE(NOTIFICATIONS)
    WebNotificationManager m_notificationManager;
#endif
    WebIconDatabaseProxy m_iconDatabaseProxy;
    
    String m_localStorageDirectory;

    RefPtr<SandboxExtension> m_applicationCachePathExtension;

#if ENABLE(PLUGIN_PROCESS)
    PluginProcessConnectionManager m_pluginProcessConnectionManager;
    bool m_disablePluginProcessMessageTimeout;
#endif

};

} // namespace WebKit

#endif // WebProcess_h
