/*
* Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef InspectorConsoleInstrumentation_h
#define InspectorConsoleInstrumentation_h

#include "InspectorInstrumentation.h"
#include "ScriptArguments.h"
#include "ScriptCallStack.h"
#include "ScriptProfile.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

inline void InspectorInstrumentation::addMessageToConsole(Page* page, MessageSource source, MessageType type, MessageLevel level, const String& message, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        addMessageToConsoleImpl(instrumentingAgents, source, type, level, message, arguments, callStack);
#endif
}

inline void InspectorInstrumentation::addMessageToConsole(Page* page, MessageSource source, MessageType type, MessageLevel level, const String& message, const String& scriptId, unsigned lineNumber)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        addMessageToConsoleImpl(instrumentingAgents, source, type, level, message, scriptId, lineNumber);
#endif
}

#if ENABLE(WORKERS)
inline void InspectorInstrumentation::addMessageToConsole(WorkerContext* workerContext, MessageSource source, MessageType type, MessageLevel level, const String& message, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForWorkerContext(workerContext))
        addMessageToConsoleImpl(instrumentingAgents, source, type, level, message, arguments, callStack);
#endif
}

inline void InspectorInstrumentation::addMessageToConsole(WorkerContext* workerContext, MessageSource source, MessageType type, MessageLevel level, const String& message, const String& scriptId, unsigned lineNumber)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForWorkerContext(workerContext))
        addMessageToConsoleImpl(instrumentingAgents, source, type, level, message, scriptId, lineNumber);
#endif
}
#endif

inline void InspectorInstrumentation::consoleCount(Page* page, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> stack)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        consoleCountImpl(instrumentingAgents, arguments, stack);
#endif
}

inline void InspectorInstrumentation::startConsoleTiming(Page* page, const String& title)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        startConsoleTimingImpl(instrumentingAgents, title);
#endif
}

inline void InspectorInstrumentation::stopConsoleTiming(Page* page, const String& title, PassRefPtr<ScriptCallStack> stack)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        stopConsoleTimingImpl(instrumentingAgents, title, stack);
#endif
}

inline void InspectorInstrumentation::consoleTimeStamp(Page* page, PassRefPtr<ScriptArguments> arguments)
{
#if ENABLE(INSPECTOR)
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        consoleTimeStampImpl(instrumentingAgents, arguments);
#endif
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
inline void InspectorInstrumentation::addStartProfilingMessageToConsole(Page* page, const String& title, unsigned lineNumber, const String& sourceURL)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        addStartProfilingMessageToConsoleImpl(instrumentingAgents, title, lineNumber, sourceURL);
#endif
}

inline void InspectorInstrumentation::addProfile(Page* page, RefPtr<ScriptProfile> profile, PassRefPtr<ScriptCallStack> callStack)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        addProfileImpl(instrumentingAgents, profile, callStack);
#endif
}

inline bool InspectorInstrumentation::profilerEnabled(Page* page)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return profilerEnabledImpl(instrumentingAgents);
#endif
    return false;
}

inline String InspectorInstrumentation::getCurrentUserInitiatedProfileName(Page* page, bool incrementProfileNumber)
{
#if ENABLE(INSPECTOR)
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        return InspectorInstrumentation::getCurrentUserInitiatedProfileNameImpl(instrumentingAgents, incrementProfileNumber);
#endif
    return "";
}
#endif

} // namespace WebCore

#endif // !defined(InspectorConsoleInstrumentation_h)
