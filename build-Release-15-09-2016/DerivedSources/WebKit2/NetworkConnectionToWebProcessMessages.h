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

#ifndef NetworkConnectionToWebProcessMessages_h
#define NetworkConnectionToWebProcessMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "SandboxExtension.h"
#include "StringReference.h"
#include <WebCore/BlobPart.h>
#include <WebCore/Cookie.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class Connection;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class ResourceResponse;
    class SessionID;
    class URL;
    struct Cookie;
    class ResourceRequest;
    class ResourceError;
}

namespace WebKit {
    class NetworkResourceLoadParameters;
    class DownloadID;
}

namespace Messages {
namespace NetworkConnectionToWebProcess {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("NetworkConnectionToWebProcess");
}

class ScheduleResourceLoad {
public:
    typedef std::tuple<WebKit::NetworkResourceLoadParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ScheduleResourceLoad"); }
    static const bool isSync = false;

    explicit ScheduleResourceLoad(const WebKit::NetworkResourceLoadParameters& resourceLoadParameters)
        : m_arguments(resourceLoadParameters)
    {
    }

    const std::tuple<const WebKit::NetworkResourceLoadParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NetworkResourceLoadParameters&> m_arguments;
};

class PerformSynchronousLoad {
public:
    typedef std::tuple<WebKit::NetworkResourceLoadParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformSynchronousLoad"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::MessageEncoder>);
        ~DelayedReply();

        bool send(const WebCore::ResourceError& error, const WebCore::ResourceResponse& response, const Vector<char>& data);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::MessageEncoder> m_encoder;
    };

    typedef IPC::Arguments<WebCore::ResourceError&, WebCore::ResourceResponse&, Vector<char>&> Reply;
    explicit PerformSynchronousLoad(const WebKit::NetworkResourceLoadParameters& resourceLoadParameters)
        : m_arguments(resourceLoadParameters)
    {
    }

    const std::tuple<const WebKit::NetworkResourceLoadParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NetworkResourceLoadParameters&> m_arguments;
};

class LoadPing {
public:
    typedef std::tuple<WebKit::NetworkResourceLoadParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadPing"); }
    static const bool isSync = false;

    explicit LoadPing(const WebKit::NetworkResourceLoadParameters& resourceLoadParameters)
        : m_arguments(resourceLoadParameters)
    {
    }

    const std::tuple<const WebKit::NetworkResourceLoadParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NetworkResourceLoadParameters&> m_arguments;
};

class RemoveLoadIdentifier {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveLoadIdentifier"); }
    static const bool isSync = false;

    explicit RemoveLoadIdentifier(uint64_t resourceLoadIdentifier)
        : m_arguments(resourceLoadIdentifier)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class SetDefersLoading {
public:
    typedef std::tuple<uint64_t, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDefersLoading"); }
    static const bool isSync = false;

    SetDefersLoading(uint64_t resourceLoadIdentifier, bool defers)
        : m_arguments(resourceLoadIdentifier, defers)
    {
    }

    const std::tuple<uint64_t, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, bool> m_arguments;
};

class PrefetchDNS {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PrefetchDNS"); }
    static const bool isSync = false;

    explicit PrefetchDNS(const String& hostname)
        : m_arguments(hostname)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class StartDownload {
public:
    typedef std::tuple<WebCore::SessionID, WebKit::DownloadID, WebCore::ResourceRequest, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StartDownload"); }
    static const bool isSync = false;

    StartDownload(const WebCore::SessionID& sessionID, const WebKit::DownloadID& downloadID, const WebCore::ResourceRequest& request, const String& suggestedName)
        : m_arguments(sessionID, downloadID, request, suggestedName)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebKit::DownloadID&, const WebCore::ResourceRequest&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebKit::DownloadID&, const WebCore::ResourceRequest&, const String&> m_arguments;
};

class ConvertMainResourceLoadToDownload {
public:
    typedef std::tuple<WebCore::SessionID, uint64_t, WebKit::DownloadID, WebCore::ResourceRequest, WebCore::ResourceResponse> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ConvertMainResourceLoadToDownload"); }
    static const bool isSync = false;

