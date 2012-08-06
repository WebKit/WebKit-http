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
#include "PluginInfoStore.h"

#include "PluginModuleInfo.h"
#include <WebCore/KURL.h>
#include <WebCore/MIMETypeRegistry.h>
#include <algorithm>
#include <wtf/ListHashSet.h>
#include <wtf/StdLibExtras.h>

using namespace std;
using namespace WebCore;

namespace WebKit {

PluginInfoStore::PluginInfoStore()
    : m_pluginListIsUpToDate(false)
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

// We use a ListHashSet so that plugins will be loaded from the additional plugins directories first
// (which in turn means those plugins will be preferred if two plugins claim the same MIME type).
#if OS(WINDOWS)
typedef ListHashSet<String, 32, CaseFoldingHash> PathHashSet;
#else
typedef ListHashSet<String, 32> PathHashSet;
#endif

static inline Vector<PluginModuleInfo> deepIsolatedCopyPluginInfoVector(const Vector<PluginModuleInfo>& vector)
{
    // Let the copy begin!
    Vector<PluginModuleInfo> copy;
    copy.reserveCapacity(vector.size());
    for (unsigned i = 0; i < vector.size(); ++i)
        copy.append(vector[i].isolatedCopy());
    return copy;
}

void PluginInfoStore::loadPluginsIfNecessary()
{
    if (m_pluginListIsUpToDate)
        return;

    PathHashSet uniquePluginPaths;

    // First, load plug-ins from the additional plug-ins directories specified.
    for (size_t i = 0; i < m_additionalPluginsDirectories.size(); ++i)
        addFromVector(uniquePluginPaths, pluginPathsInDirectory(m_additionalPluginsDirectories[i]));

    // Then load plug-ins from the standard plug-ins directories.
    Vector<String> directories = pluginsDirectories();
    for (size_t i = 0; i < directories.size(); ++i)
        addFromVector(uniquePluginPaths, pluginPathsInDirectory(directories[i]));

    // Then load plug-ins that are not in the standard plug-ins directories.
    addFromVector(uniquePluginPaths, individualPluginPaths());

    Vector<PluginModuleInfo> plugins;

    PathHashSet::const_iterator end = uniquePluginPaths.end();
    for (PathHashSet::const_iterator it = uniquePluginPaths.begin(); it != end; ++it)
        loadPlugin(plugins, *it);

    m_plugins = deepIsolatedCopyPluginInfoVector(plugins);

    m_pluginListIsUpToDate = true;
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
    MutexLocker locker(m_pluginsLock);
    loadPluginsIfNecessary();
    return deepIsolatedCopyPluginInfoVector(m_plugins);
}

PluginModuleInfo PluginInfoStore::findPluginForMIMEType(const String& mimeType) const
{
    MutexLocker locker(m_pluginsLock);

    ASSERT(!mimeType.isNull());
    
    for (size_t i = 0; i < m_plugins.size(); ++i) {
        const PluginModuleInfo& plugin = m_plugins[i];
        
        for (size_t j = 0; j < plugin.info.mimes.size(); ++j) {
            const MimeClassInfo& mimeClassInfo = plugin.info.mimes[j];
            if (mimeClassInfo.type == mimeType)
                return plugin;
        }
    }
    
    return PluginModuleInfo();
}

PluginModuleInfo PluginInfoStore::findPluginForExtension(const String& extension, String& mimeType) const
{
    MutexLocker locker(m_pluginsLock);

    ASSERT(!extension.isNull());
    
    for (size_t i = 0; i < m_plugins.size(); ++i) {
        const PluginModuleInfo& plugin = m_plugins[i];
        
        for (size_t j = 0; j < plugin.info.mimes.size(); ++j) {
            const MimeClassInfo& mimeClassInfo = plugin.info.mimes[j];
            
            const Vector<String>& extensions = mimeClassInfo.extensions;
            
            if (find(extensions.begin(), extensions.end(), extension) != extensions.end()) {
                // We found a supported extension, set the correct MIME type.
                mimeType = mimeClassInfo.type;
                return plugin;
            }
        }
    }
    
    return PluginModuleInfo();
}

static inline String pathExtension(const KURL& url)
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

#if !PLATFORM(MAC)
bool PluginInfoStore::shouldBlockPlugin(const PluginModuleInfo&)
{
    return false;
}

String PluginInfoStore::getMIMETypeForExtension(const String& extension)
{
    return MIMETypeRegistry::getMIMETypeForExtension(extension);
}
#endif

PluginModuleInfo PluginInfoStore::findPlugin(String& mimeType, const KURL& url)
{
    {
        MutexLocker locker(m_pluginsLock);
        loadPluginsIfNecessary();
    }
    
    // First, check if we can get the plug-in based on its MIME type.
    if (!mimeType.isNull()) {
        PluginModuleInfo plugin = findPluginForMIMEType(mimeType);
        if (!plugin.path.isNull())
            return plugin;
    }

    // Next, check if any plug-ins claim to support the URL extension.
    String extension = pathExtension(url).lower();
    if (!extension.isNull() && mimeType.isEmpty()) {
        PluginModuleInfo plugin = findPluginForExtension(extension, mimeType);
        if (!plugin.path.isNull())
            return plugin;
        
        // Finally, try to get the MIME type from the extension in a platform specific manner and use that.
        String extensionMimeType = getMIMETypeForExtension(extension);
        if (!extensionMimeType.isNull()) {
            PluginModuleInfo plugin = findPluginForMIMEType(extensionMimeType);
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
    MutexLocker locker(m_pluginsLock);

    for (size_t i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i].path == pluginPath)
            return m_plugins[i];
    }
    
    ASSERT_NOT_REACHED();
    return PluginModuleInfo();
}

} // namespace WebKit
