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

#ifndef NetworkProcessMessages_h
#define NetworkProcessMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "SandboxExtension.h"
#include "StringReference.h"
#include "WebsiteDataFetchOption.h"
#include "WebsiteDataType.h"
#include <WebCore/Proxy.h>
#include <WebCore/SecurityOriginData.h>
#include <chrono>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class DataReference;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class CertificateInfo;
    class SessionID;
    class ResourceRequest;
}

namespace WebKit {
    struct NetworkProcessCreationParameters;
    class DownloadID;
}

namespace Messages {
namespace NetworkProcess {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("NetworkProcess");
}

class InitializeNetworkProcess {
public:
    typedef std::tuple<WebKit::NetworkProcessCreationParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InitializeNetworkProcess"); }
    static const bool isSync = false;

    explicit InitializeNetworkProcess(const WebKit::NetworkProcessCreationParameters& processCreationParameters)
        : m_arguments(processCreationParameters)
    {
    }

    const std::tuple<const WebKit::NetworkProcessCreationParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NetworkProcessCreationParameters&> m_arguments;
};

class CreateNetworkConnectionToWebProcess {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateNetworkConnectionToWebProcess"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

#if USE(SOUP)
class SetIgnoreTLSErrors {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetIgnoreTLSErrors"); }
    static const bool isSync = false;

    explicit SetIgnoreTLSErrors(bool ignoreTLSErrors)
        : m_arguments(ignoreTLSErrors)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};
#endif

#if USE(SOUP)
class UserPreferredLanguagesChanged {
public:
    typedef std::tuple<Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UserPreferredLanguagesChanged"); }
    static const bool isSync = false;

    explicit UserPreferredLanguagesChanged(const Vector<String>& languages)
        : m_arguments(languages)
    {
    }

    const std::tuple<const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&> m_arguments;
};
#endif

#if USE(SOUP)
class SetProxies {
public:
    typedef std::tuple<WebCore::SessionID, Vector<WebCore::Proxy>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetProxies"); }
    static const bool isSync = false;

    SetProxies(const WebCore::SessionID& sessionID, const Vector<WebCore::Proxy>& proxies)
        : m_arguments(sessionID, proxies)
    {
    }

    const std::tuple<const WebCore::SessionID&, const Vector<WebCore::Proxy>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const Vector<WebCore::Proxy>&> m_arguments;
};
#endif

class ClearCachedCredentials {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearCachedCredentials"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class EnsurePrivateBrowsingSession {
public:
    typedef std::tuple<WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EnsurePrivateBrowsingSession"); }
    static const bool isSync = false;

    explicit EnsurePrivateBrowsingSession(const WebCore::SessionID& sessionID)
        : m_arguments(sessionID)
    {
    }

    const std::tuple<const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&> m_arguments;
};

class DestroyPrivateBrowsingSession {
public:
    typedef std::tuple<WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DestroyPrivateBrowsingSession"); }
    static const bool isSync = false;

    explicit DestroyPrivateBrowsingSession(const WebCore::SessionID& sessionID)
        : m_arguments(sessionID)
    {
    }

    const std::tuple<const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&> m_arguments;
};

class FetchWebsiteData {
public:
    typedef std::tuple<WebCore::SessionID, OptionSet<WebKit::WebsiteDataType>, OptionSet<WebKit::WebsiteDataFetchOption>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FetchWebsiteData"); }
    static const bool isSync = false;

    FetchWebsiteData(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, const OptionSet<WebKit::WebsiteDataFetchOption>& fetchOptions, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, fetchOptions, callbackID)
    {
    }

    const std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const OptionSet<WebKit::WebsiteDataFetchOption>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const OptionSet<WebKit::WebsiteDataFetchOption>&, uint64_t> m_arguments;
};

class DeleteWebsiteData {
public:
    typedef std::tuple<WebCore::SessionID, OptionSet<WebKit::WebsiteDataType>, std::chrono::system_clock::time_point, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteData"); }
    static const bool isSync = false;

    DeleteWebsiteData(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, const std::chrono::system_clock::time_point& modifiedSince, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, modifiedSince, callbackID)
    {
    }

    const std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const std::chrono::system_clock::time_point&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const std::chrono::system_clock::time_point&, uint64_t> m_arguments;
};

class DeleteWebsiteDataForOrigins {
public:
    typedef std::tuple<WebCore::SessionID, OptionSet<WebKit::WebsiteDataType>, Vector<WebCore::SecurityOriginData>, Vector<String>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteDataForOrigins"); }
    static const bool isSync = false;

    DeleteWebsiteDataForOrigins(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, const Vector<WebCore::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, origins, cookieHostNames, callbackID)
    {
    }

    const std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const Vector<WebCore::SecurityOriginData>&, const Vector<String>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const Vector<WebCore::SecurityOriginData>&, const Vector<String>&, uint64_t> m_arguments;
};

class DownloadRequest {
public:
    typedef std::tuple<WebCore::SessionID, WebKit::DownloadID, WebCore::ResourceRequest> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DownloadRequest"); }
    static const bool isSync = false;

    DownloadRequest(const WebCore::SessionID& sessionID, const WebKit::DownloadID& downloadID, const WebCore::ResourceRequest& request)
        : m_arguments(sessionID, downloadID, request)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebKit::DownloadID&, const WebCore::ResourceRequest&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebKit::DownloadID&, const WebCore::ResourceRequest&> m_arguments;
};

class ResumeDownload {
public:
    typedef std::tuple<WebCore::SessionID, WebKit::DownloadID, IPC::DataReference, String, WebKit::SandboxExtension::Handle> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResumeDownload"); }
    static const bool isSync = false;

