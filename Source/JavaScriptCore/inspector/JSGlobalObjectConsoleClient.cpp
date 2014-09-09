/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSGlobalObjectConsoleClient.h"

#if ENABLE(INSPECTOR)

#include "InspectorConsoleAgent.h"
#include "ScriptArguments.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace JSC;

namespace Inspector {

static bool sLogToSystemConsole = false;

bool JSGlobalObjectConsoleClient::logToSystemConsole()
{
    return sLogToSystemConsole;
}

void JSGlobalObjectConsoleClient::setLogToSystemConsole(bool shouldLog)
{
    sLogToSystemConsole = shouldLog;
}

void JSGlobalObjectConsoleClient::initializeLogToSystemConsole()
{
#if !LOG_DISABLED
    sLogToSystemConsole = true;
#elif USE(CF)
    Boolean keyExistsAndHasValidFormat = false;
    Boolean preference = CFPreferencesGetAppBooleanValue(CFSTR("JavaScriptCoreOutputConsoleMessagesToSystemConsole"), kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);
    if (keyExistsAndHasValidFormat)
        sLogToSystemConsole = preference;
#endif
}

JSGlobalObjectConsoleClient::JSGlobalObjectConsoleClient(InspectorConsoleAgent* consoleAgent)
    : ConsoleClient()
    , m_consoleAgent(consoleAgent)
{
    static std::once_flag initializeLogging;
    std::call_once(initializeLogging, []{
        JSGlobalObjectConsoleClient::initializeLogToSystemConsole();
    });
}

void JSGlobalObjectConsoleClient::messageWithTypeAndLevel(MessageType type, MessageLevel level, JSC::ExecState* exec, PassRefPtr<ScriptArguments> prpArguments)
{
    RefPtr<ScriptArguments> arguments = prpArguments;

    if (JSGlobalObjectConsoleClient::logToSystemConsole())
        ConsoleClient::printConsoleMessageWithArguments(MessageSource::ConsoleAPI, type, level, exec, arguments);

    String message;
    arguments->getFirstArgumentAsString(message);
    m_consoleAgent->addMessageToConsole(MessageSource::ConsoleAPI, type, level, message, exec, arguments.release());
}

void JSGlobalObjectConsoleClient::count(ExecState* exec, PassRefPtr<ScriptArguments> arguments)
{
    m_consoleAgent->count(exec, arguments);
}

void JSGlobalObjectConsoleClient::profile(JSC::ExecState*, const String&)
{
    // FIXME: support |console.profile| for JSContexts. <https://webkit.org/b/136466>
}

void JSGlobalObjectConsoleClient::profileEnd(JSC::ExecState*, const String&)
{
    // FIXME: support |console.profile| for JSContexts. <https://webkit.org/b/136466>
}

void JSGlobalObjectConsoleClient::time(ExecState*, const String& title)
{
    m_consoleAgent->startTiming(title);
}

void JSGlobalObjectConsoleClient::timeEnd(ExecState* exec, const String& title)
{
    RefPtr<ScriptCallStack> callStack(createScriptCallStackForConsole(exec, 1));
    m_consoleAgent->stopTiming(title, callStack.release());
}

void JSGlobalObjectConsoleClient::timeStamp(ExecState*, PassRefPtr<ScriptArguments>)
{
    // FIXME: JSContext inspection needs a timeline.
    warnUnimplemented(ASCIILiteral("console.timeStamp"));
}

void JSGlobalObjectConsoleClient::warnUnimplemented(const String& method)
{
    String message = method + " is currently ignored in JavaScript context inspection.";
    m_consoleAgent->addMessageToConsole(MessageSource::ConsoleAPI, MessageType::Log, MessageLevel::Warning, message, nullptr, nullptr);
}

} // namespace Inspector

#endif
