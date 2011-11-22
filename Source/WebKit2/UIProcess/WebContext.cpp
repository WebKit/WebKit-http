/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebContext.h"

#include "DownloadProxy.h"
#include "ImmutableArray.h"
#include "InjectedBundleMessageKinds.h"
#include "Logging.h"
#include "MutableDictionary.h"
#include "RunLoop.h"
#include "SandboxExtension.h"
#include "StatisticsData.h"
#include "TextChecker.h"
#include "WKContextPrivate.h"
#include "WebApplicationCacheManagerProxy.h"
#include "WebContextMessageKinds.h"
#include "WebContextUserMessageCoders.h"
#include "WebCookieManagerProxy.h"
#include "WebCoreArgumentCoders.h"
#include "WebDatabaseManagerProxy.h"
#include "WebGeolocationManagerProxy.h"
#include "WebIconDatabase.h"
#include "WebKeyValueStorageManagerProxy.h"
#include "WebMediaCacheManagerProxy.h"
#include "WebPluginSiteDataManager.h"
#include "WebPageGroup.h"
#include "WebMemorySampler.h"
#include "WebProcessCreationParameters.h"
#include "WebProcessMessages.h"
#include "WebProcessProxy.h"
#include "WebResourceCacheManagerProxy.h"
#include <WebCore/Language.h>
#include <WebCore/LinkHash.h>
#include <WebCore/Logging.h>
#include <WebCore/ResourceRequest.h>
#include <runtime/InitializeThreading.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

#if PLATFORM(MAC)
#include "BuiltInPDFView.h"
#endif

#ifndef NDEBUG
#include <wtf/RefCountedLeakCounter.h>
#endif

#define MESSAGE_CHECK(assertion) MESSAGE_CHECK_BASE(assertion, m_process->connection())
#define MESSAGE_CHECK_URL(url) MESSAGE_CHECK_BASE(m_process->checkURLReceivedFromWebProcess(url), m_process->connection())

using namespace WebCore;

namespace WebKit {

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, webContextCounter, ("WebContext"));

WebContext* WebContext::sharedProcessContext()
{
    JSC::initializeThreading();
    WTF::initializeMainThread();
    RunLoop::initializeMainRunLoop();
    static WebContext* context = adoptRef(new WebContext(ProcessModelSharedSecondaryProcess, String())).leakRef();
    return context;
}

WebContext* WebContext::sharedThreadContext()
{
    RunLoop::initializeMainRunLoop();
    static WebContext* context = adoptRef(new WebContext(ProcessModelSharedSecondaryThread, String())).leakRef();
    return context;
}

PassRefPtr<WebContext> WebContext::create(const String& injectedBundlePath)
{
    JSC::initializeThreading();
    WTF::initializeMainThread();
    RunLoop::initializeMainRunLoop();
    return adoptRef(new WebContext(ProcessModelSecondaryProcess, injectedBundlePath));
}

static Vector<WebContext*>& contexts()
{
    DEFINE_STATIC_LOCAL(Vector<WebContext*>, contexts, ());

    return contexts;
}

const Vector<WebContext*>& WebContext::allContexts()
{
    return contexts();
}

WebContext::WebContext(ProcessModel processModel, const String& injectedBundlePath)
    : m_processModel(processModel)
    , m_defaultPageGroup(WebPageGroup::create())
    , m_injectedBundlePath(injectedBundlePath)
    , m_visitedLinkProvider(this)
    , m_alwaysUsesComplexTextCodePath(false)
    , m_shouldUseFontSmoothing(true)
    , m_cacheModel(CacheModelDocumentViewer)
    , m_memorySamplerEnabled(false)
    , m_memorySamplerInterval(1400.0)
    , m_applicationCacheManagerProxy(WebApplicationCacheManagerProxy::create(this))
    , m_cookieManagerProxy(WebCookieManagerProxy::create(this))
    , m_databaseManagerProxy(WebDatabaseManagerProxy::create(this))
    , m_geolocationManagerProxy(WebGeolocationManagerProxy::create(this))
    , m_iconDatabase(WebIconDatabase::create(this))
    , m_keyValueStorageManagerProxy(WebKeyValueStorageManagerProxy::create(this))
    , m_mediaCacheManagerProxy(WebMediaCacheManagerProxy::create(this))
    , m_pluginSiteDataManager(WebPluginSiteDataManager::create(this))
    , m_resourceCacheManagerProxy(WebResourceCacheManagerProxy::create(this))
