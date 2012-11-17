/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 * Copyright (C) 2008, 2010 Collabora Ltd.
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2009, 2010, 2011, 2012 Igalia S.L.
 * Copyright (C) 2009 John Kjellberg <john.kjellberg@power.alstom.com>
 * Copyright (C) 2012 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ResourceHandle.h"

#include "CachedResourceLoader.h"
#include "ChromeClient.h"
#include "CookieJarSoup.h"
#include "CredentialStorage.h"
#include "FileSystem.h"
#include "Frame.h"
#include "GOwnPtrSoup.h"
#include "HTTPParsers.h"
#include "LocalizedStrings.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Page.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "TextEncoding.h"
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-request-http.h>
#include <libsoup/soup-requester.h>
#include <libsoup/soup.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/SHA1.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>

#if ENABLE(BLOB)
#include "BlobData.h"
#include "BlobRegistryImpl.h"
#include "BlobStorageData.h"
#endif

#if PLATFORM(GTK)
#include "CredentialBackingStore.h"
#endif

namespace WebCore {

#define READ_BUFFER_SIZE 8192

static bool loadingSynchronousRequest = false;

class WebCoreSynchronousLoader : public ResourceHandleClient {
    WTF_MAKE_NONCOPYABLE(WebCoreSynchronousLoader);
public:

    WebCoreSynchronousLoader(ResourceError& error, ResourceResponse& response, SoupSession* session, Vector<char>& data)
        : m_error(error)
        , m_response(response)
        , m_session(session)
        , m_data(data)
        , m_finished(false)
    {
        // We don't want any timers to fire while we are doing our synchronous load
        // so we replace the thread default main context. The main loop iterations
        // will only process GSources associated with this inner context.
        loadingSynchronousRequest = true;
        GRefPtr<GMainContext> innerMainContext = adoptGRef(g_main_context_new());
        g_main_context_push_thread_default(innerMainContext.get());
        m_mainLoop = adoptGRef(g_main_loop_new(innerMainContext.get(), false));

        adjustMaxConnections(1);
    }

    ~WebCoreSynchronousLoader()
    {
        adjustMaxConnections(-1);
        g_main_context_pop_thread_default(g_main_context_get_thread_default());
        loadingSynchronousRequest = false;
    }

    void adjustMaxConnections(int adjustment)
    {
        int maxConnections, maxConnectionsPerHost;
        g_object_get(m_session,
                     SOUP_SESSION_MAX_CONNS, &maxConnections,
                     SOUP_SESSION_MAX_CONNS_PER_HOST, &maxConnectionsPerHost,
                     NULL);
        maxConnections += adjustment;
        maxConnectionsPerHost += adjustment;
        g_object_set(m_session,
                     SOUP_SESSION_MAX_CONNS, maxConnections,
                     SOUP_SESSION_MAX_CONNS_PER_HOST, maxConnectionsPerHost,
                     NULL);

    }

    virtual bool isSynchronousClient()
    {
        return true;
    }

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
    {
        m_response = response;
    }

    virtual void didReceiveData(ResourceHandle*, const char* data, int length, int)
    {
        m_data.append(data, length);
    }

    virtual void didFinishLoading(ResourceHandle*, double)
    {
        if (g_main_loop_is_running(m_mainLoop.get()))
            g_main_loop_quit(m_mainLoop.get());
        m_finished = true;
    }

    virtual void didFail(ResourceHandle* handle, const ResourceError& error)
    {
        m_error = error;
        didFinishLoading(handle, 0);
    }

    virtual void didReceiveAuthenticationChallenge(ResourceHandle*, const AuthenticationChallenge& challenge)
    {
        // We do not handle authentication for synchronous XMLHttpRequests.
        challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);
    }

    void run()
    {
        if (!m_finished)
            g_main_loop_run(m_mainLoop.get());
    }

private:
    ResourceError& m_error;
    ResourceResponse& m_response;
    SoupSession* m_session;
    Vector<char>& m_data;
    bool m_finished;
    GRefPtr<GMainLoop> m_mainLoop;
};

class HostTLSCertificateSet {
public:
    void add(GTlsCertificate* certificate)
    {
        String certificateHash = computeCertificateHash(certificate);
        if (!certificateHash.isEmpty())
            m_certificates.add(certificateHash);
    }

    bool contains(GTlsCertificate* certificate)
    {
        return m_certificates.contains(computeCertificateHash(certificate));
    }

private:
    static String computeCertificateHash(GTlsCertificate* certificate)
    {
        GByteArray* data = 0;
        g_object_get(G_OBJECT(certificate), "certificate", &data, NULL);
        if (!data)
            return String();

        static const size_t sha1HashSize = 20;
        GRefPtr<GByteArray> certificateData = adoptGRef(data);
        SHA1 sha1;
        sha1.addBytes(certificateData->data, certificateData->len);

        Vector<uint8_t, sha1HashSize> digest;
        sha1.computeHash(digest);

        return base64Encode(reinterpret_cast<const char*>(digest.data()), sha1HashSize);
    }

    HashSet<String> m_certificates;
};

static void cleanupSoupRequestOperation(ResourceHandle*, bool isDestroying);
static void sendRequestCallback(GObject*, GAsyncResult*, gpointer);
static void readCallback(GObject*, GAsyncResult*, gpointer);
static void closeCallback(GObject*, GAsyncResult*, gpointer);
static gboolean requestTimeoutCallback(void*);
#if ENABLE(WEB_TIMING)
static int  milisecondsSinceRequest(double requestTime);
#endif

static bool gIgnoreSSLErrors = false;

