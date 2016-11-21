/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(DATABASE_PROCESS)

#include "ArgumentCoders.h"
#include "SandboxExtension.h"
#include "WebsiteDataType.h"
#include <WebCore/SecurityOriginData.h>
#include <chrono>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
    class SessionID;
}

namespace WebKit {
    struct DatabaseProcessCreationParameters;
}

namespace Messages {
namespace DatabaseProcess {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("DatabaseProcess");
}

class InitializeDatabaseProcess {
public:
    typedef std::tuple<const WebKit::DatabaseProcessCreationParameters&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InitializeDatabaseProcess"); }
    static const bool isSync = false;

    explicit InitializeDatabaseProcess(const WebKit::DatabaseProcessCreationParameters& processCreationParameters)
        : m_arguments(processCreationParameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CreateDatabaseToWebProcessConnection {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateDatabaseToWebProcessConnection"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FetchWebsiteData {
public:
    typedef std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FetchWebsiteData"); }
    static const bool isSync = false;

    FetchWebsiteData(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DeleteWebsiteData {
public:
    typedef std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const std::chrono::system_clock::time_point&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteData"); }
    static const bool isSync = false;

    DeleteWebsiteData(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, const std::chrono::system_clock::time_point& modifiedSince, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, modifiedSince, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DeleteWebsiteDataForOrigins {
public:
    typedef std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const Vector<WebCore::SecurityOriginData>&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteDataForOrigins"); }
    static const bool isSync = false;

    DeleteWebsiteDataForOrigins(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, const Vector<WebCore::SecurityOriginData>& origins, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, origins, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(SANDBOX_EXTENSIONS)
class GrantSandboxExtensionsForBlobs {
public:
    typedef std::tuple<const Vector<String>&, const WebKit::SandboxExtension::HandleArray&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GrantSandboxExtensionsForBlobs"); }
    static const bool isSync = false;

    GrantSandboxExtensionsForBlobs(const Vector<String>& paths, const WebKit::SandboxExtension::HandleArray& extensions)
        : m_arguments(paths, extensions)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if ENABLE(SANDBOX_EXTENSIONS)
class DidGetSandboxExtensionsForBlobFiles {
public:
    typedef std::tuple<uint64_t, const WebKit::SandboxExtension::HandleArray&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetSandboxExtensionsForBlobFiles"); }
    static const bool isSync = false;

    DidGetSandboxExtensionsForBlobFiles(uint64_t requestID, const WebKit::SandboxExtension::HandleArray& extensions)
        : m_arguments(requestID, extensions)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

} // namespace DatabaseProcess
} // namespace Messages

#endif // ENABLE(DATABASE_PROCESS)