#if PLATFORM(WIN)
    , m_shouldPaintNativeControls(true)
    , m_initialHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicyAlways)
#endif
    , m_processTerminationEnabled(true)
{
#if !LOG_DISABLED
    WebKit::initializeLogChannelsIfNecessary();
#endif

    contexts().append(this);

    addLanguageChangeObserver(this, languageChanged);

    WebCore::InitializeLoggingChannelsIfNecessary();

#ifndef NDEBUG
    webContextCounter.increment();
#endif
}

WebContext::~WebContext()
{
    ASSERT(contexts().find(this) != notFound);
    contexts().remove(contexts().find(this));

    removeLanguageChangeObserver(this);

    m_applicationCacheManagerProxy->invalidate();
    m_applicationCacheManagerProxy->clearContext();

    m_cookieManagerProxy->invalidate();
    m_cookieManagerProxy->clearContext();

    m_databaseManagerProxy->invalidate();
    m_databaseManagerProxy->clearContext();
    
    m_geolocationManagerProxy->invalidate();
    m_geolocationManagerProxy->clearContext();

    m_iconDatabase->invalidate();
    m_iconDatabase->clearContext();
    
    m_keyValueStorageManagerProxy->invalidate();
    m_keyValueStorageManagerProxy->clearContext();

    m_mediaCacheManagerProxy->invalidate();
    m_mediaCacheManagerProxy->clearContext();

    m_pluginSiteDataManager->invalidate();
    m_pluginSiteDataManager->clearContext();

    m_resourceCacheManagerProxy->invalidate();
    m_resourceCacheManagerProxy->clearContext();
    
    invalidateCallbackMap(m_dictionaryCallbacks);

    platformInvalidateContext();
    
#ifndef NDEBUG
    webContextCounter.decrement();
#endif
}

void WebContext::initializeInjectedBundleClient(const WKContextInjectedBundleClient* client)
{
    m_injectedBundleClient.initialize(client);
}

void WebContext::initializeConnectionClient(const WKContextConnectionClient* client)
{
    m_connectionClient.initialize(client);
}

void WebContext::initializeHistoryClient(const WKContextHistoryClient* client)
{
    m_historyClient.initialize(client);

    sendToAllProcesses(Messages::WebProcess::SetShouldTrackVisitedLinks(m_historyClient.shouldTrackVisitedLinks()));
}

void WebContext::initializeDownloadClient(const WKContextDownloadClient* client)
{
    m_downloadClient.initialize(client);
}
    
void WebContext::languageChanged(void* context)
{
    static_cast<WebContext*>(context)->languageChanged();
}

void WebContext::languageChanged()
{
    sendToAllProcesses(Messages::WebProcess::LanguageChanged(defaultLanguage()));
}

void WebContext::fullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled)
{
    sendToAllProcesses(Messages::WebProcess::FullKeyboardAccessModeChanged(fullKeyboardAccessEnabled));
}

