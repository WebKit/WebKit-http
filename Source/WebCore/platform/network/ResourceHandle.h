/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef ResourceHandle_h
#define ResourceHandle_h

#include "AuthenticationChallenge.h"
#include "AuthenticationClient.h"
#include "HTTPHeaderMap.h"
#include "NetworkingContext.h"
#include "ThreadableLoader.h"
#include <wtf/OwnPtr.h>

#if USE(SOUP)
typedef struct _SoupSession SoupSession;
#endif

#if USE(CF)
typedef const struct __CFData * CFDataRef;
#endif

#if USE(WININET)
typedef unsigned long DWORD;
typedef unsigned long DWORD_PTR;
typedef void* LPVOID;
typedef LPVOID HINTERNET;
#endif


#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
#ifdef __OBJC__
@class NSData;
@class NSError;
@class NSURLConnection;
@class WebCoreResourceHandleAsDelegate;
#else
class NSData;
class NSError;
class NSURLConnection;
class WebCoreResourceHandleAsDelegate;
typedef struct objc_object *id;
#endif
#endif

#if USE(CFNETWORK)
typedef struct _CFURLConnection* CFURLConnectionRef;
typedef int CFHTTPCookieStorageAcceptPolicy;
typedef struct OpaqueCFHTTPCookieStorage* CFHTTPCookieStorageRef;
#endif

#if USE(CFURLSTORAGESESSIONS)
typedef const struct __CFURLStorageSession* CFURLStorageSessionRef;
#endif

namespace WebCore {

class AuthenticationChallenge;
class Credential;
class FormData;
class Frame;
class KURL;
class ProtectionSpace;
class ResourceError;
class ResourceHandleClient;
class ResourceHandleInternal;
class ResourceRequest;
class ResourceResponse;
class SchedulePair;
class SharedBuffer;

template <typename T> class Timer;

class ResourceHandle : public RefCounted<ResourceHandle>
#if PLATFORM(MAC) || USE(CFNETWORK) || USE(CURL)
    , public AuthenticationClient
#endif
    {
public:
    static PassRefPtr<ResourceHandle> create(NetworkingContext*, const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff);
    static void loadResourceSynchronously(NetworkingContext*, const ResourceRequest&, StoredCredentials, ResourceError&, ResourceResponse&, Vector<char>& data);

    static void prepareForURL(const KURL&);
    static bool willLoadFromCache(ResourceRequest&, Frame*);
    static void cacheMetadata(const ResourceResponse&, const Vector<char>&);
#if PLATFORM(MAC)
    static bool didSendBodyDataDelegateExists();
#endif

    virtual ~ResourceHandle();

#if PLATFORM(MAC) || USE(CFNETWORK)
    void willSendRequest(ResourceRequest&, const ResourceResponse& redirectResponse);
    bool shouldUseCredentialStorage();
#endif
#if PLATFORM(MAC) || USE(CFNETWORK) || USE(CURL)
    void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
    virtual void receivedCredential(const AuthenticationChallenge&, const Credential&);
    virtual void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&);
    virtual void receivedCancellation(const AuthenticationChallenge&);
#endif

#if PLATFORM(MAC)
    void didCancelAuthenticationChallenge(const AuthenticationChallenge&);
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    bool canAuthenticateAgainstProtectionSpace(const ProtectionSpace&);
#endif
    NSURLConnection *connection() const;
    WebCoreResourceHandleAsDelegate *delegate();
    void releaseDelegate();
    id releaseProxy();

    void schedule(SchedulePair*);
    void unschedule(SchedulePair*);
#elif USE(CFNETWORK)
    CFURLConnectionRef connection() const;
    CFURLConnectionRef releaseConnectionForDownload();
    static void setHostAllowsAnyHTTPSCertificate(const String&);
    static void setClientCertificate(const String& host, CFDataRef);
#endif

#if PLATFORM(WIN) && USE(CURL)
    static void setHostAllowsAnyHTTPSCertificate(const String&);
#endif
#if PLATFORM(WIN) && USE(CURL) && USE(CF)
    static void setClientCertificate(const String& host, CFDataRef);
#endif

    PassRefPtr<SharedBuffer> bufferedData();
    static bool supportsBufferedData();

    bool shouldContentSniff() const;
    static bool shouldContentSniffURL(const KURL&);

    static void forceContentSniffing();

#if USE(WININET)
    void setSynchronousInternetHandle(HINTERNET);
    void fileLoadTimer(Timer<ResourceHandle>*);
    void onRedirect();
    bool onRequestComplete();
    static void CALLBACK internetStatusCallback(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);
#endif

#if PLATFORM(QT) || USE(CURL) || USE(SOUP) || PLATFORM(ANDROID)
    ResourceHandleInternal* getInternal() { return d.get(); }
#endif

#if USE(SOUP)
    static SoupSession* defaultSession();
#endif

    // Used to work around the fact that you don't get any more NSURLConnection callbacks until you return from the one you're in.
    static bool loadsBlocked();    

    bool hasAuthenticationChallenge() const;
    void clearAuthentication();
    virtual void cancel();

    // The client may be 0, in which case no callbacks will be made.
    ResourceHandleClient* client() const;
    void setClient(ResourceHandleClient*);

    void setDefersLoading(bool);
      
    ResourceRequest& firstRequest();
    const String& lastHTTPMethod() const;

    void fireFailure(Timer<ResourceHandle>*);

#if USE(CFURLSTORAGESESSIONS)
    static void setPrivateBrowsingEnabled(bool);
    static CFURLStorageSessionRef privateBrowsingStorageSession();
    static void setPrivateBrowsingStorageSessionIdentifierBase(const String&);
#endif

    using RefCounted<ResourceHandle>::ref;
    using RefCounted<ResourceHandle>::deref;

protected:
    ResourceHandle(const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff);

private:
    enum FailureType {
        NoFailure,
        BlockedFailure,
        InvalidURLFailure
    };

    void platformSetDefersLoading(bool);

    void scheduleFailure(FailureType);

    bool start(NetworkingContext*);

    virtual void refAuthenticationClient() { ref(); }
    virtual void derefAuthenticationClient() { deref(); }

#if PLATFORM(MAC)
    void createNSURLConnection(id delegate, bool shouldUseCredentialStorage, bool shouldContentSniff);
#elif USE(CF)
    void createCFURLConnection(bool shouldUseCredentialStorage, bool shouldContentSniff);
#endif

#if USE(CFURLSTORAGESESSIONS)
    static RetainPtr<CFURLStorageSessionRef> createPrivateBrowsingStorageSession(CFStringRef identifier);
    static String privateBrowsingStorageSessionIdentifierDefaultBase();
#endif

    friend class ResourceHandleInternal;
    OwnPtr<ResourceHandleInternal> d;
};

}

#endif // ResourceHandle_h
