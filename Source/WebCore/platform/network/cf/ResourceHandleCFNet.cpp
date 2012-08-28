/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#include "ResourceHandleInternal.h"

#include "AuthenticationCF.h"
#include "AuthenticationChallenge.h"
#include "CookieStorageCFNet.h"
#include "CredentialStorage.h"
#include "CachedResourceLoader.h"
#include "FormDataStreamCFNet.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "LoaderRunLoopCF.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include <CFNetwork/CFNetwork.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wtf/HashMap.h>
#include <wtf/Threading.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>

#if PLATFORM(MAC)
#include "WebCoreSystemInterface.h"
#if USE(CFNETWORK)
#include "WebCoreURLResponse.h"
#include <CFNetwork/CFURLConnectionPriv.h>
#include <CFNetwork/CFURLRequestPriv.h>
#endif
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <process.h>

// FIXME: Remove this declaration once it's in WebKitSupportLibrary.
extern "C" {
__declspec(dllimport) CFURLConnectionRef CFURLConnectionCreateWithProperties(
  CFAllocatorRef           alloc,
  CFURLRequestRef          request,
  CFURLConnectionClient *  client,
  CFDictionaryRef properties);
}
#endif

namespace WebCore {

#if USE(CFNETWORK)

class WebCoreSynchronousLoaderClient : public ResourceHandleClient {
public:
    static PassOwnPtr<WebCoreSynchronousLoaderClient> create(ResourceResponse& response, ResourceError& error)
    {
        return adoptPtr(new WebCoreSynchronousLoaderClient(response, error));
    }

    void setAllowStoredCredentials(bool allow) { m_allowStoredCredentials = allow; }
    bool isDone() { return m_isDone; }

    CFMutableDataRef data() { return m_data.get(); }

private:
    WebCoreSynchronousLoaderClient(ResourceResponse& response, ResourceError& error)
        : m_allowStoredCredentials(false)
        , m_response(response)
        , m_error(error)
        , m_isDone(false)
    {
    }

    virtual void willSendRequest(ResourceHandle*, ResourceRequest&, const ResourceResponse& /*redirectResponse*/);
    virtual bool shouldUseCredentialStorage(ResourceHandle*);
    virtual void didReceiveAuthenticationChallenge(ResourceHandle*, const AuthenticationChallenge&);
    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse&);
    virtual void didReceiveData(ResourceHandle*, const char*, int, int /*encodedDataLength*/);
    virtual void didFinishLoading(ResourceHandle*, double /*finishTime*/);
    virtual void didFail(ResourceHandle*, const ResourceError&);
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual bool canAuthenticateAgainstProtectionSpace(ResourceHandle*, const ProtectionSpace&);
#endif

    bool m_allowStoredCredentials;
    ResourceResponse& m_response;
    RetainPtr<CFMutableDataRef> m_data;
    ResourceError& m_error;
    bool m_isDone;
};

static HashSet<String>& allowsAnyHTTPSCertificateHosts()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, hosts, ());
    return hosts;
}

static HashMap<String, RetainPtr<CFDataRef> >& clientCerts()
{
    typedef HashMap<String, RetainPtr<CFDataRef> > CertsMap;
    DEFINE_STATIC_LOCAL(CertsMap, certs, ());
    return certs;
}

static void setDefaultMIMEType(CFURLResponseRef response)
{
    static CFStringRef defaultMIMETypeString = defaultMIMEType().createCFString();
    
    CFURLResponseSetMIMEType(response, defaultMIMETypeString);
}

static void applyBasicAuthorizationHeader(ResourceRequest& request, const Credential& credential)
{
    String authenticationHeader = "Basic " + base64Encode(String(credential.user() + ":" + credential.password()).utf8());
    request.clearHTTPAuthorization(); // FIXME: Should addHTTPHeaderField be smart enough to not build comma-separated lists in headers like Authorization?
    request.addHTTPHeaderField("Authorization", authenticationHeader);
}