void WebContext::ensureWebProcess()
{
    if (m_process)
        return;

    m_process = WebProcessProxy::create(this);

    WebProcessCreationParameters parameters;

    if (!injectedBundlePath().isEmpty()) {
        parameters.injectedBundlePath = injectedBundlePath();
        SandboxExtension::createHandle(parameters.injectedBundlePath, SandboxExtension::ReadOnly, parameters.injectedBundlePathExtensionHandle);
    }

    parameters.shouldTrackVisitedLinks = m_historyClient.shouldTrackVisitedLinks();
    parameters.cacheModel = m_cacheModel;
    parameters.languageCode = defaultLanguage();
    parameters.applicationCacheDirectory = applicationCacheDirectory();
    parameters.databaseDirectory = databaseDirectory();
    parameters.localStorageDirectory = localStorageDirectory();
    parameters.webInspectorLocalizedStringsPath = m_overrideWebInspectorLocalizedStringsPath;

#if PLATFORM(MAC)
    parameters.webInspectorBaseDirectory = m_overrideWebInspectorBaseDirectory;
    parameters.presenterApplicationPid = getpid();
#endif

    copyToVector(m_schemesToRegisterAsEmptyDocument, parameters.urlSchemesRegistererdAsEmptyDocument);
    copyToVector(m_schemesToRegisterAsSecure, parameters.urlSchemesRegisteredAsSecure);
    copyToVector(m_schemesToSetDomainRelaxationForbiddenFor, parameters.urlSchemesForWhichDomainRelaxationIsForbidden);

    parameters.shouldAlwaysUseComplexTextCodePath = m_alwaysUsesComplexTextCodePath;
    parameters.shouldUseFontSmoothing = m_shouldUseFontSmoothing;
    
    parameters.iconDatabaseEnabled = !iconDatabasePath().isEmpty();

    parameters.textCheckerState = TextChecker::state();

    parameters.fullKeyboardAccessEnabled = WebProcessProxy::fullKeyboardAccessEnabled();

    parameters.defaultRequestTimeoutInterval = WebURLRequest::defaultTimeoutInterval();

    // Add any platform specific parameters
    platformInitializeWebProcess(parameters);

    m_process->send(Messages::WebProcess::InitializeWebProcess(parameters, WebContextUserMessageEncoder(m_injectedBundleInitializationUserData.get())), 0);

    for (size_t i = 0; i != m_pendingMessagesToPostToInjectedBundle.size(); ++i) {
        pair<String, RefPtr<APIObject> >& message = m_pendingMessagesToPostToInjectedBundle[i];
        m_process->deprecatedSend(InjectedBundleMessage::PostMessage, 0, CoreIPC::In(message.first, WebContextUserMessageEncoder(message.second.get())));
    }
    m_pendingMessagesToPostToInjectedBundle.clear();
}

void WebContext::warmInitialProcess()  
{
    ensureWebProcess();
}

void WebContext::enableProcessTermination()
{
    m_processTerminationEnabled = true;
    if (shouldTerminate(m_process.get()))
        m_process->terminate();
}

bool WebContext::shouldTerminate(WebProcessProxy* process)
{
    // FIXME: Once we support multiple processes per context, this assertion won't hold.
    ASSERT(process == m_process);

    if (!m_processTerminationEnabled)
        return false;

    if (!m_downloads.isEmpty())
        return false;

    if (!m_applicationCacheManagerProxy->shouldTerminate(process))
        return false;
    if (!m_cookieManagerProxy->shouldTerminate(process))
        return false;
    if (!m_databaseManagerProxy->shouldTerminate(process))
        return false;
    if (!m_keyValueStorageManagerProxy->shouldTerminate(process))
        return false;
    if (!m_mediaCacheManagerProxy->shouldTerminate(process))
        return false;
    if (!m_pluginSiteDataManager->shouldTerminate(process))
        return false;
    if (!m_resourceCacheManagerProxy->shouldTerminate(process))
        return false;

    return true;
}

void WebContext::processDidFinishLaunching(WebProcessProxy* process)
{
    // FIXME: Once we support multiple processes per context, this assertion won't hold.
    ASSERT_UNUSED(process, process == m_process);

    m_visitedLinkProvider.processDidFinishLaunching();
    
    // Sometimes the memorySampler gets initialized after process initialization has happened but before the process has finished launching
    // so check if it needs to be started here
    if (m_memorySamplerEnabled) {
        SandboxExtension::Handle sampleLogSandboxHandle;        
        double now = WTF::currentTime();
        String sampleLogFilePath = String::format("WebProcess%llu", static_cast<uint64_t>(now));
        sampleLogFilePath = SandboxExtension::createHandleForTemporaryFile(sampleLogFilePath, SandboxExtension::WriteOnly, sampleLogSandboxHandle);
        
        m_process->send(Messages::WebProcess::StartMemorySampler(sampleLogSandboxHandle, sampleLogFilePath, m_memorySamplerInterval), 0);
    }
}