static HashSet<String>& allowsAnyHTTPSCertificateHosts()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, hosts, ());
    return hosts;
}

typedef HashMap<String, HostTLSCertificateSet> CertificatesMap;
static CertificatesMap& clientCertificates()
{
    DEFINE_STATIC_LOCAL(CertificatesMap, certificates, ());
    return certificates;
}

ResourceHandleInternal::~ResourceHandleInternal()
{
}

static SoupSession* sessionFromContext(NetworkingContext* context)
{
    return (context && context->isValid()) ? context->soupSession() : ResourceHandle::defaultSession();
}

ResourceHandle::~ResourceHandle()
{
    cleanupSoupRequestOperation(this, true);
}

static void ensureSessionIsInitialized(SoupSession* session)
{
    if (g_object_get_data(G_OBJECT(session), "webkit-init"))
        return;

    if (session == ResourceHandle::defaultSession()) {
        SoupCookieJar* jar = SOUP_COOKIE_JAR(soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR));
        if (!jar)
            soup_session_add_feature(session, SOUP_SESSION_FEATURE(soupCookieJar()));
        else
            setSoupCookieJar(jar);
    }

#if !LOG_DISABLED
    if (!soup_session_get_feature(session, SOUP_TYPE_LOGGER) && LogNetwork.state == WTFLogChannelOn) {
        SoupLogger* logger = soup_logger_new(static_cast<SoupLoggerLogLevel>(SOUP_LOGGER_LOG_BODY), -1);
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(logger));
        g_object_unref(logger);
    }
#endif // !LOG_DISABLED

    if (!soup_session_get_feature(session, SOUP_TYPE_REQUESTER)) {
        SoupRequester* requester = soup_requester_new();
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(requester));
        g_object_unref(requester);
    }

    g_object_set_data(G_OBJECT(session), "webkit-init", reinterpret_cast<void*>(0xdeadbeef));
}

SoupSession* ResourceHandleInternal::soupSession()
{
    SoupSession* session = sessionFromContext(m_context.get());
    ensureSessionIsInitialized(session);
    return session;
}

static void gotHeadersCallback(SoupMessage* message, gpointer data)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;
    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_cancelled)
        return;

#if ENABLE(WEB_TIMING)
    if (d->m_response.resourceLoadTiming())
        d->m_response.resourceLoadTiming()->receiveHeadersEnd = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
#endif

#if PLATFORM(GTK)
    // We are a bit more conservative with the persistent credential storage than the session store,
    // since we are waiting until we know that this authentication succeeded before actually storing.
    // This is because we want to avoid hitting the disk twice (once to add and once to remove) for
    // incorrect credentials or polluting the keychain with invalid credentials.
    if (message->status_code != 401 && message->status_code < 500 && !d->m_credentialDataToSaveInPersistentStore.credential.isEmpty()) {
        credentialBackingStore().storeCredentialsForChallenge(
            d->m_credentialDataToSaveInPersistentStore.challenge,
            d->m_credentialDataToSaveInPersistentStore.credential);
    }
#endif

    // The original response will be needed later to feed to willSendRequest in
    // restartedCallback() in case we are redirected. For this reason, so we store it
    // here.
    ResourceResponse response;
    response.updateFromSoupMessage(message);
    d->m_response = response;
}

static void applyAuthenticationToRequest(ResourceHandle* handle, bool redirect)
{
    // m_user/m_pass are credentials given manually, for instance, by the arguments passed to XMLHttpRequest.open().
    ResourceHandleInternal* d = handle->getInternal();
    String user = d->m_user;
    String password = d->m_pass;

    ResourceRequest& request = d->m_firstRequest;
    if (handle->shouldUseCredentialStorage()) {
        if (d->m_user.isEmpty() && d->m_pass.isEmpty())
            d->m_initialCredential = CredentialStorage::get(request.url());
        else if (!redirect) {
            // If there is already a protection space known for the URL, update stored credentials
            // before sending a request. This makes it possible to implement logout by sending an
            // XMLHttpRequest with known incorrect credentials, and aborting it immediately (so that
            // an authentication dialog doesn't pop up).
            CredentialStorage::set(Credential(d->m_user, d->m_pass, CredentialPersistenceNone), request.url());
        }
    }

    if (!d->m_initialCredential.isEmpty()) {
        user = d->m_initialCredential.user();
        password = d->m_initialCredential.password();
    }

    // We always put the credentials into the URL. In the CFNetwork-port HTTP family credentials are applied in
    // the didReceiveAuthenticationChallenge callback, but libsoup requires us to use this method to override
    // any previously remembered credentials. It has its own per-session credential storage.
    if (!user.isEmpty() || !password.isEmpty()) {
        KURL urlWithCredentials(request.url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        request.setURL(urlWithCredentials);
    }
}

// Called each time the message is going to be sent again except the first time.
// It's used mostly to let webkit know about redirects.
static void restartedCallback(SoupMessage* message, gpointer data)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;
    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_cancelled)
        return;

    GOwnPtr<char> uri(soup_uri_to_string(soup_message_get_uri(message), false));
    String location = String::fromUTF8(uri.get());
    KURL newURL = KURL(handle->firstRequest().url(), location);

    ResourceRequest request = handle->firstRequest();
    request.setURL(newURL);
    request.setHTTPMethod(message->method);

    // Should not set Referer after a redirect from a secure resource to non-secure one.
    if (!request.url().protocolIs("https") && protocolIs(request.httpReferrer(), "https")) {
        request.clearHTTPReferrer();
        soup_message_headers_remove(message->request_headers, "Referer");
    }

    const KURL& url = request.url();
    d->m_user = url.user();
    d->m_pass = url.pass();
    request.removeCredentials();

    ResourceResponse& redirectResponse = d->m_response;
    if (!protocolHostAndPortAreEqual(request.url(), redirectResponse.url())) {
        // If the network layer carries over authentication headers from the original request
        // in a cross-origin redirect, we want to clear those headers here. 
        request.clearHTTPAuthorization();
        soup_message_headers_remove(message->request_headers, "Authorization");

        // TODO: We are losing any username and password specified in the redirect URL, as this is the 
        // same behavior as the CFNet port. We should investigate if this is really what we want.
    } else
        applyAuthenticationToRequest(handle, true);

    // Per-request authentication is handled via the URI-embedded username/password.
    GOwnPtr<SoupURI> newSoupURI(soup_uri_new(request.urlStringForSoup().utf8().data()));
    soup_message_set_uri(message, newSoupURI.get());

    if (d->client())
        d->client()->willSendRequest(handle, request, redirectResponse);

    if (d->m_cancelled)
        return;