    ConvertMainResourceLoadToDownload(const WebCore::SessionID& sessionID, uint64_t mainResourceLoadIdentifier, const WebKit::DownloadID& downloadID, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response)
        : m_arguments(sessionID, mainResourceLoadIdentifier, downloadID, request, response)
    {
    }

    const std::tuple<const WebCore::SessionID&, uint64_t, const WebKit::DownloadID&, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, uint64_t, const WebKit::DownloadID&, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&> m_arguments;
};

class CookiesForDOM {
public:
    typedef std::tuple<WebCore::SessionID, WebCore::URL, WebCore::URL> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CookiesForDOM"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    CookiesForDOM(const WebCore::SessionID& sessionID, const WebCore::URL& firstParty, const WebCore::URL& url)
        : m_arguments(sessionID, firstParty, url)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&> m_arguments;
};

class SetCookiesFromDOM {
public:
    typedef std::tuple<WebCore::SessionID, WebCore::URL, WebCore::URL, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCookiesFromDOM"); }
    static const bool isSync = false;

    SetCookiesFromDOM(const WebCore::SessionID& sessionID, const WebCore::URL& firstParty, const WebCore::URL& url, const String& cookieString)
        : m_arguments(sessionID, firstParty, url, cookieString)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&, const String&> m_arguments;
};

class CookiesEnabled {
public:
    typedef std::tuple<WebCore::SessionID, WebCore::URL, WebCore::URL> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CookiesEnabled"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    CookiesEnabled(const WebCore::SessionID& sessionID, const WebCore::URL& firstParty, const WebCore::URL& url)
        : m_arguments(sessionID, firstParty, url)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&> m_arguments;
};

class CookieRequestHeaderFieldValue {
public:
    typedef std::tuple<WebCore::SessionID, WebCore::URL, WebCore::URL> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CookieRequestHeaderFieldValue"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    CookieRequestHeaderFieldValue(const WebCore::SessionID& sessionID, const WebCore::URL& firstParty, const WebCore::URL& url)
        : m_arguments(sessionID, firstParty, url)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&> m_arguments;
};

class GetRawCookies {
public:
    typedef std::tuple<WebCore::SessionID, WebCore::URL, WebCore::URL> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetRawCookies"); }
    static const bool isSync = true;

    typedef IPC::Arguments<Vector<WebCore::Cookie>&> Reply;
    GetRawCookies(const WebCore::SessionID& sessionID, const WebCore::URL& firstParty, const WebCore::URL& url)
        : m_arguments(sessionID, firstParty, url)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::URL&> m_arguments;
};

class DeleteCookie {
public:
    typedef std::tuple<WebCore::SessionID, WebCore::URL, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteCookie"); }
    static const bool isSync = false;

    DeleteCookie(const WebCore::SessionID& sessionID, const WebCore::URL& url, const String& cookieName)
        : m_arguments(sessionID, url, cookieName)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebCore::URL&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebCore::URL&, const String&> m_arguments;
};

class AddCookie {
public:
    typedef std::tuple<WebCore::SessionID, WebCore::URL, WebCore::Cookie> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddCookie"); }
    static const bool isSync = false;

    AddCookie(const WebCore::SessionID& sessionID, const WebCore::URL& url, const WebCore::Cookie& cookie)
        : m_arguments(sessionID, url, cookie)
    {
    }

    const std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::Cookie&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const WebCore::URL&, const WebCore::Cookie&> m_arguments;
};

class RegisterFileBlobURL {
public:
    typedef std::tuple<WebCore::URL, String, WebKit::SandboxExtension::Handle, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterFileBlobURL"); }
    static const bool isSync = false;

    RegisterFileBlobURL(const WebCore::URL& url, const String& path, const WebKit::SandboxExtension::Handle& extensionHandle, const String& contentType)
        : m_arguments(url, path, extensionHandle, contentType)
    {
    }

    const std::tuple<const WebCore::URL&, const String&, const WebKit::SandboxExtension::Handle&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::URL&, const String&, const WebKit::SandboxExtension::Handle&, const String&> m_arguments;
};

class RegisterBlobURL {
public:
    typedef std::tuple<WebCore::URL, Vector<WebCore::BlobPart>, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterBlobURL"); }
    static const bool isSync = false;

