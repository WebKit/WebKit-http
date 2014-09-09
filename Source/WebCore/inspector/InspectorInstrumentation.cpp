/*
* Copyright (C) 2011 Google Inc. All rights reserved.
* Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "InspectorInstrumentation.h"

#if ENABLE(INSPECTOR)

#include "CSSRule.h"
#include "CSSStyleRule.h"
#include "DOMWindow.h"
#include "DOMWrapperWorld.h"
#include "Database.h"
#include "DocumentLoader.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "InspectorApplicationCacheAgent.h"
#include "InspectorCSSAgent.h"
#include "InspectorDOMAgent.h"
#include "InspectorDOMDebuggerAgent.h"
#include "InspectorDOMStorageAgent.h"
#include "InspectorDatabaseAgent.h"
#include "InspectorLayerTreeAgent.h"
#include "InspectorPageAgent.h"
#include "InspectorResourceAgent.h"
#include "InspectorTimelineAgent.h"
#include "InspectorWorkerAgent.h"
#include "InstrumentingAgents.h"
#include "MainFrame.h"
#include "PageDebuggerAgent.h"
#include "PageRuntimeAgent.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "WebConsoleAgent.h"
#include "WorkerGlobalScope.h"
#include "WorkerRuntimeAgent.h"
#include "WorkerThread.h"
#include "XMLHttpRequest.h"
#include <inspector/ScriptArguments.h>
#include <inspector/ScriptCallStack.h>
#include <inspector/agents/InspectorDebuggerAgent.h>
#include <inspector/agents/InspectorProfilerAgent.h>
#include <profiler/Profile.h>
#include <runtime/ConsoleTypes.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

#if ENABLE(WEB_REPLAY)
#include "InspectorReplayAgent.h"
#include "ReplayController.h" // for ReplayPosition.
#endif

using namespace Inspector;

namespace WebCore {

static const char* const requestAnimationFrameEventName = "requestAnimationFrame";
static const char* const cancelAnimationFrameEventName = "cancelAnimationFrame";
static const char* const animationFrameFiredEventName = "animationFrameFired";
static const char* const setTimerEventName = "setTimer";
static const char* const clearTimerEventName = "clearTimer";
static const char* const timerFiredEventName = "timerFired";

namespace {
static HashSet<InstrumentingAgents*>* instrumentingAgentsSet = nullptr;
}

int InspectorInstrumentation::s_frontendCounter = 0;

static Frame* frameForScriptExecutionContext(ScriptExecutionContext* context)
{
    Frame* frame = nullptr;
    if (context->isDocument())
        frame = toDocument(context)->frame();
    return frame;
}

void InspectorInstrumentation::didClearWindowObjectInWorldImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, DOMWrapperWorld& world)
{
    InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent();
    if (pageAgent)
        pageAgent->didClearWindowObjectInWorld(frame, world);
    if (PageDebuggerAgent* debuggerAgent = instrumentingAgents->pageDebuggerAgent()) {
        if (pageAgent && &world == &mainThreadNormalWorld() && frame == pageAgent->mainFrame())
            debuggerAgent->didClearMainFrameWindowObject();
    }
    if (PageRuntimeAgent* pageRuntimeAgent = instrumentingAgents->pageRuntimeAgent()) {
        if (&world == &mainThreadNormalWorld())
            pageRuntimeAgent->didCreateMainWorldContext(frame);
    }
}

bool InspectorInstrumentation::isDebuggerPausedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        return debuggerAgent->isPaused();
    return false;
}

void InspectorInstrumentation::willInsertDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* parent)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willInsertDOMNode(parent);
}

void InspectorInstrumentation::didInsertDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didInsertDOMNode(node);
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInsertDOMNode(node);
}

void InspectorInstrumentation::willRemoveDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willRemoveDOMNode(node);
}

void InspectorInstrumentation::didRemoveDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didRemoveDOMNode(node);
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didRemoveDOMNode(node);
}

void InspectorInstrumentation::willModifyDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& oldValue, const AtomicString& newValue)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willModifyDOMAttr(element);
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->willModifyDOMAttr(element, oldValue, newValue);
}

void InspectorInstrumentation::didModifyDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& name, const AtomicString& value)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didModifyDOMAttr(element, name, value);
}

void InspectorInstrumentation::didRemoveDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& name)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didRemoveDOMAttr(element, name);
}

void InspectorInstrumentation::didInvalidateStyleAttrImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didInvalidateStyleAttr(node);
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInvalidateStyleAttr(node);
}

void InspectorInstrumentation::frameWindowDiscardedImpl(InstrumentingAgents* instrumentingAgents, DOMWindow* window)
{
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->frameWindowDiscarded(window);
}

void InspectorInstrumentation::mediaQueryResultChangedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->mediaQueryResultChanged();
}

void InspectorInstrumentation::didPushShadowRootImpl(InstrumentingAgents* instrumentingAgents, Element* host, ShadowRoot* root)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didPushShadowRoot(host, root);
}

void InspectorInstrumentation::willPopShadowRootImpl(InstrumentingAgents* instrumentingAgents, Element* host, ShadowRoot* root)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->willPopShadowRoot(host, root);
}

void InspectorInstrumentation::didCreateNamedFlowImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didCreateNamedFlow(document, namedFlow);
}

void InspectorInstrumentation::willRemoveNamedFlowImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->willRemoveNamedFlow(document, namedFlow);
}

void InspectorInstrumentation::didChangeRegionOversetImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didChangeRegionOverset(document, namedFlow);
}

void InspectorInstrumentation::didRegisterNamedFlowContentElementImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow, Node* contentElement, Node* nextContentElement)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didRegisterNamedFlowContentElement(document, namedFlow, contentElement, nextContentElement);
}

void InspectorInstrumentation::didUnregisterNamedFlowContentElementImpl(InstrumentingAgents* instrumentingAgents, Document* document, WebKitNamedFlow* namedFlow, Node* contentElement)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didUnregisterNamedFlowContentElement(document, namedFlow, contentElement);
}

void InspectorInstrumentation::mouseDidMoveOverElementImpl(InstrumentingAgents* instrumentingAgents, const HitTestResult& result, unsigned modifierFlags)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->mouseDidMoveOverElement(result, modifierFlags);
}

void InspectorInstrumentation::didScrollImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didScroll();
}

bool InspectorInstrumentation::handleTouchEventImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        return domAgent->handleTouchEvent(node);
    return false;
}

bool InspectorInstrumentation::handleMousePressImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        return domAgent->handleMousePress();
    return false;
}

bool InspectorInstrumentation::forcePseudoStateImpl(InstrumentingAgents* instrumentingAgents, Element* element, CSSSelector::PseudoClassType pseudoState)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        return cssAgent->forcePseudoState(element, pseudoState);
    return false;
}

void InspectorInstrumentation::characterDataModifiedImpl(InstrumentingAgents* instrumentingAgents, CharacterData* characterData)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->characterDataModified(characterData);
}

void InspectorInstrumentation::willSendXMLHttpRequestImpl(InstrumentingAgents* instrumentingAgents, const String& url)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willSendXMLHttpRequest(url);
}

void InspectorInstrumentation::didScheduleResourceRequestImpl(InstrumentingAgents* instrumentingAgents, const String& url, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScheduleResourceRequest(url, frame);
}

void InspectorInstrumentation::didInstallTimerImpl(InstrumentingAgents* instrumentingAgents, int timerId, int timeout, bool singleShot, ScriptExecutionContext* context)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, setTimerEventName, true);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didInstallTimer(timerId, timeout, singleShot, frameForScriptExecutionContext(context));
}

void InspectorInstrumentation::didRemoveTimerImpl(InstrumentingAgents* instrumentingAgents, int timerId, ScriptExecutionContext* context)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, clearTimerEventName, true);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didRemoveTimer(timerId, frameForScriptExecutionContext(context));
}

InspectorInstrumentationCookie InspectorInstrumentation::willCallFunctionImpl(InstrumentingAgents* instrumentingAgents, const String& scriptName, int scriptLine, ScriptExecutionContext* context)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willCallFunction(scriptName, scriptLine, frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didCallFunctionImpl(const InspectorInstrumentationCookie& cookie, ScriptExecutionContext* context)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didCallFunction(frameForScriptExecutionContext(context));
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchXHRReadyStateChangeEventImpl(InstrumentingAgents* instrumentingAgents, XMLHttpRequest* request, ScriptExecutionContext* context)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && request->hasEventListeners(eventNames().readystatechangeEvent)) {
        timelineAgent->willDispatchXHRReadyStateChangeEvent(request->url().string(), request->readyState(), frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didDispatchXHRReadyStateChangeEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchXHRReadyStateChangeEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventImpl(InstrumentingAgents* instrumentingAgents, const Event& event, bool hasEventListeners, Document* document)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && hasEventListeners) {
        timelineAgent->willDispatchEvent(event, document->frame());
        timelineAgentId = timelineAgent->id();
    }
#if ENABLE(WEB_REPLAY)
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->willDispatchEvent(event, document->frame());
#endif
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

InspectorInstrumentationCookie InspectorInstrumentation::willHandleEventImpl(InstrumentingAgents* instrumentingAgents, Event* event)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, true, event->type(), false);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void InspectorInstrumentation::didHandleEventImpl(const InspectorInstrumentationCookie& cookie)
{
    cancelPauseOnNativeEvent(cookie.instrumentingAgents());
}

void InspectorInstrumentation::didDispatchEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchEventOnWindowImpl(InstrumentingAgents* instrumentingAgents, const Event& event, DOMWindow* window)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && window->hasEventListeners(event.type())) {
        timelineAgent->willDispatchEvent(event, window ? window->frame() : nullptr);
        timelineAgentId = timelineAgent->id();
    }
#if ENABLE(WEB_REPLAY)
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->willDispatchEvent(event, window ? window->frame() : nullptr);
#endif
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didDispatchEventOnWindowImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEvent();
}

InspectorInstrumentationCookie InspectorInstrumentation::willEvaluateScriptImpl(InstrumentingAgents* instrumentingAgents, const String& url, int lineNumber, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willEvaluateScript(url, lineNumber, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didEvaluateScriptImpl(const InspectorInstrumentationCookie& cookie, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didEvaluateScript(frame);
}

void InspectorInstrumentation::scriptsEnabledImpl(InstrumentingAgents* instrumentingAgents, bool isEnabled)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->scriptsEnabled(isEnabled);
}

void InspectorInstrumentation::didCreateIsolatedContextImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, JSC::ExecState* scriptState, SecurityOrigin* origin)
{
    if (PageRuntimeAgent* runtimeAgent = instrumentingAgents->pageRuntimeAgent())
        runtimeAgent->didCreateIsolatedContext(frame, scriptState, origin);
}

InspectorInstrumentationCookie InspectorInstrumentation::willFireTimerImpl(InstrumentingAgents* instrumentingAgents, int timerId, ScriptExecutionContext* context)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, timerFiredEventName, false);

    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willFireTimer(timerId, frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didFireTimerImpl(const InspectorInstrumentationCookie& cookie)
{
    cancelPauseOnNativeEvent(cookie.instrumentingAgents());

    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didFireTimer();
}

void InspectorInstrumentation::didInvalidateLayoutImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didInvalidateLayout(frame);
}

InspectorInstrumentationCookie InspectorInstrumentation::willLayoutImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willLayout(frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didLayoutImpl(const InspectorInstrumentationCookie& cookie, RenderObject* root)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didLayout(root);

    if (InspectorPageAgent* pageAgent = cookie.instrumentingAgents()->inspectorPageAgent())
        pageAgent->didLayout();
}

InspectorInstrumentationCookie InspectorInstrumentation::willDispatchXHRLoadEventImpl(InstrumentingAgents* instrumentingAgents, XMLHttpRequest* request, ScriptExecutionContext* context)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent && request->hasEventListeners(eventNames().loadEvent)) {
        timelineAgent->willDispatchXHRLoadEvent(request->url(), frameForScriptExecutionContext(context));
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didDispatchXHRLoadEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchXHRLoadEvent();
}

void InspectorInstrumentation::willPaintImpl(InstrumentingAgents* instrumentingAgents, RenderObject* renderer)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willPaint(&renderer->frame());
}

void InspectorInstrumentation::didPaintImpl(InstrumentingAgents*  instrumentingAgents, RenderObject* renderer, GraphicsContext* context, const LayoutRect& rect)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didPaint(renderer, rect);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didPaint(context, rect);
}

void InspectorInstrumentation::willScrollLayerImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willScroll(frame);
}

void InspectorInstrumentation::didScrollLayerImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScroll();
}

InspectorInstrumentationCookie InspectorInstrumentation::willRecalculateStyleImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willRecalculateStyle(frame);
        timelineAgentId = timelineAgent->id();
    }
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willRecalculateStyle();
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didRecalculateStyleImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didRecalculateStyle();
    InstrumentingAgents* instrumentingAgents = cookie.instrumentingAgents();
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didRecalculateStyle();
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didRecalculateStyle();
}

void InspectorInstrumentation::didScheduleStyleRecalculationImpl(InstrumentingAgents* instrumentingAgents, Document* document)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScheduleStyleRecalculation(document->frame());
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didScheduleStyleRecalculation(document);
}

void InspectorInstrumentation::applyEmulatedMediaImpl(InstrumentingAgents* instrumentingAgents, String* media)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->applyEmulatedMedia(media);
}

void InspectorInstrumentation::willSendRequestImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willSendResourceRequest(identifier, request, loader->frame());
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willSendRequest(identifier, loader, request, redirectResponse);
}

void InspectorInstrumentation::continueAfterPingLoaderImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& response)
{
    willSendRequestImpl(instrumentingAgents, identifier, loader, request, response);
}

void InspectorInstrumentation::markResourceAsCachedImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->markResourceAsCached(identifier);
}

void InspectorInstrumentation::didLoadResourceFromMemoryCacheImpl(InstrumentingAgents* instrumentingAgents, DocumentLoader* loader, CachedResource* cachedResource)
{
    if (!instrumentingAgents->inspectorEnvironment().developerExtrasEnabled())
        return;
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didLoadResourceFromMemoryCache(loader, cachedResource);
}

InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceDataImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, Frame* frame, int length)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willReceiveResourceData(identifier, frame, length);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didReceiveResourceDataImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceData();
}

InspectorInstrumentationCookie InspectorInstrumentation::willReceiveResourceResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const ResourceResponse& response, Frame* frame)
{
    int timelineAgentId = 0;
    InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent();
    if (timelineAgent) {
        timelineAgent->willReceiveResourceResponse(identifier, response, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didReceiveResourceResponseImpl(const InspectorInstrumentationCookie& cookie, unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response, ResourceLoader* resourceLoader)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceResponse();
    if (!loader)
        return;
    InstrumentingAgents* instrumentingAgents = cookie.instrumentingAgents();
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveResponse(identifier, loader, response, resourceLoader);
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->didReceiveResponse(identifier, response); // This should come AFTER resource notification, front-end relies on this.
}

void InspectorInstrumentation::didReceiveResourceResponseButCanceledImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceResponse(frame, identifier, r);
    InspectorInstrumentation::didReceiveResourceResponse(cookie, identifier, loader, r, nullptr);
}

void InspectorInstrumentation::continueAfterXFrameOptionsDeniedImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void InspectorInstrumentation::continueWithPolicyDownloadImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void InspectorInstrumentation::continueWithPolicyIgnoreImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void InspectorInstrumentation::didReceiveDataImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const char* data, int dataLength, int encodedDataLength)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveData(identifier, data, dataLength, encodedDataLength);
}

void InspectorInstrumentation::didFinishLoadingImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, double finishTime)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didFinishLoadingResource(identifier, false, finishTime, loader->frame());
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFinishLoading(identifier, loader, finishTime);
}

void InspectorInstrumentation::didFailLoadingImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, const ResourceError& error)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didFinishLoadingResource(identifier, true, 0, loader->frame());
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFailLoading(identifier, loader, error);
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->didFailLoading(identifier, error); // This should come AFTER resource notification, front-end relies on this.
}

void InspectorInstrumentation::documentThreadableLoaderStartedLoadingForClientImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, ThreadableLoaderClient* client)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->documentThreadableLoaderStartedLoadingForClient(identifier, client);
}

void InspectorInstrumentation::willLoadXHRImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client, const String& method, const URL& url, bool async, PassRefPtr<FormData> formData, const HTTPHeaderMap& headers, bool includeCredentials)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willLoadXHR(client, method, url, async, formData, headers, includeCredentials);
}

void InspectorInstrumentation::didFailXHRLoadingImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFailXHRLoading(client);
}

void InspectorInstrumentation::didFinishXHRLoadingImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber, unsigned sendColumnNumber)
{
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->didFinishXHRLoading(identifier, url, sendURL, sendLineNumber, sendColumnNumber);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFinishXHRLoading(client, identifier, sourceString);
}

void InspectorInstrumentation::didReceiveXHRResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveXHRResponse(identifier);
}

void InspectorInstrumentation::willLoadXHRSynchronouslyImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willLoadXHRSynchronously();
}

void InspectorInstrumentation::didLoadXHRSynchronouslyImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didLoadXHRSynchronously();
}

void InspectorInstrumentation::scriptImportedImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const String& sourceString)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->setInitialScriptContent(identifier, sourceString);
}

void InspectorInstrumentation::scriptExecutionBlockedByCSPImpl(InstrumentingAgents* instrumentingAgents, const String& directiveText)
{
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->scriptExecutionBlockedByCSP(directiveText);
}

void InspectorInstrumentation::didReceiveScriptResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveScriptResponse(identifier);
}

void InspectorInstrumentation::domContentLoadedEventFiredImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didMarkDOMContentEvent(frame);

    if (&frame->page()->mainFrame() != frame)
        return;

    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->mainFrameDOMContentLoaded();

    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->domContentEventFired();
}

void InspectorInstrumentation::loadEventFiredImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didMarkLoadEvent(frame);

    if (&frame->page()->mainFrame() != frame)
        return;

    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->loadEventFired();
}

void InspectorInstrumentation::frameDetachedFromParentImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameDetached(frame);

#if ENABLE(WEB_REPLAY)
    if (frame->isMainFrame()) {
        if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
            replayAgent->frameDetached(frame);
    }
#endif
}

void InspectorInstrumentation::didCommitLoadImpl(InstrumentingAgents* instrumentingAgents, Page* page, DocumentLoader* loader)
{
    if (!instrumentingAgents->inspectorEnvironment().developerExtrasEnabled())
        return;

    if (loader->frame()->isMainFrame()) {
        if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
            consoleAgent->reset();

        if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
            resourceAgent->mainFrameNavigated(loader);

        if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
            profilerAgent->reset();

        if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
            cssAgent->reset();

#if ENABLE(SQL_DATABASE)
        if (InspectorDatabaseAgent* databaseAgent = instrumentingAgents->inspectorDatabaseAgent())
            databaseAgent->clearResources();
#endif

        if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
            domAgent->setDocument(page->mainFrame().document());

        if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
            layerTreeAgent->reset();
    }

    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didCommitLoad(loader->frame()->document());

    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameNavigated(loader);

#if ENABLE(WEB_REPLAY)
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->frameNavigated(loader);
#endif
}

void InspectorInstrumentation::frameDocumentUpdatedImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (!instrumentingAgents->inspectorEnvironment().developerExtrasEnabled())
        return;
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->frameDocumentUpdated(frame);
}

void InspectorInstrumentation::loaderDetachedFromFrameImpl(InstrumentingAgents* instrumentingAgents, DocumentLoader* loader)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->loaderDetachedFromFrame(loader);
}

void InspectorInstrumentation::frameStartedLoadingImpl(InstrumentingAgents& instrumentingAgents, Frame& frame)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents.inspectorPageAgent())
        inspectorPageAgent->frameStartedLoading(frame);
}

void InspectorInstrumentation::frameStoppedLoadingImpl(InstrumentingAgents& instrumentingAgents, Frame& frame)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents.inspectorPageAgent())
        inspectorPageAgent->frameStoppedLoading(frame);
}

void InspectorInstrumentation::frameScheduledNavigationImpl(InstrumentingAgents& instrumentingAgents, Frame& frame, double delay)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents.inspectorPageAgent())
        inspectorPageAgent->frameScheduledNavigation(frame, delay);
}

void InspectorInstrumentation::frameClearedScheduledNavigationImpl(InstrumentingAgents& instrumentingAgents, Frame& frame)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents.inspectorPageAgent())
        inspectorPageAgent->frameClearedScheduledNavigation(frame);
}

InspectorInstrumentationCookie InspectorInstrumentation::willRunJavaScriptDialogImpl(InstrumentingAgents* instrumentingAgents, const String& message)
{
    if (InspectorPageAgent* inspectorPageAgent = instrumentingAgents->inspectorPageAgent())
        inspectorPageAgent->willRunJavaScriptDialog(message);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void InspectorInstrumentation::didRunJavaScriptDialogImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorPageAgent* inspectorPageAgent = cookie.instrumentingAgents()->inspectorPageAgent())
        inspectorPageAgent->didRunJavaScriptDialog();
}

void InspectorInstrumentation::willDestroyCachedResourceImpl(CachedResource* cachedResource)
{
    if (!instrumentingAgentsSet)
        return;
    HashSet<InstrumentingAgents*>::iterator end = instrumentingAgentsSet->end();
    for (HashSet<InstrumentingAgents*>::iterator it = instrumentingAgentsSet->begin(); it != end; ++it) {
        InstrumentingAgents* instrumentingAgents = *it;
        if (InspectorResourceAgent* inspectorResourceAgent = instrumentingAgents->inspectorResourceAgent())
            inspectorResourceAgent->willDestroyCachedResource(cachedResource);
    }
}

InspectorInstrumentationCookie InspectorInstrumentation::willWriteHTMLImpl(InstrumentingAgents* instrumentingAgents, unsigned startLine, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willWriteHTML(startLine, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didWriteHTMLImpl(const InspectorInstrumentationCookie& cookie, unsigned endLine)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didWriteHTML(endLine);
}

// JavaScriptCore InspectorDebuggerAgent should know Console MessageTypes.
static bool isConsoleAssertMessage(MessageSource source, MessageType type)
{
    return source == MessageSource::ConsoleAPI && type == MessageType::Assert;
}

// FIXME: Drop this once we no longer generate stacks outside of Inspector.
void InspectorInstrumentation::addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, PassRefPtr<ScriptCallStack> callStack, unsigned long requestIdentifier)
{
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, callStack, requestIdentifier);
    // FIXME: This should just pass the message on to the debugger agent. JavaScriptCore InspectorDebuggerAgent should know Console MessageTypes.
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent()) {
        if (isConsoleAssertMessage(source, type))
            debuggerAgent->handleConsoleAssert();
    }
}

void InspectorInstrumentation::addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, JSC::ExecState* state, PassRefPtr<ScriptArguments> arguments, unsigned long requestIdentifier)
{
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, state, arguments, requestIdentifier);
    // FIXME: This should just pass the message on to the debugger agent. JavaScriptCore InspectorDebuggerAgent should know Console MessageTypes.
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent()) {
        if (isConsoleAssertMessage(source, type))
            debuggerAgent->handleConsoleAssert();
    }
}

void InspectorInstrumentation::addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, const String& scriptID, unsigned lineNumber, unsigned columnNumber, JSC::ExecState* state, unsigned long requestIdentifier)
{
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, scriptID, lineNumber, columnNumber, state, requestIdentifier);
}

void InspectorInstrumentation::consoleCountImpl(InstrumentingAgents* instrumentingAgents, JSC::ExecState* state, PassRefPtr<ScriptArguments> arguments)
{
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->count(state, arguments);
}

void InspectorInstrumentation::startConsoleTimingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, const String& title)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->time(frame, title);
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->startTiming(title);
}

void InspectorInstrumentation::stopConsoleTimingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, const String& title, PassRefPtr<ScriptCallStack> stack)
{
    if (WebConsoleAgent* consoleAgent = instrumentingAgents->webConsoleAgent())
        consoleAgent->stopTiming(title, stack);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->timeEnd(frame, title);
}

void InspectorInstrumentation::consoleTimeStampImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, PassRefPtr<ScriptArguments> arguments)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        String message;
        arguments->getFirstArgumentAsString(message);
        timelineAgent->didTimeStamp(frame, message);
     }
}

void InspectorInstrumentation::startProfilingImpl(InstrumentingAgents* instrumentingAgents, JSC::ExecState* exec, const String& title)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->persistentInspectorTimelineAgent())
        timelineAgent->startFromConsole(exec, title);
}

PassRefPtr<JSC::Profile> InspectorInstrumentation::stopProfilingImpl(InstrumentingAgents* instrumentingAgents, JSC::ExecState* exec, const String& title)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->persistentInspectorTimelineAgent())
        return timelineAgent->stopFromConsole(exec, title);
    return nullptr;
}

#if ENABLE(SQL_DATABASE)
void InspectorInstrumentation::didOpenDatabaseImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<Database> database, const String& domain, const String& name, const String& version)
{
    if (!instrumentingAgents->inspectorEnvironment().developerExtrasEnabled())
        return;
    if (InspectorDatabaseAgent* dbAgent = instrumentingAgents->inspectorDatabaseAgent())
        dbAgent->didOpenDatabase(database, domain, name, version);
}
#endif

void InspectorInstrumentation::didDispatchDOMStorageEventImpl(InstrumentingAgents* instrumentingAgents, const String& key, const String& oldValue, const String& newValue, StorageType storageType, SecurityOrigin* securityOrigin, Page* page)
{
    if (InspectorDOMStorageAgent* domStorageAgent = instrumentingAgents->inspectorDOMStorageAgent())
        domStorageAgent->didDispatchDOMStorageEvent(key, oldValue, newValue, storageType, securityOrigin, page);
}

bool InspectorInstrumentation::shouldPauseDedicatedWorkerOnStartImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        return workerAgent->shouldPauseDedicatedWorkerOnStart();
    return false;
}

void InspectorInstrumentation::didStartWorkerGlobalScopeImpl(InstrumentingAgents* instrumentingAgents, WorkerGlobalScopeProxy* workerGlobalScopeProxy, const URL& url)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        workerAgent->didStartWorkerGlobalScope(workerGlobalScopeProxy, url);
}

void InspectorInstrumentation::willEvaluateWorkerScript(WorkerGlobalScope* workerGlobalScope, int workerThreadStartMode)
{
    if (workerThreadStartMode != PauseWorkerGlobalScopeOnStart)
        return;
    InstrumentingAgents* instrumentingAgents = instrumentationForWorkerGlobalScope(workerGlobalScope);
    if (!instrumentingAgents)
        return;
    if (WorkerRuntimeAgent* runtimeAgent = instrumentingAgents->workerRuntimeAgent())
        runtimeAgent->pauseWorkerGlobalScope(workerGlobalScope);
}

void InspectorInstrumentation::workerGlobalScopeTerminatedImpl(InstrumentingAgents* instrumentingAgents, WorkerGlobalScopeProxy* proxy)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        workerAgent->workerGlobalScopeTerminated(proxy);
}

#if ENABLE(WEB_SOCKETS)
void InspectorInstrumentation::didCreateWebSocketImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const URL& requestURL, const URL&, const String& protocol, Document* document)
{
    if (!instrumentingAgents->inspectorEnvironment().developerExtrasEnabled())
        return;
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didCreateWebSocket(identifier, requestURL);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didCreateWebSocket(identifier, requestURL, protocol, document->frame());
}

void InspectorInstrumentation::willSendWebSocketHandshakeRequestImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const ResourceRequest& request, Document* document)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willSendWebSocketHandshakeRequest(identifier, request);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willSendWebSocketHandshakeRequest(identifier, document->frame());
}

void InspectorInstrumentation::didReceiveWebSocketHandshakeResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const ResourceResponse& response, Document* document)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketHandshakeResponse(identifier, response);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didReceiveWebSocketHandshakeResponse(identifier, document->frame());
}

void InspectorInstrumentation::didCloseWebSocketImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, Document* document)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didCloseWebSocket(identifier);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didDestroyWebSocket(identifier, document->frame());
}

void InspectorInstrumentation::didReceiveWebSocketFrameImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const WebSocketFrame& frame)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketFrame(identifier, frame);
}
void InspectorInstrumentation::didReceiveWebSocketFrameErrorImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const String& errorMessage)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketFrameError(identifier, errorMessage);
}
void InspectorInstrumentation::didSendWebSocketFrameImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const WebSocketFrame& frame)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didSendWebSocketFrame(identifier, frame);
}
#endif

#if ENABLE(WEB_REPLAY)
void InspectorInstrumentation::sessionCreatedImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<ReplaySession> session)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->sessionCreated(session);
}

void InspectorInstrumentation::sessionLoadedImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<ReplaySession> session)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->sessionLoaded(session);
}

void InspectorInstrumentation::sessionModifiedImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<ReplaySession> session)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->sessionModified(session);
}

void InspectorInstrumentation::segmentCreatedImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<ReplaySessionSegment> segment)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->segmentCreated(segment);
}

void InspectorInstrumentation::segmentCompletedImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<ReplaySessionSegment> segment)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->segmentCompleted(segment);
}

void InspectorInstrumentation::segmentLoadedImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<ReplaySessionSegment> segment)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->segmentLoaded(segment);
}

void InspectorInstrumentation::segmentUnloadedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->segmentUnloaded();
}

void InspectorInstrumentation::captureStartedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->captureStarted();
}

void InspectorInstrumentation::captureStoppedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->captureStopped();
}

void InspectorInstrumentation::playbackStartedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->playbackStarted();
}

void InspectorInstrumentation::playbackPausedImpl(InstrumentingAgents* instrumentingAgents, const ReplayPosition& position)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->playbackPaused(position);
}

void InspectorInstrumentation::playbackHitPositionImpl(InstrumentingAgents* instrumentingAgents, const ReplayPosition& position)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->playbackHitPosition(position);
}

void InspectorInstrumentation::playbackFinishedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorReplayAgent* replayAgent = instrumentingAgents->inspectorReplayAgent())
        replayAgent->playbackFinished();
}
#endif

void InspectorInstrumentation::networkStateChangedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = instrumentingAgents->inspectorApplicationCacheAgent())
        applicationCacheAgent->networkStateChanged();
}

void InspectorInstrumentation::updateApplicationCacheStatusImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = instrumentingAgents->inspectorApplicationCacheAgent())
        applicationCacheAgent->updateApplicationCacheStatus(frame);
}

bool InspectorInstrumentation::consoleAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(scriptExecutionContext);
    InspectorConsoleAgent* consoleAgent = instrumentingAgents ? instrumentingAgents->webConsoleAgent() : nullptr;
    return consoleAgent && consoleAgent->enabled();
}

bool InspectorInstrumentation::timelineAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(scriptExecutionContext);
    return instrumentingAgents && instrumentingAgents->inspectorTimelineAgent();
}

bool InspectorInstrumentation::replayAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
#if ENABLE(WEB_REPLAY)
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForContext(scriptExecutionContext);
    return instrumentingAgents && instrumentingAgents->inspectorReplayAgent();
#else
    UNUSED_PARAM(scriptExecutionContext);
    return false;
#endif
}

void InspectorInstrumentation::pauseOnNativeEventIfNeeded(InstrumentingAgents* instrumentingAgents, bool isDOMEvent, const String& eventName, bool synchronous)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->pauseOnNativeEventIfNeeded(isDOMEvent, eventName, synchronous);
}

void InspectorInstrumentation::cancelPauseOnNativeEvent(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->cancelPauseOnNextStatement();
}

void InspectorInstrumentation::didRequestAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, int callbackId, Frame* frame)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, requestAnimationFrameEventName, true);

    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didRequestAnimationFrame(callbackId, frame);
}

void InspectorInstrumentation::didCancelAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, int callbackId, Frame* frame)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, cancelAnimationFrameEventName, true);

    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didCancelAnimationFrame(callbackId, frame);
}

InspectorInstrumentationCookie InspectorInstrumentation::willFireAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, int callbackId, Frame* frame)
{
    pauseOnNativeEventIfNeeded(instrumentingAgents, false, animationFrameFiredEventName, false);

    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        timelineAgent->willFireAnimationFrame(callbackId, frame);
        timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void InspectorInstrumentation::didFireAnimationFrameImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didFireAnimationFrame();
}

void InspectorInstrumentation::registerInstrumentingAgents(InstrumentingAgents* instrumentingAgents)
{
    if (!instrumentingAgentsSet)
        instrumentingAgentsSet = new HashSet<InstrumentingAgents*>();
    instrumentingAgentsSet->add(instrumentingAgents);
}

void InspectorInstrumentation::unregisterInstrumentingAgents(InstrumentingAgents* instrumentingAgents)
{
    if (!instrumentingAgentsSet)
        return;
    instrumentingAgentsSet->remove(instrumentingAgents);
    if (instrumentingAgentsSet->isEmpty()) {
        delete instrumentingAgentsSet;
        instrumentingAgentsSet = nullptr;
    }
}

InspectorTimelineAgent* InspectorInstrumentation::retrieveTimelineAgent(const InspectorInstrumentationCookie& cookie)
{
    if (!cookie.instrumentingAgents())
        return nullptr;
    InspectorTimelineAgent* timelineAgent = cookie.instrumentingAgents()->inspectorTimelineAgent();
    if (timelineAgent && cookie.hasMatchingTimelineAgentId(timelineAgent->id()))
        return timelineAgent;
    return nullptr;
}

InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForPage(Page* page)
{
    if (!page)
        return nullptr;
    return instrumentationForPage(page);
}

InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForRenderer(RenderObject* renderer)
{
    return instrumentingAgentsForFrame(&renderer->frame());
}

InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForWorkerGlobalScope(WorkerGlobalScope* workerGlobalScope)
{
    if (!workerGlobalScope)
        return nullptr;
    return instrumentationForWorkerGlobalScope(workerGlobalScope);
}

InstrumentingAgents* InspectorInstrumentation::instrumentingAgentsForNonDocumentContext(ScriptExecutionContext* context)
{
    if (context->isWorkerGlobalScope())
        return instrumentationForWorkerGlobalScope(toWorkerGlobalScope(context));
    return nullptr;
}

void InspectorInstrumentation::layerTreeDidChangeImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->layerTreeDidChange();
}

void InspectorInstrumentation::renderLayerDestroyedImpl(InstrumentingAgents* instrumentingAgents, const RenderLayer* renderLayer)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->renderLayerDestroyed(renderLayer);
}

void InspectorInstrumentation::pseudoElementDestroyedImpl(InstrumentingAgents* instrumentingAgents, PseudoElement* pseudoElement)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->pseudoElementDestroyed(pseudoElement);
}

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