#if ENABLE(WEB_TIMING)
    redirectResponse.setResourceLoadTiming(ResourceLoadTiming::create());
    redirectResponse.resourceLoadTiming()->requestTime = monotonicallyIncreasingTime();
#endif

    // Update the first party in case the base URL changed with the redirect
    String firstPartyString = request.firstPartyForCookies().string();
    if (!firstPartyString.isEmpty()) {
        GOwnPtr<SoupURI> firstParty(soup_uri_new(firstPartyString.utf8().data()));
        soup_message_set_first_party(d->m_soupMessage.get(), firstParty.get());
    }
}

static void wroteBodyDataCallback(SoupMessage*, SoupBuffer* buffer, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;

    ASSERT(buffer);
    ResourceHandleInternal* internal = handle->getInternal();
    internal->m_bodyDataSent += buffer->length;

    if (internal->m_cancelled)
        return;
    ResourceHandleClient* client = handle->client();
    if (!client)
        return;

    client->didSendData(handle.get(), internal->m_bodyDataSent, internal->m_bodySize);
}

static void cleanupSoupRequestOperation(ResourceHandle* handle, bool isDestroying = false)
{
    ResourceHandleInternal* d = handle->getInternal();

    if (d->m_soupRequest)
        d->m_soupRequest.clear();

    if (d->m_inputStream)
        d->m_inputStream.clear();

    d->m_cancellable.clear();

    if (d->m_soupMessage) {
        g_signal_handlers_disconnect_matched(d->m_soupMessage.get(), G_SIGNAL_MATCH_DATA,
                                             0, 0, 0, 0, handle);
        g_object_set_data(G_OBJECT(d->m_soupMessage.get()), "handle", 0);
        d->m_soupMessage.clear();
    }

    if (d->m_buffer) {
        g_slice_free1(READ_BUFFER_SIZE, d->m_buffer);
        d->m_buffer = 0;
    }

    if (d->m_timeoutSource) {
        g_source_destroy(d->m_timeoutSource.get());
        d->m_timeoutSource.clear();
    }

    if (!isDestroying)
        handle->deref();
}

static bool handleUnignoredTLSErrors(ResourceHandle* handle)
{
    ResourceHandleInternal* d = handle->getInternal();
    const ResourceResponse& response = d->m_response;

    if (!response.soupMessageTLSErrors() || gIgnoreSSLErrors)
        return false;

    String lowercaseHostURL = handle->firstRequest().url().host().lower();
    if (allowsAnyHTTPSCertificateHosts().contains(lowercaseHostURL))
        return false;

    // We aren't ignoring errors globally, but the user may have already decided to accept this certificate.
    CertificatesMap::iterator i = clientCertificates().find(lowercaseHostURL);
    if (i != clientCertificates().end() && i->value.contains(response.soupMessageCertificate()))
        return false;

    handle->client()->didFail(handle, ResourceError::tlsError(d->m_soupRequest.get(), response.soupMessageTLSErrors(), response.soupMessageCertificate()));
    return true;
}

