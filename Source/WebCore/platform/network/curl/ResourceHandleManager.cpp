/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Appcelerator Inc.
 * Copyright (C) 2009 Brent Fulgham <bfulgham@webkit.org>
 * All rights reserved.
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
#include "ResourceHandleManager.h"

#include "DataURL.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"

#include <errno.h>
#include <stdio.h>
#if USE(CF)
#include <wtf/RetainPtr.h>
#endif
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

#if !OS(WINDOWS)
#include <sys/param.h>
#define MAX_PATH MAXPATHLEN
#endif

namespace WebCore {

const int selectTimeoutMS = 5;
const double pollTimeSeconds = 0.05;
const int maxRunningJobs = 5;

static const bool ignoreSSLErrors = getenv("WEBKIT_IGNORE_SSL_ERRORS");

static CString certificatePath()
{
#if USE(CF)
    CFBundleRef webKitBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.WebKit"));
    if (webKitBundle) {
        RetainPtr<CFURLRef> certURLRef(AdoptCF, CFBundleCopyResourceURL(webKitBundle, CFSTR("cacert"), CFSTR("pem"), CFSTR("certificates")));
        if (certURLRef) {
            char path[MAX_PATH];
            CFURLGetFileSystemRepresentation(certURLRef.get(), false, reinterpret_cast<UInt8*>(path), MAX_PATH);
            return path;
        }
    }
#endif
    char* envPath = getenv("CURL_CA_BUNDLE_PATH");
    if (envPath)
       return envPath;

    return CString();
}

static Mutex* sharedResourceMutex(curl_lock_data data) {
    DEFINE_STATIC_LOCAL(Mutex, cookieMutex, ());
    DEFINE_STATIC_LOCAL(Mutex, dnsMutex, ());
    DEFINE_STATIC_LOCAL(Mutex, shareMutex, ());

    switch (data) {
        case CURL_LOCK_DATA_COOKIE:
            return &cookieMutex;
        case CURL_LOCK_DATA_DNS:
            return &dnsMutex;
        case CURL_LOCK_DATA_SHARE:
            return &shareMutex;
        default:
            ASSERT_NOT_REACHED();
            return NULL;
    }
}

// libcurl does not implement its own thread synchronization primitives.
// these two functions provide mutexes for cookies, and for the global DNS
// cache.
static void curl_lock_callback(CURL* handle, curl_lock_data data, curl_lock_access access, void* userPtr)
{
    if (Mutex* mutex = sharedResourceMutex(data))
        mutex->lock();
}

static void curl_unlock_callback(CURL* handle, curl_lock_data data, void* userPtr)
{
    if (Mutex* mutex = sharedResourceMutex(data))
        mutex->unlock();
}

ResourceHandleManager::ResourceHandleManager()
    : m_downloadTimer(this, &ResourceHandleManager::downloadTimerCallback)
    , m_cookieJarFileName(0)
    , m_certificatePath (certificatePath())
    , m_runningJobs(0)

{
    curl_global_init(CURL_GLOBAL_ALL);
    m_curlMultiHandle = curl_multi_init();
    m_curlShareHandle = curl_share_init();
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_LOCKFUNC, curl_lock_callback);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_UNLOCKFUNC, curl_unlock_callback);
}

ResourceHandleManager::~ResourceHandleManager()
{
    curl_multi_cleanup(m_curlMultiHandle);
    curl_share_cleanup(m_curlShareHandle);
    if (m_cookieJarFileName)
        fastFree(m_cookieJarFileName);
    curl_global_cleanup();
}

void ResourceHandleManager::setCookieJarFileName(const char* cookieJarFileName)
{
    m_cookieJarFileName = fastStrDup(cookieJarFileName);
}

ResourceHandleManager* ResourceHandleManager::sharedInstance()
{
    static ResourceHandleManager* sharedInstance = 0;
    if (!sharedInstance)
        sharedInstance = new ResourceHandleManager();
    return sharedInstance;
}

