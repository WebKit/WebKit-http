/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "NetworkJob.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "CookieManager.h"
#include "CredentialStorage.h"
#include "Frame.h"
#include "FrameLoaderClientBlackBerry.h"
#include "HTTPParsers.h"
#include "KURL.h"
#include "MIMESniffing.h"
#include "MIMETypeRegistry.h"
#include "NetworkManager.h"
#include "Page.h"
#include "RSSFilterStream.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"

#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformSettings.h>
#include <LocalizeResource.h>
#include <network/MultipartStream.h>
#include <network/NetworkStreamFactory.h>

namespace WebCore {

static const int s_redirectMaximum = 10;

inline static bool isInfo(int statusCode)
{
    return 100 <= statusCode && statusCode < 200;
}

inline static bool isRedirect(int statusCode)
{
    return 300 <= statusCode && statusCode < 400 && statusCode != 304;
}

inline static bool isUnauthorized(int statusCode)
{
    return statusCode == 401;
}

NetworkJob::NetworkJob()
    : m_playerId(0)
    , m_deleteJobTimer(this, &NetworkJob::fireDeleteJobTimer)
    , m_streamFactory(0)
    , m_isFile(false)
    , m_isFTP(false)
    , m_isFTPDir(true)
#ifndef NDEBUG
    , m_isRunning(true) // Always started immediately after creation.
#endif
    , m_cancelled(false)
    , m_statusReceived(false)
    , m_dataReceived(false)
    , m_responseSent(false)
    , m_callingClient(false)
    , m_needsRetryAsFTPDirectory(false)
    , m_isOverrideContentType(false)
    , m_newJobWithCredentialsStarted(false)
    , m_extendedStatusCode(0)
    , m_redirectCount(0)
    , m_deferredData(*this)
    , m_deferLoadingCount(0)
    , m_frame(0)
{
}

bool NetworkJob::initialize(int playerId,
                            const String& pageGroupName,
                            const KURL& url,
                            const BlackBerry::Platform::NetworkRequest& request,
                            PassRefPtr<ResourceHandle> handle,
                            BlackBerry::Platform::NetworkStreamFactory* streamFactory,
                            const Frame& frame,
                            int deferLoadingCount,
                            int redirectCount)
{
    m_playerId = playerId;
    m_pageGroupName = pageGroupName;

    m_response.setURL(url);
    m_isFile = url.protocolIs("file") || url.protocolIs("local");
    m_isFTP = url.protocolIs("ftp");

    m_handle = handle;

    m_streamFactory = streamFactory;
    m_frame = &frame;

    if (m_frame && m_frame->loader()->pageDismissalEventBeingDispatched() != FrameLoader::NoDismissal) {
        // In the case the frame will be detached soon, we still need to ping the server, but it is
        // no longer safe to reference the Frame object.
        // See http://trac.webkit.org/changeset/65910 and https://bugs.webkit.org/show_bug.cgi?id=30457.
        m_frame = 0;
    }

    m_redirectCount = redirectCount;
    m_deferLoadingCount = deferLoadingCount;

    // We don't need to explicitly call notifyHeaderReceived, as the Content-Type
    // will ultimately get parsed when sendResponseIfNeeded gets called.
    if (!request.getOverrideContentType().empty()) {
        m_contentType = String(request.getOverrideContentType().c_str());
        m_isOverrideContentType = true;
    }

    if (!request.getSuggestedSaveName().empty()) {
        m_contentDisposition = "filename=";
        m_contentDisposition += request.getSuggestedSaveName().c_str();
    }

    BlackBerry::Platform::FilterStream* wrappedStream = m_streamFactory->createNetworkStream(request, m_playerId);
    if (!wrappedStream)
        return false;

    BlackBerry::Platform::NetworkRequest::TargetType targetType = request.getTargetType();
    if ((targetType == BlackBerry::Platform::NetworkRequest::TargetIsMainFrame
         || targetType == BlackBerry::Platform::NetworkRequest::TargetIsSubframe)
            && !m_isOverrideContentType) {
        RSSFilterStream* filter = new RSSFilterStream();
        filter->setWrappedStream(wrappedStream);
        wrappedStream = filter;
    }

    setWrappedStream(wrappedStream);

    return true;
}

int NetworkJob::cancelJob()
{
    m_cancelled = true;

    return streamCancel();
}

void NetworkJob::updateDeferLoadingCount(int delta)
{
    m_deferLoadingCount += delta;
    ASSERT(m_deferLoadingCount >= 0);

    if (!isDeferringLoading()) {
        // There might already be a timer set to call this, but it's safe to schedule it again.
        m_deferredData.scheduleProcessDeferredData();
    }
}

void NetworkJob::notifyStatusReceived(int status, const char* message)
{
    if (shouldDeferLoading())
        m_deferredData.deferOpen(status, message);
    else
        handleNotifyStatusReceived(status, message);
}

void NetworkJob::handleNotifyStatusReceived(int status, const String& message)
{
    // Check for messages out of order or after cancel.
    if (m_responseSent || m_cancelled)
        return;

    if (isInfo(status))
        return; // ignore

    // Load up error page and ask the user to change their wireless proxy settings
    if (status == 407) {
        const ResourceError error = ResourceError(ResourceError::httpErrorDomain, BlackBerry::Platform::FilterStream::StatusWifiProxyAuthError, m_response.url().string(), emptyString());
        m_handle->client()->didFail(m_handle.get(), error);
    }

    m_statusReceived = true;

    // Convert non-HTTP status codes to generic HTTP codes.
    m_extendedStatusCode = status;
    if (!status)
        m_response.setHTTPStatusCode(200);
    else if (status < 0)
        m_response.setHTTPStatusCode(404);
    else
        m_response.setHTTPStatusCode(status);

    m_response.setHTTPStatusText(message);

    if (isUnauthorized(m_extendedStatusCode)) {
        purgeCredentials();
        BlackBerry::Platform::log(BlackBerry::Platform::LogLevelCritical, "Authentication failed, purge the stored credentials for this site.");
    }
}

void NetworkJob::notifyHeadersReceived(BlackBerry::Platform::NetworkRequest::HeaderList& headers)
{
    BlackBerry::Platform::NetworkRequest::HeaderList::const_iterator endIt = headers.end();
    for (BlackBerry::Platform::NetworkRequest::HeaderList::const_iterator it = headers.begin(); it != endIt; ++it) {
        if (shouldDeferLoading())
            m_deferredData.deferHeaderReceived(it->first.c_str(), it->second.c_str());
        else {
            String keyString(it->first.c_str());
            String valueString;
            if (equalIgnoringCase(keyString, "Location")) {
                // Location, like all headers, is supposed to be Latin-1. But some sites (wikipedia) send it in UTF-8.
                // All byte strings that are valid UTF-8 are also valid Latin-1 (although outside ASCII, the meaning will
                // differ), but the reverse isn't true. So try UTF-8 first and fall back to Latin-1 if it's invalid.
                // (High Latin-1 should be url-encoded anyway.)
                //
                // FIXME: maybe we should do this with other headers?
                // Skip it for now - we don't want to rewrite random bytes unless we're sure. (Definitely don't want to
                // rewrite cookies, for instance.) Needs more investigation.
                valueString = String::fromUTF8(it->second.c_str());
                if (valueString.isNull())
                    valueString = it->second.c_str();
            } else
                valueString = it->second.c_str();

            handleNotifyHeaderReceived(keyString, valueString);
        }
    }
}

void NetworkJob::notifyMultipartHeaderReceived(const char* key, const char* value)
{
    if (shouldDeferLoading())
        m_deferredData.deferMultipartHeaderReceived(key, value);
    else
        handleNotifyMultipartHeaderReceived(key, value);
}

void NetworkJob::notifyAuthReceived(BlackBerry::Platform::NetworkRequest::AuthType authType, const char* realm, bool success, bool requireCredentials)
{
    using BlackBerry::Platform::NetworkRequest;

    ProtectionSpaceServerType serverType = ProtectionSpaceServerHTTP;
    ProtectionSpaceAuthenticationScheme scheme = ProtectionSpaceAuthenticationSchemeDefault;
    switch (authType) {
    case NetworkRequest::AuthHTTPBasic:
        scheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
        break;
    case NetworkRequest::AuthHTTPDigest:
        scheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
        break;
    case NetworkRequest::AuthNegotiate:
        scheme = ProtectionSpaceAuthenticationSchemeNegotiate;
        break;
    case NetworkRequest::AuthHTTPNTLM:
        scheme = ProtectionSpaceAuthenticationSchemeNTLM;
        break;
    case NetworkRequest::AuthFTP:
        serverType = ProtectionSpaceServerFTP;
        break;
    case NetworkRequest::AuthProxy:
        serverType = ProtectionSpaceProxyHTTP;
        break;
    case NetworkRequest::AuthNone:
    default:
        return;
    }

    if (success) {
        // Update the credentials that will be stored to match the scheme that was actually used
        AuthenticationChallenge& challenge = m_handle->getInternal()->m_currentWebChallenge;
        if (!challenge.isNull()) {
            const ProtectionSpace& oldSpace = challenge.protectionSpace();
            if (oldSpace.authenticationScheme() != scheme) {
                // The scheme might have changed, but the server type shouldn't have!
                BLACKBERRY_ASSERT(serverType == oldSpace.serverType());
                ProtectionSpace newSpace(oldSpace.host(), oldSpace.port(), oldSpace.serverType(), oldSpace.realm(), scheme);
                m_handle->getInternal()->m_currentWebChallenge = AuthenticationChallenge(newSpace,
                                                                                         challenge.proposedCredential(),
                                                                                         challenge.previousFailureCount(),
                                                                                         challenge.failureResponse(),
                                                                                         challenge.error());
            }
        }
        storeCredentials();
        return;
    } else if (serverType != ProtectionSpaceProxyHTTP)
        // If a wifi proxy auth failed, there is no point of trying anymore because the credentials are wrong.
        m_newJobWithCredentialsStarted = sendRequestWithCredentials(serverType, scheme, realm, requireCredentials);
}

void NetworkJob::notifyStringHeaderReceived(const String& key, const String& value)
{
    if (shouldDeferLoading())
        m_deferredData.deferHeaderReceived(key, value);
    else
        handleNotifyHeaderReceived(key, value);
}

void NetworkJob::handleNotifyHeaderReceived(const String& key, const String& value)
{
    // Check for messages out of order or after cancel.
    if (!m_statusReceived || m_responseSent || m_cancelled)
        return;

    String lowerKey = key.lower();
    if (lowerKey == "content-type")
        m_contentType = value.lower();
    else if (lowerKey == "content-disposition")
        m_contentDisposition = value;
    else if (lowerKey == "set-cookie") {
        // FIXME: If a tab is closed, sometimes network data will come in after the frame has been detached from its page but before it is deleted.
        // If this happens, m_frame->page() will return 0, and m_frame->loader()->client() will be in a bad state and calling into it will crash.
        // For now we check for this explicitly by checking m_frame->page(). But we should find out why the network job hasn't been cancelled when the frame was detached.
        // See RIM PR 134207
        if (m_frame && m_frame->page() && m_frame->loader() && m_frame->loader()->client()
            && static_cast<FrameLoaderClientBlackBerry*>(m_frame->loader()->client())->cookiesEnabled())
            handleSetCookieHeader(value);

        if (m_response.httpHeaderFields().contains("Set-Cookie")) {
            m_response.setHTTPHeaderField(key, m_response.httpHeaderField(key) + "\r\n" + value);
            return;
        }
    } else if (equalIgnoringCase(key, BlackBerry::Platform::NetworkRequest::HEADER_BLACKBERRY_FTP))
        handleFTPHeader(value);

    m_response.setHTTPHeaderField(key, value);
}

void NetworkJob::handleNotifyMultipartHeaderReceived(const String& key, const String& value)
{
    if (!m_multipartResponse) {
        // Create a new response based on the original set of headers + the
        // replacement headers. We only replace the same few headers that gecko
        // does. See netwerk/streamconv/converters/nsMultiMixedConv.cpp.
        m_multipartResponse = adoptPtr(new ResourceResponse);
        m_multipartResponse->setURL(m_response.url());

        // The list of BlackBerry::Platform::replaceHeaders that we do not copy from the original
        // response when generating a response.
        const WebCore::HTTPHeaderMap& map = m_response.httpHeaderFields();

        for (WebCore::HTTPHeaderMap::const_iterator it = map.begin(); it != map.end(); ++it) {
            bool needsCopyfromOriginalResponse = true;
            int replaceHeadersIndex = 0;
            while (BlackBerry::Platform::MultipartStream::replaceHeaders[replaceHeadersIndex]) {
                if (it->first.lower() == BlackBerry::Platform::MultipartStream::replaceHeaders[replaceHeadersIndex]) {
                    needsCopyfromOriginalResponse = false;
                    break;
                }
                replaceHeadersIndex++;
            }
            if (needsCopyfromOriginalResponse)
                m_multipartResponse->setHTTPHeaderField(it->first, it->second);
        }

        m_multipartResponse->setIsMultipartPayload(true);
    }

    if (key.lower() == "content-type") {
        String contentType = value.lower();
        m_multipartResponse->setMimeType(extractMIMETypeFromMediaType(contentType));
        m_multipartResponse->setTextEncodingName(extractCharsetFromMediaType(contentType));
    }
    m_multipartResponse->setHTTPHeaderField(key, value);
}

void NetworkJob::handleSetCookieHeader(const String& value)
{
    KURL url = m_response.url();
    CookieManager& manager = cookieManager();
    if ((manager.cookiePolicy() == CookieStorageAcceptPolicyOnlyFromMainDocumentDomain)
      && (m_handle->firstRequest().firstPartyForCookies() != url)
      && manager.getCookie(url, WithHttpOnlyCookies).isEmpty())
        return;
    manager.setCookies(url, value);
}

void NetworkJob::notifyDataReceivedPlain(const char* buf, size_t len)
{
    if (shouldDeferLoading())
        m_deferredData.deferDataReceived(buf, len);
    else
        handleNotifyDataReceived(buf, len);
}

void NetworkJob::handleNotifyDataReceived(const char* buf, size_t len)
{
    // Check for messages out of order or after cancel.
    if ((!m_isFile && !m_statusReceived) || m_cancelled)
        return;

    if (!buf || !len)
        return;

    // The loadFile API sets the override content type,
    // this will always be used as the content type and should not be overridden.
    if (!m_dataReceived && !m_isOverrideContentType) {
        bool shouldSniff = true;

        // Don't bother sniffing the content type of a file that
        // is on a file system if it has a MIME mappable file extension.
        // The file extension is likely to be correct.
        if (m_isFile) {
            String urlFilename = m_response.url().lastPathComponent();
            size_t pos = urlFilename.reverseFind('.');
            if (pos != notFound) {
                String extension = urlFilename.substring(pos + 1);
                String mimeType = MIMETypeRegistry::getMIMETypeForExtension(extension);
                if (!mimeType.isEmpty())
                    shouldSniff = false;
            }
        }

        if (shouldSniff) {
            MIMESniffer sniffer = MIMESniffer(m_contentType.latin1().data(), MIMETypeRegistry::isSupportedImageResourceMIMEType(m_contentType));
            if (const char* type = sniffer.sniff(buf, std::min(len, sniffer.dataSize())))
                m_sniffedMimeType = String(type);
        }
    }

    m_dataReceived = true;

    // Protect against reentrancy.
    updateDeferLoadingCount(1);

    if (shouldSendClientData()) {
        sendResponseIfNeeded();
        sendMultipartResponseIfNeeded();
        if (isClientAvailable()) {
            RecursionGuard guard(m_callingClient);
            m_handle->client()->didReceiveData(m_handle.get(), buf, len, len);
        }
    }

    updateDeferLoadingCount(-1);
}

void NetworkJob::notifyDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    if (shouldDeferLoading())
        m_deferredData.deferDataSent(bytesSent, totalBytesToBeSent);
    else
        handleNotifyDataSent(bytesSent, totalBytesToBeSent);
}