static void sendRequestCallback(GObject*, GAsyncResult* res, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    ResourceHandleInternal* d = handle->getInternal();
    ResourceHandleClient* client = handle->client();
    SoupMessage* soupMessage = d->m_soupMessage.get();

    if (d->m_cancelled || !client) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (d->m_defersLoading) {
        d->m_deferredResult = res;
        return;
    }

    GOwnPtr<GError> error;
    GInputStream* in = soup_request_send_finish(d->m_soupRequest.get(), res, &error.outPtr());
    if (error) {
        client->didFail(handle.get(), ResourceError::httpError(soupMessage, error.get(), d->m_soupRequest.get()));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    d->m_inputStream = adoptGRef(in);
    d->m_buffer = static_cast<char*>(g_slice_alloc(READ_BUFFER_SIZE));

    if (soupMessage) {
        if (handle->shouldContentSniff() && soupMessage->status_code != SOUP_STATUS_NOT_MODIFIED) {
            const char* sniffedType = soup_request_get_content_type(d->m_soupRequest.get());
            d->m_response.setSniffedContentType(sniffedType);
        }
        d->m_response.updateFromSoupMessage(soupMessage);

        if (handleUnignoredTLSErrors(handle.get())) {
            cleanupSoupRequestOperation(handle.get());
            return;
        }

    } else {
        d->m_response.setURL(handle->firstRequest().url());
        const gchar* contentType = soup_request_get_content_type(d->m_soupRequest.get());
        d->m_response.setMimeType(extractMIMETypeFromMediaType(contentType));
        d->m_response.setTextEncodingName(extractCharsetFromMediaType(contentType));
        d->m_response.setExpectedContentLength(soup_request_get_content_length(d->m_soupRequest.get()));
    }

    client->didReceiveResponse(handle.get(), d->m_response);

    if (d->m_cancelled) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    g_input_stream_read_async(d->m_inputStream.get(), d->m_buffer, READ_BUFFER_SIZE,
                              G_PRIORITY_DEFAULT, d->m_cancellable.get(), readCallback, handle.get());
}

static bool addFileToSoupMessageBody(SoupMessage* message, const String& fileNameString, size_t offset, size_t lengthToSend, unsigned long& totalBodySize)
{
    GOwnPtr<GError> error;
    CString fileName = fileSystemRepresentation(fileNameString);
    GMappedFile* fileMapping = g_mapped_file_new(fileName.data(), false, &error.outPtr());
    if (error)
        return false;

    gsize bufferLength = lengthToSend;
    if (!lengthToSend)
        bufferLength = g_mapped_file_get_length(fileMapping);
    totalBodySize += bufferLength;

    SoupBuffer* soupBuffer = soup_buffer_new_with_owner(g_mapped_file_get_contents(fileMapping) + offset,
                                                        bufferLength,
                                                        fileMapping,
                                                        reinterpret_cast<GDestroyNotify>(g_mapped_file_unref));
    soup_message_body_append_buffer(message->request_body, soupBuffer);
    soup_buffer_free(soupBuffer);
    return true;
}

#if ENABLE(BLOB)
static bool blobIsOutOfDate(const BlobDataItem& blobItem)
{
    ASSERT(blobItem.type == BlobDataItem::File);
    if (!isValidFileTime(blobItem.expectedModificationTime))
        return false;

    time_t fileModificationTime;
    if (!getFileModificationTime(blobItem.path, fileModificationTime))
        return true;

    return fileModificationTime != static_cast<time_t>(blobItem.expectedModificationTime);
}

static void addEncodedBlobItemToSoupMessageBody(SoupMessage* message, const BlobDataItem& blobItem, unsigned long& totalBodySize)
{
    if (blobItem.type == BlobDataItem::Data) {
        totalBodySize += blobItem.length;
        soup_message_body_append(message->request_body, SOUP_MEMORY_TEMPORARY,
                                 blobItem.data->data() + blobItem.offset, blobItem.length);
        return;
    }

    ASSERT(blobItem.type == BlobDataItem::File);
    if (blobIsOutOfDate(blobItem))
        return;

    addFileToSoupMessageBody(message,
                             blobItem.path,
                             blobItem.offset,
                             blobItem.length == BlobDataItem::toEndOfFile ? 0 : blobItem.length,
                             totalBodySize);
}

static void addEncodedBlobToSoupMessageBody(SoupMessage* message, const FormDataElement& element, unsigned long& totalBodySize)
{
    RefPtr<BlobStorageData> blobData = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(KURL(ParsedURLString, element.m_url));
    if (!blobData)
        return;

    for (size_t i = 0; i < blobData->items().size(); ++i)
        addEncodedBlobItemToSoupMessageBody(message, blobData->items()[i], totalBodySize);
}
#endif // ENABLE(BLOB)

static bool addFormElementsToSoupMessage(SoupMessage* message, const char*, FormData* httpBody, unsigned long& totalBodySize)
{
    soup_message_body_set_accumulate(message->request_body, FALSE);
    size_t numElements = httpBody->elements().size();
    for (size_t i = 0; i < numElements; i++) {
        const FormDataElement& element = httpBody->elements()[i];

        if (element.m_type == FormDataElement::data) {
            totalBodySize += element.m_data.size();
            soup_message_body_append(message->request_body, SOUP_MEMORY_TEMPORARY,
                                     element.m_data.data(), element.m_data.size());
            continue;
        }

        if (element.m_type == FormDataElement::encodedFile) {
            if (!addFileToSoupMessageBody(message ,
                                         element.m_filename,
                                         0 /* offset */,
                                         0 /* lengthToSend */,
                                         totalBodySize))
                return false;
            continue;
        }

#if ENABLE(BLOB)
        ASSERT(element.m_type == FormDataElement::encodedBlob);
        addEncodedBlobToSoupMessageBody(message, element, totalBodySize);
#endif
    }
    return true;
}

#if ENABLE(WEB_TIMING)
static int milisecondsSinceRequest(double requestTime)
{
    return static_cast<int>((monotonicallyIncreasingTime() - requestTime) * 1000.0);
}

static void wroteBodyCallback(SoupMessage*, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;

    ResourceHandleInternal* d = handle->getInternal();
    if (!d->m_response.resourceLoadTiming())
        return;

    d->m_response.resourceLoadTiming()->sendEnd = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
}

static void requestStartedCallback(SoupSession*, SoupMessage* soupMessage, SoupSocket*, gpointer)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(g_object_get_data(G_OBJECT(soupMessage), "handle"));
    if (!handle)
        return;

    ResourceHandleInternal* d = handle->getInternal();
    if (!d->m_response.resourceLoadTiming())
        return;

    d->m_response.resourceLoadTiming()->sendStart = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
    if (d->m_response.resourceLoadTiming()->sslStart != -1) {
        // WebCore/inspector/front-end/RequestTimingView.js assumes
        // that SSL time is included in connection time so must
        // substract here the SSL delta that will be added later (see
        // WebInspector.RequestTimingView.createTimingTable in the
        // file above for more details).
        d->m_response.resourceLoadTiming()->sendStart -=
            d->m_response.resourceLoadTiming()->sslEnd - d->m_response.resourceLoadTiming()->sslStart;
    }
}

