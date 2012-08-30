/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef Platform_h
#define Platform_h

#ifdef WIN32
#include <windows.h>
#endif

#include "WebAudioDevice.h"
#include "WebCommon.h"
#include "WebData.h"
#include "WebGamepads.h"
#include "WebGraphicsContext3D.h"
#include "WebLocalizedString.h"
#include "WebString.h"
#include "WebVector.h"

namespace WebKit {

class WebAudioBus;
class WebBlobRegistry;
class WebClipboard;
class WebCompositorSupport;
class WebCookieJar;
class WebFileSystem;
class WebFileUtilities;
class WebFlingAnimator;
class WebMediaStreamCenter;
class WebMediaStreamCenterClient;
class WebMessagePortChannel;
class WebMimeRegistry;
class WebPeerConnection00Handler;
class WebPeerConnection00HandlerClient;
class WebRTCPeerConnectionHandler;
class WebRTCPeerConnectionHandlerClient;
class WebSandboxSupport;
class WebSocketStreamHandle;
class WebStorageNamespace;
class WebThemeEngine;
class WebThread;
class WebURL;
class WebURLLoader;
class WebWorkerRunLoop;
struct WebLocalizedString;

class Platform {
public:
    // HTML5 Database ------------------------------------------------------

#ifdef WIN32
    typedef HANDLE FileHandle;
#else
    typedef int FileHandle;
#endif

    WEBKIT_EXPORT static void initialize(Platform*);
    WEBKIT_EXPORT static void shutdown();
    WEBKIT_EXPORT static Platform* current();

    // May return null.
    virtual WebCookieJar* cookieJar() { return 0; }

    // Must return non-null.
    virtual WebClipboard* clipboard() { return 0; }

    // Must return non-null.
    virtual WebFileUtilities* fileUtilities() { return 0; }

    // Must return non-null.
    virtual WebMimeRegistry* mimeRegistry() { return 0; }

    // May return null if sandbox support is not necessary
    virtual WebSandboxSupport* sandboxSupport() { return 0; }

    // May return null on some platforms.
    virtual WebThemeEngine* themeEngine() { return 0; }


    // Audio --------------------------------------------------------------

    virtual double audioHardwareSampleRate() { return 0; }
    virtual size_t audioHardwareBufferSize() { return 0; }
    virtual WebAudioDevice* createAudioDevice(size_t bufferSize, unsigned numberOfChannels, double sampleRate, WebAudioDevice::RenderCallback*) { return 0; }


    // Blob ----------------------------------------------------------------

    // Must return non-null.
    virtual WebBlobRegistry* blobRegistry() { return 0; }


    // Database ------------------------------------------------------------

    // Opens a database file; dirHandle should be 0 if the caller does not need
    // a handle to the directory containing this file
    virtual FileHandle databaseOpenFile(const WebString& vfsFileName, int desiredFlags) { return FileHandle(); }

    // Deletes a database file and returns the error code
    virtual int databaseDeleteFile(const WebString& vfsFileName, bool syncDir) { return 0; }

    // Returns the attributes of the given database file
    virtual long databaseGetFileAttributes(const WebString& vfsFileName) { return 0; }

    // Returns the size of the given database file
    virtual long long databaseGetFileSize(const WebString& vfsFileName) { return 0; }

    // Returns the space available for the given origin
    virtual long long databaseGetSpaceAvailableForOrigin(const WebKit::WebString& originIdentifier) { return 0; }


    // DOM Storage --------------------------------------------------

    // Return a LocalStorage namespace that corresponds to the following path.
    virtual WebStorageNamespace* createLocalStorageNamespace(const WebString& path, unsigned quota) { return 0; }


    // FileSystem ----------------------------------------------------------

    // Must return non-null.
    virtual WebFileSystem* fileSystem() { return 0; }


    // Gamepad -------------------------------------------------------------

    virtual void sampleGamepads(WebGamepads& into) { into.length = 0; }


    // History -------------------------------------------------------------

    // Returns the hash for the given canonicalized URL for use in visited
    // link coloring.
    virtual unsigned long long visitedLinkHash(
        const char* canonicalURL, size_t length) { return 0; }