void NetworkJob::handleNotifyDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    if (m_cancelled)
        return;

    // Protect against reentrancy.
    updateDeferLoadingCount(1);

    if (isClientAvailable()) {
        RecursionGuard guard(m_callingClient);
        m_handle->client()->didSendData(m_handle.get(), bytesSent, totalBytesToBeSent);
    }

    updateDeferLoadingCount(-1);
}

void NetworkJob::notifyClose(int status)
{
    if (shouldDeferLoading())
        m_deferredData.deferClose(status);
    else
        handleNotifyClose(status);
}

void NetworkJob::handleNotifyClose(int status)
{
#ifndef NDEBUG
    m_isRunning = false;
#endif
    if (!m_cancelled) {
        if (!m_statusReceived) {
            // Connection failed before sending notifyStatusReceived: use generic NetworkError.
            notifyStatusReceived(BlackBerry::Platform::FilterStream::StatusNetworkError, 0);
        }

        if (shouldReleaseClientResource()) {
            if (isRedirect(m_extendedStatusCode) && (m_redirectCount >= s_redirectMaximum))
                m_extendedStatusCode = BlackBerry::Platform::FilterStream::StatusTooManyRedirects;

            sendResponseIfNeeded();
            if (isClientAvailable()) {
                if (isError(status))
                    m_extendedStatusCode = status;
                RecursionGuard guard(m_callingClient);
                if (shouldNotifyClientFailed()) {
                    String domain = m_extendedStatusCode < 0 ? ResourceError::platformErrorDomain : ResourceError::httpErrorDomain;
                    ResourceError error(domain, m_extendedStatusCode, m_response.url().string(), m_response.httpStatusText());
                    m_handle->client()->didFail(m_handle.get(), error);
                } else
                    m_handle->client()->didFinishLoading(m_handle.get(), 0);
            }
        }
    }

    // Whoever called notifyClose still have a reference to the job, so
    // schedule the deletion with a timer.
    m_deleteJobTimer.startOneShot(0);

    // Detach from the ResourceHandle in any case.
    m_handle = 0;
    m_multipartResponse = nullptr;
}