static void networkEventCallback(SoupMessage*, GSocketClientEvent event, GIOStream*, gpointer data)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(data);
    if (!handle)
        return;
    ResourceHandleInternal* d = handle->getInternal();
    if (d->m_cancelled)
        return;

    int deltaTime = milisecondsSinceRequest(d->m_response.resourceLoadTiming()->requestTime);
    switch (event) {
    case G_SOCKET_CLIENT_RESOLVING:
        d->m_response.resourceLoadTiming()->dnsStart = deltaTime;
        break;
    case G_SOCKET_CLIENT_RESOLVED:
        d->m_response.resourceLoadTiming()->dnsEnd = deltaTime;
        break;
    case G_SOCKET_CLIENT_CONNECTING:
        d->m_response.resourceLoadTiming()->connectStart = deltaTime;
        if (d->m_response.resourceLoadTiming()->dnsStart != -1)
            // WebCore/inspector/front-end/RequestTimingView.js assumes
            // that DNS time is included in connection time so must
            // substract here the DNS delta that will be added later (see
            // WebInspector.RequestTimingView.createTimingTable in the
            // file above for more details).
            d->m_response.resourceLoadTiming()->connectStart -=
                d->m_response.resourceLoadTiming()->dnsEnd - d->m_response.resourceLoadTiming()->dnsStart;
        break;
    case G_SOCKET_CLIENT_CONNECTED:
        // Web Timing considers that connection time involves dns, proxy & TLS negotiation...
        // so we better pick G_SOCKET_CLIENT_COMPLETE for connectEnd
        break;
    case G_SOCKET_CLIENT_PROXY_NEGOTIATING:
        d->m_response.resourceLoadTiming()->proxyStart = deltaTime;
        break;
    case G_SOCKET_CLIENT_PROXY_NEGOTIATED:
        d->m_response.resourceLoadTiming()->proxyEnd = deltaTime;
        break;
    case G_SOCKET_CLIENT_TLS_HANDSHAKING:
        d->m_response.resourceLoadTiming()->sslStart = deltaTime;
        break;
    case G_SOCKET_CLIENT_TLS_HANDSHAKED:
        d->m_response.resourceLoadTiming()->sslEnd = deltaTime;
        break;
    case G_SOCKET_CLIENT_COMPLETE:
        d->m_response.resourceLoadTiming()->connectEnd = deltaTime;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}
#endif

static const char* gSoupRequestInitiaingPageIDKey = "wk-soup-request-initiaing-page-id";

static void setSoupRequestInitiaingPageIDFromNetworkingContext(SoupRequest* request, NetworkingContext* context)
{
    if (!context || !context->isValid())
        return;

    uint64_t* initiatingPageIDPtr = static_cast<uint64_t*>(fastMalloc(sizeof(uint64_t)));
    *initiatingPageIDPtr = context->initiatingPageID();
    g_object_set_data_full(G_OBJECT(request), g_intern_static_string(gSoupRequestInitiaingPageIDKey), initiatingPageIDPtr, fastFree);
}

static bool createSoupMessageForHandleAndRequest(ResourceHandle* handle, const ResourceRequest& request)
{
    ASSERT(handle);

    ResourceHandleInternal* d = handle->getInternal();
    ASSERT(d->m_soupRequest);

    d->m_soupMessage = adoptGRef(soup_request_http_get_message(SOUP_REQUEST_HTTP(d->m_soupRequest.get())));
    if (!d->m_soupMessage)
        return false;

    SoupMessage* soupMessage = d->m_soupMessage.get();
    request.updateSoupMessage(soupMessage);

    g_object_set_data(G_OBJECT(soupMessage), "handle", handle);
    if (!handle->shouldContentSniff())
        soup_message_disable_feature(soupMessage, SOUP_TYPE_CONTENT_SNIFFER);

    String firstPartyString = request.firstPartyForCookies().string();
    if (!firstPartyString.isEmpty()) {
        GOwnPtr<SoupURI> firstParty(soup_uri_new(firstPartyString.utf8().data()));
        soup_message_set_first_party(soupMessage, firstParty.get());
    }

    FormData* httpBody = request.httpBody();
    CString contentType = request.httpContentType().utf8().data();
    if (httpBody && !httpBody->isEmpty() && !addFormElementsToSoupMessage(soupMessage, contentType.data(), httpBody, d->m_bodySize)) {
        // We failed to prepare the body data, so just fail this load.
        d->m_soupMessage.clear();
        return false;
    }

    // Make sure we have an Accept header for subresources; some sites
    // want this to serve some of their subresources
    if (!soup_message_headers_get_one(soupMessage->request_headers, "Accept"))
        soup_message_headers_append(soupMessage->request_headers, "Accept", "*/*");

    // In the case of XHR .send() and .send("") explicitly tell libsoup to send a zero content-lenght header
    // for consistency with other backends (e.g. Chromium's) and other UA implementations like FF. It's done
    // in the backend here instead of in XHR code since in XHR CORS checking prevents us from this kind of
    // late header manipulation.
    if ((request.httpMethod() == "POST" || request.httpMethod() == "PUT")
        && (!request.httpBody() || request.httpBody()->isEmpty()))
        soup_message_headers_set_content_length(soupMessage->request_headers, 0);

    g_signal_connect(d->m_soupMessage.get(), "got-headers", G_CALLBACK(gotHeadersCallback), handle);
    g_signal_connect(d->m_soupMessage.get(), "restarted", G_CALLBACK(restartedCallback), handle);
    g_signal_connect(d->m_soupMessage.get(), "wrote-body-data", G_CALLBACK(wroteBodyDataCallback), handle);

#if ENABLE(WEB_TIMING)
    d->m_response.setResourceLoadTiming(ResourceLoadTiming::create());
    g_signal_connect(d->m_soupMessage.get(), "network-event", G_CALLBACK(networkEventCallback), handle);
    g_signal_connect(d->m_soupMessage.get(), "wrote-body", G_CALLBACK(wroteBodyCallback), handle);
#endif

    return true;
}

