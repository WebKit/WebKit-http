/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
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
#include "PluginInfoStore.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "PluginModuleInfo.h"
#include <WebCore/URL.h>
#include <WebCore/MIMETypeRegistry.h>
#include <algorithm>
#include <wtf/ListHashSet.h>
#include <wtf/StdLibExtras.h>

using namespace WebCore;

namespace WebKit {

PluginInfoStore::PluginInfoStore()
    : m_pluginListIsUpToDate(false)
    , m_client(0)
{
}

void PluginInfoStore::setAdditionalPluginsDirectories(const Vector<String>& directories)
{
    m_additionalPluginsDirectories = directories;
    refresh();
}

void PluginInfoStore::refresh()
{
    m_pluginListIsUpToDate = false;
}

template <typename T, typename U>
static void addFromVector(T& hashSet, const U& vector)
{
    for (size_t i = 0; i < vector.size(); ++i)
        hashSet.add(vector[i]);
}

void PluginInfoStore::loadPluginsIfNecessary()
{
    if (m_pluginListIsUpToDate)
        return;

    ListHashSet<String, 32> uniquePluginPaths;

    // First, load plug-ins from the additional plug-ins directories specified.
    for (size_t i = 0; i < m_additionalPluginsDirectories.size(); ++i)
        addFromVector(uniquePluginPaths, pluginPathsInDirectory(m_additionalPluginsDirectories[i]));

    // Then load plug-ins from the standard plug-ins directories.
    Vector<String> directories = pluginsDirectories();
    for (size_t i = 0; i < directories.size(); ++i)
        addFromVector(uniquePluginPaths, pluginPathsInDirectory(directories[i]));

    // Then load plug-ins that are not in the standard plug-ins directories.
    addFromVector(uniquePluginPaths, individualPluginPaths());

    m_plugins.clear();

    for (const auto& pluginPath : uniquePluginPaths)
        loadPlugin(m_plugins, pluginPath);

    m_pluginListIsUpToDate = true;

    if (m_client)
        m_client->pluginInfoStoreDidLoadPlugins(this);
}

void PluginInfoStore::loadPlugin(Vector<PluginModuleInfo>& plugins, const String& pluginPath)
{
    PluginModuleInfo plugin;
    
    if (!getPluginInfo(pluginPath, plugin))
        return;

    if (!shouldUsePlugin(plugins, plugin))
        return;
    
    plugins.append(plugin);
}

Vector<PluginModuleInfo> PluginInfoStore::plugins()
{
    loadPluginsIfNecessary();
    return m_plugins;
}

PluginModuleInfo PluginInfoStore::findPluginForMIMEType(const String& mimeType, PluginData::AllowedPluginTypes allowedPluginTypes) const
{
    ASSERT(!mimeType.isNull());

    for (const auto& plugin : m_plugins) {
        if (allowedPluginTypes == PluginData::OnlyApplicationPlugins && !plugin.info.isApplicationPlugin)
            continue;

        for (const auto& mimeClassInfo : plugin.info.mimes) {
            if (mimeClassInfo.type == mimeType)
                return plugin;
        }
    }
    
    return PluginModuleInfo();
}

PluginModuleInfo PluginInfoStore::findPluginForExtension(const String& extension, String& mimeType, PluginData::AllowedPluginTypes allowedPluginTypes) const
{
    ASSERT(!extension.isNull());

    for (const auto& plugin : m_plugins) {
        if (allowedPluginTypes == PluginData::OnlyApplicationPlugins && !plugin.info.isApplicationPlugin)
            continue;

        for (const auto& mimeClassInfo : plugin.info.mimes) {
            const Vector<String>& extensions = mimeClassInfo.extensions;
            
            if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                // We found a supported extension, set the correct MIME type.
                mimeType = mimeClassInfo.type;
                return plugin;
            }
        }
    }
    
    return PluginModuleInfo();
}

static inline String pathExtension(const URL& url)
{
    String extension;
    String filename = url.lastPathComponent();
    if (!filename.endsWith('/')) {
        int extensionPos = filename.reverseFind('.');
        if (extensionPos != -1)
            extension = filename.substring(extensionPos + 1);
    }
    
    return extension;
}

#if !PLATFORM(COCOA)
PluginModuleLoadPolicy PluginInfoStore::defaultLoadPolicyForPlugin(const PluginModuleInfo&)
{
    return PluginModuleLoadNormally;
}
    
PluginModuleInfo PluginInfoStore::findPluginWithBundleIdentifier(const String&)
{
    ASSERT_NOT_REACHED();
    return PluginModuleInfo();
}

#endif

PluginModuleInfo PluginInfoStore::findPlugin(String& mimeType, const URL& url, PluginData::AllowedPluginTypes allowedPluginTypes)
{
    loadPluginsIfNecessary();
    
    // First, check if we can get the plug-in based on its MIME type.
    if (!mimeType.isNull()) {
        PluginModuleInfo plugin = findPluginForMIMEType(mimeType, allowedPluginTypes);
        if (!plugin.path.isNull())
            return plugin;
    }

    // Next, check if any plug-ins claim to support the URL extension.
    String extension = pathExtension(url).lower();
    if (!extension.isNull() && mimeType.isEmpty()) {
        PluginModuleInfo plugin = findPluginForExtension(extension, mimeType, allowedPluginTypes);
        if (!plugin.path.isNull())
            return plugin;
        
        // Finally, try to get the MIME type from the extension in a platform specific manner and use that.
        String extensionMimeType = MIMETypeRegistry::getMIMETypeForExtension(extension);
        if (!extensionMimeType.isNull()) {
            PluginModuleInfo plugin = findPluginForMIMEType(extensionMimeType, allowedPluginTypes);
            if (!plugin.path.isNull()) {
                mimeType = extensionMimeType;
                return plugin;
            }
        }
    }
    
    return PluginModuleInfo();
}

PluginModuleInfo PluginInfoStore::infoForPluginWithPath(const String& pluginPath) const
{
    for (const auto& plugin : m_plugins) {
        if (plugin.path == pluginPath)
            return plugin;
    }

    ASSERT_NOT_REACHED();
    return PluginModuleInfo();
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