bool NetworkJob::shouldReleaseClientResource()
{
    if ((m_needsRetryAsFTPDirectory && retryAsFTPDirectory()) || (isRedirect(m_extendedStatusCode) && handleRedirect()) || m_newJobWithCredentialsStarted)
        return false;
    return true;
}

bool NetworkJob::shouldNotifyClientFailed() const
{
    if (m_handle->firstRequest().targetType() == ResourceRequest::TargetIsXHR)
        return m_extendedStatusCode < 0;
    return isError(m_extendedStatusCode) && !m_dataReceived;
}

bool NetworkJob::retryAsFTPDirectory()
{
    m_needsRetryAsFTPDirectory = false;

    ASSERT(m_handle);
    ResourceRequest newRequest = m_handle->firstRequest();
    KURL url = newRequest.url();
    url.setPath(url.path() + "/");
    newRequest.setURL(url);
    newRequest.setMustHandleInternally(true);

    // Update the UI.
    handleNotifyHeaderReceived("Location", url.string());

    return startNewJobWithRequest(newRequest);
}

bool NetworkJob::startNewJobWithRequest(ResourceRequest& newRequest, bool increaseRedirectCount)
{
    // m_frame can be null if this is a PingLoader job (See NetworkJob::initialize).
    // In this case we don't start new request.
    if (!m_frame)
        return false;

    if (isClientAvailable()) {
        RecursionGuard guard(m_callingClient);
        m_handle->client()->willSendRequest(m_handle.get(), newRequest, m_response);

        // m_cancelled can become true if the url fails the policy check.
        // newRequest can be cleared when the redirect is rejected.
        if (m_cancelled || newRequest.isEmpty())
            return false;
    }

    // Pass the ownership of the ResourceHandle to the new NetworkJob.
    RefPtr<ResourceHandle> handle = m_handle;
    cancelJob();

    NetworkManager::instance()->startJob(m_playerId,
        m_pageGroupName,
        handle,
        newRequest,
        m_streamFactory,
        *m_frame,
        m_deferLoadingCount,
        increaseRedirectCount ? m_redirectCount + 1 : m_redirectCount);
    return true;
}