static void handleLocalReceiveResponse (CURL* handle, ResourceHandle* job, ResourceHandleInternal* d)
{
    // since the code in headerCallback will not have run for local files
    // the code to set the URL and fire didReceiveResponse is never run,
    // which means the ResourceLoader's response does not contain the URL.
    // Run the code here for local files to resolve the issue.
    // TODO: See if there is a better approach for handling this.
     const char* hdr;
     CURLcode err = curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &hdr);
     ASSERT_UNUSED(err, CURLE_OK == err);
     d->m_response.setURL(KURL(ParsedURLString, hdr));
     if (d->client())
         d->client()->didReceiveResponse(job, d->m_response);
     d->m_response.setResponseFired(true);
}


// called with data after all headers have been processed via headerCallback
static size_t writeCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();
    if (d->m_cancelled)
        return 0;

#if LIBCURL_VERSION_NUM > 0x071200
    // We should never be called when deferred loading is activated.
    ASSERT(!d->m_defersLoading);
#endif

    size_t totalSize = size * nmemb;

    // this shouldn't be necessary but apparently is. CURL writes the data
    // of html page even if it is a redirect that was handled internally
    // can be observed e.g. on gmail.com
    CURL* h = d->m_handle;
    long httpCode = 0;
    CURLcode err = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &httpCode);
    if (CURLE_OK == err && httpCode >= 300 && httpCode < 400)
        return totalSize;

    if (!d->m_response.responseFired()) {
        handleLocalReceiveResponse(h, job, d);
        if (d->m_cancelled)
            return 0;
    }

    if (d->client())
        d->client()->didReceiveData(job, static_cast<char*>(ptr), totalSize, 0);
    return totalSize;
}

/*
 * This is being called for each HTTP header in the response. This includes '\r\n'
 * for the last line of the header.
 *
 * We will add each HTTP Header to the ResourceResponse and on the termination
 * of the header (\r\n) we will parse Content-Type and Content-Disposition and
 * update the ResourceResponse and then send it away.
 *
 */
static size_t headerCallback(char* ptr, size_t size, size_t nmemb, void* data)
{
    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();
    if (d->m_cancelled)
        return 0;

#if LIBCURL_VERSION_NUM > 0x071200
    // We should never be called when deferred loading is activated.
    ASSERT(!d->m_defersLoading);
#endif

    size_t totalSize = size * nmemb;
    ResourceHandleClient* client = d->client();

    String header(static_cast<const char*>(ptr), totalSize);

    /*
     * a) We can finish and send the ResourceResponse
     * b) We will add the current header to the HTTPHeaderMap of the ResourceResponse
     *
     * The HTTP standard requires to use \r\n but for compatibility it recommends to
     * accept also \n.
     */
    if (header == String("\r\n") || header == String("\n")) {
        CURL* h = d->m_handle;
        CURLcode err;

        double contentLength = 0;
        err = curl_easy_getinfo(h, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
        d->m_response.setExpectedContentLength(static_cast<long long int>(contentLength));

        const char* hdr;
        err = curl_easy_getinfo(h, CURLINFO_EFFECTIVE_URL, &hdr);
        d->m_response.setURL(KURL(ParsedURLString, hdr));

        long httpCode = 0;
        err = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &httpCode);
        d->m_response.setHTTPStatusCode(httpCode);

        d->m_response.setMimeType(extractMIMETypeFromMediaType(d->m_response.httpHeaderField("Content-Type")));
        d->m_response.setTextEncodingName(extractCharsetFromMediaType(d->m_response.httpHeaderField("Content-Type")));
        d->m_response.setSuggestedFilename(filenameFromHTTPContentDisposition(d->m_response.httpHeaderField("Content-Disposition")));

        // HTTP redirection
        if (httpCode >= 300 && httpCode < 400) {
            String location = d->m_response.httpHeaderField("location");
            if (!location.isEmpty()) {
                KURL newURL = KURL(job->firstRequest().url(), location);

                ResourceRequest redirectedRequest = job->firstRequest();
                redirectedRequest.setURL(newURL);
                if (client)
                    client->willSendRequest(job, redirectedRequest, d->m_response);

                d->m_firstRequest.setURL(newURL);

                return totalSize;
            }
        }

        if (client)
            client->didReceiveResponse(job, d->m_response);
        d->m_response.setResponseFired(true);

    } else {
        int splitPos = header.find(":");
        if (splitPos != -1)
            d->m_response.setHTTPHeaderField(header.left(splitPos), header.substring(splitPos+1).stripWhiteSpace());
    }

    return totalSize;
}

