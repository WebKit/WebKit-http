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

#import "config.h"
#import "PluginInfoStore.h"

#import "NetscapePluginModule.h"
#import "WebKitSystemInterface.h"
#import <WebCore/WebCoreNSStringExtras.h>
#import <wtf/HashSet.h>
#import <wtf/RetainPtr.h>

using namespace WebCore;

namespace WebKit {

Vector<String> PluginInfoStore::pluginsDirectories()
{
    Vector<String> pluginsDirectories;

    pluginsDirectories.append([NSHomeDirectory() stringByAppendingPathComponent:@"Library/Internet Plug-Ins"]);
    pluginsDirectories.append("/Library/Internet Plug-Ins");
    
    return pluginsDirectories;
}

// FIXME: Once the UI process knows the difference between the main thread and the web thread we can drop this and just use
// String::createCFString.
static CFStringRef safeCreateCFString(const String& string)
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar*>(string.characters()), string.length());
}
    
Vector<String> PluginInfoStore::pluginPathsInDirectory(const String& directory)
{
    Vector<String> pluginPaths;

    RetainPtr<CFStringRef> directoryCFString(AdoptCF, safeCreateCFString(directory));
    
    NSArray *filenames = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:(NSString *)directoryCFString.get() error:nil];
    for (NSString *filename in filenames)
        pluginPaths.append([(NSString *)directoryCFString.get() stringByAppendingPathComponent:filename]);
    
    return pluginPaths;
}

Vector<String> PluginInfoStore::individualPluginPaths()
{
    return Vector<String>();
}

bool PluginInfoStore::getPluginInfo(const String& pluginPath, PluginModuleInfo& plugin)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    return NetscapePluginModule::getPluginInfo(pluginPath, plugin);
#else
    UNUSED_PARAM(pluginPath);
    UNUSED_PARAM(plugin);
    return false;
#endif
}

static size_t findPluginWithBundleIdentifier(const Vector<PluginModuleInfo>& plugins, const String& bundleIdentifier)
{
    for (size_t i = 0; i < plugins.size(); ++i) {
        if (plugins[i].bundleIdentifier == bundleIdentifier)
            return i;
    }

    return notFound;
}

// Returns true if the given plug-in should be loaded, false otherwise.
static bool checkForPreferredPlugin(Vector<PluginModuleInfo>& alreadyLoadedPlugins, const PluginModuleInfo& plugin, const String& oldPluginBundleIdentifier, const String& newPluginBundleIdentifier)
{
    if (plugin.bundleIdentifier == oldPluginBundleIdentifier) {
        // If we've already found the new plug-in, we don't want to load the old plug-in.
        if (findPluginWithBundleIdentifier(alreadyLoadedPlugins, newPluginBundleIdentifier) != notFound)
            return false;
    } else if (plugin.bundleIdentifier == newPluginBundleIdentifier) {
        // If we've already found the old plug-in, remove it from the list of loaded plug-ins.
        size_t oldPluginIndex = findPluginWithBundleIdentifier(alreadyLoadedPlugins, oldPluginBundleIdentifier);
        if (oldPluginIndex != notFound)
            alreadyLoadedPlugins.remove(oldPluginIndex);
    }

    return true;
}

bool PluginInfoStore::shouldUsePlugin(Vector<PluginModuleInfo>& alreadyLoadedPlugins, const PluginModuleInfo& plugin)
{
    for (size_t i = 0; i < alreadyLoadedPlugins.size(); ++i) {
        const PluginModuleInfo& loadedPlugin = alreadyLoadedPlugins[i];

        // If a plug-in with the same bundle identifier already exists, we don't want to load it.
        // However, if the already existing plug-in is blocked we want to replace it with the new plug-in.
        if (loadedPlugin.bundleIdentifier == plugin.bundleIdentifier) {
            if (!shouldBlockPlugin(loadedPlugin))
                return false;

            alreadyLoadedPlugins.remove(i);
            break;
        }
    }

    // Prefer the Oracle Java plug-in over the Apple java plug-in.
    if (!checkForPreferredPlugin(alreadyLoadedPlugins, plugin, "com.apple.java.JavaAppletPlugin", "com.oracle.java.JavaAppletPlugin"))
        return false;

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    if (plugin.bundleIdentifier == "com.apple.java.JavaAppletPlugin" && shouldBlockPlugin(plugin) && !WKJLIsRuntimeAndWebComponentsInstalled()) {
        // If the Apple Java plug-in is blocked and there's no Java runtime installed, just pretend that the plug-in doesn't exist.
        return false;
    }
#endif

    return true;
}

bool PluginInfoStore::shouldBlockPlugin(const PluginModuleInfo& plugin)
{
    return WKShouldBlockPlugin(plugin.bundleIdentifier, plugin.versionString);
}

String PluginInfoStore::getMIMETypeForExtension(const String& extension)
{
    // FIXME: This should just call MIMETypeRegistry::getMIMETypeForExtension and be
    // strength reduced into the callsite once we can safely convert String
    // to CFStringRef off the main thread.

    RetainPtr<CFStringRef> extensionCFString(AdoptCF, safeCreateCFString(extension));
    return WKGetMIMETypeForExtension((NSString *)extensionCFString.get());
}

} // namespace WebKit