void WebContext::disconnectProcess(WebProcessProxy* process)
{
    // FIXME: Once we support multiple processes per context, this assertion won't hold.
    ASSERT_UNUSED(process, process == m_process);

    m_visitedLinkProvider.processDidClose();

    // Invalidate all outstanding downloads.
    for (HashMap<uint64_t, RefPtr<DownloadProxy> >::iterator::Values it = m_downloads.begin().values(), end = m_downloads.end().values(); it != end; ++it) {
        (*it)->processDidClose();
        (*it)->invalidate();
    }

    m_downloads.clear();

    m_applicationCacheManagerProxy->invalidate();
    m_cookieManagerProxy->invalidate();
    m_databaseManagerProxy->invalidate();
    m_geolocationManagerProxy->invalidate();
    m_keyValueStorageManagerProxy->invalidate();
    m_mediaCacheManagerProxy->invalidate();
    m_resourceCacheManagerProxy->invalidate();

    // When out of process plug-ins are enabled, we don't want to invalidate the plug-in site data
    // manager just because the web process crashes since it's not involved.
#if !ENABLE(PLUGIN_PROCESS)
    m_pluginSiteDataManager->invalidate();
#endif

    // This can cause the web context to be destroyed.
    m_process = 0;
}

PassRefPtr<WebPageProxy> WebContext::createWebPage(PageClient* pageClient, WebPageGroup* pageGroup)
{
    ensureWebProcess();

    if (!pageGroup)
        pageGroup = m_defaultPageGroup.get();

    return m_process->createWebPage(pageClient, this, pageGroup);
}

WebProcessProxy* WebContext::relaunchProcessIfNecessary()
{
    ensureWebProcess();

    ASSERT(m_process);
    return m_process.get();
}

DownloadProxy* WebContext::download(WebPageProxy* initiatingPage, const ResourceRequest& request)
{
    ensureWebProcess();

    DownloadProxy* download = createDownloadProxy();
    uint64_t initiatingPageID = initiatingPage ? initiatingPage->pageID() : 0;

#if PLATFORM(QT)
    ASSERT(initiatingPage); // Our design does not suppport downloads without a WebPage.
    initiatingPage->handleDownloadRequest(download);
#endif

    process()->send(Messages::WebProcess::DownloadRequest(download->downloadID(), initiatingPageID, request), 0);
    return download;
}

void WebContext::postMessageToInjectedBundle(const String& messageName, APIObject* messageBody)
{
    if (!m_process || !m_process->canSendMessage()) {
        m_pendingMessagesToPostToInjectedBundle.append(make_pair(messageName, messageBody));
        return;
    }

    // FIXME: We should consider returning false from this function if the messageBody cannot
    // be encoded.
    m_process->deprecatedSend(InjectedBundleMessage::PostMessage, 0, CoreIPC::In(messageName, WebContextUserMessageEncoder(messageBody)));
}

// InjectedBundle client

void WebContext::didReceiveMessageFromInjectedBundle(const String& messageName, APIObject* messageBody)
{
    m_injectedBundleClient.didReceiveMessageFromInjectedBundle(this, messageName, messageBody);
}

void WebContext::didReceiveSynchronousMessageFromInjectedBundle(const String& messageName, APIObject* messageBody, RefPtr<APIObject>& returnData)
{
    m_injectedBundleClient.didReceiveSynchronousMessageFromInjectedBundle(this, messageName, messageBody, returnData);
}

// HistoryClient

void WebContext::didNavigateWithNavigationData(uint64_t pageID, const WebNavigationDataStore& store, uint64_t frameID) 
{
    WebPageProxy* page = m_process->webPage(pageID);
    if (!page)
        return;
    
    WebFrameProxy* frame = m_process->webFrame(frameID);
    MESSAGE_CHECK(frame);
    MESSAGE_CHECK(frame->page() == page);
    
    m_historyClient.didNavigateWithNavigationData(this, page, store, frame);
}

