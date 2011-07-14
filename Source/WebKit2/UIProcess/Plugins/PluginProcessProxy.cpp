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

#include "config.h"
#include "PluginProcessProxy.h"

#if ENABLE(PLUGIN_PROCESS)

#include "PluginProcessCreationParameters.h"
#include "PluginProcessManager.h"
#include "PluginProcessMessages.h"
#include "RunLoop.h"
#include "WebContext.h"
#include "WebCoreArgumentCoders.h"
#include "WebPluginSiteDataManager.h"
#include "WebProcessMessages.h"
#include "WebProcessProxy.h"
#include <WebCore/NotImplemented.h>

#if PLATFORM(MAC)
#include "MachPort.h"
#endif

namespace WebKit {

PassOwnPtr<PluginProcessProxy> PluginProcessProxy::create(PluginProcessManager* PluginProcessManager, const PluginModuleInfo& pluginInfo)
{
    return adoptPtr(new PluginProcessProxy(PluginProcessManager, pluginInfo));
}

PluginProcessProxy::PluginProcessProxy(PluginProcessManager* PluginProcessManager, const PluginModuleInfo& pluginInfo)
    : m_pluginProcessManager(PluginProcessManager)
    , m_pluginInfo(pluginInfo)
    , m_numPendingConnectionRequests(0)
#if PLATFORM(MAC)
    , m_modalWindowIsShowing(false)
    , m_fullscreenWindowIsShowing(false)
    , m_preFullscreenAppPresentationOptions(0)
#endif
{
    ProcessLauncher::LaunchOptions launchOptions;
    launchOptions.processType = ProcessLauncher::PluginProcess;
#if PLATFORM(MAC)
    launchOptions.architecture = pluginInfo.pluginArchitecture;
    launchOptions.executableHeap = PluginProcessProxy::pluginNeedsExecutableHeap(pluginInfo);
#endif

    m_processLauncher = ProcessLauncher::create(this, launchOptions);
}

PluginProcessProxy::~PluginProcessProxy()
{
}

// Asks the plug-in process to create a new connection to a web process. The connection identifier will be 
// encoded in the given argument encoder and sent back to the connection of the given web process.
void PluginProcessProxy::getPluginProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply> reply)
{
    m_pendingConnectionReplies.append(reply);

    if (m_processLauncher->isLaunching()) {
        m_numPendingConnectionRequests++;
        return;
    }
    
    // Ask the plug-in process to create a connection. Since the plug-in can be waiting for a synchronous reply
    // we need to make sure that this message is always processed, even when the plug-in is waiting for a synchronus reply.
    m_connection->send(Messages::PluginProcess::CreateWebProcessConnection(), 0, CoreIPC::DispatchMessageEvenWhenWaitingForSyncReply);
}

void PluginProcessProxy::getSitesWithData(WebPluginSiteDataManager* webPluginSiteDataManager, uint64_t callbackID)
{
    ASSERT(!m_pendingGetSitesReplies.contains(callbackID));
    m_pendingGetSitesReplies.set(callbackID, webPluginSiteDataManager);

    if (m_processLauncher->isLaunching()) {
        m_pendingGetSitesRequests.append(callbackID);
        return;
    }

    // Ask the plug-in process for the sites with data.
    m_connection->send(Messages::PluginProcess::GetSitesWithData(callbackID), 0);
}

void PluginProcessProxy::clearSiteData(WebPluginSiteDataManager* webPluginSiteDataManager, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID)
{
    ASSERT(!m_pendingClearSiteDataReplies.contains(callbackID));
    m_pendingClearSiteDataReplies.set(callbackID, webPluginSiteDataManager);

    if (m_processLauncher->isLaunching()) {
        ClearSiteDataRequest request;
        request.sites = sites;
        request.flags = flags;
        request.maxAgeInSeconds = maxAgeInSeconds;
        request.callbackID = callbackID;
        m_pendingClearSiteDataRequests.append(request);
        return;
    }

    // Ask the plug-in process to clear the site data.
    m_connection->send(Messages::PluginProcess::ClearSiteData(sites, flags, maxAgeInSeconds, callbackID), 0);
}

void PluginProcessProxy::terminate()
{
     m_processLauncher->terminateProcess();
}