static bool createSoupRequestAndMessageForHandle(ResourceHandle* handle, bool isHTTPFamilyRequest)
{
    ResourceHandleInternal* d = handle->getInternal();
    SoupRequester* requester = SOUP_REQUESTER(soup_session_get_feature(d->soupSession(), SOUP_TYPE_REQUESTER));

    GOwnPtr<GError> error;
    ResourceRequest& request = handle->firstRequest();

    d->m_soupRequest = adoptGRef(soup_requester_request(requester, request.urlStringForSoup().utf8().data(), &error.outPtr()));
    if (error) {
        d->m_soupRequest.clear();
        return false;
    }

    // SoupMessages are only applicable to HTTP-family requests.
    if (isHTTPFamilyRequest && !createSoupMessageForHandleAndRequest(handle, request)) {
        d->m_soupRequest.clear();
        return false;
    }

    return true;
}

bool ResourceHandle::start(NetworkingContext* context)
{
    ASSERT(!d->m_soupMessage);

    // The frame could be null if the ResourceHandle is not associated to any
    // Frame, e.g. if we are downloading a file.
    // If the frame is not null but the page is null this must be an attempted
    // load from an unload handler, so let's just block it.
    // If both the frame and the page are not null the context is valid.
    if (context && !context->isValid())
        return false;

    // Used to set the keep track of custom SoupSessions for ports that support it (EFL).
    d->m_context = context;

    // Only allow the POST and GET methods for non-HTTP requests.
    const ResourceRequest& request = firstRequest();
    bool isHTTPFamilyRequest = request.url().protocolIsInHTTPFamily();
    if (!isHTTPFamilyRequest && request.httpMethod() != "GET" && request.httpMethod() != "POST") {
        this->scheduleFailure(InvalidURLFailure); // Error must not be reported immediately
        return true;
    }

    applyAuthenticationToRequest(this, false);
    // The CFNet backend clears these, so we do as well.
    d->m_user = String();
    d->m_pass = String();

    if (!createSoupRequestAndMessageForHandle(this, isHTTPFamilyRequest)) {
        this->scheduleFailure(InvalidURLFailure); // Error must not be reported immediately
        return true;
    }

    setSoupRequestInitiaingPageIDFromNetworkingContext(d->m_soupRequest.get(), context);

    // Send the request only if it's not been explicitly deferred.
    if (!d->m_defersLoading)
        sendPendingRequest();

    return true;
}

void ResourceHandle::sendPendingRequest()
{
#if ENABLE(WEB_TIMING)
    if (d->m_response.resourceLoadTiming())
        d->m_response.resourceLoadTiming()->requestTime = monotonicallyIncreasingTime();
#endif

    if (d->m_firstRequest.timeoutInterval() > 0) {
        // soup_add_timeout returns a GSource* whose only reference is owned by
        // the context. We need to have our own reference to it, hence not using adoptRef.
        d->m_timeoutSource = soup_add_timeout(g_main_context_get_thread_default(),
            d->m_firstRequest.timeoutInterval() * 1000, requestTimeoutCallback, this);
    }

    // Balanced by a deref() in cleanupSoupRequestOperation, which should always run.
    ref();

    d->m_cancellable = adoptGRef(g_cancellable_new());
    soup_request_send_async(d->m_soupRequest.get(), d->m_cancellable.get(), sendRequestCallback, this);
}

void ResourceHandle::cancel()
{
    d->m_cancelled = true;
    if (d->m_soupMessage)
        soup_session_cancel_message(d->soupSession(), d->m_soupMessage.get(), SOUP_STATUS_CANCELLED);
    else if (d->m_cancellable)
        g_cancellable_cancel(d->m_cancellable.get());
}

bool ResourceHandle::shouldUseCredentialStorage()
{
    return (!client() || client()->shouldUseCredentialStorage(this)) && firstRequest().url().protocolIsInHTTPFamily();
}

void ResourceHandle::setHostAllowsAnyHTTPSCertificate(const String& host)
{
    allowsAnyHTTPSCertificateHosts().add(host.lower());
}

void ResourceHandle::setClientCertificate(const String& host, GTlsCertificate* certificate)
{
    clientCertificates().add(host.lower(), HostTLSCertificateSet()).iterator->value.add(certificate);
}

void ResourceHandle::setIgnoreSSLErrors(bool ignoreSSLErrors)
{
    gIgnoreSSLErrors = ignoreSSLErrors;
}

#if PLATFORM(GTK)
void getCredentialFromPersistentStoreCallback(const Credential& credential, void* data)
{
    static_cast<ResourceHandle*>(data)->continueDidReceiveAuthenticationChallenge(credential);
}
#endif