    RegisterBlobURL(const WebCore::URL& url, const Vector<WebCore::BlobPart>& blobParts, const String& contentType)
        : m_arguments(url, blobParts, contentType)
    {
    }

    const std::tuple<const WebCore::URL&, const Vector<WebCore::BlobPart>&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::URL&, const Vector<WebCore::BlobPart>&, const String&> m_arguments;
};

class RegisterBlobURLFromURL {
public:
    typedef std::tuple<WebCore::URL, WebCore::URL> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterBlobURLFromURL"); }
    static const bool isSync = false;

    RegisterBlobURLFromURL(const WebCore::URL& url, const WebCore::URL& srcURL)
        : m_arguments(url, srcURL)
    {
    }

    const std::tuple<const WebCore::URL&, const WebCore::URL&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::URL&, const WebCore::URL&> m_arguments;
};

class PreregisterSandboxExtensionsForOptionallyFileBackedBlob {
public:
    typedef std::tuple<Vector<String>, WebKit::SandboxExtension::HandleArray> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PreregisterSandboxExtensionsForOptionallyFileBackedBlob"); }
    static const bool isSync = false;

    PreregisterSandboxExtensionsForOptionallyFileBackedBlob(const Vector<String>& filePaths, const WebKit::SandboxExtension::HandleArray& extensionHandles)
        : m_arguments(filePaths, extensionHandles)
    {
    }

    const std::tuple<const Vector<String>&, const WebKit::SandboxExtension::HandleArray&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&, const WebKit::SandboxExtension::HandleArray&> m_arguments;
};

class RegisterBlobURLOptionallyFileBacked {
public:
    typedef std::tuple<WebCore::URL, WebCore::URL, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterBlobURLOptionallyFileBacked"); }
    static const bool isSync = false;

    RegisterBlobURLOptionallyFileBacked(const WebCore::URL& url, const WebCore::URL& srcURL, const String& fileBackedPath)
        : m_arguments(url, srcURL, fileBackedPath)
    {
    }

    const std::tuple<const WebCore::URL&, const WebCore::URL&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::URL&, const WebCore::URL&, const String&> m_arguments;
};

class RegisterBlobURLForSlice {
public:
    typedef std::tuple<WebCore::URL, WebCore::URL, int64_t, int64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterBlobURLForSlice"); }
    static const bool isSync = false;

    RegisterBlobURLForSlice(const WebCore::URL& url, const WebCore::URL& srcURL, int64_t start, int64_t end)
        : m_arguments(url, srcURL, start, end)
    {
    }

    const std::tuple<const WebCore::URL&, const WebCore::URL&, int64_t, int64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::URL&, const WebCore::URL&, int64_t, int64_t> m_arguments;
};

class UnregisterBlobURL {
public:
    typedef std::tuple<WebCore::URL> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UnregisterBlobURL"); }
    static const bool isSync = false;

    explicit UnregisterBlobURL(const WebCore::URL& url)
        : m_arguments(url)
    {
    }

    const std::tuple<const WebCore::URL&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::URL&> m_arguments;
};

class BlobSize {
public:
    typedef std::tuple<WebCore::URL> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BlobSize"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    explicit BlobSize(const WebCore::URL& url)
        : m_arguments(url)
    {
    }

    const std::tuple<const WebCore::URL&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::URL&> m_arguments;
};

class WriteBlobsToTemporaryFiles {
public:
    typedef std::tuple<Vector<String>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WriteBlobsToTemporaryFiles"); }
    static const bool isSync = false;

    WriteBlobsToTemporaryFiles(const Vector<String>& blobURLs, uint64_t requestIdentifier)
        : m_arguments(blobURLs, requestIdentifier)
    {
    }

    const std::tuple<const Vector<String>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&, uint64_t> m_arguments;
};

class EnsureLegacyPrivateBrowsingSession {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EnsureLegacyPrivateBrowsingSession"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

} // namespace NetworkConnectionToWebProcess
} // namespace Messages

#endif // NetworkConnectionToWebProcessMessages_h