/* This is called to obtain HTTP POST or PUT data.
   Iterate through FormData elements and upload files.
   Carefully respect the given buffer size and fill the rest of the data at the next calls.
*/
size_t readCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();

    if (d->m_cancelled)
        return 0;

#if LIBCURL_VERSION_NUM > 0x071200
    // We should never be called when deferred loading is activated.
    ASSERT(!d->m_defersLoading);
#endif

    if (!size || !nmemb)
        return 0;

    if (!d->m_formDataStream.hasMoreElements())
        return 0;

    size_t sent = d->m_formDataStream.read(ptr, size, nmemb);

    // Something went wrong so cancel the job.
    if (!sent)
        job->cancel();

    return sent;
}

void ResourceHandleManager::downloadTimerCallback(Timer<ResourceHandleManager>* timer)
{
    startScheduledJobs();

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = 0;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = selectTimeoutMS * 1000;       // select waits microseconds

    // Retry 'select' if it was interrupted by a process signal.
    int rc = 0;
    do {
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);
        curl_multi_fdset(m_curlMultiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
        // When the 3 file descriptors are empty, winsock will return -1
        // and bail out, stopping the file download. So make sure we
        // have valid file descriptors before calling select.
        if (maxfd >= 0)
            rc = ::select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    } while (rc == -1 && errno == EINTR);

    if (-1 == rc) {
#ifndef NDEBUG
        perror("bad: select() returned -1: ");
#endif
        return;
    }

    int runningHandles = 0;
    while (curl_multi_perform(m_curlMultiHandle, &runningHandles) == CURLM_CALL_MULTI_PERFORM) { }

    // check the curl messages indicating completed transfers
    // and free their resources
    while (true) {
        int messagesInQueue;
        CURLMsg* msg = curl_multi_info_read(m_curlMultiHandle, &messagesInQueue);
        if (!msg)
            break;

        // find the node which has same d->m_handle as completed transfer
        CURL* handle = msg->easy_handle;
        ASSERT(handle);
        ResourceHandle* job = 0;
        CURLcode err = curl_easy_getinfo(handle, CURLINFO_PRIVATE, &job);
        ASSERT_UNUSED(err, CURLE_OK == err);
        ASSERT(job);
        if (!job)
            continue;
        ResourceHandleInternal* d = job->getInternal();
        ASSERT(d->m_handle == handle);

        if (d->m_cancelled) {
            removeFromCurl(job);
            continue;
        }

        if (CURLMSG_DONE != msg->msg)
            continue;

        if (CURLE_OK == msg->data.result) {
            if (!d->m_response.responseFired()) {
                handleLocalReceiveResponse(d->m_handle, job, d);
                if (d->m_cancelled) {
                    removeFromCurl(job);
                    continue;
                }
            }

            if (d->client())
                d->client()->didFinishLoading(job, 0);
        } else {
            char* url = 0;
            curl_easy_getinfo(d->m_handle, CURLINFO_EFFECTIVE_URL, &url);
#ifndef NDEBUG
            fprintf(stderr, "Curl ERROR for url='%s', error: '%s'\n", url, curl_easy_strerror(msg->data.result));
#endif
            if (d->client())
                d->client()->didFail(job, ResourceError(String(), msg->data.result, String(url), String(curl_easy_strerror(msg->data.result))));
        }

        removeFromCurl(job);
    }

    bool started = startScheduledJobs(); // new jobs might have been added in the meantime

    if (!m_downloadTimer.isActive() && (started || (runningHandles > 0)))
        m_downloadTimer.startOneShot(pollTimeSeconds);
}

