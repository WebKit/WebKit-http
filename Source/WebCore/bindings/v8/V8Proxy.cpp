/*
 * Copyright (C) 2008, 2009 Google Inc. All rights reserved.
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
#include "V8Proxy.h"

#include "CSSMutableStyleDeclaration.h"
#include "CachedMetadata.h"
#include "Console.h"
#include "DateExtension.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "IDBDatabaseException.h"
#include "IDBFactoryBackendInterface.h"
#include "IDBPendingTransactionMonitor.h"
#include "InspectorInstrumentation.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformBridge.h"
#include "ScriptSourceCode.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "StorageNamespace.h"
#include "V8Binding.h"
#include "V8BindingState.h"
#include "V8Collection.h"
#include "V8DOMCoreException.h"
#include "V8DOMMap.h"
#include "V8DOMWindow.h"
#include "V8EventException.h"
#include "V8FileException.h"
#include "V8HiddenPropertyName.h"
#include "V8IsolatedContext.h"
#include "V8OperationNotAllowedException.h"
#include "V8RangeException.h"
#include "V8SQLException.h"
#include "V8XMLHttpRequestException.h"
#include "V8XPathException.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"

#if ENABLE(INDEXED_DATABASE)
#include "V8IDBDatabaseException.h"
#endif

#if ENABLE(SVG)
#include "V8SVGException.h"
#endif

#include <algorithm>
#include <stdio.h>
#include <utility>
#include <wtf/Assertions.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/OwnPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Static list of registered extensions
V8Extensions V8Proxy::m_extensions;

void batchConfigureAttributes(v8::Handle<v8::ObjectTemplate> instance, 
                              v8::Handle<v8::ObjectTemplate> proto, 
                              const BatchedAttribute* attributes, 
                              size_t attributeCount)
{
    for (size_t i = 0; i < attributeCount; ++i)
        configureAttribute(instance, proto, attributes[i]);
}

void batchConfigureCallbacks(v8::Handle<v8::ObjectTemplate> proto, 
                             v8::Handle<v8::Signature> signature, 
                             v8::PropertyAttribute attributes,
                             const BatchedCallback* callbacks,
                             size_t callbackCount)
{
    for (size_t i = 0; i < callbackCount; ++i) {
        proto->Set(v8::String::New(callbacks[i].name),
                   v8::FunctionTemplate::New(callbacks[i].callback, 
                                             v8::Handle<v8::Value>(),
                                             signature),
                   attributes);
    }
}

void batchConfigureConstants(v8::Handle<v8::FunctionTemplate> functionDescriptor,
                             v8::Handle<v8::ObjectTemplate> proto,
                             const BatchedConstant* constants,
                             size_t constantCount)
{
    for (size_t i = 0; i < constantCount; ++i) {
        const BatchedConstant* constant = &constants[i];
        functionDescriptor->Set(v8::String::New(constant->name), v8::Integer::New(constant->value), v8::ReadOnly);
        proto->Set(v8::String::New(constant->name), v8::Integer::New(constant->value), v8::ReadOnly);
    }
}

typedef HashMap<Node*, v8::Object*> DOMNodeMap;
typedef HashMap<void*, v8::Object*> DOMObjectMap;
typedef HashMap<int, v8::FunctionTemplate*> FunctionTemplateMap;

static void addMessageToConsole(Page* page, const String& message, const String& sourceID, unsigned lineNumber)
{
    ASSERT(page);
    Console* console = page->mainFrame()->domWindow()->console();
    console->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, lineNumber, sourceID);
}

void logInfo(Frame* frame, const String& message, const String& url)
{
    Page* page = frame->page();
    if (!page)
        return;
    addMessageToConsole(page, message, url, 0);
}

void V8Proxy::reportUnsafeAccessTo(Frame* target)
{
    ASSERT(target);
    Document* targetDocument = target->document();
    if (!targetDocument)
        return;

    Frame* source = V8Proxy::retrieveFrameForEnteredContext();
    if (!source)
        return;
    Page* page = source->page();
    if (!page)
        return;

    Document* sourceDocument = source->document();
    if (!sourceDocument)
        return; // Ignore error if the source document is gone.

    // FIXME: This error message should contain more specifics of why the same
    // origin check has failed.
    String str = "Unsafe JavaScript attempt to access frame with URL " + targetDocument->url().string() +
                 " from frame with URL " + sourceDocument->url().string() + ". Domains, protocols and ports must match.\n";

    // Build a console message with fake source ID and line number.
    const String kSourceID = "";
    const int kLineNumber = 1;

    // NOTE: Safari prints the message in the target page, but it seems like
    // it should be in the source page. Even for delayed messages, we put it in
    // the source page.
    addMessageToConsole(page, str, kSourceID, kLineNumber);
}

static void handleFatalErrorInV8()
{
    // FIXME: We temporarily deal with V8 internal error situations
    // such as out-of-memory by crashing the renderer.
    CRASH();
}

V8Proxy::V8Proxy(Frame* frame)
    : m_frame(frame)
    , m_windowShell(V8DOMWindowShell::create(frame))
    , m_inlineCode(false)
    , m_timerCallback(false)
    , m_recursion(0)
{
}

V8Proxy::~V8Proxy()
{
    clearForClose();
    windowShell()->destroyGlobal();
}

v8::Handle<v8::Script> V8Proxy::compileScript(v8::Handle<v8::String> code, const String& fileName, const TextPosition0& scriptStartPosition, v8::ScriptData* scriptData)
{
    const uint16_t* fileNameString = fromWebCoreString(fileName);
    v8::Handle<v8::String> name = v8::String::New(fileNameString, fileName.length());
    v8::Handle<v8::Integer> line = v8::Integer::New(scriptStartPosition.m_line.zeroBasedInt());
    v8::Handle<v8::Integer> column = v8::Integer::New(scriptStartPosition.m_column.zeroBasedInt());
    v8::ScriptOrigin origin(name, line, column);
    v8::Handle<v8::Script> script = v8::Script::Compile(code, &origin, scriptData);
    return script;
}

bool V8Proxy::handleOutOfMemory()
{
    v8::Local<v8::Context> context = v8::Context::GetCurrent();

    if (!context->HasOutOfMemoryException())
        return false;

    // Warning, error, disable JS for this frame?
    Frame* frame = V8Proxy::retrieveFrame(context);

    V8Proxy* proxy = V8Proxy::retrieve(frame);
    if (proxy) {
        // Clean m_context, and event handlers.
        proxy->clearForClose();

        proxy->windowShell()->destroyGlobal();
    }

#if PLATFORM(CHROMIUM)
    PlatformBridge::notifyJSOutOfMemory(frame);
#endif

    // Disable JS.
    Settings* settings = frame->settings();
    ASSERT(settings);
    settings->setJavaScriptEnabled(false);

    return true;
}

void V8Proxy::evaluateInIsolatedWorld(int worldID, const Vector<ScriptSourceCode>& sources, int extensionGroup)
{
    // FIXME: This will need to get reorganized once we have a windowShell for the isolated world.
    windowShell()->initContextIfNeeded();

    v8::HandleScope handleScope;
    V8IsolatedContext* isolatedContext = 0;

    if (worldID > 0) {
        IsolatedWorldMap::iterator iter = m_isolatedWorlds.find(worldID);
        if (iter != m_isolatedWorlds.end()) {
            isolatedContext = iter->second;
        } else {
            isolatedContext = new V8IsolatedContext(this, extensionGroup);
            if (isolatedContext->context().IsEmpty()) {
                delete isolatedContext;
                return;
            }

            // FIXME: We should change this to using window shells to match JSC.
            m_isolatedWorlds.set(worldID, isolatedContext);

            // Setup context id for JS debugger.
            if (!setInjectedScriptContextDebugId(isolatedContext->context())) {
                m_isolatedWorlds.take(worldID);
                delete isolatedContext;
                return;
            }
        }
        
        IsolatedWorldSecurityOriginMap::iterator securityOriginIter = m_isolatedWorldSecurityOrigins.find(worldID);
        if (securityOriginIter != m_isolatedWorldSecurityOrigins.end())
            isolatedContext->setSecurityOrigin(securityOriginIter->second);
    } else {
        isolatedContext = new V8IsolatedContext(this, extensionGroup);
        if (isolatedContext->context().IsEmpty()) {
            delete isolatedContext;
            return;
        }
    }

    v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolatedContext->context());
    v8::Context::Scope context_scope(context);
    for (size_t i = 0; i < sources.size(); ++i)
      evaluate(sources[i], 0);

    if (worldID == 0)
      isolatedContext->destroy();
}

void V8Proxy::setIsolatedWorldSecurityOrigin(int worldID, PassRefPtr<SecurityOrigin> prpSecurityOriginIn)
{
    ASSERT(worldID);
    RefPtr<SecurityOrigin> securityOrigin = prpSecurityOriginIn;
    m_isolatedWorldSecurityOrigins.set(worldID, securityOrigin);
    IsolatedWorldMap::iterator iter = m_isolatedWorlds.find(worldID);
    if (iter != m_isolatedWorlds.end())
        iter->second->setSecurityOrigin(securityOrigin);
}

bool V8Proxy::setInjectedScriptContextDebugId(v8::Handle<v8::Context> targetContext)
{
    // Setup context id for JS debugger.
    v8::Context::Scope contextScope(targetContext);
    v8::Handle<v8::Context> context = windowShell()->context();
    if (context.IsEmpty())
        return false;
    int debugId = contextDebugId(context);

    char buffer[32];
    if (debugId == -1)
        snprintf(buffer, sizeof(buffer), "injected");
    else
        snprintf(buffer, sizeof(buffer), "injected,%d", debugId);
    targetContext->SetData(v8::String::New(buffer));

    return true;
}

PassOwnPtr<v8::ScriptData> V8Proxy::precompileScript(v8::Handle<v8::String> code, CachedScript* cachedScript)
{
    // A pseudo-randomly chosen ID used to store and retrieve V8 ScriptData from
    // the CachedScript. If the format changes, this ID should be changed too.
    static const unsigned dataTypeID = 0xECC13BD7;

    // Very small scripts are not worth the effort to preparse.
    static const int minPreparseLength = 1024;

    if (!cachedScript || code->Length() < minPreparseLength)
        return nullptr;

    CachedMetadata* cachedMetadata = cachedScript->cachedMetadata(dataTypeID);
    if (cachedMetadata)
        return adoptPtr(v8::ScriptData::New(cachedMetadata->data(), cachedMetadata->size()));

    OwnPtr<v8::ScriptData> scriptData = adoptPtr(v8::ScriptData::PreCompile(code));
    cachedScript->setCachedMetadata(dataTypeID, scriptData->Data(), scriptData->Length());

    return scriptData.release();
}

bool V8Proxy::executingScript() const
{
    return m_recursion;
}

v8::Local<v8::Value> V8Proxy::evaluate(const ScriptSourceCode& source, Node* node)
{
    ASSERT(v8::Context::InContext());

    V8GCController::checkMemoryUsage();

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willEvaluateScript(m_frame, source.url().isNull() ? String() : source.url().string(), source.startLine());

    v8::Local<v8::Value> result;
    {
        // Isolate exceptions that occur when compiling and executing
        // the code. These exceptions should not interfere with
        // javascript code we might evaluate from C++ when returning
        // from here.
        v8::TryCatch tryCatch;
        tryCatch.SetVerbose(true);

        // Compile the script.
        v8::Local<v8::String> code = v8ExternalString(source.source());
#if PLATFORM(CHROMIUM)
        PlatformBridge::traceEventBegin("v8.compile", node, "");
#endif
        OwnPtr<v8::ScriptData> scriptData = precompileScript(code, source.cachedScript());

        // NOTE: For compatibility with WebCore, ScriptSourceCode's line starts at
        // 1, whereas v8 starts at 0.
        v8::Handle<v8::Script> script = compileScript(code, source.url(), WTF::toZeroBasedTextPosition(source.startPosition()), scriptData.get());
#if PLATFORM(CHROMIUM)
        PlatformBridge::traceEventEnd("v8.compile", node, "");

        PlatformBridge::traceEventBegin("v8.run", node, "");
#endif
        // Set inlineCode to true for <a href="javascript:doSomething()">
        // and false for <script>doSomething</script>. We make a rough guess at
        // this based on whether the script source has a URL.
        result = runScript(script, source.url().string().isNull());
    }
#if PLATFORM(CHROMIUM)
    PlatformBridge::traceEventEnd("v8.run", node, "");
#endif

    InspectorInstrumentation::didEvaluateScript(cookie);

    return result;
}

v8::Local<v8::Value> V8Proxy::runScript(v8::Handle<v8::Script> script, bool isInlineCode)
{
    if (script.IsEmpty())
        return notHandledByInterceptor();

    V8GCController::checkMemoryUsage();
    // Compute the source string and prevent against infinite recursion.
    if (m_recursion >= kMaxRecursionDepth) {
        v8::Local<v8::String> code = v8ExternalString("throw RangeError('Recursion too deep')");
        // FIXME: Ideally, we should be able to re-use the origin of the
        // script passed to us as the argument instead of using an empty string
        // and 0 baseLine.
        script = compileScript(code, "", TextPosition0::minimumPosition());
    }

    if (handleOutOfMemory())
        ASSERT(script.IsEmpty());

    if (script.IsEmpty())
        return notHandledByInterceptor();

    // Save the previous value of the inlineCode flag and update the flag for
    // the duration of the script invocation.
    bool previousInlineCode = inlineCode();
    setInlineCode(isInlineCode);

    // Run the script and keep track of the current recursion depth.
    v8::Local<v8::Value> result;
    v8::TryCatch tryCatch;
    tryCatch.SetVerbose(true);
    {
        // See comment in V8Proxy::callFunction.
        m_frame->keepAlive();

        m_recursion++;
        result = script->Run();
        m_recursion--;
    }

    // Release the storage mutex if applicable.
    didLeaveScriptContext();

    if (handleOutOfMemory())
        ASSERT(result.IsEmpty());

    // Handle V8 internal error situation (Out-of-memory).
    if (tryCatch.HasCaught()) {
        ASSERT(result.IsEmpty());
        return notHandledByInterceptor();
    }

    if (result.IsEmpty())
        return notHandledByInterceptor();

    // Restore inlineCode flag.
    setInlineCode(previousInlineCode);

    if (v8::V8::IsDead())
        handleFatalErrorInV8();

    return result;
}

v8::Local<v8::Value> V8Proxy::callFunction(v8::Handle<v8::Function> function, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> args[])
{
    V8GCController::checkMemoryUsage();
    v8::Local<v8::Value> result;
    {
        if (m_recursion >= kMaxRecursionDepth) {
            v8::Local<v8::String> code = v8::String::New("throw new RangeError('Maximum call stack size exceeded.')");
            if (code.IsEmpty())
                return result;
            v8::Local<v8::Script> script = v8::Script::Compile(code);
            if (script.IsEmpty())
                return result;
            script->Run();
            return result;
        }

        // Evaluating the JavaScript could cause the frame to be deallocated,
        // so we start the keep alive timer here.
        // Frame::keepAlive method adds the ref count of the frame and sets a
        // timer to decrease the ref count. It assumes that the current JavaScript
        // execution finishs before firing the timer.
        m_frame->keepAlive();

        InspectorInstrumentationCookie cookie;
        if (InspectorInstrumentation::hasFrontends()) {
            v8::ScriptOrigin origin = function->GetScriptOrigin();
            String resourceName("undefined");
            int lineNumber = 1;
            if (!origin.ResourceName().IsEmpty()) {
                resourceName = toWebCoreString(origin.ResourceName());
                lineNumber = function->GetScriptLineNumber() + 1;
            }
            cookie = InspectorInstrumentation::willCallFunction(m_frame, resourceName, lineNumber);
        }

        m_recursion++;
        result = function->Call(receiver, argc, args);
        m_recursion--;

        InspectorInstrumentation::didCallFunction(cookie);
    }

    // Release the storage mutex if applicable.
    didLeaveScriptContext();

    if (v8::V8::IsDead())
        handleFatalErrorInV8();

    return result;
}

v8::Local<v8::Value> V8Proxy::callFunctionWithoutFrame(v8::Handle<v8::Function> function, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> args[])
{
    V8GCController::checkMemoryUsage();
    v8::Local<v8::Value> result = function->Call(receiver, argc, args);

    if (v8::V8::IsDead())
        handleFatalErrorInV8();

    return result;
}

v8::Local<v8::Value> V8Proxy::newInstance(v8::Handle<v8::Function> constructor, int argc, v8::Handle<v8::Value> args[])
{
    // No artificial limitations on the depth of recursion, see comment in
    // V8Proxy::callFunction.
    v8::Local<v8::Value> result;
    {
        // See comment in V8Proxy::callFunction.
        m_frame->keepAlive();

        result = constructor->NewInstance(argc, args);
    }

    if (v8::V8::IsDead())
        handleFatalErrorInV8();

    return result;
}

DOMWindow* V8Proxy::retrieveWindow(v8::Handle<v8::Context> context)
{
    v8::Handle<v8::Object> global = context->Global();
    ASSERT(!global.IsEmpty());
    global = V8DOMWrapper::lookupDOMWrapper(V8DOMWindow::GetTemplate(), global);
    ASSERT(!global.IsEmpty());
    return V8DOMWindow::toNative(global);
}

Frame* V8Proxy::retrieveFrame(v8::Handle<v8::Context> context)
{
    DOMWindow* window = retrieveWindow(context);
    Frame* frame = window->frame();
    if (frame && frame->domWindow() == window)
        return frame;
    // We return 0 here because |context| is detached from the Frame.  If we
    // did return |frame| we could get in trouble because the frame could be
    // navigated to another security origin.
    return 0;
}

Frame* V8Proxy::retrieveFrameForEnteredContext()
{
    v8::Handle<v8::Context> context = v8::Context::GetEntered();
    if (context.IsEmpty())
        return 0;
    return retrieveFrame(context);
}

Frame* V8Proxy::retrieveFrameForCurrentContext()
{
    v8::Handle<v8::Context> context = v8::Context::GetCurrent();
    if (context.IsEmpty())
        return 0;
    return retrieveFrame(context);
}

Frame* V8Proxy::retrieveFrameForCallingContext()
{
    v8::Handle<v8::Context> context = v8::Context::GetCalling();
    if (context.IsEmpty())
        return 0;
    return retrieveFrame(context);
}

V8Proxy* V8Proxy::retrieve()
{
    DOMWindow* window = retrieveWindow(currentContext());
    ASSERT(window);
    return retrieve(window->frame());
}

V8Proxy* V8Proxy::retrieve(Frame* frame)
{
    if (!frame)
        return 0;
    return frame->script()->canExecuteScripts(NotAboutToExecuteScript) ? frame->script()->proxy() : 0;
}

V8Proxy* V8Proxy::retrieve(ScriptExecutionContext* context)
{
    if (!context || !context->isDocument())
        return 0;
    return retrieve(static_cast<Document*>(context)->frame());
}

void V8Proxy::didLeaveScriptContext()
{
    Page* page = m_frame->page();
    if (!page)
        return;
    // If we've just left a top level script context and local storage has been
    // instantiated, we must ensure that any storage locks have been freed.
    // Per http://dev.w3.org/html5/spec/Overview.html#storage-mutex
    if (m_recursion)
        return;
#if ENABLE(INDEXED_DATABASE)
    // If we've just left a script context and indexed database has been
    // instantiated, we must let its transaction coordinator know so it can terminate
    // any not-yet-started transactions.
    IDBPendingTransactionMonitor::abortPendingTransactions();
#endif // ENABLE(INDEXED_DATABASE)
    if (page->group().hasLocalStorage())
        page->group().localStorage()->unlock();
}

void V8Proxy::resetIsolatedWorlds()
{
    for (IsolatedWorldMap::iterator iter = m_isolatedWorlds.begin();
         iter != m_isolatedWorlds.end(); ++iter) {
        iter->second->destroy();
    }
    m_isolatedWorlds.clear();
    m_isolatedWorldSecurityOrigins.clear();
}

void V8Proxy::clearForClose()
{
    resetIsolatedWorlds();
    windowShell()->clearForClose();
}

void V8Proxy::clearForNavigation()
{
    resetIsolatedWorlds();
    windowShell()->clearForNavigation();
}

void V8Proxy::setDOMException(int exceptionCode)
{
    if (exceptionCode <= 0)
        return;

    ExceptionCodeDescription description;
    getExceptionCodeDescription(exceptionCode, description);

    v8::Handle<v8::Value> exception;
    switch (description.type) {
    case DOMExceptionType:
        exception = toV8(DOMCoreException::create(description));
        break;
    case RangeExceptionType:
        exception = toV8(RangeException::create(description));
        break;
    case EventExceptionType:
        exception = toV8(EventException::create(description));
        break;
    case XMLHttpRequestExceptionType:
        exception = toV8(XMLHttpRequestException::create(description));
        break;
#if ENABLE(SVG)
    case SVGExceptionType:
        exception = toV8(SVGException::create(description));
        break;
#endif
#if ENABLE(XPATH)
    case XPathExceptionType:
        exception = toV8(XPathException::create(description));
        break;
#endif
#if ENABLE(DATABASE)
    case SQLExceptionType:
        exception = toV8(SQLException::create(description));
        break;
#endif
#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
    case FileExceptionType:
        exception = toV8(FileException::create(description));
        break;
    case OperationNotAllowedExceptionType:
        exception = toV8(OperationNotAllowedException::create(description));
        break;
#endif
#if ENABLE(INDEXED_DATABASE)
    case IDBDatabaseExceptionType:
        exception = toV8(IDBDatabaseException::create(description));
        break;
#endif
    default:
        ASSERT_NOT_REACHED();
    }

    if (!exception.IsEmpty())
        v8::ThrowException(exception);
}

v8::Handle<v8::Value> V8Proxy::throwError(ErrorType type, const char* message)
{
    switch (type) {
    case RangeError:
        return v8::ThrowException(v8::Exception::RangeError(v8String(message)));
    case ReferenceError:
        return v8::ThrowException(v8::Exception::ReferenceError(v8String(message)));
    case SyntaxError:
        return v8::ThrowException(v8::Exception::SyntaxError(v8String(message)));
    case TypeError:
        return v8::ThrowException(v8::Exception::TypeError(v8String(message)));
    case GeneralError:
        return v8::ThrowException(v8::Exception::Error(v8String(message)));
    default:
        ASSERT_NOT_REACHED();
        return notHandledByInterceptor();
    }
}

v8::Handle<v8::Value> V8Proxy::throwTypeError()
{
    return throwError(TypeError, "Type error");
}

v8::Handle<v8::Value> V8Proxy::throwSyntaxError()
{
    return throwError(SyntaxError, "Syntax error");
}

v8::Local<v8::Context> V8Proxy::context(Frame* frame)
{
    v8::Local<v8::Context> context = V8Proxy::mainWorldContext(frame);
    if (context.IsEmpty())
        return v8::Local<v8::Context>();

    if (V8IsolatedContext* isolatedContext = V8IsolatedContext::getEntered()) {
        context = v8::Local<v8::Context>::New(isolatedContext->context());
        if (frame != V8Proxy::retrieveFrame(context))
            return v8::Local<v8::Context>();
    }

    return context;
}

v8::Local<v8::Context> V8Proxy::context()
{
    if (V8IsolatedContext* isolatedContext = V8IsolatedContext::getEntered()) {
        RefPtr<SharedPersistent<v8::Context> > context = isolatedContext->sharedContext();
        if (m_frame != V8Proxy::retrieveFrame(context->get()))
            return v8::Local<v8::Context>();
        return v8::Local<v8::Context>::New(context->get());
    }
    return mainWorldContext();
}

v8::Local<v8::Context> V8Proxy::mainWorldContext()
{
    windowShell()->initContextIfNeeded();
    return v8::Local<v8::Context>::New(windowShell()->context());
}

v8::Local<v8::Context> V8Proxy::mainWorldContext(Frame* frame)
{
    V8Proxy* proxy = retrieve(frame);
    if (!proxy)
        return v8::Local<v8::Context>();

    return proxy->mainWorldContext();
}

v8::Local<v8::Context> V8Proxy::currentContext()
{
    return v8::Context::GetCurrent();
}

v8::Handle<v8::Value> V8Proxy::checkNewLegal(const v8::Arguments& args)
{
    if (!AllowAllocation::current())
        return throwError(TypeError, "Illegal constructor");

    return args.This();
}

void V8Proxy::registerExtensionWithV8(v8::Extension* extension)
{
    // If the extension exists in our list, it was already registered with V8.
    if (!registeredExtensionWithV8(extension))
        v8::RegisterExtension(extension);
}

bool V8Proxy::registeredExtensionWithV8(v8::Extension* extension)
{
    for (size_t i = 0; i < m_extensions.size(); ++i) {
        if (m_extensions[i] == extension)
            return true;
    }

    return false;
}

void V8Proxy::registerExtension(v8::Extension* extension)
{
    registerExtensionWithV8(extension);
    m_extensions.append(extension);
}

bool V8Proxy::setContextDebugId(int debugId)
{
    ASSERT(debugId > 0);
    v8::HandleScope scope;
    v8::Handle<v8::Context> context = windowShell()->context();
    if (context.IsEmpty())
        return false;
    if (!context->GetData()->IsUndefined())
        return false;

    v8::Context::Scope contextScope(context);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "page,%d", debugId);
    context->SetData(v8::String::New(buffer));

    return true;
}

int V8Proxy::contextDebugId(v8::Handle<v8::Context> context)
{
    v8::HandleScope scope;
    if (!context->GetData()->IsString())
        return -1;
    v8::String::AsciiValue ascii(context->GetData());
    char* comma = strnstr(*ascii, ",", ascii.length());
    if (!comma)
        return -1;
    return atoi(comma + 1);
}

v8::Local<v8::Context> toV8Context(ScriptExecutionContext* context, const WorldContextHandle& worldContext)
{
    if (context->isDocument()) {
        if (V8Proxy* proxy = V8Proxy::retrieve(context))
            return worldContext.adjustedContext(proxy);
#if ENABLE(WORKERS)
    } else if (context->isWorkerContext()) {
        if (WorkerContextExecutionProxy* proxy = static_cast<WorkerContext*>(context)->script()->proxy())
            return proxy->context();
#endif
    }
    return v8::Local<v8::Context>();
}

}  // namespace WebCore
