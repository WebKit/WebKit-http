/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DocumentLoader_h
#define DocumentLoader_h

#include "CachedRawResourceClient.h"
#include "CachedResourceHandle.h"
#include "DocumentLoadTiming.h"
#include "DocumentWriter.h"
#include "IconDatabaseBase.h"
#include "NavigationAction.h"
#include "ResourceError.h"
#include "ResourceLoaderOptions.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "StringWithDirection.h"
#include "SubstituteData.h"
#include "Timer.h"
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

#if HAVE(RUNLOOP_TIMER)
#include <wtf/RunLoopTimer.h>
#endif

#if USE(QUICK_LOOK)
#include "QuickLook.h"
#endif

namespace WebCore {

    class ApplicationCacheHost;
    class Archive;
    class ArchiveResource;
    class ArchiveResourceCollection;
    class CachedRawResource;
    class CachedResourceLoader;
    class ContentFilter;
    class FormState;
    class Frame;
    class FrameLoader;
    class Page;
    class ResourceBuffer;
    class ResourceLoader;
    class SharedBuffer;
    class SubstituteResource;

    typedef HashMap<unsigned long, RefPtr<ResourceLoader>> ResourceLoaderMap;
    typedef Vector<ResourceResponse> ResponseVector;

    class DocumentLoader : public RefCounted<DocumentLoader>, private CachedRawResourceClient {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassRefPtr<DocumentLoader> create(const ResourceRequest& request, const SubstituteData& data)
        {
            return adoptRef(new DocumentLoader(request, data));
        }
        WEBCORE_EXPORT virtual ~DocumentLoader();

        WEBCORE_EXPORT void setFrame(Frame*);
        Frame* frame() const { return m_frame; }

        WEBCORE_EXPORT virtual void attachToFrame();
        WEBCORE_EXPORT virtual void detachFromFrame();

        WEBCORE_EXPORT FrameLoader* frameLoader() const;
        WEBCORE_EXPORT ResourceLoader* mainResourceLoader() const;
        WEBCORE_EXPORT PassRefPtr<ResourceBuffer> mainResourceData() const;
        
        DocumentWriter& writer() const { return m_writer; }

        WEBCORE_EXPORT const ResourceRequest& originalRequest() const;
        WEBCORE_EXPORT const ResourceRequest& originalRequestCopy() const;

        WEBCORE_EXPORT const ResourceRequest& request() const;
        WEBCORE_EXPORT ResourceRequest& request();

        CachedResourceLoader& cachedResourceLoader() { return m_cachedResourceLoader.get(); }

        const SubstituteData& substituteData() const { return m_substituteData; }

        // FIXME: This is the same as requestURL(). We should remove one of them.
        WEBCORE_EXPORT const URL& url() const;
        WEBCORE_EXPORT const URL& unreachableURL() const;

        const URL& originalURL() const;
        WEBCORE_EXPORT const URL& requestURL() const;
        WEBCORE_EXPORT const URL& responseURL() const;
        WEBCORE_EXPORT const String& responseMIMEType() const;
#if PLATFORM(IOS)
        // FIXME: This method seems to violate the encapsulation of this class.
        WEBCORE_EXPORT void setResponseMIMEType(const String&);
#endif

        void replaceRequestURLForSameDocumentNavigation(const URL&);
        bool isStopping() const { return m_isStopping; }
        void stopLoading();
        void setCommitted(bool committed) { m_committed = committed; }
        bool isCommitted() const { return m_committed; }
        WEBCORE_EXPORT bool isLoading() const;

        const ResourceError& mainDocumentError() const { return m_mainDocumentError; }

        const ResourceResponse& response() const { return m_response; }
#if PLATFORM(IOS)
        // FIXME: This method seems to violate the encapsulation of this class.
        void setResponse(const ResourceResponse& response) { m_response = response; }
#endif