void ResourceHandleManager::setProxyInfo(const String& host,
                                         unsigned long port,
                                         ProxyType type,
                                         const String& username,
                                         const String& password)
{
    m_proxyType = type;

    if (!host.length()) {
        m_proxy = emptyString();
    } else {
        String userPass;
        if (username.length() || password.length())
            userPass = username + ":" + password + "@";

        m_proxy = String("http://") + userPass + host + ":" + String::number(port);
    }
}

void ResourceHandleManager::removeFromCurl(ResourceHandle* job)
{
    ResourceHandleInternal* d = job->getInternal();
    ASSERT(d->m_handle);
    if (!d->m_handle)
        return;
    m_runningJobs--;
    curl_multi_remove_handle(m_curlMultiHandle, d->m_handle);
    curl_easy_cleanup(d->m_handle);
    d->m_handle = 0;
    job->deref();
}

void ResourceHandleManager::setupPUT(ResourceHandle*, struct curl_slist**)
{
    notImplemented();
}

/* Calculate the length of the POST.
   Force chunked data transfer if size of files can't be obtained.
 */
void ResourceHandleManager::setupPOST(ResourceHandle* job, struct curl_slist** headers)
{
    ResourceHandleInternal* d = job->getInternal();
    curl_easy_setopt(d->m_handle, CURLOPT_POST, TRUE);
    curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDSIZE, 0);

    if (!job->firstRequest().httpBody())
        return;

    Vector<FormDataElement> elements = job->firstRequest().httpBody()->elements();
    size_t numElements = elements.size();
    if (!numElements)
        return;

    // Do not stream for simple POST data
    if (numElements == 1) {
        job->firstRequest().httpBody()->flatten(d->m_postBytes);
        if (d->m_postBytes.size() != 0) {
            curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDSIZE, d->m_postBytes.size());
            curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDS, d->m_postBytes.data());
        }
        return;
    }

    // Obtain the total size of the POST
    // The size of a curl_off_t could be different in WebKit and in cURL depending on
    // compilation flags of both. For CURLOPT_POSTFIELDSIZE_LARGE we have to pass the
    // right size or random data will be used as the size.
    static int expectedSizeOfCurlOffT = 0;
    if (!expectedSizeOfCurlOffT) {
        curl_version_info_data *infoData = curl_version_info(CURLVERSION_NOW);
        if (infoData->features & CURL_VERSION_LARGEFILE)
            expectedSizeOfCurlOffT = sizeof(long long);
        else
            expectedSizeOfCurlOffT = sizeof(int);
    }

#if COMPILER(MSVC)
    // work around compiler error in Visual Studio 2005.  It can't properly
    // handle math with 64-bit constant declarations.
#pragma warning(disable: 4307)
#endif
    static const long long maxCurlOffT = (1LL << (expectedSizeOfCurlOffT * 8 - 1)) - 1;
    curl_off_t size = 0;
    bool chunkedTransfer = false;
    for (size_t i = 0; i < numElements; i++) {
        FormDataElement element = elements[i];
        if (element.m_type == FormDataElement::encodedFile) {
            long long fileSizeResult;
            if (getFileSize(element.m_filename, fileSizeResult)) {
                if (fileSizeResult > maxCurlOffT) {
                    // File size is too big for specifying it to cURL
                    chunkedTransfer = true;
                    break;
                }
                size += fileSizeResult;
            } else {
                chunkedTransfer = true;
                break;
            }
        } else
            size += elements[i].m_data.size();
    }

    // cURL guesses that we want chunked encoding as long as we specify the header
    if (chunkedTransfer)
        *headers = curl_slist_append(*headers, "Transfer-Encoding: chunked");
    else {
        if (sizeof(long long) == expectedSizeOfCurlOffT)
          curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDSIZE_LARGE, (long long)size);
        else
          curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDSIZE_LARGE, (int)size);
    }

    curl_easy_setopt(d->m_handle, CURLOPT_READFUNCTION, readCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_READDATA, job);
}