void ResourceHandle::continueDidReceiveAuthenticationChallenge(const Credential& credentialFromPersistentStorage)
{
    ASSERT(!d->m_currentWebChallenge.isNull());
    AuthenticationChallenge& challenge = d->m_currentWebChallenge;

    ASSERT(challenge.soupSession());
    ASSERT(challenge.soupMessage());
    if (!credentialFromPersistentStorage.isEmpty())
        challenge.setProposedCredential(credentialFromPersistentStorage);

    if (!client()) {
        soup_session_unpause_message(challenge.soupSession(), challenge.soupMessage());
        clearAuthentication();
        return;
    }

    ASSERT(challenge.soupSession());
    ASSERT(challenge.soupMessage());
    client()->didReceiveAuthenticationChallenge(this, challenge);
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    ASSERT(d->m_currentWebChallenge.isNull());

    bool useCredentialStorage = shouldUseCredentialStorage();
    if (!d->m_user.isNull() && !d->m_pass.isNull()) {
        Credential credential = Credential(d->m_user, d->m_pass, CredentialPersistenceForSession);
        if (useCredentialStorage)
            CredentialStorage::set(credential, challenge.protectionSpace(), challenge.failureResponse().url());
        soup_auth_authenticate(challenge.soupAuth(), credential.user().utf8().data(), credential.password().utf8().data());

        return;
    }

    // FIXME: Per the specification, the user shouldn't be asked for credentials if there were incorrect ones provided explicitly.
    if (useCredentialStorage) {
        if (!d->m_initialCredential.isEmpty() || challenge.previousFailureCount()) {
            // The stored credential wasn't accepted, stop using it. There is a race condition
            // here, since a different credential might have already been stored by another
            // ResourceHandle, but the observable effect should be very minor, if any.
            CredentialStorage::remove(challenge.protectionSpace());
        }

        if (!challenge.previousFailureCount()) {
            Credential credential = CredentialStorage::get(challenge.protectionSpace());
            if (!credential.isEmpty() && credential != d->m_initialCredential) {
                ASSERT(credential.persistence() == CredentialPersistenceNone);

                // Store the credential back, possibly adding it as a default for this directory.
                if (challenge.failureResponse().httpStatusCode() == 401)
                    CredentialStorage::set(credential, challenge.protectionSpace(), challenge.failureResponse().url());

                soup_auth_authenticate(challenge.soupAuth(), credential.user().utf8().data(), credential.password().utf8().data());
                return;
            }
        }
    }

    d->m_currentWebChallenge = challenge;
    soup_session_pause_message(challenge.soupSession(), challenge.soupMessage());

#if PLATFORM(GTK)
    // We could also do this before we even start the request, but that would be at the expense
    // of all request latency, versus a one-time latency for the small subset of requests that
    // use HTTP authentication. In the end, this doesn't matter much, because persistent credentials
    // will become session credentials after the first use.
    if (useCredentialStorage) {
        credentialBackingStore().credentialForChallenge(challenge, getCredentialFromPersistentStoreCallback, this);
        return;
    }
#endif

    continueDidReceiveAuthenticationChallenge(Credential());
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    ASSERT(!challenge.isNull());
    if (challenge != d->m_currentWebChallenge)
        return;
    soup_session_unpause_message(challenge.soupSession(), challenge.soupMessage());

    clearAuthentication();
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
    ASSERT(!challenge.isNull());
    if (challenge != d->m_currentWebChallenge)
        return;

    // FIXME: Support empty credentials. Currently, an empty credential cannot be stored in WebCore credential storage, as that's empty value for its map.
    if (credential.isEmpty()) {
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    if (shouldUseCredentialStorage()) {
        // Eventually we will manage per-session credentials only internally or use some newly-exposed API from libsoup,
        // because once we authenticate via libsoup, there is no way to ignore it for a particular request. Right now,
        // we place the credentials in the store even though libsoup will never fire the authenticate signal again for
        // this protection space.
        if (credential.persistence() == CredentialPersistenceForSession || credential.persistence() == CredentialPersistencePermanent)
            CredentialStorage::set(credential, challenge.protectionSpace(), challenge.failureResponse().url());

#if PLATFORM(GTK)
        if (credential.persistence() == CredentialPersistencePermanent) {
            d->m_credentialDataToSaveInPersistentStore.credential = credential;
            d->m_credentialDataToSaveInPersistentStore.challenge = challenge;
        }
#endif
    }

    ASSERT(challenge.soupSession());
    ASSERT(challenge.soupMessage());
    soup_auth_authenticate(challenge.soupAuth(), credential.user().utf8().data(), credential.password().utf8().data());
    soup_session_unpause_message(challenge.soupSession(), challenge.soupMessage());

    clearAuthentication();
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    ASSERT(!challenge.isNull());
    if (challenge != d->m_currentWebChallenge)
        return;

    soup_session_unpause_message(challenge.soupSession(), challenge.soupMessage());

    if (client())
        client()->receivedCancellation(this, challenge);

    clearAuthentication();
}

static bool waitingToSendRequest(ResourceHandle* handle)
{
    // We need to check for d->m_soupRequest because the request may have raised a failure
    // (for example invalid URLs). We cannot  simply check for d->m_scheduledFailure because
    // it's cleared as soon as the failure event is fired.
    return handle->getInternal()->m_soupRequest && !handle->getInternal()->m_cancellable;
}

void ResourceHandle::platformSetDefersLoading(bool defersLoading)
{
    if (d->m_cancelled)
        return;

    // Except when canceling a possible timeout timer, we only need to take action here to UN-defer loading.
    if (defersLoading) {
        if (d->m_timeoutSource) {
            g_source_destroy(d->m_timeoutSource.get());
            d->m_timeoutSource.clear();
        }
        return;
    }

    if (waitingToSendRequest(this)) {
        sendPendingRequest();
        return;
    }

    if (d->m_deferredResult) {
        GRefPtr<GAsyncResult> asyncResult = adoptGRef(d->m_deferredResult.leakRef());

        if (d->m_inputStream)
            readCallback(G_OBJECT(d->m_inputStream.get()), asyncResult.get(), this);
        else
            sendRequestCallback(G_OBJECT(d->m_soupRequest.get()), asyncResult.get(), this);
    }
}

bool ResourceHandle::loadsBlocked()
{
    return false;
}

bool ResourceHandle::willLoadFromCache(ResourceRequest&, Frame*)
{
    // Not having this function means that we'll ask the user about re-posting a form
    // even when we go back to a page that's still in the cache.
    notImplemented();
    return false;
}

void ResourceHandle::loadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials /*storedCredentials*/, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
#if ENABLE(BLOB)
    if (request.url().protocolIs("blob")) {
        blobRegistry().loadResourceSynchronously(request, error, response, data);
        return;
    }
#endif
 
    ASSERT(!loadingSynchronousRequest);
    if (loadingSynchronousRequest) // In practice this cannot happen, but if for some reason it does,
        return;                    // we want to avoid accidentally going into an infinite loop of requests.

    WebCoreSynchronousLoader syncLoader(error, response, sessionFromContext(context), data);
    RefPtr<ResourceHandle> handle = create(context, request, &syncLoader, false /*defersLoading*/, false /*shouldContentSniff*/);
    if (!handle)
        return;

    // If the request has already failed, do not run the main loop, or else we'll block indefinitely.
    if (handle->d->m_scheduledFailureType != NoFailure)
        return;

    syncLoader.run();
}