static CFURLRequestRef willSendRequest(CFURLConnectionRef conn, CFURLRequestRef cfRequest, CFURLResponseRef cfRedirectResponse, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    if (!cfRedirectResponse) {
        CFRetain(cfRequest);
        return cfRequest;
    }

    LOG(Network, "CFNet - willSendRequest(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    ResourceRequest request;
    if (cfRedirectResponse) {
        CFHTTPMessageRef httpMessage = CFURLResponseGetHTTPResponse(cfRedirectResponse);
        if (httpMessage && CFHTTPMessageGetResponseStatusCode(httpMessage) == 307) {
            RetainPtr<CFStringRef> lastHTTPMethod(AdoptCF, handle->lastHTTPMethod().createCFString());
            RetainPtr<CFStringRef> newMethod(AdoptCF, CFURLRequestCopyHTTPRequestMethod(cfRequest));
            if (CFStringCompareWithOptions(lastHTTPMethod.get(), newMethod.get(), CFRangeMake(0, CFStringGetLength(lastHTTPMethod.get())), kCFCompareCaseInsensitive)) {
                RetainPtr<CFMutableURLRequestRef> mutableRequest(AdoptCF, CFURLRequestCreateMutableCopy(0, cfRequest));
#if USE(CFURLSTORAGESESSIONS)
                wkSetRequestStorageSession(ResourceHandle::currentStorageSession(), mutableRequest.get());
#endif
                CFURLRequestSetHTTPRequestMethod(mutableRequest.get(), lastHTTPMethod.get());

                FormData* body = handle->firstRequest().httpBody();
                if (!equalIgnoringCase(handle->firstRequest().httpMethod(), "GET") && body && !body->isEmpty())
                    WebCore::setHTTPBody(mutableRequest.get(), body);

                String originalContentType = handle->firstRequest().httpContentType();
                RetainPtr<CFStringRef> originalContentTypeCF(AdoptCF, originalContentType.createCFString());
                if (!originalContentType.isEmpty())
                    CFURLRequestSetHTTPHeaderFieldValue(mutableRequest.get(), CFSTR("Content-Type"), originalContentTypeCF.get());

                request = mutableRequest.get();
            }
        }
    }
    if (request.isNull())
        request = cfRequest;

    // Should not set Referer after a redirect from a secure resource to non-secure one.
    if (!request.url().protocolIs("https") && protocolIs(request.httpReferrer(), "https"))
        request.clearHTTPReferrer();

    handle->willSendRequest(request, cfRedirectResponse);

    if (request.isNull())
        return 0;

    cfRequest = request.cfURLRequest();

    CFRetain(cfRequest);
    return cfRequest;
}

static void didReceiveResponse(CFURLConnectionRef conn, CFURLResponseRef cfResponse, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    LOG(Network, "CFNet - didReceiveResponse(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    if (!handle->client())
        return;

#if PLATFORM(MAC)
    // Avoid MIME type sniffing if the response comes back as 304 Not Modified.
    CFHTTPMessageRef msg = wkGetCFURLResponseHTTPResponse(cfResponse);
    int statusCode = msg ? CFHTTPMessageGetResponseStatusCode(msg) : 0;

    if (statusCode != 304)
        adjustMIMETypeIfNecessary(cfResponse);

    if (_CFURLRequestCopyProtocolPropertyForKey(handle->firstRequest().cfURLRequest(), CFSTR("ForceHTMLMIMEType")))
        wkSetCFURLResponseMIMEType(cfResponse, CFSTR("text/html"));
#else
    if (!CFURLResponseGetMIMEType(cfResponse)) {
        // We should never be applying the default MIMEType if we told the networking layer to do content sniffing for handle.
        ASSERT(!handle->shouldContentSniff());
        setDefaultMIMEType(cfResponse);
    }
#endif
    
    handle->client()->didReceiveResponse(handle, cfResponse);
}

#if HAVE(NETWORK_CFDATA_ARRAY_CALLBACK)
static void didReceiveDataArray(CFURLConnectionRef conn, CFArrayRef dataArray, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    if (!handle->client())
        return;

    LOG(Network, "CFNet - didReceiveDataArray(conn=%p, handle=%p, arrayLength=%ld) (%s)", conn, handle, CFArrayGetCount(dataArray), handle->firstRequest().url().string().utf8().data());

    handle->handleDataArray(dataArray);
}
#endif

static void didReceiveData(CFURLConnectionRef conn, CFDataRef data, CFIndex originalLength, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    const UInt8* bytes = CFDataGetBytePtr(data);
    CFIndex length = CFDataGetLength(data);

    LOG(Network, "CFNet - didReceiveData(conn=%p, handle=%p, bytes=%ld) (%s)", conn, handle, length, handle->firstRequest().url().string().utf8().data());

    if (handle->client())
        handle->client()->didReceiveData(handle, (const char*)bytes, length, originalLength);
}

static void didSendBodyData(CFURLConnectionRef, CFIndex, CFIndex totalBytesWritten, CFIndex totalBytesExpectedToWrite, const void *clientInfo)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    if (!handle || !handle->client())
        return;
    handle->client()->didSendData(handle, totalBytesWritten, totalBytesExpectedToWrite);
}

static Boolean shouldUseCredentialStorageCallback(CFURLConnectionRef conn, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = const_cast<ResourceHandle*>(static_cast<const ResourceHandle*>(clientInfo));

    LOG(Network, "CFNet - shouldUseCredentialStorage(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    if (!handle)
        return false;

    return handle->shouldUseCredentialStorage();
}

static void didFinishLoading(CFURLConnectionRef conn, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    LOG(Network, "CFNet - didFinishLoading(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    if (handle->client())
        handle->client()->didFinishLoading(handle, 0);
}

static void didFail(CFURLConnectionRef conn, CFErrorRef error, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    LOG(Network, "CFNet - didFail(conn=%p, handle=%p, error = %p) (%s)", conn, handle, error, handle->firstRequest().url().string().utf8().data());

    if (handle->client())
        handle->client()->didFail(handle, ResourceError(error));
}

static CFCachedURLResponseRef willCacheResponse(CFURLConnectionRef, CFCachedURLResponseRef cachedResponse, const void* clientInfo)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    CFURLResponseRef wrappedResponse = CFCachedURLResponseGetWrappedResponse(cachedResponse);

    // Workaround for <rdar://problem/6300990> Caching does not respect Vary HTTP header.
    // FIXME: WebCore cache has issues with Vary, too (bug 58797, bug 71509).
    if (CFHTTPMessageRef httpResponse = CFURLResponseGetHTTPResponse(wrappedResponse)) {
        ASSERT(CFHTTPMessageIsHeaderComplete(httpResponse));
        RetainPtr<CFStringRef> varyValue = adoptCF(CFHTTPMessageCopyHeaderFieldValue(httpResponse, CFSTR("Vary")));
        if (varyValue)
            return 0;
    }

#if PLATFORM(WIN)
    if (handle->client() && !handle->client()->shouldCacheResponse(handle, cachedResponse))
        return 0;
#else
    CFCachedURLResponseRef newResponse = handle->client()->willCacheResponse(handle, cachedResponse);
    if (newResponse != cachedResponse)
        return newResponse;
#endif

    CacheStoragePolicy policy = static_cast<CacheStoragePolicy>(CFCachedURLResponseGetStoragePolicy(cachedResponse));

    if (handle->client())
        handle->client()->willCacheResponse(handle, policy);

    if (static_cast<CFURLCacheStoragePolicy>(policy) != CFCachedURLResponseGetStoragePolicy(cachedResponse)) {
#if HAVE(NETWORK_CFDATA_ARRAY_CALLBACK)
        RetainPtr<CFArrayRef> receiverData(AdoptCF, CFCachedURLResponseCopyReceiverDataArray(cachedResponse));
        cachedResponse = CFCachedURLResponseCreateWithDataArray(kCFAllocatorDefault,
                                                                wrappedResponse,
                                                                receiverData.get(),
                                                                CFCachedURLResponseGetUserInfo(cachedResponse),
                                                                static_cast<CFURLCacheStoragePolicy>(policy));
#else
        cachedResponse = CFCachedURLResponseCreateWithUserInfo(kCFAllocatorDefault, 
                                                               wrappedResponse,
                                                               CFCachedURLResponseGetReceiverData(cachedResponse),
                                                               CFCachedURLResponseGetUserInfo(cachedResponse), 
                                                               static_cast<CFURLCacheStoragePolicy>(policy));
#endif
    } else
        CFRetain(cachedResponse);

    return cachedResponse;
}

static void didReceiveChallenge(CFURLConnectionRef conn, CFURLAuthChallengeRef challenge, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    ASSERT(handle);
    LOG(Network, "CFNet - didReceiveChallenge(conn=%p, handle=%p (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    handle->didReceiveAuthenticationChallenge(AuthenticationChallenge(challenge, handle));
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
static Boolean canRespondToProtectionSpace(CFURLConnectionRef conn, CFURLProtectionSpaceRef protectionSpace, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    ASSERT(handle);

    LOG(Network, "CFNet - canRespondToProtectionSpace(conn=%p, handle=%p (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    return handle->canAuthenticateAgainstProtectionSpace(core(protectionSpace));
}
#endif

ResourceHandleInternal::~ResourceHandleInternal()
{
    if (m_connection) {
        LOG(Network, "CFNet - Cancelling connection %p (%s)", m_connection.get(), m_firstRequest.url().string().utf8().data());
        CFURLConnectionCancel(m_connection.get());
    }
}

ResourceHandle::~ResourceHandle()
{
    LOG(Network, "CFNet - Destroying job %p (%s)", this, d->m_firstRequest.url().string().utf8().data());
}

static CFURLRequestRef makeFinalRequest(const ResourceRequest& request, bool shouldContentSniff)
{
    CFMutableURLRequestRef newRequest = CFURLRequestCreateMutableCopy(kCFAllocatorDefault, request.cfURLRequest());
#if USE(CFURLSTORAGESESSIONS)
    wkSetRequestStorageSession(ResourceHandle::currentStorageSession(), newRequest);
#endif
    
    if (!shouldContentSniff)
        wkSetCFURLRequestShouldContentSniff(newRequest, false);

    RetainPtr<CFMutableDictionaryRef> sslProps;

    if (allowsAnyHTTPSCertificateHosts().contains(request.url().host().lower())) {
        sslProps.adoptCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLAllowsAnyRoot, kCFBooleanTrue);
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLAllowsExpiredRoots, kCFBooleanTrue);
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLAllowsExpiredCertificates, kCFBooleanTrue);
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLValidatesCertificateChain, kCFBooleanFalse);
    }

    HashMap<String, RetainPtr<CFDataRef> >::iterator clientCert = clientCerts().find(request.url().host().lower());
    if (clientCert != clientCerts().end()) {
        if (!sslProps)
            sslProps.adoptCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
#if PLATFORM(WIN)
        wkSetClientCertificateInSSLProperties(sslProps.get(), (clientCert->second).get());
#endif
    }

    if (sslProps)
        CFURLRequestSetSSLProperties(newRequest, sslProps.get());

    if (RetainPtr<CFHTTPCookieStorageRef> cookieStorage = currentCFHTTPCookieStorage()) {
        CFURLRequestSetHTTPCookieStorage(newRequest, cookieStorage.get());
        CFHTTPCookieStorageAcceptPolicy policy = CFHTTPCookieStorageGetCookieAcceptPolicy(cookieStorage.get());
        CFURLRequestSetHTTPCookieStorageAcceptPolicy(newRequest, policy);

        // If a URL already has cookies, then we'll relax the 3rd party cookie policy and accept new cookies.
        if (policy == CFHTTPCookieStorageAcceptPolicyOnlyFromMainDocumentDomain) {
            CFURLRef url = CFURLRequestGetURL(newRequest);
            RetainPtr<CFArrayRef> cookies(AdoptCF, CFHTTPCookieStorageCopyCookiesForURL(cookieStorage.get(), url, false));
            if (CFArrayGetCount(cookies.get()))
                CFURLRequestSetMainDocumentURL(newRequest, url);
        }
    }

    return newRequest;
}

static CFDictionaryRef createConnectionProperties(bool shouldUseCredentialStorage)
{
    static const CFStringRef webKitPrivateSessionCF = CFSTR("WebKitPrivateSession");
    static const CFStringRef _kCFURLConnectionSessionID = CFSTR("_kCFURLConnectionSessionID");
    static const CFStringRef kCFURLConnectionSocketStreamProperties = CFSTR("kCFURLConnectionSocketStreamProperties");

    CFDictionaryRef sessionID = shouldUseCredentialStorage ?
        CFDictionaryCreate(0, 0, 0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks) :
        CFDictionaryCreate(0, (const void**)&_kCFURLConnectionSessionID, (const void**)&webKitPrivateSessionCF, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFDictionaryRef propertiesDictionary = CFDictionaryCreate(0, (const void**)&kCFURLConnectionSocketStreamProperties, (const void**)&sessionID, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFRelease(sessionID);
    return propertiesDictionary;
}

void ResourceHandle::createCFURLConnection(bool shouldUseCredentialStorage, bool shouldContentSniff)
{
    if ((!d->m_user.isEmpty() || !d->m_pass.isEmpty()) && !firstRequest().url().protocolIsInHTTPFamily()) {
        // Credentials for ftp can only be passed in URL, the didReceiveAuthenticationChallenge delegate call won't be made.
        KURL urlWithCredentials(firstRequest().url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        firstRequest().setURL(urlWithCredentials);
    }

    // <rdar://problem/7174050> - For URLs that match the paths of those previously challenged for HTTP Basic authentication, 
    // try and reuse the credential preemptively, as allowed by RFC 2617.
    if (shouldUseCredentialStorage && firstRequest().url().protocolIsInHTTPFamily()) {
        if (d->m_user.isEmpty() && d->m_pass.isEmpty()) {
            // <rdar://problem/7174050> - For URLs that match the paths of those previously challenged for HTTP Basic authentication, 
            // try and reuse the credential preemptively, as allowed by RFC 2617.
            d->m_initialCredential = CredentialStorage::get(firstRequest().url());
        } else {
            // If there is already a protection space known for the URL, update stored credentials before sending a request.
            // This makes it possible to implement logout by sending an XMLHttpRequest with known incorrect credentials, and aborting it immediately
            // (so that an authentication dialog doesn't pop up).
            CredentialStorage::set(Credential(d->m_user, d->m_pass, CredentialPersistenceNone), firstRequest().url());
        }
    }
        
    if (!d->m_initialCredential.isEmpty()) {
        // FIXME: Support Digest authentication, and Proxy-Authorization.
        applyBasicAuthorizationHeader(firstRequest(), d->m_initialCredential);
    }

    RetainPtr<CFURLRequestRef> request(AdoptCF, makeFinalRequest(firstRequest(), shouldContentSniff));

#if HAVE(NETWORK_CFDATA_ARRAY_CALLBACK) && USE(PROTECTION_SPACE_AUTH_CALLBACK)
    CFURLConnectionClient_V6 client = { 6, this, 0, 0, 0, WebCore::willSendRequest, didReceiveResponse, didReceiveData, 0, didFinishLoading, didFail, willCacheResponse, didReceiveChallenge, didSendBodyData, shouldUseCredentialStorageCallback, 0, canRespondToProtectionSpace, 0, didReceiveDataArray};
#else
    CFURLConnectionClient_V3 client = { 3, this, 0, 0, 0, WebCore::willSendRequest, didReceiveResponse, didReceiveData, 0, didFinishLoading, didFail, willCacheResponse, didReceiveChallenge, didSendBodyData, shouldUseCredentialStorageCallback, 0};
#endif
    RetainPtr<CFDictionaryRef> connectionProperties(AdoptCF, createConnectionProperties(shouldUseCredentialStorage));

    d->m_connection.adoptCF(CFURLConnectionCreateWithProperties(0, request.get(), reinterpret_cast<CFURLConnectionClient*>(&client), connectionProperties.get()));
}

bool ResourceHandle::start(NetworkingContext* context)
{
    if (!context)
        return false;

    // If NetworkingContext is invalid then we are no longer attached to a Page,
    // this must be an attempted load from an unload handler, so let's just block it.
    if (!context->isValid())
        return false;

    bool shouldUseCredentialStorage = !client() || client()->shouldUseCredentialStorage(this);

    createCFURLConnection(shouldUseCredentialStorage, d->m_shouldContentSniff);

#if PLATFORM(WIN)
    CFURLConnectionScheduleWithCurrentMessageQueue(d->m_connection.get());
#else
    CFURLConnectionScheduleWithRunLoop(d->m_connection.get(), CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
#endif
    CFURLConnectionScheduleDownloadWithRunLoop(d->m_connection.get(), loaderRunLoop(), kCFRunLoopDefaultMode);
    CFURLConnectionStart(d->m_connection.get());

    LOG(Network, "CFNet - Starting URL %s (handle=%p, conn=%p)", firstRequest().url().string().utf8().data(), this, d->m_connection.get());

    return true;
}

void ResourceHandle::cancel()
{
    if (d->m_connection) {
        CFURLConnectionCancel(d->m_connection.get());
        d->m_connection = 0;
    }
}

void ResourceHandle::willSendRequest(ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    const KURL& url = request.url();
    d->m_user = url.user();
    d->m_pass = url.pass();
    d->m_lastHTTPMethod = request.httpMethod();
    request.removeCredentials();

    if (!protocolHostAndPortAreEqual(request.url(), redirectResponse.url())) {
        // If the network layer carries over authentication headers from the original request
        // in a cross-origin redirect, we want to clear those headers here.
        request.clearHTTPAuthorization();
    } else {
        // Only consider applying authentication credentials if this is actually a redirect and the redirect
        // URL didn't include credentials of its own.
        if (d->m_user.isEmpty() && d->m_pass.isEmpty() && !redirectResponse.isNull()) {
            Credential credential = CredentialStorage::get(request.url());
            if (!credential.isEmpty()) {
                d->m_initialCredential = credential;
                
                // FIXME: Support Digest authentication, and Proxy-Authorization.
                applyBasicAuthorizationHeader(request, d->m_initialCredential);
            }
        }
    }

#if USE(CFURLSTORAGESESSIONS)
     request.setStorageSession(ResourceHandle::currentStorageSession());
#endif

    client()->willSendRequest(this, request, redirectResponse);
}

bool ResourceHandle::shouldUseCredentialStorage()
{
    LOG(Network, "CFNet - shouldUseCredentialStorage()");
    if (client())
        return client()->shouldUseCredentialStorage(this);

    return false;
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    LOG(Network, "CFNet - didReceiveAuthenticationChallenge()");
    ASSERT(d->m_currentWebChallenge.isNull());
    // Since CFURLConnection networking relies on keeping a reference to the original CFURLAuthChallengeRef,
    // we make sure that is actually present
    ASSERT(challenge.cfURLAuthChallengeRef());
    ASSERT(challenge.authenticationClient() == this); // Should be already set.

#if !PLATFORM(WIN)
    // Proxy authentication is handled by CFNetwork internally. We can get here if the user cancels
    // CFNetwork authentication dialog, and we shouldn't ask the client to display another one in that case.
    if (challenge.protectionSpace().isProxy()) {
        // Cannot use receivedRequestToContinueWithoutCredential(), because current challenge is not yet set.
        CFURLConnectionUseCredential(d->m_connection.get(), 0, challenge.cfURLAuthChallengeRef());
        return;
    }
#endif

    if (!d->m_user.isNull() && !d->m_pass.isNull()) {
        RetainPtr<CFStringRef> user(AdoptCF, d->m_user.createCFString());
        RetainPtr<CFStringRef> pass(AdoptCF, d->m_pass.createCFString());
        RetainPtr<CFURLCredentialRef> credential(AdoptCF,
            CFURLCredentialCreate(kCFAllocatorDefault, user.get(), pass.get(), 0, kCFURLCredentialPersistenceNone));
        
        KURL urlToStore;
        if (challenge.failureResponse().httpStatusCode() == 401)
            urlToStore = challenge.failureResponse().url();
        CredentialStorage::set(core(credential.get()), challenge.protectionSpace(), urlToStore);
        
        CFURLConnectionUseCredential(d->m_connection.get(), credential.get(), challenge.cfURLAuthChallengeRef());
        d->m_user = String();
        d->m_pass = String();
        // FIXME: Per the specification, the user shouldn't be asked for credentials if there were incorrect ones provided explicitly.
        return;
    }

    if (!client() || client()->shouldUseCredentialStorage(this)) {
        if (!d->m_initialCredential.isEmpty() || challenge.previousFailureCount()) {
            // The stored credential wasn't accepted, stop using it.
            // There is a race condition here, since a different credential might have already been stored by another ResourceHandle,
            // but the observable effect should be very minor, if any.
            CredentialStorage::remove(challenge.protectionSpace());
        }

        if (!challenge.previousFailureCount()) {
            Credential credential = CredentialStorage::get(challenge.protectionSpace());
            if (!credential.isEmpty() && credential != d->m_initialCredential) {
                ASSERT(credential.persistence() == CredentialPersistenceNone);
                if (challenge.failureResponse().httpStatusCode() == 401) {
                    // Store the credential back, possibly adding it as a default for this directory.
                    CredentialStorage::set(credential, challenge.protectionSpace(), challenge.failureResponse().url());
                }
                RetainPtr<CFURLCredentialRef> cfCredential(AdoptCF, createCF(credential));
                CFURLConnectionUseCredential(d->m_connection.get(), cfCredential.get(), challenge.cfURLAuthChallengeRef());
                return;
            }
        }
    }

    d->m_currentWebChallenge = challenge;
    
    if (client())
        client()->didReceiveAuthenticationChallenge(this, d->m_currentWebChallenge);
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
bool ResourceHandle::canAuthenticateAgainstProtectionSpace(const ProtectionSpace& protectionSpace)
{
    if (client())
        return client()->canAuthenticateAgainstProtectionSpace(this, protectionSpace);

    return false;
}
#endif

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
    LOG(Network, "CFNet - receivedCredential()");
    ASSERT(!challenge.isNull());
    ASSERT(challenge.cfURLAuthChallengeRef());
    if (challenge != d->m_currentWebChallenge)
        return;

    // FIXME: Support empty credentials. Currently, an empty credential cannot be stored in WebCore credential storage, as that's empty value for its map.
    if (credential.isEmpty()) {
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    if (credential.persistence() == CredentialPersistenceForSession) {
        // Manage per-session credentials internally, because once NSURLCredentialPersistencePerSession is used, there is no way
        // to ignore it for a particular request (short of removing it altogether).
        Credential webCredential(credential.user(), credential.password(), CredentialPersistenceNone);
        RetainPtr<CFURLCredentialRef> cfCredential(AdoptCF, createCF(webCredential));
        
        KURL urlToStore;
        if (challenge.failureResponse().httpStatusCode() == 401)
            urlToStore = challenge.failureResponse().url();      
        CredentialStorage::set(webCredential, challenge.protectionSpace(), urlToStore);

        CFURLConnectionUseCredential(d->m_connection.get(), cfCredential.get(), challenge.cfURLAuthChallengeRef());
    } else {
        RetainPtr<CFURLCredentialRef> cfCredential(AdoptCF, createCF(credential));
        CFURLConnectionUseCredential(d->m_connection.get(), cfCredential.get(), challenge.cfURLAuthChallengeRef());
    }

    clearAuthentication();
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    LOG(Network, "CFNet - receivedRequestToContinueWithoutCredential()");
    ASSERT(!challenge.isNull());
    ASSERT(challenge.cfURLAuthChallengeRef());
    if (challenge != d->m_currentWebChallenge)
        return;

    CFURLConnectionUseCredential(d->m_connection.get(), 0, challenge.cfURLAuthChallengeRef());

    clearAuthentication();
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    LOG(Network, "CFNet - receivedCancellation()");
    if (challenge != d->m_currentWebChallenge)
        return;

    if (client())
        client()->receivedCancellation(this, challenge);
}

CFURLConnectionRef ResourceHandle::connection() const
{
    return d->m_connection.get();
}

CFURLConnectionRef ResourceHandle::releaseConnectionForDownload()
{
    LOG(Network, "CFNet - Job %p releasing connection %p for download", this, d->m_connection.get());
    return d->m_connection.leakRef();
}

CFStringRef ResourceHandle::synchronousLoadRunLoopMode()
{
    return CFSTR("WebCoreSynchronousLoaderRunLoopMode");
}

void ResourceHandle::loadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials storedCredentials, ResourceError& error, ResourceResponse& response, Vector<char>& vector)
{
    LOG(Network, "ResourceHandle::loadResourceSynchronously:%s allowStoredCredentials:%u", request.url().string().utf8().data(), storedCredentials);

    ASSERT(!request.isEmpty());

    ASSERT(response.isNull());
    ASSERT(error.isNull());

    OwnPtr<WebCoreSynchronousLoaderClient> client = WebCoreSynchronousLoaderClient::create(response, error);
    client->setAllowStoredCredentials(storedCredentials == AllowStoredCredentials);

    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(request, client.get(), false /*defersLoading*/, true /*shouldContentSniff*/));

    if (context && handle->d->m_scheduledFailureType != NoFailure) {
        error = context->blockedError(request);
        return;
    }

    handle->createCFURLConnection(storedCredentials == AllowStoredCredentials, ResourceHandle::shouldContentSniffURL(request.url()));

    CFURLConnectionScheduleWithRunLoop(handle->connection(), CFRunLoopGetCurrent(), synchronousLoadRunLoopMode());
    CFURLConnectionScheduleDownloadWithRunLoop(handle->connection(), CFRunLoopGetCurrent(), synchronousLoadRunLoopMode());
    CFURLConnectionStart(handle->connection());

    while (!client->isDone())
        CFRunLoopRunInMode(synchronousLoadRunLoopMode(), UINT_MAX, true);

    CFURLConnectionCancel(handle->connection());
    
    if (error.isNull() && response.mimeType().isNull())
        setDefaultMIMEType(response.cfURLResponse());

    RetainPtr<CFDataRef> data = client->data();
    
    if (!error.isNull()) {
        response = ResourceResponse(request.url(), String(), 0, String(), String());

        CFErrorRef cfError = error;
        CFStringRef domain = CFErrorGetDomain(cfError);
        // FIXME: Return the actual response for failed authentication.
        if (domain == kCFErrorDomainCFNetwork)
            response.setHTTPStatusCode(CFErrorGetCode(cfError));
        else
            response.setHTTPStatusCode(404);
    }

    if (data) {
        ASSERT(vector.isEmpty());
        vector.append(CFDataGetBytePtr(data.get()), CFDataGetLength(data.get()));
    }
}

void ResourceHandle::setHostAllowsAnyHTTPSCertificate(const String& host)
{
    allowsAnyHTTPSCertificateHosts().add(host.lower());
}

void ResourceHandle::setClientCertificate(const String& host, CFDataRef cert)
{
    clientCerts().set(host.lower(), cert);
}

void ResourceHandle::platformSetDefersLoading(bool defers)
{
    if (!d->m_connection)
        return;

    if (defers)
        CFURLConnectionHalt(d->m_connection.get());
    else
        CFURLConnectionResume(d->m_connection.get());
}

bool ResourceHandle::loadsBlocked()
{
    return false;
}

bool ResourceHandle::willLoadFromCache(ResourceRequest& request, Frame*)
{
    request.setCachePolicy(ReturnCacheDataDontLoad);
    
    CFURLResponseRef cfResponse = 0;
    CFErrorRef cfError = 0;
    RetainPtr<CFURLRequestRef> cfRequest(AdoptCF, makeFinalRequest(request, true));
    RetainPtr<CFDataRef> data(AdoptCF, CFURLConnectionSendSynchronousRequest(cfRequest.get(), &cfResponse, &cfError, request.timeoutInterval()));
    bool cached = cfResponse && !cfError;

    if (cfError)
        CFRelease(cfError);
    if (cfResponse)
        CFRelease(cfResponse);

    return cached;
}

#if PLATFORM(MAC)
void ResourceHandle::schedule(SchedulePair* pair)
{
    CFRunLoopRef runLoop = pair->runLoop();
    if (!runLoop)
        return;

    CFURLConnectionScheduleWithRunLoop(d->m_connection.get(), runLoop, pair->mode());
    if (d->m_startWhenScheduled) {
        CFURLConnectionStart(d->m_connection.get());
        d->m_startWhenScheduled = false;
    }
}

void ResourceHandle::unschedule(SchedulePair* pair)
{
    CFRunLoopRef runLoop = pair->runLoop();
    if (!runLoop)
        return;

    CFURLConnectionUnscheduleFromRunLoop(d->m_connection.get(), runLoop, pair->mode());
}
#endif

void WebCoreSynchronousLoaderClient::willSendRequest(ResourceHandle* handle, ResourceRequest& request, const ResourceResponse& /*redirectResponse*/)
{
    // FIXME: This needs to be fixed to follow the redirect correctly even for cross-domain requests.
    if (!protocolHostAndPortAreEqual(handle->firstRequest().url(), request.url())) {
        ASSERT(!m_error.cfError());
        RetainPtr<CFErrorRef> cfError(AdoptCF, CFErrorCreate(kCFAllocatorDefault, kCFErrorDomainCFNetwork, kCFURLErrorBadServerResponse, 0));
        m_error = cfError.get();
        m_isDone = true;
        CFURLRequestRef nullRequest = 0;
        request = nullRequest;
        return;
    }
}
void WebCoreSynchronousLoaderClient::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    m_response = response;
}

void WebCoreSynchronousLoaderClient::didReceiveData(ResourceHandle*, const char* data, int length, int /*encodedDataLength*/)
{
    if (!m_data)
        m_data.adoptCF(CFDataCreateMutable(kCFAllocatorDefault, 0));
    CFDataAppendBytes(m_data.get(), reinterpret_cast<const UInt8*>(data), length);
}

void WebCoreSynchronousLoaderClient::didFinishLoading(ResourceHandle*, double)
{
    m_isDone = true;
}

void WebCoreSynchronousLoaderClient::didFail(ResourceHandle*, const ResourceError& error)
{
    m_error = error;
    m_isDone = true;
}

void WebCoreSynchronousLoaderClient::didReceiveAuthenticationChallenge(ResourceHandle* handle, const AuthenticationChallenge& challenge)
{
    // FIXME: The user should be asked for credentials, as in async case.
    CFURLConnectionUseCredential(handle->connection(), 0, challenge.cfURLAuthChallengeRef());
}

bool WebCoreSynchronousLoaderClient::shouldUseCredentialStorage(ResourceHandle*)
{
    // FIXME: We should ask FrameLoaderClient whether using credential storage is globally forbidden.
    return m_allowStoredCredentials;
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
bool WebCoreSynchronousLoaderClient::canAuthenticateAgainstProtectionSpace(ResourceHandle*, const ProtectionSpace&)
{
    // FIXME: We should ask FrameLoaderClient. <http://webkit.org/b/65196>
    return true;
}
#endif

#endif // USE(CFNETWORK)

#if HAVE(NETWORK_CFDATA_ARRAY_CALLBACK)
void ResourceHandle::handleDataArray(CFArrayRef dataArray)
{
    ASSERT(client());
    if (client()->supportsDataArray()) {
        client()->didReceiveDataArray(this, dataArray);
        return;
    }

    CFIndex count = CFArrayGetCount(dataArray);
    ASSERT(count);
    if (count == 1) {
        CFDataRef data = static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, 0));
        CFIndex length = CFDataGetLength(data);
        client()->didReceiveData(this, reinterpret_cast<const char*>(CFDataGetBytePtr(data)), length, static_cast<int>(length));
        return;
    }

    CFIndex totalSize = 0;
    CFIndex index;
    for (index = 0; index < count; index++)
        totalSize += CFDataGetLength(static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, index)));

    RetainPtr<CFMutableDataRef> mergedData(AdoptCF, CFDataCreateMutable(kCFAllocatorDefault, totalSize));
    for (index = 0; index < count; index++) {
        CFDataRef data = static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, index));
        CFDataAppendBytes(mergedData.get(), CFDataGetBytePtr(data), CFDataGetLength(data));
    }

    client()->didReceiveData(this, reinterpret_cast<const char*>(CFDataGetBytePtr(mergedData.get())), totalSize, static_cast<int>(totalSize));
}
#endif

