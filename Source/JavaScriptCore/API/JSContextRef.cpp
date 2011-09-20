/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
#include "JSContextRef.h"
#include "JSContextRefPrivate.h"

#include "APICast.h"
#include "InitializeThreading.h"
#include <interpreter/CallFrame.h>
#include <interpreter/Interpreter.h>
#include "JSCallbackObject.h"
#include "JSClassRef.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "UStringBuilder.h"
#include <wtf/text/StringHash.h>


#if OS(DARWIN)
#include <mach-o/dyld.h>

static const int32_t webkitFirstVersionWithConcurrentGlobalContexts = 0x2100500; // 528.5.0
#endif

using namespace JSC;

JSContextGroupRef JSContextGroupCreate()
{
    initializeThreading();
    return toRef(JSGlobalData::createContextGroup(ThreadStackTypeSmall).leakRef());
}

JSContextGroupRef JSContextGroupRetain(JSContextGroupRef group)
{
    toJS(group)->ref();
    return group;
}

void JSContextGroupRelease(JSContextGroupRef group)
{
    toJS(group)->deref();
}

JSGlobalContextRef JSGlobalContextCreate(JSClassRef globalObjectClass)
{
    initializeThreading();
#if OS(DARWIN)
    // When running on Tiger or Leopard, or if the application was linked before JSGlobalContextCreate was changed
    // to use a unique JSGlobalData, we use a shared one for compatibility.
#ifndef BUILDING_ON_LEOPARD
    if (NSVersionOfLinkTimeLibrary("JavaScriptCore") <= webkitFirstVersionWithConcurrentGlobalContexts) {
#else
    {
#endif
        JSLock lock(LockForReal);
        return JSGlobalContextCreateInGroup(toRef(&JSGlobalData::sharedInstance()), globalObjectClass);
    }
#endif // OS(DARWIN)

    return JSGlobalContextCreateInGroup(0, globalObjectClass);
}

JSGlobalContextRef JSGlobalContextCreateInGroup(JSContextGroupRef group, JSClassRef globalObjectClass)
{
    initializeThreading();

    JSLock lock(LockForReal);
    RefPtr<JSGlobalData> globalData = group ? PassRefPtr<JSGlobalData>(toJS(group)) : JSGlobalData::createContextGroup(ThreadStackTypeSmall);

    APIEntryShim entryShim(globalData.get(), false);

    globalData->makeUsableFromMultipleThreads();

    if (!globalObjectClass) {
        JSGlobalObject* globalObject = JSGlobalObject::create(*globalData, JSGlobalObject::createStructure(*globalData, jsNull()));
        return JSGlobalContextRetain(toGlobalRef(globalObject->globalExec()));
    }

    JSGlobalObject* globalObject = JSCallbackObject<JSGlobalObject>::create(*globalData, globalObjectClass, JSCallbackObject<JSGlobalObject>::createStructure(*globalData, 0, jsNull()));
    ExecState* exec = globalObject->globalExec();
    JSValue prototype = globalObjectClass->prototype(exec);
    if (!prototype)
        prototype = jsNull();
    globalObject->resetPrototype(*globalData, prototype);
    return JSGlobalContextRetain(toGlobalRef(exec));
}

JSGlobalContextRef JSGlobalContextRetain(JSGlobalContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSGlobalData& globalData = exec->globalData();
    gcProtect(exec->dynamicGlobalObject());
    globalData.ref();
    return ctx;
}

void JSGlobalContextRelease(JSGlobalContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    JSLock lock(exec);

    JSGlobalData& globalData = exec->globalData();
    JSGlobalObject* dgo = exec->dynamicGlobalObject();
    IdentifierTable* savedIdentifierTable = wtfThreadData().setCurrentIdentifierTable(globalData.identifierTable);

    // One reference is held by JSGlobalObject, another added by JSGlobalContextRetain().
    bool releasingContextGroup = globalData.refCount() == 2;
    bool releasingGlobalObject = Heap::heap(dgo)->unprotect(dgo);
    // If this is the last reference to a global data, it should also
    // be the only remaining reference to the global object too!
    ASSERT(!releasingContextGroup || releasingGlobalObject);

    // An API 'JSGlobalContextRef' retains two things - a global object and a
    // global data (or context group, in API terminology).
    // * If this is the last reference to any contexts in the given context group,
    //   call destroy on the heap (the global data is being  freed).
    // * If this was the last reference to the global object, then unprotecting
    //   it may release a lot of GC memory - tickle the activity callback to
    //   garbage collect soon.
    // * If there are more references remaining the the global object, then do nothing
    //   (specifically that is more protects, which we assume come from other JSGlobalContextRefs).
    if (releasingContextGroup) {
        globalData.clearBuiltinStructures();
        globalData.heap.destroy();
    } else if (releasingGlobalObject) {
        globalData.heap.activityCallback()->synchronize();
        (*globalData.heap.activityCallback())();
    }

    globalData.deref();

    wtfThreadData().setCurrentIdentifierTable(savedIdentifierTable);
}

JSObjectRef JSContextGetGlobalObject(JSContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    // It is necessary to call toThisObject to get the wrapper object when used with WebCore.
    return toRef(exec->lexicalGlobalObject()->toThisObject(exec));
}

JSContextGroupRef JSContextGetGroup(JSContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    return toRef(&exec->globalData());
}

JSGlobalContextRef JSContextGetGlobalContext(JSContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toGlobalRef(exec->lexicalGlobalObject()->globalExec());
}
    
JSStringRef JSContextCreateBacktrace(JSContextRef ctx, unsigned maxStackSize)
{
    ExecState* exec = toJS(ctx);
    JSLock lock(exec);

    unsigned count = 0;
    UStringBuilder builder;
    CallFrame* callFrame = exec;
    UString functionName;
    if (exec->callee()) {
        if (asObject(exec->callee())->inherits(&InternalFunction::s_info)) {
            functionName = asInternalFunction(exec->callee())->name(exec);
            builder.append("#0 ");
            builder.append(functionName);
            builder.append("() ");
            count++;
        }
    }
    while (true) {
        ASSERT(callFrame);
        int signedLineNumber;
        intptr_t sourceID;
        UString urlString;
        JSValue function;
        
        UString levelStr = UString::number(count);
        
        exec->interpreter()->retrieveLastCaller(callFrame, signedLineNumber, sourceID, urlString, function);

        if (function)
            functionName = asFunction(function)->name(exec);
        else {
            // Caller is unknown, but if frame is empty we should still add the frame, because
            // something called us, and gave us arguments.
            if (count)
                break;
        }
        unsigned lineNumber = signedLineNumber >= 0 ? signedLineNumber : 0;
        if (!builder.isEmpty())
            builder.append("\n");
        builder.append("#");
        builder.append(levelStr);
        builder.append(" ");
        builder.append(functionName);
        builder.append("() at ");
        builder.append(urlString);
        builder.append(":");
        builder.append(UString::number(lineNumber));
        if (!function || ++count == maxStackSize)
            break;
        callFrame = callFrame->callerFrame();
    }
    return OpaqueJSString::create(builder.toUString()).leakRef();
}