    // Returns whether the given link hash is in the user's history. The
    // hash must have been generated by calling VisitedLinkHash().
    virtual bool isLinkVisited(unsigned long long linkHash) { return false; }


    // Hyphenation ---------------------------------------------------------

    // Returns whether we can support hyphenation for the given locale.
    virtual bool canHyphenate(const WebString& locale) { return false; }

    // Returns the last position where we can add a hyphen before the given position.
    virtual size_t computeLastHyphenLocation(const WebUChar* characters, size_t length, size_t beforeIndex, const WebString& locale) { return 0; }


    // Keygen --------------------------------------------------------------

    // Handle the <keygen> tag for generating client certificates
    // Returns a base64 encoded signed copy of a public key from a newly
    // generated key pair and the supplied challenge string. keySizeindex
    // specifies the strength of the key.
    virtual WebString signedPublicKeyAndChallengeString(unsigned keySizeIndex,
                                                        const WebString& challenge,
                                                        const WebURL& url) { return WebString(); }


    // Memory --------------------------------------------------------------

    // Returns the current space allocated for the pagefile, in MB.
    // That is committed size for Windows and virtual memory size for POSIX
    virtual size_t memoryUsageMB() { return 0; }

    // Same as above, but always returns actual value, without any caches.
    virtual size_t actualMemoryUsageMB() { return 0; }

    // If memory usage is below this threshold, do not bother forcing GC.
    virtual size_t lowMemoryUsageMB() { return 256; }

    // If memory usage is above this threshold, force GC more aggressively.
    virtual size_t highMemoryUsageMB() { return 1024; }

    // Delta of memory usage growth (vs. last actualMemoryUsageMB()) to force GC when memory usage is high.
    virtual size_t highUsageDeltaMB() { return 128; }

    // Returns private and shared usage, in bytes. Private bytes is the amount of
    // memory currently allocated to this process that cannot be shared. Returns
    // false on platform specific error conditions.
    virtual bool processMemorySizesInBytes(size_t* privateBytes, size_t* sharedBytes) { return false; }


    // Message Ports -------------------------------------------------------

    // Creates a Message Port Channel. This can be called on any thread.
    // The returned object should only be used on the thread it was created on.
    virtual WebMessagePortChannel* createMessagePortChannel() { return 0; }


    // Network -------------------------------------------------------------

    // Returns a new WebURLLoader instance.
    virtual WebURLLoader* createURLLoader() { return 0; }

    // A suggestion to prefetch IP information for the given hostname.
    virtual void prefetchHostName(const WebString&) { }

    // Returns a new WebSocketStreamHandle instance.
    virtual WebSocketStreamHandle* createSocketStreamHandle() { return 0; }

    // Returns the User-Agent string that should be used for the given URL.
    virtual WebString userAgent(const WebURL&) { return WebString(); }

    // A suggestion to cache this metadata in association with this URL.
    virtual void cacheMetadata(const WebURL&, double responseTime, const char* data, size_t dataSize) { }


    // Resources -----------------------------------------------------------

    // Returns a localized string resource (with substitution parameters).
    virtual WebString queryLocalizedString(WebLocalizedString::Name) { return WebString(); }
    virtual WebString queryLocalizedString(WebLocalizedString::Name, const WebString& parameter) { return WebString(); }
    virtual WebString queryLocalizedString(WebLocalizedString::Name, const WebString& parameter1, const WebString& parameter2) { return WebString(); }


    // Threads -------------------------------------------------------

    // Creates an embedder-defined thread.
    virtual WebThread* createThread(const char* name) { return 0; }

    // Returns an interface to the current thread. This is owned by the
    // embedder.
    virtual WebThread* currentThread() { return 0; }


    // Profiling -----------------------------------------------------------

    virtual void decrementStatsCounter(const char* name) { }
    virtual void incrementStatsCounter(const char* name) { }


    // Resources -----------------------------------------------------------

    // Returns a blob of data corresponding to the named resource.
    virtual WebData loadResource(const char* name) { return WebData(); }