        bool isClientRedirect() const { return m_isClientRedirect; }
        void setIsClientRedirect(bool isClientRedirect) { m_isClientRedirect = isClientRedirect; }
        void handledOnloadEvents();
        bool wasOnloadHandled() { return m_wasOnloadHandled; }
        WEBCORE_EXPORT bool isLoadingInAPISense() const;
        WEBCORE_EXPORT void setTitle(const StringWithDirection&);
        const String& overrideEncoding() const { return m_overrideEncoding; }

#if PLATFORM(MAC)
        void schedule(SchedulePair&);
        void unschedule(SchedulePair&);
#endif

#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
        void setArchive(PassRefPtr<Archive>);
        WEBCORE_EXPORT void addAllArchiveResources(Archive*);
        WEBCORE_EXPORT void addArchiveResource(PassRefPtr<ArchiveResource>);
        PassRefPtr<Archive> popArchiveForSubframe(const String& frameName, const URL&);
        WEBCORE_EXPORT SharedBuffer* parsedArchiveData() const;

        WEBCORE_EXPORT bool scheduleArchiveLoad(ResourceLoader*, const ResourceRequest&);
#endif

        // Return the ArchiveResource for the URL only when loading an Archive
        ArchiveResource* archiveResourceForURL(const URL&) const;

        WEBCORE_EXPORT PassRefPtr<ArchiveResource> mainResource() const;

        // Return an ArchiveResource for the URL, either creating from live data or
        // pulling from the ArchiveResourceCollection
        WEBCORE_EXPORT PassRefPtr<ArchiveResource> subresource(const URL&) const;
        WEBCORE_EXPORT Vector<RefPtr<ArchiveResource>> subresources() const;

#ifndef NDEBUG
        bool isSubstituteLoadPending(ResourceLoader*) const;
#endif
        void cancelPendingSubstituteLoad(ResourceLoader*);   
        
        void addResponse(const ResourceResponse&);
        const ResponseVector& responses() const { return m_responses; }

        const NavigationAction& triggeringAction() const { return m_triggeringAction; }
        void setTriggeringAction(const NavigationAction& action) { m_triggeringAction = action; }
        void setOverrideEncoding(const String& encoding) { m_overrideEncoding = encoding; }
        void setLastCheckedRequest(const ResourceRequest& request) { m_lastCheckedRequest = request; }
        const ResourceRequest& lastCheckedRequest()  { return m_lastCheckedRequest; }

        void stopRecordingResponses();
        const StringWithDirection& title() const { return m_pageTitle; }

        WEBCORE_EXPORT URL urlForHistory() const;
        WEBCORE_EXPORT bool urlForHistoryReflectsFailure() const;

        // These accessors accommodate WebCore's somewhat fickle custom of creating history
        // items for redirects, but only sometimes. For "source" and "destination",
        // these accessors return the URL that would have been used if a history
        // item were created. This allows WebKit to link history items reflecting
        // redirects into a chain from start to finish.
        String clientRedirectSourceForHistory() const { return m_clientRedirectSourceForHistory; } // null if no client redirect occurred.
        String clientRedirectDestinationForHistory() const { return urlForHistory(); }
        void setClientRedirectSourceForHistory(const String& clientRedirectSourceForHistory) { m_clientRedirectSourceForHistory = clientRedirectSourceForHistory; }
        
        String serverRedirectSourceForHistory() const { return (urlForHistory() == url() || url() == blankURL()) ? String() : urlForHistory().string(); } // null if no server redirect occurred.
        String serverRedirectDestinationForHistory() const { return url(); }

        bool didCreateGlobalHistoryEntry() const { return m_didCreateGlobalHistoryEntry; }
        void setDidCreateGlobalHistoryEntry(bool didCreateGlobalHistoryEntry) { m_didCreateGlobalHistoryEntry = didCreateGlobalHistoryEntry; }

        bool subresourceLoadersArePageCacheAcceptable() const { return m_subresourceLoadersArePageCacheAcceptable; }

        void setDefersLoading(bool);
        void setMainResourceDataBufferingPolicy(DataBufferingPolicy);

        void startLoadingMainResource();
        WEBCORE_EXPORT void cancelMainResourceLoad(const ResourceError&);
        
        // Support iconDatabase in synchronous mode.
        void iconLoadDecisionAvailable();
        
        // Support iconDatabase in asynchronous mode.
        void continueIconLoadWithDecision(IconLoadDecision);
        void getIconLoadDecisionForIconURL(const String&);
        void getIconDataForIconURL(const String&);

        bool isLoadingMainResource() const { return m_loadingMainResource; }
        bool isLoadingMultipartContent() const { return m_isLoadingMultipartContent; }

