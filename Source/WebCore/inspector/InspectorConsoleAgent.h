/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorConsoleAgent_h
#define InspectorConsoleAgent_h

#include "ConsoleTypes.h"
#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/StringHash.h>
#include <wtf/Vector.h>

namespace WebCore {

#if ENABLE(INSPECTOR)

class ConsoleMessage;
class DOMWindow;
class InspectorFrontend;
class InspectorState;
class InjectedScriptManager;
class InstrumentingAgents;
class ResourceError;
class ResourceResponse;
class ScriptArguments;
class ScriptCallStack;
class ScriptProfile;

typedef String ErrorString;

class InspectorConsoleAgent : public InspectorBaseAgent<InspectorConsoleAgent>, public InspectorBackendDispatcher::ConsoleCommandHandler {
    WTF_MAKE_NONCOPYABLE(InspectorConsoleAgent);
public:
    InspectorConsoleAgent(InstrumentingAgents*, InspectorState*, InjectedScriptManager*);
    virtual ~InspectorConsoleAgent();

    virtual void enable(ErrorString*);
    virtual void disable(ErrorString*);
    virtual void clearMessages(ErrorString*);
    void reset();

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void restore();

    void addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message, PassRefPtr<ScriptArguments>, PassRefPtr<ScriptCallStack>);
    void addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message, const String& scriptId, unsigned lineNumber);

    void startTiming(const String& title);
    void stopTiming(const String& title, PassRefPtr<ScriptCallStack>);
    void count(PassRefPtr<ScriptArguments>, PassRefPtr<ScriptCallStack>);

    void frameWindowDiscarded(DOMWindow*);

    void resourceRetrievedByXMLHttpRequest(unsigned long identifier, const String& url, const String& sendURL, unsigned sendLineNumber);
    void didReceiveResponse(unsigned long identifier, const ResourceResponse&);
    void didFailLoading(unsigned long identifier, const ResourceError&);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    void addProfileFinishedMessageToConsole(PassRefPtr<ScriptProfile>, unsigned lineNumber, const String& sourceURL);
    void addStartProfilingMessageToConsole(const String& title, unsigned lineNumber, const String& sourceURL);
#endif
    virtual void setMonitoringXHREnabled(ErrorString*, bool enabled);
    virtual void addInspectedNode(ErrorString*, int nodeId) = 0;
    virtual void addInspectedHeapObject(ErrorString*, int inspectedHeapObjectId);

protected:
    void addConsoleMessage(PassOwnPtr<ConsoleMessage>);

    virtual bool developerExtrasEnabled() = 0;

    InjectedScriptManager* m_injectedScriptManager;
    InspectorFrontend::Console* m_frontend;
    ConsoleMessage* m_previousMessage;
    Vector<OwnPtr<ConsoleMessage> > m_consoleMessages;
    int m_expiredConsoleMessageCount;
    HashMap<String, unsigned> m_counts;
    HashMap<String, double> m_times;
};

#endif

} // namespace WebCore

#endif // !defined(InspectorConsoleAgent_h)
