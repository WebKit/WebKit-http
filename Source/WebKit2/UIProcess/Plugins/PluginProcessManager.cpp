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
#include "PluginProcessManager.h"

#if ENABLE(PLUGIN_PROCESS)

#include "PluginProcessProxy.h"
#include "WebContext.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

PluginProcessManager& PluginProcessManager::shared()
{
    DEFINE_STATIC_LOCAL(PluginProcessManager, pluginProcessManager, ());
    return pluginProcessManager;
}

PluginProcessManager::PluginProcessManager()
{
}

void PluginProcessManager::getPluginProcessConnection(const PluginInfoStore& pluginInfoStore, const String& pluginPath, PassRefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply> reply)
{
    ASSERT(!pluginPath.isNull());

    PluginModuleInfo plugin = pluginInfoStore.infoForPluginWithPath(pluginPath);
    PluginProcessProxy* pluginProcess = getOrCreatePluginProcess(plugin);
    pluginProcess->getPluginProcessConnection(reply);
}

void PluginProcessManager::removePluginProcessProxy(PluginProcessProxy* pluginProcessProxy)
{
    size_t vectorIndex = m_pluginProcesses.find(pluginProcessProxy);
    ASSERT(vectorIndex != notFound);

    m_pluginProcesses.remove(vectorIndex);
}

void PluginProcessManager::getSitesWithData(const PluginModuleInfo& plugin, WebPluginSiteDataManager* webPluginSiteDataManager, uint64_t callbackID)
{
    PluginProcessProxy* pluginProcess = getOrCreatePluginProcess(plugin);
    pluginProcess->getSitesWithData(webPluginSiteDataManager, callbackID);
}

void PluginProcessManager::clearSiteData(const PluginModuleInfo& plugin, WebPluginSiteDataManager* webPluginSiteDataManager, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID)
{
    PluginProcessProxy* pluginProcess = getOrCreatePluginProcess(plugin);
    pluginProcess->clearSiteData(webPluginSiteDataManager, sites, flags, maxAgeInSeconds, callbackID);
}

void PluginProcessManager::pluginSyncMessageSendTimedOut(const String& pluginPath)
{
    PluginProcessProxy* pluginProcess = pluginProcessWithPath(pluginPath);
    if (!pluginProcess)
        return;

    pluginProcess->terminate();
}

PluginProcessProxy* PluginProcessManager::pluginProcessWithPath(const String& pluginPath)
{
    for (size_t i = 0; i < m_pluginProcesses.size(); ++i) {
        if (m_pluginProcesses[i]->pluginInfo().path == pluginPath)
            return m_pluginProcesses[i];
    }
    return 0;
}

PluginProcessProxy* PluginProcessManager::getOrCreatePluginProcess(const PluginModuleInfo& plugin)
{
    if (PluginProcessProxy* pluginProcess = pluginProcessWithPath(plugin.path))
        return pluginProcess;

    PluginProcessProxy* pluginProcess = PluginProcessProxy::create(this, plugin).leakPtr();
    m_pluginProcesses.append(pluginProcess);

    return pluginProcess;
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