void ResourceHandleManager::add(ResourceHandle* job)
{
    // we can be called from within curl, so to avoid re-entrancy issues
    // schedule this job to be added the next time we enter curl download loop
    job->ref();
    m_resourceHandleList.append(job);
    if (!m_downloadTimer.isActive())
        m_downloadTimer.startOneShot(pollTimeSeconds);
}

bool ResourceHandleManager::removeScheduledJob(ResourceHandle* job)
{
    int size = m_resourceHandleList.size();
    for (int i = 0; i < size; i++) {
        if (job == m_resourceHandleList[i]) {
            m_resourceHandleList.remove(i);
            job->deref();
            return true;
        }
    }
    return false;
}

bool ResourceHandleManager::startScheduledJobs()
{
    // TODO: Create a separate stack of jobs for each domain.

    bool started = false;
    while (!m_resourceHandleList.isEmpty() && m_runningJobs < maxRunningJobs) {
        ResourceHandle* job = m_resourceHandleList[0];
        m_resourceHandleList.remove(0);
        startJob(job);
        started = true;
    }
    return started;
}

void ResourceHandleManager::dispatchSynchronousJob(ResourceHandle* job)
{
    KURL kurl = job->firstRequest().url();

    if (kurl.protocolIsData()) {
        handleDataURL(job);
        return;
    }

    ResourceHandleInternal* handle = job->getInternal();

#if LIBCURL_VERSION_NUM > 0x071200
    // If defersLoading is true and we call curl_easy_perform
    // on a paused handle, libcURL would do the transfert anyway
    // and we would assert so force defersLoading to be false.
    handle->m_defersLoading = false;
#endif

    initializeHandle(job);

    // curl_easy_perform blocks until the transfert is finished.
    CURLcode ret =  curl_easy_perform(handle->m_handle);

    if (ret != 0) {
        ResourceError error(String(handle->m_url), ret, String(handle->m_url), String(curl_easy_strerror(ret)));
        handle->client()->didFail(job, error);
    }

    curl_easy_cleanup(handle->m_handle);
}

void ResourceHandleManager::startJob(ResourceHandle* job)
{
    KURL kurl = job->firstRequest().url();

    if (kurl.protocolIsData()) {
        handleDataURL(job);
        return;
    }

    initializeHandle(job);

    m_runningJobs++;
    CURLMcode ret = curl_multi_add_handle(m_curlMultiHandle, job->getInternal()->m_handle);
    // don't call perform, because events must be async
    // timeout will occur and do curl_multi_perform
    if (ret && ret != CURLM_CALL_MULTI_PERFORM) {
#ifndef NDEBUG
        fprintf(stderr, "Error %d starting job %s\n", ret, encodeWithURLEscapeSequences(job->firstRequest().url().string()).latin1().data());
#endif
        job->cancel();
        return;
    }
}