void WebContext::didPerformClientRedirect(uint64_t pageID, const String& sourceURLString, const String& destinationURLString, uint64_t frameID)
{
    WebPageProxy* page = m_process->webPage(pageID);
    if (!page)
        return;

    if (sourceURLString.isEmpty() || destinationURLString.isEmpty())
        return;
    
    WebFrameProxy* frame = m_process->webFrame(frameID);
    MESSAGE_CHECK(frame);
    MESSAGE_CHECK(frame->page() == page);
    MESSAGE_CHECK_URL(sourceURLString);
    MESSAGE_CHECK_URL(destinationURLString);

    m_historyClient.didPerformClientRedirect(this, page, sourceURLString, destinationURLString, frame);
}

void WebContext::didPerformServerRedirect(uint64_t pageID, const String& sourceURLString, const String& destinationURLString, uint64_t frameID)
{
    WebPageProxy* page = m_process->webPage(pageID);
    if (!page)
        return;
    
    if (sourceURLString.isEmpty() || destinationURLString.isEmpty())
        return;
    
    WebFrameProxy* frame = m_process->webFrame(frameID);
    MESSAGE_CHECK(frame);
    MESSAGE_CHECK(frame->page() == page);
    MESSAGE_CHECK_URL(sourceURLString);
    MESSAGE_CHECK_URL(destinationURLString);

    m_historyClient.didPerformServerRedirect(this, page, sourceURLString, destinationURLString, frame);
}

void WebContext::didUpdateHistoryTitle(uint64_t pageID, const String& title, const String& url, uint64_t frameID)
{
    WebPageProxy* page = m_process->webPage(pageID);
    if (!page)
        return;

    WebFrameProxy* frame = m_process->webFrame(frameID);
    MESSAGE_CHECK(frame);
    MESSAGE_CHECK(frame->page() == page);
    MESSAGE_CHECK_URL(url);

    m_historyClient.didUpdateHistoryTitle(this, page, title, url, frame);
}

void WebContext::populateVisitedLinks()
{
    m_historyClient.populateVisitedLinks(this);
}

WebContext::Statistics& WebContext::statistics()
{
    static Statistics statistics = Statistics();

    return statistics;
}

void WebContext::setAdditionalPluginsDirectory(const String& directory)
{
    Vector<String> directories;
    directories.append(directory);

    m_pluginInfoStore.setAdditionalPluginsDirectories(directories);
}

void WebContext::setAlwaysUsesComplexTextCodePath(bool alwaysUseComplexText)
{
    m_alwaysUsesComplexTextCodePath = alwaysUseComplexText;
    sendToAllProcesses(Messages::WebProcess::SetAlwaysUsesComplexTextCodePath(alwaysUseComplexText));
}

void WebContext::setShouldUseFontSmoothing(bool useFontSmoothing)
{
    m_shouldUseFontSmoothing = useFontSmoothing;
    sendToAllProcesses(Messages::WebProcess::SetShouldUseFontSmoothing(useFontSmoothing));
}

void WebContext::registerURLSchemeAsEmptyDocument(const String& urlScheme)
{
    m_schemesToRegisterAsEmptyDocument.add(urlScheme);
    sendToAllProcesses(Messages::WebProcess::RegisterURLSchemeAsEmptyDocument(urlScheme));
}

void WebContext::registerURLSchemeAsSecure(const String& urlScheme)
{
    m_schemesToRegisterAsSecure.add(urlScheme);
    sendToAllProcesses(Messages::WebProcess::RegisterURLSchemeAsSecure(urlScheme));
}

void WebContext::setDomainRelaxationForbiddenForURLScheme(const String& urlScheme)
{
    m_schemesToSetDomainRelaxationForbiddenFor.add(urlScheme);
    sendToAllProcesses(Messages::WebProcess::SetDomainRelaxationForbiddenForURLScheme(urlScheme));
}