    // Decodes the in-memory audio file data and returns the linear PCM audio data in the destinationBus.
    // A sample-rate conversion to sampleRate will occur if the file data is at a different sample-rate.
    // Returns true on success.
    virtual bool loadAudioResource(WebAudioBus* destinationBus, const char* audioFileData, size_t dataSize, double sampleRate) { return false; }


    // Sandbox ------------------------------------------------------------

    // In some browsers, a "sandbox" restricts what operations a program
    // is allowed to preform. Such operations are typically abstracted out
    // via this API, but sometimes (like in HTML 5 database opening) WebKit
    // needs to behave differently based on whether it's restricted or not.
    // In these cases (and these cases only) you can call this function.
    // It's OK for this value to be conservitive (i.e. true even if the
    // sandbox isn't active).
    virtual bool sandboxEnabled() { return false; }


    // Screen -------------------------------------------------------------

    // Supplies the system monitor color profile.
    virtual void screenColorProfile(WebVector<char>* profile) { }


    // Sudden Termination --------------------------------------------------

    // Disable/Enable sudden termination.
    virtual void suddenTerminationChanged(bool enabled) { }


    // System --------------------------------------------------------------

    // Returns a value such as "en-US".
    virtual WebString defaultLocale() { return WebString(); }

    // Wall clock time in seconds since the epoch.
    virtual double currentTime() { return 0; }

    // Monotonically increasing time in seconds from an arbitrary fixed point in the past.
    // This function is expected to return at least millisecond-precision values. For this reason,
    // it is recommended that the fixed point be no further in the past than the epoch.
    virtual double monotonicallyIncreasingTime() { return 0; }

    // WebKit clients must implement this funcion if they use cryptographic randomness.
    virtual void cryptographicallyRandomValues(unsigned char* buffer, size_t length) = 0;

    // Delayed work is driven by a shared timer.
    typedef void (*SharedTimerFunction)();
    virtual void setSharedTimerFiredFunction(SharedTimerFunction timerFunction) { }
    virtual void setSharedTimerFireInterval(double) { }
    virtual void stopSharedTimer() { }

    // Callable from a background WebKit thread.
    virtual void callOnMainThread(void (*func)(void*), void* context) { }


    // Tracing -------------------------------------------------------------

    // Get a pointer to the enabled state of the given trace category. The
    // embedder can dynamically change the enabled state as trace event
    // recording is started and stopped by the application. Only long-lived
    // literal strings should be given as the category name. The implementation
    // expects the returned pointer to be held permanently in a local static. If
    // the unsigned char is non-zero, tracing is enabled. If tracing is enabled,
    // addTraceEvent is expected to be called by the trace event macros.
    virtual const unsigned char* getTraceCategoryEnabledFlag(const char* categoryName) { return 0; }