bool NetworkJob::handleRedirect()
{
    ASSERT(m_handle);
    if (!m_handle || m_redirectCount >= s_redirectMaximum)
        return false;

    String location = m_response.httpHeaderField("Location");
    if (location.isNull())
        return false;

    KURL newURL(m_response.url(), location);
    if (!newURL.isValid())
        return false;

    ResourceRequest newRequest = m_handle->firstRequest();
    newRequest.setURL(newURL);
    newRequest.setMustHandleInternally(true);

    String method = newRequest.httpMethod().upper();
    if (method != "GET" && method != "HEAD") {
        newRequest.setHTTPMethod("GET");
        newRequest.setHTTPBody(0);
        newRequest.clearHTTPContentLength();
        newRequest.clearHTTPContentType();
    }

    // Do not send existing credentials with the new request.
    m_handle->getInternal()->m_currentWebChallenge.nullify();

    return startNewJobWithRequest(newRequest, true);
}

void NetworkJob::sendResponseIfNeeded()
{
    if (m_responseSent)
        return;

    m_responseSent = true;

    if (shouldNotifyClientFailed())
        return;

    String urlFilename;
    if (!m_response.url().protocolIsData())
        urlFilename = m_response.url().lastPathComponent();

    // Get the MIME type that was set by the content sniffer
    // if there's no custom sniffer header, try to set it from the Content-Type header
    // if this fails, guess it from extension.
    String mimeType = m_sniffedMimeType;
    if (m_isFTP && m_isFTPDir)
        mimeType = "application/x-ftp-directory";
    else if (mimeType.isNull())
        mimeType = extractMIMETypeFromMediaType(m_contentType);
    if (mimeType.isNull())
        mimeType = MIMETypeRegistry::getMIMETypeForPath(urlFilename);
    if (!m_dataReceived && mimeType == "application/octet-stream") {
        // For empty content, if can't guess its mimetype from filename, we manually
        // set the mimetype to "text/plain" in case it goes to download.
        mimeType = "text/plain";
    }
    m_response.setMimeType(mimeType);

    // Set encoding from Content-Type header.
    m_response.setTextEncodingName(extractCharsetFromMediaType(m_contentType));

    // Set content length from header.
    String contentLength = m_response.httpHeaderField("Content-Length");
    if (!contentLength.isNull())
        m_response.setExpectedContentLength(contentLength.toInt64());

    String suggestedFilename = filenameFromHTTPContentDisposition(m_contentDisposition);
    if (suggestedFilename.isEmpty()) {
        // Check and see if an extension already exists.
        String mimeExtension = MIMETypeRegistry::getPreferredExtensionForMIMEType(mimeType);
        if (urlFilename.isEmpty()) {
            if (mimeExtension.isEmpty()) // No extension found for the mimeType.
                suggestedFilename = String(BlackBerry::Platform::LocalizeResource::getString(BlackBerry::Platform::FILENAME_UNTITLED));
            else
                suggestedFilename = String(BlackBerry::Platform::LocalizeResource::getString(BlackBerry::Platform::FILENAME_UNTITLED)) + "." + mimeExtension;
        } else {
            if (urlFilename.reverseFind('.') == notFound && !mimeExtension.isEmpty())
               suggestedFilename = urlFilename + '.' + mimeExtension;
            else
               suggestedFilename = urlFilename;
        }
    }
    m_response.setSuggestedFilename(suggestedFilename);

    if (isClientAvailable()) {
        RecursionGuard guard(m_callingClient);
        m_handle->client()->didReceiveResponse(m_handle.get(), m_response);
    }
}