#if USE(CFURLSTORAGESESSIONS)

RetainPtr<CFURLStorageSessionRef> ResourceHandle::createPrivateBrowsingStorageSession(CFStringRef identifier)
{
#if PLATFORM(WIN)
    return RetainPtr<CFURLStorageSessionRef>(AdoptCF, wkCreatePrivateStorageSession(identifier, defaultStorageSession()));
#else
    return RetainPtr<CFURLStorageSessionRef>(AdoptCF, wkCreatePrivateStorageSession(identifier));
#endif
}

CFURLStorageSessionRef ResourceHandle::currentStorageSession()
{
    if (CFURLStorageSessionRef privateStorageSession = privateBrowsingStorageSession())
        return privateStorageSession;
    return defaultStorageSession();
}

static RetainPtr<CFURLStorageSessionRef>& defaultCFURLStorageSession()
{
    DEFINE_STATIC_LOCAL(RetainPtr<CFURLStorageSessionRef>, storageSession, ());
    return storageSession;
}

void ResourceHandle::setDefaultStorageSession(CFURLStorageSessionRef storageSession)
{
    defaultCFURLStorageSession() = storageSession;
}

CFURLStorageSessionRef ResourceHandle::defaultStorageSession()
{
    return defaultCFURLStorageSession().get();
}