static void closeCallback(GObject*, GAsyncResult* res, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = handle->getInternal();

    g_input_stream_close_finish(d->m_inputStream.get(), res, 0);

    ResourceHandleClient* client = handle->client();
    if (client && loadingSynchronousRequest)
        client->didFinishLoading(handle.get(), 0);

    cleanupSoupRequestOperation(handle.get());
}

static void readCallback(GObject*, GAsyncResult* asyncResult, gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);

    ResourceHandleInternal* d = handle->getInternal();
    ResourceHandleClient* client = handle->client();

    if (d->m_cancelled || !client) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (d->m_defersLoading) {
        d->m_deferredResult = asyncResult;
        return;
    }

    GOwnPtr<GError> error;
    gssize bytesRead = g_input_stream_read_finish(d->m_inputStream.get(), asyncResult, &error.outPtr());
    if (error) {
        client->didFail(handle.get(), ResourceError::genericIOError(error.get(), d->m_soupRequest.get()));
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    if (!bytesRead) {
        // We inform WebCore of load completion now instead of waiting for the input
        // stream to close because the input stream is closed asynchronously. If this
        // is a synchronous request, we wait until the closeCallback, because we don't
        // want to halt the internal main loop before the input stream closes.
        if (client && !loadingSynchronousRequest) {
            client->didFinishLoading(handle.get(), 0);
            handle->setClient(0); // Unset the client so that we do not try to access th
                                  // client in the closeCallback.
        }
        g_input_stream_close_async(d->m_inputStream.get(), G_PRIORITY_DEFAULT, 0, closeCallback, handle.get());
        return;
    }

    // It's mandatory to have sent a response before sending data
    ASSERT(!d->m_response.isNull());

    client->didReceiveData(handle.get(), d->m_buffer, bytesRead, bytesRead);

    // didReceiveData may cancel the load, which may release the last reference.
    if (d->m_cancelled || !client) {
        cleanupSoupRequestOperation(handle.get());
        return;
    }

    g_input_stream_read_async(d->m_inputStream.get(), d->m_buffer, READ_BUFFER_SIZE, G_PRIORITY_DEFAULT,
                              d->m_cancellable.get(), readCallback, handle.get());
}

static gboolean requestTimeoutCallback(gpointer data)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(data);
    handle->client()->didFail(handle.get(), ResourceError::timeoutError(handle->getInternal()->m_firstRequest.url().string()));
    handle->cancel();

    return FALSE;
}

static void authenticateCallback(SoupSession* session, SoupMessage* soupMessage, SoupAuth* soupAuth, gboolean retrying)
{
    RefPtr<ResourceHandle> handle = static_cast<ResourceHandle*>(g_object_get_data(G_OBJECT(soupMessage), "handle"));
    if (!handle)
        return;
    handle->didReceiveAuthenticationChallenge(AuthenticationChallenge(session, soupMessage, soupAuth, retrying, handle.get()));
}

SoupSession* ResourceHandle::defaultSession()
{
    static SoupSession* session = 0;
    // Values taken from http://www.browserscope.org/  following
    // the rule "Do What Every Other Modern Browser Is Doing". They seem
    // to significantly improve page loading time compared to soup's
    // default values.
    static const int maxConnections = 35;
    static const int maxConnectionsPerHost = 6;

    if (!session) {
        session = soup_session_async_new();
        g_object_set(session,
                     SOUP_SESSION_MAX_CONNS, maxConnections,
                     SOUP_SESSION_MAX_CONNS_PER_HOST, maxConnectionsPerHost,
                     SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
                     SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER,
                     SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                     SOUP_SESSION_USE_THREAD_CONTEXT, TRUE,
                     NULL);
        g_signal_connect(session, "authenticate", G_CALLBACK(authenticateCallback), 0);

#if ENABLE(WEB_TIMING)
        g_signal_connect(session, "request-started", G_CALLBACK(requestStartedCallback), 0);
#endif
    }

    return session;
}

uint64_t ResourceHandle::getSoupRequestInitiaingPageID(SoupRequest* request)
{
    uint64_t* initiatingPageIDPtr = static_cast<uint64_t*>(g_object_get_data(G_OBJECT(request), gSoupRequestInitiaingPageIDKey));
    return initiatingPageIDPtr ? *initiatingPageIDPtr : 0;
}

}