void WebContext::setCacheModel(CacheModel cacheModel)
{
    m_cacheModel = cacheModel;
    sendToAllProcesses(Messages::WebProcess::SetCacheModel(static_cast<uint32_t>(m_cacheModel)));
}

void WebContext::setDefaultRequestTimeoutInterval(double timeoutInterval)
{
    sendToAllProcesses(Messages::WebProcess::SetDefaultRequestTimeoutInterval(timeoutInterval));
}

void WebContext::addVisitedLink(const String& visitedURL)
{
    if (visitedURL.isEmpty())
        return;

    LinkHash linkHash = visitedLinkHash(visitedURL.characters(), visitedURL.length());
    addVisitedLinkHash(linkHash);
}

void WebContext::addVisitedLinkHash(LinkHash linkHash)
{
    m_visitedLinkProvider.addVisitedLink(linkHash);
}

void WebContext::getPlugins(bool refresh, Vector<PluginInfo>& pluginInfos)
{
    if (refresh)
        m_pluginInfoStore.refresh();

    Vector<PluginModuleInfo> plugins = m_pluginInfoStore.plugins();
    for (size_t i = 0; i < plugins.size(); ++i)
        pluginInfos.append(plugins[i].info);

#if PLATFORM(MAC)
    // Add built-in PDF last, so that it's not used when a real plug-in is installed.
    if (!omitPDFSupport())
        pluginInfos.append(BuiltInPDFView::pluginInfo());
#endif
}

void WebContext::getPluginPath(const String& mimeType, const String& urlString, String& pluginPath)
{
    MESSAGE_CHECK_URL(urlString);

    String newMimeType = mimeType.lower();

    PluginModuleInfo plugin = pluginInfoStore().findPlugin(newMimeType, KURL(KURL(), urlString));
    if (!plugin.path)
        return;

    pluginPath = plugin.path;
}

#if !ENABLE(PLUGIN_PROCESS)
void WebContext::didGetSitesWithPluginData(const Vector<String>& sites, uint64_t callbackID)
{
    m_pluginSiteDataManager->didGetSitesWithData(sites, callbackID);
}

void WebContext::didClearPluginSiteData(uint64_t callbackID)
{
    m_pluginSiteDataManager->didClearSiteData(callbackID);
}
#endif

DownloadProxy* WebContext::createDownloadProxy()
{
    RefPtr<DownloadProxy> downloadProxy = DownloadProxy::create(this);
    m_downloads.set(downloadProxy->downloadID(), downloadProxy);
    return downloadProxy.get();
}

void WebContext::downloadFinished(DownloadProxy* downloadProxy)
{
    ASSERT(m_downloads.contains(downloadProxy->downloadID()));

    downloadProxy->invalidate();
    m_downloads.remove(downloadProxy->downloadID());
}

// FIXME: This is not the ideal place for this function.
HashSet<String, CaseFoldingHash> WebContext::pdfAndPostScriptMIMETypes()
{
    HashSet<String, CaseFoldingHash> mimeTypes;

    mimeTypes.add("application/pdf");
    mimeTypes.add("application/postscript");
    mimeTypes.add("text/pdf");
    
    return mimeTypes;
}