void NetworkJob::sendMultipartResponseIfNeeded()
{
    if (m_multipartResponse && isClientAvailable()) {
        m_handle->client()->didReceiveResponse(m_handle.get(), *m_multipartResponse);
        m_multipartResponse = nullptr;
    }
}

bool NetworkJob::handleFTPHeader(const String& header)
{
    size_t spacePos = header.find(' ');
    if (spacePos == notFound)
        return false;
    String statusCode = header.left(spacePos);
    switch (statusCode.toInt()) {
    case 213:
        m_isFTPDir = false;
        break;
    case 530:
        purgeCredentials();
        sendRequestWithCredentials(ProtectionSpaceServerFTP, ProtectionSpaceAuthenticationSchemeDefault, "ftp");
        break;
    case 230:
        storeCredentials();
        break;
    case 550:
        // The user might have entered an URL which point to a directory but forgot type '/',
        // e.g., ftp://ftp.trolltech.com/qt/source where 'source' is a directory. We need to
        // added '/' and try again.
        if (m_handle && !m_handle->firstRequest().url().path().endsWith("/"))
            m_needsRetryAsFTPDirectory = true;
        break;
    }

    return true;
}

bool NetworkJob::sendRequestWithCredentials(ProtectionSpaceServerType type, ProtectionSpaceAuthenticationScheme scheme, const String& realm, bool requireCredentials)
{
    ASSERT(m_handle);
    if (!m_handle)
        return false;

    KURL newURL = m_response.url();
    if (!newURL.isValid())
        return false;

    String host;
    int port;
    host = m_response.url().host();
    port = m_response.url().port();

    ProtectionSpace protectionSpace(host, port, type, realm, scheme);

    // We've got the scheme and realm. Now we need a username and password.
    Credential credential;
    if (!requireCredentials) {
        // Don't overwrite any existing credentials with the empty credential
        if (m_handle->getInternal()->m_currentWebChallenge.isNull())
            m_handle->getInternal()->m_currentWebChallenge = AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError());
    } else if (!(credential = CredentialStorage::get(protectionSpace)).isEmpty()) {
        // First search the CredentialStorage.
        m_handle->getInternal()->m_currentWebChallenge = AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError());
        m_handle->getInternal()->m_currentWebChallenge.setStored(true);
    } else {
        if (m_handle->firstRequest().targetType() == ResourceRequest::TargetIsFavicon) {
            // The favicon loading is triggerred after the main resource has been loaded
            // and parsed, so if we cancel the authentication challenge when loading the main
            // resource, we should also cancel loading the favicon when it starts to
            // load. If not we will receive another challenge which may confuse the user.
            return false;
        }

        // CredentialStore is empty. Ask the user via dialog.
        String username;
        String password;

        // Before asking the user for credentials, we check if the URL contains that.
        if (!m_handle->getInternal()->m_user.isEmpty() && !m_handle->getInternal()->m_pass.isEmpty()) {
            username = m_handle->getInternal()->m_user;
            password = m_handle->getInternal()->m_pass;

            // Prevent them from been used again if they are wrong.
            // If they are correct, they will the put into CredentialStorage.
            m_handle->getInternal()->m_user = "";
            m_handle->getInternal()->m_pass = "";
        } else {
            if (m_handle->firstRequest().targetType() != ResourceRequest::TargetIsMainFrame && BlackBerry::Platform::Settings::instance()->isChromeProcess())
                return false;

            m_handle->getInternal()->m_currentWebChallenge = AuthenticationChallenge();
            m_frame->page()->chrome()->client()->platformPageClient()->authenticationChallenge(newURL, protectionSpace, Credential(), this);
            return true;
        }

        credential = Credential(username, password, CredentialPersistenceForSession);

        m_handle->getInternal()->m_currentWebChallenge = AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError());
    }

    notifyChallengeResult(newURL, protectionSpace, AuthenticationChallengeSuccess, credential);
    return m_newJobWithCredentialsStarted;
}

