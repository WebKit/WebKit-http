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

#ifndef InspectorTimelineAgent_h
#define InspectorTimelineAgent_h

#if ENABLE(INSPECTOR)

#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "LayoutTypes.h"
#include "PlatformInstrumentation.h"
#include "ScriptGCEvent.h"
#include "ScriptGCEventListener.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {
class Event;
class Frame;
class InspectorClient;
class InspectorFrontend;
class InspectorPageAgent;
class InspectorState;
class InstrumentingAgents;
class IntRect;
class RenderObject;
class ResourceRequest;
class ResourceResponse;

typedef String ErrorString;

class InspectorTimelineAgent
    : public InspectorBaseAgent<InspectorTimelineAgent>,
      public ScriptGCEventListener,
      public InspectorBackendDispatcher::TimelineCommandHandler,
      public PlatformInstrumentationClient {
    WTF_MAKE_NONCOPYABLE(InspectorTimelineAgent);
public:
    enum InspectorType { PageInspector, WorkerInspector };

    static PassOwnPtr<InspectorTimelineAgent> create(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorState* state, InspectorType type, InspectorClient* client)
    {
        return adoptPtr(new InspectorTimelineAgent(instrumentingAgents, pageAgent, state, type, client));
    }

    ~InspectorTimelineAgent();

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void restore();

    virtual void start(ErrorString*, const int* maxCallStackDepth);
    virtual void stop(ErrorString*);
    virtual void setIncludeMemoryDetails(ErrorString*, bool);
    virtual void supportsFrameInstrumentation(ErrorString*, bool*);

    int id() const { return m_id; }

    void didCommitLoad();

    // Methods called from WebCore.
    void willCallFunction(const String& scriptName, int scriptLine, Frame*);
    void didCallFunction();

    void willDispatchEvent(const Event&, Frame*);
    void didDispatchEvent();

    void didBeginFrame();
    void didCancelFrame();

    void didInvalidateLayout(Frame*);
    void willLayout(Frame*);
    void didLayout(RenderObject*);

    void didScheduleStyleRecalculation(Frame*);
    void willRecalculateStyle(Frame*);
    void didRecalculateStyle();

    void willPaint(const LayoutRect&, Frame*);
    void didPaint();

    void willComposite();
    void didComposite();

    // FIXME: |length| should be passed in didWrite instead willWrite
    // as the parser can not know how much it will process until it tries.
    void willWriteHTML(unsigned int length, unsigned int startLine, Frame*);
    void didWriteHTML(unsigned int endLine);

    void didInstallTimer(int timerId, int timeout, bool singleShot, Frame*);
    void didRemoveTimer(int timerId, Frame*);
    void willFireTimer(int timerId, Frame*);
    void didFireTimer();

    void willDispatchXHRReadyStateChangeEvent(const String&, int, Frame*);
    void didDispatchXHRReadyStateChangeEvent();
    void willDispatchXHRLoadEvent(const String&, Frame*);
    void didDispatchXHRLoadEvent();

    void willEvaluateScript(const String&, int, Frame*);
    void didEvaluateScript();

    void didTimeStamp(Frame*, const String&);
    void didMarkDOMContentEvent(Frame*);
    void didMarkLoadEvent(Frame*);

    void time(Frame*, const String&);
    void timeEnd(Frame*, const String&);

    void didScheduleResourceRequest(const String& url, Frame*);
    void willSendResourceRequest(unsigned long, const ResourceRequest&, Frame*);
    void willReceiveResourceResponse(unsigned long, const ResourceResponse&, Frame*);
    void didReceiveResourceResponse();
    void didFinishLoadingResource(unsigned long, bool didFail, double finishTime, Frame*);
    void willReceiveResourceData(unsigned long identifier, Frame*, int length);
    void didReceiveResourceData();

    void didRequestAnimationFrame(int callbackId, Frame*);
    void didCancelAnimationFrame(int callbackId, Frame*);
    void willFireAnimationFrame(int callbackId, Frame*);
    void didFireAnimationFrame();

    virtual void didGC(double, double, size_t);

    void willProcessTask();
    void didProcessTask();

    // PlatformInstrumentationClient methods.
    virtual void willDecodeImage(const String& imageType) OVERRIDE;
    virtual void didDecodeImage() OVERRIDE;
    virtual void willResizeImage(bool shouldCache) OVERRIDE;
    virtual void didResizeImage() OVERRIDE;

private:
    struct TimelineRecordEntry {
        TimelineRecordEntry(PassRefPtr<InspectorObject> record, PassRefPtr<InspectorObject> data, PassRefPtr<InspectorArray> children, const String& type, const String& frameId, size_t usedHeapSizeAtStart)
            : record(record), data(data), children(children), type(type), frameId(frameId), usedHeapSizeAtStart(usedHeapSizeAtStart)
        {
        }
        RefPtr<InspectorObject> record;
        RefPtr<InspectorObject> data;
        RefPtr<InspectorArray> children;
        String type;
        String frameId;
        size_t usedHeapSizeAtStart;
    };
        
    InspectorTimelineAgent(InstrumentingAgents*, InspectorPageAgent*, InspectorState*, InspectorType, InspectorClient*);

    void pushCurrentRecord(PassRefPtr<InspectorObject>, const String& type, bool captureCallStack, Frame*, bool hasLowLevelDetails = false);
    void setHeapSizeStatistics(InspectorObject* record);

    void didCompleteCurrentRecord(const String& type);
    void commitFrameRecord();
    void appendRecord(PassRefPtr<InspectorObject> data, const String& type, bool captureCallStack, Frame*);
    void addRecordToTimeline(PassRefPtr<InspectorObject>, const String& type, const String& frameId);
    void innerAddRecordToTimeline(PassRefPtr<InspectorObject>, const String& type, const String& frameId);

    void pushGCEventRecords();
    void clearRecordStack();

    double timestamp();
    double timestampFromMicroseconds(double microseconds);

    InspectorPageAgent* m_pageAgent;

    InspectorFrontend::Timeline* m_frontend;
    double m_timestampOffset;

    Vector<TimelineRecordEntry> m_recordStack;

    int m_id;
    struct GCEvent {
        GCEvent(double startTime, double endTime, size_t collectedBytes)
            : startTime(startTime), endTime(endTime), collectedBytes(collectedBytes)
        {
        }
        double startTime;
        double endTime;
        size_t collectedBytes;
    };
    typedef Vector<GCEvent> GCEvents;
    GCEvents m_gcEvents;
    int m_maxCallStackDepth;
    unsigned m_platformInstrumentationClientInstalledAtStackDepth;
    RefPtr<InspectorObject> m_pendingFrameRecord;
    InspectorType m_inspectorType;
    InspectorClient* m_client;
};

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
#endif // !defined(InspectorTimelineAgent_h)