void WebContext::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    if (messageID.is<CoreIPC::MessageClassWebContext>()) {
        didReceiveWebContextMessage(connection, messageID, arguments);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassDownloadProxy>()) {
        if (DownloadProxy* downloadProxy = m_downloads.get(arguments->destinationID()).get())
            downloadProxy->didReceiveDownloadProxyMessage(connection, messageID, arguments);
        
        return;
    }

    if (messageID.is<CoreIPC::MessageClassWebApplicationCacheManagerProxy>()) {
        m_applicationCacheManagerProxy->didReceiveMessage(connection, messageID, arguments);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassWebCookieManagerProxy>()) {
        m_cookieManagerProxy->didReceiveMessage(connection, messageID, arguments);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassWebDatabaseManagerProxy>()) {
        m_databaseManagerProxy->didReceiveWebDatabaseManagerProxyMessage(connection, messageID, arguments);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassWebGeolocationManagerProxy>()) {
        m_geolocationManagerProxy->didReceiveMessage(connection, messageID, arguments);
        return;
    }
    
    if (messageID.is<CoreIPC::MessageClassWebIconDatabase>()) {
        m_iconDatabase->didReceiveMessage(connection, messageID, arguments);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassWebKeyValueStorageManagerProxy>()) {
        m_keyValueStorageManagerProxy->didReceiveMessage(connection, messageID, arguments);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassWebMediaCacheManagerProxy>()) {
        m_mediaCacheManagerProxy->didReceiveMessage(connection, messageID, arguments);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassWebResourceCacheManagerProxy>()) {
        m_resourceCacheManagerProxy->didReceiveWebResourceCacheManagerProxyMessage(connection, messageID, arguments);
        return;
    }

    switch (messageID.get<WebContextLegacyMessage::Kind>()) {
        case WebContextLegacyMessage::PostMessage: {
            String messageName;
            RefPtr<APIObject> messageBody;
            WebContextUserMessageDecoder messageDecoder(messageBody, this);
            if (!arguments->decode(CoreIPC::Out(messageName, messageDecoder)))
                return;

            didReceiveMessageFromInjectedBundle(messageName, messageBody.get());
            return;
        }
        case WebContextLegacyMessage::PostSynchronousMessage:
            ASSERT_NOT_REACHED();
    }

    ASSERT_NOT_REACHED();
}

void WebContext::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments, OwnPtr<CoreIPC::ArgumentEncoder>& reply)
{
    if (messageID.is<CoreIPC::MessageClassWebContext>()) {
        didReceiveSyncWebContextMessage(connection, messageID, arguments, reply);
        return;
    }

    if (messageID.is<CoreIPC::MessageClassDownloadProxy>()) {
        if (DownloadProxy* downloadProxy = m_downloads.get(arguments->destinationID()).get())
            downloadProxy->didReceiveSyncDownloadProxyMessage(connection, messageID, arguments, reply);
        return;
    }
    
    if (messageID.is<CoreIPC::MessageClassWebIconDatabase>()) {
        m_iconDatabase->didReceiveSyncMessage(connection, messageID, arguments, reply);
        return;
    }
    
    switch (messageID.get<WebContextLegacyMessage::Kind>()) {
        case WebContextLegacyMessage::PostSynchronousMessage: {
            // FIXME: We should probably encode something in the case that the arguments do not decode correctly.

            String messageName;
            RefPtr<APIObject> messageBody;
            WebContextUserMessageDecoder messageDecoder(messageBody, this);
            if (!arguments->decode(CoreIPC::Out(messageName, messageDecoder)))
                return;

            RefPtr<APIObject> returnData;
            didReceiveSynchronousMessageFromInjectedBundle(messageName, messageBody.get(), returnData);
            reply->encode(CoreIPC::In(WebContextUserMessageEncoder(returnData.get())));
            return;
        }
        case WebContextLegacyMessage::PostMessage:
            ASSERT_NOT_REACHED();
    }
}

void WebContext::setEnhancedAccessibility(bool flag)
{
    sendToAllProcesses(Messages::WebProcess::SetEnhancedAccessibility(flag));
}
    
void WebContext::startMemorySampler(const double interval)
{    
    // For new WebProcesses we will also want to start the Memory Sampler
    m_memorySamplerEnabled = true;
    m_memorySamplerInterval = interval;
    
    // For UIProcess
#if ENABLE(MEMORY_SAMPLER)
    WebMemorySampler::shared()->start(interval);
#endif
    
    // For WebProcess
    SandboxExtension::Handle sampleLogSandboxHandle;    
    double now = WTF::currentTime();
    String sampleLogFilePath = String::format("WebProcess%llu", static_cast<uint64_t>(now));
    sampleLogFilePath = SandboxExtension::createHandleForTemporaryFile(sampleLogFilePath, SandboxExtension::WriteOnly, sampleLogSandboxHandle);
    
    sendToAllProcesses(Messages::WebProcess::StartMemorySampler(sampleLogSandboxHandle, sampleLogFilePath, interval));
}