static RetainPtr<CFURLStorageSessionRef>& privateStorageSession()
{
    DEFINE_STATIC_LOCAL(RetainPtr<CFURLStorageSessionRef>, storageSession, ());
    return storageSession;
}

static String& privateBrowsingStorageSessionIdentifierBase()
{
    DEFINE_STATIC_LOCAL(String, base, ());
    return base;
}

void ResourceHandle::setPrivateBrowsingEnabled(bool enabled)
{
    if (!enabled) {
        privateStorageSession() = nullptr;
        return;
    }

    if (privateStorageSession())
        return;

    String base = privateBrowsingStorageSessionIdentifierBase().isNull() ? privateBrowsingStorageSessionIdentifierDefaultBase() : privateBrowsingStorageSessionIdentifierBase();
    RetainPtr<CFStringRef> cfIdentifier(AdoptCF, String(base + ".PrivateBrowsing").createCFString());

    privateStorageSession() = createPrivateBrowsingStorageSession(cfIdentifier.get());
}

CFURLStorageSessionRef ResourceHandle::privateBrowsingStorageSession()
{
    return privateStorageSession().get();
}

void ResourceHandle::setPrivateBrowsingStorageSessionIdentifierBase(const String& identifier)
{
    privateBrowsingStorageSessionIdentifierBase() = identifier;
}

#if PLATFORM(WIN) || USE(CFNETWORK)

String ResourceHandle::privateBrowsingStorageSessionIdentifierDefaultBase()
{
    return String(reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), kCFBundleIdentifierKey)));
}

#endif // PLATFORM(WIN)

#endif // USE(CFURLSTORAGESESSIONS)

} // namespace WebCore

