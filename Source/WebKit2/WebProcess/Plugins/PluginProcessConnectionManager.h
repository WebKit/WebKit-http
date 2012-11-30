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

#ifndef PluginProcessConnectionManager_h
#define PluginProcessConnectionManager_h

#if ENABLE(PLUGIN_PROCESS)

#include "PluginProcess.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

// Manages plug-in process connections for the given web process.

namespace CoreIPC {
    class Connection;
}

namespace WebKit {

class PluginProcessConnection;
        
class PluginProcessConnectionManager {
    WTF_MAKE_NONCOPYABLE(PluginProcessConnectionManager);
public:
    PluginProcessConnectionManager();
    ~PluginProcessConnectionManager();

    PluginProcessConnection* getPluginProcessConnection(const String& pluginPath, PluginProcess::Type);
    void removePluginProcessConnection(PluginProcessConnection*);

    // Called on the web process connection work queue.
    void pluginProcessCrashed(const String& pluginPath, PluginProcess::Type);

private:
    Vector<RefPtr<PluginProcessConnection> > m_pluginProcessConnections;

    Mutex m_pathsAndConnectionsMutex;
    HashMap<std::pair<String, PluginProcess::Type>, RefPtr<CoreIPC::Connection> > m_pathsAndConnections;
};

}

#endif // ENABLE(PLUGIN_PROCESS)

#endif // PluginProcessConnectionManager_h
