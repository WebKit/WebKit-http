/*
 * Copyright (C) 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SocketStreamHandle.h"

#include "Credential.h"
#include "CredentialStorage.h"
#include "Logging.h"
#include "ProtectionSpace.h"
#include "SocketStreamError.h"
#include "SocketStreamHandleClient.h"
#include <wtf/MainThread.h>
#include <wtf/text/WTFString.h>

#ifdef BUILDING_ON_LEOPARD
#include <SystemConfiguration/SystemConfiguration.h>
#endif

#if PLATFORM(WIN)
#include "LoaderRunLoopCF.h"
#include <CFNetwork/CFNetwork.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#else
#include "WebCoreSystemInterface.h"
#endif


namespace WebCore {

SocketStreamHandle::SocketStreamHandle(const KURL& url, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(url, client)
    , m_connectingSubstate(New)
    , m_connectionType(Unknown)
    , m_sentStoredCredentials(false)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);

    ASSERT(url.protocolIs("ws") || url.protocolIs("wss"));

    KURL httpsURL(KURL(), "https://" + m_url.host());
    m_httpsURL.adoptCF(httpsURL.createCFURL());

    createStreams();
    ASSERT(!m_readStream == !m_writeStream);
    if (!m_readStream) // Doing asynchronous PAC file processing, streams will be created later.
        return;

    scheduleStreams();
}

void SocketStreamHandle::scheduleStreams()
{
    ASSERT(m_readStream);
    ASSERT(m_writeStream);

    CFStreamClientContext clientContext = { 0, this, retainSocketStreamHandle, releaseSocketStreamHandle, copyCFStreamDescription };
    // FIXME: Pass specific events we're interested in instead of -1.
    CFReadStreamSetClient(m_readStream.get(), static_cast<CFOptionFlags>(-1), readStreamCallback, &clientContext);
    CFWriteStreamSetClient(m_writeStream.get(), static_cast<CFOptionFlags>(-1), writeStreamCallback, &clientContext);

#if PLATFORM(WIN)
    CFReadStreamScheduleWithRunLoop(m_readStream.get(), loaderRunLoop(), kCFRunLoopDefaultMode);
    CFWriteStreamScheduleWithRunLoop(m_writeStream.get(), loaderRunLoop(), kCFRunLoopDefaultMode);
#else
    CFReadStreamScheduleWithRunLoop(m_readStream.get(), CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
    CFWriteStreamScheduleWithRunLoop(m_writeStream.get(), CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
#endif

    CFReadStreamOpen(m_readStream.get());
    CFWriteStreamOpen(m_writeStream.get());

    if (m_pacRunLoopSource)
        removePACRunLoopSource();

    m_connectingSubstate = WaitingForConnect;
}

void* SocketStreamHandle::retainSocketStreamHandle(void* info)
{
    SocketStreamHandle* handle = static_cast<SocketStreamHandle*>(info);
    handle->ref();
    return handle;
}

void SocketStreamHandle::releaseSocketStreamHandle(void* info)
{
    SocketStreamHandle* handle = static_cast<SocketStreamHandle*>(info);
    handle->deref();
}

CFStringRef SocketStreamHandle::copyPACExecutionDescription(void*)
{
    return CFSTR("WebSocket proxy PAC file execution");
}

struct MainThreadPACCallbackInfo {
    MainThreadPACCallbackInfo(SocketStreamHandle* handle, CFArrayRef proxyList) : handle(handle), proxyList(proxyList) { }
    RefPtr<SocketStreamHandle> handle;
    CFArrayRef proxyList;
};

void SocketStreamHandle::pacExecutionCallback(void* client, CFArrayRef proxyList, CFErrorRef)
{
    SocketStreamHandle* handle = static_cast<SocketStreamHandle*>(client);
    MainThreadPACCallbackInfo info(handle, proxyList);
    // If we're already on main thread (e.g. on Mac), callOnMainThreadAndWait() will be just a function call.
    callOnMainThreadAndWait(pacExecutionCallbackMainThread, &info);
}

void SocketStreamHandle::pacExecutionCallbackMainThread(void* invocation)
{
    MainThreadPACCallbackInfo* info = static_cast<MainThreadPACCallbackInfo*>(invocation);
    ASSERT(info->handle->m_connectingSubstate == ExecutingPACFile);
    // This time, the array won't have PAC as a first entry.
    if (info->handle->m_state != Connecting)
        return;
    info->handle->chooseProxyFromArray(info->proxyList);
    info->handle->createStreams();
    info->handle->scheduleStreams();
}

void SocketStreamHandle::executePACFileURL(CFURLRef pacFileURL)
{
    // CFNetwork returns an empty proxy array for WebScoket schemes, so use m_httpsURL.
    CFStreamClientContext clientContext = { 0, this, retainSocketStreamHandle, releaseSocketStreamHandle, copyPACExecutionDescription };
    m_pacRunLoopSource.adoptCF(CFNetworkExecuteProxyAutoConfigurationURL(pacFileURL, m_httpsURL.get(), pacExecutionCallback, &clientContext));
#if PLATFORM(WIN)
    CFRunLoopAddSource(loaderRunLoop(), m_pacRunLoopSource.get(), kCFRunLoopDefaultMode);
#else
    CFRunLoopAddSource(CFRunLoopGetCurrent(), m_pacRunLoopSource.get(), kCFRunLoopCommonModes);
#endif
    m_connectingSubstate = ExecutingPACFile;
}

void SocketStreamHandle::removePACRunLoopSource()
{
    ASSERT(m_pacRunLoopSource);

    CFRunLoopSourceInvalidate(m_pacRunLoopSource.get());
#if PLATFORM(WIN)
    CFRunLoopRemoveSource(loaderRunLoop(), m_pacRunLoopSource.get(), kCFRunLoopDefaultMode);
#else
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_pacRunLoopSource.get(), kCFRunLoopCommonModes);
#endif
    m_pacRunLoopSource = 0;
}

void SocketStreamHandle::chooseProxy()
{
#ifndef BUILDING_ON_LEOPARD
    RetainPtr<CFDictionaryRef> proxyDictionary(AdoptCF, CFNetworkCopySystemProxySettings());
#else
    // We don't need proxy information often, so there is no need to set up a permanent dynamic store session.
    RetainPtr<CFDictionaryRef> proxyDictionary(AdoptCF, SCDynamicStoreCopyProxies(0));
#endif

    // SOCKS or HTTPS (AKA CONNECT) proxies are supported.
    // WebSocket protocol relies on handshake being transferred unchanged, so we need a proxy that will not modify headers.
    // Since HTTP proxies must add Via headers, they are highly unlikely to work.
    // Many CONNECT proxies limit connectivity to port 443, so we prefer SOCKS, if configured.

    if (!proxyDictionary) {
        m_connectionType = Direct;
        return;
    }

    // CFNetworkCopyProxiesForURL doesn't know about WebSocket schemes, so pretend to use http.
    // Always use "https" to get HTTPS proxies in result - we'll try to use those for ws:// even though many are configured to reject connections to ports other than 443.
    RetainPtr<CFArrayRef> proxyArray(AdoptCF, CFNetworkCopyProxiesForURL(m_httpsURL.get(), proxyDictionary.get()));

    chooseProxyFromArray(proxyArray.get());
}

void SocketStreamHandle::chooseProxyFromArray(CFArrayRef proxyArray)
{
    if (!proxyArray) {
        m_connectionType = Direct;
        return;
    }

    CFIndex proxyArrayCount = CFArrayGetCount(proxyArray);

    // PAC is always the first entry, if present.
    if (proxyArrayCount) {
        CFDictionaryRef proxyInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(proxyArray, 0));
        CFTypeRef proxyType = CFDictionaryGetValue(proxyInfo, kCFProxyTypeKey);
        if (proxyType && CFGetTypeID(proxyType) == CFStringGetTypeID()) {
            if (CFEqual(proxyType, kCFProxyTypeAutoConfigurationURL)) {
                CFTypeRef pacFileURL = CFDictionaryGetValue(proxyInfo, kCFProxyAutoConfigurationURLKey);
                if (pacFileURL && CFGetTypeID(pacFileURL) == CFURLGetTypeID()) {
                    executePACFileURL(static_cast<CFURLRef>(pacFileURL));
                    return;
                }
            }
        }
    }

    CFDictionaryRef chosenProxy = 0;
    for (CFIndex i = 0; i < proxyArrayCount; ++i) {
        CFDictionaryRef proxyInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(proxyArray, i));
        CFTypeRef proxyType = CFDictionaryGetValue(proxyInfo, kCFProxyTypeKey);
        if (proxyType && CFGetTypeID(proxyType) == CFStringGetTypeID()) {
            if (CFEqual(proxyType, kCFProxyTypeSOCKS)) {
                m_connectionType = SOCKSProxy;
                chosenProxy = proxyInfo;
                break;
            }
            if (CFEqual(proxyType, kCFProxyTypeHTTPS)) {
                m_connectionType = CONNECTProxy;
                chosenProxy = proxyInfo;
                // Keep looking for proxies, as a SOCKS one is preferable.
            }
        }
    }

    if (chosenProxy) {
        ASSERT(m_connectionType != Unknown);
        ASSERT(m_connectionType != Direct);

        CFTypeRef proxyHost = CFDictionaryGetValue(chosenProxy, kCFProxyHostNameKey);
        CFTypeRef proxyPort = CFDictionaryGetValue(chosenProxy, kCFProxyPortNumberKey);

        if (proxyHost && CFGetTypeID(proxyHost) == CFStringGetTypeID() && proxyPort && CFGetTypeID(proxyPort) == CFNumberGetTypeID()) {
            m_proxyHost = static_cast<CFStringRef>(proxyHost);
            m_proxyPort = static_cast<CFNumberRef>(proxyPort);
            return;
        }
    }

    m_connectionType = Direct;
}


void SocketStreamHandle::createStreams()
{
    if (m_connectionType == Unknown)
        chooseProxy();

    // If it's still unknown, then we're resolving a PAC file asynchronously.
    if (m_connectionType == Unknown)
        return;

    RetainPtr<CFStringRef> host(AdoptCF, m_url.host().createCFString());

    // Creating streams to final destination, not to proxy.
    CFReadStreamRef readStream = 0;
    CFWriteStreamRef writeStream = 0;
    CFStreamCreatePairWithSocketToHost(0, host.get(), port(), &readStream, &writeStream);

    m_readStream.adoptCF(readStream);
    m_writeStream.adoptCF(writeStream);

    switch (m_connectionType) {
    case Unknown:
        ASSERT_NOT_REACHED();
        break;
    case Direct:
        break;
    case SOCKSProxy: {
        // FIXME: SOCKS5 doesn't do challenge-response, should we try to apply credentials from Keychain right away?
        // But SOCKS5 credentials don't work at the time of this writing anyway, see <rdar://6776698>.
        const void* proxyKeys[] = { kCFStreamPropertySOCKSProxyHost, kCFStreamPropertySOCKSProxyPort };
        const void* proxyValues[] = { m_proxyHost.get(), m_proxyPort.get() };
        RetainPtr<CFDictionaryRef> connectDictionary(AdoptCF, CFDictionaryCreate(0, proxyKeys, proxyValues, WTF_ARRAY_LENGTH(proxyKeys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        CFReadStreamSetProperty(m_readStream.get(), kCFStreamPropertySOCKSProxy, connectDictionary.get());
        break;
        }
    case CONNECTProxy:
        wkSetCONNECTProxyForStream(m_readStream.get(), m_proxyHost.get(), m_proxyPort.get());
        break;
    }

    if (shouldUseSSL()) {
        const void* keys[] = { kCFStreamSSLPeerName, kCFStreamSSLLevel };
        const void* values[] = { host.get(), kCFStreamSocketSecurityLevelNegotiatedSSL };
        RetainPtr<CFDictionaryRef> settings(AdoptCF, CFDictionaryCreate(0, keys, values, WTF_ARRAY_LENGTH(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        CFReadStreamSetProperty(m_readStream.get(), kCFStreamPropertySSLSettings, settings.get());
        CFWriteStreamSetProperty(m_writeStream.get(), kCFStreamPropertySSLSettings, settings.get());
    }
}

static bool getStoredCONNECTProxyCredentials(const ProtectionSpace& protectionSpace, String& login, String& password)
{
    // Try system credential storage first, matching HTTP behavior (CFNetwork only asks the client for password if it couldn't find it in Keychain).
    Credential storedCredential = CredentialStorage::getFromPersistentStorage(protectionSpace);
    if (storedCredential.isEmpty())
        storedCredential = CredentialStorage::get(protectionSpace);

    if (storedCredential.isEmpty())
        return false;

    login = storedCredential.user();
    password = storedCredential.password();

    return true;
}

static ProtectionSpaceAuthenticationScheme authenticationSchemeFromAuthenticationMethod(CFStringRef method)
{
    if (CFEqual(method, kCFHTTPAuthenticationSchemeBasic))
        return ProtectionSpaceAuthenticationSchemeHTTPBasic;
    if (CFEqual(method, kCFHTTPAuthenticationSchemeDigest))
        return ProtectionSpaceAuthenticationSchemeHTTPDigest;
    if (CFEqual(method, kCFHTTPAuthenticationSchemeNTLM))
        return ProtectionSpaceAuthenticationSchemeNTLM;
    if (CFEqual(method, kCFHTTPAuthenticationSchemeNegotiate))
        return ProtectionSpaceAuthenticationSchemeNegotiate;
    ASSERT_NOT_REACHED();
    return ProtectionSpaceAuthenticationSchemeUnknown;
}

void SocketStreamHandle::addCONNECTCredentials(CFHTTPMessageRef proxyResponse)
{
    RetainPtr<CFHTTPAuthenticationRef> authentication(AdoptCF, CFHTTPAuthenticationCreateFromResponse(0, proxyResponse));

    if (!CFHTTPAuthenticationRequiresUserNameAndPassword(authentication.get())) {
        // That's all we can offer...
        m_client->didFailSocketStream(this, SocketStreamError()); // FIXME: Provide a sensible error.
        return;
    }

    int port = 0;
    CFNumberGetValue(m_proxyPort.get(), kCFNumberIntType, &port);
    RetainPtr<CFStringRef> methodCF(AdoptCF, CFHTTPAuthenticationCopyMethod(authentication.get()));
    RetainPtr<CFStringRef> realmCF(AdoptCF, CFHTTPAuthenticationCopyRealm(authentication.get()));
    ProtectionSpace protectionSpace(String(m_proxyHost.get()), port, ProtectionSpaceProxyHTTPS, String(realmCF.get()), authenticationSchemeFromAuthenticationMethod(methodCF.get()));
    String login;
    String password;
    if (!m_sentStoredCredentials && getStoredCONNECTProxyCredentials(protectionSpace, login, password)) {
        // Try to apply stored credentials, if we haven't tried those already.
        RetainPtr<CFStringRef> loginCF(AdoptCF, login.createCFString());
        RetainPtr<CFStringRef> passwordCF(AdoptCF, password.createCFString());
        // Creating a temporary request to make CFNetwork apply credentials to it. Unfortunately, this cannot work with NTLM authentication.
        RetainPtr<CFHTTPMessageRef> dummyRequest(AdoptCF, CFHTTPMessageCreateRequest(0, CFSTR("GET"), m_httpsURL.get(), kCFHTTPVersion1_1));

        Boolean appliedCredentials = CFHTTPMessageApplyCredentials(dummyRequest.get(), authentication.get(), loginCF.get(), passwordCF.get(), 0);
        ASSERT_UNUSED(appliedCredentials, appliedCredentials);

        RetainPtr<CFStringRef> proxyAuthorizationString(AdoptCF, CFHTTPMessageCopyHeaderFieldValue(dummyRequest.get(), CFSTR("Proxy-Authorization")));

        if (!proxyAuthorizationString) {
            // Fails e.g. for NTLM auth.
            m_client->didFailSocketStream(this, SocketStreamError()); // FIXME: Provide a sensible error.
            return;
        }

        // Setting the authorization results in a new connection attempt.
        wkSetCONNECTProxyAuthorizationForStream(m_readStream.get(), proxyAuthorizationString.get());
        m_sentStoredCredentials = true;
        return;
    }

    // FIXME: Ask the client if credentials could not be found.

    m_client->didFailSocketStream(this, SocketStreamError()); // FIXME: Provide a sensible error.
}

CFStringRef SocketStreamHandle::copyCFStreamDescription(void* info)
{
    SocketStreamHandle* handle = static_cast<SocketStreamHandle*>(info);
    return String("WebKit socket stream, " + handle->m_url.string()).createCFString();
}

struct MainThreadEventCallbackInfo {
    MainThreadEventCallbackInfo(CFStreamEventType type, SocketStreamHandle* handle) : type(type), handle(handle) { }
    CFStreamEventType type;
    RefPtr<SocketStreamHandle> handle;
};

void SocketStreamHandle::readStreamCallback(CFReadStreamRef stream, CFStreamEventType type, void* clientCallBackInfo)
{
    SocketStreamHandle* handle = static_cast<SocketStreamHandle*>(clientCallBackInfo);
    ASSERT_UNUSED(stream, stream == handle->m_readStream.get());
#if PLATFORM(WIN)
    MainThreadEventCallbackInfo info(type, handle);
    callOnMainThreadAndWait(readStreamCallbackMainThread, &info);
#else
    ASSERT(isMainThread());
    handle->readStreamCallback(type);
#endif
}

void SocketStreamHandle::writeStreamCallback(CFWriteStreamRef stream, CFStreamEventType type, void* clientCallBackInfo)
{
    SocketStreamHandle* handle = static_cast<SocketStreamHandle*>(clientCallBackInfo);
    ASSERT_UNUSED(stream, stream == handle->m_writeStream.get());
#if PLATFORM(WIN)
    MainThreadEventCallbackInfo info(type, handle);
    callOnMainThreadAndWait(writeStreamCallbackMainThread, &info);
#else
    ASSERT(isMainThread());
    handle->writeStreamCallback(type);
#endif
}

#if PLATFORM(WIN)
void SocketStreamHandle::readStreamCallbackMainThread(void* invocation)
{
    MainThreadEventCallbackInfo* info = static_cast<MainThreadEventCallbackInfo*>(invocation);
    info->handle->readStreamCallback(info->type);
}

void SocketStreamHandle::writeStreamCallbackMainThread(void* invocation)
{
    MainThreadEventCallbackInfo* info = static_cast<MainThreadEventCallbackInfo*>(invocation);
    info->handle->writeStreamCallback(info->type);
}
#endif // PLATFORM(WIN)

void SocketStreamHandle::readStreamCallback(CFStreamEventType type)
{
    switch(type) {
    case kCFStreamEventNone:
        break;
    case kCFStreamEventOpenCompleted:
        break;
    case kCFStreamEventHasBytesAvailable: {
        if (m_connectingSubstate == WaitingForConnect) {
            if (m_connectionType == CONNECTProxy) {
                RetainPtr<CFHTTPMessageRef> proxyResponse(AdoptCF, wkCopyCONNECTProxyResponse(m_readStream.get(), m_httpsURL.get()));
                if (proxyResponse && (407 == CFHTTPMessageGetResponseStatusCode(proxyResponse.get()))) {
                    addCONNECTCredentials(proxyResponse.get());
                    return;
                }
            }
        } else if (m_connectingSubstate == WaitingForCredentials)
            break;

        if (m_connectingSubstate == WaitingForConnect) {
            m_connectingSubstate = Connected;
            m_state = Open;
            m_client->didOpenSocketStream(this);
            if (m_state == Closed)
                break;
            // Fall through.
        } else if (m_state == Closed)
            break;

        ASSERT(m_state == Open);
        ASSERT(m_connectingSubstate == Connected);

        CFIndex length;
        UInt8 localBuffer[1024]; // Used if CFReadStreamGetBuffer couldn't return anything.
        const UInt8* ptr = CFReadStreamGetBuffer(m_readStream.get(), 0, &length);
        if (!ptr) {
            length = CFReadStreamRead(m_readStream.get(), localBuffer, sizeof(localBuffer));
            ptr = localBuffer;
        }

        m_client->didReceiveSocketStreamData(this, reinterpret_cast<const char*>(ptr), length);

        break;
    }
    case kCFStreamEventCanAcceptBytes:
        ASSERT_NOT_REACHED();
        break;
    case kCFStreamEventErrorOccurred: {
        RetainPtr<CFErrorRef> error(AdoptCF, CFReadStreamCopyError(m_readStream.get()));
        reportErrorToClient(error.get());
        break;
    }
    case kCFStreamEventEndEncountered:
        platformClose();
        break;
    }
}

void SocketStreamHandle::writeStreamCallback(CFStreamEventType type)
{
    switch(type) {
    case kCFStreamEventNone:
        break;
    case kCFStreamEventOpenCompleted:
        break;
    case kCFStreamEventHasBytesAvailable:
        ASSERT_NOT_REACHED();
        break;
    case kCFStreamEventCanAcceptBytes: {
        // Possibly, a spurious event from CONNECT handshake.
        if (!CFWriteStreamCanAcceptBytes(m_writeStream.get()))
            return;

        if (m_connectingSubstate == WaitingForCredentials)
            break;

        if (m_connectingSubstate == WaitingForConnect) {
            m_connectingSubstate = Connected;
            m_state = Open;
            m_client->didOpenSocketStream(this);
            break;
        }

        ASSERT(m_state == Open);
        ASSERT(m_connectingSubstate == Connected);

        sendPendingData();
        break;
    }
    case kCFStreamEventErrorOccurred: {
        RetainPtr<CFErrorRef> error(AdoptCF, CFWriteStreamCopyError(m_writeStream.get()));
        reportErrorToClient(error.get());
        break;
    }
    case kCFStreamEventEndEncountered:
        // FIXME: Currently, we handle closing in read callback, but these can come independently (e.g. a server can stop listening, but keep sending data).
        break;
    }
}

void SocketStreamHandle::reportErrorToClient(CFErrorRef error)
{
    CFIndex errorCode = CFErrorGetCode(error);
    String description;

#if PLATFORM(MAC)

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

    if (CFEqual(CFErrorGetDomain(error), kCFErrorDomainOSStatus)) {
        const char* descriptionOSStatus = GetMacOSStatusCommentString(static_cast<OSStatus>(errorCode));
        if (descriptionOSStatus && descriptionOSStatus[0] != '\0')
            description = "OSStatus Error " + String::number(errorCode) + ": " + descriptionOSStatus;
    }

#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

#endif

    if (description.isNull()) {
        RetainPtr<CFStringRef> descriptionCF(AdoptCF, CFErrorCopyDescription(error));
        description = String(descriptionCF.get());
    }

    m_client->didFailSocketStream(this, SocketStreamError(static_cast<int>(errorCode), m_url.string(), description));
}

SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p dtor", this);

    ASSERT(!m_pacRunLoopSource);
}

int SocketStreamHandle::platformSend(const char* data, int length)
{
    if (!CFWriteStreamCanAcceptBytes(m_writeStream.get()))
        return 0;

    return CFWriteStreamWrite(m_writeStream.get(), reinterpret_cast<const UInt8*>(data), length);
}

void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);

    if (m_pacRunLoopSource) 
        removePACRunLoopSource();

    ASSERT(!m_readStream == !m_writeStream);
    if (!m_readStream) {
        if (m_connectingSubstate == New || m_connectingSubstate == ExecutingPACFile)
            m_client->didCloseSocketStream(this);
        return;
    }

#if PLATFORM(WIN)
    CFReadStreamUnscheduleFromRunLoop(m_readStream.get(), loaderRunLoop(), kCFRunLoopDefaultMode);
    CFWriteStreamUnscheduleFromRunLoop(m_writeStream.get(), loaderRunLoop(), kCFRunLoopDefaultMode);
#else
    CFReadStreamUnscheduleFromRunLoop(m_readStream.get(), CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
    CFWriteStreamUnscheduleFromRunLoop(m_writeStream.get(), CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
#endif

    CFReadStreamClose(m_readStream.get());
    CFWriteStreamClose(m_writeStream.get());
    
    m_readStream = 0;
    m_writeStream = 0;

    m_client->didCloseSocketStream(this);
}

void SocketStreamHandle::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
}

void SocketStreamHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
}

void SocketStreamHandle::receivedCancellation(const AuthenticationChallenge&)
{
}

unsigned short SocketStreamHandle::port() const
{
    if (unsigned short urlPort = m_url.port())
        return urlPort;
    if (shouldUseSSL())
        return 443;
    return 80;
}

}  // namespace WebCore