void ResourceHandleManager::initializeHandle(ResourceHandle* job)
{
    KURL kurl = job->firstRequest().url();

    // Remove any fragment part, otherwise curl will send it as part of the request.
    kurl.removeFragmentIdentifier();

    ResourceHandleInternal* d = job->getInternal();
    String url = kurl.string();

    if (kurl.isLocalFile()) {
        String query = kurl.query();
        // Remove any query part sent to a local file.
        if (!query.isEmpty()) {
            int queryIndex = url.find(query);
            if (queryIndex != -1)
                url = url.left(queryIndex - 1);
        }
        // Determine the MIME type based on the path.
        d->m_response.setMimeType(MIMETypeRegistry::getMIMETypeForPath(url));
    }

    d->m_handle = curl_easy_init();

#if LIBCURL_VERSION_NUM > 0x071200
    if (d->m_defersLoading) {
        CURLcode error = curl_easy_pause(d->m_handle, CURLPAUSE_ALL);
        // If we did not pause the handle, we would ASSERT in the
        // header callback. So just assert here.
        ASSERT_UNUSED(error, error == CURLE_OK);
    }
#endif
#ifndef NDEBUG
    if (getenv("DEBUG_CURL"))
        curl_easy_setopt(d->m_handle, CURLOPT_VERBOSE, 1);
#endif
    curl_easy_setopt(d->m_handle, CURLOPT_PRIVATE, job);
    curl_easy_setopt(d->m_handle, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEDATA, job);
    curl_easy_setopt(d->m_handle, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEHEADER, job);
    curl_easy_setopt(d->m_handle, CURLOPT_AUTOREFERER, 1);
    curl_easy_setopt(d->m_handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(d->m_handle, CURLOPT_MAXREDIRS, 10);
    curl_easy_setopt(d->m_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(d->m_handle, CURLOPT_SHARE, m_curlShareHandle);
    curl_easy_setopt(d->m_handle, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5); // 5 minutes
    // FIXME: Enable SSL verification when we have a way of shipping certs
    // and/or reporting SSL errors to the user.
    if (ignoreSSLErrors)
        curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYPEER, false);

    if (!m_certificatePath.isNull())
       curl_easy_setopt(d->m_handle, CURLOPT_CAINFO, m_certificatePath.data());

    // enable gzip and deflate through Accept-Encoding:
    curl_easy_setopt(d->m_handle, CURLOPT_ENCODING, "");

    // url must remain valid through the request
    ASSERT(!d->m_url);

    // url is in ASCII so latin1() will only convert it to char* without character translation.
    d->m_url = fastStrDup(url.latin1().data());
    curl_easy_setopt(d->m_handle, CURLOPT_URL, d->m_url);

    if (m_cookieJarFileName) {
        curl_easy_setopt(d->m_handle, CURLOPT_COOKIEFILE, m_cookieJarFileName);
        curl_easy_setopt(d->m_handle, CURLOPT_COOKIEJAR, m_cookieJarFileName);
    }

    struct curl_slist* headers = 0;
    if (job->firstRequest().httpHeaderFields().size() > 0) {
        HTTPHeaderMap customHeaders = job->firstRequest().httpHeaderFields();
        HTTPHeaderMap::const_iterator end = customHeaders.end();
        for (HTTPHeaderMap::const_iterator it = customHeaders.begin(); it != end; ++it) {
            String key = it->first;
            String value = it->second;
            String headerString(key);
            headerString.append(": ");
            headerString.append(value);
            CString headerLatin1 = headerString.latin1();
            headers = curl_slist_append(headers, headerLatin1.data());
        }
    }

    if ("GET" == job->firstRequest().httpMethod())
        curl_easy_setopt(d->m_handle, CURLOPT_HTTPGET, TRUE);
    else if ("POST" == job->firstRequest().httpMethod())
        setupPOST(job, &headers);
    else if ("PUT" == job->firstRequest().httpMethod())
        setupPUT(job, &headers);
    else if ("HEAD" == job->firstRequest().httpMethod())
        curl_easy_setopt(d->m_handle, CURLOPT_NOBODY, TRUE);

    if (headers) {
        curl_easy_setopt(d->m_handle, CURLOPT_HTTPHEADER, headers);
        d->m_customHeaders = headers;
    }
    // curl CURLOPT_USERPWD expects username:password
    if (d->m_user.length() || d->m_pass.length()) {
        String userpass = d->m_user + ":" + d->m_pass;
        curl_easy_setopt(d->m_handle, CURLOPT_USERPWD, userpass.utf8().data());
    }

    // Set proxy options if we have them.
    if (m_proxy.length()) {
        curl_easy_setopt(d->m_handle, CURLOPT_PROXY, m_proxy.utf8().data());
        curl_easy_setopt(d->m_handle, CURLOPT_PROXYTYPE, m_proxyType);
    }
}

void ResourceHandleManager::cancel(ResourceHandle* job)
{
    if (removeScheduledJob(job))
        return;

    ResourceHandleInternal* d = job->getInternal();
    d->m_cancelled = true;
    if (!m_downloadTimer.isActive())
        m_downloadTimer.startOneShot(pollTimeSeconds);
}

} // namespace WebCore