        void stopLoadingPlugIns();
        void stopLoadingSubresources();

        void addSubresourceLoader(ResourceLoader*);
        void removeSubresourceLoader(ResourceLoader*);
        WEBCORE_EXPORT void addPlugInStreamLoader(ResourceLoader*);
        WEBCORE_EXPORT void removePlugInStreamLoader(ResourceLoader*);

        void subresourceLoaderFinishedLoadingOnePart(ResourceLoader*);

        void setDeferMainResourceDataLoad(bool defer) { m_deferMainResourceDataLoad = defer; }
        
        void didTellClientAboutLoad(const String& url)
        { 
#if !PLATFORM(COCOA)
            // Don't include data urls here, as if a lot of data is loaded
            // that way, we hold on to the (large) url string for too long.
            if (protocolIs(url, "data"))
                return;
#endif
            if (!url.isEmpty())
                m_resourcesClientKnowsAbout.add(url);
        }
        bool haveToldClientAboutLoad(const String& url) { return m_resourcesClientKnowsAbout.contains(url); }
        void recordMemoryCacheLoadForFutureClientNotification(const ResourceRequest&);
        void takeMemoryCacheLoadsForClientNotification(Vector<ResourceRequest>& loads);

        DocumentLoadTiming* timing() { return &m_documentLoadTiming; }
        void resetTiming() { m_documentLoadTiming = DocumentLoadTiming(); }

        // The WebKit layer calls this function when it's ready for the data to
        // actually be added to the document.
        WEBCORE_EXPORT void commitData(const char* bytes, size_t length);

        ApplicationCacheHost* applicationCacheHost() const { return m_applicationCacheHost.get(); }

        void checkLoadComplete();

        // The URL of the document resulting from this DocumentLoader.
        URL documentURL() const;

#if USE(QUICK_LOOK)
        void setQuickLookHandle(std::unique_ptr<QuickLookHandle> quickLookHandle) { m_quickLookHandle = WTF::move(quickLookHandle); }
        QuickLookHandle* quickLookHandle() const { return m_quickLookHandle.get(); }
#endif

    protected:
        WEBCORE_EXPORT DocumentLoader(const ResourceRequest&, const SubstituteData&);

        bool m_deferMainResourceDataLoad;

    private:
        Document* document() const;

        void setRequest(const ResourceRequest&);

        void commitIfReady();
        void setMainDocumentError(const ResourceError&);
        void commitLoad(const char*, int);
        void clearMainResourceLoader();

        void setupForReplace();
        void maybeFinishLoadingMultipartContent();
        
        bool maybeCreateArchive();
#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
        void clearArchiveResources();
#endif

        void willSendRequest(ResourceRequest&, const ResourceResponse&);
        void finishedLoading(double finishTime);
        void mainReceivedError(const ResourceError&);
        WEBCORE_EXPORT virtual void redirectReceived(CachedResource*, ResourceRequest&, const ResourceResponse&) override;
        WEBCORE_EXPORT virtual void responseReceived(CachedResource*, const ResourceResponse&) override;
        WEBCORE_EXPORT virtual void dataReceived(CachedResource*, const char* data, int length) override;
        WEBCORE_EXPORT virtual void notifyFinished(CachedResource*) override;

        bool maybeLoadEmpty();

        bool isMultipartReplacingLoad() const;
        bool isPostOrRedirectAfterPost(const ResourceRequest&, const ResourceResponse&);

        void continueAfterNavigationPolicy(const ResourceRequest&, bool shouldContinue);

        void continueAfterContentPolicy(PolicyAction);

        void stopLoadingForPolicyChange();
        ResourceError interruptedForPolicyChangeError() const;

#if HAVE(RUNLOOP_TIMER)
        typedef RunLoopTimer<DocumentLoader> DocumentLoaderTimer;
#else
        typedef Timer<DocumentLoader> DocumentLoaderTimer;
#endif
        void handleSubstituteDataLoadSoon();
        void handleSubstituteDataLoadNow(DocumentLoaderTimer*);
        void startDataLoadTimer();

        void deliverSubstituteResourcesAfterDelay();
        void substituteResourceDeliveryTimerFired(Timer<DocumentLoader>&);

        void clearMainResource();