void NetworkJob::storeCredentials()
{
    if (!m_handle)
        return;

    AuthenticationChallenge& challenge = m_handle->getInternal()->m_currentWebChallenge;
    if (challenge.isNull())
        return;

    if (challenge.isStored())
        return;

    CredentialStorage::set(challenge.proposedCredential(), challenge.protectionSpace(), m_response.url());
    challenge.setStored(true);

    if (challenge.protectionSpace().serverType() == ProtectionSpaceProxyHTTP) {
        BlackBerry::Platform::Settings::instance()->setProxyCredential(challenge.proposedCredential().user().utf8().data(),
                                                                challenge.proposedCredential().password().utf8().data());
        m_frame->page()->chrome()->client()->platformPageClient()->syncProxyCredential(challenge.proposedCredential());
    }
}

void NetworkJob::purgeCredentials()
{
    if (!m_handle)
        return;

    AuthenticationChallenge& challenge = m_handle->getInternal()->m_currentWebChallenge;
    if (challenge.isNull())
        return;

    CredentialStorage::remove(challenge.protectionSpace());
    challenge.setStored(false);
}

bool NetworkJob::shouldSendClientData() const
{
    return (!isRedirect(m_extendedStatusCode) || !m_response.httpHeaderFields().contains("Location"))
           && !m_needsRetryAsFTPDirectory;
}

void NetworkJob::fireDeleteJobTimer(Timer<NetworkJob>*)
{
    NetworkManager::instance()->deleteJob(this);
}

void NetworkJob::notifyChallengeResult(const KURL& url, const ProtectionSpace& protectionSpace, AuthenticationChallengeResult result, const Credential& credential)
{
    if (result != AuthenticationChallengeSuccess || protectionSpace.host().isEmpty() || !url.isValid()) {
        m_newJobWithCredentialsStarted = false;
        return;
    }

    if (m_handle->getInternal()->m_currentWebChallenge.isNull())
        m_handle->getInternal()->m_currentWebChallenge = AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError());

    ResourceRequest newRequest = m_handle->firstRequest();
    newRequest.setURL(m_response.url());
    newRequest.setMustHandleInternally(true);
    m_newJobWithCredentialsStarted = startNewJobWithRequest(newRequest);
}

} // namespace WebCore
