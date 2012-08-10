/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#ifndef MainResourceLoader_h
#define MainResourceLoader_h

#include "FrameLoaderTypes.h"
#include "ResourceLoader.h"
#include "SubstituteData.h"
#include <wtf/Forward.h>

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
OBJC_CLASS WebFilterEvaluator;
#endif

#if HAVE(RUNLOOP_TIMER)
#include "RunLoopTimer.h"
#else
#include "Timer.h"
#endif

namespace WebCore {

    class FormState;
    class ResourceRequest;

    class MainResourceLoader : public ResourceLoader {
    public:
        static PassRefPtr<MainResourceLoader> create(Frame*);
        virtual ~MainResourceLoader();

        void load(const ResourceRequest&, const SubstituteData&);
        virtual void addData(const char*, int, bool allAtOnce) OVERRIDE;

        virtual void setDefersLoading(bool) OVERRIDE;

        virtual void willSendRequest(ResourceRequest&, const ResourceResponse& redirectResponse) OVERRIDE;
        virtual void didReceiveResponse(const ResourceResponse&) OVERRIDE;
        virtual void didReceiveData(const char*, int, long long encodedDataLength, bool allAtOnce) OVERRIDE;
        virtual void didFinishLoading(double finishTime) OVERRIDE;
        virtual void didFail(const ResourceError&) OVERRIDE;

#if HAVE(RUNLOOP_TIMER)
        typedef RunLoopTimer<MainResourceLoader> MainResourceLoaderTimer;
#else
        typedef Timer<MainResourceLoader> MainResourceLoaderTimer;
#endif

        bool isLoadingMultipartContent() const { return m_loadingMultipartContent; }

        virtual void reportMemoryUsage(MemoryObjectInfo*) const OVERRIDE;

    private:
        explicit MainResourceLoader(Frame*);

        virtual void willCancel(const ResourceError&) OVERRIDE;
        virtual void didCancel(const ResourceError&) OVERRIDE;

        bool loadNow(ResourceRequest&);

        void handleEmptyLoad(const KURL&, bool forURLScheme);
        void handleSubstituteDataLoadSoon(const ResourceRequest&);
        void handleSubstituteDataLoadNow(MainResourceLoaderTimer*);

        void startDataLoadTimer();

        void receivedError(const ResourceError&);
        ResourceError interruptedForPolicyChangeError() const;
        void stopLoadingForPolicyChange();
        bool isPostOrRedirectAfterPost(const ResourceRequest& newRequest, const ResourceResponse& redirectResponse);

        static void callContinueAfterNavigationPolicy(void*, const ResourceRequest&, PassRefPtr<FormState>, bool shouldContinue);
        void continueAfterNavigationPolicy(const ResourceRequest&, bool shouldContinue);

        static void callContinueAfterContentPolicy(void*, PolicyAction);
        void continueAfterContentPolicy(PolicyAction);
        void continueAfterContentPolicy(PolicyAction, const ResourceResponse&);
        
#if PLATFORM(QT)
        void substituteMIMETypeFromPluginDatabase(const ResourceResponse&);
#endif

        ResourceRequest m_initialRequest;
        SubstituteData m_substituteData;

        MainResourceLoaderTimer m_dataLoadTimer;

        bool m_loadingMultipartContent;
        bool m_waitingForContentPolicy;
        double m_timeOfLastDataReceived;

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
        WebFilterEvaluator *m_filter;
#endif
    };

}

#endif