    ResumeDownload(const WebCore::SessionID& sessionID, const WebKit::DownloadID& downloadID, const IPC::DataReference& resumeData, const String& path, const WebKit::SandboxExtension::Handle& sandboxExtensionHandle)
        : m_arguments(sessionID, downloadID, resumeData, path, sandboxExtensionHandle)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebKit::DownloadID&, const IPC::DataReference&, const String&, const WebKit::SandboxExtension::Handle&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebKit::DownloadID&, const IPC::DataReference&, const String&, const WebKit::SandboxExtension::Handle&> m_arguments;
};

class CancelDownload {
public:
    typedef std::tuple<WebKit::DownloadID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelDownload"); }
    static const bool isSync = false;

    explicit CancelDownload(const WebKit::DownloadID& downloadID)
        : m_arguments(downloadID)
    {
    }

    const std::tuple<const WebKit::DownloadID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::DownloadID&> m_arguments;
};

#if USE(NETWORK_SESSION)
class ContinueCanAuthenticateAgainstProtectionSpace {
public:
    typedef std::tuple<WebKit::DownloadID, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueCanAuthenticateAgainstProtectionSpace"); }
    static const bool isSync = false;

    ContinueCanAuthenticateAgainstProtectionSpace(const WebKit::DownloadID& downloadID, bool canAuthenticate)
        : m_arguments(downloadID, canAuthenticate)
    {
    }

    const std::tuple<const WebKit::DownloadID&, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::DownloadID&, bool> m_arguments;
};
#endif

#if USE(NETWORK_SESSION)
class ContinueWillSendRequest {
public:
    typedef std::tuple<WebKit::DownloadID, WebCore::ResourceRequest> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueWillSendRequest"); }
    static const bool isSync = false;

    ContinueWillSendRequest(const WebKit::DownloadID& downloadID, const WebCore::ResourceRequest& request)
        : m_arguments(downloadID, request)
    {
    }

    const std::tuple<const WebKit::DownloadID&, const WebCore::ResourceRequest&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::DownloadID&, const WebCore::ResourceRequest&> m_arguments;
};
#endif

#if USE(NETWORK_SESSION)
class ContinueDecidePendingDownloadDestination {
public:
    typedef std::tuple<WebKit::DownloadID, String, WebKit::SandboxExtension::Handle, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueDecidePendingDownloadDestination"); }
    static const bool isSync = false;

    ContinueDecidePendingDownloadDestination(const WebKit::DownloadID& downloadID, const String& destination, const WebKit::SandboxExtension::Handle& sandboxExtensionHandle, bool allowOverwrite)
        : m_arguments(downloadID, destination, sandboxExtensionHandle, allowOverwrite)
    {
    }

    const std::tuple<const WebKit::DownloadID&, const String&, const WebKit::SandboxExtension::Handle&, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::DownloadID&, const String&, const WebKit::SandboxExtension::Handle&, bool> m_arguments;
};
#endif

class SetProcessSuppressionEnabled {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetProcessSuppressionEnabled"); }
    static const bool isSync = false;

    explicit SetProcessSuppressionEnabled(bool flag)
        : m_arguments(flag)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

#if PLATFORM(COCOA)
class SetQOS {
public:
    typedef std::tuple<int, int> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetQOS"); }
    static const bool isSync = false;

    SetQOS(const int& latencyQOS, const int& throughputQOS)
        : m_arguments(latencyQOS, throughputQOS)
    {
    }

    const std::tuple<const int&, const int&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const int&, const int&> m_arguments;
};
#endif

class AllowSpecificHTTPSCertificateForHost {
public:
    typedef std::tuple<WebCore::CertificateInfo, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AllowSpecificHTTPSCertificateForHost"); }
    static const bool isSync = false;

    AllowSpecificHTTPSCertificateForHost(const WebCore::CertificateInfo& certificate, const String& host)
        : m_arguments(certificate, host)
    {
    }

    const std::tuple<const WebCore::CertificateInfo&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::CertificateInfo&, const String&> m_arguments;
};

class SetCanHandleHTTPSServerTrustEvaluation {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCanHandleHTTPSServerTrustEvaluation"); }
    static const bool isSync = false;

    explicit SetCanHandleHTTPSServerTrustEvaluation(bool value)
        : m_arguments(value)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class GetNetworkProcessStatistics {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetNetworkProcessStatistics"); }
    static const bool isSync = false;

    explicit GetNetworkProcessStatistics(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class ClearCacheForAllOrigins {
public:
    typedef std::tuple<uint32_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearCacheForAllOrigins"); }
    static const bool isSync = false;

    explicit ClearCacheForAllOrigins(uint32_t cachesToClear)
        : m_arguments(cachesToClear)
    {
    }

    const std::tuple<uint32_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t> m_arguments;
};

class SetCacheModel {
public:
    typedef std::tuple<uint32_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCacheModel"); }
    static const bool isSync = false;

    explicit SetCacheModel(uint32_t cacheModel)
        : m_arguments(cacheModel)
    {
    }

    const std::tuple<uint32_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t> m_arguments;
};

class ProcessWillSuspendImminently {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ProcessWillSuspendImminently"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class PrepareToSuspend {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PrepareToSuspend"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class CancelPrepareToSuspend {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelPrepareToSuspend"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class ProcessDidResume {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ProcessDidResume"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class DidGrantSandboxExtensionsToDatabaseProcessForBlobs {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGrantSandboxExtensionsToDatabaseProcessForBlobs"); }
    static const bool isSync = false;

    explicit DidGrantSandboxExtensionsToDatabaseProcessForBlobs(uint64_t requestID)
        : m_arguments(requestID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

} // namespace NetworkProcess
} // namespace Messages

#endif // NetworkProcessMessages_h