    // Add a trace event to the platform tracing system. Depending on the actual
    // enabled state, this event may be recorded or dropped. Returns
    // thresholdBeginId for use in a corresponding end addTraceEvent call.
    // - phase specifies the type of event:
    //   - BEGIN ('B'): Marks the beginning of a scoped event.
    //   - END ('E'): Marks the end of a scoped event.
    //   - INSTANT ('I'): Standalone, instantaneous event.
    //   - START ('S'): Marks the beginning of an asynchronous event (the end
    //     event can occur in a different scope or thread). The id parameter is
    //     used to match START/FINISH pairs.
    //   - FINISH ('F'): Marks the end of an asynchronous event.
    //   - COUNTER ('C'): Used to trace integer quantities that change over
    //     time. The argument values are expected to be of type int.
    //   - METADATA ('M'): Reserved for internal use.
    // - categoryEnabled is the pointer returned by getTraceCategoryEnabledFlag.
    // - name is the name of the event. Also used to match BEGIN/END and
    //   START/FINISH pairs.
    // - id optionally allows events of the same name to be distinguished from
    //   each other. For example, to trace the consutruction and destruction of
    //   objects, specify the pointer as the id parameter.
    // - numArgs specifies the number of elements in argNames, argTypes, and
    //   argValues.
    // - argNames is the array of argument names. Use long-lived literal strings
    //   or specify the COPY flag.
    // - argTypes is the array of argument types:
    //   - BOOL (1): bool
    //   - UINT (2): unsigned long long
    //   - INT (3): long long
    //   - DOUBLE (4): double
    //   - POINTER (5): void*
    //   - STRING (6): char* (long-lived null-terminated char* string)
    //   - COPY_STRING (7): char* (temporary null-terminated char* string)
    // - argValues is the array of argument values. Each value is the unsigned
    //   long long member of a union of all supported types.
    // - thresholdBeginId optionally specifies the value returned by a previous
    //   call to addTraceEvent with a BEGIN phase.
    // - threshold is used on an END phase event in conjunction with the
    //   thresholdBeginId of a prior BEGIN event. The threshold is the minimum
    //   number of microseconds that must have passed since the BEGIN event. If
    //   less than threshold microseconds has passed, the BEGIN/END pair is
    //   dropped.
    // - flags can be 0 or one or more of the following, ORed together:
    //   - COPY (0x1): treat all strings (name, argNames and argValues of type
    //     string) as temporary so that they will be copied by addTraceEvent.
    //   - HAS_ID (0x2): use the id argument to uniquely identify the event for
    //     matching with other events of the same name.
    //   - MANGLE_ID (0x4): specify this flag if the id parameter is the value
    //     of a pointer.
    virtual int addTraceEvent(
        char phase,
        const unsigned char* categoryEnabledFlag,
        const char* name,
        unsigned long long id,
        int numArgs,
        const char** argNames,
        const unsigned char* argTypes,
        const unsigned long long* argValues,
        int thresholdBeginId,
        long long threshold,
        unsigned char flags) { return -1; }

    // Callbacks for reporting histogram data.
    // CustomCounts histogram has exponential bucket sizes, so that min=1, max=1000000, bucketCount=50 would do.
    virtual void histogramCustomCounts(const char* name, int sample, int min, int max, int bucketCount) { }
    // Enumeration histogram buckets are linear, boundaryValue should be larger than any possible sample value.
    virtual void histogramEnumeration(const char* name, int sample, int boundaryValue) { }


    // GPU ----------------------------------------------------------------
    //
    // May return null if GPU is not supported.
    // Returns newly allocated and initialized offscreen WebGraphicsContext3D instance.
    virtual WebGraphicsContext3D* createOffscreenGraphicsContext3D(const WebGraphicsContext3D::Attributes&) { return 0; }

    // Returns true if the platform is capable of producing an offscreen context suitable for accelerating 2d canvas.
    // This will return false if the platform cannot promise that contexts will be preserved across operations like
    // locking the screen or if the platform cannot provide a context with suitable performance characteristics.
    //
    // This value must be checked again after a context loss event as the platform's capabilities may have changed.
    virtual bool canAccelerate2dCanvas() { return false; }

    virtual WebCompositorSupport* compositorSupport() { return 0; }

    virtual WebFlingAnimator* createFlingAnimator() { return 0; }

    // WebRTC ----------------------------------------------------------

    // DEPRECATED
    // Creates an WebPeerConnection00Handler for PeerConnection00.
    // This is an highly experimental feature not yet in the WebRTC standard.
    // May return null if WebRTC functionality is not avaliable or out of resources.
    virtual WebPeerConnection00Handler* createPeerConnection00Handler(WebPeerConnection00HandlerClient*) { return 0; }

    // Creates an WebRTCPeerConnectionHandler for RTCPeerConnection.
    // May return null if WebRTC functionality is not avaliable or out of resources.
    virtual WebRTCPeerConnectionHandler* createRTCPeerConnectionHandler(WebRTCPeerConnectionHandlerClient*) { return 0; }

    // May return null if WebRTC functionality is not avaliable or out of resources.
    virtual WebMediaStreamCenter* createMediaStreamCenter(WebMediaStreamCenterClient*) { return 0; }


    // WebWorker ----------------------------------------------------------

    virtual void didStartWorkerRunLoop(const WebWorkerRunLoop&) { }
    virtual void didStopWorkerRunLoop(const WebWorkerRunLoop&) { }

protected:
    virtual ~Platform() { }
};

} // namespace WebKit

#endif