        Frame* m_frame;
        Ref<CachedResourceLoader> m_cachedResourceLoader;

        CachedResourceHandle<CachedRawResource> m_mainResource;
        ResourceLoaderMap m_subresourceLoaders;
        ResourceLoaderMap m_multipartSubresourceLoaders;
        ResourceLoaderMap m_plugInStreamLoaders;
        
        mutable DocumentWriter m_writer;

        // A reference to actual request used to create the data source.
        // This should only be used by the resourceLoadDelegate's
        // identifierForInitialRequest:fromDatasource: method. It is
        // not guaranteed to remain unchanged, as requests are mutable.
        ResourceRequest m_originalRequest;   

        SubstituteData m_substituteData;

        // A copy of the original request used to create the data source.
        // We have to copy the request because requests are mutable.
        ResourceRequest m_originalRequestCopy;
        
        // The 'working' request. It may be mutated
        // several times from the original request to include additional
        // headers, cookie information, canonicalization and redirects.
        ResourceRequest m_request;

        ResourceResponse m_response;
    
        ResourceError m_mainDocumentError;    

        bool m_originalSubstituteDataWasValid;
        bool m_committed;
        bool m_isStopping;
        bool m_gotFirstByte;
        bool m_isClientRedirect;
        bool m_isLoadingMultipartContent;

        // FIXME: Document::m_processingLoadEvent and DocumentLoader::m_wasOnloadHandled are roughly the same
        // and should be merged.
        bool m_wasOnloadHandled;

        StringWithDirection m_pageTitle;

        String m_overrideEncoding;

        // The action that triggered loading - we keep this around for the
        // benefit of the various policy handlers.
        NavigationAction m_triggeringAction;

        // The last request that we checked click policy for - kept around
        // so we can avoid asking again needlessly.
        ResourceRequest m_lastCheckedRequest;

        // We retain all the received responses so we can play back the
        // WebResourceLoadDelegate messages if the item is loaded from the
        // page cache.
        ResponseVector m_responses;
        bool m_stopRecordingResponses;
        
        typedef HashMap<RefPtr<ResourceLoader>, RefPtr<SubstituteResource>> SubstituteResourceMap;
        SubstituteResourceMap m_pendingSubstituteResources;
        Timer<DocumentLoader> m_substituteResourceDeliveryTimer;

        OwnPtr<ArchiveResourceCollection> m_archiveResourceCollection;
#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
        RefPtr<Archive> m_archive;
        RefPtr<SharedBuffer> m_parsedArchiveData;
#endif

        HashSet<String> m_resourcesClientKnowsAbout;
        Vector<ResourceRequest> m_resourcesLoadedFromMemoryCacheForClientNotification;
        
        String m_clientRedirectSourceForHistory;
        bool m_didCreateGlobalHistoryEntry;

        bool m_loadingMainResource;
        DocumentLoadTiming m_documentLoadTiming;

        double m_timeOfLastDataReceived;
        unsigned long m_identifierForLoadWithoutResourceLoader;

        DocumentLoaderTimer m_dataLoadTimer;
        bool m_waitingForContentPolicy;

        RefPtr<IconLoadDecisionCallback> m_iconLoadDecisionCallback;
        RefPtr<IconDataCallback> m_iconDataCallback;

        bool m_subresourceLoadersArePageCacheAcceptable;

        friend class ApplicationCacheHost;  // for substitute resource delivery
        OwnPtr<ApplicationCacheHost> m_applicationCacheHost;

#if ENABLE(CONTENT_FILTERING)
        std::unique_ptr<ContentFilter> m_contentFilter;
#endif

#if USE(QUICK_LOOK)
        std::unique_ptr<QuickLookHandle> m_quickLookHandle;
#endif
    };

    inline void DocumentLoader::recordMemoryCacheLoadForFutureClientNotification(const ResourceRequest& request)
    {
        m_resourcesLoadedFromMemoryCacheForClientNotification.append(request);
    }

    inline void DocumentLoader::takeMemoryCacheLoadsForClientNotification(Vector<ResourceRequest>& loadsSet)
    {
        loadsSet.swap(m_resourcesLoadedFromMemoryCacheForClientNotification);
        m_resourcesLoadedFromMemoryCacheForClientNotification.clear();
    }

}

#endif // DocumentLoader_h