void PluginProcessProxy::pluginProcessCrashedOrFailedToLaunch()
{
    // The plug-in process must have crashed or exited, send any pending sync replies we might have.
    while (!m_pendingConnectionReplies.isEmpty()) {
        RefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply> reply = m_pendingConnectionReplies.takeFirst();

#if PLATFORM(MAC)
        reply->send(CoreIPC::Attachment(0, MACH_MSG_TYPE_MOVE_SEND));
#elif USE(UNIX_DOMAIN_SOCKETS)
        reply->send(CoreIPC::Attachment());
#else
        notImplemented();
#endif
    }

    while (!m_pendingGetSitesReplies.isEmpty())
        didGetSitesWithData(Vector<String>(), m_pendingGetSitesReplies.begin()->first);

    while (!m_pendingClearSiteDataReplies.isEmpty())
        didClearSiteData(m_pendingClearSiteDataReplies.begin()->first);

    // Tell the plug-in process manager to forget about this plug-in process proxy.
    m_pluginProcessManager->removePluginProcessProxy(this);
    delete this;
}

void PluginProcessProxy::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceivePluginProcessProxyMessage(connection, messageID, arguments);
}

void PluginProcessProxy::didClose(CoreIPC::Connection*)
{
#if PLATFORM(MAC)
    if (m_modalWindowIsShowing)
        endModal();

    if (m_fullscreenWindowIsShowing)
        exitFullscreen();
#endif

    const Vector<WebContext*>& contexts = WebContext::allContexts();
    for (size_t i = 0; i < contexts.size(); ++i)
        contexts[i]->sendToAllProcesses(Messages::WebProcess::PluginProcessCrashed(m_pluginInfo.path));

    // This will cause us to be deleted.
    pluginProcessCrashedOrFailedToLaunch();
}

void PluginProcessProxy::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::MessageID)
{
}

void PluginProcessProxy::syncMessageSendTimedOut(CoreIPC::Connection*)
{
}

void PluginProcessProxy::didFinishLaunching(ProcessLauncher*, CoreIPC::Connection::Identifier connectionIdentifier)
{
    ASSERT(!m_connection);

    if (!connectionIdentifier) {
        pluginProcessCrashedOrFailedToLaunch();
        return;
    }
    
    m_connection = CoreIPC::Connection::createServerConnection(connectionIdentifier, this, RunLoop::main());
#if PLATFORM(MAC)
    m_connection->setShouldCloseConnectionOnMachExceptions();
#endif

    m_connection->open();
    
    PluginProcessCreationParameters parameters;

    parameters.pluginPath = m_pluginInfo.path;

    platformInitializePluginProcess(parameters);

    // Initialize the plug-in host process.
    m_connection->send(Messages::PluginProcess::InitializePluginProcess(parameters), 0);

    // Send all our pending requests.
    for (size_t i = 0; i < m_pendingGetSitesRequests.size(); ++i)
        m_connection->send(Messages::PluginProcess::GetSitesWithData(m_pendingGetSitesRequests[i]), 0);
    m_pendingGetSitesRequests.clear();

    for (size_t i = 0; i < m_pendingClearSiteDataRequests.size(); ++i) {
        const ClearSiteDataRequest& request = m_pendingClearSiteDataRequests[i];
        m_connection->send(Messages::PluginProcess::ClearSiteData(request.sites, request.flags, request.maxAgeInSeconds, request.callbackID), 0);
    }
    m_pendingClearSiteDataRequests.clear();

    for (unsigned i = 0; i < m_numPendingConnectionRequests; ++i)
        m_connection->send(Messages::PluginProcess::CreateWebProcessConnection(), 0);
    
    m_numPendingConnectionRequests = 0;
}

void PluginProcessProxy::didCreateWebProcessConnection(const CoreIPC::Attachment& connectionIdentifier)
{
    ASSERT(!m_pendingConnectionReplies.isEmpty());

    // Grab the first pending connection reply.
    RefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply> reply = m_pendingConnectionReplies.takeFirst();

#if PLATFORM(MAC)
    reply->send(CoreIPC::Attachment(connectionIdentifier.port(), MACH_MSG_TYPE_MOVE_SEND));
#elif USE(UNIX_DOMAIN_SOCKETS)
    reply->send(connectionIdentifier);
#else
    notImplemented();
#endif
}

void PluginProcessProxy::didGetSitesWithData(const Vector<String>& sites, uint64_t callbackID)
{
    RefPtr<WebPluginSiteDataManager> webPluginSiteDataManager = m_pendingGetSitesReplies.take(callbackID);
    ASSERT(webPluginSiteDataManager);

    webPluginSiteDataManager->didGetSitesWithDataForSinglePlugin(sites, callbackID);
}

void PluginProcessProxy::didClearSiteData(uint64_t callbackID)
{
    RefPtr<WebPluginSiteDataManager> webPluginSiteDataManager = m_pendingClearSiteDataReplies.take(callbackID);
    ASSERT(webPluginSiteDataManager);
    
    webPluginSiteDataManager->didClearSiteDataForSinglePlugin(callbackID);
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