void WebContext::stopMemorySampler()
{    
    // For WebProcess
    m_memorySamplerEnabled = false;
    
    // For UIProcess
#if ENABLE(MEMORY_SAMPLER)
    WebMemorySampler::shared()->stop();
#endif

    sendToAllProcesses(Messages::WebProcess::StopMemorySampler());
}

String WebContext::databaseDirectory() const
{
    if (!m_overrideDatabaseDirectory.isEmpty())
        return m_overrideDatabaseDirectory;

    return platformDefaultDatabaseDirectory();
}

void WebContext::setIconDatabasePath(const String& path)
{
    m_overrideIconDatabasePath = path;
    m_iconDatabase->setDatabasePath(path);
}

String WebContext::iconDatabasePath() const
{
    if (!m_overrideIconDatabasePath.isEmpty())
        return m_overrideIconDatabasePath;

    return platformDefaultIconDatabasePath();
}

String WebContext::localStorageDirectory() const
{
    if (!m_overrideLocalStorageDirectory.isEmpty())
        return m_overrideLocalStorageDirectory;

    return platformDefaultLocalStorageDirectory();
}

void WebContext::setHTTPPipeliningEnabled(bool enabled)
{
#if PLATFORM(MAC)
    ResourceRequest::setHTTPPipeliningEnabled(enabled);
#endif
}

bool WebContext::httpPipeliningEnabled() const
{
#if PLATFORM(MAC)
    return ResourceRequest::httpPipeliningEnabled();
#else
    return false;
#endif
}

void WebContext::getWebCoreStatistics(PassRefPtr<DictionaryCallback> prpCallback)
{
    RefPtr<DictionaryCallback> callback = prpCallback;
    
    uint64_t callbackID = callback->callbackID();
    m_dictionaryCallbacks.set(callbackID, callback.get());
    process()->send(Messages::WebProcess::GetWebCoreStatistics(callbackID), 0);
}

static PassRefPtr<MutableDictionary> createDictionaryFromHashMap(const HashMap<String, uint64_t>& map)
{
    RefPtr<MutableDictionary> result = MutableDictionary::create();
    HashMap<String, uint64_t>::const_iterator end = map.end();
    for (HashMap<String, uint64_t>::const_iterator it = map.begin(); it != end; ++it)
        result->set(it->first, RefPtr<WebUInt64>(WebUInt64::create(it->second)).get());
    
    return result;
}

void WebContext::didGetWebCoreStatistics(const StatisticsData& statisticsData, uint64_t callbackID)
{
    RefPtr<DictionaryCallback> callback = m_dictionaryCallbacks.take(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        return;
    }

    RefPtr<MutableDictionary> statistics = createDictionaryFromHashMap(statisticsData.statisticsNumbers);
    statistics->set("JavaScriptProtectedObjectTypeCounts", createDictionaryFromHashMap(statisticsData.javaScriptProtectedObjectTypeCounts).get());
    statistics->set("JavaScriptObjectTypeCounts", createDictionaryFromHashMap(statisticsData.javaScriptObjectTypeCounts).get());
    
    size_t cacheStatisticsCount = statisticsData.webCoreCacheStatistics.size();
    Vector<RefPtr<APIObject> > cacheStatisticsVector(cacheStatisticsCount);
    for (size_t i = 0; i < cacheStatisticsCount; ++i)
        cacheStatisticsVector[i] = createDictionaryFromHashMap(statisticsData.webCoreCacheStatistics[i]);
    statistics->set("WebCoreCacheStatistics", ImmutableArray::adopt(cacheStatisticsVector).get());
    
    callback->performCallbackWithReturnValue(statistics.get());
}
    
void WebContext::garbageCollectJavaScriptObjects()
{
    process()->send(Messages::WebProcess::GarbageCollectJavaScriptObjects(), 0);
}

} // namespace WebKit
